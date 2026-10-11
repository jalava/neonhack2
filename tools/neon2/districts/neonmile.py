# Neon Mile: the nightlife strip where a new game starts.
#
# North side, west to east: Doc Okoye's street clinic (a cyberware shop
# with her operating room next door), the Static Lounge (Mags Vance's
# bar), a convenience store, a noodle bar, a hardware stall.
# South side: your safehouse, a gun shop, an armourer, the little park
# with the fountain, the holo-arcade, a capsule hotel.
# The metro station is at the west end of the main street.


def build():
    d = District("nc-neonmile", "Neon Mile", """
The nightlife strip: bars, clinics and capsule hotels under a ceiling of
signs.  A new game starts on Doc Okoye's operating table.
""")

    # --- north side ---------------------------------------------------
    # Okoye's clinic: the shop, and her operating room with its own door
    clinic = d.building(5, 1, 9, 6, doors=[(9, 6)])
    d.shop(clinic, "clinic")
    d.sign(9, 7, "OKOYE CHROME & STITCHES - WALK-INS WELCOME")
    oproom = d.building(13, 1, 8, 6, doors=[(16, 6)])
    d.room(oproom)
    d.npc(18, 3, "okoye")
    d.arrive(14, 2, 15, 4)            # you wake up here
    d.obj(14, 5, "medkit")

    # the Static Lounge, with the bar counter along its east side
    lounge = d.building(23, 1, 21, 6, doors=[(31, 6, "open")])
    d.room(lounge)
    for y in (3, 4):                  # the bar counter (clear of the wall)
        d.put(36, y, "F")
    d.npc(40, 2, "mags")
    d.sign(31, 7, "THE STATIC LOUNGE - NO CHROME AT THE BAR")
    d.mon(27, 3, "urchin", peaceful=True)
    d.mon(33, 5, "tourist", peaceful=True)

    shop1 = d.building(46, 1, 9, 6, doors=[(50, 6)])
    d.shop(shop1, "shop")
    d.sign(50, 7, "KWIK-STOP 24/7")
    shop2 = d.building(56, 1, 8, 6, doors=[(59, 6)])
    d.shop(shop2, "food shop")
    d.sign(59, 7, "NOODLE BAR")
    shop3 = d.building(65, 1, 11, 6, doors=[(70, 6)])
    d.shop(shop3, "tool shop")
    d.sign(70, 7, "SPARKY'S PARTS")

    # --- the street ---------------------------------------------------
    d.metro(2, 9)
    d.sign(2, 8, "METRO - NEON MILE")

    # --- south side ---------------------------------------------------
    safe = d.building(2, 12, 8, 6, doors=[(5, 12)])
    d.room(safe)
    d.sign(5, 11, "UNIT 9 - RESIDENTS ONLY")
    gun = d.building(12, 12, 10, 6, doors=[(16, 12)])
    d.shop(gun, "weapon shop")
    d.sign(16, 11, "LEAD & LIGHT ARMS")
    armor = d.building(24, 12, 10, 6, doors=[(28, 12)])
    d.shop(armor, "armor shop")
    d.sign(28, 11, "SECOND SKIN")

    # the park: trees round a fountain
    for x, y in ((37, 13), (41, 13), (45, 13), (37, 16), (45, 16), (49, 14)):
        d.put(x, y, "T")
    d.put(41, 15, "{")
    d.sign(41, 12, "MAYOR TRAN MEMORIAL PARK")

    arcade = d.building(52, 12, 11, 6, doors=[(57, 12, "open")])
    d.room(arcade)
    d.sign(57, 11, "HOLO-ARCADE")
    d.mon(55, 14, "urchin", peaceful=True)
    d.mon(59, 15, "urchin", peaceful=True)
    hotel = d.building(65, 12, 11, 6, doors=[(70, 12)])
    d.room(hotel)
    d.sign(70, 11, "CAPSULE INN - BY THE HOUR")
    return d
