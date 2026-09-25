#!/usr/bin/env python3
"""Experimental PcsSub script-command input correction; never executes the game.

Accepts only the exact LOCAL ROUTING EXPERIMENTAL candidate. Output and report
are created exclusively, with rollback of files created by this invocation on
failure. No original, existing output, or GitHub binary is ever modified.
"""
from __future__ import annotations
import argparse
import array
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

BASE_SHA = "0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378"
BASE_SIZE = 60_755_456
HELPER_VA = 0x01C95340
ENTRY_VAS = (0x02976C30, 0x029FF600)
STOCK_BODY = bytes.fromhex(
    "558bec81eccc000000535657518dbd34ffffffb933000000"
    "b8ccccccccf3ab59894df833c05f5e5b8be55dc20400"
)
HELPER = bytes.fromhex(
    "31c085c9742d833d88917d0500742481391cf4dc04751c8b1584917d05"
    "85d27412399178100000750a83ba400e0000010f94c0c20400"
)
HELPER_SHA = "962da0c9ff2da1afa6ae58a1e22036d5dd93b2513a1f42d3762095b9fd59aae7"


def sha(data: bytes | bytearray) -> str:
    return hashlib.sha256(data).hexdigest()


class PE:
    def __init__(self, data: bytes | bytearray):
        self.data = data
        if len(data) < 256 or data[:2] != b"MZ":
            raise ValueError("not a PE image")
        self.pe = struct.unpack_from("<I", data, 0x3C)[0]
        if self.pe + 248 > len(data) or data[self.pe:self.pe+4] != b"PE\0\0":
            raise ValueError("invalid PE header")
        self.opt = self.pe + 24
        if self.u16(self.pe+4) != 0x14C or self.u16(self.opt) != 0x10B:
            raise ValueError("expected PE32 i386")
        self.base = self.u32(self.opt+28)
        self.checksum_offset = self.opt+64
        sh = self.opt + self.u16(self.pe+20)
        self.sections = []
        for i in range(self.u16(self.pe+6)):
            pos = sh+i*40
            if pos+40 > len(data):
                raise ValueError("truncated section table")
            vs, rva, size, raw = struct.unpack_from("<4I", data, pos+8)
            if size and (raw < self.u32(self.opt+60) or raw+size > len(data)):
                raise ValueError("invalid raw section extent")
            self.sections.append({
                "name": bytes(data[pos:pos+8]).rstrip(b"\0").decode("ascii"),
                "va": self.base+rva, "virtual_size": vs, "raw": raw,
                "raw_size": size, "flags": self.u32(pos+36),
            })

    def u16(self, offset: int) -> int:
        return struct.unpack_from("<H", self.data, offset)[0]

    def u32(self, offset: int) -> int:
        return struct.unpack_from("<I", self.data, offset)[0]

    def offset(self, va: int, size: int = 1) -> int:
        if size < 0:
            raise ValueError("negative read size")
        for s in self.sections:
            delta = va - s["va"]
            if delta >= 0 and delta+size <= s["raw_size"]:
                return s["raw"] + delta
        raise ValueError(f"address is not fully file-backed: {va:#x}")

    def read(self, va: int, size: int) -> bytes:
        start = self.offset(va, size)
        return bytes(self.data[start:start+size])


def checksum(data: bytes | bytearray, offset: int) -> int:
    copy = bytearray(data)
    struct.pack_into("<I", copy, offset, 0)
    if len(copy) & 1:
        copy.append(0)
    words = array.array("H")
    words.frombytes(copy)
    if sys.byteorder != "little":
        words.byteswap()
    result = sum(words)
    while result >> 16:
        result = (result & 0xFFFF) + (result >> 16)
    return (result + len(data)) & 0xFFFFFFFF


