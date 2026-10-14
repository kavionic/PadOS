# This file is part of PadOS.
# Copyright (c) 2026 Kurt Skauen
# SPDX-License-Identifier: Apache-2.0

"""Client and JSON framing for the local bridge inside Visual Studio's GDB."""

import argparse
import json
import os
from pathlib import Path
import socket
import struct
import sys
import time
import uuid

PROTOCOL_VERSION = 1
MAX_MESSAGE_BYTES = 2 * 1024 * 1024
MAX_WAIT_SECONDS = 30.0


def endpoint_directory():
    return Path(os.environ.get("LOCALAPPDATA", Path.home() / ".local" / "state")) / "PadOS" / "GDBBridge"


def receive_exact(connection, length, deadline):
    result = bytearray()
    while len(result) < length:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("Bridge response deadline elapsed")
        connection.settimeout(remaining)
        fragment = connection.recv(length - len(result))
        if not fragment:
            raise ConnectionError("Bridge connection closed")
        result.extend(fragment)
    return bytes(result)


def receive_message(connection, deadline=None):
    if deadline is None:
        deadline = time.monotonic() + 5.0
    length = struct.unpack("!I", receive_exact(connection, 4, deadline))[0]
    if length > MAX_MESSAGE_BYTES:
        raise ValueError("Message exceeds bridge size limit")
    message = json.loads(receive_exact(connection, length, deadline).decode("utf-8"))
    if not isinstance(message, dict):
        raise ValueError("Expected a JSON object")
    return message


def send_message(connection, message):
    data = json.dumps(message, ensure_ascii=True, allow_nan=False).encode("utf-8")
    if len(data) > MAX_MESSAGE_BYTES:
        raise ValueError("Message exceeds bridge size limit")
    connection.sendall(struct.pack("!I", len(data)) + data)


def discover(directory=None):
    sessions = []
    for path in sorted(Path(directory or endpoint_directory()).glob("gdb-*.json")):
        try:
            descriptor = json.loads(path.read_text(encoding="utf-8"))
            session = {key: value for key, value in descriptor.items() if key != "token"} | {"endpoint": str(path)}
            try:
                response = exchange(path, {"action": "ping", "id": "discovery", "wait": 0}, 0.5)
                session["reachable"] = response.get("state") == "completed"
            except (OSError, ValueError, KeyError, ConnectionError):
                session["reachable"] = False
            sessions.append(session)
        except (OSError, ValueError, AttributeError):
            continue
    return sessions


def exchange(endpoint, message, timeout):
    deadline = time.monotonic() + timeout
    descriptor = json.loads(Path(endpoint).read_text(encoding="utf-8"))
    if descriptor.get("version") != PROTOCOL_VERSION or descriptor.get("host") != "127.0.0.1":
        raise ValueError("Unsupported or nonlocal endpoint descriptor")
    message = dict(message, token=descriptor["token"], session=descriptor["session"], version=PROTOCOL_VERSION)
    with socket.create_connection(("127.0.0.1", descriptor["port"]), timeout=timeout) as connection:
        connection.settimeout(max(0.001, deadline - time.monotonic()))
        send_message(connection, message)
        return receive_message(connection, deadline)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--endpoint", help="Discovery JSON file; required when more than one session exists")
    parser.add_argument("--timeout", type=float, default=5.0, help="Bounded client wait in seconds (0 < timeout <= 30)")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("discover", help="List descriptor paths and session identities without exposing tokens")
    request_parser = commands.add_parser("request", help="Submit exactly once; never retry an uncertain command")
    request_parser.add_argument("operation")
    request_parser.add_argument("arguments", nargs="?", default="{}", help="JSON object of operation arguments")
    request_parser.add_argument("--id", default=None, help="Optional unique request ID")
    request_parser.add_argument("--queue-seconds", type=float, default=5.0, help="Deadline to start, not an execution timeout")
    for command in ("result", "cancel"):
        commands.add_parser(command).add_argument("id")
    arguments = parser.parse_args()
    if not 0 < arguments.timeout <= MAX_WAIT_SECONDS:
        parser.error("timeout must be greater than zero and at most 30 seconds")
    if arguments.command == "discover":
        print(json.dumps(discover(), indent=2))
        return 0
    endpoint = arguments.endpoint
    if endpoint is None:
        sessions = [session for session in discover() if session["reachable"]]
        if len(sessions) != 1:
            parser.error("Specify --endpoint after using discover; there must be exactly one selected session")
        endpoint = sessions[0]["endpoint"]
    request_id = arguments.id if arguments.command != "request" else arguments.id or uuid.uuid4().hex
    message = {"action": arguments.command, "id": request_id, "wait": max(0.0, arguments.timeout - 0.5)}
    if arguments.command == "request":
        try:
            parameters = json.loads(arguments.arguments)
        except ValueError as error:
            parser.error(str(error))
        if not isinstance(parameters, dict) or not 0 < arguments.queue_seconds <= MAX_WAIT_SECONDS:
            parser.error("arguments must be an object and queue-seconds must be greater than zero and at most 30")
        message.update(operation=arguments.operation, arguments=parameters, queue_seconds=arguments.queue_seconds)
    try:
        reply = exchange(endpoint, message, arguments.timeout)
    except (OSError, ValueError, KeyError, ConnectionError) as error:
        reply = {"id": request_id, "state": "unknown", "error": str(error),
                 "advice": "Do not resubmit. Query result with this ID and the same endpoint/session."}
    print(json.dumps(reply, indent=2, ensure_ascii=True))
    if reply.get("state") == "completed":
        return 0
    return 2 if reply.get("state") in ("queued", "executing", "unknown") else 1


if __name__ == "__main__":
    sys.exit(main())
