# Neon Mile: the nightlife strip where a new game starts.
#
# The Mile runs east-west between fences, cluttered with billboard
# pillars and a noodle cart.  North: Doc Okoye's clinic and her
# operating room (the start), a dead-end alley, the Static Lounge (Mags
# Vance behind the bar), another alley, the Kwik-Stop, the noodle bar
# and Sparky's.  South: the safehouse off a side alley, the gun shop,
# the armourer, the park, the holo-arcade and the capsule hotel with its
# back rooms; behind them all, a zigzag back alley.

PROTO = "nc-neonmile"
TITLE = "Neon Mile"
BLURB = """
The nightlife strip: bars, clinics and capsule hotels under a ceiling of
signs.  A new game starts on Doc Okoye's operating table.
"""

MAP = r"""
 --------------------------------------------- -------------------------------
 |.........|........|..|...|.................|.|.......|......|..............|
 |....1....|@.....o.|..|...|.....u..F....g...|.|...3...|..4...|.......5......|
 |.........|@.......|..--+--........F........|.|.......|......|..............|
 |.........|..k....z|..|.....2......F........|.|.......---+---|..............|
 -----+---------+----vv|...............n.....|.----+----......--------+-------
F..a..b................--------/--------------.....d......e...........f.......F
F.=..........---...............c...............................---............F
F............---........----................---................---............F
F...............i.......----................---.......l..............r........F
 --------..-----+-----.....j............p........-----/------.-------+--------
 |......|..|.........|.----+-----..T...T...T.....|..........|.|..............|
 |......|y.|.........|.|........|................|....9.....|.|..............|
 |..8...+..|....6....|.|...7....|......{.......T.|..q.......|.|......h.......|
 |......|..|.........|.|........|................|.......q..|t|..............|
 |......|..|.........|.|........|..T.......T.....|..........|.|..............|
 --------..-----------.----------................------------.----+-----------
        |........---..........s.....................--........|........|
        |.....................s..---..........................|........|
        |.....................s...............................----------
        -------------------------------------------------------
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # you wake up here
    # north side
    d.room("1", "clinic")
    d.lit("z")                          # Okoye's operating room
    d.npc("o", "okoye")
    d.obj("k", "medkit")
    d.lit("2")                          # the Static Lounge
    d.npc("g", "mags")
    d.mon("u", "urchin", peaceful=True)
    d.mon("n", "salaryman", peaceful=True)
    d.room("3", "shop")
    d.room("4", "food shop")
    d.room("5", "tool shop")
    # south side
    d.lit("8")                          # the safehouse
    d.room("6", "weapon shop")
    d.room("7", "armor shop")
    d.lit("9")                          # the holo-arcade
    d.mon("q", "urchin", peaceful=True)
    d.lit("h")                          # the capsule hotel
    # signs
    d.sign("a", "METRO - NEON MILE")
    d.sign("b", "OKOYE CHROME & STITCHES - WALK-INS WELCOME")
    d.sign("c", "THE STATIC LOUNGE - NO CHROME AT THE BAR")
    d.sign("d", "KWIK-STOP 24/7")
    d.sign("e", "NOODLE BAR")
    d.sign("f", "SPARKY'S PARTS")
    d.sign("i", "LEAD & LIGHT ARMS")
    d.sign("j", "SECOND SKIN")
    d.sign("l", "HOLO-ARCADE")
    d.sign("p", "MAYOR TRAN MEMORIAL PARK")
    d.sign("r", "CAPSULE INN - BY THE HOUR")
    d.sign("y", "UNIT 9 - RESIDENTS ONLY")
    # each game is a little different
    d.variant("v", 50, "-")             # the alley by the lounge walled off
    d.variant("s", 40, "F")             # a fence across the back alley
    d.variant("t", 30, "F")             # the cut by the hotel fenced
