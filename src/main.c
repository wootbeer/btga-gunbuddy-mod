// Gun Buddy Select: pick which gun buddy you put down, for the
// BattleTanx: Global Assault recomp.
//
// The game has six gun buddy types (see btga.h). A normal deploy always puts
// down type 5. A+B (the powered-up deploy) lays out a grid and fills every
// third spot with a random one of types 0, 1, 2 and 4. Both go through
// func_800E4F3C, which takes the type as its first argument, so this mod
// replaces that function with the same code plus a type swap.
//
// func_800E4F3C's only callers (checked in the ROM):
//   0x8008E244, func_8008C5D8 (weapon fire), Gun Buddies: type 5
//   0x800F2B10, func_800F2980 (the A+B grid): types 0, 1, 2, 4
// So type 5 is a normal deploy and anything else is the A+B grid.

#include "modding.h"
#include "recompconfig.h"
#include "btga.h"

// "deploy_type" option, in mod.toml's order.
enum {
    DEPLOY_GAME,
    DEPLOY_TYPE_0,
    DEPLOY_TYPE_1,
    DEPLOY_TYPE_2,
    DEPLOY_TYPE_4,
    DEPLOY_RANDOM,
};

// "ab_deploy" option.
enum {
    AB_GAME,
    AB_SAME_AS_DEPLOY,
};

// "applies_to" option.
enum {
    APPLIES_PLAYERS,
    APPLIES_EVERYONE,
};

// What A+B picks from: the game's own mix (rand & 3, then 3 becomes 4).
static s32 random_type(void) {
    static const u8 mix[4] = { 0, 1, 2, 4 };
    return mix[func_8009D914() & 3];
}

