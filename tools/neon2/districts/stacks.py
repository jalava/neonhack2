# The Stacks: megablocks and gang wars.
#
# North: Block 5 (sealed), a dead-end cut, Block 7 (its lobby stairs
# climb into the tower; the Chrome Saints hold the lobby and the old mail
# room), another cut, Block 9 (condemned).  South: the free clinic and
# the soy kebab stand with a back lane behind them, Block 6 with a light
# well inside (locked), and a lot full of burned-out cars.  The metro
# station is at the west end.

PROTO = "nc-stacks"
TITLE = "The Stacks"
BLURB = """
Megablocks stacked into the smog, forty floors of capsule flats each.
The Chrome Saints run Block 7 now, and they don't like visitors.
"""

MAP = r"""
 --------------------  -----------------------  ------------------------------
 |..................|..|.....|...............|..|............................|
 |..................|..|.....+...............|..|............................|
 |..................|..|.....|.g.........<...|..|............................|
 |..................|..-------...............|..|............................|
 |..................|..|.............g.......|..|............................|
 |..................|..|......7..............|..|............................|
 |..................|..|.....................|..-------------%----------------
 ---------%----------vv|.....................|...............d................
F.a.......b............-----------!-----------................................F
F.=..@@@..............^...........c...........^...............................F
F....e........f...................h.........................k.................F
 ----+-----..........-------------%---------------.i....................u.....F
 |........----+----..|...........................|...k........k.....^.........F
 |...1....|.......|..|..G........................|..............g.....k.......F
 |........|...2...|..|.....-------!-------.......|.......k....................F
 |........|.......|..|.....|.............|.......|........Q.......k...........F
-------------------..|.....|.............|.......|........Q...................F
|....................|.....|.............|.......|........Q...............k...F
|....................|.....|.............|.......|........Q...................F
-------------------------------------------------------------------------------
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # by the station
    d.lamp("^")                         # the few street lights left
    d.sign("a", "METRO - THE STACKS")
    d.sign("b", "BLOCK 5 - QUARANTINE")
    # Block 7
    d.lit("7")
    d.stair("<", up=True, dest="Block 7")
    d.mon("g", "street punk", peaceful=False)    # Chrome Saints
    d.sign("c", "BLOCK 7 - CHROME SAINTS - PAY OR PRAY")
    d.sign("d", "BLOCK 9 - CONDEMNED")
    # south side
    d.room("1", "clinic")
    d.sign("e", "STACKS FREE CLINIC - DONATIONS EXPECTED")
    d.room("2", "food shop")
    d.sign("f", "SOY KEBAB")
    d.sign("h", "BLOCK 6")
    d.obj("G", "strongbox")                      # someone's stash in Block 6
    d.sign("i", "NO DUMPING - VIOLATORS WILL BE RECYCLED")
    d.obj("k", "boulder")                        # burned-out cars
    d.mon("u", "urchin", peaceful=True)
    # each game is a little different
    d.variant("v", 50, "-")                      # the cut by Block 5 walled
    d.variant("Q", 40, "F")                      # a fence across the lot
