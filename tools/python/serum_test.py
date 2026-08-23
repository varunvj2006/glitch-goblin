import serial
import time

PORT = "COM9"
BAUD = 115200


def crc16_ccitt(data):
    crc = 0xFFFF

    for byte in data:
        crc ^= byte << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc


body = bytes([
    0x01,
    0x01,
    0x00, 0x00,
    0x00, 0x01
])

crc = crc16_ccitt(body)

packet = bytes([
    0x53,
    0x45
]) + body + crc.to_bytes(2, "big")


bad_packet = bytearray(packet)
bad_packet[3] ^= 0x01


with serial.Serial(PORT, BAUD, timeout=1) as ser:
    time.sleep(1)

    ser.reset_input_buffer()

    print("Valid SERUM packet:")
    print(packet.hex(" "))

    ser.write(packet)

    time.sleep(0.5)

    response = ser.read_all()

    print(response.decode(errors="replace"))

    ser.reset_input_buffer()

    print("Corrupted SERUM packet:")
    print(bad_packet.hex(" "))

    ser.write(bad_packet)

    time.sleep(0.5)

    response = ser.read_all()

    print(response.decode(errors="replace"))