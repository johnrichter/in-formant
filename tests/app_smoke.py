"""Smoke tests of the built application.

The tests start the staged application bundle as a user would start it, from the root folder and with no test
hook. They check the log file, the idle CPU use, the start with no QML error, the quit at a signal, and the
signature. The tests need a built application, so CTest skips them (exit code 77) when the bundle is missing.

Usage: python3 -I app_smoke.py --app InFormant.app --case log|idle|start|signature
"""
import argparse
import os
import signal
import subprocess
import sys
import time

SKIP = 77
ERROR_WORDS = ("typeerror", "referenceerror", "is not a type", "platform plugin", "could not load")


def config_path():
    return os.path.expanduser("~/Library/Application/informant.conf")


def log_path():
    return os.path.expanduser("~/Library/Logs/InFormant/InFormant.log")


def remove_config():
    # The application writes this file when it quits. A test must leave no file behind.
    try:
        os.remove(config_path())
    except FileNotFoundError:
        pass


class Running:
    """Starts the application from the root folder, and quits it at the end."""

    def __init__(self, app):
        self.binary = os.path.join(app, "Contents", "MacOS", "InFormant")
        self.process = None

    def __enter__(self):
        remove_config()
        self.start = time.time()
        self.process = subprocess.Popen([self.binary], cwd="/", stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return self

    def alive(self):
        return self.process.poll() is None

    def quit(self, wait=15.0):
        """Sends SIGTERM. Returns the seconds to the end, or None when the process did not end."""
        if not self.alive():
            return 0.0
        began = time.time()
        self.process.send_signal(signal.SIGTERM)
        while time.time() - began < wait:
            if not self.alive():
                return time.time() - began
            time.sleep(0.1)
        return None

    def __exit__(self, *args):
        if self.process is not None and self.alive():
            self.process.kill()
            self.process.wait()
        remove_config()


def new_log_lines(since):
    """The lines that the application wrote to its log file after the time `since`."""
    try:
        with open(log_path(), encoding="utf-8", errors="replace") as handle:
            lines = handle.read().splitlines()
    except FileNotFoundError:
        return []
    stamp = time.strftime("[%Y-%m-%d %H:%M:%S]", time.localtime(since - 2))
    return [line for line in lines if line[:21] >= stamp]


def cpu_seconds(pid):
    out = subprocess.run(["ps", "-o", "cputime=", "-p", str(pid)], capture_output=True, text=True).stdout.strip()
    if not out:
        return None
    # The format is [hours:]minutes:seconds.hundredths
    total = 0.0
    for part in out.split(":"):
        total = total * 60 + float(part)
    return total


def case_start(app):
    """TP-APP-01: the application starts with no QML error and quits within 15 s at SIGTERM."""
    with Running(app) as run:
        time.sleep(8)
        if not run.alive():
            return "the application ended by itself"
        lines = new_log_lines(run.start)
        took = run.quit()
        if took is None:
            return "the application did not quit within 15 s"
        bad = [line for line in lines if any(word in line.lower() for word in ERROR_WORDS)]
        if bad:
            return "error in the log: " + bad[0]
        # The start must get past the capture step with no wait for a system question. Without the microphone
        # access the capture stream stays closed. With the access it opens.
        passed = [line for line in lines if "capture stream stays closed" in line or "Input device \"" in line]
        if not passed:
            return "the start did not get past the capture step in 8 s"
    return None


def case_log(app):
    """TP-LOG-05: started from the root folder, the application writes a log with stamps in the folder of the user."""
    with Running(app) as run:
        time.sleep(6)
        run.quit()
        lines = new_log_lines(run.start)
        if not lines:
            return "no new line in " + log_path()
        if not all(line.startswith("[") and line[20:21] == "]" for line in lines if line.strip()):
            return "a line has no stamp: " + next(line for line in lines if not line.startswith("["))
    if os.path.exists("/InFormant.log"):
        return "a log file is in the root folder"
    return None


def case_idle(app):
    """TP-IDLE-03: a paused application uses less than 6 percent of one core in 20 s."""
    with Running(app) as run:
        time.sleep(10)
        if not run.alive():
            return "the application ended by itself"
        first = cpu_seconds(run.process.pid)
        began = time.time()
        time.sleep(20)
        second = cpu_seconds(run.process.pid)
        elapsed = time.time() - began
        run.quit()
        if first is None or second is None:
            return "no CPU time from ps"
        use = 100.0 * (second - first) / elapsed
        print("CPU use while paused: %.1f percent" % use)
        if use >= 6.0:
            return "the CPU use is %.1f percent" % use
    return None


def case_signature(app):
    """TP-REL-13: the bundle has a valid signature, and the signed application starts."""
    check = subprocess.run(["codesign", "--verify", "--deep", "--strict", app], capture_output=True, text=True)
    if check.returncode != 0:
        return "the signature is not valid: " + check.stderr.strip()
    return case_start(app)


CASES = {"start": case_start, "log": case_log, "idle": case_idle, "signature": case_signature}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", required=True)
    parser.add_argument("--case", required=True, choices=sorted(CASES))
    args = parser.parse_args()

    if sys.platform != "darwin":
        print("These tests run on macOS only.")
        return SKIP
    if not os.path.isdir(args.app):
        print("No application at %s. Run build-macos.sh first." % args.app)
        return SKIP
    if os.environ.get("INFORMANT_NO_APP_TESTS"):
        print("INFORMANT_NO_APP_TESTS is set.")
        return SKIP

    problem = CASES[args.case](args.app)
    if problem:
        print("FAILED: " + problem)
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
