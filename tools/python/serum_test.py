import serial
import time

PORT = "COM9"
BAUD = 115200
ACK_TIMEOUT = 0.25  # we need timeout to see if the stm has acked back 
MAX_RETRIES = 3
SERUM_MSG_ACK = 0x05     
SERUM_MSG_NACK = 0x06


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
        return None

    if packet[0] != 0x53 or packet[1] != 0x45:
        return None

    version = packet[2]
    msg_type = packet[3]

    length = (packet[4] << 8) | packet[5]
    sequence = (packet[6] << 8) | packet[7]

    expected_size = 8 + length + 2

    if len(packet) < expected_size:
        return None

    payload = packet[8:8 + length]

    received_crc = (
        packet[8 + length] << 8
    ) | packet[9 + length]

    body = packet[2:8 + length]
    calculated_crc = crc16_ccitt(body)

    if received_crc != calculated_crc:
        return None

    return {
        "version": version,
        "type": msg_type,
        "length": length,
        "sequence": sequence,
        "payload": payload
    }


sequence = 1

packet = build_serum_packet(
    msg_type=0x01,
    sequence=sequence
)

print("Sending SERUM packet:")
print(packet.hex(" "))


def send_reliable(ser, packet, sequence):
    for attempt in range(1, MAX_RETRIES + 1):

        print(
            f"TX sequence #{sequence}, "
            f"attempt {attempt}"
        )

        ser.reset_input_buffer()
        ser.write(packet)

        start_time = time.time()

        while time.time() - start_time < ACK_TIMEOUT:

            if ser.in_waiting >= 10:
                raw = ser.read(ser.in_waiting)

                response = parse_serum_packet(raw)

                if response is None:
                    continue

                if (
                    response["type"] == SERUM_MSG_ACK
                    and
                    response["sequence"] == sequence
                ):
                    print(
                        f"ACK #{sequence} received"
                    )

                    return True

            time.sleep(0.01)

        print(
            f"ACK #{sequence} timeout"
        )

    print(
        f"Packet #{sequence} failed "
        f"after {MAX_RETRIES} attempts"
    )

    return False



with serial.Serial(
    PORT,
    BAUD,
    timeout=1
) as ser:

    time.sleep(1)
    ser.reset_input_buffer()

    success = send_reliable(
        ser,
        packet,
        sequence
    )

    if success:
        print(f"Packet #{sequence} delivered successfully")
    else:
        print(f"Packet #{sequence} delivery failed")