# The Stacks: megablocks and gang wars.
#
# North side: Block 5 (sealed), Block 7 (its lobby stairs climb into the
# tower; the Chrome Saints hold the lobby), Block 9 (sealed).
# South side: the free clinic, a kebab stand, Block 6 (sealed), and a
# lot full of burned-out cars.  The metro station is at the west end.


def build():
    d = District("nc-stacks", "The Stacks", """
Megablocks stacked into the smog, forty floors of capsule flats each.
The Chrome Saints run Block 7 now, and they don't like visitors.
""")

    # --- north side ---------------------------------------------------
    d.building(1, 1, 18, 7, doors=[(10, 7, "locked")])
    d.sign(10, 8, "BLOCK 5 - QUARANTINE")
    lobby = d.building(21, 1, 25, 7, doors=[(33, 7, "broken")])
    d.room(lobby)
    d.sign(33, 8, "BLOCK 7")
    d.stair(40, 3, up=True, dest="Block 7")
    for x, y in ((26, 3), (30, 5), (37, 4)):
        d.mon(x, y, "street punk", peaceful=False)
    d.sign(24, 2, "CHROME SAINTS - PAY OR PRAY")
    d.building(48, 1, 28, 7, doors=[(61, 7, "locked")])
    d.sign(61, 8, "BLOCK 9 - CONDEMNED")

    # --- the street -----------------------------------------------------
    d.metro(2, 9)
    d.sign(2, 10, "METRO - THE STACKS")

    # --- south side ---------------------------------------------------
    clinic = d.building(1, 12, 10, 6, doors=[(5, 12)])
    d.shop(clinic, "clinic")
    d.sign(5, 11, "STACKS FREE CLINIC - DONATIONS EXPECTED")
    kebab = d.building(12, 12, 8, 6, doors=[(15, 12, "open")])
    d.shop(kebab, "food shop")
    d.sign(15, 11, "SOY KEBAB")
    d.building(21, 12, 28, 6, doors=[(34, 12, "locked")])
    d.sign(34, 11, "BLOCK 6")

    # the lot: wrecks (boulders) and a couple of scavengers
    for x, y in ((53, 13), (57, 15), (62, 13), (66, 16), (70, 14), (60, 11)):
        d.obj(x, y, "boulder")
    d.mon(64, 14, "street punk", peaceful=False)
    d.mon(72, 12, "urchin", peaceful=True)
    d.sign(51, 12, "NO DUMPING - VIOLATORS WILL BE RECYCLED")
    return d
