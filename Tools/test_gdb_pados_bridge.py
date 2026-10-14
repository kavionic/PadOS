# This file is part of PadOS.
# Copyright (c) 2026 Kurt Skauen
# SPDX-License-Identifier: Apache-2.0

"""Host-only transport tests. No debugger, probe or target connection is opened."""

import json
import os
from pathlib import Path
import queue
import socket
import subprocess
import tempfile
import threading
import time
import unittest

from gdb_pados_bridge import BridgeServer
from pados_gdb_client import discover, exchange, receive_message, send_message


class BridgeTransportTests(unittest.TestCase):
    def setUp(self):
        self.m_Temporary = tempfile.TemporaryDirectory(prefix="pados-bridge-test-")
        self.m_Events = queue.Queue()
        self.m_Calls = []
        self.m_Server = BridgeServer(self.dispatch, self.m_Events.put, self.m_Temporary.name, {"program": "test-only"})
        self.m_Server.start()
        self.assertTrue(self.m_Server.m_Ready.wait(5))
        self.assertIsNone(self.m_Server.m_Error)
        self.m_Endpoint = self.m_Server.m_Path

    def tearDown(self):
        self.m_Server.stop()
        self.assertTrue(self.m_Server.m_Stopped.wait(6), "Transport left workers behind")
        self.m_Server.m_Thread.join(1)
        self.assertFalse(self.m_Server.m_Thread.is_alive())
        self.m_Temporary.cleanup()

    def dispatch(self, operation, arguments):
        self.m_Calls.append((operation, threading.get_ident()))
        if operation == "error":
            raise RuntimeError("expected dispatch failure")
        return arguments

    def message(self, action, request_id="test", **arguments):
        return exchange(self.m_Endpoint, {"action": action, "id": request_id, "wait": 0, **arguments}, 2)

    def submit(self, request_id="test", operation="inspect", **arguments):
        return self.message("request", request_id, operation=operation, arguments=arguments)

    def run_event(self):
        self.m_Events.get(timeout=2)()

    def test_discovery_hides_credentials(self):
        sessions = discover(self.m_Temporary.name)
        self.assertEqual(len(sessions), 1)
        self.assertNotIn("token", sessions[0])
        self.assertEqual(sessions[0]["host"], "127.0.0.1")

    def test_dispatch_runs_only_on_scheduler_thread_and_preserves_order(self):
        self.assertEqual(self.submit("first", value="UTF-8: \u00e6\u00f8\u00e5")["state"], "queued")
        self.assertEqual(self.submit("second")["state"], "queued")
        self.assertEqual(self.m_Calls, [])
        self.run_event()
        self.run_event()
        self.assertEqual([call[1] for call in self.m_Calls], [threading.get_ident()] * 2)
        result = self.message("result", "first")
        self.assertEqual(result["state"], "completed")
        self.assertEqual(result["result"], {"value": "UTF-8: \u00e6\u00f8\u00e5"})

    def test_cancel_queued_prevents_execution(self):
        self.submit()
        self.assertTrue(self.message("cancel")["canceled"])
        self.run_event()
        self.assertEqual(self.m_Calls, [])
        self.assertEqual(self.message("result")["state"], "canceled")

    def test_expired_queue_deadline_prevents_execution(self):
        self.submit()
        with self.m_Server.m_Lock:
            self.m_Server.m_Requests["test"].m_Deadline = 0
        self.run_event()
        self.assertEqual(self.message("result")["state"], "expired")
        self.assertEqual(self.m_Calls, [])

    def test_executing_timeout_is_not_cancellation(self):
        entered = threading.Event()
        release = threading.Event()

        def blocking_dispatch(operation, arguments):
            entered.set()
            if not release.wait(5):
                raise RuntimeError("Test did not release dispatcher")
            return {"finished": True}

        self.m_Server.m_Dispatch = blocking_dispatch
        self.submit()
        simulated_gdb = threading.Thread(target=self.run_event)
        simulated_gdb.start()
        try:
            self.assertTrue(entered.wait(2))
            reply = self.message("cancel")
            self.assertEqual(reply["state"], "executing")
            self.assertFalse(reply["canceled"])
            self.assertEqual(self.message("result")["state"], "executing")
        finally:
            release.set()
            simulated_gdb.join(3)
        self.assertEqual(self.message("result")["state"], "completed")

    def test_duplicate_never_repeats_and_unknown_is_not_canceled(self):
        self.submit()
        self.run_event()
        self.assertEqual(self.submit()["state"], "rejected")
        self.assertEqual(len(self.m_Calls), 1)
        self.assertEqual(self.message("cancel", "missing")["state"], "unknown")

    def test_failure_is_recorded_and_next_request_still_runs(self):
        self.submit(operation="error")
        self.submit("next")
        self.run_event()
        self.run_event()
        self.assertIn("expected dispatch failure", self.message("result")["error"])
        self.assertEqual(self.message("result", "next")["state"], "completed")

    def test_invalid_authentication_cannot_schedule(self):
        descriptor = json.loads(self.m_Endpoint.read_text())
        with socket.create_connection(("127.0.0.1", descriptor["port"]), timeout=2) as connection:
            send_message(connection, {"version": 1, "session": descriptor["session"], "token": "incorrect",
                                      "action": "request", "id": "bad", "operation": "inspect"})
            self.assertRaises((ConnectionError, OSError), receive_message, connection)
        self.assertTrue(self.m_Events.empty())

    def test_disconnected_client_can_retrieve_without_resubmitting(self):
        descriptor = json.loads(self.m_Endpoint.read_text())
        with socket.create_connection(("127.0.0.1", descriptor["port"]), timeout=2) as connection:
            send_message(connection, {"version": 1, "session": descriptor["session"], "token": descriptor["token"],
                                      "action": "request", "id": "lost", "operation": "inspect", "arguments": {}, "wait": 0})
            callback = self.m_Events.get(timeout=2)
        callback()
        self.assertEqual(self.message("result", "lost")["state"], "completed")
        self.assertEqual(len(self.m_Calls), 1)

    def test_stop_cancels_queue_and_removes_descriptor(self):
        self.submit()
        self.m_Server.stop()
        self.run_event()
        self.assertTrue(self.m_Server.m_Stopped.wait(3))
        self.assertFalse(self.m_Endpoint.exists())
        self.assertEqual(self.m_Calls, [])


