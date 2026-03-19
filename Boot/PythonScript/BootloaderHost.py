import serial
import struct
import time
import zlib
import threading

# ==============================
# Protocol constants
# ==============================

START_BYTE = 0xAA

CMD_PING  = 0x01
CMD_READ  = 0x02
CMD_ERASE = 0x03
CMD_WRITE = 0x04
CMD_JUMP  = 0x05

ACK  = 0x79
NACK = 0x1F

CHUNK_SIZE = 255

# ==============================
# Shared state
# ==============================

ack_event = threading.Event()
nack_event = threading.Event()
packet_event = threading.Event()

rx_buffer = bytearray()
last_packet = None

running = True

# ==============================
# Packet helpers
# ==============================

def build_packet(cmd, data=b''):
    length = len(data)

    header = struct.pack("<BBB", START_BYTE, cmd, length)
    crc_input = struct.pack("<BB", cmd, length) + data
    crc = zlib.crc32(crc_input) & 0xFFFFFFFF

    return header + data + struct.pack("<I", crc)


def send_packet(ser, cmd, data=b''):
    pkt = build_packet(cmd, data)
    print("TX:", pkt.hex(" "))
    ser.write(pkt)

# ==============================
# RX thread with packet parser
# ==============================

def rx_worker(ser):
    global running, rx_buffer, last_packet

    while running:
        if ser.in_waiting:
            b = ser.read(1)
            rx_buffer += b

            print("RX:", b.hex())

            # ACK / NACK detection
            if b[0] == ACK:
                ack_event.set()
                continue
            elif b[0] == NACK:
                nack_event.set()
                continue

            # Packet parsing
            while len(rx_buffer) >= 3:
                if rx_buffer[0] != START_BYTE:
                    rx_buffer.pop(0)
                    continue

                cmd = rx_buffer[1]
                length = rx_buffer[2]

                total_len = 3 + length + 4
                if len(rx_buffer) < total_len:
                    break

                pkt = rx_buffer[:total_len]
                rx_buffer = rx_buffer[total_len:]

                last_packet = pkt
                packet_event.set()

        time.sleep(0.001)

# ==============================
# Wait helpers
# ==============================

def wait_ack(timeout=2.0):
    ack_event.clear()
    nack_event.clear()

    start = time.time()
    while time.time() - start < timeout:
        if ack_event.is_set():
            return True
        if nack_event.is_set():
            return False
        time.sleep(0.001)

    return False


def wait_packet(timeout=2.0):
    packet_event.clear()

    start = time.time()
    while time.time() - start < timeout:
        if packet_event.is_set():
            return last_packet
        time.sleep(0.001)

    return None

# ==============================
# Bootloader commands
# ==============================

def cmd_ping(ser):
    send_packet(ser, CMD_PING)
    print("PING:", "OK" if wait_ack() else "FAIL")


def cmd_erase(ser):
    send_packet(ser, CMD_ERASE)
    print("ERASE:", "OK" if wait_ack() else "FAIL")


def cmd_write(ser, filename):
    try:
        with open(filename, "rb") as f:
            addr = 0

            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break

                print(f"WRITE {addr:08X} ({len(chunk)} bytes)")
                send_packet(ser, CMD_WRITE, chunk)

                if not wait_ack():
                    print("WRITE FAIL")
                    return

                addr += len(chunk)

        print("WRITE DONE")
    except FileNotFoundError:
        print("File not found")


def cmd_read(ser, addr, length):
    payload = struct.pack("<IH", addr, length)

    send_packet(ser, CMD_READ, payload)

    if not wait_ack():
        print("READ command not acknowledged")
        return

    pkt = wait_packet()
    if pkt is None:
        print("No data packet received")
        return

    start, cmd, l = struct.unpack("<BBB", pkt[:3])
    data = pkt[3:3+l]
    crc_rx = struct.unpack("<I", pkt[3+l:3+l+4])[0]

    crc_calc = zlib.crc32(struct.pack("<BB", cmd, l) + data) & 0xFFFFFFFF

    if crc_rx != crc_calc:
        print("CRC mismatch")
        return

    print(f"READ {len(data)} bytes:")
    print(data.hex(" "))


def cmd_jump(ser):
    send_packet(ser, CMD_JUMP)
    print("JUMP:", "OK" if wait_ack() else "FAIL")

# ==============================
# Interactive shell
# ==============================

def shell(ser):
    print("Bootloader shell ready")
    print("Commands: ping, erase, write <file>, read <addr> <len>, jump, exit")

    while True:
        try:
            cmd = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            break

        if cmd == "":
            continue

        tokens = cmd.split()

        if tokens[0] == "ping":
            cmd_ping(ser)

        elif tokens[0] == "erase":
            cmd_erase(ser)

        elif tokens[0] == "write":
            if len(tokens) < 2:
                print("Usage: write firmware.bin")
            else:
                cmd_write(ser, tokens[1])

        elif tokens[0] == "read":
            if len(tokens) != 3:
                print("Usage: read <addr_hex> <len>")
            else:
                addr = int(tokens[1], 16)
                length = int(tokens[2])
                cmd_read(ser, addr, length)

        elif tokens[0] == "jump":
            cmd_jump(ser)

        elif tokens[0] in ("exit", "quit"):
            break

        else:
            print("Unknown command")

# ==============================
# Main
# ==============================

def main():
    global running

    port = input("Serial port (e.g. COM3 or /dev/ttyUSB0): ")

    ser = serial.Serial(port, 115200, timeout=0.1)
    time.sleep(2)

    rx_thread = threading.Thread(target=rx_worker, args=(ser,))
    rx_thread.start()

    try:
        shell(ser)
    finally:
        running = False
        rx_thread.join()
        ser.close()


if __name__ == "__main__":
    main()