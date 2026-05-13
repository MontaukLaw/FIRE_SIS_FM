import ctypes
import platform
from pathlib import Path

from config import AUDIO_COMFORT, AUDIO_HEAVY, AUDIO_LIGHT, LEVEL_1, LEVEL_2, LEVEL_3


class EventVoicePlayer:
    def __init__(self, base_dir=None, parent=None):
        del parent
        self.base_dir = Path(base_dir or Path(__file__).resolve().parent)
        self.available = platform.system() == "Windows"
        self.alias = "baseline_event_voice"
        self.last_error = ""
        self.audio_files = {
            "light": self.base_dir / AUDIO_LIGHT,
            "comfort": self.base_dir / AUDIO_COMFORT,
            "heavy": self.base_dir / AUDIO_HEAVY,
        }

    def play_for_peak(self, peak):
        level = self.level_for_peak(peak)
        if not level:
            return None

        if not self.available:
            self.last_error = "Windows MCI audio backend is unavailable"
            return level

        path = self.audio_files.get(level)
        if not path or not path.exists():
            self.last_error = f"Missing audio file: {path}"
            return level

        self.stop()
        if not self._mci(f'open "{path}" type mpegvideo alias {self.alias}'):
            return level
        self._mci(f"play {self.alias}")
        return level

    def stop(self):
        if not self.available:
            return
        self._mci(f"stop {self.alias}", keep_error=False)
        self._mci(f"close {self.alias}", keep_error=False)

    @staticmethod
    def level_for_peak(peak):
        if peak < LEVEL_1:
            return None
        if peak < LEVEL_2:
            return "light"
        if peak < LEVEL_3:
            return "comfort"
        return "heavy"

    def _mci(self, command, keep_error=True):
        error_code = ctypes.windll.winmm.mciSendStringW(command, None, 0, None)
        if error_code == 0:
            if keep_error:
                self.last_error = ""
            return True

        if keep_error:
            buf = ctypes.create_unicode_buffer(256)
            ctypes.windll.winmm.mciGetErrorStringW(error_code, buf, len(buf))
            self.last_error = f"MCI error {error_code}: {buf.value}"
        return False
