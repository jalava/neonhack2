# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
# Zenith: the corporate island, where the story ends.
#
# A sea wall all round.  West: the maglev station.  North-west: Zenith
# Medical and Maison Kurosawa.  In the middle a checkpoint fence rings the
# plaza in front of the Zenith Spire; the lift in its lobby is the way
# up.  North-east: the yacht club; south-east: the Aurora sky bar;
# south-west: villas behind a line of trees.

PROTO = "nc-zenith"
TITLE = "Zenith"
BLURB = """
An island of glass in a sea of black water.  The Spire at its heart is
where Neo Yendor's Grid is run from, and where it can be taken.
"""

MAP = r"""
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}............------------------......---------------......-----------.....}}}
}}}............|........|.......|......|.............|......|.........|.....}}}
}}}............|...1....|...2...|......|....-----....|......|....Y.z..|.....}}}
}}}...........^|........|.......|...^..|....|.<.|....|...^..|.........|.....}}}
}}}............----+--------+----......|....|V..|....|......-----+-----.....}}}
}}}----------.......b........c.........|....--+--....|......................}}}
}}}|........|.....................FFFFF|.............|FFFFF.................}}}
}........@@.!.....................F....|.............|....F..............^..}}}
}......=.@@.|a....................F....|.U...........|....F.................}}}
}}}|.....@@.!.................^...F..p.|.............|....F.RR..............}}}
}}}|........|..T.T.T.T.T.T.T.T.T..F....|.............|....F.................}}}
}}}----------.....................F....-------/-------....F.......f...........}
}}}............----+-----.........F....................p..F...------/------.}}}
}}}............|........|---+----.F...........e...........F...|...........|.}}}
}}}............|......z.||......|.F.......................F...|.....N.....|.}}}
}}}.......^....|.....O..||......|.FFFFFFFFFFF...FFFFFFFFFFF...|........z..|.}}}
}}}............|........||......|..QQQ......g...g.............|...........|.}}}
}}}............------------------...........................^.-------------.}}}
}}}.........................................................................}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # on the maglev platform
    d.sign("a", "MAGLEV - ZENITH")
    d.lamp("^")
    d.room("1", "clinic")
    d.sign("b", "ZENITH MEDICAL - CONCIERGE CARE")
    d.room("2", "armor shop")
    d.sign("c", "MAISON KUROSAWA")
    # the Spire
    d.lit("U")                          # the lobby
    d.lit("V")                          # the lift
    d.stair("<", up=True, dest="Zenith Spire")
    d.sign("e", "ZENITH SPIRE - THE FUTURE, MANAGED")
    d.mon("g", "corpsec sergeant")      # at the checkpoint
    d.mon("p", "security drone", peaceful=False)
    # the rest of the island
    d.lit("Y")                          # the yacht club
    d.lit("N")                          # the sky bar
    d.sign("f", "SKY BAR AURORA")
    d.lit("O")                          # a villa
    d.mon("z", "corporate exec", peaceful=True)
    # each game is a little different
    d.variant("Q", 50, "F")             # a security barrier
    d.variant("R", 50, "T")             # planters
