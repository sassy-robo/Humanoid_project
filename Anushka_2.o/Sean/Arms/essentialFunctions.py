"""
Arms client API for other modules (BOSS, Brain, ...).

The serial ports belong to Arms/arms_controller.py (the "haath" process), so
nothing here opens them. Gestures are validated against gesture_registry and
queued on the haath control file for the controller to execute.
"""

import os
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SEAN_ROOT = Path(__file__).resolve().parents[1]
DEPS_ROOT = REPO_ROOT / ".deps"
for entry in (str(REPO_ROOT), str(SEAN_ROOT), str(DEPS_ROOT)):
    if entry not in sys.path:
        sys.path.insert(0, entry)

from anushka_runtime.config import CONTROL_FILES, LEFT_ARM_MEGA_PORT, RIGHT_ARM_MEGA_PORT
from anushka_runtime.ipc import append_message
from Arms.gesture_registry import resolve_gesture


def arm_port_status() -> dict[str, str]:
    """Per side: 'sim' (no port set), 'missing' (port set, device absent) or 'ok'."""
    status = {}
    for side, port in (("left", LEFT_ARM_MEGA_PORT), ("right", RIGHT_ARM_MEGA_PORT)):
        if not port:
            status[side] = "sim"
        elif port.startswith("/dev/") and not os.path.exists(port):
            status[side] = "missing"
        else:
            status[side] = "ok"
    return status


def send_gesture(token: str) -> bool:
    """Queue a registered gesture (code like "20" or name like "open_arms")."""
    spec = resolve_gesture(token)
    if spec is None:
        return False
    append_message(CONTROL_FILES["haath"], spec.code)
    return True


if __name__ == "__main__":
    print(arm_port_status())
    assert resolve_gesture("open_arms").code == "20"
    assert resolve_gesture("20").name == "open_arms"
    assert resolve_gesture("not_a_gesture") is None
    print("ok")
