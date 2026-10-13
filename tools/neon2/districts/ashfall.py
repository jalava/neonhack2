# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
# Ashfall: old downtown, broken by the quake.
#
# Ruined blocks with their walls torn open, fissures where the ground
# split, rubble everywhere.  The old Halcyon lab in the north still has
# its sealed lift down to the Seal Vault.  The squatters' camp in the
# north-east has St. Rocco's clinic, a soup kitchen and a shelter.  In the
# middle of the south, a ring wall around the Shaft: the way down into
# the Undercity, sealed until the story opens it.  The metro station in
# the east is half buried.

PROTO = "nc-ashfall"
TITLE = "Ashfall"
BLURB = """
Downtown, before the quake.  Now it's rubble, squatters, and the Shaft:
the hole that goes all the way down.
"""

MAP = r"""
-------------------------------------------------------------------------------
|.................----------...----------------.....|........|.......|........|
|..-----..-----...|............|..............|.....|..1.....|..2....|.....u..|
|..|..........|.......g....|...|.....-----..........|........|.......|...O....|
|..|....g.........|........|...|.....|.D.|....|.k...|........|.......|.n......|
|..|..............-----..---.........--+--....|.....----+--------+---|........|
|..|..........|...............................|..........b........c..----+-----
|..--.---------................|..............|.............^.................|
|........................k.....-------..-------...............................|
|....k..........  .....................e....................k...........-------
|.RRR.............  .......................k...  .......................|...@@|
|...................  ...............f...........  ...................a.!...@@|
|...........k.........   .....-------!-------......   ............k.....|..=..|
|........................  QQ.|.............|......... .....g.........^.|.....|
|.............................|...---.---...|..........  ...............---!---
|....  .............k.........|...|  >  |...|.................................|
|......  .....................|...-------...|.----..--------------.----.......|
|........ ..----.------....k..|.............|.|.......................|.......|
|...........|.........|.......---------------.--------------..---------.......|
|.......k...|.........|.......................................................|
-------------------------------------------------------------------------------
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # in the station
    d.sign("a", "METRO - ASHFALL - SERVICE SUSPENDED")
    d.lamp("^")                         # the camp's fire barrels
    d.room("1", "clinic")
    d.sign("b", "ST. ROCCO'S CLINIC - DONATIONS")
    d.room("2", "food shop")
    d.sign("c", "SOUP")
    d.lit("O")                          # the shelter
    d.mon("u", "urchin", peaceful=True)
    d.mon("n", "citizen", peaceful=True)
    # the old lab
    d.stair("D", up=False, dest="Seal Vault")
    d.sign("e", "HALCYON R&D - SEAL 4 - AUTHORIZED PERSONNEL ONLY")
    # the Shaft
    d.stair(">", up=False, dest="The Undercity:1")
    d.sign("f", "THE SHAFT - DANGER - NO DESCENT")
    # the ruins
    d.obj("k", "boulder")
    d.mon("g", "feral", peaceful=False)
    # each game is a little different
    d.variant("Q", 50, " ")             # the fissure has spread
    d.variant("R", 50, "-")             # a fallen girder
