# Halcyon Tower, the top floor: the executive level.
#
# The lift lobby in the west, with server closets either side; offices
# north and south of a ring corridor; the boardroom and the executive
# lounge in the east.  In the core: the security post and the vault,
# where Halcyon keeps its splinter behind a cage.

PROTO = "nc-halctop"
TITLE = "Halcyon Tower, executive level"
BLURB = """
Carpet you sink into, art nobody looks at, and a vault in the middle
of it all.  The view goes all the way to the sea.
"""
FLAGS = ("noteleport",)
LIT = True

MAP = r"""
---------------------------------------------------------------------------
|...........|.........|.........|.........|.........|.........|...........|
|..k........|....o....|..q......|....o....|.........|....o....|...........|
|...........|.........|.........|.........|.........|.........|.....N.....|
|...........-----+---------+---------+---------+---------+----|...........|
-----+-----.......^................r........^.................|...........|
|.....a...|.........-------+-------...-------------------.....------/------
|.........|.........|.............|...|........F.....G..|.................|
|...>...../....t....|...c..M..b...|...|..V...../...s....|.t.r.............|
|.........|.........|.............|...|........F........|.................|
|.........|.........---------------...----/--------------...------/--------
-----+-----...^.....................^.....................^.|.............|
|...........------+-----------+-----------+-----------+-----|.............|
|...........|...........|...........|...........|...........|.....Y.......|
|......k....|.....o.....|...........|.....o.....|...........|.........e...|
|...........|...........|...........|...........|...........|.............|
---------------------------------------------------------------------------
"""


def legend(d):
    d.stair(">", up=False)
    d.sign("a", "HALCYON DYNAMICS - EXECUTIVE LEVEL")
    d.lamp("^")
    d.obj("k", "large box")             # in the server closets
    d.obj("q", "cyberdeck")
    d.mon("o", "salaryman", peaceful=True)
    d.mon("t", "corpsec trooper")       # Halcyon's, friendly or not
    d.mon("r", "security drone")
    d.lit("M")                          # the security post
    d.mon("c", "security chief")
    d.mon("b", "sentry bot")
    d.lit("V")                          # the vault
    d.item("s", "splinter_halcyon")
    d.obj("G", "strongbox")
    d.lit("N")                          # the boardroom
    d.lit("Y")                          # the lounge
    d.mon("e", "corporate exec", peaceful=True)
