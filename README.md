# btga-gunbuddy-mod

**Gun Buddy Select**: pick which gun buddy you put down, in the
[BattleTanx: Global Assault recomp](https://github.com/wootbeer/btgarecomp).

Made from the [mod template](https://github.com/wootbeer/btgarecomp-mod-template).

## What it does
The game has more gun buddies than the one a normal deploy gives you. A+B
(the powered-up deploy) lays out a grid of them and picks each one at random
from four other types. This mod lets you choose, in the Mods menu:

- **Gun Buddy**: what a normal deploy puts down.
  - **Game's Own** (default): the usual one.
  - **Type 0, Type 1, Type 2, Type 4**: the four types A+B mixes. Type 1
    comes up out of the ground right away; the others drop in, like the
    usual one.
  - **Random**: a different one of the four each time, like A+B.
- **A+B Deploy**: **Mixed** (default, the game's random mix) or **Same as
  Gun Buddy** (every one in the grid is your pick).
- **Applies To**: **Players** (default: gangs with a human player in them,
  so AI tanks in your gang too) or **Everyone** (AI gangs as well).

The types are named by number until each one is matched to its model in
game (see NOTES.md).

## How to install
1. Get `btga_gunbuddy_mod.nrm` from Releases, or from the Build action's
   Artifacts.
2. Open the BattleTanx: Global Assault recomp and go to the Mods tab.
3. Drop the `.nrm` file in, turn the mod on and set its options.

Needs the BattleTanx: Global Assault recomp 0.1.8 or newer.

## Building
Needs clang 18, `make` and `RecompModTool` (see the
[mod template](https://github.com/wootbeer/btgarecomp-mod-template#what-you-need)).
Run `make nrm`; the mod is `build/btga_gunbuddy_mod.nrm`. Pushing to GitHub
builds it too.

## How it works
It replaces the game's gun buddy spawner, `func_800E4F3C`, with the same
code plus a type swap. Both a normal deploy and the A+B grid go through it,
with the type as its first argument. See NOTES.md for what was found in the
game's code.

`func_800E4F3C` isn't one of the functions btgarecomp changes, but only one
mod can patch it at a time.
