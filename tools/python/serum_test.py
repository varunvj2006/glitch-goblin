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


def build_serum_packet(msg_type, sequence, payload=b""):
    length = len(payload)

    body = bytes([
        0x01,
        msg_type,
        (length >> 8) & 0xFF,
        length & 0xFF,
        (sequence >> 8) & 0xFF,
        sequence & 0xFF
    ]) + payload

    crc = crc16_ccitt(body)

    packet = bytes([
        0x53,
        0x45
    ]) + body + crc.to_bytes(2, "big")

    return packet


def parse_serum_packet(packet):
    if len(packet) < 10:
        print("Response too short")
        return

    if packet[0] != 0x53 or packet[1] != 0x45:
        print("Invalid SERUM sync bytes")
        return

    version = packet[2]
    msg_type = packet[3]

    length = (packet[4] << 8) | packet[5]

    sequence = (packet[6] << 8) | packet[7]

    expected_size = 8 + length + 2

    if len(packet) < expected_size:
        print("Incomplete SERUM packet")
        return

    payload = packet[8:8 + length]

    received_crc = (
        packet[8 + length] << 8
    ) | packet[9 + length]

    body = packet[2:8 + length]

    calculated_crc = crc16_ccitt(body)

    print("SERUM response:")
    print(f"Version:  {version}")
    print(f"Type:     0x{msg_type:02X}")
    print(f"Length:   {length}")
    print(f"Sequence: {sequence}")
    print(f"Payload:  {payload.hex(' ')}")
    print(f"CRC RX:   0x{received_crc:04X}")
    print(f"CRC CALC: 0x{calculated_crc:04X}")

    if calculated_crc == received_crc:
        print("CRC: VALID")
    else:
        print("CRC: INVALID")

    if msg_type == 0x05:
        print(f"ACK received for packet #{sequence}")
    elif msg_type == 0x06:
        print(f"NACK received for packet #{sequence}")


sequence = 1

packet = build_serum_packet(
    msg_type=0x01,
    sequence=sequence
)

print("Sending SERUM packet:")
print(packet.hex(" "))

with serial.Serial(
    PORT,
    BAUD,
    timeout=1
) as ser:

    time.sleep(1)

    ser.reset_input_buffer()

    ser.write(packet)

    time.sleep(0.5)

    response = ser.read_all()

    print()
    print("Raw STM32 response:")
    print(response.hex(" "))

    print()

    if response:
        parse_serum_packet(response)
    else:
        print("No response from STM32")