from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QApplication, QFrame, QHBoxLayout, QMainWindow, QWidget

from config import PLOT_FPS
from info_panel import InfoPanel
from plot_panel import BaselinePlotPanel
from serial_reader import SerialReader


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.reader = None
        self.frozen = False

        self.setWindowTitle("Baseline Tracking Monitor")
        self.resize(1500, 860)
        self._build_ui()

        self.plot_timer = QTimer(self)
        self.plot_timer.timeout.connect(self.plot_panel.update_curves)
        self.plot_timer.start(int(1000 / max(1, PLOT_FPS)))

    def _build_ui(self):
        app = QApplication.instance()
        if app:
            app.setFont(QFont("Microsoft YaHei", 10))

        root = QWidget()
        root.setStyleSheet("background: #07090C;")
        self.setCentralWidget(root)

        layout = QHBoxLayout(root)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)

        left_frame = QFrame()
        left_frame.setStyleSheet("background: #0B0D10; border-radius: 8px;")
        left_layout = QHBoxLayout(left_frame)
        left_layout.setContentsMargins(0, 0, 0, 0)
        self.plot_panel = BaselinePlotPanel()
        left_layout.addWidget(self.plot_panel)

        right_frame = QFrame()
        right_frame.setStyleSheet("background: #0D1117; border-radius: 8px;")
        right_layout = QHBoxLayout(right_frame)
        right_layout.setContentsMargins(0, 0, 0, 0)
        self.info_panel = InfoPanel()
        right_layout.addWidget(self.info_panel)

        layout.addWidget(left_frame, 3)
        layout.addWidget(right_frame, 1)

        self.info_panel.connect_requested.connect(self.connect_serial)
        self.info_panel.disconnect_requested.connect(self.disconnect_serial)
        self.info_panel.freeze_toggled.connect(self.set_frozen)
        self.info_panel.refresh_requested.connect(lambda: self.log("Ports refreshed"))

    def connect_serial(self, port, baud):
        self.disconnect_serial()
        self.reader = SerialReader(port, baud, self)
        self.reader.frame_received.connect(self.on_frame)
        self.reader.fps_updated.connect(self.on_fps)
        self.reader.log_message.connect(self.log)
        self.reader.error_message.connect(self.on_error)
        self.reader.finished.connect(lambda: self.info_panel.set_connected(False))
        self.reader.start()
        self.info_panel.set_connected(True)
        self.log(f"Connecting {port} @ {baud}...")

    def disconnect_serial(self):
        if not self.reader:
            return

        self.reader.stop()
        self.reader.wait(1000)
        self.reader = None
        self.info_panel.set_connected(False)

    def on_frame(self, frame):
        if self.frozen:
            return

        self.plot_panel.add_frame(frame)
        self.info_panel.update_frame(frame)

    def set_frozen(self, frozen):
        self.frozen = frozen
        self.log("Data display frozen" if frozen else "Data display resumed")

    def on_fps(self, fps, _buffer_size, dropped):
        self.info_panel.update_fps(fps)
        if dropped:
            self.log(f"Dropped frames: {dropped}")

    def on_error(self, message):
        self.log(message)
        self.disconnect_serial()

    def log(self, message):
        self.info_panel.append_log(message)

    def closeEvent(self, event):
        self.disconnect_serial()
        super().closeEvent(event)
