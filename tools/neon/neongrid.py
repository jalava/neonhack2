#!/usr/bin/env python3
"""NeonGrid: NeonHack's symbol sets, generated and previewed from one table.

    neongrid.py preview [--plus] [--stack]
                        sample level in NetHack ASCII, IBMgraphics (as
                        Unicode) and NeonGrid (or NeonGrid+), side by side
    neongrid.py glyphs  every character the sets use, to check a font
    neongrid.py symset  print the NeonGrid and NeonGrid+ symsets
    neongrid.py install write them into dat/symbols

NeonGrid uses only WGL4 characters, which every Windows font has
(Consolas, Cascadia Mono, Lucida Console); NeonGrid+ adds rounded corners,
dashed doors, diagonal beams and thin engulf edges for fonts that have
them (Cascadia Mono does).  Run preview and glyphs in the terminal you
play in, over ssh too.  See doc/neongrid-plan.md for the reasoning.
"""

import os
import sys

ESC = "\033"
RESET = ESC + "[0m"

# NetHack colour numbers (CLR_*) as ANSI SGR, the way tty draws them
CLR = {
    "black": "90", "red": "31", "green": "32", "brown": "33", "blue": "34",
    "magenta": "35", "cyan": "36", "gray": "37", "orange": "91",
    "bright green": "92", "yellow": "93", "bright blue": "94",
    "bright magenta": "95", "bright cyan": "96", "white": "97",
}


def ansi(clr):
    return ESC + "[" + CLR[clr] + "m"


def rgb(col):
    return ESC + "[38;2;%d;%d;%dm" % col


# --- palette ---------------------------------------------------------------
# quiet background, loud foreground: structure is dim, features are
# mid-bright, monsters and objects keep NetHack's own colours
BRANCHES = [  # (G_ suffix, name, wall colour)
    ("main", "Undercity", (60, 150, 190)),      # steel cyan
    ("mines", "Old Metro", (200, 130, 20)),     # amber
    ("sokoban", "Sokoban", (80, 120, 255)),     # blue, double lines
    ("gehennom", "Gehennom", (200, 40, 40)),    # red
    ("knox", "Ludios", (230, 190, 60)),         # gold
]
FLOOR, DARKFLOOR = (120, 120, 145), (62, 62, 84)
CORR, LITCORR = (95, 95, 120), (150, 150, 180)
DOOR = (240, 160, 40)
STAIR, BRANCH, LADDER = (255, 255, 255), (255, 220, 0), (200, 140, 60)
BRIDGE = (160, 110, 50)
ALTARS = [  # (G_ name, description, colour)
    ("G_lawful_altar", "lawful altar", (235, 235, 255)),
    ("G_neutral_altar", "neutral altar", (165, 165, 165)),
    ("G_chaotic_altar", "chaotic altar", (230, 50, 50)),
    ("G_unaligned_altar", "unaligned altar", (200, 60, 200)),
    ("G_altar_other", "high altar", (255, 110, 255)),
]

# --- the sets ----------------------------------------------------------------
WALLS = ["S_vwall", "S_hwall", "S_tlcorn", "S_trcorn", "S_blcorn",
         "S_brcorn", "S_crwall", "S_tuwall", "S_tdwall", "S_tlwall",
         "S_trwall"]
SINGLE = dict(zip(WALLS, "│─┌┐└┘┼┴┬┤├"))
DOUBLE = dict(zip(WALLS, "║═╔╗╚╝╬╩╦╣╠"))
ROUNDED = {"S_tlcorn": "╭", "S_trcorn": "╮", "S_blcorn": "╰",
           "S_brcorn": "╯"}

