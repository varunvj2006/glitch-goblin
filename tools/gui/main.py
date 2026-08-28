import sys

import serial
import serial.tools.list_ports

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPushButton,
    QPlainTextEdit,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)


class GlitchGoblinGUI(QMainWindow):
    def __init__(self):
        super().__init__()

        self.serial_port = None

        self.setWindowTitle("Glitch Goblin V2")
        self.resize(1000, 700)

        self.build_ui()
        self.refresh_ports()

        self.serial_timer = QTimer()
        self.serial_timer.timeout.connect(
            self.read_serial
        )
        self.serial_timer.start(20)

    def build_ui(self):
        root = QWidget()

        main_layout = QVBoxLayout(root)

        connection_box = self.build_connection_box()
        test_box = self.build_test_box()
        command_box = self.build_command_box()

        self.terminal = QPlainTextEdit()
        self.terminal.setReadOnly(True)

        main_layout.addWidget(connection_box)
        main_layout.addWidget(test_box)
        main_layout.addWidget(command_box)

        main_layout.addWidget(
            QLabel("Serial Output")
        )

        main_layout.addWidget(
            self.terminal
        )

        self.setCentralWidget(root)

    def build_connection_box(self):
        box = QGroupBox("ESP32 Connection")

        layout = QHBoxLayout(box)

        self.port_box = QComboBox()

        refresh_button = QPushButton(
            "Refresh"
        )

        self.connect_button = QPushButton(
            "Connect"
        )

        refresh_button.clicked.connect(
            self.refresh_ports
        )

        self.connect_button.clicked.connect(
            self.toggle_connection
        )

        layout.addWidget(
            QLabel("Port:")
        )

        layout.addWidget(
            self.port_box
        )

        layout.addWidget(
            refresh_button
        )

        layout.addWidget(
            self.connect_button
        )

        return box

    def build_test_box(self):
        box = QGroupBox(
            "SERUM Tests"
        )

        layout = QGridLayout(box)

        normal_button = QPushButton(
            "Normal"
        )

        crc_button = QPushButton(
            "CRC Fault"
        )

        drop_button = QPushButton(
            "Drop"
        )

        duplicate_button = QPushButton(
            "Duplicate"
        )

        delay_button = QPushButton(
            "Delay"
        )

        reliable_button = QPushButton(
            "Reliable"
        )

        replay_button = QPushButton(
            "Replay"
        )

        stats_button = QPushButton(
            "STM32 Stats"
        )

        normal_button.clicked.connect(
            lambda: self.send_command(
                "normal"
            )
        )

        crc_button.clicked.connect(
            lambda: self.send_command(
                "crc"
            )
        )

        drop_button.clicked.connect(
            lambda: self.send_command(
                "drop"
            )
        )

        duplicate_button.clicked.connect(
            lambda: self.send_command(
                "duplicate"
            )
        )

        delay_button.clicked.connect(
            lambda: self.send_command(
                "delay"
            )
        )

        reliable_button.clicked.connect(
            lambda: self.send_command(
                "reliable"
            )
        )

        replay_button.clicked.connect(
            lambda: self.send_command(
                "replay"
            )
        )

        stats_button.clicked.connect(
            lambda: self.send_command(
                "stats"
            )
        )

        layout.addWidget(
            normal_button,
            0,
            0
        )

        layout.addWidget(
            crc_button,
            0,
            1
        )

        layout.addWidget(
            drop_button,
            0,
            2
        )

        layout.addWidget(
            duplicate_button,
            0,
            3
        )

        layout.addWidget(
            delay_button,
            1,
            0
        )

        layout.addWidget(
            reliable_button,
            1,
            1
        )

        layout.addWidget(
            replay_button,
            1,
            2
        )

        layout.addWidget(
            stats_button,
            1,
            3
        )

        self.bench_count = QSpinBox()
        self.bench_count.setRange(
            1,
            10000
        )
        self.bench_count.setValue(
            100
        )

        bench_button = QPushButton(
            "Run Benchmark"
        )

        bench_button.clicked.connect(
            self.run_benchmark
        )

        self.chaos_count = QSpinBox()
        self.chaos_count.setRange(
            1,
            10000
        )
        self.chaos_count.setValue(
            100
        )

        chaos_button = QPushButton(
            "Run Chaos"
        )

        chaos_button.clicked.connect(
            self.run_chaos
        )

        layout.addWidget(
            QLabel("Benchmark packets:"),
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
            2
        )

        layout.addWidget(
            QLabel("Chaos packets:"),
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
            2
        )

        return box

    def build_command_box(self):
        box = QGroupBox(
            "Manual Command"
        )

        layout = QHBoxLayout(box)

        self.command_input = QLineEdit()

        self.command_input.setPlaceholderText(
            "Example: faults 60 10 15 10 5"
        )

        send_button = QPushButton(
            "Send"
        )

        send_button.clicked.connect(
            self.send_manual_command
        )

        self.command_input.returnPressed.connect(
            self.send_manual_command
        )

        layout.addWidget(
            self.command_input
        )

        layout.addWidget(
            send_button
        )

        return box

    def refresh_ports(self):
        self.port_box.clear()

        ports = serial.tools.list_ports.comports()

        for port in ports:
            self.port_box.addItem(
                port.device
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
                "No serial port selected."
            )
            return

        try:
            self.serial_port = serial.Serial(
                port=port,
                baudrate=115200,
                timeout=0
            )

            self.connect_button.setText(
                "Disconnect"
            )

            self.log(
                f"Connected to {port}"
            )

        except serial.SerialException as error:
            self.serial_port = None

            self.log(
                f"Connection failed: {error}"
            )

    def disconnect_serial(self):
        if self.serial_port:
            self.serial_port.close()

        self.serial_port = None

        self.connect_button.setText(
            "Connect"
        )

        self.log(
            "Disconnected"
        )

    def send_command(self, command):
        if not self.serial_port:
            self.log(
                "ESP32 is not connected."
            )
            return

        data = (
            command +
            "\n"
        ).encode()

        self.serial_port.write(
            data
        )

        self.log(
            f"> {command}"
        )

    def run_benchmark(self):
        count = self.bench_count.value()

        self.send_command(
            f"bench {count}"
        )

    def run_chaos(self):
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

        self.send_command(
            command
        )

        self.command_input.clear()

    def read_serial(self):
        if not self.serial_port:
            return

        try:
            waiting = (
                self.serial_port.in_waiting
            )

            if waiting == 0:
                return

            data = self.serial_port.read(
                waiting
            )

            text = data.decode(
                errors="replace"
            )

            self.terminal.insertPlainText(
                text
            )

            scrollbar = (
                self.terminal
                .verticalScrollBar()
            )

            scrollbar.setValue(
                scrollbar.maximum()
            )

        except serial.SerialException:
            self.log(
                "Serial connection lost."
            )

            self.disconnect_serial()

    def log(self, message):
        self.terminal.appendPlainText(
            message
        )


app = QApplication(sys.argv)

window = GlitchGoblinGUI()
window.show()

sys.exit(
    app.exec()
)