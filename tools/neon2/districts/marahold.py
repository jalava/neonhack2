# The Mara Sun, hold 3: the bottom of the ship.
#
# Aft, the stairs up and the engine room; amidships, containers stacked
# two high with the smugglers' lanes between them; forward, under the
# bow, Halcyon customs has caged the cargo it seized.  Matron Lin wants
# one crate out of that cage, and so does Adrian Vos.

PROTO = "nc-marahold"
TITLE = "Mara Sun, hold 3"
BLURB = """
Diesel, rust and bilge water.  The Mara Sun brings in whatever pays,
and Halcyon customs keeps whatever it can seize.
"""
FLAGS = ("noteleport",)

MAP = r"""
 ----------------------------------------------------------             
 |......|.....|.........................|.................----          
 |.E....|.....|.------..-------.........|.....s..............----       
 |....k.|.....|.------..-------..------.|.......................----    
 |......|.....|.....s............------.+.........FFFFFFFFFFF......---  
 ----+---.....|.----....................|.........F.........F........|  
 |............|.----..------............|.........F...t.....F........|  
 |............+.......------...-------..|....a....+....c....F..j..G..|  
 |............|..............s.-------..|.........F......t..F........|  
 |............|..-------................|.........F.........F........|  
 |..<.........|..-------...-------......+.........FFFFFFFFFFF......---  
 |............|............-------......|.......................----    
 |......$.....|....................s....|....................----       
 |............|.........................|.................----          
 ----------------------------------------------------------             
"""


def legend(d):
    d.stair("<", up=True)
    d.lit("E")                          # the engine room
    d.obj("k", "boulder")               # a spare engine block
    d.sign("a", "SEIZED - HALCYON CUSTOMS - DO NOT BREAK SEAL")
    d.mon("s", "smuggler", peaceful=False)
    d.mon("j", "juicer", peaceful=False)          # hired muscle
    d.mon("t", "corpsec trooper")                 # customs, Halcyon's
    d.item("c", "crate")
    d.obj("G", "strongbox")
    d.obj("$", "gold piece")
