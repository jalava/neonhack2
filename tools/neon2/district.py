#!/usr/bin/env python3
"""Build NeonHack 2 district level files (dat/nc-*.lua) from a layout.

A district is a 77x19 map of streets with buildings drawn as rectangles.  Each layout in tools/neon2/districts/ calls the
methods below; this tool turns that into des.map() plus the des.* calls
for doors, shops, NPCs, stairs and signs, with coordinates that always
match the map.  Edit the layouts, not the generated .lua files:

    tools/neon2/district.py            # write every dat/nc-<name>.lua
    tools/neon2/district.py --show X   # print district X's map
"""

import importlib.util
import os
import sys

W, H = 77, 19
TOP = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LAYOUTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "districts")


def lua_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


class District:
    def __init__(self, proto, title, blurb=""):
        self.proto, self.title, self.blurb = proto, title, blurb
        # streets run to the edge of the map: the city goes on past it
        self.grid = [["."] * W for _ in range(H)]
        self.calls = []        # des.* lines after the map
        self.flags = ["mazelevel", "noflip"]
        self.used = {}         # (x, y) -> what, to catch overlaps
        self.has_arrive = False
        self.doors = []        # des.door() calls, emitted after the rooms

    # --- drawing -------------------------------------------------------
    def _claim(self, x, y, what):
        if not (0 <= x < W and 0 <= y < H):
            raise SystemExit(f"{self.proto}: {what} at ({x},{y}) is off the map")
        if (x, y) in self.used:
            raise SystemExit(f"{self.proto}: {what} at ({x},{y}) clashes with "
                             f"{self.used[(x, y)]}")
        self.used[(x, y)] = what

    def put(self, x, y, ch):
        self.grid[y][x] = ch

    def fill(self, x1, y1, x2, y2, ch):
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                self.grid[y][x] = ch

    def building(self, x, y, w, h, doors=(), door_state="closed"):
        """Walls around (x,y)-(x+w-1,y+h-1); doors are absolute (x, y)."""
        for i in range(w):
            self.grid[y][x + i] = self.grid[y + h - 1][x + i] = "-"
        for j in range(h):
            self.grid[y + j][x] = self.grid[y + j][x + w - 1] = "|"
        self.fill(x + 1, y + 1, x + w - 2, y + h - 2, ".")
        for d in doors:
            dx, dy, *st = d
            self.door(dx, dy, st[0] if st else door_state)
        return (x + 1, y + 1, x + w - 2, y + h - 2)  # interior

    def wall(self, x1, y1, x2, y2):
        """A straight inner wall."""
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                self.grid[y][x] = "|" if x1 == x2 else "-"

    def door(self, x, y, state="closed"):
        # as in the Minetown levels: '+' in the map, and des.door() only
        # after the rooms exist, so the door is the room's edge and
        # shopkeepers stay inside their shops
        if self.grid[y][x] not in "-|":
            raise SystemExit(f"{self.proto}: door at ({x},{y}) isn't in a wall")
        self.grid[y][x] = "+"
        self.doors.append(f'des.door({lua_str(state)}, {x}, {y})')

    # --- contents -------------------------------------------------------
    def shop(self, area, kind):
        x1, y1, x2, y2 = area
        self.calls.append(f'des.region({{ region = {{{x1},{y1},{x2},{y2}}}, '
                          f'lit = 1, type = {lua_str(kind)}, filled = 1 }})')

    def room(self, area, kind="ordinary", filled=0):
        x1, y1, x2, y2 = area
        self.calls.append(f'des.region({{ region = {{{x1},{y1},{x2},{y2}}}, '
                          f'lit = 1, type = {lua_str(kind)}, filled = {filled} }})')

    def npc(self, x, y, npc_id):
        self._claim(x, y, f"npc {npc_id}")
        self.calls.append(f'des.monster({{ npc = {lua_str(npc_id)}, x = {x}, y = {y} }})')

    def mon(self, x, y, name, peaceful=None, extra=""):
        self._claim(x, y, f"monster {name}")
        p = "" if peaceful is None else f", peaceful = {1 if peaceful else 0}"
        self.calls.append(f'des.monster({{ id = {lua_str(name)}, x = {x}, y = {y}{p}{extra} }})')

    def obj(self, x, y, name):
        self.calls.append(f'des.object({{ id = {lua_str(name)}, x = {x}, y = {y} }})')

    def sign(self, x, y, text):
        self._claim(x, y, f"sign {text!r}")
        self.calls.append(f'des.engraving({{ x = {x}, y = {y}, type = "burn", '
                          f'text = {lua_str(text)} }})')

    def metro(self, x, y):
        self._claim(x, y, "metro")
        self.calls.append(f'des.stair({{ kind = "metro", dir = "down", x = {x}, y = {y} }})')

    def stair(self, x, y, up, dest=None):
        self._claim(x, y, f"stairs to {dest}")
        d = f", dest = {lua_str(dest)}" if dest else ""
        self.calls.append(f'des.stair({{ dir = {lua_str("up" if up else "down")}, '
                          f'x = {x}, y = {y}{d} }})')

    def arrive(self, x1, y1, x2, y2):
        """Where you land when no stairs or station decide it."""
        self.has_arrive = True
        for d in ("down", "up"):
            self.calls.append(f'des.teleport_region({{ region = {{{x1},{y1},{x2},{y2}}}, '
                              f'dir = {lua_str(d)} }})')

    def lua(self, line):
        self.calls.append(line)

    # --- output ---------------------------------------------------------
    def mapstr(self):
        return "\n".join("".join(r) for r in self.grid)

    def emit(self):
        if not self.has_arrive:
            # by default, somewhere on the main street
            self.arrive(0, 8, W - 1, 10)
        out = [f"-- NeonHack 2: {self.title}",
               f"-- generated by tools/neon2/district.py from districts/{self.proto[3:]}.py;",
               "-- edit that file, not this one"]
        if self.blurb:
            out += ["--"] + ["-- " + l for l in self.blurb.strip().splitlines()]
        out += ["",
                'des.level_init({ style = "solidfill", fg = " " })',
                "des.level_flags(" + ", ".join(lua_str(f) for f in self.flags) + ")",
                "",
                "des.map([[",
                self.mapstr(),
                "]])",
                "",
                f'des.region(selection.area(00,00,{W - 1},{H - 1}), "lit")']
        # rooms and regions first, then doors, then everything else
        rooms = [c for c in self.calls if c.startswith("des.region(")]
        rest = [c for c in self.calls if not c.startswith("des.region(")]
        out += rooms + self.doors + rest
        return "\n".join(out) + "\n"


def load(name):
    path = os.path.join(LAYOUTS, name + ".py")
    spec = importlib.util.spec_from_file_location("layout_" + name, path)
    mod = importlib.util.module_from_spec(spec)
    mod.District = District
    spec.loader.exec_module(mod)
    return mod.build()


def main(argv):
    names = sorted(f[:-3] for f in os.listdir(LAYOUTS) if f.endswith(".py"))
    if len(argv) > 2 and argv[1] == "--show":
        print(load(argv[2]).mapstr())
        return
    for n in names:
        d = load(n)
        path = os.path.join(TOP, "dat", d.proto + ".lua")
        with open(path, "w") as f:
            f.write(d.emit())
        print("wrote", os.path.relpath(path, TOP))


if __name__ == "__main__":
    main(sys.argv)