# S_ symbol: (character, colour or None to keep NetHack's, comment)
NEONGRID = {
    "S_ndoor": ("·", DOOR, "doorway, broken door"),
    "S_vodoor": ("─", DOOR, "open door, perpendicular to its wall"),
    "S_hodoor": ("│", DOOR, "open door"),
    "S_vcdoor": ("■", DOOR, "closed door"),
    "S_hcdoor": ("■", DOOR, "closed door"),
    "S_bars": ("≡", (120, 200, 220), "iron bars"),
    "S_tree": ("♣", (40, 200, 80), "tree"),
    "S_room": ("·", FLOOR, "lit floor"),
    "S_darkroom": ("·", DARKFLOOR, "dark or remembered floor"),
    "S_engroom": ("§", (190, 170, 255), "engraving"),
    "S_corr": ("░", CORR, "dark corridor"),
    "S_litcorr": ("▒", LITCORR, "lit corridor"),
    "S_engrcorr": ("§", (190, 170, 255), "engraving in a corridor"),
    "S_upstair": ("<", STAIR, "staircase up"),
    "S_dnstair": (">", STAIR, "staircase down"),
    "S_upladder": ("≤", LADDER, "ladder up"),
    "S_dnladder": ("≥", LADDER, "ladder down"),
    "S_brupstair": ("<", BRANCH, "branch staircase up"),
    "S_brdnstair": (">", BRANCH, "branch staircase down"),
    "S_brupladder": ("≤", BRANCH, "branch ladder up"),
    "S_brdnladder": ("≥", BRANCH, "branch ladder down"),
    "S_altar": ("Ω", (165, 165, 165), "altar (see the G_ lines)"),
    "S_grave": ("†", (200, 200, 200), "grave"),
    "S_throne": ("\\", (255, 215, 0), "throne"),
    "S_sink": ("#", (225, 225, 225), "sink"),
    "S_fountain": ("⌠", (60, 140, 255), "fountain"),
    "S_pool": ("≈", (40, 90, 255), "pool, moat"),
    "S_ice": ("◦", (150, 230, 255), "ice"),
    "S_lava": ("▓", (255, 70, 0), "lava"),
    "S_lavawall": ("█", (255, 140, 0), "wall of lava"),
    "S_vodbridge": ("═", BRIDGE, "lowered drawbridge"),
    "S_hodbridge": ("║", BRIDGE, "lowered drawbridge"),
    "S_vcdbridge": ("■", BRIDGE, "raised drawbridge"),
    "S_hcdbridge": ("■", BRIDGE, "raised drawbridge"),
    "S_cloud": ("▒", (210, 210, 220), "cloud"),
    "S_water": ("≈", (70, 140, 255), "wall of water"),
    "S_web": ("¤", (190, 190, 190), "web"),
    "S_vbeam": ("│", None, "zap beams keep their zap colours"),
    "S_hbeam": ("─", None, ""),
    "S_poisoncloud": ("▓", (110, 240, 80), "poison cloud"),
    # S_boulder is ignored by UTF8 symsets; the boulder's glyph isn't
    "G_boulder": ("0", (190, 190, 190), "boulder"),
}
# NeonGrid+: changes on top of NeonGrid, outside WGL4
PLUS = {
    "S_vodoor": ("╌", DOOR, "open door, dashed"),
    "S_hodoor": ("╎", DOOR, "open door, dashed"),
    "S_web": ("╳", (190, 190, 190), "web"),
    "S_lslant": ("╲", None, "diagonal beams"),
    "S_rslant": ("╱", None, ""),
    "S_sw_tc": ("▔", None, "engulf edges"),
    "S_sw_ml": ("▏", None, ""),
    "S_sw_mr": ("▕", None, ""),
    "S_sw_bc": ("▁", None, ""),
    "S_expl_tc": ("▔", None, "explosion edges"),
    "S_expl_ml": ("▏", None, ""),
    "S_expl_mr": ("▕", None, ""),
    "S_expl_bc": ("▁", None, ""),
}


# rule 2: these must differ in shape, not just colour.  Symbols NeonGrid
# leaves alone are given as their NetHack character.
MUST_DIFFER = [
    ("S_lava", "S_pool"), ("S_ice", "S_room"), ("S_sink", "S_fountain"),
    ("S_vcdoor", "+"), ("S_altar", "_"), ("S_web", '"'),
    ("S_grave", "S_vwall"), ("S_tree", "S_corr"), ("G_boulder", "S_engroom"),
    ("S_vodoor", "S_vwall"), ("S_hodoor", "S_hwall"),
    ("S_lavawall", "S_lava"), ("S_poisoncloud", "S_corr"),
]


def check(plus):
    """pairs from MUST_DIFFER that share a character"""
    table = symset_table(plus)
    chars = dict((s, ch) for s, (ch, _, _) in table.items())
    chars.update(walls("main", plus))

    def char(s):
        return chars.get(s, s)
    return [(a, b) for a, b in MUST_DIFFER if char(a) == char(b)]


def walls(branch, plus):
    """wall S_ name -> character for a branch"""
    if branch == "sokoban":
        return dict(DOUBLE)  # no rounded double corners exist
    out = dict(SINGLE)
    if plus:
        out.update(ROUNDED)
    return out


def symset_table(plus):
    table = dict(NEONGRID)
    if plus:
        table.update(PLUS)
    return table


