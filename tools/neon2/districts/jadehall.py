# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
# The Jade Den, 3F: Matron Lin's hall.
#
# The stairs come down into a hall in the west; a passage with guard
# rooms leads to the courtyard and its koi pond.  North of the courtyard
# is the shrine, south the vault (locked).  East is the Matron's hall,
# where Lin sits, with her tea room and her quarters behind it.

PROTO = "nc-jadehall"
TITLE = "Jade Den, the Matron's hall"
BLURB = """
Lacquer, incense and armed men.  Matron Lin has run the Jade Circuit
from this hall for thirty years.
"""
FLAGS = ("noteleport",)

MAP = r"""
                    -------------------------------------------
              ------|.....................|.........|.........|
              |.....|....................b|....4.._.|....E....|
------------- |..y..|...y.................+.........|.........|
|...........| |.....|.....................|.........|.........|
|.....^.....|----+--|.......}}}}}}}.......---------------+-----
|...........|.......|.......}}...}}.......|..y................|
|..<........+......./a...X..}}.{.}}......./.......Y.......l...|
|...........|.......|.......}}...}}.......|..y................|
|...........|----+--|.......}}}}}}}.......---------------+-----
|...........| |.....|.....................|.........|.........|
------------- |..y..|.................y...%....G....|.........|
              |.....|.....................|..$...$..|....O.k..|
              ------|.....................|.........|.........|
                    -------------------------------------------
"""


def legend(d):
    d.stair("<", up=True)
    d.lamp("^")
    d.sign("a", "THE MATRON RECEIVES")
    d.mon("y", "yakuza")                # friends or foes, as Lin decides
    d.lit("X")                          # the courtyard
    d.room("4", "temple")
    d.altar("_", "law")
    d.sign("b", "SHRINE OF THE JADE MOTHER")
    d.obj("G", "strongbox")             # the vault
    d.obj("$", "gold piece")
    d.lit("Y")                          # the Matron's hall
    d.npc("l", "lin")
    d.lit("E")                          # the tea room
    d.lit("O")                          # her quarters
    d.obj("k", "jade")
