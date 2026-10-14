# This file is part of PadOS.
# Copyright (c) 2026 Kurt Skauen
# SPDX-License-Identifier: Apache-2.0

"""Source this file in the existing GDB. Importing it outside GDB starts nothing."""

import os
import sys
import sysconfig

# The relocatable MinGW Python embedded in this GDB retains a build-time exec_prefix.
# Use its installed runtime paths; keep the DLL-directory handle alive for later imports.
if os.name == "nt" and "gdb" in sys.modules:
    if not hasattr(sys, "_pados_bridge_dll_directory"):
        sys._pados_bridge_dll_directory = os.add_dll_directory(os.path.join(sys.prefix, "bin"))
    extension_directory = sysconfig.get_config_var("DESTSHARED")
    if extension_directory and extension_directory not in sys.path:
        sys.path.append(extension_directory)

import contextlib
import ctypes
from ctypes import wintypes
import hmac
import json
import math
from pathlib import Path
import queue
import re
import secrets
import selectors
import socket
import stat
import threading
import time

tools_directory = str(Path(__file__).resolve().parent)
if tools_directory not in sys.path:
    sys.path.insert(0, tools_directory)
from pados_gdb_client import MAX_MESSAGE_BYTES, MAX_WAIT_SECONDS, PROTOCOL_VERSION
from pados_gdb_client import endpoint_directory, receive_message, send_message

MAX_REQUESTS = 1024
MAX_CLIENTS = 8
MAX_OUTPUT_CHARACTERS = 128 * 1024
TERMINAL_STATES = ("completed", "failed", "canceled", "expired")


