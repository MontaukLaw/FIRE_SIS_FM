import serial.tools.list_ports
from PyQt5.QtCore import pyqtSignal
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import (
    QComboBox,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPlainTextEdit,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from config import DEFAULT_BAUD, DEFAULT_PORT


STATE_TEXT = {
    0: "空闲",
    1: "检测中",
    2: "不应期",
}

STATE_COLOR = {
    0: "#BAC4D0",
    1: "#7BE495",
    2: "#FFEAB6",
}


class InfoPanel(QWidget):
    connect_requested = pyqtSignal(str, int)
    disconnect_requested = pyqtSignal()
    refresh_requested = pyqtSignal()
    freeze_toggled = pyqtSignal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.connected = False
        self.frozen = False
        self._build_ui()
        self.refresh_ports()

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(12)

        serial_group = self._make_group("串口控制")
        serial_layout = QVBoxLayout(serial_group)
        serial_layout.setSpacing(10)

        row = QHBoxLayout()
        self.port_combo = QComboBox()
        self.port_combo.setMinimumHeight(34)
        self.port_combo.setStyleSheet(self._combo_style())
        self.refresh_button = self._make_button("刷新", "#EEF3F8", "#17202A")
        self.refresh_button.clicked.connect(self.on_refresh_clicked)
        row.addWidget(self.port_combo, 1)
        row.addWidget(self.refresh_button)

        baud_row = QHBoxLayout()
        baud_label = QLabel("波特率")
        baud_label.setStyleSheet("color: #BAC4D0;")
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["9600", "57600", "115200", "230400", "460800", "921600"])
        self.baud_combo.setCurrentText(str(DEFAULT_BAUD))
        self.baud_combo.setMinimumHeight(34)
        self.baud_combo.setStyleSheet(self._combo_style())
        baud_row.addWidget(baud_label)
        baud_row.addWidget(self.baud_combo, 1)

        self.connect_button = self._make_button("连接", "#DDFBE6", "#12351E")
        self.connect_button.clicked.connect(self.on_connect_clicked)

        self.freeze_button = self._make_button("冻结当前数据", "#EEF3F8", "#17202A")
        self.freeze_button.clicked.connect(self.on_freeze_clicked)

        self.fps_label = QLabel("帧率: -- Hz")
        self.fps_label.setFont(QFont("Consolas", 11, QFont.Bold))
        self.fps_label.setStyleSheet("color: #7BE495; padding: 2px;")

        serial_layout.addLayout(row)
        serial_layout.addLayout(baud_row)
        serial_layout.addWidget(self.connect_button)
        serial_layout.addWidget(self.freeze_button)
        serial_layout.addWidget(self.fps_label)

        info_group = self._make_group("实时信息")
        info_layout = QGridLayout(info_group)
        info_layout.setHorizontalSpacing(8)
        info_layout.setVerticalSpacing(8)

        self.value_labels = {}
        fields = [
            ("sample", "采样"),
            ("baseline", "基准值"),
            ("threshold", "触发阈值"),
            ("result", "输出"),
        ]
        for row_index, (key, title) in enumerate(fields):
            title_label = QLabel(title)
            title_label.setStyleSheet("color: #BAC4D0;")
            value_label = QLabel("--")
            value_label.setFont(QFont("Consolas", 12, QFont.Bold))
            value_label.setStyleSheet("color: #F5F7FA;")
            info_layout.addWidget(title_label, row_index, 0)
            info_layout.addWidget(value_label, row_index, 1)
            self.value_labels[key] = value_label

        state_title = QLabel("当前状态")
        state_title.setStyleSheet("color: #BAC4D0;")
        self.state_label = QLabel("--")
        self.state_label.setFont(QFont("Microsoft YaHei", 24, QFont.Bold))
        self.state_label.setStyleSheet("color: #BAC4D0; padding: 8px 0;")
        state_row = len(fields)
        info_layout.addWidget(state_title, state_row, 0)
        info_layout.addWidget(self.state_label, state_row, 1)

        log_group = self._make_group("Log")
        log_layout = QVBoxLayout(log_group)
        self.log_box = QPlainTextEdit()
        self.log_box.setReadOnly(True)
        self.log_box.setMaximumBlockCount(500)
        self.log_box.setStyleSheet(
            """
            QPlainTextEdit {
                background: #080A0D;
                color: #DDE6F0;
                border: 1px solid #26313D;
                border-radius: 6px;
                padding: 8px;
            }
            """
        )
        log_layout.addWidget(self.log_box)

        layout.addWidget(serial_group, 0)
        layout.addWidget(info_group, 0)
        layout.addWidget(log_group, 1)

    def refresh_ports(self):
        current = self.port_combo.currentText() or DEFAULT_PORT
        self.port_combo.clear()
        ports = [port.device for port in serial.tools.list_ports.comports()]
        if not ports:
            ports = [DEFAULT_PORT]
        self.port_combo.addItems(ports)
        if current in ports:
            self.port_combo.setCurrentText(current)

    def set_connected(self, connected):
        self.connected = connected
        self.port_combo.setEnabled(not connected)
        self.baud_combo.setEnabled(not connected)
        self.refresh_button.setEnabled(not connected)
        self.connect_button.setText("断开" if connected else "连接")
        color = "#FFE2E2" if connected else "#DDFBE6"
        text = "#4D1515" if connected else "#12351E"
        self.connect_button.setStyleSheet(self._button_style(color, text))

    def update_frame(self, frame):
        self.value_labels["sample"].setText(f"{frame.sample:.3f}")
        self.value_labels["baseline"].setText(f"{frame.baseline:.3f}")
        self.value_labels["threshold"].setText(f"{frame.threshold:.3f}")
        self.value_labels["result"].setText(f"{frame.result:.3f}")
        state = int(round(frame.state))
        state_text = STATE_TEXT.get(state, f"未知 {state}")
        state_color = STATE_COLOR.get(state, "#FF8A65")
        self.state_label.setText(state_text)
        self.state_label.setStyleSheet(f"color: {state_color}; padding: 8px 0;")

    def update_fps(self, fps):
        self.fps_label.setText(f"帧率: {fps:.1f} Hz")

    def append_log(self, message):
        self.log_box.appendPlainText(message)

    def on_refresh_clicked(self):
        self.refresh_ports()
        self.refresh_requested.emit()

    def on_connect_clicked(self):
        if self.connected:
            self.disconnect_requested.emit()
            return

        try:
            baud = int(self.baud_combo.currentText())
        except ValueError:
            baud = DEFAULT_BAUD
        self.connect_requested.emit(self.port_combo.currentText(), baud)

    def on_freeze_clicked(self):
        self.frozen = not self.frozen
        self.freeze_button.setText("解冻数据" if self.frozen else "冻结当前数据")
        color = "#FFEAB6" if self.frozen else "#EEF3F8"
        text = "#4A3210" if self.frozen else "#17202A"
        self.freeze_button.setStyleSheet(self._button_style(color, text))
        self.freeze_toggled.emit(self.frozen)

    def _make_group(self, title):
        group = QGroupBox(title)
        group.setStyleSheet(
            """
            QGroupBox {
                color: #E8EDF2;
                border: 1px solid #26313D;
                border-radius: 8px;
                margin-top: 12px;
                background: #11161C;
                font-weight: 600;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px;
            }
            """
        )
        return group

    def _make_button(self, text, background, color):
        button = QPushButton(text)
        button.setMinimumHeight(36)
        button.setFont(QFont("Microsoft YaHei", 10, QFont.Bold))
        button.setStyleSheet(self._button_style(background, color))
        return button

    @staticmethod
    def _button_style(background, color):
        return f"""
            QPushButton {{
                background: {background};
                color: {color};
                border: 0;
                border-radius: 6px;
                padding: 7px 12px;
            }}
            QPushButton:hover {{
                background: #FFFFFF;
            }}
            QPushButton:disabled {{
                background: #313B46;
                color: #7C8794;
            }}
        """

    @staticmethod
    def _combo_style():
        return """
            QComboBox {
                background: #080A0D;
                color: #F5F7FA;
                border: 1px solid #344252;
                border-radius: 6px;
                padding: 5px 8px;
            }
            QComboBox:disabled {
                color: #778391;
                background: #151B22;
            }
        """
