import math
from collections import deque

from utils import clamp, lpf_alpha


class TapDetector:
    DEFAULTS = {
        "TH_DYN": 80.0,
        "TH_JERK": 30.0,
        "TH_E_SHORT": 12000.0,
        "TH_RATIO": 0.30,
        "TH_FLIP_ANGLE": 12.0,
        "TH_FLIP_DETECT": 18.0,
        "TH_FLIP_CANCEL": 18.0,
        "FLIP_DYN_GUARD": 120.0,
        "WIDTH_LEVEL": 35.0,
        "WIDTH_MIN": 4.0,
        "WIDTH_MAX": 16.0,
    }

    def __init__(self, fs=200.0):
        self.fs = fs

        # Filters
        self.alpha_pre = lpf_alpha(18.0, fs)  # remove burr noise
        self.alpha_g = lpf_alpha(1.5, fs)  # gravity LPF

        # Tunable thresholds
        self.reset_thresholds()

        self.CANDIDATE_WIN = 6
        self.REFRACTORY_MS = 200.0
        self.FLIP_INHIBIT_MS = 200.0
        self.THETA_WINDOW_MS = 200.0
        self.REFRACTORY_SAMPLES = self.ms_to_samples(self.REFRACTORY_MS)
        self.FLIP_INHIBIT_SAMPLES = self.ms_to_samples(self.FLIP_INHIBIT_MS)
        self.THETA_WINDOW_SAMPLES = self.ms_to_samples(self.THETA_WINDOW_MS)

        self.reset()

    def ms_to_samples(self, duration_ms):
        return max(1, int(round(self.fs * duration_ms / 1000.0)))

    def get_thresholds(self):
        return {name: getattr(self, name) for name in self.DEFAULTS}

    def set_thresholds(self, values):
        for name in self.DEFAULTS:
            if name in values:
                setattr(self, name, float(values[name]))

    def reset_thresholds(self):
        self.set_thresholds(self.DEFAULTS)

    def reset(self):
        self.initialized = False

        self.xf = self.yf = self.zf = 0.0
        self.gx = self.gy = self.gz = 0.0
        self.dx_prev = self.dy_prev = self.dz_prev = 0.0

        self.short_win = deque(maxlen=6)
        self.long_win = deque(maxlen=40)
        self.g_hist = deque(maxlen=self.THETA_WINDOW_SAMPLES)

        self.state = "IDLE"
        self.candidate_count = 0
        self.candidate_start_packet = 0
        self.candidate_peak_dyn = 0.0
        self.candidate_peak_jerk = 0.0
        self.candidate_max_ratio = 0.0
        self.candidate_max_theta = 0.0
        self.candidate_width = 0
        self.candidate_width_start = 0
        self.candidate_width_end = 0

        self.refractory_count = 0
        self.flip_inhibit_count = 0

        self.tap_count = 0
        self.packet_count = 0

    def process(self, ax, ay, az):
        self.packet_count += 1
        event = None
        fail_info = None

        if not self.initialized:
            self.xf, self.yf, self.zf = float(ax), float(ay), float(az)
            self.gx, self.gy, self.gz = float(ax), float(ay), float(az)
            self.initialized = True

        # 1) pre-filter
        self.xf += self.alpha_pre * (ax - self.xf)
        self.yf += self.alpha_pre * (ay - self.yf)
        self.zf += self.alpha_pre * (az - self.zf)

        # 2) gravity estimation
        self.gx += self.alpha_g * (self.xf - self.gx)
        self.gy += self.alpha_g * (self.yf - self.gy)
        self.gz += self.alpha_g * (self.zf - self.gz)

        # 3) dynamic component
        dx = self.xf - self.gx
        dy = self.yf - self.gy
        dz = self.zf - self.gz

        dyn_mag = math.sqrt(dx * dx + dy * dy + dz * dz)

        # 4) jerk
        jx = dx - self.dx_prev
        jy = dy - self.dy_prev
        jz = dz - self.dz_prev
        jerk_mag = math.sqrt(jx * jx + jy * jy + jz * jz)

        self.dx_prev, self.dy_prev, self.dz_prev = dx, dy, dz

        # 5) energy windows
        dyn2 = dx * dx + dy * dy + dz * dz
        self.short_win.append(dyn2)
        self.long_win.append(dyn2)

        e_short = sum(self.short_win)
        e_long = sum(self.long_win) + 1e-6
        ratio = e_short / e_long

        # 6) flip indicator: slow gravity direction change
        self.g_hist.append((self.gx, self.gy, self.gz))
        flip_angle_deg = 0.0
        if len(self.g_hist) >= self.g_hist.maxlen:
            g0 = self.g_hist[0]
            g1 = self.g_hist[-1]
            n0 = math.sqrt(g0[0] * g0[0] + g0[1] * g0[1] + g0[2] * g0[2]) + 1e-6
            n1 = math.sqrt(g1[0] * g1[0] + g1[1] * g1[1] + g1[2] * g1[2]) + 1e-6
            c = (g0[0] * g1[0] + g0[1] * g1[1] + g0[2] * g1[2]) / (n0 * n1)
            c = clamp(c, -1.0, 1.0)
            flip_angle_deg = math.degrees(math.acos(c))

        # flip inhibit logic
        if flip_angle_deg > self.TH_FLIP_DETECT and dyn_mag < self.FLIP_DYN_GUARD:
            self.flip_inhibit_count = self.FLIP_INHIBIT_SAMPLES

        if self.flip_inhibit_count > 0:
            self.flip_inhibit_count -= 1

        # state machine
        if self.refractory_count > 0:
            self.refractory_count -= 1
            self.state = "REFRACTORY"
            if self.refractory_count == 0:
                self.state = "IDLE"

        else:
            if self.state == "IDLE":
                if (
                    self.flip_inhibit_count == 0
                    and dyn_mag > self.TH_DYN
                    and jerk_mag > self.TH_JERK
                    and e_short > self.TH_E_SHORT
                    and ratio > self.TH_RATIO
                    and flip_angle_deg < self.TH_FLIP_ANGLE
                ):
                    self.state = "CANDIDATE"
                    self.candidate_count = self.CANDIDATE_WIN
                    self.candidate_start_packet = self.packet_count
                    self.candidate_peak_dyn = dyn_mag
                    self.candidate_peak_jerk = jerk_mag
                    self.candidate_max_ratio = ratio
                    self.candidate_max_theta = flip_angle_deg
                    self.candidate_width = 1 if dyn_mag > self.WIDTH_LEVEL else 0
                    if dyn_mag > self.WIDTH_LEVEL:
                        self.candidate_width_start = self.packet_count
                        self.candidate_width_end = self.packet_count
                    else:
                        self.candidate_width_start = 0
                        self.candidate_width_end = 0

            elif self.state == "CANDIDATE":
                self.candidate_count -= 1
                self.candidate_peak_dyn = max(self.candidate_peak_dyn, dyn_mag)
                self.candidate_peak_jerk = max(self.candidate_peak_jerk, jerk_mag)
                self.candidate_max_ratio = max(self.candidate_max_ratio, ratio)
                self.candidate_max_theta = max(self.candidate_max_theta, flip_angle_deg)

                if dyn_mag > self.WIDTH_LEVEL:
                    self.candidate_width += 1
                    if self.candidate_width_start == 0:
                        self.candidate_width_start = self.packet_count
                    self.candidate_width_end = self.packet_count

                if flip_angle_deg > self.TH_FLIP_CANCEL:
                    fail_reasons = [
                        f"max_theta={self.candidate_max_theta:.1f} >= flip_cancel={self.TH_FLIP_CANCEL:.1f}"
                    ]
                    fail_info = {
                        "peak_dyn": self.candidate_peak_dyn,
                        "peak_jerk": self.candidate_peak_jerk,
                        "peak_width": self.candidate_width,
                        "max_ratio": self.candidate_max_ratio,
                        "max_theta": self.candidate_max_theta,
                        "end_ratio": ratio,
                        "end_theta": flip_angle_deg,
                        "fail_reasons": fail_reasons,
                    }
                    self.state = "IDLE"

                elif self.candidate_count <= 0:
                    fail_reasons = []
                    if not (self.WIDTH_MIN <= self.candidate_width <= self.WIDTH_MAX):
                        fail_reasons.append(
                            f"peak_width={self.candidate_width} not in [{self.WIDTH_MIN:.0f}, {self.WIDTH_MAX:.0f}]"
                        )
                    if not (self.candidate_peak_dyn > self.TH_DYN):
                        fail_reasons.append(
                            f"peak_dyn={self.candidate_peak_dyn:.1f} <= TH_DYN={self.TH_DYN:.1f}"
                        )
                    if not (self.candidate_peak_jerk > self.TH_JERK):
                        fail_reasons.append(
                            f"peak_jerk={self.candidate_peak_jerk:.1f} <= TH_JERK={self.TH_JERK:.1f}"
                        )
                    if not (ratio > self.TH_RATIO):
                        fail_reasons.append(
                            f"end_ratio={ratio:.2f} <= TH_RATIO={self.TH_RATIO:.2f}"
                        )
                    if not (flip_angle_deg < self.TH_FLIP_ANGLE):
                        fail_reasons.append(
                            f"end_theta={flip_angle_deg:.1f} >= TH_FLIP_ANGLE={self.TH_FLIP_ANGLE:.1f}"
                        )

                    if not fail_reasons:
                        self.tap_count += 1
                        event = "TAP"
                        self.state = "REFRACTORY"
                        self.refractory_count = self.REFRACTORY_SAMPLES
                    else:
                        fail_info = {
                            "peak_dyn": self.candidate_peak_dyn,
                            "peak_jerk": self.candidate_peak_jerk,
                            "peak_width": self.candidate_width,
                            "max_ratio": self.candidate_max_ratio,
                            "max_theta": self.candidate_max_theta,
                            "end_ratio": ratio,
                            "end_theta": flip_angle_deg,
                            "fail_reasons": fail_reasons,
                        }
                        self.state = "IDLE"

        tap_region_start = self.candidate_width_start
        tap_region_end = self.candidate_width_end
        if tap_region_start == 0 or tap_region_end == 0:
            tap_region_start = self.candidate_start_packet
            tap_region_end = self.packet_count

        tap_width_samples = 0
        tap_duration_ms = 0.0
        if event == "TAP":
            tap_width_samples = max(1, tap_region_end - tap_region_start + 1)
            tap_duration_ms = 1000.0 * tap_width_samples / self.fs

        return {
            "ax": ax,
            "ay": ay,
            "az": az,
            "gx": self.gx,
            "gy": self.gy,
            "gz": self.gz,
            "dx": dx,
            "dy": dy,
            "dz": dz,
            "dyn_mag": dyn_mag,
            "jerk_mag": jerk_mag,
            "e_short": e_short,
            "e_long": e_long,
            "ratio": ratio,
            "flip_angle_deg": flip_angle_deg,
            "state": self.state,
            "tap_count": self.tap_count,
            "packet_count": self.packet_count,
            "tap_region_start": tap_region_start,
            "tap_region_end": tap_region_end,
            "tap_width_samples": tap_width_samples,
            "tap_duration_ms": tap_duration_ms,
            "peak_dyn": self.candidate_peak_dyn if event == "TAP" else 0.0,
            "peak_jerk": self.candidate_peak_jerk if event == "TAP" else 0.0,
            "peak_width": self.candidate_width if event == "TAP" else 0,
            "max_ratio": self.candidate_max_ratio if event == "TAP" else 0.0,
            "max_theta": self.candidate_max_theta if event == "TAP" else 0.0,
            "fail_info": fail_info,
            "event": event,
        }
