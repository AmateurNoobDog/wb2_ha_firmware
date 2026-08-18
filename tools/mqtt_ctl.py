#!/usr/bin/env python3
"""MQTT control tool for the ctrl_reboot board.

Publishes a command on wb2/control and prints the ack received on wb2/status.

Usage:
  python3 mqtt_ctl.py reboot
  python3 mqtt_ctl.py --broker 192.168.31.121 --port 1883 flash
  python3 mqtt_ctl.py --watch                 # just listen for acks
"""
import argparse
import sys
import time

import paho.mqtt.client as mqtt

TOPIC_CTRL = "wb2/control"
TOPIC_STATUS = "wb2/status"

DEFAULT_BROKER = "192.168.31.121"
DEFAULT_PORT = 1883

VALID_CMDS = ("reboot", "flash", "boot")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("cmd", nargs="?", choices=VALID_CMDS, help="command to send")
    ap.add_argument("--broker", default=DEFAULT_BROKER, help="MQTT broker (default %(default)s)")
    ap.add_argument("--port", type=int, default=DEFAULT_PORT, help="MQTT port (default %(default)d)")
    ap.add_argument("--watch", action="store_true", help="only listen for acks, send nothing")
    ap.add_argument("--wait", type=float, default=6.0, help="seconds to wait for ack")
    args = ap.parse_args()

    if not args.watch and not args.cmd:
        ap.error("please provide a command or --watch")

    got = []

    def on_connect(client, userdata, flags, rc):
        print(f"[mqtt] connected to {args.broker}:{args.port} rc={rc}", flush=True)
        client.subscribe(TOPIC_STATUS)

    def on_message(client, userdata, msg):
        payload = msg.payload.decode(errors="replace")
        got.append(payload)
        print(f"[mqtt] RCV {msg.topic}: {payload!r}", flush=True)
        if not args.watch:
            client.disconnect()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(args.broker, args.port, 30)
    client.loop_start()

    time.sleep(1)
    if args.cmd:
        print(f"[mqtt] SEND {TOPIC_CTRL}: {args.cmd}", flush=True)
        client.publish(TOPIC_CTRL, args.cmd, qos=1)

    deadline = time.time() + (args.wait if args.cmd else 30)
    while time.time() < deadline and (args.watch or not got):
        time.sleep(0.2)
    client.loop_stop()

    if args.cmd and got:
        print(f"[ok] ack: {got[0]}")
        return 0
    if args.cmd and not got:
        print(f"[fail] no ack for '{args.cmd}' within {args.wait}s")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
