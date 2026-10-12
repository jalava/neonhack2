# The Seal Vault, 2F: the Loom.
#
# Halcyon's old R&D floor, where the splinters were cut sixty years ago.
# The lift hall in the west, collapsed labs either side of the corridor,
# and the Loom hall: a ring of bars around the machine, open to the
# south, where its console is.  The control room is east.  Whoever took
# Hex brought her here; she is at the console, and her captor is with
# her.

PROTO = "nc-sealloom"
TITLE = "Seal Vault, the Loom"
BLURB = """
Dust on everything, and under the dust, a machine that is still warm.
"""
FLAGS = ("noteleport",)

MAP = r"""
                                           ---                            
         --------------           ----------.----------                   
         |............|        ----...................----                
         |..k ........|      ---.........................---              
         |.........g..|    -- k............................---            
 ----------------... .|   --............FFFFFFFFF............--           
 |...........|  |.....|  --..........FFF.........FFF...m......------------
 |......^....----.....----m........FFF...-------...FFF...................|
 |.................................FF....|     |....FF................q..|
 |...<............................FF.....|     |.....FF.......!....R.....|
 |.................................FF....|     |....FF...................|
 |...........----.....----.........FFF...-------...FFF......$............|
 |...........|  |.....|  --..........FFF^...h...^FFF..........------------
 ----------------.....|   --............FFF...FFF............--           
         |.........g..|    ---.....m........a............k ---            
         |....k ......|      -- .........................---              
         |............|        ----...................----                
         --------------           ----------.----------                   
                                           ---                            
"""


def legend(d):
    d.stair("<", up=True)
    d.lamp("^")                         # emergency lights
    d.npc("h", "hex")                   # if the story has brought her here
    d.sign("a", "LOOM - SPLINTER ASSEMBLY - DO NOT ENERGIZE")
    d.lamp("R")                         # the control room's screens
    d.obj("q", "cyberdeck")
    d.obj("k", "boulder")               # rubble
    d.obj("$", "gold piece")
    d.mon("m", "mutant runt", peaceful=False)
    d.mon("g", "ghoul", peaceful=False)