def u(ch):
    return "U+%04X" % ord(ch)


def colour(col):
    return "/%d-%d-%d" % col if col else ""


def symset_text(plus):
    name = "NeonGrid+" if plus else "NeonGrid"
    desc = ("NeonHack grid with rounded corners (needs Cascadia Mono or "
            "similar)" if plus else
            "NeonHack grid: box drawing and 24-bit colour, any font")
    lines = ["start: " + name, "\tDescription: " + desc,
             "\tRestrictions: primary", "\tHandling: UTF8"]

    def add(key, ch, col, note):
        entry = "\t%s: %s%s" % (key, u(ch), colour(col))
        lines.append("%-44s# %s %s" % (entry, ch, note) if note
                     else "%-44s# %s" % (entry, ch))

    main_col = BRANCHES[0][2]
    for s, ch in walls("main", plus).items():
        add(s, ch, main_col, "")
    for s, (ch, col, note) in symset_table(plus).items():
        add(s, ch, col, note)
    for g, desc, col in ALTARS:
        add(g, "Ω", col, desc)
    for suffix, bname, col in BRANCHES[1:]:
        for s, ch in walls(suffix, plus).items():
            add("G_%s_%s" % (s[2:], suffix), ch, col, bname)
    lines.append("finish")
    return "\n".join(lines) + "\n"


BEGIN = "# BEGIN NeonGrid symsets (generated by tools/neon/neongrid.py)\n"
END = "# END NeonGrid symsets\n"


def install(path):
    for plus in (False, True):
        clash = check(plus)
        if clash:
            sys.exit("not installed, these look alike: %s" % clash)
    with open(path, encoding="utf-8") as f:
        text = f.read()
    block = (BEGIN + "# Edit the table in tools/neon/neongrid.py and run "
             "\"neongrid.py install\".\n" + symset_text(False) + "\n"
             + symset_text(True) + END)
    if BEGIN in text:
        head, rest = text.split(BEGIN, 1)
        tail = rest.split(END, 1)[1]
    else:  # first install: replace the hand-written NeonGrid block
        start = text.index("start: NeonGrid\n")
        stop = text.index("finish\n", start) + len("finish\n")
        head, tail = text[:start], text[stop:]
    with open(path, "w", encoding="utf-8") as f:
        f.write(head + block + tail)


# --- preview -------------------------------------------------------------------
# code: (description, S_ symbol, ascii (char, clr), ibm (char, clr));
# walls ('W'), doors and altars are handled separately
FEATURES = {
    ".": ("lit floor", "S_room", (".", "gray"), ("·", "gray")),
    ",": ("dark floor", "S_darkroom", (".", "black"), ("·", "black")),
    "#": ("dark corridor", "S_corr", ("#", "gray"), ("░", "gray")),
    "=": ("lit corridor", "S_litcorr", ("#", "gray"), ("▒", "gray")),
    "D": ("doorway", "S_ndoor", (".", "gray"), ("·", "gray")),
    "B": ("broken door", "S_ndoor", (".", "gray"), ("·", "gray")),
    "<": ("staircase up", "S_upstair", ("<", "gray"), ("<", "gray")),
    ">": ("staircase down", "S_dnstair", (">", "gray"), (">", "gray")),
    "b": ("branch stairs", "S_brdnstair", (">", "yellow"),
          (">", "yellow")),
    "u": ("ladder up", "S_upladder", ("<", "brown"), ("<", "brown")),
    "K": ("sink", "S_sink", ("{", "white"), ("⌠", "white")),
    "{": ("fountain", "S_fountain", ("{", "bright blue"),
          ("⌠", "bright blue")),
    "G": ("grave", "S_grave", ("|", "white"), ("|", "white")),
    "T": ("throne", "S_throne", ("\\", "yellow"), ("\\", "yellow")),
    "Y": ("tree", "S_tree", ("#", "green"), ("±", "green")),
    "H": ("iron bars", "S_bars", ("#", "cyan"), ("≡", "cyan")),
    "~": ("pool", "S_pool", ("}", "blue"), ("≈", "blue")),
    "V": ("lava", "S_lava", ("}", "red"), ("≈", "red")),
    "I": ("ice", "S_ice", (".", "cyan"), ("·", "cyan")),
    "E": ("engraving", "S_engroom", ("`", "bright blue"),
          ("ε", "bright blue")),
    "w": ("web", "S_web", ('"', "gray"), ('"', "gray")),
    "0": ("boulder", "G_boulder", ("`", "gray"), ("`", "gray")),
}
ALTAR_CODES = {"L": 0, "N": 1, "X": 2}
# things NetHack colours itself: identical in every panel
THINGS = {
    "@": "white", "f": "white", "d": "brown", "o": "yellow",
    "^": "magenta", "!": "bright magenta", "?": "white",
    "+": "bright blue", "[": "cyan", ")": "cyan", '"': "cyan",
    "$": "yellow",
}

