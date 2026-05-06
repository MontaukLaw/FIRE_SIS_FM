from collections import deque
import io
import math
import struct
import time
import wave
import winsound

import pyqtgraph as pg
import serial.tools.list_ports

from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import (
    QComboBox,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPlainTextEdit,
    QPushButton,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

from serial_reader_thread import SerialReaderThread
from tap_detector import TapDetector
from utils import now_str


class MainWindow(QMainWindow):
    HISTORY = 400
    CONNECT_COLOR = "#2F9E44"
    DISCONNECT_COLOR = "#CF1322"
    TARGET_FS = 200.0
    TAP_BEEP_FREQ = 1800
    TAP_BEEP_MS = 300
    THRESHOLD_FIELDS = [
        ("TH_DYN", "dyn阈值"),
        ("TH_JERK", "jerk阈值"),
        ("TH_E_SHORT", "E_short阈值"),
        ("TH_RATIO", "ratio阈值"),
        ("TH_FLIP_ANGLE", "theta阈值"),
        ("TH_FLIP_DETECT", "flip检测"),
        ("TH_FLIP_CANCEL", "flip取消"),
        ("WIDTH_LEVEL", "width电平"),
        ("WIDTH_MIN", "width最小"),
        ("WIDTH_MAX", "width最大"),
    ]

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Tap Detection Monitor - CH340 / 115200")
        self.resize(1600, 900)

        self.reader = None
        self.detector = TapDetector(fs=self.TARGET_FS)

        self.raw_x = deque(maxlen=self.HISTORY)
        self.raw_y = deque(maxlen=self.HISTORY)
        self.raw_z = deque(maxlen=self.HISTORY)
        self.dyn_hist = deque(maxlen=self.HISTORY)
        self.jerk_hist = deque(maxlen=self.HISTORY)
        self.sample_index = 0
        self.frame_times = deque()
        self.measured_fps = 0.0
        self.threshold_inputs = {}
        self.tap_sound_wav = self.build_tap_sound()
        self.thresholds_dirty = False

        self.latest = None

        self.init_ui()
        self.refresh_ports()

        self.plot_timer = QTimer(self)
        self.plot_timer.timeout.connect(self.update_plots)
        self.plot_timer.start(33)

    def init_ui(self):
        pg.setConfigOptions(antialias=True, background="#0F0F0F", foreground="w")

        root = QWidget()
        self.setCentralWidget(root)

        main_layout = QHBoxLayout(root)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)

        left_frame = QFrame()
        left_frame.setStyleSheet("background-color: #111111; border-radius: 8px;")
        left_layout = QVBoxLayout(left_frame)
        left_layout.setContentsMargins(8, 8, 8, 8)
        left_layout.setSpacing(8)

        self.raw_plot = pg.PlotWidget()
        self.raw_plot.setTitle("Raw XYZ", color="w", size="14pt")
        self.raw_plot.showGrid(x=True, y=True, alpha=0.25)
        self.raw_plot.setYRange(-512, 512)
        self.raw_plot.setLabel("left", "Value")
        self.raw_plot.setLabel("bottom", "Samples")
        self.raw_plot.addLegend()

        self.curve_x = self.raw_plot.plot(pen=pg.mkPen("#FF4D4F", width=2), name="X")
        self.curve_y = self.raw_plot.plot(pen=pg.mkPen("#52C41A", width=2), name="Y")
        self.curve_z = self.raw_plot.plot(pen=pg.mkPen("#40A9FF", width=2), name="Z")

        self.feature_plot = pg.PlotWidget()
        self.feature_plot.setTitle("Features: dyn_mag / jerk_mag", color="w", size="14pt")
        self.feature_plot.showGrid(x=True, y=True, alpha=0.25)
        self.feature_plot.setLabel("left", "Value")
        self.feature_plot.setLabel("bottom", "Samples")
        self.feature_plot.addLegend()
        self.feature_plot.setYRange(0, 300)

        self.curve_dyn = self.feature_plot.plot(pen=pg.mkPen("#FFD666", width=2), name="dyn_mag")
        self.curve_jerk = self.feature_plot.plot(pen=pg.mkPen("#B37FEB", width=2), name="jerk_mag")

        left_layout.addWidget(self.raw_plot, 3)
        left_layout.addWidget(self.feature_plot, 2)
        self.feature_hint = QLabel(
            "判定提示: dyn_mag = sqrt(dx*dx + dy*dy + dz*dz)，与安装方向无关，拍打时会突然冲高，慢速翻板通常不大。\n"
            "jerk_mag = sqrt((dx-dx_prev)^2 + (dy-dy_prev)^2 + (dz-dz_prev)^2)\n"
            "dyn_mag > Th1\n"
            "jerk_mag > Th2\n"
            "E_short > Th3\n"
            "theta_200ms < Th4\n"
            "peak_width in [2, 8] samples\n"
            "触发后进入 200ms refractory"
        )
        self.feature_hint.setWordWrap(True)
        self.feature_hint.setFont(QFont("Microsoft YaHei", 10))
        self.feature_hint.setStyleSheet(
            """
            QLabel {
                color: #D9D9D9;
                background-color: #191919;
                border: 1px solid #333333;
                border-radius: 8px;
                padding: 10px 12px;
            }
            """
        )
        left_layout.addWidget(self.feature_hint, 0)
        self.tap_duration_label = QLabel("最近拍打框时长: --")
        self.tap_duration_label.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))
        self.tap_duration_label.setStyleSheet("color: #FFD666; padding: 2px 4px;")
        left_layout.addWidget(self.tap_duration_label, 0)

        right_frame = QFrame()
        right_frame.setStyleSheet("background-color: #151515; border-radius: 8px;")
        right_layout = QVBoxLayout(right_frame)
        right_layout.setContentsMargins(10, 10, 10, 10)
        right_layout.setSpacing(10)

        serial_group = self.create_group("Serial Control")
        serial_layout = QVBoxLayout(serial_group)

        row1 = QHBoxLayout()
        lbl_port = QLabel("串口")
        lbl_port.setFont(QFont("Arial", 12, QFont.Bold))

        self.port_combo = QComboBox()
        self.port_combo.setMinimumHeight(38)
        self.port_combo.setFont(QFont("Arial", 12))
        self.port_combo.setStyleSheet(self.combo_style())

        self.btn_refresh = self.make_button("刷新串口", "#1677FF")
        self.btn_refresh.clicked.connect(self.refresh_ports)

        row1.addWidget(lbl_port)
        row1.addWidget(self.port_combo, 1)
        row1.addWidget(self.btn_refresh)

        row2 = QHBoxLayout()
        self.btn_connect_toggle = self.make_button("连接串口", self.CONNECT_COLOR)
        self.btn_connect_toggle.clicked.connect(self.toggle_serial_connection)
        row2.addWidget(self.btn_connect_toggle)

        row3 = QHBoxLayout()
        self.btn_reset_algo = self.make_button("重置算法", "#D46B08")
        self.btn_reset_algo.clicked.connect(self.reset_algo)
        row3.addWidget(self.btn_reset_algo)

        self.fps_label = QLabel(
            "\u76ee\u6807\u5e27\u7387: "
            f"{self.TARGET_FS:.0f} Hz | "
            "\u5b9e\u9645: -- Hz"
        )
        self.fps_label.setFont(QFont("Consolas", 11, QFont.Bold))
        self.fps_label.setStyleSheet("color: #73D13D; padding: 4px 2px;")

        serial_layout.addLayout(row1)
        serial_layout.addLayout(row2)
        serial_layout.addLayout(row3)
        serial_layout.addWidget(self.fps_label)

        threshold_group = self.create_group("Thresholds")
        threshold_layout = QGridLayout(threshold_group)
        threshold_layout.setHorizontalSpacing(10)
        threshold_layout.setVerticalSpacing(6)

        current_thresholds = self.detector.get_thresholds()
        for index, (key, label_text) in enumerate(self.THRESHOLD_FIELDS):
            row = index // 2
            col = (index % 2) * 2
            label = QLabel(label_text)
            label.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))
            label.setStyleSheet("color: #D9D9D9;")

            editor = QLineEdit(f"{current_thresholds[key]:g}")
            editor.setMinimumHeight(34)
            editor.setFont(QFont("Consolas", 10))
            editor.setStyleSheet(
                """
                QLineEdit {
                    background-color: #0F0F0F;
                    color: #FFD666;
                    border: 1px solid #444444;
                    border-radius: 6px;
                    padding: 4px 8px;
                }
                """
            )
            editor.textEdited.connect(self.on_threshold_text_edited)

            threshold_layout.addWidget(label, row, col)
            threshold_layout.addWidget(editor, row, col + 1)
            self.threshold_inputs[key] = editor

        self.btn_apply_thresholds = self.make_button("确认阈值生效", "#1677FF")
        self.btn_apply_thresholds.clicked.connect(self.apply_thresholds_from_ui)
        threshold_layout.addWidget(
            self.btn_apply_thresholds,
            (len(self.THRESHOLD_FIELDS) + 1) // 2,
            0,
            1,
            2,
        )

        self.btn_reset_thresholds = self.make_button("重置阈值", "#FA8C16")
        self.btn_reset_thresholds.clicked.connect(self.reset_thresholds)
        threshold_layout.addWidget(
            self.btn_reset_thresholds,
            (len(self.THRESHOLD_FIELDS) + 1) // 2,
            2,
            1,
            2,
        )

        var_group = self.create_group("Variables")
        var_layout = QGridLayout(var_group)
        var_layout.setHorizontalSpacing(10)
        var_layout.setVerticalSpacing(6)

        self.value_labels = {}
        var_names = [
            "dyn_mag",
            "jerk_mag",
            "e_short",
            "e_long",
            "ratio",
            "flip_angle_deg",
            "state",
            "tap_count",
            "packet_count",
        ]

        for i, name in enumerate(var_names):
            r = i // 2
            c = (i % 2) * 2
            name_label = QLabel(name)
            name_label.setFont(QFont("Consolas", 11, QFont.Bold))
            name_label.setStyleSheet("color: #D9D9D9;")
            value_label = QLabel("--")
            value_label.setFont(QFont("Consolas", 11))
            value_label.setStyleSheet("color: #FFD666;")

            var_layout.addWidget(name_label, r, c)
            var_layout.addWidget(value_label, r, c + 1)
            self.value_labels[name] = value_label

        log_group = self.create_group("Log")
        log_layout = QVBoxLayout(log_group)

        self.log_edit = QPlainTextEdit()
        self.log_edit.setReadOnly(True)
        self.log_edit.setFont(QFont("Consolas", 11))
        self.log_edit.setStyleSheet(
            """
            QPlainTextEdit {
                background-color: #0D0D0D;
                color: #C9D1D9;
                border: 1px solid #333333;
                border-radius: 6px;
                padding: 6px;
            }
            """
        )
        log_layout.addWidget(self.log_edit)

        right_layout.addWidget(serial_group, 0)
        right_layout.addWidget(threshold_group, 0)
        right_layout.addWidget(var_group, 0)
        right_layout.addWidget(log_group, 1)

        main_layout.addWidget(left_frame, 7)
        main_layout.addWidget(right_frame, 3)

    def create_group(self, title):
        box = QGroupBox(title)
        box.setFont(QFont("Arial", 13, QFont.Bold))
        box.setStyleSheet(
            """
            QGroupBox {
                color: #FFFFFF;
                border: 1px solid #333333;
                border-radius: 8px;
                margin-top: 10px;
                background-color: #1A1A1A;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 6px 0 6px;
            }
            """
        )
        return box

    def combo_style(self):
        return """
            QComboBox {
                background-color: #0F0F0F;
                color: white;
                border: 1px solid #444444;
                border-radius: 6px;
                padding: 6px;
            }
            QComboBox QAbstractItemView {
                background-color: #111111;
                color: white;
                selection-background-color: #1677FF;
            }
        """

    def make_button(self, text, color):
        btn = QPushButton(text)
        btn.setMinimumHeight(42)
        btn.setFont(QFont("Arial", 12, QFont.Bold))
        btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.set_button_color(btn, color)
        return btn

    def set_button_color(self, button, color):
        button.setStyleSheet(
            f"""
            QPushButton {{
                background-color: {color};
                color: white;
                border: none;
                border-radius: 8px;
                padding: 8px 14px;
            }}
            QPushButton:hover {{
                background-color: #FFFFFF;
                color: black;
            }}
            QPushButton:pressed {{
                padding-top: 10px;
                padding-left: 16px;
            }}
            """
        )

    def update_connection_button(self):
        connected = self.reader is not None
        self.btn_connect_toggle.setText("断开连接" if connected else "连接串口")
        self.set_button_color(
            self.btn_connect_toggle,
            self.DISCONNECT_COLOR if connected else self.CONNECT_COLOR,
        )

    def log(self, text):
        self.log_edit.appendPlainText(text)

    def build_tap_sound(self):
        sample_rate = 22050
        total_samples = int(sample_rate * self.TAP_BEEP_MS / 1000.0)
        buffer = io.BytesIO()
        with wave.open(buffer, "wb") as wav_file:
            wav_file.setnchannels(1)
            wav_file.setsampwidth(2)
            wav_file.setframerate(sample_rate)
            frames = bytearray()
            fade_samples = max(1, int(sample_rate * 0.02))
            for i in range(total_samples):
                t = i / sample_rate
                envelope = 1.0
                if i < fade_samples:
                    envelope = i / fade_samples
                elif total_samples - i < fade_samples:
                    envelope = (total_samples - i) / fade_samples
                sample = int(
                    16000
                    * envelope
                    * math.sin(2.0 * math.pi * self.TAP_BEEP_FREQ * t)
                )
                frames.extend(struct.pack("<h", sample))
            wav_file.writeframes(bytes(frames))
        return buffer.getvalue()

    def play_tap_sound(self):
        try:
            winsound.PlaySound(None, 0)
            winsound.PlaySound(
                self.tap_sound_wav,
                winsound.SND_ASYNC | winsound.SND_MEMORY | winsound.SND_NODEFAULT,
            )
        except RuntimeError:
            pass

    def on_threshold_text_edited(self, _text):
        self.thresholds_dirty = True
        self.btn_apply_thresholds.setText("确认阈值生效*")

    def apply_thresholds_from_ui(self):
        values = {}
        for key, _ in self.THRESHOLD_FIELDS:
            editor = self.threshold_inputs[key]
            text = editor.text().strip()
            try:
                values[key] = float(text)
            except ValueError:
                editor.setText(f"{getattr(self.detector, key):g}")
                self.log(f"[{now_str()}] Invalid threshold for {key}: {text}")
                return

        self.detector.set_thresholds(values)
        self.thresholds_dirty = False
        self.btn_apply_thresholds.setText("确认阈值生效")
        self.log(
            f"[{now_str()}] Thresholds applied | "
            f"dyn={self.detector.TH_DYN:g}, jerk={self.detector.TH_JERK:g}, "
            f"E={self.detector.TH_E_SHORT:g}, ratio={self.detector.TH_RATIO:g}, "
            f"theta={self.detector.TH_FLIP_ANGLE:g}, width_level={self.detector.WIDTH_LEVEL:g}, "
            f"width=[{self.detector.WIDTH_MIN:g}, {self.detector.WIDTH_MAX:g}]"
        )

    def reset_thresholds(self):
        self.detector.reset_thresholds()
        thresholds = self.detector.get_thresholds()
        for key, editor in self.threshold_inputs.items():
            editor.setText(f"{thresholds[key]:g}")
        self.thresholds_dirty = False
        self.btn_apply_thresholds.setText("确认阈值生效")
        self.log(f"[{now_str()}] Thresholds reset to defaults")

    def update_fps(self):
        now = time.monotonic()
        self.frame_times.append(now)
        cutoff = now - 1.0
        while self.frame_times and self.frame_times[0] < cutoff:
            self.frame_times.popleft()

        if len(self.frame_times) >= 2:
            span = self.frame_times[-1] - self.frame_times[0]
            if span > 0:
                self.measured_fps = (len(self.frame_times) - 1) / span
        elif self.frame_times:
            self.measured_fps = 0.0

        self.fps_label.setText(
            "\u76ee\u6807\u5e27\u7387: "
            f"{self.TARGET_FS:.0f} Hz | "
            f"\u5b9e\u9645: {self.measured_fps:.1f} Hz"
        )

    def update_tap_duration(self, duration_ms):
        self.tap_duration_label.setText(f"最近拍打框时长: {duration_ms:.1f} ms")

    def summarize_fail_reason(self, fail_reasons):
        if not fail_reasons:
            return "原因未识别"

        first = fail_reasons[0]
        if first.startswith("peak_jerk="):
            return "主要失败原因: jerk峰值不足"
        if first.startswith("peak_dyn="):
            return "主要失败原因: dyn峰值不足"
        if first.startswith("peak_width="):
            return "主要失败原因: peak_width不在范围内"
        if first.startswith("end_ratio="):
            return "主要失败原因: ratio不足"
        if first.startswith("end_theta="):
            return "主要失败原因: theta超阈值"
        if first.startswith("max_theta="):
            return "主要失败原因: 翻板角度触发取消"
        return f"主要失败原因: {first}"

    def refresh_ports(self):
        self.port_combo.clear()

        ports = list(serial.tools.list_ports.comports())
        matched = []

        for port in ports:
            desc = port.description or ""
            if "USB-SERIAL CH340" in desc:
                matched.append(port)

        if matched:
            for port in matched:
                self.port_combo.addItem(f"{port.device} | {port.description}", port.device)
            self.log(f"[{now_str()}] Found CH340 port(s): {len(matched)}")
        else:
            for port in ports:
                self.port_combo.addItem(f"{port.device} | {port.description}", port.device)
            if ports:
                self.log(f"[{now_str()}] CH340 not found, fallback to all ports")
            else:
                self.log(f"[{now_str()}] No serial ports found")

    def connect_serial(self):
        if self.reader is not None:
            self.log(f"[{now_str()}] Already connected")
            self.update_connection_button()
            return

        if self.port_combo.count() == 0:
            self.log(f"[{now_str()}] No port to connect")
            return

        port = self.port_combo.currentData()
        if not port:
            self.log(f"[{now_str()}] Invalid selected port")
            return

        self.reader = SerialReaderThread(port=port, baud=115200)
        self.reader.sample_received.connect(self.on_sample)
        self.reader.log_message.connect(self.log)
        self.reader.error_message.connect(self.on_serial_error)
        self.reader.start()
        self.update_connection_button()

    def disconnect_serial(self):
        if self.reader is not None:
            self.reader.stop()
            self.reader.wait(1000)
            self.reader = None
            self.log(f"[{now_str()}] Disconnect requested")
        self.update_connection_button()

    def toggle_serial_connection(self):
        if self.reader is None:
            self.connect_serial()
        else:
            self.disconnect_serial()

    def on_serial_error(self, msg):
        self.log(msg)
        self.disconnect_serial()

    def clear_plots(self):
        self.raw_x.clear()
        self.raw_y.clear()
        self.raw_z.clear()
        self.dyn_hist.clear()
        self.jerk_hist.clear()
        self.sample_index = 0
        self.latest = None
        self.curve_x.setData([])
        self.curve_y.setData([])
        self.curve_z.setData([])
        self.curve_dyn.setData([])
        self.curve_jerk.setData([])
        self.tap_duration_label.setText("最近拍打框时长: --")
        self.fps_label.setText(
            "\u76ee\u6807\u5e27\u7387: "
            f"{self.TARGET_FS:.0f} Hz | "
            "\u5b9e\u9645: -- Hz"
        )
        self.log(f"[{now_str()}] Plot buffers cleared")

    def reset_algo(self):
        self.detector.reset()
        self.tap_duration_label.setText("最近拍打框时长: --")
        self.log(f"[{now_str()}] Algorithm reset")

    def on_sample(self, x, y, z, pkt_hex):
        self.sample_index += 1
        self.update_fps()
        self.raw_x.append(x)
        self.raw_y.append(y)
        self.raw_z.append(z)

        result = self.detector.process(x, y, z)
        self.latest = result

        self.dyn_hist.append(result["dyn_mag"])
        self.jerk_hist.append(result["jerk_mag"])

        self.update_labels(result)

        if result["event"] == "TAP":
            self.update_tap_duration(result["tap_duration_ms"])
            self.play_tap_sound()
            self.log(
                f"[{now_str()}] TAP detected | peak_dyn={result['peak_dyn']:.1f}, "
                f"peak_jerk={result['peak_jerk']:.1f}, max_ratio={result['max_ratio']:.2f}, "
                f"max_theta={result['max_theta']:.1f}deg, "
                f"peak_width={int(result['peak_width'])} samples ({result['tap_duration_ms']:.1f} ms), "
                f"count={result['tap_count']}"
            )
        elif result["fail_info"] is not None:
            fail = result["fail_info"]
            reason_text = "\n".join(fail["fail_reasons"]) if fail["fail_reasons"] else "unknown"
            summary = self.summarize_fail_reason(fail["fail_reasons"])
            self.log(
                f"[{now_str()}] fail:\n"
                f"peak_dyn={fail['peak_dyn']:.1f}\n"
                f"peak_jerk={fail['peak_jerk']:.1f}\n"
                f"peak_width={int(fail['peak_width'])}\n"
                f"max_ratio={fail['max_ratio']:.2f}\n"
                f"max_theta={fail['max_theta']:.1f}\n"
                f"{summary}\n"
                f"reason:\n{reason_text}"
            )

    def update_labels(self, values):
        for key, lbl in self.value_labels.items():
            if key not in values:
                continue

            value = values[key]
            if isinstance(value, float):
                lbl.setText(f"{value:.2f}")
            else:
                lbl.setText(str(value))

            if key == "state":
                if value == "IDLE":
                    lbl.setStyleSheet("color: #95DE64;")
                elif value == "CANDIDATE":
                    lbl.setStyleSheet("color: #FFD666;")
                elif value == "REFRACTORY":
                    lbl.setStyleSheet("color: #FF7875;")
                else:
                    lbl.setStyleSheet("color: #FFFFFF;")

    def update_plots(self):
        self.curve_x.setData(list(self.raw_x))
        self.curve_y.setData(list(self.raw_y))
        self.curve_z.setData(list(self.raw_z))

        self.curve_dyn.setData(list(self.dyn_hist))
        self.curve_jerk.setData(list(self.jerk_hist))

        max_feat = 50.0
        if self.dyn_hist:
            max_feat = max(max_feat, max(self.dyn_hist))
        if self.jerk_hist:
            max_feat = max(max_feat, max(self.jerk_hist))

        upper = max(300, min(max_feat * 1.2, 2000))
        self.feature_plot.setYRange(0, upper)

    def closeEvent(self, event):
        self.disconnect_serial()
        super().closeEvent(event)
