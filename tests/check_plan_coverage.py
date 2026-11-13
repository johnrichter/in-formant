#!/usr/bin/env python3
"""Check that each case of type Unit in the test plan has a test or a recorded reason.

A case is covered when a test has its ID in the name, in the form TEST(Suite, TP_AREA_NN...).
A case that has no test needs a line in the deferred file: "TP-AREA-NN | package | reason".
The check fails when:
- a Unit case has no test and no line in the deferred file,
- a line of the deferred file names an ID that is not a Unit case of the plan,
- a line of the deferred file names a case that has a test (remove the line),
- a test names an ID that is not in the plan (a typo).
"""
import argparse
import re
import sys
from pathlib import Path

CASE = re.compile(r"^- \*\*(TP-[A-Z0-9]+-\d+)\*\* \(([^,)]+), (P\d)\)")
# The ID is in the test name. A prefix DISABLED_ marks a test that waits for a feature that does not exist.
TEST = re.compile(r"\bTEST(?:_F|_P)?\s*\(\s*\w+\s*,\s*(DISABLED_)?(TP_[A-Z0-9]+_\d+)")


def read_cases(plan):
    cases = {}
    for line in plan.read_text(encoding="utf-8").splitlines():
        m = CASE.match(line)
        if m:
            cases[m.group(1)] = m.group(2)
    return cases


def read_tests(folder):
    found = {}
    for path in sorted(folder.rglob("*.cpp")):
        text = path.read_text(encoding="utf-8")
        for m in TEST.finditer(text):
            found.setdefault(m.group(2).replace("_", "-"), []).append((path.name, bool(m.group(1))))
    return found


def read_deferred(path):
    deferred = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        if len(parts) < 3 or not parts[2]:
            raise SystemExit(f"{path}:{number}: a line needs the form 'ID | package | reason'")
        deferred[parts[0]] = (parts[1], parts[2])
    return deferred


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--plan", required=True, type=Path)
    parser.add_argument("--tests", required=True, type=Path)
    parser.add_argument("--deferred", required=True, type=Path)
    parser.add_argument("--report", action="store_true", help="print the open cases for each package")
    args = parser.parse_args()

    cases = read_cases(args.plan)
    unit = {k for k, v in cases.items() if v == "Unit"}
    tests = read_tests(args.tests)
    deferred = read_deferred(args.deferred)

    errors = []
    for case in sorted(unit):
        if case not in tests and case not in deferred:
            errors.append(f"{case}: no test and no line in the deferred file")
    for case in sorted(deferred):
        if case not in unit:
            errors.append(f"{case}: in the deferred file, but not a Unit case of the plan")
        elif case in tests:
            errors.append(f"{case}: has a test, so remove it from the deferred file")
    for case in sorted(tests):
        if case not in cases:
            errors.append(f"{case}: a test names an ID that is not in the plan")

    covered = sorted(c for c in unit if c in tests)
    disabled = sorted(c for c in covered if all(d for _, d in tests[c]))
    print(f"Unit cases: {len(unit)}, with a test: {len(covered)} ({len(disabled)} disabled), deferred: {len(deferred)}")
    if args.report:
        by_package = {}
        for case, (package, reason) in sorted(deferred.items()):
            by_package.setdefault(package, []).append(case)
        for package in sorted(by_package):
            print(f"  {package}: {len(by_package[package])} open")
    for e in errors:
        print("ERROR " + e)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
