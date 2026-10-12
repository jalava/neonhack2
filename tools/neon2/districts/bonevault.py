# The Boneyard, 2F: the Long Road's vault.
#
# Under the scrapyard, a hangar dug into the old landfill: rigs parked in
# rows, the stairs up in the west.  East: the workshop, the Road's
# strong room (its door is kept locked) and a junk pile behind a fence.
# Old Juno keeps the Long Road's splinter close; if Halcyon ever comes
# here, it comes for that.

PROTO = "nc-bonevault"
TITLE = "The Boneyard, the vault"
BLURB = """
Rigs, fuel drums and forty years of the Long Road's salvage, under
twenty metres of landfill.
"""
FLAGS = ("noteleport",)

MAP = r"""
     -----------------       -------------------------------------    
     |...............---------................----------------------- 
     |........................................|..........|..........- 
------........................................|..........|..........| 
|.........--------...--------...--------......|...U......+...Q......| 
|.........--------...--------...--------....o.|..........|..........| 
|.............................................|..........|..........| 
|.............................................-----/----------------- 
|...<...^..........o..........^....o..........................F......|
|.............................................-------%-------.F...k..|
|.............................................|.............|.F......|
|.........--------...--------...--------......|.............|.F..k...|
|.........--------...--------...--------....o.|.....V....$..|.F.k....|
|.............................................|..$.....G....|.F-------
----........s.................................|.............|.F|      
   |..........................................---------------.F|      
   -------------------------------------------------------------      
"""


def legend(d):
    d.stair("<", up=True)
    d.lamp("^")
    d.lit("U")                          # the workshop
    d.lit("Q")                          # the bunk room
    d.lit("V")                          # the strong room
    d.obj("G", "strongbox")
    d.obj("$", "gold piece")
    d.mon("o", "outrider")              # the Road's people
    d.mon("s", "salvager", peaceful=True)
    d.obj("k", "boulder")               # junk
