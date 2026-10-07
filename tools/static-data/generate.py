#!/usr/bin/env python3
"""
Decides which of the original executable's global data can live in our own memory, and generates the
table of it (with its initial values) that `source/StaticData.cpp` uses.

Run from the repository root:  python3 tools/static-data/generate.py [path/to/gta_sa_compact.exe]

Inputs (in `tools/static-data/input/`):
  bn_functions.txt     Start address of every function Binary Ninja found in the executable (see `export_bn.py`)
  bn_datarefs.json     For each data address, the functions that refer to it (see `export_bn.py`)
  static_data_map.txt  Written by the game when the line `map` is in `gta-reversed-static-data.txt`:
                       address and size of every global declared with `StaticRef<T, 0xADDRESS>()`

A global is taken only if nothing but our own code can be using its original location:
  - Every function referring to (any address inside) it has an active hook install, and isn't a stub forwarding to the original
  - Nothing in the executable's static data looks like a pointer into it
  - The source doesn't touch its address in any other way than the `StaticRef` declaration
  - Its size is known (it has been seen in the map)
  - It isn't followed by an undeclared address that fails the above (it may be bigger than declared, and contain that address)
"""
import bisect, collections, json, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = sys.argv[1] if len(sys.argv) > 1 else 'gta_sa_compact.exe'
OUT_INC = 'source/StaticDataInit.inc'

GAME_END = 0x748F20                        # End of the game's own code (RenderWare, the C runtime etc. follow)
DATA_LO, DATA_HI = 0x857000, 0xCB1000
# (address, virtual size, size in file, offset in file) of the data sections of the 1.0 US compact executable
SECTIONS = [(0x858000, 0x4C000, 0x4B400, 0x456800), (0x8A4000, 0x3FA000, 0x40000, 0x4A1C00),
            (0xC9E000, 0x11000, 0x10C00, 0x4E1C00), (0xCAF000, 0x1000, 0x200, 0x4F2800)]

exe = open(EXE, 'rb').read()
assert len(exe) == 5189632, 'Not the 1.0 US compact executable'

def initial_bytes(b, e):
    out = bytearray(e - b)
    for va, vs, rs, ro in SECTIONS:
        lo, hi = max(b, va), min(e, va + rs)
        if lo < hi:
            out[lo - b:hi - b] = exe[ro + lo - va:ro + hi - va]
    return bytes(out)

funcs = [int(l, 16) for l in open(HERE + '/input/bn_functions.txt') if l.strip()]
refs = {int(k, 16): v for k, v in json.load(open(HERE + '/input/bn_datarefs.json')).items()}

# --- Scan the source
hookre = re.compile(r'RH_Scoped[A-Za-z]*Install[A-Za-z]*\([^;]*?0x([0-9A-Fa-f]{6,8})')
stubre = re.compile(r'plugin::Call[A-Za-z]*<[^;]*?0x([0-9A-Fa-f]{6,8})|\)\s*\(?0x([0-9A-Fa-f]{6,8})\)?\)\s*\(|reinterpret_cast<[^;>]*\(\s*(?:__\w+\s*)?\*\s*\)[^;]*>\(0x([0-9A-Fa-f]{6,8})\)|mov\s+e[a-d]x\s*,\s*0x([0-9A-Fa-f]{6,8})')
declre = re.compile(r'(?:([A-Za-z_]\w*)\s*=\s*)?StaticRef<(.*?),\s*(0x[0-9A-Fa-f]+|\d+)>\(\)')
newform = re.compile(r'StaticRef<.*?,\s*(?:0x[0-9A-Fa-f]+|\d+)>\(\)')
hexre = re.compile(r'0x([0-9A-Fa-f]{6,8})\b')
hooks, stubs, raw, decl = set(), set(), set(), {}
for root, _, fs in os.walk('source'):
    for f in fs:
        if not f.endswith(('.cpp', '.h', '.hpp')):
            continue
        for l in open(os.path.join(root, f), errors='ignore'):
            if not l.lstrip().startswith('//'):
                for m in hookre.finditer(l):
                    hooks.add(int(m.group(1), 16))
                for m in stubre.finditer(l):
                    stubs.add(int(next(g for g in m.groups() if g), 16))
            for m in declre.finditer(l):
                a = int(m.group(3), 0)
                if DATA_LO <= a < DATA_HI:
                    decl.setdefault(a, os.path.splitext(f)[0])
            for m in hexre.finditer(newform.sub('', l).split('//')[0]):
                v = int(m.group(1), 16)
                if DATA_LO <= v < DATA_HI:
                    raw.add(v)
ours = (hooks - stubs) & {f for f in funcs if f < GAME_END}

