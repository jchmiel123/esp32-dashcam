"""Read ext4 root directory from raw disk — aligned reads for Windows"""
import struct, sys, ctypes, os

OUTFILE = os.path.join(os.path.dirname(__file__), "ext4_results.txt")

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

if not is_admin():
    ctypes.windll.shell32.ShellExecuteW(
        None, "runas", sys.executable, f'"{__file__}"', None, 1)
    sys.exit()

DISK = r"\\.\PhysicalDrive2"
PART2_OFFSET = 541065216
SECTOR = 512

lines = []

def aligned_read(f, offset, size):
    """Read with 512-byte alignment for Windows raw disk"""
    align_off = offset % SECTOR
    seek_pos = offset - align_off
    read_size = ((size + align_off + SECTOR - 1) // SECTOR) * SECTOR
    f.seek(seek_pos)
    data = f.read(read_size)
    return data[align_off:align_off + size]

try:
    # Open with no buffering for aligned access
    f = open(DISK, "rb", buffering=0)

    # Superblock
    sb = aligned_read(f, PART2_OFFSET + 1024, 1024)
    magic = struct.unpack_from("<H", sb, 56)[0]
    lines.append(f"Magic: 0x{magic:04x} ({'VALID ext4' if magic == 0xEF53 else 'NOT ext4'})")

    if magic != 0xEF53:
        raise Exception("Not ext4")

    block_size = 1024 << struct.unpack_from("<I", sb, 24)[0]
    block_count = struct.unpack_from("<I", sb, 4)[0]
    free_blocks = struct.unpack_from("<I", sb, 12)[0]
    inode_size = struct.unpack_from("<H", sb, 88)[0]
    vol = sb[120:136].decode("utf-8", errors="replace").rstrip("\x00")
    mnt = sb[136:200].decode("utf-8", errors="replace").rstrip("\x00")

    lines.append(f"Volume: '{vol}'")
    lines.append(f"Last mount: '{mnt}'")
    lines.append(f"Block size: {block_size}")
    lines.append(f"Size: {block_count * block_size / 1073741824:.1f} GB")
    lines.append(f"Free: {free_blocks * block_size / 1073741824:.1f} GB")
    lines.append(f"Used: {(block_count - free_blocks) * block_size / 1073741824:.1f} GB")
    lines.append(f"Inode size: {inode_size}")

    # Block group descriptor (at block 1 for 4K blocks, or block 2 for 1K)
    bgd_block = 1 if block_size > 1024 else 2
    bgd = aligned_read(f, PART2_OFFSET + bgd_block * block_size, 64)
    inode_table_lo = struct.unpack_from("<I", bgd, 8)[0]
    lines.append(f"Inode table block: {inode_table_lo}")

    # Read inode 2 (root directory) — inode numbering starts at 1
    inode2_off = PART2_OFFSET + inode_table_lo * block_size + (2 - 1) * inode_size
    inode = aligned_read(f, inode2_off, inode_size)

    mode = struct.unpack_from("<H", inode, 0)[0]
    size_lo = struct.unpack_from("<I", inode, 4)[0]
    flags = struct.unpack_from("<I", inode, 32)[0]
    lines.append(f"\nRoot inode: mode=0o{mode:o} size={size_lo} flags=0x{flags:08x}")

    # Check if uses extents
    uses_extents = bool(flags & 0x80000)

    def parse_dir_block(data, max_entries=100):
        entries = []
        pos = 0
        while pos < len(data) - 8 and len(entries) < max_entries:
            d_inode = struct.unpack_from("<I", data, pos)[0]
            d_rec_len = struct.unpack_from("<H", data, pos + 4)[0]
            if d_rec_len == 0:
                break
            d_name_len = data[pos + 6]
            d_file_type = data[pos + 7]
            if d_inode > 0 and d_name_len > 0:
                name = data[pos + 8:pos + 8 + d_name_len].decode("utf-8", errors="replace")
                ftypes = {1: "file", 2: "dir", 7: "symlink"}
                ft = ftypes.get(d_file_type, f"type{d_file_type}")
                entries.append((ft, name))
            pos += d_rec_len
        return entries

    if uses_extents:
        eh_magic = struct.unpack_from("<H", inode, 40)[0]
        eh_entries = struct.unpack_from("<H", inode, 42)[0]
        eh_depth = struct.unpack_from("<H", inode, 46)[0]
        lines.append(f"Extent: magic=0x{eh_magic:04x} entries={eh_entries} depth={eh_depth}")

        if eh_magic == 0xF30A and eh_depth == 0:
            lines.append(f"\n--- Root Directory (/) ---")
            for i in range(eh_entries):
                off = 52 + i * 12
                ee_len = struct.unpack_from("<H", inode, off + 4)[0]
                ee_start_hi = struct.unpack_from("<H", inode, off + 6)[0]
                ee_start_lo = struct.unpack_from("<I", inode, off + 8)[0]
                phys_block = (ee_start_hi << 32) | ee_start_lo

                read_blocks = min(ee_len, 8)
                dirdata = aligned_read(f, PART2_OFFSET + phys_block * block_size, block_size * read_blocks)

                for ft, name in parse_dir_block(dirdata):
                    lines.append(f"  [{ft}] {name}")
    else:
        # Direct block pointers
        block_ptr = struct.unpack_from("<I", inode, 40)[0]
        if block_ptr > 0:
            dirdata = aligned_read(f, PART2_OFFSET + block_ptr * block_size, block_size)
            lines.append(f"\n--- Root Directory (/) ---")
            for ft, name in parse_dir_block(dirdata):
                lines.append(f"  [{ft}] {name}")

    f.close()
except Exception as e:
    import traceback
    lines.append(f"Error: {e}")
    lines.append(traceback.format_exc())

with open(OUTFILE, "w") as out:
    out.write("\n".join(lines))
