import serial

from PyQt5.QtCore import QThread, pyqtSignal

from utils import now_str


class SerialReaderThread(QThread):
    sample_received = pyqtSignal(int, int, int, str)
    log_message = pyqtSignal(str)
    error_message = pyqtSignal(str)

    HEADER = b"\xA5\x5A"
    TAIL = b"\x0D\x0A"
    TYPE_RAW = 0x01
    FRAME_SIZE = 11

    def __init__(self, port, baud=115200, parent=None):
        super().__init__(parent)
        self.port = port
        self.baud = baud
        self._running = True
        self.ser = None

    def stop(self):
        self._running = False

    def run(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.02)
            self.log_message.emit(f"[{now_str()}] Serial connected: {self.port} @ {self.baud}")
        except Exception as exc:
            self.error_message.emit(f"[{now_str()}] Open serial failed: {exc}")
            return

        rx_buf = bytearray()

        try:
            while self._running:
                try:
                    n = self.ser.in_waiting
                    data = self.ser.read(n if n > 0 else 1)
                except Exception as exc:
                    self.error_message.emit(f"[{now_str()}] Serial read error: {exc}")
                    break

                if data:
                    rx_buf.extend(data)
                    packets = self.extract_packets(rx_buf)
                    for x, y, z, pkt_hex in packets:
                        self.sample_received.emit(x, y, z, pkt_hex)
                else:
                    self.msleep(5)

        finally:
            try:
                if self.ser and self.ser.is_open:
                    self.ser.close()
            except Exception:
                pass
            self.log_message.emit(f"[{now_str()}] Serial disconnected")

    @classmethod
    def extract_packets(cls, buf: bytearray):
        """
        Parse fixed frame:
          [0]=A5 [1]=5A [2]=type [3..8]=x/y/z int16 little-endian [9]=0D [10]=0A
        """
        packets = []

        while True:
            start = buf.find(cls.HEADER)
            if start < 0:
                # Keep at most one possible leading header byte.
                if len(buf) > 0 and buf[-1] == cls.HEADER[0]:
                    buf[:] = buf[-1:]
                else:
                    buf.clear()
                break

            if start > 0:
                del buf[:start]

            if len(buf) < cls.FRAME_SIZE:
                break

            frame = bytes(buf[: cls.FRAME_SIZE])

            if frame[0:2] != cls.HEADER or frame[9:11] != cls.TAIL:
                del buf[0]
                continue

            frame_type = frame[2]
            if frame_type != cls.TYPE_RAW:
                del buf[: cls.FRAME_SIZE]
                continue

            x = int.from_bytes(frame[3:5], byteorder="little", signed=True)
            y = int.from_bytes(frame[5:7], byteorder="little", signed=True)
            z = int.from_bytes(frame[7:9], byteorder="little", signed=True)
            packets.append((x, y, z, frame.hex(" ").upper()))
            del buf[: cls.FRAME_SIZE]

        return packets
