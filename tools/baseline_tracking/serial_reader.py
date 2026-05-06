import struct
import time

import serial
from PyQt5.QtCore import QThread, pyqtSignal

from config import FRAME_TAIL, PAYLOAD_LEN
from models import BaselineFrame


class SerialReader(QThread):
    frame_received = pyqtSignal(object)
    fps_updated = pyqtSignal(float, int, int)
    log_message = pyqtSignal(str)
    error_message = pyqtSignal(str)

    def __init__(self, port, baud, parent=None):
        super().__init__(parent)
        self.port = port
        self.baud = baud
        self._running = True
        self._serial = None

    def stop(self):
        self._running = False

    def run(self):
        try:
            self._serial = serial.Serial(self.port, self.baud, timeout=0)
            self.log_message.emit(f"Serial connected: {self.port} @ {self.baud}")
        except Exception as exc:
            self.error_message.emit(f"Failed to open {self.port} @ {self.baud}: {exc}")
            return

        buf = bytearray()
        dropped = 0
        frame_count = 0
        fps_t0 = time.time()

        try:
            while self._running:
                try:
                    chunk = self._serial.read(4096)
                except Exception as exc:
                    self.error_message.emit(f"Serial read error: {exc}")
                    break

                if chunk:
                    buf.extend(chunk)

                    if len(buf) > 1024 * 1024:
                        buf[:] = buf[-65536:]

                    parsed = self._extract_frames(buf)
                    for frame in parsed:
                        self.frame_received.emit(frame)
                        frame_count += 1
                else:
                    self.msleep(1)

                now = time.time()
                if now - fps_t0 >= 1.0:
                    fps = frame_count / (now - fps_t0)
                    self.fps_updated.emit(fps, len(buf), dropped)
                    frame_count = 0
                    dropped = 0
                    fps_t0 = now

        finally:
            try:
                if self._serial and self._serial.is_open:
                    self._serial.close()
            except Exception:
                pass
            self.log_message.emit("Serial disconnected")

    @staticmethod
    def _extract_frames(buf):
        frames = []

        while True:
            idx = buf.find(FRAME_TAIL)
            if idx < 0:
                break

            start = idx - PAYLOAD_LEN
            if start < 0:
                del buf[: idx + len(FRAME_TAIL)]
                continue

            payload = bytes(buf[start:idx])
            del buf[: idx + len(FRAME_TAIL)]

            try:
                values = struct.unpack("<fffff", payload)
            except struct.error:
                continue

            frames.append(BaselineFrame.from_tuple(values))

        return frames
