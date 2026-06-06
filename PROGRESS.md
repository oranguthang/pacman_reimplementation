# PROGRESS

## Goal

Build a playable Pac-Man on top of `nes-starter-kit`, gradually replacing
starter-kit behavior with Pac-Man-specific graphics, layout, and mechanics.

## What already works

### Infrastructure

- Machine-specific paths removed from the `Makefile`.
- `make install-dev` brings up a local toolchain via `tools/create-nes-game`.
- `make build`, `make run`, and `make start` work through local paths in
  `tools/`.
- `make split-rom` splits `Pac-Man (J) (V1.0) [!].nes` into `header/prg/chr`.

### Title screen

- The Pac-Man title screen is hooked up in place of the ASCII starter-kit title.
- The logo is rendered with tiles from the Pac-Man CHR.
- Text and logo palette/attributes are in a working state.
- The `1 PLAYER` / `2 PLAYERS` menu reliably starts the game.

### Maze

- The game screen is no longer drawn with an ASCII placeholder.
- The exact maze tile stream from the original ROM is hooked up.
- Incorrectly imported bottom maze rows were fixed.
- The original gameplay palette and maze attributes are wired up.
- The maze visually matches the original much more closely than the starter-kit
  map.
- Pellets and power pellets now live as level state in RAM.
- Eaten pellets are cleared with a targeted VRAM update and do not reappear after
  the screen is redrawn.

### Pac-Man sprite

- The player is no longer drawn with menu tiles or ASCII garbage.
- The original Pac-Man sprite tiles and palette are used.
- Pac-Man animation is in a working state, close to the original.
- A stray single tile in the top-left corner was fixed: unused gameplay sprite
  slots are now hidden.

### Movement and collisions

- The player start position was moved to the bottom-center area of the maze.
- Movement is limited to four directions: `up/left/right/down`.
- No diagonal movement.
- Blue walls act as a barrier.
- Basic grid alignment is in place.
- Buffered turns are in place:
  - reversing direction triggers immediately;
  - a sideways turn can be pressed ahead of time;
  - if a turn is currently blocked by a wall, Pac-Man keeps moving and turns at
    the first available spot.
- The needed passages around the start area and above the ghost-house exit are
  opened.

## What is not ready yet

### Game logic

- No power pellets and no frightened mode.
- Pellets do not yet award points or contribute to level completion.
- No score and no hi-score.
- No lives and no restart-round flow.
- No level completion and no transition to the next level.

### Ghosts

- No 4 full ghosts.
- No scatter/chase/frightened AI.
- No house-exit logic.

### Interface

- The HUD is not yet restored to a full Pac-Man layout.
- No score row / hi-score / fruits / life icons in their final form.

### Fidelity to the original

- Player behavior already feels close, but is not yet a pixel-perfect replica.
- Some individual gameplay details of the original have not been reimplemented yet.

## Recent notable commits

- `4bfa3df` — `Localize toolchain and vendor Pac-Man references`
- `3d4ea7d` — `Fix Pac-Man buffered turn movement`
- `82c386f` — `Open ghost house side lanes`
- `f0e0b81` — `Improve Pac-Man turn buffering`
- `09959a8` — `Add Pac-Man grid movement and wall collisions`
- `ce03703` — `Hide unused gameplay sprite slots`
- `6029074` — `Refine Pac-Man player animation`
- `dc7a00b` — `Use original Pac-Man player sprites`
- `914ed43` — `Fix imported Pac-Man maze rows`
- `62e33ae` — `Use original Pac-Man maze tile stream`
- `58cfada` — `Render maze with Pac-Man tilemap`
- `73f5255` — `Fix title screen text attributes`
- `357efb2` — `Hook up Pac-Man title screen`

## Near-term plan

1. Restore the HUD into a Pac-Man layout without conflicting with the game
   screen.
2. Wire pellets to score / level-complete / power pellet flow.
3. Start implementing ghosts: sprites, start positions, basic movement.
4. After that, move on to round flow, death, and frightened mode.
