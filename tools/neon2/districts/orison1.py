# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
# Orison, deck 1: the cold open's way in.
#
# Halcyon's black site under the sea.  The sub surfaces in the moon pool
# in the west; a dock, a checkpoint of bars with its gate, the guard
# booth and the armoury, the security office and the crew quarters, the
# pump room, and in the south-east the lift down.

PROTO = "nc-orison1"
TITLE = "Orison, deck 1"
BLURB = """
Halcyon's Orison black site, a week before you wake up: the job.
"""
FLAGS = ("noteleport",)
LIT = True

MAP = r"""
-----------------------------------------------------------------------------
|}}}}}}}}}..........F--------...|.............|.............|...............|
|}}}}}}}}}..........F|......|...|.......g.....|.....^.......|...............|
|}}}}}}}}}..........F|..t..t|...|...M.........|.............|.....R.........|
|}}}}}}}}}..........F|......|...|..........t..|.............|...............|
|}}}}}}}}}......a...F---+----...|.............|.............|.........z.....|
|}------}}..........F...........-------/-------.............|...............|
|}|....|}}..........F.......................................------/----------
|}|...@|......^.....F...............................d.......................|
|}|..@.!...........b/.........................................e.............|
|}|.@..|............F...................................d...................|
|}|....|}}..........F...........------/--------.............-------/---------
|}------}}..........F-----+-----|.............|.............|...............|
|}}}}}}}}}..........F|..........|...........q.|.............|...............|
|}}}}}}}}}..........F|.......k..|.......N.....|.............|...Y...........|
|}}}}}}}}}..........F|....k.....|..t..........|........^....|.........>.....|
|}}}}}}}}}..........F|..........|.............|.............|............c..|
|}}}}}}}}}..........F--------------------------.............|...............|
-----------------------------------------------------------------------------
"""


def legend(d):
    d.arrive("@")                       # in the sub
    d.sign("a", "ORISON - HALCYON DYNAMICS - NO UNAUTHORIZED PERSONNEL")
    d.sign("b", "CHECKPOINT - ID ON REQUEST")
    d.sign("e", "LIFT - DECKS 2 AND 3 - CLEARANCE BLACK")
    d.lamp("^")
    d.mon("t", "corpsec trooper", peaceful=False)
    d.mon("g", "corpsec sergeant", peaceful=False)
    d.mon("d", "security drone", peaceful=False)
    d.mon("c", "sentry bot", peaceful=False)
    d.mon("z", "combat synth", peaceful=False)
    d.lit("M")                          # the security office
    d.lit("N")                          # the crew quarters
    d.lit("R")                          # the pump room
    d.lit("Y")                          # the lift
    d.obj("k", "large box")             # the armoury's crates
    d.obj("q", "medkit")
    d.stair(">", up=False)
