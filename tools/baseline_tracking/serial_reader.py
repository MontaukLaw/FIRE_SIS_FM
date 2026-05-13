import struct
import time

import serial
from PyQt5.QtCore import QThread, pyqtSignal

from config import (
    EVENT_PAYLOAD_LEN,
    FRAME_HEADER,
    FRAME_TAIL,
    FRAME_TYPE_EVENT,
    FRAME_TYPE_MONITOR,
    PAYLOAD_LEN,
)
from models import BaselineEvent, BaselineFrame


class SerialReader(QThread):
    frame_received = pyqtSignal(object)
    event_received = pyqtSignal(object)
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
                    for packet_type, packet in parsed:
                        if packet_type == FRAME_TYPE_MONITOR:
                            self.frame_received.emit(packet)
                            frame_count += 1
                        elif packet_type == FRAME_TYPE_EVENT:
                            self.event_received.emit(packet)
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
            start = buf.find(FRAME_HEADER)
            if start < 0:
                if buf and buf[-1] == FRAME_HEADER[0]:
                    buf[:] = buf[-1:]
                else:
                    buf.clear()
                break

            if start > 0:
                del buf[:start]

            if len(buf) < 3:
                break

            frame_type = buf[2]
            if frame_type == FRAME_TYPE_MONITOR:
                payload_len = PAYLOAD_LEN
            elif frame_type == FRAME_TYPE_EVENT:
                payload_len = EVENT_PAYLOAD_LEN
            else:
                del buf[0]
                continue

            frame_len = len(FRAME_HEADER) + 1 + payload_len + len(FRAME_TAIL)
            if len(buf) < frame_len:
                break

            if bytes(buf[frame_len - len(FRAME_TAIL):frame_len]) != FRAME_TAIL:
                del buf[0]
                continue

            payload_start = len(FRAME_HEADER) + 1
            payload = bytes(buf[payload_start:payload_start + payload_len])
            del buf[:frame_len]

            try:
                if frame_type == FRAME_TYPE_MONITOR:
                    values = struct.unpack("<fffff", payload)
                    frames.append((frame_type, BaselineFrame.from_tuple(values)))
                else:
                    values = struct.unpack("<HHIHB", payload)
                    frames.append((frame_type, BaselineEvent.from_tuple(values)))
            except struct.error:
                continue

        return frames
