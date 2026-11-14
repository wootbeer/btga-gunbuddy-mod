# Notes

What this mod relies on, found by reading the game's code (USA 1.0 ROM).

## How a gun buddy is put down
- Weapon fire is `func_8008C5D8`, a switch on the weapon. Gun Buddies is
  case 16 (0x8008E0D0). Its third argument is the fire mode; mode 3 is the
  powered-up A+B fire.
- **Normal deploy** (0x8008E178 on): works out a spot behind the tank and
  calls `func_800E4F3C(5, pos, heading + 0x8000, tank layer, tank gang)`.
- **A+B** (0x8008E0DC): calls `func_800F28AC`, which makes a grid record.
  `func_800F2980` works through it, and on every third spot calls
  `func_800E4F3C` with a random type: `func_8009D914() & 3`, with 3 turned
  into 4, so 0, 1, 2 or 4.
- Those are `func_800E4F3C`'s only two callers, so in it type 5 means a
  normal deploy and any other type means the A+B grid.
- `func_800E4F3C(type, pos, heading, layer, gang)`: returns 0 if
  `func_800E29A4` finds no room. Type 1 is made at once with
  `func_800E2AEC`. Any other type gets a type 0x29 object (0x24 bytes) with
  a collision box, which `func_800E5164` turns into the gun buddy
  (`func_800E2AEC`) once 0x21 ticks of `BTGA_LEVEL_TIME` have gone by.
- `func_800E2AEC(type, pos, heading, a3, layer, gang, a6)` makes a gun buddy
  object. Type is at +0x20 and the gang at +0x22 (0x7F: none).

## The types
Stats and model ids are in a table at `0x80123BB0`, 0x60 bytes per type, six
types (the table runs into the level names after type 5). Model ids at
+0x18, +0x1C, +0x20, +0x24 (0x107 looks like "none"):

| Type | Models | Used by |
|---|---|---|
| 0 | 0x98, 0x99, 0x9A | A+B mix |
| 1 | 0x71, 0x72, 0x73 | A+B mix; made at once (no drop) |
| 2 | 0xB3, 0xB4 | A+B mix |
| 3 | 0xCA | two put on a tank when it's made, if its tank type has flag 0x80 (0x8008B55C) |
| 4 | 0xCE | A+B mix; most health |
| 5 | 0x98, 0x99, 0x9A | normal deploy (type 0's models, other stats) |

Not yet checked in game: which type is the double-barreled one and which one
hides. Guesses from the code: type 1 hides (it's made at once, so it can't
drop in, and it has extra timing fields at +0x30); type 4 is the big
four-barrel one (one model and two muzzle offsets at +0x3C, which the game
mirrors left and right when it fires, `func_800E2F9C`).

Type 3 is left out: it's only ever mounted on a tank, never put on the
ground.

## Damage
`func_800E4530` takes damage off a gun buddy (+0x3C health). btgarecomp
hooks it for the Gun Buddies Are Invulnerable cheat, so mods can't.

## Room for a bigger type
`func_800E29A4` checks the type's own box (stats +0x2C: half its size) on
the collision map, the deploying tank included. A normal deploy's spot is
worked out for type 5 (half size 0x24), just clear of the tank, so type 4
(0x64) overlaps the tank there and is refused: the weapon has already played
its sound and used its ammo by then, and nothing comes down. The mod moves a
bigger type further from the tank (heading - 0x8000 points from the tank to
the spot) by the difference plus 8, up to three times, and puts down the
game's own type if it still doesn't fit. Sizes: types 0, 2, 5: 0x24; type 1:
0x1E; type 3: 0x05; type 4: 0x64.
