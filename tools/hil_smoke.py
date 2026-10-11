#!/usr/bin/env python3
"""Hardware-in-the-loop smoke test: flash every sketch, press its keys, check verdicts.

    python3 tools/hil_smoke.py --port /dev/cu.usbmodem1101            # all sketches
    python3 tools/hil_smoke.py --port /dev/ttyACM0 --only race inversion

Needs an ESP32-C3 on USB, arduino-cli (see compile_all.sh) and pyserial
(`pip install pyserial`, or `apt install python3-serial`). The expected verdicts
below are the behaviour each lab README promises; keep them in sync.
"""

import argparse
import os
import re
import subprocess
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing: pip install pyserial")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FQBN = os.environ.get("FQBN", "esp32:esp32:esp32c3:CDCOnBoot=cdc")

# sketch -> list of (key to send, regex the VERDICT line must match, seconds to wait).
# key None = no key, the verdict comes by itself after reset.
CHECKS = {
    "lab0-arduino-basics/hello_serial": [
        ("i", r"CHIP=ESP32-C3 CPU_MHZ=160", 5), ("t", r"SIZEOF_INT=4 SIZEOF_LONG=4 SIZEOF_PTR=4", 5),
        ("f", r"FORMATS_DONE", 5)],
    "lab0-arduino-basics/blink": [
        ("b", r"BLINK=OFF", 5), ("b", r"BLINK=ON", 5), ("l", r"LOOPS_PER_S=\d+", 5),
        ("a", r"BLOCKED_MS=\d+", 10)],
    "lab0-arduino-basics/button_poll": [
        ("m", r"MODE=FOLLOW", 5), ("m", r"MODE=TOGGLE", 5), ("v", r"PRESSES=0", 5)],
    "lab0-arduino-basics/pwm_fade": [
        ("5", r"DUTY_PERCENT=20", 5), ("f", r"FADE_DONE", 5), ("s", r"SOFT_PWM_HZ=5$", 6),
        ("h", r"SOFT_PWM_HZ=500", 6)],
    # clocks runs once by itself after reset, usually before the port is open
    # here, so press 'r' to run it again and judge that run.
    "lab1-time-latency/clocks": [("r", r"CLOCKS_DONE", 30)],
    "lab1-time-latency/periodic_jitter": [
        ("a", r"DRIFT_MS=\d+", 10), ("b", r"JITTER_US=\d+", 10), ("c", r"JITTER_US=\d+", 10),
        ("d", r"JITTER_US=\d+", 10), ("e", r"JITTER_US=\d+", 20)],
    "lab2-scheduling/scheduling": [
        ("a", r"FINISH_ORDER=HML", 5), ("b", r"FINISH_ORDER=ABC", 5),
        ("c", r"FINISH_ORDER=HL", 5), ("d", r"FINISH_ORDER=HML", 5)],
    "lab2-scheduling/starvation_wdt": [
        ("a", r"NO_RESET", 5), ("c", r"NO_RESET", 15),
        ("b", r"RESET_DURING_b reason=TASK_WDT", 15), ("d", r"RESET_DURING_d reason=TASK_WDT", 15)],
    "lab3-periodic-rm/wcet_measure": [
        ("a", r"WCET_US=", 5), ("b", r"WCET_US=", 5), ("c", r"WCET_US=", 5), ("d", r"WCET_US=", 10)],
    "lab3-periodic-rm/rm_taskset": [
        ("a", r"SCHEDULABLE", 5), ("b", r"SCHEDULABLE", 5), ("c", r"MISSES .*logger=[1-9]", 5),
        ("d", r"MISSES sensor=[1-9]", 5), ("e", r"MISSES .*logger=[1-9]", 5)],
    "lab4-interrupts/isr_latency": [(k, r"ISR_MAX_US=", 6) for k in "abcdef"],
    "lab4-interrupts/button": [("v", r"EDGES=\d+ PRESSES=\d+", 5)],
    "lab5-shared-resources/race": [
        ("a", r"LOST=[1-9]", 10), ("b", r"LOST=0\b", 10), ("c", r"LOST=0\b", 30),
        ("d", r"LOST=0\b", 10), ("e", r"LOST=0\b", 10)],
    "lab5-shared-resources/inversion": [
        ("a", r"\bUNBOUNDED", 5), ("b", r"\bBOUNDED", 5), ("c", r"\bUNBOUNDED", 5), ("d", r"\bBOUNDED", 5)],
    "lab5-shared-resources/deadlock": [
        ("a", r"DEADLOCK$", 5), ("b", r"RECOVERED", 5), ("c", r"NO_DEADLOCK", 5),
        # d must show at least one deadlock: NO_DEADLOCK there means the two
        # tasks did not really run together.
        ("d", r"RECOVERED", 40)],
}


def find_cli():
    cli = os.environ.get("ARDUINO_CLI")
    if cli:
        return cli
    for c in ("arduino-cli",
              "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"):
        if subprocess.call(["which", c], stdout=subprocess.DEVNULL) == 0 or os.path.exists(c):
            return c
    sys.exit("arduino-cli not found; set ARDUINO_CLI")


def open_port(port, deadline):
    """Open the port, retrying while the board re-enumerates after a reset."""
    while True:
        try:
            return serial.Serial(port, 115200, timeout=0.2)
        except serial.SerialException:
            if time.time() > deadline:
                raise
            time.sleep(0.3)


def wait_for(port, ser, pattern, seconds, log):
    """Read lines until a VERDICT line matches. Survives a board reset."""
    deadline = time.time() + seconds
    buf = b""
    while time.time() < deadline:
        try:
            chunk = ser.read(256)
        except (serial.SerialException, OSError):
            ser.close()
            time.sleep(1.0)  # the board resets and USB comes back
            ser = open_port(port, deadline)
            continue
        buf += chunk
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            text = line.decode(errors="replace").rstrip()
            log.write(text + "\n")
            if text.startswith("VERDICT:"):
                verdict = text[len("VERDICT:"):].strip()
                if re.search(pattern, verdict):
                    return ser, True, verdict
                return ser, False, verdict
    return ser, False, "(timeout)"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--only", nargs="*", help="sketch names, e.g. race inversion")
    args = ap.parse_args()

    cli = find_cli()
    env = dict(os.environ, ARDUINO_DIRECTORIES_USER=os.path.join(ROOT, ".arduino-user"))
    failures = 0
    log_path = os.path.join(ROOT, "hil_smoke.log")
    with open(log_path, "w") as log:
        for sketch, checks in CHECKS.items():
            if args.only and os.path.basename(sketch) not in args.only:
                continue
            print(f"== {sketch}: flashing")
            log.write(f"\n===== {sketch}\n")
            r = subprocess.run([cli, "compile", "--upload", "-p", args.port, "--fqbn", FQBN,
                                os.path.join(ROOT, sketch)], env=env, capture_output=True, text=True)
            if r.returncode != 0:
                print(r.stdout[-2000:], r.stderr[-2000:])
                failures += 1
                continue
            time.sleep(2.0)
            ser = open_port(args.port, time.time() + 15)
            time.sleep(3.5)  # rt_begin() waits up to 3 s for the monitor
            for key, pattern, seconds in checks:
                if key:
                    ser.reset_input_buffer()
                    ser.write(key.encode())
                ser, ok, verdict = wait_for(args.port, ser, pattern, seconds, log)
                print(f"   {key or '-'}: {'ok  ' if ok else 'FAIL'} {verdict}")
                failures += 0 if ok else 1
            ser.close()
    print(f"\n{failures} failure(s); full serial output in {log_path}")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
