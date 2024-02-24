# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# Helper script that generates stub implementation of all known nids + lookup tables
# for shadps4

import sys, os
import struct
#from hashlib import sha1
import hashlib
from base64 import b64encode as base64enc
from binascii import unhexlify as uhx

#ref https://github.com/SocraticBliss/ps4_name2nid_plugin
#ref derived from https://github.com/zecoxao/sce_symbols.git

# needs ps4_names.txt (see: https://github.com/zecoxao/sce_symbols.git for full list)
# generates "stubs.inl" to include in Core\PS4\Util

NEW_NIDS = {}
NAMES   = 'ps4_names.txt'

def name2nid(name):
    symbol = hashlib.sha1(name.encode() + uhx('518D64A635DED8C1E6B039B1C3E55230')).digest()
    id     = struct.unpack('<Q', symbol[:8])[0]
    nid    = base64enc(uhx('%016x' % id), b'+-').rstrip(b'=')
    NEW_NIDS[nid]=name

def save_stubs(NIDS):
    nidsSorted=sorted(NIDS.items(), key=lambda x: x[0])

    nidsFile=open("aerolib.inl", "w")
    nidsFile.writelines('// generated using ps4_names2stubs.py\n\n')
    for nid, name in nidsSorted:
        nidsFile.writelines('STUB("%s", %s)\n' % (str(nid,'utf-8'), name))

    nidsFile.close()



f = open(NAMES,"r")
for line in f.readlines():
    line = line.strip()
    name2nid(line)

f.close()

save_stubs(NEW_NIDS)