def protect_directory(directory):
    """Protect discovery credentials; fail closed if access control cannot be applied."""
    directory.mkdir(parents=True, exist_ok=True, mode=0o700)
    metadata = directory.lstat()
    if directory.is_symlink() or getattr(metadata, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT:
        raise ValueError("Discovery directory must not be a link or reparse point")
    if os.name != "nt":
        if metadata.st_uid != os.getuid():
            raise PermissionError("Discovery directory belongs to another user")
        directory.chmod(0o700)
        return
    security = ctypes.WinDLL("advapi32", use_last_error=True)
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    security.GetTokenInformation.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD,
                                            ctypes.POINTER(wintypes.DWORD)]
    security.ConvertSidToStringSidW.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
    security.ConvertStringSecurityDescriptorToSecurityDescriptorW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD,
                                                                             ctypes.POINTER(ctypes.c_void_p), ctypes.c_void_p]
    security.SetFileSecurityW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, ctypes.c_void_p]
    security.GetNamedSecurityInfoW.argtypes = [wintypes.LPWSTR, ctypes.c_int, wintypes.DWORD,
                                             ctypes.POINTER(ctypes.c_void_p), ctypes.c_void_p, ctypes.c_void_p,
                                             ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
    security.EqualSid.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    kernel.LocalFree.argtypes = [ctypes.c_void_p]
    token_data = ctypes.create_string_buffer(1024)
    token_size = wintypes.DWORD()
    # Windows 10/11 process-token pseudo-handle; no real handle to leak or close.
    if not security.GetTokenInformation(wintypes.HANDLE(-4), 1, token_data, len(token_data), ctypes.byref(token_size)):
        raise ctypes.WinError(ctypes.get_last_error())
    sid_text = ctypes.c_void_p()
    descriptor = ctypes.c_void_p()
    owner_descriptor = ctypes.c_void_p()
    owner_sid = ctypes.c_void_p()
    try:
        error = security.GetNamedSecurityInfoW(str(directory), 1, 1, ctypes.byref(owner_sid), None, None, None,
                                               ctypes.byref(owner_descriptor))
        if error:
            raise ctypes.WinError(error)
        if not security.EqualSid(owner_sid, ctypes.c_void_p.from_buffer(token_data)):
            raise PermissionError("Discovery directory belongs to another user")
        if not security.ConvertSidToStringSidW(ctypes.c_void_p.from_buffer(token_data), ctypes.byref(sid_text)):
            raise ctypes.WinError(ctypes.get_last_error())
        user_sid = ctypes.wstring_at(sid_text)
        access = "D:P(A;OICI;FA;;;" + user_sid + ")(A;OICI;FA;;;SY)"
        if not security.ConvertStringSecurityDescriptorToSecurityDescriptorW(access, 1, ctypes.byref(descriptor), None):
            raise ctypes.WinError(ctypes.get_last_error())
        if not security.SetFileSecurityW(str(directory), 0x80000004, descriptor):
            raise ctypes.WinError(ctypes.get_last_error())
    finally:
        if owner_descriptor.value is not None:
            kernel.LocalFree(owner_descriptor)
        if descriptor.value is not None:
            kernel.LocalFree(descriptor)
        if sid_text.value is not None:
            kernel.LocalFree(sid_text)


class Request:
    def __init__(self, request_id, operation, arguments, deadline):
        self.m_Id = request_id
        self.m_Operation = operation
        self.m_Arguments = arguments
        self.m_Deadline = deadline
        self.m_State = "queued"
        self.m_Result = None
        self.m_Error = None
        self.m_Done = threading.Event()

    def snapshot(self):
        return {"id": self.m_Id, "state": self.m_State, "result": self.m_Result, "error": self.m_Error}


class BridgeServer:
    def __init__(self, dispatch, post_event, directory, identity, thread_class=threading.Thread, notify=None):
        self.m_Dispatch = dispatch
        self.m_PostEvent = post_event
        self.m_Notify = notify
        self.m_Directory = Path(directory)
        self.m_Identity = identity
        self.m_Token = secrets.token_hex(32)
        self.m_Session = secrets.token_hex(16)
        self.m_Path = self.m_Directory / ("gdb-" + str(os.getpid()) + ".json")
        self.m_Lock = threading.RLock()
        self.m_WorkersChanged = threading.Condition(self.m_Lock)
        self.m_Workers = set()
        self.m_Slots = threading.BoundedSemaphore(MAX_CLIENTS)
        self.m_Requests = {}
        self.m_Queue = queue.SimpleQueue()
        self.m_Scheduled = False
        self.m_Stopping = threading.Event()
        self.m_Stopped = threading.Event()
        self.m_Ready = threading.Event()
        self.m_Error = None
        self.m_Wakeup = None
        self.m_Thread = thread_class(target=self.serve, name="PadOS GDB bridge", daemon=True)

    def start(self):
        self.m_Thread.start()

    def stop(self):
        with self.m_Lock:
            self.m_Stopping.set()
            for request in self.m_Requests.values():
                if request.m_State == "queued":
                    request.m_State = "canceled"
                    request.m_Done.set()
            if self.m_Wakeup is not None:
                try:
                    self.m_Wakeup.send(b"x")  # Nonblocking wakeup only; no network waits on GDB's thread.
                except (BlockingIOError, OSError):
                    pass

    def serve(self):
        published = False
        try:
            protect_directory(self.m_Directory)
            with socket.socket() as listener:
                # Socketpair wakes select for shutdown without periodic accept polling.
                wake_reader, wake_writer = socket.socketpair()
                with wake_reader, wake_writer, selectors.DefaultSelector() as selector:
                    wake_writer.setblocking(False)
                    with self.m_Lock:
                        self.m_Wakeup = wake_writer
                    listener.bind(("127.0.0.1", 0))
                    listener.listen(MAX_CLIENTS)
                    descriptor = dict(self.m_Identity, version=PROTOCOL_VERSION, host="127.0.0.1",
                                      port=listener.getsockname()[1], pid=os.getpid(), session=self.m_Session, token=self.m_Token)
                    with self.m_Path.open("x", encoding="utf-8") as stream:
                        if os.name != "nt":
                            os.chmod(self.m_Path, 0o600)
                        json.dump(descriptor, stream)
                    published = True
                    selector.register(listener, selectors.EVENT_READ)
                    selector.register(wake_reader, selectors.EVENT_READ)
                    self.m_Ready.set()
                    if self.m_Notify is not None:
                        self.m_PostEvent(self.m_Notify)
                    while not self.m_Stopping.is_set():
                        for key, events in selector.select():
                            if key.fileobj is listener and not self.m_Stopping.is_set():
                                connection, address = listener.accept()
                                if self.m_Slots.acquire(blocking=False):
                                    # Signals are inherited blocked from the gdb.Thread listener.
                                    worker = threading.Thread(target=self.handle_client, args=(connection,), daemon=True)
                                    with self.m_Lock:
                                        self.m_Workers.add(worker)
                                    try:
                                        worker.start()
                                    except Exception:
                                        with self.m_Lock:
                                            self.m_Workers.remove(worker)
                                        self.m_Slots.release()
                                        connection.close()
                                        raise
                                else:
                                    connection.close()
        except Exception as error:
            self.m_Error = str(error)
        finally:
            self.stop()
            with self.m_WorkersChanged:
                self.m_Wakeup = None
                self.m_WorkersChanged.wait_for(lambda: not self.m_Workers)
            try:
                if published:
                    self.m_Path.unlink(missing_ok=True)
            except OSError as error:
                self.m_Error = str(error)
            finally:
                self.m_Ready.set()
                self.m_Stopped.set()
                if self.m_Notify is not None:
                    try:
                        self.m_PostEvent(self.m_Notify)
                    except Exception:
                        pass  # GDB may already be exiting; never print from a transport thread.

    def handle_client(self, connection):
        try:
            with connection:
                connection.settimeout(5.0)
                message = receive_message(connection)
                if (message.get("version") != PROTOCOL_VERSION or message.get("session") != self.m_Session
                        or not isinstance(message.get("token"), str) or not hmac.compare_digest(message["token"], self.m_Token)):
                    raise PermissionError("Invalid bridge credentials or session")
                response = self.handle_message(message)
                connection.settimeout(5.0)
                try:
                    send_message(connection, response)
                except ValueError:
                    send_message(connection, {"id": message.get("id"), "state": "failed", "error": "Result exceeds size limit"})
        except Exception:
            pass  # Broken/unauthenticated clients never affect accepted requests or write to GDB streams.
        finally:
            with self.m_WorkersChanged:
                self.m_Workers.remove(threading.current_thread())
                self.m_Slots.release()
                self.m_WorkersChanged.notify_all()

    def handle_message(self, message):
        request_id = message.get("id")
        if not isinstance(request_id, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,80}", request_id):
            raise ValueError("Invalid request ID")
        wait_seconds = float(message.get("wait", 0))
        if not math.isfinite(wait_seconds) or not 0 <= wait_seconds <= MAX_WAIT_SECONDS:
            raise ValueError("Invalid wait")
        with self.m_Lock:
            action = message.get("action")
            if action == "ping":
                return {"id": request_id, "state": "completed", "session": self.m_Session}
            request = self.m_Requests.get(request_id)
            if action == "request":
                if request is not None:
                    return {"id": request_id, "state": "rejected", "error": "Duplicate ID; use result, never resubmit"}
                if self.m_Stopping.is_set() or len(self.m_Requests) >= MAX_REQUESTS:
                    return {"id": request_id, "state": "rejected",
                            "error": "Bridge stopping or history full; reload after shutdown"}
                queue_seconds = float(message.get("queue_seconds", 5))
                operation = message.get("operation")
                parameters = message.get("arguments", {})
                if (not math.isfinite(queue_seconds) or not 0 < queue_seconds <= MAX_WAIT_SECONDS
                        or not isinstance(operation, str) or not isinstance(parameters, dict)):
                    raise ValueError("Invalid operation, arguments or queue deadline")
                request = Request(request_id, operation, parameters, time.monotonic() + queue_seconds)
                self.m_Requests[request_id] = request
                self.m_Queue.put(request)
                if not self.m_Scheduled:
                    self.m_Scheduled = True
                    try:
                        self.m_PostEvent(self.dispatch_one)
                    except Exception as error:
                        self.m_Scheduled = False
                        request.m_State = "failed"
                        request.m_Error = "Could not schedule GDB operation: " + str(error)
                        request.m_Done.set()
            elif action not in ("result", "cancel"):
                raise ValueError("Unknown transport action")
            if request is None:
                return {"id": request_id, "state": "unknown", "error": "ID not recorded in this session; do not resubmit"}
            if action == "cancel":
                if request.m_State == "queued":
                    request.m_State = "canceled"
                    request.m_Done.set()
                return request.snapshot() | {"canceled": request.m_State == "canceled"}
        request.m_Done.wait(wait_seconds)
        with self.m_Lock:
            return request.snapshot()

    def dispatch_one(self):
        with self.m_Lock:
            request = self.m_Queue.get_nowait()
            if request.m_State == "queued" and time.monotonic() >= request.m_Deadline:
                request.m_State = "expired"
                request.m_Done.set()
            execute = request.m_State == "queued"
            if execute:
                request.m_State = "executing"
        if execute:
            try:
                result = self.m_Dispatch(request.m_Operation, request.m_Arguments)
                # Validate serialization before declaring completion. Do not silently lose a result.
                if len(json.dumps(result, ensure_ascii=True).encode("utf-8")) > MAX_MESSAGE_BYTES - 1024:
                    raise ValueError("Result exceeds size limit; narrow the request")
                with self.m_Lock:
                    request.m_Result = result
                    request.m_State = "completed"
            except BaseException as error:
                with self.m_Lock:
                    request.m_Error = type(error).__name__ + ": " + str(error)
                    request.m_State = "failed"
            finally:
                request.m_Done.set()
        with self.m_Lock:
            if self.m_Queue.empty():
                self.m_Scheduled = False
            else:
                self.m_PostEvent(self.dispatch_one)


def single_line(value):
    if not isinstance(value, str) or not value or "\n" in value or "\r" in value or "\x00" in value:
        raise ValueError("Expected nonempty single-line text")
    return value


class GdbOperations:
    def __init__(self, debugger, owned_breakpoints):
        self.m_Gdb = debugger
        self.m_OwnedBreakpoints = owned_breakpoints
        self.m_Server = None

    def status(self):
        debugger = self.m_Gdb
        threads = debugger.selected_inferior().threads()
        selected = debugger.selected_thread()
        return {"gdb": debugger.VERSION, "program": debugger.current_progspace().filename,
                "running": any(thread.is_running() for thread in threads), "connected": bool(threads),
                "thread": selected.global_num if selected is not None else None,
                "mi_async": debugger.parameter("mi-async"), "non_stop": debugger.parameter("non-stop"),
                "endpoint": str(self.m_Server.m_Path), "session": self.m_Server.m_Session}

    @contextlib.contextmanager
    def selection(self, parameters):
        debugger = self.m_Gdb
        original_thread = debugger.selected_thread()
        original_frame = debugger.selected_frame() if original_thread is not None else None
        try:
            self.select(parameters)
            yield
        finally:
            if original_thread is not None and original_thread.is_valid() and original_thread.is_stopped():
                original_thread.switch()
                if original_frame is not None and original_frame.is_valid():
                    original_frame.select()

    def select(self, parameters):
        if "thread" in parameters:
            thread_number = int(parameters["thread"])
            matches = [thread for thread in self.m_Gdb.selected_inferior().threads() if thread.global_num == thread_number]
            if len(matches) != 1:
                raise ValueError("Unknown GDB global thread number")
            matches[0].switch()
        if "frame" in parameters:
            frame_number = int(parameters["frame"])
            if not 0 <= frame_number <= 256:
                raise ValueError("Frame must be between 0 and 256")
            frame = self.m_Gdb.newest_frame()
            for index in range(frame_number):
                frame = frame.older() if frame is not None else None
            if frame is None:
                raise ValueError("Frame does not exist")
            frame.select()

    def text(self, command):
        # post_event runs under the MI interpreter. Switch only this inspection to console
        # so captured output is plain text, not MI ~"..." stream records.
        output = self.m_Gdb.execute("interpreter-exec console " + json.dumps(command), from_tty=False, to_string=True)
        return {"text": output[:MAX_OUTPUT_CHARACTERS], "truncated": len(output) > MAX_OUTPUT_CHARACTERS}

    def inspect(self, operation, parameters):
        debugger = self.m_Gdb
        with self.selection(parameters), contextlib.ExitStack() as guards:
            for setting in ("may-call-functions", "may-write-memory", "may-write-registers"):
                guards.enter_context(debugger.with_parameter(setting, False))
            if operation == "evaluate":
                value = debugger.parse_and_eval(single_line(parameters["expression"]))
                return {"value": str(value), "type": str(value.type)}
            if operation == "type":
                return self.text("ptype " + single_line(parameters["expression"]))
            if operation == "locals":
                return {"arguments": self.text("info args"), "locals": self.text("info locals")}
            if operation == "registers":
                names = parameters.get("names", [])
                if not isinstance(names, list) or any(not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", name) for name in names):
                    raise ValueError("Invalid register names")
                return self.text("info registers " + " ".join(names))
            count = int(parameters.get("count", 32))
            if not 1 <= count <= 256:
                raise ValueError("Count must be between 1 and 256")
            if operation == "stack":
                return self.text("backtrace " + str(count))
            if operation == "disassemble":
                address = int(debugger.parse_and_eval(single_line(parameters.get("address", "$pc"))))
                return debugger.selected_frame().architecture().disassemble(address, count=count)
        raise ValueError("Unknown inspection operation")

    def breakpoints(self):
        debugger = self.m_Gdb
        self.m_OwnedBreakpoints.intersection_update(breakpoint.number for breakpoint in debugger.breakpoints())
        return {"owned": sorted(self.m_OwnedBreakpoints), "listing": self.text("info breakpoints")}

    def change_breakpoint(self, operation, parameters):
        debugger = self.m_Gdb
        if operation in ("break", "watch"):
            if operation == "break":
                command = "hbreak " if parameters.get("hardware", True) else "break "
                if parameters.get("temporary", False):
                    command = "t" + command
                command += single_line(parameters["location"])
            else:
                command = {"write": "watch ", "read": "rwatch ", "access": "awatch "}[parameters.get("access", "write")]
                command += single_line(parameters["expression"])
            before = {breakpoint.number for breakpoint in debugger.breakpoints()}
            try:
                debugger.execute(command, from_tty=False, to_string=False)
            finally:
                created = {breakpoint.number for breakpoint in debugger.breakpoints()} - before
                self.m_OwnedBreakpoints.update(created)
            return {"created": sorted(created), "breakpoints": self.breakpoints()}
        number = int(parameters["number"])
        self.breakpoints()
        if number not in self.m_OwnedBreakpoints:
            raise ValueError("Only bridge-owned breakpoints can be modified")
        command = {"delete": "delete", "enable": "enable", "disable": "disable", "condition": "condition"}[operation]
        command += " " + str(number)
        if operation == "condition" and parameters.get("expression"):
            command += " " + single_line(parameters["expression"])
        debugger.execute(command, from_tty=False, to_string=False)
        return self.breakpoints()

    def dispatch(self, operation, parameters):
        if operation == "shutdown":
            self.m_Server.stop()
            return {"transport": "stopping", "target": "unchanged", "breakpoints": "retained"}
        state = self.status()
        if operation == "status":
            return state
        if operation == "threads":
            return self.text("info threads")
        if operation == "breakpoints":
            return self.breakpoints()
        if operation == "halt":
            if state["running"]:
                self.m_Gdb.execute("interrupt -a", from_tty=False, to_string=False)
            return {"execution": "interrupt requested" if state["running"] else "already stopped"}
        if state["running"]:
            raise ValueError("Target must be stopped; bridge never halts implicitly")
        if operation in ("break", "watch", "delete", "enable", "disable", "condition"):
            return self.change_breakpoint(operation, parameters)
        if operation in ("continue", "step", "next", "stepi", "nexti", "finish"):
            if not state["connected"] or state["non_stop"]:
                raise ValueError("Execution requires a connected all-stop target")
            self.m_Gdb.execute(operation + " &", from_tty=False, to_string=False)
            return {"execution": "accepted", "operation": operation, "note": "Command acceptance is not a subsequent stop"}
        if operation == "select":
            self.select(parameters)
            return self.status()
        if operation in ("evaluate", "type", "locals", "registers", "stack", "disassemble"):
            return self.inspect(operation, parameters)
        raise ValueError("Unknown bridge operation: " + operation)


def start_bridge(debugger):
    existing = getattr(debugger, "_pados_bridge", None)
    if existing is not None and not existing.m_Stopped.is_set():
        debugger.write("PadOS bridge " + ("stopping" if existing.m_Stopping.is_set() else "already started") + "\n")
        return
    if not hasattr(debugger, "_pados_bridge_owned_breakpoints"):
        debugger._pados_bridge_owned_breakpoints = set()
    operations = GdbOperations(debugger, debugger._pados_bridge_owned_breakpoints)
    identity = {"program": debugger.current_progspace().filename, "cwd": os.getcwd()}

    def report_lifecycle():
        state = "stopped: " if server.m_Stopped.is_set() else "ready: "
        message = "failed: " + server.m_Error if server.m_Error is not None else state + str(server.m_Path)
        debugger.write("PadOS bridge " + message + "\n")

    server = BridgeServer(
        operations.dispatch,
        debugger.post_event,
        endpoint_directory(),
        identity,
        debugger.Thread,
        report_lifecycle)
    operations.m_Server = server
    debugger._pados_bridge = server
    if not hasattr(debugger, "_pados_bridge_command"):
        class BridgeCommand(debugger.Command):
            def __init__(self):
                super().__init__("pados-bridge", debugger.COMMAND_MAINTENANCE)

            def invoke(self, argument, from_tty):
                active = debugger._pados_bridge
                if argument.strip() == "stop":
                    active.stop()
                elif argument.strip() not in ("", "status"):
                    raise debugger.GdbError("Usage: pados-bridge [status|stop]")
                debugger.write(json.dumps({"endpoint": str(active.m_Path), "ready": active.m_Ready.is_set(),
                                           "stopping": active.m_Stopping.is_set(), "stopped": active.m_Stopped.is_set(),
                                           "error": active.m_Error}) + "\n")
        debugger._pados_bridge_command = BridgeCommand()
        debugger.events.gdb_exiting.connect(lambda event: debugger._pados_bridge.stop())
    server.start()
    debugger.write("PadOS bridge starting: " + str(server.m_Path) + "; use pados-bridge status to check startup\n")


if __name__ == "__main__":
    import gdb
    start_bridge(gdb)
