#!/usr/bin/env python3
"""Read-only observations for one exact January experimental game image.

Does not patch, inject, suspend, send input, or certify gameplay. The executable
on disk AND its new in-memory code section must match the tested candidate.
No memory dump or absolute executable path is written to the report.
"""
from __future__ import annotations
import argparse
import ctypes
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import sys
import time

IMAGE_SHA = '0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378'
IMAGE_SIZE = 60_755_456
CODE_VA = 0x057E2000
CODE_SIZE = 2064
CODE_SHA = '2d13782b0b90ea593f8d1ae8abdd2c6df7be9c0522e6e250aec95d6c60a396e9'
TRACKER = 0x057D9184
CAMERA = 0x05799D3C
GAMEPAD = 0x057A7480
READ_ACCESS = 0x0010 | 0x1000  # VM_READ | QUERY_LIMITED_INFORMATION, never write.


def digest(data):
    return hashlib.sha256(data).hexdigest()


def pointer(value):
    return 0x10000 <= value < 0x80000000 and value % 4 == 0


def floats(data):
    return [v if math.isfinite(v) else None for v in struct.unpack('<'+'f'*(len(data)//4), data)]


class WindowsReader:
    def __init__(self, pid):
        if os.name != 'nt':
            raise OSError('Este lector requiere Windows; no se ha abierto ningun proceso.')
        if not 0 < pid < 2**32:
            raise ValueError('PID fuera de rango')
        from ctypes import wintypes as w
        self.k = ctypes.WinDLL('kernel32', use_last_error=True)
        self.k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
        self.k.OpenProcess.restype = w.HANDLE
        self.k.ReadProcessMemory.argtypes = [w.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                             ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
        self.k.ReadProcessMemory.restype = w.BOOL
        self.k.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, ctypes.POINTER(w.DWORD)]
        self.k.QueryFullProcessImageNameW.restype = w.BOOL
        self.k.CloseHandle.argtypes = [w.HANDLE]
        self.k.CloseHandle.restype = w.BOOL
        self.handle = self.k.OpenProcess(READ_ACCESS, False, pid)
        if not self.handle:
            raise ctypes.WinError(ctypes.get_last_error())
        self.pid = pid

    def image_path(self):
        from ctypes import wintypes as w
        buf = ctypes.create_unicode_buffer(32768)
        length = w.DWORD(len(buf))
        if not self.k.QueryFullProcessImageNameW(self.handle, 0, buf, ctypes.byref(length)):
            raise ctypes.WinError(ctypes.get_last_error())
        return Path(buf.value)

    def read(self, address, size):
        if not self.handle:
            raise OSError('Lector cerrado')
        if not (0 <= address < 2**(ctypes.sizeof(ctypes.c_void_p)*8) and 0 < size <= 8192):
            raise ValueError('Lectura fuera de limites')
        buf = ctypes.create_string_buffer(size)
        got = ctypes.c_size_t()
        if not self.k.ReadProcessMemory(self.handle, address, buf, size, ctypes.byref(got)):
            raise ctypes.WinError(ctypes.get_last_error())
        if got.value != size:
            raise OSError('Lectura parcial; no se interpretan bytes incompletos')
        return buf.raw

    def close(self):
        if self.handle:
            self.k.CloseHandle(self.handle)
            self.handle = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


def verify_image(reader, path):
    if path.stat().st_size != IMAGE_SIZE:
        raise ValueError('El proceso no usa el candidato de enero esperado (tamano).')
    with path.open('rb') as source:
        image_sha = hashlib.file_digest(source, 'sha256').hexdigest()
    if image_sha != IMAGE_SHA:
        raise ValueError('El proceso no usa el candidato de enero esperado (SHA-256).')
    if reader.read(0x00400000, 2) != b'MZ':
        raise ValueError('ImageBase cargado inesperado')
    if digest(reader.read(CODE_VA, CODE_SIZE)) != CODE_SHA:
        raise ValueError('El modulo cargado no coincide con el candidato; no continuar.')
    return image_sha


class Sampler:
    def __init__(self, reader):
        self.reader = reader

    def u32(self, address):
        return struct.unpack('<I', self.reader.read(address, 4))[0]

    def guarded(self, function):
        try:
            return function()
        except (OSError, ValueError, struct.error) as exc:
            return {'read_error': str(exc)}

    def actor(self, address):
        if not pointer(address):
            return None
        serial, mode = struct.unpack('<ii', self.reader.read(address+0xE3C, 8))
        pack = self.u32(address+0x1524)
        result = {'address': hex(address), 'serial': serial, 'think_mode': mode,
                  'position_xyz': floats(self.reader.read(address+0x40, 12)),
                  'move_rotate_aim': floats(self.reader.read(address+0x1670, 24)),
                  'pack_address': hex(pack)}
        result['herbs'] = self.guarded(lambda: self.u32(pack+0xC8)) if pointer(pack) else None
        return result

    def camera(self):
        camera = self.u32(CAMERA)
        if not pointer(camera):
            return None
        first, second = struct.unpack('<II', self.reader.read(camera+0xCE0, 8))
        result = {'address': hex(camera), 'self_manager': hex(first), 'partner_manager': hex(second)}
        result['self_target'] = self.u32(first+0x74) if pointer(first) else 0
        result['partner_target'] = self.u32(second+0x74) if pointer(second) else 0
        result['views'] = []
        for offset in (0x40, 0x1D0):
            raw = self.reader.read(camera+offset, 5)
            result['views'].append({'enabled': raw[0], 'mode': raw[3], 'display': raw[4]})
        return result

    def pad(self):
        pad = self.u32(GAMEPAD)
        if not pointer(pad):
            return None
        return {'address': hex(pad), 'start_pad_no': self.u32(pad+0x970),
                # Change fingerprints are observations, NOT proof of independent control.
                'slot_fingerprints': [digest(self.reader.read(pad+0x668+i*0xC0, 0xC0)) for i in range(2)]}

    def sample(self):
        before = self.reader.read(TRACKER, 8)
        sub, active = struct.unpack('<II', before)
        result = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  'active': active, 'tracked_sub0': hex(sub),
                  'sub0': self.guarded(lambda: self.actor(sub)),
                  'camera': self.guarded(self.camera), 'pad': self.guarded(self.pad),
                  'gameplay_certified': False}
        camera = result['camera']
        if isinstance(camera, dict) and 'self_target' in camera:
            result['self_camera_target_actor'] = self.guarded(lambda: self.actor(camera['self_target']))
        after = self.reader.read(TRACKER, 8)
        result['binding_changed_during_sample'] = before != after
        actor = result['sub0']
        result['local_split_structure_observed'] = bool(
            before == after and active and isinstance(actor, dict) and actor.get('think_mode') == 1
            and isinstance(camera, dict) and camera.get('partner_target') == sub
            and camera.get('self_manager') != camera.get('partner_manager')
            and camera.get('self_target', 0) not in (0, sub)
            and camera.get('views') == [{'enabled': 1, 'mode': 2, 'display': 0},
                                       {'enabled': 1, 'mode': 3, 'display': 0}])
        return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--pid', type=int, required=True, help='PID del juego ya iniciado')
    ap.add_argument('--output', type=Path, required=True, help='Archivo nuevo JSONL')
    ap.add_argument('--seconds', type=float, default=20)
    ap.add_argument('--interval', type=float, default=0.25)
    args = ap.parse_args()
    try:
        if not 0 < args.seconds <= 300 or not 0.05 <= args.interval <= 5:
            raise ValueError('Duracion 0..300 segundos; intervalo 0.05..5 segundos')
        if args.output.exists():
            raise ValueError('El informe ya existe; no se sobrescribe')
        with WindowsReader(args.pid) as reader:
            path = reader.image_path()
            if args.output.resolve() == path.resolve():
                raise ValueError('El informe no puede ser el ejecutable')
            image_sha = verify_image(reader, path)
            sampler = Sampler(reader)
            with args.output.open('x', encoding='utf-8') as report:
                report.write(json.dumps({'kind': 'header', 'pid': args.pid, 'image_name': path.name,
                    'image_sha256': image_sha, 'read_only': True, 'gameplay_certified': False})+'\n')
                start = time.monotonic()
                count = 0
                while time.monotonic()-start < args.seconds:
                    observation = sampler.guarded(sampler.sample)
                    report.write(json.dumps(observation, allow_nan=False)+'\n')
                    report.flush()
                    count += 1
                    if 'read_error' in observation:
                        break
                    time.sleep(args.interval)
        print(f'{count} observaciones guardadas. No equivalen a validar toda la campana.')
        return 0
    except (OSError, ValueError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 2

if __name__ == '__main__':
    raise SystemExit(main())
