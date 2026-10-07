#!/usr/bin/env python3
"""Makes standalone/gta_image.bin: the data of the original executable as its start-up code leaves it.

That's the data sections of the executable's file, with launcher/StartupData.inc (see generate.py) applied.
The standalone executable has it as a resource, and puts it where the original has it (standalone/Entry.cpp).
It's the original's data, so it isn't in the repository: everyone makes it from their own executable.

usage: make_image.py <original exe (1.0 US, "compact")>
"""
import os, re, struct, sys
if len(sys.argv) != 2:
    sys.exit(__doc__)
BEGIN, END = 0x858000, 0xCB1000
root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
exe = open(sys.argv[1], "rb").read()
pe = struct.unpack_from("<I", exe, 0x3C)[0]
nsec = struct.unpack_from("<H", exe, pe + 6)[0]
opt = struct.unpack_from("<H", exe, pe + 20)[0]
base, size_of_image = struct.unpack_from("<I", exe, pe + 24 + 28)[0], struct.unpack_from("<I", exe, pe + 24 + 56)[0]
entry = struct.unpack_from("<I", exe, pe + 24 + 16)[0]
if (base, size_of_image, base + entry) != (0x400000, 0x8B1000, 0x824570):
    sys.exit("Not the 1.0 US compact executable")
img = bytearray(END - BEGIN)
for i in range(nsec):
    o = pe + 24 + opt + i * 40
    vsz, va, rsz, raw, _, _, _, _, flags = struct.unpack_from("<IIIIIIHHI", exe, o + 8)
    va += base
    if flags & 0x20000000 or not BEGIN <= va < END:  # code
        continue
    k = min(rsz, vsz) if vsz else rsz
    img[va - BEGIN:va - BEGIN + k] = exe[raw:raw + k]
inc = open(os.path.join(root, "launcher", "StartupData.inc")).read()
runs_text, data_text = inc.split("static constexpr unsigned char STARTUP_DATA[]{")
runs = [(int(a, 16), int(s)) for a, s in re.findall(r"\{0x([0-9A-F]+),(\d+)\}", runs_text)]
data = bytes(int(x) for x in re.findall(r"\d+", data_text))
assert sum(s for _, s in runs) == len(data)
p = 0
for addr, size in runs:
    img[addr - BEGIN:addr - BEGIN + size] = data[p:p + size]
    p += size
dst = os.path.join(root, "standalone", "gta_image.bin")
open(dst, "wb").write(struct.pack("<II", BEGIN, END) + img)
print("%s: [0x%X, 0x%X), %d start-up runs applied" % (dst, BEGIN, END, len(runs)))
