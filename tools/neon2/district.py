#!/usr/bin/env python3
# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
"""Build NeonHack 2 level files (dat/nc-*.lua) from hand-drawn maps.

Each layout in tools/neon2/districts/<name>.py defines PROTO, TITLE, an
optional BLURB, FLAGS and LIT (True: no neon pools, all of it lit), a MAP
drawn in des.map characters (up to 79x21)
and a legend(d) function that says what the marker characters mean:

    terrain    ' ' stone  . floor  - | walls  # corridor  { fountain
               K sink  T tree  F iron bars (fences, counters)  and the
               rest of des.map's characters
    doors      +  closed (shops) or random    /  open
               %  locked                      !  broken (empty doorway)
    markers    digits, lowercase letters except w and x, the capitals that
               aren't terrain (D E G J M N O Q R U V X Y), and @ = < > * ^
               & $ ? ; _   they stand on floor unless legend() says otherwise

    d.room('1', "clinic")      a shop, temple or clinic: the room around
                               the marker (one door; lit)
    d.lit('2')                 light the room around the marker (a bar,
                               a home); not a game room
    d.npc('o', "okoye")        story NPC     d.mon('g', "street punk", False)
    d.obj('k', "medkit")       object        d.altar('_', "neutral")
    d.item('c', "crate")       a story item (an id from dat/story.lua)
    d.sign('a', "TEXT")        a neon sign (burned engraving); lights a pool
    d.metro('=')               metro station; lights a pool
    d.stair('<', up=True, dest="Block 7")
    d.arrive('@')              where you land when nothing else decides
    d.light('*')               extra lit street cells
    d.lamp('^')                a street light: a pool of light, no sign
    d.variant('v', 50, '-')    each game, 50% chance these cells become '-'
                               (their base terrain is '.', or base='|' ...)

The tool checks the map (shops have one usable door, rooms are closed,
every important place is reachable from the metro whatever the variants
do) and lights "neon pools": the street within a few steps of every sign,
station and shop door.  Back alleys stay dark.

    tools/neon2/district.py            write every dat/nc-<name>.lua
    tools/neon2/district.py --check    check every layout and show its map
                                       ('*' lit street, ':' dark street)
    tools/neon2/district.py --show X   the same for one layout
"""

import importlib.util
import itertools
import os
import sys

MAXW, MAXH = 79, 21
TOP = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LAYOUTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "districts")

TERRAIN = set(" #.-|+ACSHK{}\\PLZIWTFB")
WALLS = set("-|B")
DOORS = {"+": None, "/": "open", "%": "locked", "!": "nodoor"}
PASSABLE = set(".#{K\\ACI")      # walkable terrain; doors handled apart
MARKERS = set("0123456789abcdefghijklmnopqrstuvyz@=<>*^&$?;_DEGJMNOQRUVXY")
POOL_RADIUS = 3.0


def lua_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


class LayoutError(Exception):
    pass


