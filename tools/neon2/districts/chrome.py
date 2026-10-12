# Chrome Heights: the corporate plaza.
#
# The odd one out: wide, clean and lit, the way Halcyon likes it.  North:
# Halcyon Wellness (a clinic with prices to match), Atelier Noir, Lumina,
# the lobby of Halcyon Tower (security on the doors; the lift core holds
# the way up), Cafe Meridian where Adrian Vos keeps a table, Ono Optics
# and a gallery.  The plaza has fountains and planters; south of it are
# residences and a parking deck, with service lanes between them.  The
# metro station is at the east end of the plaza.

PROTO = "nc-chrome"
TITLE = "Chrome Heights"
BLURB = """
Glass, steel and fountains that recycle their water twelve times.
Corpsec keeps the plaza spotless, and that includes the people.
"""

MAP = r"""
-------------------------------------------------------------------------------
|........|.......|......|..|........-------........|..|.........|......|......|
|...1....|..2....|..3...|..|........|V.<..|........|..|.E.......|..4...|...N..|
|........|.......|......|..|........---+---........|..|.....v...|......|....y.|
|........|.......---+----..|..U....................|..|.........---+---|......|
-----+-------+----...d.....|...g...............g...|..----/------.h....---+----
|.....b......c.............|.......................|.....f.................i..|
|..........................--------/---/---/--------..........................|
|.......^..............................e............................^.........|
|.............T...........T.........................T...........T..........a..|
|...................{.........^.................^.........{.............@@....|
|........z.............................{................................@@..=.|
|.............T...........T.........................T...........T.......@@....|
|...................^............T...........T............^...................|
|..........QQQ.........................R......................................|
------+------.------+------.......................------+------..------+-------
|...........|.|...........|...---!-----------!---.|...........|..|............|
|...........|.|...........|...|.........p.......|.|...........|..|............|
|...........|.|...........|...|....-...-...-....|.|...........|..|............|
|...........|.|...........|...|.................|.|...........|..|............|
-------------------------------------------------------------------------------
"""


def legend(d):
    d.metro("=")
    d.arrive("@")                       # by the station
    d.sign("a", "METRO - CHROME HEIGHTS")
    d.lamp("^")                         # the plaza is never dark
    d.room("1", "clinic")
    d.sign("b", "HALCYON WELLNESS - YOUR HEALTH IS OUR ASSET")
    d.room("2", "armor shop")
    d.sign("c", "ATELIER NOIR")
    d.room("3", "ring shop")
    d.sign("d", "LUMINA")
    # Halcyon Tower
    d.lit("U")                          # the lobby
    d.lit("V")                          # the lift core
    d.stair("<", up=True, dest="Halcyon Tower")
    d.mon("g", "security guard")
    d.sign("e", "HALCYON DYNAMICS - WE KEEP THE LIGHTS ON")
    # north-east
    d.lit("E")                          # Cafe Meridian
    d.npc("v", "vos")
    d.sign("f", "CAFE MERIDIAN")
    d.room("4", "tool shop")
    d.sign("h", "ONO OPTICS - SEE MORE")
    d.lit("N")                          # the gallery
    d.obj("y", "statue")
    d.sign("i", "GALLERY OF THE NEW FLESH")
    # the plaza and below
    d.mon("z", "corporate exec", peaceful=True)
    d.mon("p", "corpsec trooper")       # in the parking deck
    # each game is a little different
    d.variant("Q", 50, "F")             # crowd barriers
    d.variant("R", 50, "T")             # a planter
