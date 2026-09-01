import csv
import random
import re
import sys

from datetime import datetime

import serial
import serial.tools.list_ports

from PySide6.QtCore import QTimer, Qt
from PySide6.QtGui import QColor, QFont, QPainter, QPainterPath, QPen
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QFileDialog,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPlainTextEdit,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)


GREEN = "#00ff41"
MID_GREEN = "#00aa33"
DARK_GREEN = "#004d16"
BLACK = "#000000"


class MetricCard(QFrame):
    def __init__(self, title, value="--"):
        super().__init__()

        self.setObjectName("metricCard")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 10, 12, 10)
        layout.setSpacing(4)

        title_label = QLabel(title)
        title_label.setObjectName("metricTitle")

        self.value_label = QLabel(value)
        self.value_label.setObjectName("metricValue")

        layout.addWidget(title_label)
        layout.addWidget(self.value_label)

    def set_value(self, value, state=None):
        self.value_label.setText(str(value))

        if state == "fail":
            color = "#ff3b3b"
        else:
            color = GREEN

        self.value_label.setStyleSheet(
            f"""
            color: {color};
            font-size: 19pt;
            font-weight: bold;
            """
        )


class ScanlineOverlay(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setAttribute(
            Qt.WidgetAttribute.WA_TransparentForMouseEvents,
            True
        )

        self.setAttribute(
            Qt.WidgetAttribute.WA_TranslucentBackground,
            True
        )

    def paintEvent(self, event):
        painter = QPainter(self)

        painter.setPen(
            QPen(
                QColor(0, 0, 0, 40),
                1
            )
        )

        for y in range(0, self.height(), 4):
            painter.drawLine(
                0,
                y,
                self.width(),
                y
            )


class RTTGraph(QFrame):
    def __init__(self):
        super().__init__()

        self.values = []

        self.setMinimumHeight(180)

    def clear(self):
        self.values.clear()
        self.update()

    def add_value(self, value):
        self.values.append(value)

        if len(self.values) > 120:
            self.values.pop(0)

        self.update()

    def paintEvent(self, event):
        super().paintEvent(event)

        painter = QPainter(self)

        painter.setRenderHint(
            QPainter.RenderHint.Antialiasing,
            True
        )

        painter.fillRect(
            self.rect(),
            QColor(BLACK)
        )

        painter.setPen(
            QPen(
                QColor(DARK_GREEN),
                1
            )
        )

        painter.drawRect(
            self.rect().adjusted(
                0,
                0,
                -1,
                -1
            )
        )

        painter.setFont(
            QFont(
                "Consolas",
                9,
                QFont.Weight.Bold
            )
        )

        painter.setPen(
            QColor(GREEN)
        )

        painter.drawText(
            12,
            20,
            "RTT TREND // us"
        )

        left = 12
        right = self.width() - 12
        top = 35
        bottom = self.height() - 20

        painter.setPen(
            QPen(
                QColor(0, 120, 30, 90),
                1
            )
        )

        for i in range(5):
            y = (
                top
                + (bottom - top)
                * i
                / 4
            )

            painter.drawLine(
                left,
                int(y),
                right,
                int(y)
            )

        if not self.values:
            painter.setPen(
                QColor(MID_GREEN)
            )

            painter.drawText(
                left,
                int((top + bottom) / 2),
                "WAITING FOR RTT DATA..."
            )

            return

        maximum = max(
            max(self.values),
            1
        )

        path = QPainterPath()

        count = len(self.values)

        for index, value in enumerate(self.values):
            if count == 1:
                x = left
            else:
                x = (
                    left
                    + (right - left)
                    * index
                    / (count - 1)
                )

            y = (
                bottom
                - (value / maximum)
                * (bottom - top)
            )

            if index == 0:
                path.moveTo(x, y)
            else:
                path.lineTo(x, y)

        painter.setPen(
            QPen(
                QColor(GREEN),
                2
            )
        )

        painter.drawPath(path)

        painter.setPen(
            QColor(GREEN)
        )

        painter.drawText(
            left,
            self.height() - 4,
            f"LATEST {self.values[-1]} us"
        )

        painter.drawText(
            max(left, right - 130),
            self.height() - 4,
            f"MAX {maximum} us"
        )


class HorizontalBarGraph(QFrame):
    def __init__(self, title, labels):
        super().__init__()

        self.title = title
        self.labels = labels

        self.values = {
            label: 0
            for label in labels
        }

        self.setMinimumHeight(180)

    def clear(self):
        for label in self.labels:
            self.values[label] = 0

        self.update()

    def set_value(self, label, value):
        if label in self.values:
            self.values[label] = value

        self.update()

    def paintEvent(self, event):
        super().paintEvent(event)

        painter = QPainter(self)

        painter.setRenderHint(
            QPainter.RenderHint.Antialiasing,
            True
        )

        painter.fillRect(
            self.rect(),
            QColor(BLACK)
        )

        painter.setPen(
            QPen(
                QColor(DARK_GREEN),
                1
            )
        )

        painter.drawRect(
            self.rect().adjusted(
                0,
                0,
                -1,
                -1
            )
        )

        painter.setFont(
            QFont(
                "Consolas",
                9,
                QFont.Weight.Bold
            )
        )

        painter.setPen(
            QColor(GREEN)
        )

        painter.drawText(
            12,
            20,
            self.title
        )

        maximum = max(
            max(self.values.values()),
            1
        )

        start_y = 38
        available_height = (
            self.height()
            - start_y
            - 8
        )

        row_height = (
            available_height
            / max(len(self.labels), 1)
        )

        label_width = 80
        bar_start = 12 + label_width
        bar_right = self.width() - 42

        for index, label in enumerate(self.labels):
            value = self.values[label]

            y = (
                start_y
                + index * row_height
            )

            painter.setPen(
                QColor(MID_GREEN)
            )

            painter.drawText(
                12,
                int(
                    y
                    + row_height * 0.65
                ),
                label.upper()
            )

            total_width = max(
                bar_right - bar_start,
                1
            )

            width = (
                total_width
                * value
                / maximum
            )

            bar_height = max(
                int(row_height - 10),
                4
            )

            painter.setPen(
                QPen(
                    QColor(DARK_GREEN),
                    1
                )
            )

            painter.drawRect(
                int(bar_start),
                int(y + 5),
                int(total_width),
                bar_height
            )

            painter.fillRect(
                int(bar_start),
                int(y + 5),
                int(width),
                bar_height,
                QColor(0, 255, 65, 170)
            )

            painter.setPen(
                QColor(GREEN)
            )

            painter.drawText(
                int(bar_right + 5),
                int(
                    y
                    + row_height * 0.65
                ),
                str(value)
            )


class GlitchGoblinGUI(QMainWindow):
    def __init__(self):
        super().__init__()

        self.serial_port = None
        self.serial_buffer = ""

        self.metric_data = {
            "delivery_pct": None,
            "recovery_pct": None,
            "avg_rtt_us": None,
            "max_rtt_us": None,
            "min_rtt_us": None,
            "successful": None,
            "failed": None,
            "retries": None,
            "timeouts": None,
            "verdict": None,
            "normal": 0,
            "crc_faults": 0,
            "drops": 0,
            "duplicates": 0,
            "delays": 0,
        }

        self.setWindowTitle(
            "Glitch Goblin V2"
        )

        self.resize(
            1400,
            900
        )

        self.apply_theme()

        self.build_ui()

        self.refresh_ports()

        self.start_boot_animation()
        self.start_glitch_animation()

        self.serial_timer = QTimer(self)

        self.serial_timer.timeout.connect(
            self.read_serial
        )

        self.serial_timer.start(20)

    def apply_theme(self):
        self.setStyleSheet(
            """
            QMainWindow {
                background-color: #000000;
            }

            QWidget {
                background-color: #000000;
                color: #00ff41;
                font-family: "Consolas", "Cascadia Mono",
                             "Courier New", monospace;
                font-size: 10.5pt;
            }

            QFrame#headerFrame {
                border: 1px solid #007a22;
                border-radius: 4px;
            }

            QLabel#titleLabel {
                color: #00ff41;
                font-size: 25pt;
                font-weight: bold;
            }

            QLabel#subtitleLabel {
                color: #00a92f;
                font-size: 9pt;
            }

            QLabel#statusLabel {
                color: #00a92f;
                border: 1px solid #006b1d;
                border-radius: 3px;
                padding: 7px 12px;
                font-weight: bold;
            }

            QGroupBox {
                border: 1px solid #006b1d;
                border-radius: 3px;
                margin-top: 13px;
                padding-top: 12px;
                font-weight: bold;
                color: #00ff41;
            }

            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0px 5px;
                color: #00ff41;
            }

            QPushButton {
                background-color: #001306;
                color: #00ff41;
                border: 1px solid #007a22;
                border-radius: 2px;
                padding: 9px;
                font-weight: bold;
            }

            QPushButton:hover {
                background-color: #00220a;
                border: 1px solid #00ff41;
            }

            QPushButton:pressed {
                background-color: #003c10;
            }

            QLineEdit,
            QComboBox,
            QSpinBox {
                background-color: #000000;
                color: #00ff41;
                border: 1px solid #006b1d;
                border-radius: 2px;
                padding: 8px;
                selection-background-color: #007a22;
            }

            QComboBox QAbstractItemView {
                background-color: #000000;
                color: #00ff41;
                border: 1px solid #007a22;
                selection-background-color: #003c10;
            }

            QPlainTextEdit {
                background-color: #000000;
                color: #00ff41;
                border: 1px solid #006b1d;
                border-radius: 2px;
                padding: 10px;
                font-family: "Consolas", "Cascadia Mono", monospace;
                font-size: 10.5pt;
            }

            QFrame#metricCard {
                border: 1px solid #006b1d;
                border-radius: 3px;
            }

            QLabel#metricTitle {
                color: #008f28;
                font-size: 8.5pt;
                font-weight: bold;
            }

            QLabel#metricValue {
                color: #00ff41;
                font-size: 19pt;
                font-weight: bold;
            }

            QLabel#sectionLabel {
                color: #00ff41;
                font-size: 10pt;
                font-weight: bold;
            }

            QScrollBar:vertical {
                background: #000000;
                width: 10px;
            }

            QScrollBar::handle:vertical {
                background: #007a22;
                min-height: 20px;
            }

            QScrollBar::handle:vertical:hover {
                background: #00ff41;
            }

            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {
                height: 0px;
            }
            """
        )

    def build_ui(self):
        root = QWidget()

        main_layout = QVBoxLayout(root)

        main_layout.setContentsMargins(
            14,
            14,
            14,
            14
        )

        main_layout.setSpacing(12)

        # IMPORTANT:
        # Terminal is created BEFORE build_command_box()
        # because the CLEAR button references self.terminal.
        self.terminal = QPlainTextEdit()

        self.terminal.setReadOnly(True)

        self.terminal.setFont(
            QFont(
                "Consolas",
                10
            )
        )

        main_layout.addWidget(
            self.build_header()
        )

        body = QHBoxLayout()

        body.setSpacing(12)

        left = QVBoxLayout()

        left.setSpacing(12)

        left.addWidget(
            self.build_connection_box()
        )

        left.addWidget(
            self.build_test_box()
        )

        left.addWidget(
            self.build_command_box()
        )

        left.addStretch()

        right = QVBoxLayout()

        right.setSpacing(12)

        right.addWidget(
            self.build_metrics_box()
        )

        right.addWidget(
            self.build_graphs_box()
        )

        terminal_label = QLabel(
            "LIVE SERIAL OUTPUT // TARGET STREAM"
        )

        terminal_label.setObjectName(
            "sectionLabel"
        )

        right.addWidget(
            terminal_label
        )

        right.addWidget(
            self.terminal,
            1
        )

        body.addLayout(
            left,
            0
        )

        body.addLayout(
            right,
            1
        )

        main_layout.addLayout(
            body,
            1
        )

        self.setCentralWidget(root)

        self.scanlines = ScanlineOverlay(root)

        self.scanlines.setGeometry(
            root.rect()
        )

        self.scanlines.raise_()

    def build_header(self):
        frame = QFrame()

        frame.setObjectName(
            "headerFrame"
        )

        layout = QHBoxLayout(frame)

        layout.setContentsMargins(
            16,
            12,
            16,
            12
        )

        left = QVBoxLayout()

        left.setSpacing(2)

        self.title_label = QLabel(
            "GLITCH GOBLIN // V2"
        )

        self.title_label.setObjectName(
            "titleLabel"
        )

        subtitle = QLabel(
            "SERUM LINK CONTROL // FAULT INJECTION // TARGET VALIDATION"
        )

        subtitle.setObjectName(
            "subtitleLabel"
        )

        left.addWidget(
            self.title_label
        )

        left.addWidget(
            subtitle
        )

        self.status_label = QLabel(
            "OFFLINE"
        )

        self.status_label.setObjectName(
            "statusLabel"
        )

        self.status_label.setAlignment(
            Qt.AlignmentFlag.AlignCenter
        )

        self.status_label.setMinimumWidth(
            175
        )

        layout.addLayout(left)

        layout.addStretch()

        layout.addWidget(
            self.status_label
        )

        return frame

    def build_connection_box(self):
        box = QGroupBox(
            "TARGET LINK"
        )

        layout = QHBoxLayout(box)

        self.port_box = QComboBox()

        refresh_button = QPushButton(
            "SCAN"
        )

        self.connect_button = QPushButton(
            "CONNECT"
        )

        refresh_button.clicked.connect(
            self.refresh_ports
        )

        self.connect_button.clicked.connect(
            self.toggle_connection
        )

        layout.addWidget(
            QLabel("PORT")
        )

        layout.addWidget(
            self.port_box,
            1
        )

        layout.addWidget(
            refresh_button
        )

        layout.addWidget(
            self.connect_button
        )

        return box

    def make_test_button(self, name, command):
        button = QPushButton(name)

        button.clicked.connect(
            lambda:
            self.send_command(command)
        )

        return button

    def build_test_box(self):
        box = QGroupBox(
            "SERUM OPERATIONS"
        )

        layout = QGridLayout(box)

        layout.setSpacing(8)

        tests = [
            ("NORMAL", "normal"),
            ("CRC FAULT", "crc"),
            ("DROP", "drop"),
            ("DUPLICATE", "duplicate"),
            ("DELAY", "delay"),
            ("RELIABLE", "reliable"),
            ("REPLAY", "replay"),
            ("STM32 STATS", "stats"),
        ]

        for index, (name, command) in enumerate(tests):
            button = self.make_test_button(
                name,
                command
            )

            layout.addWidget(
                button,
                index // 4,
                index % 4
            )

        self.bench_count = QSpinBox()

        self.bench_count.setRange(
            1,
            10000
        )

        self.bench_count.setValue(100)

        bench_button = QPushButton(
            "RUN BENCH"
        )

        bench_button.clicked.connect(
            self.run_benchmark
        )

        self.chaos_count = QSpinBox()

        self.chaos_count.setRange(
            1,
            10000
        )

        self.chaos_count.setValue(100)

        chaos_button = QPushButton(
            "RUN CHAOS"
        )

        chaos_button.clicked.connect(
            self.run_chaos
        )

        layout.addWidget(
            QLabel("BENCH PACKETS"),
            2,
            0
        )

        layout.addWidget(
            self.bench_count,
            2,
            1
        )

        layout.addWidget(
            bench_button,
            2,
            2,
            1,
            2
        )

        layout.addWidget(
            QLabel("CHAOS PACKETS"),
            3,
            0
        )

        layout.addWidget(
            self.chaos_count,
            3,
            1
        )

        layout.addWidget(
            chaos_button,
            3,
            2,
            1,
            2
        )

        return box

    def build_command_box(self):
        box = QGroupBox(
            "COMMAND TERMINAL"
        )

        layout = QVBoxLayout(box)

        command_row = QHBoxLayout()

        self.command_input = QLineEdit()

        self.command_input.setPlaceholderText(
            "faults 60 10 15 10 5"
        )

        self.command_input.returnPressed.connect(
            self.send_manual_command
        )

        send_button = QPushButton(
            "EXECUTE"
        )

        send_button.clicked.connect(
            self.send_manual_command
        )

        command_row.addWidget(
            self.command_input,
            1
        )

        command_row.addWidget(
            send_button
        )

        utility_row = QGridLayout()

        config_button = QPushButton(
            "CONFIG"
        )

        help_button = QPushButton(
            "HELP"
        )

        clear_button = QPushButton(
            "CLEAR"
        )

        csv_button = QPushButton(
            "EXPORT CSV"
        )

        report_button = QPushButton(
            "SAVE REPORT"
        )

        config_button.clicked.connect(
            lambda:
            self.send_command("config")
        )

        help_button.clicked.connect(
            lambda:
            self.send_command("help")
        )

        clear_button.clicked.connect(
            self.terminal.clear
        )

        csv_button.clicked.connect(
            self.export_csv
        )

        report_button.clicked.connect(
            self.save_report
        )

        utility_row.addWidget(
            config_button,
            0,
            0
        )

        utility_row.addWidget(
            help_button,
            0,
            1
        )

        utility_row.addWidget(
            clear_button,
            0,
            2
        )

        utility_row.addWidget(
            csv_button,
            1,
            0
        )

        utility_row.addWidget(
            report_button,
            1,
            1,
            1,
            2
        )

        layout.addLayout(
            command_row
        )

        layout.addLayout(
            utility_row
        )

        return box

    def build_metrics_box(self):
        box = QGroupBox(
            "LIVE TEST METRICS"
        )

        layout = QGridLayout(box)

        layout.setSpacing(8)

        self.card_delivery = MetricCard(
            "DELIVERY"
        )

        self.card_recovery = MetricCard(
            "RECOVERY"
        )

        self.card_avg_rtt = MetricCard(
            "AVG RTT"
        )

        self.card_max_rtt = MetricCard(
            "MAX RTT"
        )

        self.card_success = MetricCard(
            "SUCCESS"
        )

        self.card_failed = MetricCard(
            "FAILED"
        )

        self.card_retries = MetricCard(
            "RETRIES"
        )

        self.card_timeouts = MetricCard(
            "TIMEOUTS"
        )

        self.card_verdict = MetricCard(
            "VERDICT"
        )

        cards = [
            self.card_delivery,
            self.card_recovery,
            self.card_avg_rtt,
            self.card_max_rtt,
            self.card_success,
            self.card_failed,
            self.card_retries,
            self.card_timeouts,
            self.card_verdict,
        ]

        for index, card in enumerate(cards):
            layout.addWidget(
                card,
                index // 3,
                index % 3
            )

        return box

    def build_graphs_box(self):
        box = QGroupBox(
            "LIVE ANALYTICS"
        )

        layout = QGridLayout(box)

        layout.setSpacing(8)

        self.rtt_graph = RTTGraph()

        self.retry_graph = HorizontalBarGraph(
            "RELIABILITY EVENTS",
            [
                "Retries",
                "Timeouts"
            ]
        )

        self.fault_graph = HorizontalBarGraph(
            "FAULT DISTRIBUTION",
            [
                "Normal",
                "CRC",
                "Drop",
                "Duplicate",
                "Delay"
            ]
        )

        layout.addWidget(
            self.rtt_graph,
            0,
            0
        )

        layout.addWidget(
            self.retry_graph,
            0,
            1
        )

        layout.addWidget(
            self.fault_graph,
            0,
            2
        )

        layout.setColumnStretch(
            0,
            2
        )

        layout.setColumnStretch(
            1,
            1
        )

        layout.setColumnStretch(
            2,
            1
        )

        return box

    def start_boot_animation(self):
        self.boot_lines = [
            "[BOOT] GLITCH GOBLIN V2",
            "Attaching target interface...",
            "Scanning serial transport...",
            "Loading SERUM protocol stack...",
            "Initializing fault engine...",
            "Initializing telemetry parser...",
            "Arming replay protection monitor...",
            "Preparing reliability analyzer...",
            "Loading chaos test controller...",
            "System ready.",
        ]

        self.boot_index = 0

        self.boot_timer = QTimer(self)

        self.boot_timer.timeout.connect(
            self.boot_step
        )

        self.boot_timer.start(150)

    def boot_step(self):
        if self.boot_index >= len(self.boot_lines):
            self.boot_timer.stop()

            self.log("")

            return

        line = self.boot_lines[
            self.boot_index
        ]

        if 3 <= self.boot_index <= 8:
            line += "  OK"

        self.log(line)

        self.boot_index += 1

    def start_glitch_animation(self):
        self.glitch_timer = QTimer(self)

        self.glitch_timer.timeout.connect(
            self.glitch_title
        )

        self.glitch_timer.start(850)

    def glitch_title(self):
        if random.random() > 0.30:
            return

        variants = [
            "GL1TCH GOBLIN // V2",
            "GLITCH G0BLIN // V2",
            "GLITCH GOBLIN // V2_",
            "GLITCH GOBLIN // V2 ▓",
            "GL!TCH GOBLIN // V2",
            "GLITCH_GOBLIN // V2",
        ]

        self.title_label.setText(
            random.choice(variants)
        )

        QTimer.singleShot(
            random.randint(
                45,
                110
            ),
            self.restore_title
        )

    def restore_title(self):
        self.title_label.setText(
            "GLITCH GOBLIN // V2"
        )

    def refresh_ports(self):
        current = ""

        if hasattr(self, "port_box"):
            current = self.port_box.currentText()

            self.port_box.clear()

        ports = (
            serial.tools
            .list_ports
            .comports()
        )

        for port in ports:
            self.port_box.addItem(
                port.device
            )

        if current:
            index = self.port_box.findText(
                current
            )

            if index >= 0:
                self.port_box.setCurrentIndex(
                    index
                )

    def toggle_connection(self):
        if self.serial_port:
            self.disconnect_serial()
        else:
            self.connect_serial()

    def connect_serial(self):
        port = self.port_box.currentText()

        if not port:
            self.log(
                "ERROR: no target port selected"
            )

            return

        try:
            self.status_label.setText(
                "CONNECTING..."
            )

            self.log(
                f"Connecting to target {port}..."
            )

            self.serial_port = serial.Serial(
                port=port,
                baudrate=115200,
                timeout=0
            )

            self.connect_button.setText(
                "DISCONNECT"
            )

            self.status_label.setText(
                f"ONLINE // {port}"
            )

            self.status_label.setStyleSheet(
                """
                color: #00ff41;
                background-color: #001b07;
                border: 1px solid #00ff41;
                border-radius: 3px;
                padding: 7px 12px;
                font-weight: bold;
                """
            )

            self.log(
                "Target link established... SUCCESS"
            )

            self.log(
                "SERUM transport ready."
            )

        except serial.SerialException as error:
            self.serial_port = None

            self.status_label.setText(
                "OFFLINE"
            )

            self.log(
                f"LINK ERROR: {error}"
            )

    def disconnect_serial(self):
        if self.serial_port:
            self.serial_port.close()

        self.serial_port = None

        self.connect_button.setText(
            "CONNECT"
        )

        self.status_label.setText(
            "OFFLINE"
        )

        self.status_label.setStyleSheet(
            """
            color: #00a92f;
            background-color: #000000;
            border: 1px solid #006b1d;
            border-radius: 3px;
            padding: 7px 12px;
            font-weight: bold;
            """
        )

        self.log(
            "Target link closed."
        )

    def send_command(self, command):
        if not self.serial_port:
            self.log(
                "ERROR: target not connected"
            )

            return

        data = (
            command
            + "\n"
        ).encode()

        self.serial_port.write(data)

        self.log(
            f"> {command}"
        )

    def reset_test_visuals(self):
        self.rtt_graph.clear()
        self.retry_graph.clear()
        self.fault_graph.clear()

        self.metric_data.update(
            {
                "delivery_pct": None,
                "recovery_pct": None,
                "avg_rtt_us": None,
                "max_rtt_us": None,
                "min_rtt_us": None,
                "successful": None,
                "failed": None,
                "retries": None,
                "timeouts": None,
                "verdict": None,
                "normal": 0,
                "crc_faults": 0,
                "drops": 0,
                "duplicates": 0,
                "delays": 0,
            }
        )

        cards = [
            self.card_delivery,
            self.card_recovery,
            self.card_avg_rtt,
            self.card_max_rtt,
            self.card_success,
            self.card_failed,
            self.card_retries,
            self.card_timeouts,
            self.card_verdict,
        ]

        for card in cards:
            card.set_value("--")

    def run_benchmark(self):
        self.reset_test_visuals()

        count = self.bench_count.value()

        self.send_command(
            f"bench {count}"
        )

    def run_chaos(self):
        self.reset_test_visuals()

        count = self.chaos_count.value()

        self.send_command(
            f"chaos {count}"
        )

    def send_manual_command(self):
        command = (
            self.command_input
            .text()
            .strip()
        )

        if not command:
            return

        self.send_command(command)

        self.command_input.clear()

    def read_serial(self):
        if not self.serial_port:
            return

        try:
            waiting = (
                self.serial_port
                .in_waiting
            )

            if waiting == 0:
                return

            data = (
                self.serial_port
                .read(waiting)
            )

            text = data.decode(
                errors="replace"
            )

            self.terminal.insertPlainText(
                text
            )

            self.serial_buffer += text

            self.process_serial_lines()

            scrollbar = (
                self.terminal
                .verticalScrollBar()
            )

            scrollbar.setValue(
                scrollbar.maximum()
            )

        except serial.SerialException:
            self.log(
                "LINK LOST"
            )

            self.disconnect_serial()

    def process_serial_lines(self):
        while "\n" in self.serial_buffer:
            line, self.serial_buffer = (
                self.serial_buffer.split(
                    "\n",
                    1
                )
            )

            self.parse_serial_line(
                line.strip()
            )

    def parse_serial_line(self, line):
        if not line:
            return

        match = re.search(
            r"^RTT:\s*(\d+)\s*us",
            line
        )

        if match:
            self.rtt_graph.add_value(
                int(match.group(1))
            )

        match = re.search(
            r"Delivery:\s*([0-9.]+)%",
            line
        )

        if match:
            value = float(match.group(1))

            self.metric_data[
                "delivery_pct"
            ] = value

            self.card_delivery.set_value(
                f"{value:.2f}%"
            )

        match = re.search(
            r"Recovery rate:\s*([0-9.]+)%",
            line
        )

        if match:
            value = float(match.group(1))

            self.metric_data[
                "recovery_pct"
            ] = value

            self.card_recovery.set_value(
                f"{value:.2f}%"
            )

        match = re.search(
            r"Average RTT:\s*(\d+)\s*us",
            line
        )

        if match:
            value = int(match.group(1))

            self.metric_data[
                "avg_rtt_us"
            ] = value

            self.card_avg_rtt.set_value(
                f"{value} us"
            )

        match = re.search(
            r"Max RTT:\s*(\d+)\s*us",
            line
        )

        if match:
            value = int(match.group(1))

            self.metric_data[
                "max_rtt_us"
            ] = value

            self.card_max_rtt.set_value(
                f"{value} us"
            )

        match = re.search(
            r"Min RTT:\s*(\d+)\s*us",
            line
        )

        if match:
            self.metric_data[
                "min_rtt_us"
            ] = int(
                match.group(1)
            )

        match = re.search(
            r"^Successful:\s*(\d+)",
            line
        )

        if match:
            value = int(match.group(1))

            self.metric_data[
                "successful"
            ] = value

            self.card_success.set_value(
                value
            )

        match = re.search(
            r"^Failed:\s*(\d+)",
            line
        )

        if match:
            value = int(match.group(1))

            self.metric_data[
                "failed"
            ] = value

            self.card_failed.set_value(
                value
            )

        match = re.search(
            r"^Retries:\s*(\d+)",
            line
        )

        if match:
            value = int(match.group(1))

            self.metric_data[
                "retries"
            ] = value

            self.card_retries.set_value(
                value
            )

            self.retry_graph.set_value(
                "Retries",
                value
            )

        match = re.search(
            r"^Timeouts:\s*(\d+)",
            line
        )

        if match:
            value = int(match.group(1))

            self.metric_data[
                "timeouts"
            ] = value

            self.card_timeouts.set_value(
                value
            )

            self.retry_graph.set_value(
                "Timeouts",
                value
            )

        fault_patterns = {
            "normal": (
                r"^Normal:\s*(\d+)",
                "Normal"
            ),
            "crc_faults": (
                r"^CRC faults:\s*(\d+)",
                "CRC"
            ),
            "drops": (
                r"^Drops:\s*(\d+)",
                "Drop"
            ),
            "duplicates": (
                r"^Duplicates:\s*(\d+)",
                "Duplicate"
            ),
            "delays": (
                r"^Delays:\s*(\d+)",
                "Delay"
            ),
        }

        for key, (
            pattern,
            graph_label
        ) in fault_patterns.items():
            match = re.search(
                pattern,
                line
            )

            if match:
                value = int(
                    match.group(1)
                )

                self.metric_data[
                    key
                ] = value

                self.fault_graph.set_value(
                    graph_label,
                    value
                )

        match = re.search(
            r"RESULT:\s*(PASS|FAIL)",
            line
        )

        if match:
            verdict = match.group(1)

            self.metric_data[
                "verdict"
            ] = verdict

            if verdict == "PASS":
                self.card_verdict.set_value(
                    "PASS",
                    "pass"
                )
            else:
                self.card_verdict.set_value(
                    "FAIL",
                    "fail"
                )

    def export_csv(self):
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Export Glitch Goblin CSV",
            "glitch_goblin_results.csv",
            "CSV Files (*.csv)"
        )

        if not path:
            return

        row = {
            "timestamp":
                datetime.now().isoformat(
                    timespec="seconds"
                ),
            **self.metric_data
        }

        try:
            with open(
                path,
                "w",
                newline="",
                encoding="utf-8"
            ) as file:
                writer = csv.DictWriter(
                    file,
                    fieldnames=row.keys()
                )

                writer.writeheader()

                writer.writerow(row)

            self.log(
                f"CSV exported: {path}"
            )

        except OSError as error:
            self.log(
                f"CSV ERROR: {error}"
            )

    def save_report(self):
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Save Glitch Goblin Report",
            "glitch_goblin_report.txt",
            "Text Files (*.txt)"
        )

        if not path:
            return

        report = (
            "GLITCH GOBLIN V2 TEST REPORT\n"
            "================================\n"
            f"Timestamp: "
            f"{datetime.now().isoformat(timespec='seconds')}\n\n"

            f"Delivery: "
            f"{self.metric_data['delivery_pct']}%\n"

            f"Recovery: "
            f"{self.metric_data['recovery_pct']}%\n"

            f"Successful: "
            f"{self.metric_data['successful']}\n"

            f"Failed: "
            f"{self.metric_data['failed']}\n"

            f"Retries: "
            f"{self.metric_data['retries']}\n"

            f"Timeouts: "
            f"{self.metric_data['timeouts']}\n"

            f"Min RTT: "
            f"{self.metric_data['min_rtt_us']} us\n"

            f"Average RTT: "
            f"{self.metric_data['avg_rtt_us']} us\n"

            f"Max RTT: "
            f"{self.metric_data['max_rtt_us']} us\n"

            f"Verdict: "
            f"{self.metric_data['verdict']}\n\n"

            "FAULT DISTRIBUTION\n"
            "================================\n"

            f"Normal: "
            f"{self.metric_data['normal']}\n"

            f"CRC faults: "
            f"{self.metric_data['crc_faults']}\n"

            f"Drops: "
            f"{self.metric_data['drops']}\n"

            f"Duplicates: "
            f"{self.metric_data['duplicates']}\n"

            f"Delays: "
            f"{self.metric_data['delays']}\n\n"

            "SERIAL LOG\n"
            "================================\n"
            f"{self.terminal.toPlainText()}\n"
        )

        try:
            with open(
                path,
                "w",
                encoding="utf-8"
            ) as file:
                file.write(report)

            self.log(
                f"Report saved: {path}"
            )

        except OSError as error:
            self.log(
                f"REPORT ERROR: {error}"
            )

    def log(self, message):
        self.terminal.appendPlainText(
            message
        )

    def resizeEvent(self, event):
        super().resizeEvent(event)

        if (
            hasattr(self, "scanlines")
            and self.centralWidget()
        ):
            self.scanlines.setGeometry(
                self.centralWidget().rect()
            )

            self.scanlines.raise_()

    def closeEvent(self, event):
        if self.serial_port:
            self.serial_port.close()

        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)

    app.setFont(
        QFont(
            "Consolas",
            10
        )
    )

    window = GlitchGoblinGUI()

    window.show()

    sys.exit(
        app.exec()
    )