class District:
    def __init__(self, proto, title, art, blurb="", flags=()):
        self.proto, self.title, self.blurb = proto, title, blurb
        rows = art.strip("\n").split("\n")
        if len(rows) > MAXH:
            raise LayoutError(f"{proto}: {len(rows)} rows (max {MAXH})")
        self.w = max(len(r) for r in rows)
        if self.w > MAXW:
            raise LayoutError(f"{proto}: {self.w} columns (max {MAXW})")
        self.h = len(rows)
        self.art = [list(r.ljust(self.w)) for r in rows]
        self.flags = ["mazelevel", "noflip"] + list(flags)
        self.spec = {}        # marker -> (kind, args)
        self.variants = []    # (marker, pct, becomes, base)
        self.errors = []
        self.layout_name = proto
        self.alllit = False

    # --- legend -----------------------------------------------------------
    def _mark(self, ch, kind, *args):
        if ch not in MARKERS:
            raise LayoutError(f"{self.proto}: '{ch}' can't be a marker")
        if ch in self.spec:
            raise LayoutError(f"{self.proto}: marker '{ch}' defined twice")
        self.spec[ch] = (kind, args)

    def room(self, ch, kind, filled=1):
        self._mark(ch, "room", kind, filled)

    def lit(self, ch):
        self._mark(ch, "litroom")

    def npc(self, ch, npc_id):
        self._mark(ch, "npc", npc_id)

    def mon(self, ch, name, peaceful=None):
        self._mark(ch, "mon", name, peaceful)

    def obj(self, ch, name):
        self._mark(ch, "obj", name)

    def item(self, ch, story_id):
        self._mark(ch, "item", story_id)

    def altar(self, ch, align):
        self._mark(ch, "altar", align)

    def sign(self, ch, text):
        self._mark(ch, "sign", text)

    def metro(self, ch="="):
        self._mark(ch, "metro")

    def stair(self, ch, up, dest=None):
        self._mark(ch, "stair", up, dest)

    def arrive(self, ch="@"):
        self._mark(ch, "arrive")

    def light(self, ch="*"):
        self._mark(ch, "light")

    def lamp(self, ch="^"):
        self._mark(ch, "lamp")

    def variant(self, ch, pct, becomes, base="."):
        self._mark(ch, "variant")
        self.variants.append((ch, pct, becomes, base))

    # --- analysis -----------------------------------------------------------
    def cells(self, ch):
        return [(x, y) for y in range(self.h) for x in range(self.w)
                if self.art[y][x] == ch]

    def _resolve(self):
        """the terrain without markers, and the doors' states"""
        self.terrain = [row[:] for row in self.art]
        self.doorstate = {}
        vbase = {v[0]: v[3] for v in self.variants}
        for y in range(self.h):
            for x in range(self.w):
                c = self.art[y][x]
                if c in DOORS:
                    self.terrain[y][x] = "+"
                    self.doorstate[(x, y)] = DOORS[c]
                elif c in TERRAIN:
                    pass
                elif c in self.spec:
                    self.terrain[y][x] = vbase.get(c, ".")
                else:
                    self.errors.append(f"({x},{y}): unknown character '{c}'")
                    self.terrain[y][x] = "."

    def _nbrs8(self, x, y):
        for dx, dy in itertools.product((-1, 0, 1), repeat=2):
            if (dx or dy) and 0 <= x + dx < self.w and 0 <= y + dy < self.h:
                yield x + dx, y + dy

    def _flood_room(self, x0, y0):
        """the room around (x0,y0): 8-way through anything but walls,
           doors and stone, as the engine floods irregular rooms"""
        seen, todo = {(x0, y0)}, [(x0, y0)]
        while todo:
            x, y = todo.pop()
            for nx, ny in self._nbrs8(x, y):
                t = self.terrain[ny][nx]
                if (nx, ny) in seen or t in WALLS or t == "+" or t == " ":
                    continue
                if nx in (0, self.w - 1) or ny in (0, self.h - 1):
                    raise LayoutError(f"room at ({x0},{y0}) leaks out to "
                                      f"the edge of the map at ({nx},{ny})")
                seen.add((nx, ny))
                todo.append((nx, ny))
                if len(seen) > 400:
                    raise LayoutError(f"room at ({x0},{y0}) isn't closed")
        return seen

    def _rooms(self):
        self.rooms, self.roomcells, self.roomring = [], set(), set()
        for ch, (kind, args) in self.spec.items():
            if kind not in ("room", "litroom"):
                continue
            for (x, y) in self.cells(ch):
                cells = self._flood_room(x, y)
                xs = [c[0] for c in cells]
                ys = [c[1] for c in cells]
                rect = (min(xs), min(ys), max(xs), max(ys))
                isrect = len(cells) == ((rect[2] - rect[0] + 1)
                                        * (rect[3] - rect[1] + 1))
                doors = sorted({(nx, ny) for (cx, cy) in cells
                                for (nx, ny) in ((cx + 1, cy), (cx - 1, cy),
                                                 (cx, cy + 1), (cx, cy - 1))
                                if self.terrain[ny][nx] == "+"})
                ring = {c for (cx, cy) in cells
                        for c in self._nbrs8(cx, cy)} - cells
                r = dict(marker=ch, kind=kind, args=args, cells=cells,
                         rect=rect, isrect=isrect, doors=doors, seed=(x, y))
                self.rooms.append(r)
                self.roomcells |= cells
                self.roomring |= ring
                if kind == "room":
                    self._check_shop(r)

    def _check_shop(self, r):
        where = f"{r['args'][0]} at {r['seed']}"
        if len(r["doors"]) != 1:
            self.errors.append(f"{where}: {len(r['doors'])} doors (needs 1)")
            return
        dx, dy = r["doors"][0]
        if self.doorstate.get((dx, dy)) in ("locked", "nodoor"):
            self.errors.append(f"{where}: its door can't be locked or missing")
        inside = [c for c in ((dx + 1, dy), (dx - 1, dy), (dx, dy + 1),
                              (dx, dy - 1)) if c in r["cells"]]
        if len(inside) != 1:
            self.errors.append(f"{where}: door at ({dx},{dy}) is at a corner")
            return
        sx, sy = inside[0]
        if sum(1 for c in self._nbrs8(sx, sy) if c in r["cells"]) < 2:
            self.errors.append(f"{where}: the shopkeeper can't step aside "
                               f"from ({sx},{sy})")

    def _reach(self, terrain, start):
        seen, todo = {start}, [start]
        while todo:
            x, y = todo.pop()
            for nx, ny in self._nbrs8(x, y):
                t = terrain[ny][nx]
                if (nx, ny) in seen or not (t in PASSABLE or t == "+"):
                    continue
                if (nx != x and ny != y
                        and (terrain[y][x] == "+" or t == "+")):
                    continue  # no diagonal moves through doorways
                seen.add((nx, ny))
                todo.append((nx, ny))
        return seen

    def _check_reach(self):
        starts = [c for ch, (k, _) in self.spec.items() if k == "metro"
                  for c in self.cells(ch)]
        starts += [c for ch, (k, _) in self.spec.items() if k == "stair"
                   for c in self.cells(ch)]
        if not starts:
            self.errors.append("no metro station or stairs to start from")
            return
        start = starts[0]
        keys = {}
        for ch, (kind, args) in self.spec.items():
            if kind in ("npc", "stair", "metro"):
                for c in self.cells(ch):
                    keys[c] = f"{kind} '{ch}'"
        for r in self.rooms:
            for d in r["doors"]:
                keys[d] = "door of a room"
        arrive = [c for ch, (k, _) in self.spec.items() if k == "arrive"
                  for c in self.cells(ch)]
        vcells = {}
        for (vch, pct, becomes, base) in self.variants:
            vcells[vch] = self.cells(vch)
            for c in vcells[vch]:
                if c in self.roomcells or c in self.roomring or c in keys:
                    self.errors.append(f"variant '{vch}' touches a room, door "
                                       f"or key place at {c}")
        if len(self.variants) > 12:
            self.errors.append("more than 12 variants")
            return
        for combo in itertools.product((False, True), repeat=len(self.variants)):
            terrain = [row[:] for row in self.terrain]
            for on, (vch, pct, becomes, base) in zip(combo, self.variants):
                if on:
                    for (x, y) in vcells[vch]:
                        terrain[y][x] = becomes
            reach = self._reach(terrain, start)
            active = [v[0] for on, v in zip(combo, self.variants) if on]
            how = f" with variants {active}" if active else ""
            for c, what in keys.items():
                if c not in reach:
                    self.errors.append(f"{what} at {c} can't be reached from "
                                       f"{start}{how}")
                    return
            if arrive and not any(c in reach for c in arrive):
                self.errors.append(f"the arrival area can't be reached{how}")
                return

    def _pools(self):
        """lit street: within walking distance POOL_RADIUS of a sign, the
           station or a shop door, never inside rooms"""
        centers = []
        for ch, (kind, _) in self.spec.items():
            if kind in ("sign", "metro", "lamp"):
                centers += self.cells(ch)
        for r in self.rooms:
            if r["kind"] != "room":
                continue
            for (dx, dy) in r["doors"]:
                for c in ((dx + 1, dy), (dx - 1, dy), (dx, dy + 1), (dx, dy - 1)):
                    x, y = c
                    if (0 <= x < self.w and 0 <= y < self.h
                            and c not in r["cells"]
                            and self.terrain[y][x] in PASSABLE):
                        centers.append(c)
        lit = set()
        for (cx, cy) in centers:
            seen, todo = {(cx, cy)}, [(cx, cy)]
            while todo:
                x, y = todo.pop()
                lit.add((x, y))
                for nx, ny in self._nbrs8(x, y):
                    if ((nx, ny) in seen or (nx, ny) in self.roomcells
                            or self.terrain[ny][nx] not in PASSABLE):
                        continue
                    # tall terminal cells: pools reach further sideways
                    if ((nx - cx) / 1.7) ** 2 + (ny - cy) ** 2 > POOL_RADIUS ** 2:
                        continue
                    seen.add((nx, ny))
                    todo.append((nx, ny))
        for ch, (kind, _) in self.spec.items():
            if kind == "light":
                lit |= set(self.cells(ch))
        self.lit_cells = lit

    def analyze(self):
        self._resolve()
        try:
            self._rooms()
        except LayoutError as e:
            self.errors.append(str(e))
            self.rooms, self.roomcells, self.roomring = [], set(), set()
        self._check_reach()
        self._pools()
        for ch, (kind, _) in self.spec.items():
            if not self.cells(ch):
                self.errors.append(f"marker '{ch}' ({kind}) isn't on the map")
        kinds = {k for (k, _) in self.spec.values()}
        if "metro" in kinds and "arrive" not in kinds:
            # districts need one, or a level teleport or a lost arrival
            # can put you inside a shop or a sealed block
            self.errors.append("a district needs an arrival area (d.arrive)")
        if sum(1 for r in self.rooms if r["kind"] == "room") > 30:
            self.errors.append("too many rooms")
        return not self.errors

    # --- output -------------------------------------------------------------
    def preview(self):
        out = []
        for y in range(self.h):
            row = []
            for x in range(self.w):
                c = self.art[y][x]
                if c == "." and (x, y) not in self.roomcells:
                    c = "*" if (x, y) in self.lit_cells or self.alllit else ":"
                row.append(c)
            out.append("".join(row))
        return "\n".join(out)

    def _runs(self, cells):
        runs = []
        for y in range(self.h):
            xs = sorted(x for (x, yy) in cells if yy == y)
            i = 0
            while i < len(xs):
                j = i
                while j + 1 < len(xs) and xs[j + 1] == xs[j] + 1:
                    j += 1
                runs.append((xs[i], y, xs[j]))
                i = j + 1
        return runs

    def emit(self):
        out = [f"-- NeonHack 2: {self.title}",
               f"-- generated by tools/neon2/district.py from "
               f"districts/{self.layout_name}.py; edit that file, not this one",
               "-- Copyright (c) 2026 jalava",
               "-- NetHack may be freely redistributed.  See license for details."]
        if self.blurb:
            out += ["--"] + ["-- " + l for l in self.blurb.strip().splitlines()]
        mapstr = "\n".join("".join(r) for r in self.terrain)
        out += ["",
                'des.level_init({ style = "solidfill", fg = " " })',
                "des.level_flags(" + ", ".join(lua_str(f) for f in self.flags) + ")",
                "",
                "des.map([[", mapstr, "]])", ""]
        # random changes first, as Grotto Town does
        for (vch, pct, becomes, base) in self.variants:
            out.append(f"if percent({pct}) then")
            for (x, y) in self.cells(vch):
                out.append(f"   des.terrain({x}, {y}, {lua_str(becomes)})")
            out.append("end")
        # lighting: dark streets with neon pools, or (LIT = True) all lit
        if self.alllit:
            out.append(f'des.region(selection.area(0,0,{self.w - 1},{self.h - 1}), "lit")')
        else:
            out.append(f'des.region(selection.area(0,0,{self.w - 1},{self.h - 1}), "unlit")')
        for (x1, y, x2) in self._runs(self.lit_cells):
            if self.alllit:
                break
            out.append(f'des.region(selection.area({x1},{y},{x2},{y}), "lit")')
        for r in self.rooms:
            if r["kind"] == "litroom":
                for (x1, y, x2) in self._runs(r["cells"]):
                    out.append(f'des.region(selection.area({x1},{y},{x2},{y}), "lit")')
        # game rooms (shops, temples, clinics)
        for r in self.rooms:
            if r["kind"] != "room":
                continue
            rtype, filled = r["args"]
            x1, y1, x2, y2 = r["rect"]
            if r["isrect"]:
                out.append(f"des.region({{ region = {{{x1},{y1},{x2},{y2}}}, "
                           f"lit = 1, type = {lua_str(rtype)}, filled = {filled} }})")
            else:
                sx, sy = r["seed"]
                out.append(f"des.region({{ region = {{{sx},{sy},{sx},{sy}}}, "
                           f"lit = 1, irregular = 1, type = {lua_str(rtype)}, "
                           f"filled = {filled} }})")
        # doors only after the rooms
        shopdoors = {d for r in self.rooms if r["kind"] == "room" for d in r["doors"]}
        for (x, y) in sorted(self.doorstate, key=lambda c: (c[1], c[0])):
            st = self.doorstate[(x, y)]
            if st is None:
                st = "closed" if (x, y) in shopdoors else "random"
            out.append(f"des.door({lua_str(st)}, {x}, {y})")
        # stairs, stations, altars
        for ch, (kind, args) in sorted(self.spec.items()):
            for (x, y) in self.cells(ch):
                if kind == "metro":
                    out.append(f'des.stair({{ kind = "metro", dir = "down", '
                               f'x = {x}, y = {y} }})')
                elif kind == "stair":
                    up, dest = args
                    d = f", dest = {lua_str(dest)}" if dest else ""
                    out.append(f'des.stair({{ dir = {lua_str("up" if up else "down")}, '
                               f'x = {x}, y = {y}{d} }})')
                elif kind == "altar":
                    out.append(f'des.altar({{ x = {x}, y = {y}, '
                               f'align = {lua_str(args[0])}, type = "shrine" }})')
        # people, things, signs
        for ch, (kind, args) in sorted(self.spec.items()):
            for (x, y) in self.cells(ch):
                if kind == "npc":
                    out.append(f'des.monster({{ npc = {lua_str(args[0])}, '
                               f'x = {x}, y = {y} }})')
                elif kind == "mon":
                    name, peaceful = args
                    p = "" if peaceful is None else f", peaceful = {1 if peaceful else 0}"
                    out.append(f'des.monster({{ id = {lua_str(name)}, '
                               f'x = {x}, y = {y}{p} }})')
                elif kind == "obj":
                    out.append(f'des.object({{ id = {lua_str(args[0])}, '
                               f'x = {x}, y = {y} }})')
                elif kind == "item":
                    out.append(f'des.object({{ id = "data shard", '
                               f'name = {lua_str(args[0])}, buc = "uncursed", '
                               f'x = {x}, y = {y} }})')
                elif kind == "sign":
                    out.append(f'des.engraving({{ x = {x}, y = {y}, type = "burn", '
                               f'text = {lua_str(args[0])} }})')
        arr = [c for ch, (k, _) in self.spec.items() if k == "arrive"
               for c in self.cells(ch)]
        if arr:
            xs, ys = [c[0] for c in arr], [c[1] for c in arr]
            for d in ("down", "up"):
                out.append(f"des.teleport_region({{ region = {{{min(xs)},{min(ys)},"
                           f"{max(xs)},{max(ys)}}}, dir = {lua_str(d)} }})")
        return "\n".join(out) + "\n"


