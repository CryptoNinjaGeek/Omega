#!/usr/bin/env python3
"""Generate `castle_labyrinth.json` — a 4x4 grid of cubic rooms linked by
through-portal doorways. Rooms are 16x16x16 world units so the square
Plane object (size s → 2s x 2s) fits each surface exactly without
overlap. Run from anywhere; writes next to this file."""

import json
import os
from pathlib import Path

# ---------------------------------------------------------------- constants
HALF = 8                       # plane.size — each surface is 16x16
ROOM_HEIGHT = 2 * HALF         # cubic room, ceiling at y=16
ROOM_SPACING = 5 * 2 * HALF    # 80 units — plenty of air between shells
CAM_Y = 2.0

# Door (portal) geometry. We sit the portal 0.1 unit inside the wall face
# so the portal quad never z-fights with the wall.
DOOR_W, DOOR_H = 3.0, 5.0
DOOR_OFFSET = HALF - 0.1       # 7.9

GRID_COLS, GRID_ROWS = 4, 4

# Which neighbour pairs have a CLOSED door (player opens with O key).
CLOSED_PAIRS = {
    ("r10", "r20"),
    ("r22", "r32"),
    ("r12", "r13"),
}

FLOORS = ["floor_stone", "floor_stone_pattern",
          "floor_tiles_tan_large", "floor_wood_planks"]
WALLS  = ["wall_stone", "wall_brick_stone_center", "wall_rock",
          "wall_rock_structure", "wall_brick_sand_center"]
CEILS  = ["wall_timber", "wall_timber_structure"]

TEXTURE_DECL = {
    # Floor tex
    "floor_stone":            {"file": ":/castle/textures/floor_stone.png"},
    "floor_stone_pattern":    {"file": ":/castle/textures/floor_stone_pattern.png"},
    "floor_tiles_tan_large":  {"file": ":/castle/textures/floor_tiles_tan_large.png"},
    "floor_wood_planks":      {"file": ":/castle/textures/floor_wood_planks.png"},
    # Wall tex
    "wall_stone":             {"file": ":/castle/textures/wall_stone.png"},
    "wall_brick_stone_center":{"file": ":/castle/textures/wall_brick_stone_center.png"},
    "wall_rock":              {"file": ":/castle/textures/wall_rock.png"},
    "wall_rock_structure":    {"file": ":/castle/textures/wall_rock_structure.png"},
    "wall_brick_sand_center": {"file": ":/castle/textures/wall_brick_sand_center.png"},
    # Ceiling tex
    "wall_timber":            {"file": ":/castle/textures/wall_timber.png"},
    "wall_timber_structure":  {"file": ":/castle/textures/wall_timber_structure.png"},
    # Box props
    "crate":                  {"file": ":/castle/textures/wall_timber.png"},
}

# --------------------------------------------------------------------- helpers
def rid(c, r):        return f"r{c}{r}"
def centre(c, r):     return [c * ROOM_SPACING, 0.0, r * ROOM_SPACING]

def plane(name, position, rotation, size, texture, physics=False):
    obj = {
        "type": "plane",
        "name": name,
        "position": position,
        "size": size,
        "textures": [texture],
        "material": "default",
        "physics": {"enabled": physics,
                    "bodyType": "STATIC", "colliderType": "PLANE"}
                    if physics else {"enabled": False},
    }
    if rotation is not None:
        obj["rotation"] = rotation
    return obj


def room_surfaces(room_id, col, row):
    cx, _, cz = centre(col, row)
    floor_tex = FLOORS[(col + row) % len(FLOORS)]
    wall_tex  = WALLS[(col + row) % len(WALLS)]
    ceil_tex  = CEILS[(col + row) % len(CEILS)]

    objs = []
    # Floor @ y=0, default +Y normal
    objs.append(plane(f"{room_id}_floor", [cx, 0.0, cz],
                      None, HALF, floor_tex, physics=True))
    # Ceiling @ y=ROOM_HEIGHT, flipped to -Y via Rx(180)
    objs.append(plane(f"{room_id}_ceiling",
                      [cx, ROOM_HEIGHT, cz],
                      [180.0, 0.0, 0.0], HALF, ceil_tex))
    # Walls. All four surfaces are 16x16 squares fitting exactly.
    #   N wall — +Z normal, at z=-HALF  → Rx(+90)
    objs.append(plane(f"{room_id}_wall_N",
                      [cx, HALF, cz - HALF],
                      [90.0, 0.0, 0.0], HALF, wall_tex))
    #   S wall — -Z normal, at z=+HALF  → Rx(-90)
    objs.append(plane(f"{room_id}_wall_S",
                      [cx, HALF, cz + HALF],
                      [-90.0, 0.0, 0.0], HALF, wall_tex))
    #   E wall — -X normal, at x=+HALF  → Rz(+90)
    objs.append(plane(f"{room_id}_wall_E",
                      [cx + HALF, HALF, cz],
                      [0.0, 0.0, 90.0], HALF, wall_tex))
    #   W wall — +X normal, at x=-HALF  → Rz(-90)
    objs.append(plane(f"{room_id}_wall_W",
                      [cx - HALF, HALF, cz],
                      [0.0, 0.0, -90.0], HALF, wall_tex))
    return objs


def box(name, position, size, texture, scale=None, mass=1.0):
    obj = {
        "type": "box",
        "name": name,
        "position": position,
        "size": size,
        "textures": [texture],
        "material": "default",
        "physics": {"enabled": False},
    }
    if scale is not None:
        obj["scale"] = scale
    return obj