# the sample level: W = wall (joined automatically), C/O = closed/open door
SAMPLE = [
    " WWWWWWWWWCWWWW          WWWWWWWWWWWWW   ",
    " W..........$.W          W,,,,,,,,,,,W   ",
    " W.@f.....{...O##########D,,<,,,,K,,,W   ",
    " W............W     #    W,,,,,,,,,,,W   ",
    " W..!?+..E..0.W     #    W,L,,N,,X,,,W   ",
    ' W..[)..".^.w.W     #    W,,,,G,,,T,,W   ',
    " WWWWWDWWWWWWWW     #    WWWWWWWWWWWWW   ",
    "      #             #                    ",
    "      ##========    #  WWWWWWWWWWDWWWWWW ",
    "               =    ###B....~~~..IIII..W ",
    "               =       W.Y.d.~~~..II>..W ",
    "               ========O.Y...VVV..b.u..W ",
    "                       W..HHHHH......o.W ",
    "                       WWWWWWWWWWWWWWWWW ",
]
DOORS = "CODB"
# (up, down, left, right) wall neighbours -> wall symbol
SHAPES = {
    (0, 0, 1, 1): "S_hwall", (1, 1, 0, 0): "S_vwall",
    (0, 1, 0, 1): "S_tlcorn", (0, 1, 1, 0): "S_trcorn",
    (1, 0, 0, 1): "S_blcorn", (1, 0, 1, 0): "S_brcorn",
    (1, 1, 1, 1): "S_crwall", (1, 0, 1, 1): "S_tuwall",
    (0, 1, 1, 1): "S_tdwall", (1, 1, 1, 0): "S_tlwall",
    (1, 1, 0, 1): "S_trwall",
}
ASCII_WALL = {"S_vwall": "|", "S_tlwall": "|", "S_trwall": "|"}


def at(rows, x, y):
    if 0 <= y < len(rows) and 0 <= x < len(rows[y]):
        return rows[y][x]
    return " "


def wall_symbol(rows, x, y):
    def joins(c):
        return c == "W" or c in DOORS
    up, down = joins(at(rows, x, y - 1)), joins(at(rows, x, y + 1))
    left, right = joins(at(rows, x - 1, y)), joins(at(rows, x + 1, y))
    key = (int(up), int(down), int(left), int(right))
    if key not in SHAPES:  # line ends: extend the line
        key = (0, 0, 1, 1) if (left or right) else (1, 1, 0, 0)
    return SHAPES[key]


def in_vertical_wall(rows, x, y):
    return at(rows, x, y - 1) == "W" or at(rows, x, y + 1) == "W"


def cell(rows, x, y, style, plus=False, branch=0):
    c = rows[y][x]
    if c == " ":
        return " "
    if c in THINGS:
        attr = ESC + "[7m" if c == "f" else ""  # hilite_pet
        return attr + ansi(THINGS[c]) + c + RESET
    if c == "W":
        sym = wall_symbol(rows, x, y)
        if style == "ascii":
            return ansi("gray") + ASCII_WALL.get(sym, "-") + RESET
        if style == "ibm":
            return ansi("gray") + SINGLE[sym] + RESET
        suffix, _, col = BRANCHES[branch]
        return rgb(col) + walls(suffix, plus)[sym] + RESET
    if c in "CO":
        vert = in_vertical_wall(rows, x, y)
        if style == "ascii" or (style == "ibm" and c == "C"):
            ch = "+" if c == "C" else "-" if vert else "|"
            return ansi("brown") + ch + RESET
        if style == "ibm":
            return ansi("brown") + "■" + RESET
        sym = ("S_v" if vert else "S_h") + ("cdoor" if c == "C" else "odoor")
        ch, col, _ = symset_table(plus)[sym]
        return rgb(col) + ch + RESET
    if c in ALTAR_CODES:
        if style != "neon":
            return ansi("gray") + "_" + RESET
        return rgb(ALTARS[ALTAR_CODES[c]][2]) + "Ω" + RESET
    desc, sym, a, i = FEATURES[c]
    if style == "ascii":
        return ansi(a[1]) + a[0] + RESET
    if style == "ibm":
        return ansi(i[1]) + i[0] + RESET
    ch, col, _ = symset_table(plus)[sym]
    return (rgb(col) if col else ansi("gray")) + ch + RESET