def load(name):
    path = os.path.join(LAYOUTS, name + ".py")
    spec = importlib.util.spec_from_file_location("layout_" + name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    d = District(mod.PROTO, mod.TITLE, mod.MAP, getattr(mod, "BLURB", ""),
                 getattr(mod, "FLAGS", ()))
    d.layout_name = name
    d.alllit = getattr(mod, "LIT", False)
    mod.legend(d)
    d.analyze()
    return d


def main(argv):
    names = sorted(f[:-3] for f in os.listdir(LAYOUTS) if f.endswith(".py"))
    check = len(argv) > 1 and argv[1] in ("--check", "--show")
    if len(argv) > 2 and argv[1] == "--show":
        names = [argv[2]]
    bad = 0
    for n in names:
        try:
            d = load(n)
        except LayoutError as e:
            print(f"{n}: {e}")
            bad += 1
            continue
        if check:
            nrooms = sum(1 for r in d.rooms if r["kind"] == "room")
            print(f"== {n}: {d.proto}, {d.w}x{d.h}, {nrooms} shops/rooms, "
                  f"{len(d.variants)} variants")
            print(d.preview())
        for e in d.errors:
            print(f"{n}: {e}")
        if d.errors:
            bad += 1
            continue
        if not check:
            path = os.path.join(TOP, "dat", d.proto + ".lua")
            with open(path, "w") as f:
                f.write(d.emit())
            print("wrote", os.path.relpath(path, TOP))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main(sys.argv)
