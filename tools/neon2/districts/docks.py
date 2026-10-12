# Rustwater Docks: warehouses, container stacks, the harbour.
#
# North: the Dockside Clinic, the fish market, the Rusty Anchor bar,
# a bonded warehouse the smugglers have taken over (locked; a back office
# with a strongbox), the marine supply store and the harbour authority.
# The middle is the container yard: stacks two high, a crane, lanes that
# end in steel.  South: the quay, the storm-drain outfall in the west
# (down into the drains) and the freighter Mara Sun at her berth (down
# into her holds).  The metro station is at the east end of the quay.

PROTO = "nc-docks"
TITLE = "Rustwater Docks"
BLURB = """
Cranes, containers and black water.  Whatever comes into Neo Yendor by
sea comes through Rustwater, and some of it even pays duty.
"""

MAP = r"""
-------------------------------------------------------------------------------
|.........|..|.......|..............|..|.....k.....|.....|..|.......|.........|
|..1......|..|...2...|...r..........|..|....g....k.|.....|..|...3...|....o....|
|.........|..|.......|..........n...|..|...........+..G..|..|.......|......u..|
-----+-----..----+---|..............|..|..k....g...|.....|..----+---|.........|
F.....b.........c....-------/--------..|...........|.....|.....f....-----+-----
F..........^...............d...F..F....---------%---------..^...........i.....F
F..------...........-------....F...............e..............................F
F..------...-----...-------...............................------...------.....F
F...........-----.............-----...--------............------...------.....F
F.................QQ^.........-----...--------................................F
F.-----...............------.....................-----..........-------.......F
F.-----..---------....------.........------......-----..-----...-------..----.F
F........---------...................------.............-----............----.F
F..............................................^......R................@@..a..F
F...l...................................j.............R................@@...=.F
}}---!---}}}..}}}}}}}}}}}}}}-----------!-----------}}}}}}}}}..}}}}}}}}}}}}}}}}}
 }|.....|}}}..}}}}}}}}}}}}}}|.......|.............|}}}}}}}}}..}}}}}}}}}}}}}}}} 
 }|..D..|}}}..}}}}}}}}}}}}}}|..v....+........>....|}}}}}}}}}..}}}}}}}}}}}}}}}} 
 }-------}}}}}}}}}}}}}}}}}}}|.......|.............|}}}}}}}}}}}}}}}}}}}}}}}}}}} 
 }}}}}}}}}}}}}}}}}}}}}}}}}}}-----------------------}}}}}}}}}}}}}}}}}}}}}}}}}}} 
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # by the station
    d.sign("a", "METRO - RUSTWATER DOCKS")
    d.lamp("^")                         # sodium lamps over the yard
    # north side
    d.room("1", "clinic")
    d.sign("b", "DOCKSIDE CLINIC - CASH ONLY")
    d.room("2", "food shop")
    d.sign("c", "FRESH-ISH FISH")
    d.lit("r")                          # the Rusty Anchor
    d.sign("d", "THE RUSTY ANCHOR")
    d.mon("n", "outrider", peaceful=True)        # a Long Road driver, drinking
    d.mon("g", "smuggler", peaceful=False)       # in the bonded warehouse
    d.obj("k", "boulder")                        # crates
    d.obj("G", "strongbox")
    d.sign("e", "BONDED STORE - KEEP OUT")
    d.room("3", "tool shop")
    d.sign("f", "MARINE SUPPLY")
    d.lit("o")                          # the harbour authority
    d.mon("u", "security guard", peaceful=True)
    d.sign("i", "HARBOUR AUTHORITY - ALL CARGO IS INSPECTED")
    # the quay
    d.stair(">", up=False, dest="Mara Sun")
    d.sign("j", "MARA SUN - MONROVIA")
    d.mon("v", "smuggler", peaceful=None)        # on deck
    d.stair("D", up=False, dest="Storm Drains")
    d.sign("l", "STORM OUTFALL 3 - NO ENTRY")
    # each game is a little different
    d.variant("Q", 50, "-")             # a container blocks a lane
    d.variant("R", 40, "-")             # cargo stacked on the quay