// Is there a human-driven tank in this gang? Gun buddies only know their
// gang, not the tank that put them down.
static s32 gang_has_player(u32 gang) {
    s32 i;
    if (gang == 0x7F) {
        return 0;
    }
    for (i = 0; i < BTGA_TANK_SLOTS; i++) {
        u8* tank = BTGA_TANKS + BTGA_TANK_SIZE * i;
        if ((TANK_FLAGS(tank) & 1) && TANK_CONTROL(tank) == 0 && TANK_GANG(tank) == gang) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// The cycle button: each player can step through the gun buddies in game.

// "cycle_button" option, in mod.toml's order, and the N64 button for each.
static const u16 cycle_buttons[] = {
    0,                                    // Off
    BTN_L, BTN_R, BTN_Z,
    BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT,
    BTN_CUP, BTN_CDOWN, BTN_CLEFT, BTN_CRIGHT,
};
#define CYCLE_BUTTON_COUNT (sizeof(cycle_buttons) / sizeof(cycle_buttons[0]))

// What the message says for each "deploy_type" choice.
static const char* const deploy_names[] = {
    "GUN BUDDIES NORMAL",
    "GUN BUDDIES TYPE 0",
    "GUN BUDDIES TYPE 1",
    "GUN BUDDIES TYPE 2",
    "GUN BUDDIES TYPE 4",
    "GUN BUDDIES RANDOM",
};
#define DEPLOY_CHOICES (sizeof(deploy_names) / sizeof(deploy_names[0]))

// Each human gang's pick (a "deploy_type" index), and the option it started
// from: when the option is changed in the Mods menu, everyone goes back to it.
static u8 player_pick[BTGA_HUMAN_GANGS];
static u32 picks_from = 0xFFFFFFFF;
// Each controller port's buttons last frame, to act once per press.
static u16 last_buttons[BTGA_CONTROLLER_PORTS];

static void sync_picks(void) {
    u32 deploy = recomp_get_config_u32("deploy_type");
    u32 i;
    if (deploy != picks_from) {
        for (i = 0; i < BTGA_HUMAN_GANGS; i++) {
            player_pick[i] = (u8)deploy;
        }
        picks_from = deploy;
    }
}

// The "deploy_type" choice for a gun buddy of `gang`.
static u32 deploy_choice(u32 gang) {
    sync_picks();
    if (recomp_get_config_u32("cycle_button") != 0 && gang < BTGA_HUMAN_GANGS && gang_has_player(gang)) {
        return player_pick[gang];
    }
    return picks_from;
}

// Once a frame: a press of the cycle button moves that player to the next
// gun buddy and says which one it is.
RECOMP_HOOK("func_8007A0A0")
void gunbuddy_cycle_on_frame(void) {
    u32 option = recomp_get_config_u32("cycle_button");
    u16 button = option < CYCLE_BUTTON_COUNT ? cycle_buttons[option] : 0;
    u32 gang;

    sync_picks();
    for (gang = 0; gang < BTGA_HUMAN_GANGS; gang++) {
        u32 port = BTGA_GANG_PORT(gang);
        u16 now, pressed;
        if (port >= BTGA_CONTROLLER_PORTS) {
            continue;
        }
        now = BTGA_PAD_BUTTONS(port);
        pressed = now & (u16)~last_buttons[port];
        last_buttons[port] = now;

        if (button == 0 || !(pressed & button) || !gang_has_player(gang)) {
            continue;
        }
        player_pick[gang] = (u8)((player_pick[gang] + 1) % DEPLOY_CHOICES);
        if (BTGA_GAME_MODE != 10) { // the game shows no pickup messages in mode 10
            func_800CA620(gang, deploy_names[player_pick[gang]], 45);
        }
    }
}

// The type this mod puts down instead of `type`, for a gun buddy of `gang`.
static s32 chosen_type(s32 type, u32 gang) {
    u32 deploy = deploy_choice(gang);
    s32 from_grid = type != GUNBUDDY_TYPE_DEPLOY;

    if (deploy == DEPLOY_GAME) {
        return type;
    }
    if (from_grid && recomp_get_config_u32("ab_deploy") == AB_GAME) {
        return type;
    }
    if (recomp_get_config_u32("applies_to") == APPLIES_PLAYERS && !gang_has_player(gang)) {
        return type;
    }

    switch (deploy) {
        case DEPLOY_TYPE_0: return 0;
        case DEPLOY_TYPE_1: return 1;
        case DEPLOY_TYPE_2: return 2;
        case DEPLOY_TYPE_4: return 4;
        case DEPLOY_RANDOM: return random_type();
        default:            return type;
    }
}

// A spot for a gun buddy of `type` where the game picked one for `game_type`.
// func_800E29A4 checks the type's own box (stats +0x2C), so a bigger type
// than the game's can overlap the tank that put it down (type 4's box is
// 100 across to type 5's 36). A normal deploy's spot is the tank's position
// plus its heading's direction times a distance, and the gun buddy faces the
// other way (heading + 0x8000). So heading - 0x8000 points from the tank to
// the spot, and a bigger type is tried further out along it and at angles
// to either side. Grid spots are only tried where they are. Returns pos,
// `out` with the moved spot, or NULL if there's no room.
static f32* find_room(s32 type, s32 game_type, f32* pos, f32* out, s32 heading, s32 layer) {
    // Directions to try, as turns from straight out (behind the tank), and
    // how far: rings of (grow + 16) each, out to 8 rings.
    static const s16 turns[] = { 0, 0x1800, -0x1800, 0x3000, -0x3000 };
    s32 grow, ring, t;

    if (func_800E29A4(type, pos, heading, layer)) {
        return pos;
    }
    if (game_type != GUNBUDDY_TYPE_DEPLOY) {
        return NULL;
    }

    grow = GUNBUDDY_STAT_HALF_SIZE(type) - GUNBUDDY_STAT_HALF_SIZE(game_type);
    if (grow < 0) {
        grow = 0;
    }
    for (ring = 1; ring <= 8; ring++) {
        f32 d = (f32)((grow + 16) * ring);
        for (t = 0; t < (s32)(sizeof(turns) / sizeof(turns[0])); t++) {
            s32 angle = (heading - 0x8000 + turns[t]) & 0xFFFF;
            out[0] = pos[0] + func_8009D4B0(angle) * d;
            out[1] = pos[1] + func_8009D510(angle) * d;
            out[2] = pos[2];
            if (func_800E29A4(type, out, heading, layer)) {
                return out;
            }
        }
    }
    return NULL;
}

// Puts down a gun buddy of `type` at pos (x, z, y), facing `heading`.
// Type 1 is made right away; the others first drop in from above, as a
// type 0x29 object that func_800E5164 turns into the gun buddy about a
// second later. Returns nonzero if it was put down.
//
// The same as the game's function (0x800E4F3C), apart from chosen_type().
RECOMP_PATCH s32 func_800E4F3C(s32 type, f32* pos, s32 heading, s32 layer, s32 gang) {
    u8* drop;
    s32 half;
    f32 top;
    f32* game_pos = pos;
    f32 moved[3];

    type &= 0xFF;
    heading &= 0xFFFF;
    layer &= 0xFF;
    gang &= 0xFF;

    {
        s32 game_type = type;
        type = chosen_type(type, gang);
        if (type != game_type) {
            pos = find_room(type, game_type, pos, moved, heading, layer);
            if (pos == NULL) {
                // No room for the pick: the game's own, and say so.
                if (gang < BTGA_HUMAN_GANGS && gang_has_player(gang) && BTGA_GAME_MODE != 10) {
                    func_800CA620(gang, "NO ROOM FOR THAT GUN BUDDY", 45);
                }
                type = game_type;
                pos = game_pos;
            }
        }
    }

    if (!func_800E29A4(type, pos, heading, layer)) {
        return 0;
    }

    if (type == 1) {
        return (s32)func_800E2AEC(1, pos, heading, 0, layer, gang, 0);
    }

    drop = func_800A18D0(0x29, 0x24);
    if (drop == NULL) {
        return 0;
    }

    half = GUNBUDDY_STAT_HALF_SIZE(type);
    top = pos[2] + GUNBUDDY_STAT_DROP_HEIGHT(type);
    *(s16*)(drop + 0x0A) = (s16)func_800B1898(drop,
        (s16)(s32)pos[0], (s16)(s32)pos[1], 0,
        (s16)-half, (s16)half, (s16)-half, (s16)half,
        (s16)(s32)(pos[2] - 50.0f),
        (70.0f < top) ? (s16)(s32)top : 70,
        heading, 0x1000, layer);

    *(f32*)(drop + 0x0C) = pos[0];
    *(f32*)(drop + 0x10) = pos[1];
    *(f32*)(drop + 0x14) = pos[2];
    *(u8*)(drop + 0x1A) = (u8)type;
    *(u8*)(drop + 0x1B) = (u8)layer;
    *(u16*)(drop + 0x18) = (u16)heading;
    *(u8*)(drop + 0x20) = (u8)gang;
    *(s32*)(drop + 0x1C) = BTGA_LEVEL_TIME;
    return 1;
}
