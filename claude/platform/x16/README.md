# X16 Boot Flow Notes

The X16 path is split deliberately:

- `BOOT.BAS` owns splash/loading behavior.
- `PKX16` is the compiled cc65 frontend binary.
- The current host CLI and host file loaders are not part of this target.

## Current Boot Contract

`BOOT.BAS` currently assumes:

- map file: `ARCHIPELAGO.MAP`
- program file: `PKX16`
- starting RAM bank: `1`
- starting address: `$A000`

The intended flow is:

1. Show splash screen.
2. Wait for player input.
3. Select the starting bank with `POKE 0, MAPBANK`.
4. Load the map into banked RAM starting at `MAPADDR`.
5. Restore bank `0`.
6. Load the cc65 program.

## Important Assumption To Verify

The loader currently assumes that a `LOAD` into banked RAM continues spilling into higher RAM banks when the file exceeds the remaining space in the starting bank.

That assumption is plausible and useful, but it should be treated as provisional until verified against the real Commander X16 KERNAL/ROM behavior or emulator behavior.

The future banked map/storage implementation should depend on an explicit, tested memory contract rather than folklore.