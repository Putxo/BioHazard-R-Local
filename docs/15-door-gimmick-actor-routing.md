# 15 — Door gimmick: actor routing after the door_2p audit

Date: 2026-09-25. Status: static analysis; no gameplay execution and no new canonical patch.

## Reproducible input and repository preflight

Read `main` at `3654bdc7f316e11afa9d536021ee9f6d60afde95` and the newer research branch `research/door2p-routing` at `f1b987081710cbea5dff8bfa989935d335d73fca`. No open PRs were returned by the preflight. This work uses a separate branch rooted at that research commit; it does not overwrite the previous branch.

Both files were actually read and hashed in the current environment:

- Original January EXE: SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.
- Uploaded canonical v13 EXE: SHA-256 `3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa`.

The original is PE32 with ImageBase `0x00400000`; `.text` VA is `0x01B79000`, raw offset `0x400`. The addresses below are VAs, not RVAs. Inspection uses the actual PE section table and GNU objdump. No executable or asset is uploaded to GitHub.

## 1. Correction to the previous continuation label — CONFIRMED

The instruction `0x02591121` is NOT inside `PlayerDoorGimmickWaitState`.

It is inside `0x02590F60`, a virtual method of:

```
.?AVMoveState@door_gimmick@obj_model@chara@game@app@@
```

Exact RTTI chain:

```
vtable                     0x04D5D684
vtable[-1] COL              0x05336704
COL+0x0C TypeDescriptor     0x054855C0
TypeDescriptor+8            the decorated class name above
vtable[8], byte offset 0x20 0x01C4DF17 -> 0x02590F60
constructor                0x025907E0
vtable installation        0x02590812
```

`0x02590F60` checks the controlled actor's states against four DTIs: Start `0x05581DB4`, Move `0x05581D74`, End `0x05581D94`, and Wait `0x05581E14`. Comparing the Wait DTI does not make this function the Wait class itself.

## 2. A pl-only lookup precedes the hardcoded pad read — CONFIRMED

Minimal instruction evidence:

```asm
02590F92 mov eax,[ebp-8]        ; door MoveState
02590F95 mov ecx,[eax+10h]      ; stored actor serial
02590F98 push ecx
02590F99 call 01B95665h         ; network manager getter
02590FA0 call 01C07341h         ; -> 02DA3840h
02590FA5 mov [ebp-20h],eax
02590FA8 cmp dword ptr [ebp-20h],0
02590FAC jne 02590FB3h
02590FAE jmp 025914D6h          ; leave this update
```

`0x02DA3840` iterates the actor collection, calls `0x01C8C9A1` at `0x02DA3898`, skips non-pl entries at `0x02DA38A6`, and only then compares the actor serial to its argument at `0x02DA38BF`.

Therefore changing ONLY `push 0` at `0x02591118` to a P2 pad selector cannot make a non-pl Sub0 reachable through this update. The lookup must first be understood in the context of the door's initiating actor. This is a static conditional conclusion, not a claim that a particular campaign scene has been tested.

## 3. Initialization distinguishes a serial and a partner sentinel — CONFIRMED

The same vtable's slot 5 is `0x01C01AB8 -> 0x02590D00`.

It stores its second argument in `MoveState+0x10` at `0x02590D29`. When that stored value is nonnegative, it calls the same pl-only lookup at `0x02590DD3`. For a negative value it instead calls the non-Self resolver `0x01C4C9C3` at `0x02590DE6`.

The resulting actor is passed through the player cast and its controller is transitioned to `PlayerDoorGimmickStartState` (DTI `0x05581DB4`) at `0x02590E40..0x02590E4F`.

The update `0x02590F60` does not repeat this negative-sentinel fallback at its entry. Its treatment must be analyzed before assigning a new meaning to `MoveState+0x10` or changing the global finder.

## 4. Pad read itself — CONFIRMED

When a relevant actor state is present and the door predicate `0x01C8FE62 -> 0x025916D0` succeeds (it reads `uDoorGimmick+0x11B0`):

```asm
02591118 push 0
0259111A call 01C8B7F4h
0259111F mov ecx,eax
02591121 call 01B94F53h         ; -> 02DB0CF0h
```

A true result sets `MoveState+0x1D` to one at `0x02591130`. The semantic name of this state byte is not yet assigned.

## Immediate next work

Trace the producers of the MoveState entry arguments, the door's actor-selection/ActionCommand path, and the actual PlayerDoorGimmick state implementations. Establish whether Sub0 can reach them and whether the negative-sentinel path is deliberate AI-only handling. Keep v13 canonical. Do not globally widen `isPlayer`, do not change the process-local network index, and do not describe this checkpoint as a working door patch.
