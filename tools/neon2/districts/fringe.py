# The Fringe: where the city stops and the dust starts.
#
# West: the end of the metro line, a platform under a broken canopy.
# North: a squat, the field clinic, the scrap market and Road Arms.
# In the middle the Long Road has drawn its rigs into a ring around a
# fire; Old Juno's rig sits inside it, and Rook works the fire.  East:
# the Boneyard, the Long Road's fenced scrapyard, with the way down into
# its vaults.  Shacks in the south-west; dead trees; a dust fence all
# round.

PROTO = "nc-fringe"
TITLE = "The Fringe"
BLURB = """
The last stop on the line.  Past the fence there's nothing but dust and
the road, and the Long Road owns the road.
"""

MAP = r"""
F--------FFFF------------------F--------FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
F|......|....|........|.......|.|..3...|......................................F
F|......|....|...1....|...2...|.|......|......................................F
F|......|....|........|.......|.---+----..................FFFFFFFFFFFFFFFFFFFFF
F---+----....----+--------+----.....d.....................F...................F
F...............b..........c...........------...........T.F.............k.....F
F..............................------..------..------.....F...k...............F
F-----------...................------.....^..r.------.....F...................F
F|.........|........T.................................||..F.........k.........F
F|......@@.!.................||..o..------------......||.....o.............k..F
F|....=.@@.|a................||.....|..........|......||..............D.......F
F|......@@.!........QQQ......||.....|.J...j....|......||..F.g.................F
F|.........|............T....||.....|..........|..........F.....o.............F
F-----!-----..T.....................-----/------..o.......F.......k...........F
F.......................................e.................F...............k...F
F----+----.....................------...........-----.....F.....k.............F
F|.......|..----+------+---....------...------..-----.....F...................F
F|.......|..|.......|.....|.............------............FFFFFFFFFFFFFFFFFFFFF
F|.......|..|.......|.....|T................RRR...............................F
F---------..---------------.........................T.........................F
FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # on the platform
    d.sign("a", "METRO - END OF THE LINE")
    d.room("1", "clinic")
    d.sign("b", "FIELD CLINIC - NO QUESTIONS, NO REFUNDS")
    d.room("2", "tool shop")
    d.sign("c", "SCRAP & PARTS")
    d.room("3", "weapon shop")
    d.sign("d", "ROAD ARMS")
    # the camp
    d.lit("J")                          # Juno's rig
    d.npc("j", "juno")
    d.sign("e", "THE LONG ROAD - RIDE FREE OR DON'T RIDE")
    d.npc("r", "rook")
    d.lamp("^")                         # the fire
    d.mon("o", "outrider")              # friendly or not, as the Road decides
    # the Boneyard
    d.stair("D", up=False, dest="The Boneyard")
    d.sign("g", "THE BONEYARD - LONG ROAD PROPERTY - SCAVVERS WILL BE SCRAPPED")
    d.obj("k", "boulder")               # wrecks
    # each game is a little different
    d.variant("Q", 50, "F")             # a fence across the waste
    d.variant("R", 50, "-")             # a rig parked by the fence
