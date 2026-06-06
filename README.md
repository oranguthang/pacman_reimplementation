# Pac-Man Reimplementation (NES)

A from-scratch reimplementation of NES Pac-Man in C, built on top of
`nes-starter-kit`.

This is **not** a port of the original game. The original ROM was disassembled,
and this project re-writes its behavior, graphics data, and game logic as new C
and assembly code, using that disassembly as a reference for accuracy. The
original ROM is used only as a source of data (tile streams, palettes, CHR) and
as a behavioral reference — none of its machine code is reused.

> ⚠️ **Heavily work in progress — may not work.**
> This project is far from finished. Large parts of the game (ghost AI, scoring,
> level flow, HUD, frightened mode, fruits) are missing or only partially
> implemented, and a build may not run correctly or at all on a given machine.
> Treat it as an experimental, incomplete code drop, not a playable game.

## Current state

The repository has moved past the starter-kit demo and now contains a rough
Pac-Man skeleton. What is present:

- A title screen in the style of the original Pac-Man, with logo and a
  `1 PLAYER` / `2 PLAYERS` selection.
- A maze layout recreated to match the original, with matching tile IDs and
  palette/attribute data.
- Pac-Man sprite rendering and animation close to the original, driven by CHR
  graphics (the CHR itself is not bundled — see [Graphics data](#graphics-data)).
- Four-direction grid movement (no diagonals).
- Wall collision against the maze.
- Buffered turns: if a turn is currently blocked by a wall, Pac-Man keeps going
  straight and turns at the first available node.
- Pellets and power pellets as level state, with the tile cleared when eaten.
- A local workflow with no hard-coded `D:/...` paths.

Not done yet:

- Frightened mode, scoring, and level completion on top of the pellet logic.
- Ghosts and their AI.
- Score, lives, fruits, and a proper HUD.
- A full round game loop, death handling, and advancing to the next level.

## Quick start

Requirements:

- Windows
- `git`
- `node`
- `npm`
- `make`

First-time setup:

```powershell
make install-dev
```

This:

- places `create-nes-game` in `tools/create-nes-game`;
- installs its npm dependencies;
- downloads build dependencies into `tools/`;
- installs a local `FCEUX` into `tools/emulators/fceux`.

Build:

```powershell
make build
```

Run:

```powershell
make run
```

or

```powershell
make start
```

The ROM is built to:

```text
rom/nes-starter-kit-example.nes
```

## Working with the original ROM

If you place the following file in the repository root:

```text
Pac-Man (J) (V1.0) [!].nes
```

then:

```powershell
make split-rom
```

splits it into:

- `temp/pacman_rom_split/header.bin`
- `temp/pacman_rom_split/prg.bin`
- `temp/pacman_rom_split/chr.bin`
- `temp/pacman_rom_split/metadata.json`

## Graphics data

The build expects two CHR files that are **not** included in this repository:

- `graphics/tiles.chr`
- `graphics/sprites.chr`

These contain artwork from the original Pac-Man ROM (copyrighted by Namco), so
they are intentionally git-ignored and must not be committed. To build the
project you have to provide them locally, generated from a copy of the ROM you
legally own (the `make split-rom` step above extracts the original CHR data as a
starting point).

`graphics/ascii.chr` (a generic font shipped with `nes-starter-kit`) and the
`graphics/palettes/*.pal` color tables are not copyrighted artwork and are kept
in the repository.

## Layout

Key locations in the repository:

- `source/c/pacman/` — Pac-Man-specific title/render logic.
- `source/c/map/map.c` — loading and rendering of the game screen.
- `source/c/sprites/player.c` — Pac-Man movement, buffered turns, animation, and
  collisions.
- `tools/scripts/install-dev.ps1` — local toolchain setup.
- `tools/scripts/split-rom.js` — splitting the original ROM.
- `PROGRESS.md` — live status of the reimplementation and roadmap.

## Next tasks

The next practical milestone:

1. Bring the HUD back into a Pac-Man layout instead of the starter-kit layout.
2. Wire pellets to scoring, the power pellet flow, and level completion.
3. Implement the ghosts and the round logic.

## Legal

Pac-Man is a trademark of and copyrighted by Bandai Namco Entertainment. This is
an unofficial, non-commercial, educational reimplementation project and is not
affiliated with or endorsed by Bandai Namco. No copyrighted ROM data or artwork
is distributed with this repository; any such data must be obtained from a copy
of the original game that you legally own.