def portal(pid, name, position, normal, dest, closed=False, mirror=False):
    p = {
        "id": pid,
        "name": name,
        "type": "doorway",
        "position": position,
        "normal": normal,
        "width":  DOOR_W,
        "height": DOOR_H,
        "destination": dest,
        "passable": True,
        "open": not closed,
        "mirrorOverlay": mirror,
        "enabled": True,
        "visible": True,
        "framebuffer": {"width": 512, "height": 512},
    }
    return p


# ---------------------------------------------------------- build everything
def build():
    objects = []
    portals = []

    # Rooms
    for c in range(GRID_COLS):
        for r in range(GRID_ROWS):
            objects.extend(room_surfaces(rid(c, r), c, r))

    # A few prop crates scattered across the labyrinth
    crate_spots = [
        ("r00", -3.0, 1.0, -3.0, 1.0),
        ("r11",  2.5, 0.75,  1.5, 0.75),
        ("r11", -2.0, 1.5,  -2.0, 1.5),
        ("r21",  0.0, 1.0,   3.0, 1.0),
        ("r22", -2.5, 0.5,  -3.5, 0.5),
        ("r33",  2.0, 1.25, -2.0, 1.25),
        ("r30", -1.5, 1.0,   1.5, 1.0),
    ]
    for i, (room, dx, dy, dz, size) in enumerate(crate_spots):
        col = int(room[1]); row = int(room[2])
        cx, _, cz = centre(col, row)
        objects.append(box(f"{room}_crate_{i}",
                           [cx + dx, dy, cz + dz], size, "crate"))

    # Portals — connect adjacent rooms on the 4x4 grid. Every cardinal
    # adjacency becomes a through-portal pair (a on one side, b on the
    # other). Doors whose id begins "closed_" boot shut.
    def pair(a_room, b_room, side):
        """side is 'E' (a east → b west) or 'S' (a south → b north)."""
        a_col, a_row = int(a_room[1]), int(a_room[2])
        b_col, b_row = int(b_room[1]), int(b_room[2])
        ca = centre(a_col, a_row)
        cb = centre(b_col, b_row)
        closed = (a_room, b_room) in CLOSED_PAIRS \
              or (b_room, a_room) in CLOSED_PAIRS
        prefix = "closed_door" if closed else "door"
        id_a = f"{prefix}_{a_room}_{b_room}_a"
        id_b = f"{prefix}_{a_room}_{b_room}_b"
        door_cy = DOOR_H / 2.0   # portal centre sits DOOR_H/2 above the floor
        if side == "E":
            a_pos = [ca[0] + DOOR_OFFSET, door_cy, ca[2]]
            a_nor = [-1.0, 0.0, 0.0]
            b_pos = [cb[0] - DOOR_OFFSET, door_cy, cb[2]]
            b_nor = [ 1.0, 0.0, 0.0]
        elif side == "S":
            a_pos = [ca[0], door_cy, ca[2] + DOOR_OFFSET]
            a_nor = [0.0, 0.0, -1.0]
            b_pos = [cb[0], door_cy, cb[2] - DOOR_OFFSET]
            b_nor = [0.0, 0.0,  1.0]
        else:
            raise ValueError(side)
        portals.append(portal(id_a, f"{a_room}->{b_room} (side {side})",
                              a_pos, a_nor, id_b, closed=closed))
        portals.append(portal(id_b, f"{b_room}->{a_room}", b_pos, b_nor,
                              id_a, closed=closed))

    for c in range(GRID_COLS):
        for r in range(GRID_ROWS):
            if c + 1 < GRID_COLS:
                pair(rid(c, r), rid(c + 1, r), "E")
            if r + 1 < GRID_ROWS:
                pair(rid(c, r), rid(c, r + 1), "S")

    # A flavour mirror on r11's west wall — self-linked doorway with
    # mirrorOverlay=true renders the reflection via the mirror path.
    c, r = 1, 1
    cx, _, cz = centre(c, r)
    portals.append(portal(
        "mirror_r11", "r11 west mirror",
        [cx - DOOR_OFFSET, DOOR_H / 2.0 + 1.0, cz + 2.0],
        [1.0, 0.0, 0.0],
        "mirror_r11", closed=False, mirror=True))

    # Textures list
    textures = {
        name: {"file": meta["file"], "type": "diffuse"}
        for name, meta in TEXTURE_DECL.items()
    }

    scene = {
        "version": "1.0",
        "scene": {
            "name": "Castle Labyrinth POC",
            "_doc": (
                f"16 cubic rooms ({2*HALF}x{2*HALF}x{2*HALF}) arranged on a "
                f"{GRID_COLS}x{GRID_ROWS} grid with {ROOM_SPACING}-unit "
                "separation. Rooms are physically disconnected — the "
                "portal math is the only thing that moves you between "
                "them. Doorway ids beginning 'closed_' start shut; cycle "
                "with O."
            ),
            "camera": {
                "position": [0.0, CAM_Y, 0.0],
                "yaw": -90.0,
                "pitch": 0.0,
            },
            "ambient": [0.45, 0.45, 0.45, 1.0],
            "textures": textures,
            "materials": {
                "default": {"shininess": 32.0},
                "metal":   {"shininess": 128.0},
            },
            "objects": objects,
            "portals": portals,
        }
    }
    return scene


if __name__ == "__main__":
    here = Path(__file__).resolve().parent
    scene = build()
    out = here / "castle_labyrinth.json"
    out.write_text(json.dumps(scene, indent=2))
    obj_count = len(scene["scene"]["objects"])
    portal_count = len(scene["scene"]["portals"])
    print(f"Wrote {out}: {obj_count} objects, {portal_count} portals")
