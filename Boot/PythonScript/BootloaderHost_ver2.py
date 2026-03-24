import serial
import struct
import time
import threading
import os
from CRC import calculate_crc32

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

# Flash layout (MUST match bootloader)
BL_APP_START = 0x0040C000
BL_APP_SIZE  = (2000 * 1024)    ## 2000KB max app size
FLASH_START  = BL_APP_START
FLASH_END    = BL_APP_START + BL_APP_SIZE

MAX_DATA_LEN = 252
ADDR_LEN = 4
CHUNK_SIZE = MAX_DATA_LEN - ADDR_LEN  # 248 bytes payload

# ==============================
# Shared RX state
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
    crc = calculate_crc32(crc_input) & 0xFFFFFFFF

    return header + data + struct.pack("<I", crc)


def send_packet(ser, cmd, data=b''):
    pkt = build_packet(cmd, data)
    print("TX:", pkt.hex(" "))
    ser.write(pkt)

# ==============================
# RX worker
# ==============================

def rx_worker(ser):
    global running, rx_buffer, last_packet

    while running:
        if ser.in_waiting:
            b = ser.read(1)
            byte = b[0]
            print(f"RX BYTE: {byte:02X}")
            rx_buffer += b

            if b[0] == ACK:
                print("RX: ACK")
                ack_event.set()
                continue
            elif b[0] == NACK:
                print("RX: NACK")
                nack_event.set()
                continue

            while len(rx_buffer) >= 3:
                if rx_buffer[0] != START_BYTE:
                    rx_buffer.pop(0)
                    continue

                cmd = rx_buffer[1]
                length = rx_buffer[2]
                total = 3 + length + 4

                if len(rx_buffer) < total:
                    break

                pkt = rx_buffer[:total]
                rx_buffer = rx_buffer[total:]

                last_packet = pkt
                packet_event.set()

        time.sleep(0.001)

# ==============================
# Wait helpers
# ==============================

def wait_ack(timeout=20.0):
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


def wait_packet(timeout=20.0):
    packet_event.clear()

    start = time.time()
    while time.time() - start < timeout:
        if packet_event.is_set():
            return last_packet
        time.sleep(0.001)
    return None

# ==============================
# HEX parser
# ==============================

def parse_hex_line(line):
    if not line.startswith(":"):
        return None

    length = int(line[1:3], 16)
    addr   = int(line[3:7], 16)
    rectype = int(line[7:9], 16)
    data   = bytes.fromhex(line[9:9 + length * 2])

    return length, addr, rectype, data

# ==============================
# Commands
# ==============================

def cmd_ping(ser):
    send_packet(ser, CMD_PING)
    print("PING:", "OK" if wait_ack() else "FAIL")


def cmd_erase(ser):
    send_packet(ser, CMD_ERASE)
    print("ERASE:", "OK" if wait_ack() else "FAIL")


# ---------- BIN WRITE ----------

def cmd_write_bin(ser, filename):
    try:
        size = os.path.getsize(filename)
        print(f"BIN size: {size} bytes")

        addr = BL_APP_START

        with open(filename, "rb") as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break

                payload = struct.pack("<I", addr) + chunk

                print(f"WRITE {addr:08X} ({len(chunk)} bytes)")
                send_packet(ser, CMD_WRITE, payload)

                if not wait_ack():
                    print("WRITE FAIL")
                    return

                addr += len(chunk)

        print("BIN WRITE DONE")

    except FileNotFoundError:
        print("File not found")


# ---------- HEX WRITE (FILTERED) ----------

def cmd_write_hex(ser, filename):
    base_addr = 0
    total_written = 0

    print(f"HEX file: {filename}")
    print(f"Flash range: {FLASH_START:08X} - {FLASH_END:08X}")

    try:
        with open(filename, "r") as f:
            for line in f:
                line = line.strip()
                rec = parse_hex_line(line)
                if rec is None:
                    continue

                length, addr, rectype, data = rec

                # -------------------------
                # DATA RECORD
                # -------------------------
                if rectype == 0x00:
                    abs_addr = base_addr + addr

                    # skip completely outside flash
                    if abs_addr >= FLASH_END or abs_addr + length <= FLASH_START:
                        print(f"SKIP {abs_addr:08X} ({length} bytes) outside flash")
                        continue

                    # clip to flash boundaries
                    start = max(abs_addr, FLASH_START)
                    end   = min(abs_addr + length, FLASH_END)

                    slice_offset = start - abs_addr
                    slice_len    = end - start

                    data_slice = data[slice_offset:slice_offset + slice_len]

                    offset = 0
                    while offset < len(data_slice):
                        chunk = data_slice[offset:offset + CHUNK_SIZE]
                        write_addr = start + offset

                        payload = struct.pack("<I", write_addr) + chunk

                        print(f"WRITE {write_addr:08X} ({len(chunk)} bytes)")
                        send_packet(ser, CMD_WRITE, payload)

                        if not wait_ack():
                            print("WRITE FAIL")
                            return

                        total_written += len(chunk)
                        offset += len(chunk)

                # -------------------------
                # EXTENDED LINEAR ADDRESS
                # -------------------------
                elif rectype == 0x04:
                    base_addr = int(line[9:13], 16) << 16
                    print(f"BASE -> {base_addr:08X}")

                # -------------------------
                # EOF
                # -------------------------
                elif rectype == 0x01:
                    break

        print(f"HEX WRITE DONE ({total_written} bytes written)")

    except FileNotFoundError:
        print("File not found")


# ---------- READ ----------

def cmd_read(ser, addr):
    payload = struct.pack("<I", addr)

    send_packet(ser, CMD_READ, payload)

    if not wait_ack():
        print("READ command not acknowledged")
        return

    pkt = wait_packet()
    if pkt is None:
        print("No data packet received")
        return

    start, cmd, length = struct.unpack("<BBB", pkt[:3])
    data = pkt[3:3 + length]

    print(f"READ {length} bytes:")
    print(data.hex(" "))

# ---------- JUMP ----------

def cmd_jump(ser):
    send_packet(ser, CMD_JUMP)
    print("JUMP:", "OK" if wait_ack() else "FAIL")

# ==============================
# Shell
# ==============================

def shell(ser):
    print("Commands:")
    print(" ping")
    print(" erase")
    print(" write firmware.bin|firmware.hex")
    print(" read <addr_hex>")
    print(" jump")
    print(" exit")

    while True:
        try:
            cmd = input("> ").strip()
        except:
            break

        if not cmd:
            continue

        tokens = cmd.split()

        if tokens[0] == "ping":
            cmd_ping(ser)

        elif tokens[0] == "erase":
            cmd_erase(ser)

        elif tokens[0] == "write":
            if len(tokens) < 2:
                print("Usage: write file.bin|file.hex")
            else:
                fname = tokens[1]
                if fname.lower().endswith(".hex"):
                    cmd_write_hex(ser, fname)
                else:
                    cmd_write_bin(ser, fname)

        elif tokens[0] == "read":
            if len(tokens) != 2:
                print("Usage: read <addr_hex>")
            else:
                cmd_read(ser, int(tokens[1], 16))

        elif tokens[0] == "jump":
            cmd_jump(ser)

        elif tokens[0] in ("exit", "quit"):
            break

# ==============================
# Main
# ==============================

def main():
    global running

    port = input("Serial port: ")
    ser = serial.Serial(port, 115200, timeout=0.1)

    time.sleep(2)

    t = threading.Thread(target=rx_worker, args=(ser,))
    t.start()

    try:
        shell(ser)
    finally:
        running = False
        t.join()
        ser.close()


if __name__ == "__main__":
    main()