# --- Sizes, from the game
size = {}
for l in open(HERE + '/input/static_data_map.txt'):
    if not l.startswith('#'):
        a, s, _ = l.split()
        a, s = int(a, 16), int(s)
        if DATA_LO <= a < DATA_HI and s:
            size[a] = max(size.get(a, 0), s)
objs = []  # Globals, overlapping declarations merged
for b, e in sorted((a, a + s) for a, s in size.items()):
    if objs and b < objs[-1][1]:
        objs[-1][1] = max(objs[-1][1], e)
    else:
        objs.append([b, e])

# --- Hazards: addresses that can't move
ptrs = set()
for va, vs, rs, ro in SECTIONS:
    for off in range(0, rs - 3, 4):
        v = struct.unpack_from('<I', exe, ro + off)[0]
        if DATA_LO <= v < DATA_HI:
            ptrs.add(v)
haz = {}
for a, r in refs.items():
    if DATA_LO <= a < DATA_HI and any(f not in ours for f in r):
        haz[a] = 'referenced by code that is not ours'
for v in ptrs:
    haz.setdefault(v, 'pointed to by static data')
for v in raw:
    haz.setdefault(v, 'touched by raw address in the source')
for a in decl:
    if a not in size:
        haz.setdefault(a, 'declared, but its size is not known (not seen in the map)')
hz = sorted(haz)
def hazards_in(b, e):
    return hz[bisect.bisect_left(hz, b):bisect.bisect_left(hz, e)]

why = collections.Counter()
safe, blocked = [], []
for b, e in objs:
    h = hazards_in(b, e)
    if h:
        why[haz[h[0]]] += 1
        blocked.append((b, e))
    else:
        safe.append((b, e))

# --- Regions: consecutive safe globals, up to the next thing that can't move.
# (A global may be bigger than its declaration says, and code reaches into arrays at fixed offsets.)
stops = sorted(set(hz) | {b for b, e in blocked})
def next_stop(a):
    lim = next((va + vs for va, vs, rs, ro in SECTIONS if va <= a < va + vs), a)
    i = bisect.bisect_left(stops, a)
    return min(stops[i], lim) if i < len(stops) else lim
regions = []
for b, e in safe:
    if regions and b < regions[-1][1]:
        continue
    end = next_stop(b)
    if end >= e:
        regions.append([b, end])
# A region stopped by a hazard at an address nothing is declared at: that address may be inside the global right
# before it (declared too small), so that global can't be trusted to be safe.
blockedstarts = {b for b, e in blocked}
safestarts = [b for b, e in safe]
out, dropped = [], 0
for b, end in regions:
    if end in haz and end not in decl and end not in blockedstarts and end not in size:
        last = safestarts[bisect.bisect_left(safestarts, end) - 1]
        dropped += 1
        if last <= b:
            continue
        end = last
    out.append((b, end))
regions = out

ncov = sum(1 for a in decl if any(b <= a < e for b, e in regions))
print(f'hooked functions that are ours: {len(ours)} | stubs: {len(stubs & set(funcs))}')
print(f'declared globals: {len(decl)} | with known size: {len(size)}')
print(f'owned: {ncov} globals ({100 * ncov // len(decl)}%) in {len(regions)} ranges, {sum(e - b for b, e in regions)} bytes')
print('left out:', dict(why), f'| followed by an undeclared hazard: {dropped}')

# --- Output
L = ['// Generated by `tools/static-data/generate.py`, do not edit.',
     '// The parts of the original executable\'s data that live in our own memory, with their initial values.',
     '// Trailing zero bytes of the initial values are left out.', '']
table = []
ninit = 0
for b, e in regions:
    files = collections.Counter(decl[a] for a in decl if b <= a < e)
    name = '+'.join(f for f, _ in files.most_common(2)) or '?'
    data = initial_bytes(b, e).rstrip(b'\0')
    sym = 'nullptr'
    if data:
        sym = f's_Init_{b:06X}'
        ninit += len(data)
        L.append(f'static const uint8 {sym}[]{{')
        for i in range(0, len(data), 32):
            L.append('    ' + ','.join(f'0x{x:02X}' for x in data[i:i + 32]) + ',')
        L.append('};')
    table.append(f'    {{ 0x{b:06X}, 0x{e:06X}, "{name}", {sym}, {len(data)} }}, // {sum(files.values())} globals')
L += ['', 'static const GeneratedRange GENERATED_RANGES[]{'] + table + ['};', '']
open(OUT_INC, 'w', newline='\n').write('\n'.join(L))
print(f'wrote {OUT_INC}: {len(regions)} ranges, {ninit} bytes of initial values')
