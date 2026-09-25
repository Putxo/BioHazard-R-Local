#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BASE = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(
    "/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v13 SYMMETRIC AMMO RELIEF.exe"
)
OUT = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(
    "/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v14 DOOR GIMMICK PAD2.exe"
)

ASM = ROOT / "research/patches/door_gimmick_pad_v14.S"
WORK = Path("/mnt/data/rev1_local_coop_research")
WORK.mkdir(parents=True, exist_ok=True)
OBJ = WORK / "door_gimmick_pad_v14.o"
ELF = WORK / "door_gimmick_pad_v14.elf"
BIN = WORK / "door_gimmick_pad_v14.bin"
MAN = WORK / "local-coop-v14-door-gimmick-pad2-manifest.json"

EXPECTED_BASE = "3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa"
EXPECTED_OUT = "73fe1255697c47a624025ba40b33bab8df5812e76021bdc2d9acdeec3daf56ea"
EXPECTED_HELPER = "0c83033764b4760949441b145486edf44807acfbf6af352f1f31198bc2301ac3"

TEXT_VA = 0x01B79000
TEXT_RAW = 0x400
HELPER = 0x01C95300
CALLSITE = 0x02591118
CALLSITE_LEN = 14
OLD_CALLSITE = bytes.fromhex("6a00e8d5a66fff8bc8e82d3e60ff")


def off(va):
    return va - TEXT_VA + TEXT_RAW


def sha_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha(path):
    return sha_bytes(path.read_bytes())


def rel32(src, n, dst):
    return struct.pack("<i", dst - (src + n))


def diff_ranges(a, b):
    out = []
    start = None
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y and start is None:
            start = i
        elif x == y and start is not None:
            out.append((start, i))
            start = None
    if start is not None:
        out.append((start, min(len(a), len(b))))
    if len(a) != len(b):
        out.append((min(len(a), len(b)), max(len(a), len(b))))
    return out


if sha(BASE) != EXPECTED_BASE:
    raise RuntimeError(f"unexpected v13 SHA: {sha(BASE)}")

subprocess.run(["as", "--32", "-o", str(OBJ), str(ASM)], check=True)
subprocess.run(
    ["ld", "-m", "elf_i386", "-Ttext", hex(HELPER), "-o", str(ELF), str(OBJ)],
    check=True,
)
subprocess.run(
    ["objcopy", "-O", "binary", "-j", ".text", str(ELF), str(BIN)],
    check=True,
)

blob = BIN.read_bytes()
if sha_bytes(blob) != EXPECTED_HELPER:
    raise RuntimeError(f"unexpected helper SHA: {sha_bytes(blob)}")
if len(blob) != 35:
    raise RuntimeError(f"unexpected helper size: {len(blob)}")
if HELPER + len(blob) > 0x01C95400:
    raise RuntimeError("helper exceeds verified code cave")

base = BASE.read_bytes()
data = bytearray(base)

helper_off = off(HELPER)
if bytes(data[helper_off : helper_off + len(blob)]) != b"\xCC" * len(blob):
    raise RuntimeError("v14 helper cave is not empty")

site_off = off(CALLSITE)
got = bytes(data[site_off : site_off + CALLSITE_LEN])
if got != OLD_CALLSITE:
    raise RuntimeError(
        f"unexpected MoveState input callsite @ {CALLSITE:#x}: {got.hex()}"
    )

new_callsite = b"\xE8" + rel32(CALLSITE, 5, HELPER) + b"\x90" * 9
assert len(new_callsite) == CALLSITE_LEN

data[helper_off : helper_off + len(blob)] = blob
data[site_off : site_off + CALLSITE_LEN] = new_callsite

OUT.write_bytes(data)

if len(data) != len(base):
    raise RuntimeError("file size changed")
if sha(OUT) != EXPECTED_OUT:
    raise RuntimeError(f"v14 SHA mismatch: {sha(OUT)}")

# Exact lineage proof: remove only the helper and callsite hook.
rev = bytearray(data)
rev[helper_off : helper_off + len(blob)] = b"\xCC" * len(blob)
rev[site_off : site_off + CALLSITE_LEN] = OLD_CALLSITE
if bytes(rev) != base or sha_bytes(rev) != EXPECTED_BASE:
    raise RuntimeError("v14 lineage/reversibility check failed")

rr = diff_ranges(base, data)

manifest = {
    "base": BASE.name,
    "base_sha256": EXPECTED_BASE,
    "output": OUT.name,
    "output_sha256": sha(OUT),
    "status": "STATICALLY VERIFIED ONLY - gameplay not executed in this environment",
    "purpose": (
        "Let door_gimmick::MoveState query the pad belonging to its resolved actor "
        "during canonical local co-op, while preserving stock selector 0 otherwise."
    ),
    "helper": {
        "va": hex(HELPER),
        "size": len(blob),
        "sha256": sha_bytes(blob),
        "local_flag": "0x057D9188",
        "actor_selector_thunk": "0x01C6C746",
        "gamepad_singleton_thunk": "0x01C8B7F4",
        "pad_query_thunk": "0x01B94F53",
    },
    "callsite": {
        "va": hex(CALLSITE),
        "old": OLD_CALLSITE.hex(),
        "new": new_callsite.hex(),
        "context": "door_gimmick::MoveState slot8 (0x02590F60)",
    },
    "behavior": {
        "stock_or_online": "gLocalCoopActive=0 -> selector 0, byte-equivalent semantics",
        "local_p1": "resolved actor selector -> 0 -> PadData[0]",
        "local_sub0": "exact tracked Sub0 selector -> 1 -> PadData[1]",
        "other_actor": "selector -> 0",
        "network_preservation": "ThinkMode::Network is not modified and local flag gate preserves stock input selector",
    },
    "evidence": {
        "move_state_target": "MoveState+0x10 can resolve explicit actor or pPt/partner",
        "hardcoded_stock_selector": "0x02591118 pushes 0 before sGamePad query",
        "state_effect": "successful query sets MoveState+0x1D, which can transition internal state 2 -> 6",
        "net_param": "DoorGimmick_NetParam+0x04 is metadata-named mUsePlayer and is serialized as one byte",
    },
    "diff_vs_v13": {
        "same_file_size": len(data) == len(base),
        "different_bytes": sum(e - s for s, e in rr),
        "ranges": len(rr),
        "range_list": [
            {"start": hex(s), "end": hex(e), "bytes": e - s} for s, e in rr
        ],
    },
    "lineage_check": "Restoring only the helper cave and 14-byte callsite reproduces v13 byte-for-byte.",
    "limitations": [
        "Runtime validation is required.",
        "The exact semantic name of MoveState internal state 6 is not proven; it is terminal/no-op in the audited update.",
        "This patch addresses only the demonstrated hardcoded Pad 0 query in door_gimmick::MoveState.",
        "Other interaction families, QTEs, forced cameras, menus and cutscenes remain separate audits.",
    ],
}

MAN.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print(json.dumps(manifest, indent=2))
