# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
# Orison, deck 3: the vault, where the cold open ends.
#
# The lift lobby in the west; the long hall with its defence alcoves;
# the blast door; and the vault, where the fourth splinter sits in a cage
# of bars.  Take it, and ARBITER comes for you.

PROTO = "nc-orison3"
TITLE = "Orison, the vault"
BLURB = """
The bottom of the black site, a week before you wake up.
"""
FLAGS = ("noteleport",)
LIT = True

MAP = r"""
                ------  ------  ------  ------                              
                |....|  |....|  |....|  |....|    --------------------------
                |.c..|  |....|  |.c..|  |....|    |--....................--|
                |....|  |....|  |....|  |....|    |-......................-|
          --------!-------!-------!-------!-------|.....q..................|
 ---------|.......................................|........................|
 |.....a..|.................d.....................|.............FFFFF......|
 |........|..................................g....|.................F......|
 |...<..../.........z...............z...........b./.........V.....s.F......|
 |........|..................................t....|.................F......|
 |........|...................d...................|.............FFFFF......|
 ---------|.......................................|........................|
          --------!-------!-------!-------!-------|.....q..................|
                |....|  |....|  |....|  |....|    |-......................-|
                |....|  |.c..|  |....|  |.c..|    |--....................--|
                |....|  |....|  |....|  |....|    --------------------------
                ------  ------  ------  ------                              
"""


def legend(d):
    d.stair("<", up=True)
    d.sign("a", "DECK 3 - VAULT - CLEARANCE BLACK")
    d.sign("b", "BLAST DOOR - KEEP CLEAR")
    d.mon("c", "sentry bot", peaceful=False)      # the alcoves' guns
    d.mon("z", "combat synth", peaceful=False)
    d.mon("g", "corpsec lieutenant", peaceful=False)
    d.mon("t", "corpsec trooper", peaceful=False)
    d.mon("d", "security drone", peaceful=False)
    d.lit("V")                          # the vault
    d.item("s", "splinter_mine")
    d.obj("q", "medkit")
