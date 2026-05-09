# ==============================
# Chunk builder
# ==============================
def build_chunks_from_mem(mem):
    if not mem:
        return []

    chunks = []
    addrs  = sorted(mem.keys())
    
    # Align start về bội số 16
    first_addr = addrs[0]
    aligned_start = first_addr & ~0xF
    
    # Tạo contiguous buffer từ aligned_start
    last_addr = addrs[-1]
    total_len = last_addr - aligned_start + 1
    
    # Điền 0xFF (erased flash value) cho các gap
    buf = bytearray([0xFF] * total_len)
    for a, v in mem.items():
        buf[a - aligned_start] = v
    
    # Chia thành chunks CHUNK_SIZE, mỗi chunk align 16
    chunks = []
    offset = 0
    while offset < total_len:
        size = min(CHUNK_SIZE, total_len - offset)
        chunk_addr = aligned_start + offset
        chunk_data = bytes(buf[offset:offset + size])
        chunks.append((chunk_addr, chunk_data))
        offset += size
    
    return chunks
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

BL_APP_START = 0x0040C000
BL_APP_SIZE  = (2000 * 1024)
FLASH_START  = BL_APP_START
FLASH_END    = BL_APP_START + BL_APP_SIZE

MAX_DATA_LEN = 244
ADDR_LEN     = 4
CHUNK_SIZE   = MAX_DATA_LEN - ADDR_LEN  # 240 bytes

# Gap lớn hơn ngưỡng này sẽ tách thành region riêng (không fill 0xFF)
GAP_THRESHOLD = CHUNK_SIZE  # 240 bytes

ERASE_TIMEOUT = 60.0
WRITE_TIMEOUT = 10.0
ACK_TIMEOUT   = 15.0

# ==============================
# Shared RX state
# ==============================
ack_event  = threading.Event()
nack_event = threading.Event()

rx_buffer = bytearray()
running   = True

# ==============================
# Packet helpers
# ==============================
def build_packet(cmd, data=b''):
    length    = len(data)
    header    = struct.pack("<BBB", START_BYTE, cmd, length)
    crc_input = struct.pack("<BB", cmd, length) + data
    crc       = calculate_crc32(crc_input) & 0xFFFFFFFF
    return header + data + struct.pack("<I", crc)

def send_packet(ser, cmd, data=b''):
    pkt = build_packet(cmd, data)
    ser.write(pkt)

# ==============================
# RX worker
# ==============================
def rx_worker(ser):
    global running
    while running:
        n = ser.in_waiting
        if n:
            data = ser.read(n)
            for byte in data:
                if byte == ACK:
                    ack_event.set()
                elif byte == NACK:
                    nack_event.set()
                else:
                    rx_buffer.append(byte)

# ==============================
# Wait helpers
# ==============================
def wait_ack(timeout=ACK_TIMEOUT):
    ack_event.clear()
    nack_event.clear()
    if ack_event.wait(timeout):
        return True
    if nack_event.is_set():
        return False
    return False

def read_block(size, timeout=ACK_TIMEOUT):
    t0 = time.time()
    while time.time() - t0 < timeout:
        if len(rx_buffer) >= size:
            data = rx_buffer[:size]
            del rx_buffer[:size]
            return bytes(data)
        time.sleep(0.001)
    return None

# ==============================
# HEX parser
# ==============================
def parse_hex_line(line):
    if not line.startswith(":"):
        return None
    length  = int(line[1:3], 16)
    addr    = int(line[3:7], 16)
    rectype = int(line[7:9], 16)
    data    = bytes.fromhex(line[9:9 + length * 2])
    return length, addr, rectype, data

# ==============================
# Chunk builder
# Tách region khi gap > GAP_THRESHOLD, align 16 byte, chia chunks 240 byte
# ==============================
def build_chunks_from_mem(mem):
    if not mem:
        return []

    addrs = sorted(mem.keys())