def render(rows, style, plus=False, branch=0):
    return ["".join(cell(rows, x, y, style, plus, branch)
                    for x in range(len(rows[y]))) for y in range(len(rows))]


def preview(plus, stack):
    width = max(len(r) for r in SAMPLE)
    rows = [r.ljust(width) for r in SAMPLE]
    name = "NeonGrid+" if plus else "NeonGrid"
    panels = [
        ("NetHack ASCII (default)", render(rows, "ascii")),
        ("IBMgraphics, as Unicode", render(rows, "ibm")),
        (name, render(rows, "neon", plus)),
    ]
    try:
        cols = os.get_terminal_size().columns
    except OSError:
        cols = 80
    gap = "  "
    if not stack and cols >= 3 * width + 2 * len(gap):
        print(gap.join(t.ljust(width) for t, _ in panels))
        for y in range(len(rows)):
            print(gap.join(p[y] for _, p in panels))
    else:
        for title, panel in panels:
            print(title)
            print("\n".join(panel))
            print()
    print()
    # the same room in each branch's walls
    room = ["WWWWWWWW", "W......W", "W..<...W", "WWWWWWWW"]
    shown = [render(room, "neon", plus, b) for b in range(len(BRANCHES))]
    print("  ".join(n.ljust(8) for _, n, _ in BRANCHES))
    for y in range(len(room)):
        print("  ".join(p[y] for p in shown))
    print()
    print("Legend (%s):" % name)
    table = symset_table(plus)
    items = [rgb(BRANCHES[0][2]) + "".join(walls("main", plus)[s] for s in
                                           ("S_hwall", "S_vwall", "S_tlcorn",
                                            "S_crwall")) + RESET + " walls",
             rgb(DOOR) + table["S_vcdoor"][0] + RESET + " closed door",
             rgb(DOOR) + table["S_vodoor"][0] + " " + table["S_hodoor"][0]
             + RESET + " open door"]
    for c in ".,#=DB<>bu{KGTYH~VIEw0":
        desc, sym, _, _ = FEATURES[c]
        ch, col, _ = table[sym]
        items.append((rgb(col) if col else ansi("gray")) + ch + RESET
                     + " " + desc)
    for g, desc, col in ALTARS[:3]:
        items.append(rgb(col) + "Ω" + RESET + " " + desc)
    for i in range(0, len(items), 3):
        print("  " + "".join(s + " " * max(1, 24 - visible_len(s))
                             for s in items[i:i + 3]))


def visible_len(s):
    out, skip = 0, False
    for ch in s:
        if ch == ESC:
            skip = True
        elif skip:
            if ch == "m":
                skip = False
        else:
            out += 1
    return out


def glyphs():
    print("Each character sits between bars.  It is fine if the bars stay")
    print("in straight columns and the character looks like the rest of")
    print("the font.  Note any that are boxes, question marks, blank, or")
    print("push the bars out of line (emoji are meant to: they are two")
    print("cells wide, which is why the map never uses them).\n")
    base = sorted({ch for ch, _, _ in NEONGRID.values()}
                  | set(SINGLE.values()) | set(DOUBLE.values()) | {"Ω"})
    extra = sorted({ch for ch, _, _ in PLUS.values()}
                   | set(ROUNDED.values()))
    groups = [("NeonGrid (WGL4: in every Windows font)", base),
              ("NeonGrid+ adds (Cascadia Mono and similar)", extra),
              ("Emoji (messages only, never the map)",
               list("💥🚨💻🦾"))]
    for title, chars in groups:
        print(title)
        for i in range(0, len(chars), 16):
            row = chars[i:i + 16]
            print("  |" + "|".join(row) + "|")
            print("  |" + "|".join("." * len(row)) + "|")
        print()


def main(argv):
    cmd = argv[1] if len(argv) > 1 else ""
    if cmd == "preview":
        preview("--plus" in argv, "--stack" in argv)
    elif cmd == "glyphs":
        glyphs()
    elif cmd == "symset":
        sys.stdout.write(symset_text(False) + "\n" + symset_text(True))
    elif cmd == "install":
        top = os.path.dirname(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__))))
        install(os.path.join(top, "dat", "symbols"))
    else:
        print(__doc__.strip())
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
