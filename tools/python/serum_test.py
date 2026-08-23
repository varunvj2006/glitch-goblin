import serial
import time

PORT = "COM9"
BAUD = 115200

packet = bytes([
    0x53, 0x45,     # SERUM sync

    0x01,           # version
    0x01,           # PING

    0x00, 0x00,     # payload length = 0

    0x00, 0x01      # sequence = 1
])

with serial.Serial(PORT, BAUD, timeout=1) as ser:

    # Give STM32/ST-Link time to settle
    time.sleep(1)

    # Throw away old startup messages
    ser.reset_input_buffer()

    print("Injecting SERUM...")
    print(packet.hex(" "))

    ser.write(packet)

    time.sleep(0.5)

    response = ser.read_all()

    print("STM32:")
    print(response.decode(errors="replace"))