# Bước 1: Tách thành các region liên tục
    # Gap > GAP_THRESHOLD → region mới (tránh fill 0xFF vào gap lớn như 2MB)
    regions     = []
    region_start = addrs[0]
    region_data  = bytearray()
    prev_addr    = addrs[0]

    for a in addrs:
        gap = a - prev_addr
        if gap > GAP_THRESHOLD and len(region_data) > 0:
            # Flush region hiện tại, bắt đầu region mới
            regions.append((region_start, bytes(region_data)))
            region_start = a
            region_data  = bytearray()
            prev_addr    = a

        # Fill gap nhỏ trong cùng region bằng 0xFF
        while len(region_data) < (a - region_start):
            region_data.append(0xFF)

        region_data.append(mem[a])
        prev_addr = a

    # Flush region cuối
    if region_data:
        regions.append((region_start, bytes(region_data)))

    print(f"  [INFO] Tìm thấy {len(regions)} region(s):")
    for i, (rstart, rdata) in enumerate(regions):
        print(f"         Region {i+1}: 0x{rstart:08X} — 0x{rstart+len(rdata)-1:08X} ({len(rdata)} bytes)")

    # Bước 2: Với mỗi region, align về 16 byte rồi chia chunks
    chunks = []
    for (rstart, rdata) in regions:
        # Align địa chỉ bắt đầu xuống bội số 16
        aligned_start = rstart & ~0xF
        prefix_pad    = rstart - aligned_start

        buf = bytearray([0xFF] * prefix_pad) + bytearray(rdata)

        # Align độ dài lên bội số 16
        tail = len(buf) % 16
        if tail != 0:
            buf += bytearray([0xFF] * (16 - tail))

        # Chia thành chunks CHUNK_SIZE (240 bytes)
        offset = 0
        while offset < len(buf):
            size       = min(CHUNK_SIZE, len(buf) - offset)
            chunk_addr = aligned_start + offset
            chunk_data = bytes(buf[offset:offset + size])
            chunks.append((chunk_addr, chunk_data))
            offset += size

    return chunks

# ==============================
# Core write loop
# ==============================
def write_chunks(ser, chunks, total_bytes):
    written = 0
    t0      = time.time()

    for idx, (addr, chunk) in enumerate(chunks):
        payload = struct.pack("<I", addr) + chunk
        send_packet(ser, CMD_WRITE, payload)

        if not wait_ack(timeout=WRITE_TIMEOUT):
            print(f"\nWRITE FAIL tại addr 0x{addr:08X} (packet {idx})")
            return False

        time.sleep(0.002)
        written += len(chunk)

        elapsed = time.time() - t0
        kbps    = (written / 1024) / elapsed if elapsed > 0 else 0
        pct     = written * 100 // total_bytes

        print(f"\rProgress: {pct}%  ({written}/{total_bytes} bytes)  {kbps:.2f} KB/s",
              end="", flush=True)

    print()
    return True

# ==============================
# Commands
# ==============================
def cmd_ping(ser):
    send_packet(ser, CMD_PING)
    print("PING:", "OK" if wait_ack() else "FAIL")

def cmd_erase(ser):
    send_packet(ser, CMD_ERASE)
print("Erasing flash...", end="", flush=True)
    t0 = time.time()
    ok = wait_ack(timeout=ERASE_TIMEOUT)
    print(f" {time.time()-t0:.1f}s — {'OK' if ok else 'FAIL'}")

# ---------- BIN WRITE ----------
def cmd_write_bin(ser, filename):
    try:
        size = os.path.getsize(filename)
        print(f"BIN size: {size} bytes")

        chunks = []
        addr   = BL_APP_START

        with open(filename, "rb") as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                chunks.append((addr, chunk))
                addr += len(chunk)

        total_size = sum(len(c) for _, c in chunks)
        print(f"Số packets: {len(chunks)}")

        t0 = time.time()
        if write_chunks(ser, chunks, total_size):
            elapsed = time.time() - t0
            print(f"BIN WRITE DONE — {total_size} bytes trong {elapsed:.1f}s")

    except FileNotFoundError:
        print("File not found")

