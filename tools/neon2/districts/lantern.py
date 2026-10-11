# Lantern Market: the night market, Jade Circuit turf.
#
# North side: Dr. Sato's clinic and back room, Benny Two-Tone's stall,
# the Jade Circuit's den (locked; its stairs go down to the den), and
# three market stalls.  South side: the Church of the Signal, four more
# stalls.  The metro station is at the east end.


def build():
    d = District("nc-lantern", "Lantern Market", """
The night market under paper lanterns and pirate holo-ads.  The Jade
Circuit keeps the peace here, for a price.
""")

    # --- north side ---------------------------------------------------
    clinic = d.building(1, 1, 9, 6, doors=[(5, 6)])
    d.shop(clinic, "clinic")
    d.sign(5, 7, "DR. SATO - IMPLANTS, REPAIRS, NO QUESTIONS")
    back = d.building(9, 1, 8, 6, doors=[(12, 6)])
    d.room(back)
    d.npc(14, 3, "sato")

    stall = d.building(18, 1, 8, 6, doors=[(21, 6, "open")])
    d.room(stall)
    d.npc(23, 3, "benny")
    d.sign(21, 7, "BENNY'S - BUY / SELL / NO REFUNDS")

    den = d.building(27, 1, 20, 6, doors=[(36, 6, "locked")])
    d.room(den)
    d.sign(36, 7, "JADE CIRCUIT - MEMBERS ONLY")
    d.stair(44, 3, up=False, dest="Jade Den")
    d.mon(31, 3, "yakuza", peaceful=True)
    d.mon(40, 4, "yakuza", peaceful=True)

    # three stalls sharing walls
    s1 = d.building(48, 1, 9, 6, doors=[(52, 6, "open")])
    d.shop(s1, "food shop")
    d.sign(52, 7, "DUMPLING CART")
    s2 = d.building(56, 1, 10, 6, doors=[(60, 6, "open")])
    d.shop(s2, "potion shop")
    d.sign(60, 7, "STIMS & TONICS")
    s3 = d.building(65, 1, 11, 6, doors=[(70, 6, "open")])
    d.shop(s3, "wand shop")
    d.sign(70, 7, "GADGET GRANNY")

    # --- the market street --------------------------------------------
    d.mon(30, 9, "yakuza", peaceful=True)
    d.mon(50, 10, "urchin", peaceful=True)
    d.metro(74, 9)
    d.sign(74, 8, "METRO - LANTERN MARKET")

    # --- south side ---------------------------------------------------
    church = d.building(1, 12, 13, 6, doors=[(7, 12)])
    d.room(church, "temple", filled=1)
    d.lua('des.altar({ x = 7, y = 14, align = "neutral", type = "shrine" })')
    d.sign(7, 11, "CHURCH OF THE SIGNAL - ALL FREQUENCIES WELCOME")
    for i, (kind, name) in enumerate((("ring shop", "MODS & MOODS"),
                                      ("scroll shop", "WAREZ"),
                                      ("book shop", "DATA BROKER"),
                                      ("candle shop", "LUMEN"))):
        x = 16 + 11 * i
        s = d.building(x, 12, 10, 6, doors=[(x + 4, 12, "open")])
        d.shop(s, kind)
        d.sign(x + 4, 11, name)
    return d
