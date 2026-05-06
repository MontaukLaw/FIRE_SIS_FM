from collections import deque

import pyqtgraph as pg
from PyQt5.QtWidgets import QVBoxLayout, QWidget

from config import HISTORY


class BaselinePlotPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.history = HISTORY
        self.xs = list(range(-self.history + 1, 1))
        self.result = deque([0.0] * self.history, maxlen=self.history)
        self.baseline = deque([0.0] * self.history, maxlen=self.history)
        self.threshold = deque([0.0] * self.history, maxlen=self.history)
        self.sample = deque([0.0] * self.history, maxlen=self.history)
        self._dirty = False

        self._build_ui()

    def _build_ui(self):
        pg.setConfigOptions(antialias=True, background="#0B0D10", foreground="#E8EDF2")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(12)

        self.signal_plot = self._make_plot("Signal", "Value")
        self.result_curve = self.signal_plot.plot(
            self.xs, list(self.result), pen=pg.mkPen("#5CC8FF", width=2), name="输出"
        )
        self.sample_curve = self.signal_plot.plot(
            self.xs, list(self.sample), pen=pg.mkPen("#7BE495", width=2), name="采样"
        )
        self.baseline_curve = self.signal_plot.plot(
            self.xs, list(self.baseline), pen=pg.mkPen("#F4D35E", width=2), name="基准值"
        )

        self.threshold_plot = self._make_plot("Threshold", "Value")
        self.threshold_curve = self.threshold_plot.plot(
            self.xs, list(self.threshold), pen=pg.mkPen("#FF8A65", width=2), name="触发阈值"
        )

        layout.addWidget(self.signal_plot, 3)
        layout.addWidget(self.threshold_plot, 2)

    def _make_plot(self, title, left_label):
        plot = pg.PlotWidget()
        plot.setTitle(title, color="#E8EDF2", size="13pt")
        plot.setLabel("left", left_label)
        plot.setLabel("bottom", "Samples")
        plot.showGrid(x=True, y=True, alpha=0.22)
        plot.addLegend(offset=(-10, 10))
        plot.setMenuEnabled(False)
        plot.setMouseEnabled(x=False, y=False)
        plot.getPlotItem().getAxis("left").setPen("#59636F")
        plot.getPlotItem().getAxis("bottom").setPen("#59636F")
        return plot

    def add_frame(self, frame):
        self.result.append(frame.result)
        self.baseline.append(frame.baseline)
        self.threshold.append(frame.threshold)
        self.sample.append(frame.sample)
        self._dirty = True

    def update_curves(self):
        if not self._dirty:
            return

        self.result_curve.setData(self.xs, list(self.result))
        self.sample_curve.setData(self.xs, list(self.sample))
        self.baseline_curve.setData(self.xs, list(self.baseline))
        self.threshold_curve.setData(self.xs, list(self.threshold))

        self._set_range(self.signal_plot, [self.result, self.sample, self.baseline])
        self._set_range(self.threshold_plot, [self.threshold])
        self._dirty = False

    def clear(self):
        for series in (self.result, self.baseline, self.threshold, self.sample):
            series.clear()
            series.extend([0.0] * self.history)
        self._dirty = True
        self.update_curves()

    @staticmethod
    def _set_range(plot, series_group):
        values = []
        for series in series_group:
            values.extend(series)

        ymin = min(values)
        ymax = max(values)
        if ymin == ymax:
            ymin -= 1.0
            ymax += 1.0

        pad = (ymax - ymin) * 0.1
        plot.setYRange(ymin - pad, ymax + pad, padding=0)
