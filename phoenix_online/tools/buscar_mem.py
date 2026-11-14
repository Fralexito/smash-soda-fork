# buscar_mem.py - SOLO LECTURA. Busca textos en la memoria del PES base y vuelca bytes alrededor.
# Uso: python -I buscar_mem.py <salida.txt> <ctx_bytes> texto1 texto2 ...
# No escribe en el proceso, no lo pausa, no usa depurador (OpenProcess con QUERY_INFORMATION|VM_READ).
import ctypes, ctypes.wintypes as w, sys, subprocess, re
out_path, ctx = sys.argv[1], int(sys.argv[2])
needles = [s.encode('ascii') for s in sys.argv[3:]]
k = ctypes.WinDLL('kernel32', use_last_error=True)
class MBI(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_void_p), ('AllocationBase', ctypes.c_void_p), ('AllocationProtect', w.DWORD),
                ('PartitionId', w.WORD), ('RegionSize', ctypes.c_size_t), ('State', w.DWORD), ('Protect', w.DWORD), ('Type', w.DWORD)]
k.OpenProcess.restype = w.HANDLE
k.VirtualQueryEx.argtypes = [w.HANDLE, ctypes.c_void_p, ctypes.POINTER(MBI), ctypes.c_size_t]
k.ReadProcessMemory.argtypes = [w.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
pid = None
for line in subprocess.check_output(['tasklist', '/FI', 'IMAGENAME eq PES2021.exe', '/FO', 'CSV', '/NH'], text=True).splitlines():
    p = line.strip('"').split('","')
    if len(p) > 1 and p[0].lower() == 'pes2021.exe':
        pid = int(p[1])
if pid is None:
    print('PES2021 no esta abierto'); sys.exit(1)
h = k.OpenProcess(0x0400 | 0x0010, False, pid)   # QUERY_INFORMATION | VM_READ
if not h:
    print('OpenProcess fallo', ctypes.get_last_error()); sys.exit(2)
addr = 0; hits = {n: [] for n in needles}; mbi = MBI()
while addr < 0x7FFFFFFFFFFF and k.VirtualQueryEx(h, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)):
    base = mbi.BaseAddress or 0; size = mbi.RegionSize
    if mbi.State == 0x1000 and mbi.Type == 0x20000 and not (mbi.Protect & 0x101) and size < 0x40000000:  # COMMIT, PRIVATE, legible
        pos = 0
        while pos < size:
            n = min(0x400000, size - pos)
            buf = ctypes.create_string_buffer(n); got = ctypes.c_size_t(0)
            if k.ReadProcessMemory(h, ctypes.c_void_p(base + pos), buf, n, ctypes.byref(got)) and got.value:
                data = buf.raw[:got.value]
                for nd in needles:
                    for m in re.finditer(re.escape(nd), data):
                        hits[nd].append(base + pos + m.start())
            pos += n
    addr = base + size
with open(out_path, 'w', encoding='utf-8') as f:
    for nd, lst in hits.items():
        f.write(f'### {nd.decode()}: {len(lst)} apariciones\n')
        for a in lst[:12]:
            lo = max(a - ctx, 0); buf = ctypes.create_string_buffer(ctx * 2 + 64); got = ctypes.c_size_t(0)
            k.ReadProcessMemory(h, ctypes.c_void_p(lo), buf, len(buf), ctypes.byref(got))
            f.write(f'--- {hex(a)} (volcado desde {hex(lo)}, {got.value} bytes)\n')
            raw = buf.raw[:got.value]
            for i in range(0, len(raw), 16):
                c = raw[i:i+16]
                f.write(f'{lo+i:x}  {c.hex(" "):<48}  {"".join(chr(b) if 32 <= b < 127 else "." for b in c)}\n')
k.CloseHandle(h)
print('hecho', {n.decode(): len(v) for n, v in hits.items()})
