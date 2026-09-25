# Script member ownership — independent January audit

Date: 2026-09-25. Base source commit: `5bcf15adf2f3e6526c71d5b6d3fdbeb9a841559f`.

Original EXE inspected locally: SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`, 60,748,800 bytes. No game execution.

The previous local-routing module was independently rebuilt from its published `core.cpp`, `hooks.S` and `link.ld`; the resulting PE reproduced `0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378`, 60,755,456 bytes, and reversed exactly to the owner-fixed base. This identifies the input for this continuation; it does not establish gameplay correctness.

## Three different owners, not three interchangeable player callbacks

| Callback | Thunk | Constructor binding | Owner |
|---|---|---|---|
| `0x02976C30` | `0x01BBB149` | PUSH at `0x029707B1`; group installation at `0x029708CE` | cFsmAction |
| `0x029FF600` | `0x01C7E054` | PUSH at `0x029FD2E7`; group installation at `0x029FD3F5` | cFsmActionPcs |
| `0x02DEE8D0` | `0x01C0758A` | PUSH at `0x02DEE540`; group installation at `0x02DEE629` | uPcsInput |

All three original callbacks return zero and use `ret 4`. cFsmAction constructs six commands at `+0xA0`, stride `0x110`. cFsmActionPcs constructs six additional commands at `+0x860`. uPcsInput constructs a command at `+0x50`.

RTTI confirms cFsmAction vtable `0x04DBD47C` through COL `0x05365424` and type descriptor `0x054B6DBC`. cFsmActionPcs uses vtable `0x04DC7AD8`, COL `0x0536AEF0`, descriptor `0x054BC418`. uPcsInput uses vtable `0x04E15EF4`, COL `0x0538BF30`, descriptor `0x054E1DCC`.

## A narrowly identifiable actor-owned subclass

cFsmActionPcsSub constructor `0x02A75BC0` calls the cFsmActionPcs constructor and installs vtable `0x04DCF41C` at `0x02A75BEE`. Its RTTI chain is COL `0x05374250` -> descriptor `0x054C5294` -> `.?AVcFsmActionPcsSub@fsm@game@app@@`.

The constructor initializes `+0x1070=0`, `+0x1074=-1`, `+0x1078=null`.

The setter `0x02DC FBC0` (without the formatting space: `0x02DCFBC0`) stores its three arguments at those offsets. Its caller `0x02DCF960` loads the actor/context argument from `sPcsManager+0xB44 + group*0x44 + slot*4`, and the associated serial from the parallel table at `+0x704`. The table writer `0x02DD04E0` stores the object and derives its serial using `0x01BEDBB7 -> 0x01CB7610` (object+0xE3C).

Consequently, comparing a verified cFsmActionPcsSub instance's `+0x1078` with the already-tracked live Sub0 is grounded in actual producer/consumer code. Reading this offset in every cFsmAction or uPcsInput would not be justified.

## Proposed bounded correction

The two FSM member callbacks may share a leaf helper returning 1 only for the exact cFsmActionPcsSub vtable, an active local session, a non-null tracked Sub0 still in Pad mode, and a matching actor/context and serial. Every other case retains the original zero. The context pointer is compared, not blindly dereferenced.

uPcsInput remains unchanged at this checkpoint. Its base `uPcs+0x30` is found by searching a parent object's contained units; this is not demonstrated to be a player pointer. Replacing its zero with Pad 2 merely because the field exists would repeat the earlier owner/layout error.

Status: ownership evidence recorded; new helper construction, byte-level tests and integration still pending. No claim that HUD, menus, checkpoints, cutscenes, actor creation in partnerless scenes or the full game are complete.
