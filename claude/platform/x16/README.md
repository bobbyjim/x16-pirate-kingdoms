# X16 Boot Flow Notes

The X16 path is split deliberately:

- `BOOT.BAS` owns splash/loading behavior.
- `PKX16` is the compiled cc65 frontend binary.
- The current host CLI and host file loaders are not part of this target.

## Current Boot Contract

`BOOT.BAS` currently uses a basename bundle:

- `<base>.MAP` for static world/map data
- `<base>.SAV` for dynamic game state
- `PKX16` for the program binary

Default values in the script are:

- basename: `PKGAME`
- map start: bank `1`, address `$A000`
- save start: bank `20`, address `$A000`
- program: `PKGAME.PRG`

The intended flow is:

1. Show splash screen.
2. Wait for player input.
3. Select the map bank with `POKE 0, MB`.
4. Load `<base>.MAP` into banked RAM at `MA`.
5. Select the save bank with `POKE 0, SB`.
6. Load `<base>.SAV` into banked RAM at `SA`.
7. Restore bank `0`.
8. Load `<base>.PRG`.

## Important Assumption To Verify

The loader currently assumes that a `LOAD` into banked RAM continues spilling into higher RAM banks when the file exceeds the remaining space in the starting bank.

That assumption is plausible and useful, but it should be treated as provisional until verified against the real Commander X16 KERNAL/ROM behavior or emulator behavior.

The future banked map/storage implementation should depend on an explicit, tested memory contract rather than folklore.