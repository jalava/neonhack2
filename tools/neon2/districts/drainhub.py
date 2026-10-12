# The storm drains, 3F: the Drain Rats' chop shop.
#
# The stairs come down into a round chamber in the west.  A tunnel leads
# to the junction, where the drain channel snakes through and the Drain
# Rats are cutting up the hijacked rig Dustmother; its nav core is still
# in the cab.  Tunnels run on to the Rats' nest (north-east), the sump
# (south-east) and a dead end (south-west).

PROTO = "nc-drainhub"
TITLE = "Storm drains, the junction"
BLURB = """
Black water, sodium work lights and the scream of cutting torches.  The
Drain Rats strip whatever they drag down here, convoy rigs included.
"""
FLAGS = ("noteleport",)

MAP = r"""
                                     ---                         ---     
                              --------.--------               ----.----  
                           ----...............----          ---.......-- 
       ---               ---....-------------....---   ------.......$..--
    ----.----        -----......|.R.......n.|.....j-----..........p.....|
  ---.......----------........^.|...........|.^.........------.........--
  |...........--................------+------........----    --.......-- 
 --....................................}}}.....}}}...--       ----.----  
 |....<...............-...}..........}}...........}}..|          ---     
 --.............--------...}}......}}................--        ---       
  |...........---  ----|.....}..}}}..................----  -----.-----   
  ---.......---  --|..---...........j...................----.........--  
    ----.---- ----.---..--..p................p........................-- 
       ---   --.......--..--.....................-..........p........G.| 
             |.........|--..---...............----------..............-- 
             --.......-- ---- --------.--------        -----.........--  
              ----.----              ---                   -----.-----   
                 ---                                           ---       
"""


def legend(d):
    d.stair("<", up=True)
    d.lit("R")                          # the rig's cab
    d.item("n", "navcore")
    d.mon("p", "street punk", peaceful=False)    # Drain Rats
    d.mon("j", "juicer", peaceful=False)
    d.obj("$", "gold piece")
    d.obj("G", "strongbox")
    d.lamp("^")                         # work lights
