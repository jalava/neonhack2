# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
# Lantern Market: the night market, Jade Circuit turf.
#
# North: Dr. Sato's clinic and back room, Benny Two-Tone's stall, the
# Jade Circuit's den (its stairs go down to the den), the Church
# of the Signal and the dumpling cart.  The middle is a warren of stalls
# around a fountain square; the south is back lanes, homes and more
# shops.  The metro station is at the east end.

PROTO = "nc-lantern"
TITLE = "Lantern Market"
BLURB = """
The night market under paper lanterns and pirate holo-ads.  The Jade
Circuit keeps the peace here, for a price.
"""

MAP = r"""
 ----------------  --------------------------------  -------------------------
 |.......|......|..|........|.....................|..|............|..........|
 |...1...|....s.|..|.2...b..|........3........>...|..|..4.........|....5.....|
 |.......|.z....|..|........|....y................|..|....._......|..........|
 |.......|......|..----/-----..............y......|..|............|..........|
 ----+------+----......e....|.....................|..|............-----/------
F....c......d...............-----------/-----------..------+-------....o......F
F....q............------...............f.............E.....g..................F
F..--+---.........|.8..|....u.....------..........---+---.....................F
F..|.6..|.------..|....+t.--+---..|.h..|..------..|..j..|..------...........a.F
F..|....|.|.7..|..------..|.9..|.v+....|..|.i..|..|.....|..|.k..|.......@@..=.F
F..------.|....|QQ........|....|..------..|....|..-------.G+....|.....{.@@....F
F.........--+---..........------..........--+---...........------.............F
F...........r.....M.........................D...........RR....................F
F.....J.......----+----.................N.....................O...............F
F-----+------.|.......|.............----/-----............----+-----..........F
F|..........|.|.......|..----+----..|........|..---+----..|........|..---+----F
F|....l.....|.|...m...|..|.......|..|...n....|..|......|..|...p....|..|......|F
F|..........|.|.......|..|.......|..|........|..|......|..|........|..|......|F
F|..........|.|.......|..|.......|..|........|..|......|..|........|..|......|F
-------------------------------------------------------------------------------
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # by the station
    d.sign("a", "METRO - LANTERN MARKET")
    # north side
    d.room("1", "clinic")
    d.sign("c", "DR. SATO - IMPLANTS, REPAIRS, NO QUESTIONS")
    d.lit("z")                          # Sato's back room
    d.npc("s", "sato")
    d.sign("d", "PRIVATE - KNOCK")
    d.lit("2")                          # Benny's stall
    d.npc("b", "benny")
    d.sign("e", "BENNY'S - BUY / SELL / NO REFUNDS")
    d.lit("3")                          # the Jade Circuit's den
    d.mon("y", "yakuza", peaceful=True)
    d.stair(">", up=False, dest="Jade Den")
    d.sign("f", "JADE CIRCUIT - MEMBERS ONLY")
    d.room("4", "temple")
    d.altar("_", "neutral")
    d.sign("g", "CHURCH OF THE SIGNAL - ALL FREQUENCIES WELCOME")
    d.room("5", "food shop")
    d.sign("o", "DUMPLING CART")
    # the stalls
    d.room("6", "potion shop")
    d.sign("q", "STIMS & TONICS")
    d.room("7", "wand shop")
    d.sign("r", "GADGET GRANNY")
    d.room("8", "ring shop")
    d.sign("t", "MODS & MOODS")
    d.room("9", "scroll shop")
    d.sign("u", "WAREZ")
    d.room("h", "book shop")
    d.sign("v", "DATA BROKER")
    d.room("i", "candle shop")
    d.sign("D", "LUMEN")
    d.lit("j")                          # the tea house
    d.sign("E", "TEA - HOT - CHEAP")
    d.lit("k")                          # the fortune teller
    d.sign("G", "MADAME QUBIT - FUTURES READ")
    # south side
    d.lit("l")                          # the noodle house
    d.sign("J", "NOODLES")
    d.room("m", "shop")                 # the pawn shop
    d.sign("M", "PAWN")
    d.room("n", "tool shop")            # the junk dealer
    d.sign("N", "JUNK - SCRAP - SALVAGE")
    d.room("p", "armor shop")           # the tailor
    d.sign("O", "TAILOR - KEVLAR LININGS")
    # each game is a little different
    d.variant("Q", 50, "F")             # crates block a lane
    d.variant("R", 50, "F")
