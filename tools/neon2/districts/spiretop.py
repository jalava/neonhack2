# The Zenith Spire, the top floor: the crown.
#
# A round floor at the top of the Spire.  The stairs come up in the west;
# the outer ring is server stacks and patrols; the inner ring holds the
# Grid's guardians, and in the middle, behind ARBITER, is the sanctum
# with the root console.  Hex is waiting there (the story brings her).

PROTO = "nc-spiretop"
TITLE = "Zenith Spire, the crown"
BLURB = """
The highest room in Neo Yendor.  From here the whole Grid can be
switched off, sold, or set free.
"""
FLAGS = ("noteleport",)
LIT = True

MAP = r"""
                      -------------------------                     
               --------...........k...........--------              
           -----......-----...............-----......-----          
       -----................-------------................-----      
     ---..s..........--------...........--------.............---    
    --..-----....-----.........................-----....-----..--   
  ---..........---..i..............................---..........--- 
  |...........--............-------------............--...........| 
 --..........--.............|..c........|.............--..........--
 |..>..a...../........b.....|.....h.....|.....b......../....t......|
 --..........--.............|...........|.............--..........--
  |...........--............------+------............--...........| 
  ---..........---..............................i..---..........--- 
    --..-----....-----............J............-----....-----..--   
     ---..t..........--------...........--------..........s..---    
       -----................-------------................-----      
           -----......-----...............-----......-----          
               --------.......................--------              
                      -------------------------                     
"""


def legend(d):
    d.stair(">", up=False)
    d.sign("a", "ZENITH SPIRE - CROWN - ROOT ACCESS ONLY")
    d.npc("h", "hex")                   # if the story has brought her here
    d.sign("c", "ROOT CONSOLE - NEO YENDOR GRID")
    d.npc("J", "arbiter")
    d.mon("b", "black ICE", peaceful=False)
    d.mon("i", "combat synth", peaceful=False)
    d.mon("s", "sentry bot")            # Halcyon's, friendly or not
    d.mon("t", "corpsec captain")
    d.obj("k", "medkit")