def build(base: bytes) -> tuple[bytes, dict]:
    if len(base) != BASE_SIZE or sha(base) != BASE_SHA:
        raise ValueError("requires the exact LOCAL ROUTING EXPERIMENTAL base")
    if len(HELPER) != 54 or sha(HELPER) != HELPER_SHA:
        raise ValueError("embedded selector has changed")
    pe = PE(base)
    if pe.base != 0x400000 or len(pe.sections) != 10:
        raise ValueError("unexpected base layout")
    if pe.u16(pe.opt+70) & 0x40 or not pe.u16(pe.pe+22) & 1:
        raise ValueError("requires the fixed-address, relocation-stripped image")
    if pe.u32(pe.opt+96+4*8) or pe.u32(pe.opt+96+5*8):
        raise ValueError("unexpected certificate or relocation directory")
    for va in ENTRY_VAS:
        if pe.read(va, len(STOCK_BODY)) != STOCK_BODY:
            raise ValueError(f"callback is not the audited stock body: {va:#x}")
    if pe.read(HELPER_VA, len(HELPER)) != b"\xCC"*len(HELPER):
        raise ValueError("selector cave is occupied")
    data = bytearray(base)
    records: list[dict] = []

    def patch(offset: int, old: bytes, new: bytes, label: str, va: int | None):
        if len(old) != len(new) or data[offset:offset+len(old)] != old:
            raise ValueError(f"unexpected bytes for {label}")
        if any(offset < r["offset"]+r["size"] and
               r["offset"] < offset+len(old) for r in records):
            raise ValueError("overlapping modifications")
        data[offset:offset+len(old)] = new
        records.append({"label": label, "va": va, "offset": offset,
                        "size": len(old), "old": old.hex(), "new": new.hex(),
                        "changed_bytes": sum(a != b for a,b in zip(old,new))})

    patch(pe.offset(HELPER_VA,len(HELPER)), b"\xCC"*len(HELPER), HELPER,
          "exact PcsSub owner and Pad actor selector", HELPER_VA)
    for va in ENTRY_VAS:
        jump = b"\xE9"+struct.pack("<i", HELPER_VA-va-5)+b"\x90"*4
        patch(pe.offset(va,9), STOCK_BODY[:9], jump, "script member entry", va)
    cs = pe.checksum_offset
    patch(cs, base[cs:cs+4], struct.pack("<I",checksum(data,cs)), "PE checksum", None)
    output = bytes(data)
    reverse = bytearray(output)
    for r in records:
        offset = r["offset"]
        if reverse[offset:offset+r["size"]] != bytes.fromhex(r["new"]):
            raise ValueError("reversal precondition failed")
        reverse[offset:offset+r["size"]] = bytes.fromhex(r["old"])
    if bytes(reverse) != base or sha(reverse) != BASE_SHA:
        raise ValueError("exact reversibility failed")
    if PE(output).sections != pe.sections or len(output) != len(base):
        raise ValueError("image layout unexpectedly changed")
    report = {
        "status": "EXPERIMENTAL; COMPONENT TESTS ONLY; GAMEPLAY NOT EXECUTED",
        "base_sha256": BASE_SHA, "output_sha256": sha(output),
        "file_size": len(output), "helper_sha256": HELPER_SHA,
        "helper_va": HELPER_VA, "helper_size": len(HELPER),
        "entry_vas": list(ENTRY_VAS), "changes": records,
        "different_bytes": sum(r["changed_bytes"] for r in records),
        "identical_layout": True, "reversed_exactly_to_base": True,
        "behavior": "member 1 only for active exact cFsmActionPcsSub whose +0x1078 is the tracked Sub0 in Pad mode; otherwise 0",
        "unchanged": ["uPcsInput callback 0x02DEE8D0", "existing .lcfix/.lcdata modules",
                      "global Self/serial/GameMode", "Network control mode"],
        "limitations": ["No game execution", "No all-campaign QTE claim",
                        "uPcsInput ownership remains a separate question",
                        "HUD, menus, checkpoints, cutscenes and missing-partner scenes not completed by this patch"],
    }
    return output, report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args(argv)
    created: list[Path] = []
    try:
        src, dst, report_path = (p.resolve() for p in (args.input,args.output,args.report))
        if len({src,dst,report_path}) != 3:
            raise ValueError("input, output and report must be distinct paths")
        # lexists rejects dangling symlinks too; O_EXCL below closes the TOCTOU window.
        if any(os.path.lexists(p) for p in (args.output,args.report)):
            raise ValueError("output/report already exists; no overwrite is allowed")
        if not src.is_file() or not dst.parent.is_dir() or not report_path.parent.is_dir():
            raise ValueError("input file and output parent directories must exist")
        output, report = build(src.read_bytes())
        report.update(input_name=args.input.name, output_name=args.output.name)
        payloads = ((dst,output), (report_path,(json.dumps(report,indent=2)+"\n").encode("utf-8")))
        for path, payload in payloads:
            with path.open("xb") as stream:
                created.append(path)
                stream.write(payload)
                stream.flush()
                os.fsync(stream.fileno())
        print(json.dumps({"output": str(dst), "sha256": report["output_sha256"],
                          "bytes": len(output), "report": str(report_path)}))
        return 0
    except (OSError, ValueError, struct.error) as exc:
        for path in reversed(created):
            try:
                path.unlink()
            except OSError:
                pass
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
