import math
import time


def lpf_alpha(fc_hz, fs_hz):
    """1st-order LPF alpha."""
    dt = 1.0 / fs_hz
    rc = 1.0 / (2.0 * math.pi * fc_hz)
    return dt / (rc + dt)


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def now_str():
    return time.strftime("%H:%M:%S")