@unittest.skipUnless(os.environ.get("PADOS_TEST_GDB"), "Set PADOS_TEST_GDB to test an unconnected GDB/MI process")
class OfflineGdbTests(unittest.TestCase):
    def setUp(self):
        self.m_Temporary = tempfile.TemporaryDirectory(prefix="pados-offline-gdb-")
        environment = dict(os.environ, LOCALAPPDATA=self.m_Temporary.name)
        self.m_Process = subprocess.Popen(
            [os.environ["PADOS_TEST_GDB"], "-nx", "-nh", "-q", "--interpreter=mi2"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            env=environment)
        self.m_Lines = queue.Queue()
        self.m_Reader = threading.Thread(target=self.read_output, daemon=True)
        self.m_Reader.start()
        self.m_CommandNumber = 0
        self.m_Endpoint = Path(self.m_Temporary.name) / "PadOS" / "GDBBridge" / ("gdb-" + str(self.m_Process.pid) + ".json")
        self.wait_for(lambda line: line == "(gdb)")

    def tearDown(self):
        self.m_Process.stdin.write("999-gdb-exit\n")
        self.m_Process.stdin.flush()
        self.m_Process.wait(timeout=10)
        self.m_Reader.join(2)
        self.m_Process.stdin.close()
        self.m_Process.stdout.close()
        self.m_Temporary.cleanup()

    def read_output(self):
        for line in self.m_Process.stdout:
            self.m_Lines.put(line.rstrip())

    def wait_for(self, predicate):
        deadline = time.monotonic() + 10
        output = []
        for unused in range(10000):
            line = self.m_Lines.get(timeout=max(0.001, deadline - time.monotonic()))
            output.append(line)
            if predicate(line):
                return output
        self.fail("Unexpected GDB output flood")

    def console(self, command):
        self.m_CommandNumber += 1
        token = str(self.m_CommandNumber)
        self.m_Process.stdin.write(token + "-interpreter-exec console " + json.dumps(command) + "\n")
        self.m_Process.stdin.flush()
        lines = self.wait_for(lambda line: line.startswith(token + "^"))
        self.assertEqual(lines[-1], token + "^done", lines)
        return lines

    def request(self, request_id, operation, **arguments):
        reply = exchange(self.m_Endpoint, {"action": "request", "id": request_id, "operation": operation,
                                          "arguments": arguments, "wait": 3}, 4)
        self.assertEqual(reply["state"], "completed", reply)
        return reply["result"]

    def test_loading_inspection_mi_notifications_and_reload_without_target(self):
        source = "source " + str(Path(__file__).with_name("gdb_pados_bridge.py").resolve()).replace("\\", "/")
        self.console(source)
        self.wait_for(lambda line: "PadOS bridge ready:" in line)
        first_session = json.loads(self.m_Endpoint.read_text())["session"]
        self.assertFalse(self.request("status", "status")["connected"])
        self.assertEqual(self.request("value", "evaluate", expression="sizeof(void*)")["value"], "4")
        self.assertEqual(self.request("type", "type", expression="int")["text"], "type = int\n")
        self.console(source)
        self.assertEqual(json.loads(self.m_Endpoint.read_text())["session"], first_session)
        breakpoint = self.request("create", "break", location="*0x08000000", hardware=False)["created"][0]
        self.wait_for(lambda line: line.startswith("=breakpoint-created,"))
        self.request("disable", "disable", number=breakpoint)
        self.wait_for(lambda line: line.startswith("=breakpoint-modified,"))
        self.request("delete", "delete", number=breakpoint)
        self.wait_for(lambda line: line.startswith("=breakpoint-deleted,"))
        self.assertEqual(self.request("list", "breakpoints")["owned"], [])
        self.request("stop", "shutdown")
        self.wait_for(lambda line: "PadOS bridge stopped:" in line)
        self.assertFalse(self.m_Endpoint.exists())
        self.console(source)
        self.wait_for(lambda line: "PadOS bridge ready:" in line)
        self.assertNotEqual(json.loads(self.m_Endpoint.read_text())["session"], first_session)
        self.request("stop", "shutdown")
        self.wait_for(lambda line: "PadOS bridge stopped:" in line)


if __name__ == "__main__":
    unittest.main()
