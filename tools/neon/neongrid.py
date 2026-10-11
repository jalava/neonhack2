#!/usr/bin/env python3
"""NeonGrid 2: the proposed NeonHack symbol set, previewed in a terminal.

    neongrid.py preview [--stack]   sample map in NetHack ASCII, IBMgraphics
                                    (as Unicode) and NeonGrid 2, side by side
    neongrid.py glyphs              every character the proposal uses, to
                                    check the terminal's font

Run it in the terminal you play in (over ssh, too): the point is to see
what *your* terminal and font make of these characters.  See
doc/neongrid-plan.md for the reasoning.
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


def rgb(r, g, b):
    return ESC + "[38;2;%d;%d;%dm" % (r, g, b)


# --- NeonGrid 2 palette (24-bit) ----------------------------------------
# quiet background, loud foreground: structure is dim, features are
# mid-bright, monsters and objects keep NetHack's own colours
WALL = {
    "main": (60, 150, 190),       # Undercity: steel cyan
    "mines": (200, 130, 20),      # Old Metro: amber
    "sokoban": (80, 120, 255),    # Sokoban: blue, double lines
    "gehennom": (200, 40, 40),    # Gehennom: red
    "knox": (230, 190, 60),       # Fort Ludios: gold
}
P = {
    "floor": (120, 120, 145), "darkfloor": (62, 62, 84),
    "corr": (95, 95, 120), "litcorr": (150, 150, 180),
    "door": (240, 160, 40),
    "stair": (255, 255, 255), "branch": (255, 220, 0),
    "ladder": (200, 140, 60),
    "lawful": (235, 235, 255), "neutral": (165, 165, 165),
    "chaotic": (230, 50, 50), "unaligned": (200, 60, 200),
    "grave": (200, 200, 200), "throne": (255, 215, 0),
    "fountain": (60, 140, 255), "sink": (225, 225, 225),
    "tree": (40, 200, 80), "bars": (120, 200, 220),
    "pool": (40, 90, 255), "lava": (255, 70, 0), "ice": (150, 230, 255),
    "web": (190, 190, 190), "engr": (190, 170, 255),
    "bridge": (160, 110, 50),
}

# --- what each map code is ------------------------------------------------
# code: (description, ascii (char, clr), ibm (char, clr), neon (char, rgb))
# Walls ('W'), doors and branch walls are handled separately.
FEATURES = {
    ".": ("lit floor", (".", "gray"), ("·", "gray"), ("·", P["floor"])),
    ",": ("dark or remembered floor", (".", "black"), ("·", "black"),
          ("·", P["darkfloor"])),
    "#": ("dark corridor", ("#", "gray"), ("░", "gray"), ("░", P["corr"])),
    "=": ("lit corridor", ("#", "gray"), ("▒", "gray"),
          ("▒", P["litcorr"])),
    "D": ("doorway", (".", "gray"), ("·", "gray"), ("·", P["door"])),
    "B": ("broken door", (".", "gray"), ("·", "gray"), ("·", P["door"])),
    "C": ("closed door", ("+", "brown"), ("+", "brown"), ("■", P["door"])),
    "<": ("staircase up", ("<", "gray"), ("<", "gray"), ("<", P["stair"])),
    ">": ("staircase down", (">", "gray"), (">", "gray"),
          (">", P["stair"])),
    "b": ("branch staircase down", (">", "yellow"), (">", "yellow"),
          (">", P["branch"])),
    "u": ("ladder up", ("<", "brown"), ("<", "brown"), ("≤", P["ladder"])),
    "K": ("sink", ("{", "white"), ("⌠", "white"), ("#", P["sink"])),
    "{": ("fountain", ("{", "bright blue"), ("⌠", "bright blue"),
          ("⌠", P["fountain"])),
    "L": ("lawful altar", ("_", "gray"), ("_", "gray"), ("Ω", P["lawful"])),
    "N": ("neutral altar", ("_", "gray"), ("_", "gray"),
          ("Ω", P["neutral"])),
    "X": ("chaotic altar", ("_", "gray"), ("_", "gray"),
          ("Ω", P["chaotic"])),
    "G": ("grave", ("|", "white"), ("|", "white"), ("†", P["grave"])),
    "T": ("throne", ("\\", "yellow"), ("\\", "yellow"), ("\\", P["throne"])),
    "Y": ("tree", ("#", "green"), ("±", "green"), ("♣", P["tree"])),
    "H": ("iron bars", ("#", "cyan"), ("≡", "cyan"), ("≡", P["bars"])),
    "~": ("pool", ("}", "blue"), ("≈", "blue"), ("≈", P["pool"])),
    "V": ("lava", ("}", "red"), ("≈", "red"), ("▓", P["lava"])),
    "I": ("ice", (".", "cyan"), ("·", "cyan"), ("◦", P["ice"])),
    "E": ("engraving", ("`", "bright blue"), ("ε", "bright blue"),
          ("§", P["engr"])),
    "w": ("web", ('"', "gray"), ('"', "gray"), ("¤", P["web"])),
    "0": ("boulder", ("`", "gray"), ("`", "gray"), ("0", (200, 200, 200))),
    " ": ("solid rock", (" ", "gray"), (" ", "gray"), (" ", (0, 0, 0))),
}
# things NetHack colours itself: identical in every panel
THINGS = {
    "@": ("you", "white"), "f": ("your kitten (hilite_pet)", "white"),
    "d": ("jackal", "brown"), "o": ("hill mutant", "yellow"),
    "^": ("teleportation trap", "magenta"), "!": ("vial", "bright magenta"),
    "?": ("script", "white"), "+": ("spellchip", "bright blue"),
    "[": ("armor", "cyan"), ")": ("weapon", "cyan"),
    '"': ("amulet", "cyan"), "$": ("credits", "yellow"),
}

# the sample level: W = wall (joined automatically), O = open door
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

# box drawing by which neighbours are wall: (up, down, left, right)
BOX = {
    "single": {
        (0, 0, 1, 1): "─", (1, 1, 0, 0): "│", (0, 1, 0, 1): "┌",
        (0, 1, 1, 0): "┐", (1, 0, 0, 1): "└", (1, 0, 1, 0): "┘",
        (1, 1, 1, 1): "┼", (1, 0, 1, 1): "┴", (0, 1, 1, 1): "┬",
        (1, 1, 1, 0): "┤", (1, 1, 0, 1): "├",
    },
    "double": {
        (0, 0, 1, 1): "═", (1, 1, 0, 0): "║", (0, 1, 0, 1): "╔",
        (0, 1, 1, 0): "╗", (1, 0, 0, 1): "╚", (1, 0, 1, 0): "╝",
        (1, 1, 1, 1): "╬", (1, 0, 1, 1): "╩", (0, 1, 1, 1): "╦",
        (1, 1, 1, 0): "╣", (1, 1, 0, 1): "╠",
    },
}
# NetHack's ASCII wall characters for the same shapes
ASCII_WALL = {
    (0, 0, 1, 1): "-", (1, 1, 0, 0): "|", (1, 1, 1, 0): "|",
    (1, 1, 0, 1): "|",
}


def at(rows, x, y):
    if 0 <= y < len(rows) and 0 <= x < len(rows[y]):
        return rows[y][x]
    return " "


def wall_shape(rows, x, y):
    def joins(c):
        return c == "W" or c in DOORS
    up, down = joins(at(rows, x, y - 1)), joins(at(rows, x, y + 1))
    left, right = joins(at(rows, x - 1, y)), joins(at(rows, x + 1, y))
    key = (int(up), int(down), int(left), int(right))
    if key not in BOX["single"]:  # line ends: extend the line
        key = (0, 0, 1, 1) if (left or right) else (1, 1, 0, 0)
    return key


def door_in_vertical_wall(rows, x, y):
    return at(rows, x, y - 1) == "W" or at(rows, x, y + 1) == "W"


def cell(rows, x, y, style, branch="main"):
    c = rows[y][x]
    if c in THINGS:
        desc, clr = THINGS[c]
        attr = ESC + "[7m" if c == "f" else ""
        return attr + ansi(clr) + c + RESET
    if c == "W":
        shape = wall_shape(rows, x, y)
        if style == "ascii":
            return ansi("gray") + ASCII_WALL.get(shape, "-") + RESET
        if style == "ibm":
            return ansi("gray") + BOX["single"][shape] + RESET
        box = "double" if branch == "sokoban" else "single"
        return rgb(*WALL[branch]) + BOX[box][shape] + RESET
    if c == "O":
        vert = door_in_vertical_wall(rows, x, y)
        if style == "ascii":
            return ansi("brown") + ("-" if vert else "|") + RESET
        if style == "ibm":
            return ansi("brown") + "■" + RESET
        return rgb(*P["door"]) + ("─" if vert else "│") + RESET
    desc, a, i, n = FEATURES[c]
    if style == "ascii":
        return ansi(a[1]) + a[0] + RESET
    if style == "ibm":
        return ansi(i[1]) + i[0] + RESET
    return rgb(*n[1]) + n[0] + RESET


def render(rows, style, branch="main"):
    return ["".join(cell(rows, x, y, style, branch)
                    for x in range(len(rows[y]))) for y in range(len(rows))]


def preview(stack):
    width = max(len(r) for r in SAMPLE)
    rows = [r.ljust(width) for r in SAMPLE]
    panels = [
        ("NetHack ASCII (default)", render(rows, "ascii")),
        ("IBMgraphics, as Unicode", render(rows, "ibm")),
        ("NeonGrid 2 (proposed)", render(rows, "neon")),
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
    line = []
    for branch in ("main", "mines", "sokoban", "gehennom", "knox"):
        line.append(render(room, "neon", branch))
    names = ("Undercity", "Old Metro", "Sokoban", "Gehennom", "Ludios")
    print("  ".join(n.ljust(8) for n in names))
    for y in range(len(room)):
        print("  ".join(p[y] for p in line))
    print()
    print("Legend (NeonGrid 2):")
    seen = []
    for c in "WCODB.,#=<>bu{KLNXGTYH~VIEw0":
        if c == "W":
            seen.append(rgb(*WALL["main"]) + "─│┌┼" + RESET + " walls")
        elif c == "O":
            seen.append(rgb(*P["door"]) + "─ │" + RESET + " open door")
        else:
            desc, _, _, (ch, col) = FEATURES[c]
            seen.append(rgb(*col) + ch + RESET + " " + desc)
    for i in range(0, len(seen), 3):
        print("  " + "".join(s + " " * max(1, 27 - visible_len(s))
                             for s in seen[i:i + 3]))


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


GLYPHS = [
    ("WGL4: in Consolas, Cascadia, Lucida Console and every Windows font",
     [("─│┌┐└┘├┤┬┴┼", "single walls"),
      ("═║╔╗╚╝╠╣╦╩╬", "double walls (Sokoban)"),
      ("░▒▓█", "corridors, lava"),
      ("·◦", "floor, doorway, ice"),
      ("■", "closed door"),
      ("≈≡≤≥", "water, bars, ladders"),
      ("♣†Ω§¤⌠", "tree grave altar engraving web fountain")]),
    ("Beyond WGL4: Cascadia Mono or font fallback (candidates for extras)",
     [("╭╮╰╯", "rounded corners"),
      ("╌╎", "dashed lines (open doors)"),
      ("╱╲╳", "diagonal beams, web"),
      ("▔▁▏▕", "engulf / explosion edges"),
      ("≋", "triple tilde")]),
    ("Emoji (messages only, never the map): two cells wide",
     [("💥🚨💻🦾", "gunfire heat hacking implant")]),
]


def glyphs():
    print("Each character sits between bars.  It is fine if the bars stay")
    print("in straight columns and the character looks like the rest of")
    print("the font.  Note any that are boxes, question marks, blank, or")
    print("push the bars out of line (emoji are meant to: they are two")
    print("cells wide, which is why the map never uses them).\n")
    for title, groups in GLYPHS:
        print(title)
        for chars, desc in groups:
            print("  |" + "|".join(chars) + "|   " + desc)
            print("  |" + "|".join("." * len(chars)) + "|")
        print()


def main(argv):
    if len(argv) < 2 or argv[1] not in ("preview", "glyphs"):
        print(__doc__.strip())
        return 1
    if argv[1] == "preview":
        preview("--stack" in argv)
    else:
        glyphs()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
