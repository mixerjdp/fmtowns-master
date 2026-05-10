import os
path = os.path.expanduser("/home/juan/.config/retroarch/config/FM Towns (MAME)/FM Towns (MAME).opt")
with open(path, "w") as f:
    f.write('fmtowns_model = "fmtownssj"\n')
print("Written:", repr(open(path).read()))
