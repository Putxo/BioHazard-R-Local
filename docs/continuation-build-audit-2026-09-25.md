# Continuation build audit — 2026-09-25

Base ref inspected: `d67b13836386dfb5f2f95654c10aec43adbe3546`, after PR #3 was merged as `ae57c34eead9d178ffb1f8bf03402711aa7707a8` into research/door2p-routing. main was still `3654bdc7f316e11afa9d536021ee9f6d60afde95` at preflight.

## Local inputs reidentified

All inputs remain local; no game EXE/DLL/assets are uploaded.

- Original January: 60,748,800 bytes, SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.
- v13: 60,748,800 bytes, SHA-256 `3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa`.
- Owner-corrected experimental base: 60,748,800 bytes, SHA-256 `e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a`.

## Recovered source, not rewritten from summaries

The local copies used for compilation have the exact remote Git blob hashes:

- core.cpp: `5aa0960d646540fce9ee13c92c483c31997ac401`.
- hooks.S: `c5df39261afa1bdff1782807bd4b75fd61387d1e`.
- link.ld: `e28d640bffd784e7523f1effa9c1832a72dfbcb5`.

The module compiles to ELF32 i386 with the fixed engine addresses supplied by link.ld. This is compilation, not execution of the game.

## Checks actually executed in this continuation

A new host test suite executes the recovered C++ with mocked engine APIs. It passed 204 assertions with an optimized host build and again with AddressSanitizer/UndefinedBehaviorSanitizer. Coverage includes local-off fallback, Pad/CPU/Network modes, null pointers, wrong actors, duplicate/out-of-range serials, nearest eligible sensor candidate, invalid coordinates, stale binding rejection, table capacity, actor-specific inventory member, rescue counterpart selection, preservation of the other four delegates, and delayed activation of component selectors.

These are host behavioral checks. They do not execute Win32 thiscall, the game's machine code, rendering, input devices or gameplay. An attempted tiny i386 ELF probe returned Exec format error in this environment.

## Local instruction evidence rechecked

- Binder 0x02DF4F30 allocates 0x114 bytes; preserves previous actor at [ebp-0x20]; clears +0x44 at 0x02DF4FA7; restores binding at 0x02DF501F. Its matching release is at 0x02DF505F. The proposed two snapshot locals require all four frame-size/initialization edits, not merely the allocation.
- Door proximity starts with Self at 0x025902C4, availability at 0x0259035D, and the pl/Self-only sensor branch at 0x025903EE..0x02590429.
- WaitState reads Self at 0x02590072 and process-local serial at 0x025900B4. These must agree with the chosen sensor actor and the actual uObjModel ActionCommand member callback.
- Native ActionGroup copying at 0x02335AA0 copies five delegates at offsets 0, 0x0C, 0x18, 0x24 and 0x30. The static manager at 0x01CEB6F0 copies/clears/compares its pointer; it does not free that pointee.
- The four proposed actor-group installation sites are 0x02706B37, 0x027088E9, 0x026E9B53 and 0x026EC5B3. The first two use actor local [ebp-0x14]; the latter two component initializers save their owning actor from [ebp+8] into component+4.

## Not yet claimed

At this checkpoint the new PE builder and bridge verification are still being completed. No new executable is declared playable. HUD, pause/menu ownership, scripted events, forced cameras, death/revive semantics and checkpoints are not certified by the host assertions. They need separate evidence and actual compatible game assets/runtime.
