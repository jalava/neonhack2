# Block 7, the roof (5F): Hex's barricaded server room, under siege.


def build():
    d = District("nc-b7roof", "Block 7 roof", """
The roof of Block 7: air-con stacks, antennas, and a server room whose
door the Chrome Saints have been failing to open for a week.
""")
    d.flags.append("noteleport")
    d.stair(4, 9, up=False)                 # down to the fourth floor
    d.sign(6, 9, "ROOF ACCESS - NO SMOKING")

    srv = d.building(28, 4, 21, 10, doors=[(38, 13, "locked")])
    d.room(srv)
    for y in (6, 7, 8, 10, 11):             # server racks
        d.put(31, y, "F")
        d.put(45, y, "F")
    d.npc(38, 8, "hex")
    d.obj(36, 10, "cyberdeck")
    d.sign(38, 14, "GO AWAY. I MEAN IT. -H")

    # air-con stacks and antennas
    for x, y in ((12, 3), (14, 3), (12, 15), (60, 4), (64, 4), (60, 14)):
        d.obj(x, y, "boulder")
    for x, y in ((20, 1), (54, 2), (70, 9), (22, 17)):
        d.put(x, y, "F")

    # the Saints at the door
    for x, y in ((34, 15), (42, 15), (27, 14)):
        d.mon(x, y, "street punk", peaceful=False)
    d.mon(38, 17, "black hat", peaceful=False)
    return d