# ---------- HEX WRITE ----------
def cmd_write_hex(ser, filename):
    base_addr = 0
    mem       = {}

    try:
        with open(filename, "r") as f:
            lines = f.readlines()

        for line in lines:
            line = line.strip()
            rec  = parse_hex_line(line)
            if rec is None:
                continue

            length, addr, rectype, data = rec

            if rectype == 0x00:
                abs_addr = base_addr + addr
                for i, b in enumerate(data):
                    a = abs_addr + i
                    if FLASH_START <= a < FLASH_END:
                        mem[a] = b

            elif rectype == 0x04:
                base_addr = int(line[9:13], 16) << 16

            elif rectype == 0x01:
                break

        if not mem:
            print("Không có data trong flash range")
            return

        chunks     = build_chunks_from_mem(mem)
        total_size = sum(len(c) for _, c in chunks)  # bytes thực sự sẽ ghi

        print(f"HEX data: {len(mem)} bytes thực → {len(chunks)} packets → {total_size} bytes sẽ ghi")

        t0 = time.time()
        if write_chunks(ser, chunks, total_size):
            elapsed = time.time() - t0
            print(f"HEX WRITE DONE — {total_size} bytes trong {elapsed:.1f}s")

    except FileNotFoundError:
        print("File not found")

# ---------- READ BLOCK ----------
def cmd_read(ser, addr):
    payload = struct.pack("<I", addr)
    send_packet(ser, CMD_READ, payload)

    if not wait_ack():
        print("READ FAIL")
        return

    data = read_block(CHUNK_SIZE)
    if data:
        print(data.hex(" "))
    else:
        print("Timeout")

# ---------- READ FLASH TO FILE ----------
def cmd_readfile(ser, start_addr, total_size, filename):
    addr      = start_addr
    remaining = total_size
    total     = 0

    t0 = time.time()

    with open(filename, "wb") as f:
        print(f"Dumping {total_size} bytes from 0x{start_addr:08X}")

        while remaining > 0:
payload = struct.pack("<I", addr)
            send_packet(ser, CMD_READ, payload)

            if not wait_ack():
                print(f"\nNo ACK at 0x{addr:08X}")
                return

            chunk = min(CHUNK_SIZE, remaining)
            data  = read_block(chunk)

            if data is None:
                print(f"\nTimeout at 0x{addr:08X}")
                return

            f.write(data)

            addr      += chunk
            remaining -= chunk
            total     += chunk

            elapsed = time.time() - t0
            speed   = total / elapsed if elapsed > 0 else 0
            pct     = total * 100 / total_size

            print(
                f"\r{pct:6.2f}% | {total}/{total_size} | "
                f"{speed/1024:6.2f} KB/s | addr 0x{addr:08X}",
                end="", flush=True
            )

    print("\nDump complete")

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
    print(" write <file.bin|file.hex>")
    print(" read <addr_hex>")
    print(" readfile <addr_hex> <size> <file>")
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
                print("Usage: write file.bin | file.hex")
            else:
                fname = tokens[1]
                if fname.lower().endswith(".hex"):
                    cmd_write_hex(ser, fname)
                else:
                    cmd_write_bin(ser, fname)

        elif tokens[0] == "read" and len(tokens) == 2:
            cmd_read(ser, int(tokens[1], 16))

        elif tokens[0] == "readfile" and len(tokens) == 4:
            start = int(tokens[1], 16)
            size  = int(tokens[2])
            fname = tokens[3]
            cmd_readfile(ser, start, size, fname)

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

    port = input("Serial port: ")
    ser  = serial.Serial(port, 115200, timeout=0.1)

    time.sleep(2)
    ser.reset_input_buffer()

    t = threading.Thread(target=rx_worker, args=(ser,), daemon=True)
    t.start()

    try:
        shell(ser)
    finally:
        running = False
        ser.close()

if __name__ == "__main__":
    main()
