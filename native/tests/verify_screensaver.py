from pathlib import Path
import re
import shutil
import subprocess
import sys
import os


def run_case(executable, appdata, activity):
    env = os.environ.copy()
    env.update({
        "APPDATA": str(appdata),
        "DAZZLE_SCR_TEST_HOOKS": "1",
        "DAZZLE_SCR_TEST_ACTIVITY": activity,
        "SDL_VIDEODRIVER": "dummy",
        "SDL_AUDIODRIVER": "dummy",
    })
    result = subprocess.run([str(executable), "/s"], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=12)
    output = result.stdout + result.stderr
    if result.returncode != 0:
        raise AssertionError(f"/s test {activity} exited {result.returncode}: {output}")
    match = re.search(r"scr_test: first_mode=(\d+) dismissed=(\d+) reason=(\w+) delay_ms=(\d+) result=(\d+)", output)
    if not match:
        raise AssertionError(f"/s test {activity} produced no lifecycle receipt: {output}")
    mode, dismissed, reason, delay, engine_result = match.groups()
    if int(mode) == 0:
        raise AssertionError(f"/s presented a text-mode first frame: {output}")
    if dismissed != "1" or engine_result != "0":
        raise AssertionError(f"/s did not dismiss cleanly for {activity}: {output}")
    expected = "key" if activity == "jitter" else "motion" if activity == "motion" else "key"
    if reason != expected:
        raise AssertionError(f"/s dismissed for {reason}, expected {expected}: {output}")
    if activity == "jitter" and int(delay) < 250:
        raise AssertionError(f"small mouse jitter dismissed the saver: {output}")


def main():
    executable = Path(sys.argv[1]).resolve()
    appdata = Path(sys.argv[2]).resolve()
    appdata.mkdir(parents=True, exist_ok=True)
    try:
        for activity in ("key", "motion", "jitter"):
            run_case(executable, appdata, activity)
    finally:
        shutil.rmtree(appdata, ignore_errors=True)


if __name__ == "__main__":
    main()
