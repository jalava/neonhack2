#!/usr/bin/env python3
"""Apply NeonHack role and race identities to src/role.c.

Only display strings change (names, rank titles, pantheons, quest
locations); filecodes, PM_ values and all mechanics stay as in NetHack.
Idempotent: each role block is located by its filecode.
"""
import re, sys

ROLES = {
    # filecode: (male, female, [(rank_m, rank_f)...9], (L, N, C), comment, home, goal)
    "Arc": ("Salvager", None,
            [("Scrapper", None), ("Picker", None), ("Rummager", None),
             ("Digger", None), ("Wreck Diver", None), ("Excavator", None),
             ("Deep Diver", None), ("Relic Hunter", None), ("Curator", None)],
            ("Ozymandias", "_Mnemosyne", "Entropy"), "salvage cult",
            "the Salvagers' Guild", "the Buried Datacenter"),
    "Bar": ("Nomad", None,
            [("Drifter", None), ("Outrider", None), ("Raider", None),
             ("Road Warrior", None), ("Convoy Rider", None), ("Reaver", None),
             ("Wasteland Slayer", None), ("Chieftain", "Chieftainess"),
             ("Conqueror", "Conqueress")],
            ("Saint Diesel", "_Mother Road", "Burnout"), "road saints",
            "the Duali Clan Convoy", "the Duali Oasis"),
    "Cav": ("Feral", None,
            [("Gutter Rat", None), ("Sewer Dweller", None), ("Scrounger", None),
             ("Vagrant", None), ("Pipe Crawler", None), ("Vent Runner", None),
             ("Ruin Stalker", None), ("Tunnel Lord", "Tunnel Lady"),
             ("Pioneer", None)],
            ("Old Concrete", "_Rat Mother", "Big Noise"), "undercity spirits",
            "the Sewers of the Ancestors", "the Dragon's Lair"),
    "Hea": ("Ripperdoc", None,
            [("Orderly", None), ("Medic", None), ("Paramedic", None),
             ("Trauma Tech", None), ("Bonesetter", None), ("Chemist", None),
             ("Street Surgeon", None), ("Chromesmith", None),
             ("Ripperdoc", None)],
            ("_Asklepia", "Galen", "Pestis"), "medical saints",
            "the Epidaurus Free Clinic", "the Coeus Pharmaceuticals Lab"),
    "Kni": ("Marshal", None,
            [("Recruit", None), ("Deputy", None), ("Patrolman", "Patrolwoman"),
             ("Officer", None), ("Sergeant", None), ("Detective", None),
             ("Lieutenant", None), ("Captain", None), ("High Marshal", None)],
            ("Lex", "_Justicia", "Black Badge"), "the Code",
            "Camelot Precinct", "the Isle of Glass"),
    "Mon": ("Monk", None,
            [("Candidate", None), ("Novice", None), ("Initiate", None),
             ("Student of Stones", None), ("Student of Waters", None),
             ("Student of Metals", None), ("Student of Circuits", None),
             ("Student of Fire", None), ("Master", None)],
            None, None,
            "the Monastery of Chan-Sune", "the Monastery of the Earth-Lord"),
    "Pri": ("Techpriest", "Techpriestess",
            [("Aspirant", None), ("Acolyte", None), ("Adept", None),
             ("Techpriest", "Techpriestess"), ("Curate", None),
             ("Canon", "Canoness"), ("Archivist", None),
             ("Patriarch", "Matriarch"), ("Machine Pontiff", None)],
            None, None,
            "the Great Cathedral of the Machine", "the Temple of Nalzok"),
    "Ran": ("Gunslinger", None,
            [("Tenderfoot", None), ("Plinker", None), ("Shooter", None),
             ("Gunhand", None), ("Pistolero", "Pistolera"),
             ("Gunfighter", None), ("Sharpshooter", None), ("Deadeye", None),
             ("Marksman", "Markswoman")],
            ("High Noon", "_Lady Luck", "Desperado"), "frontier saints",
            None, None),
    "Rog": ("Infiltrator", None,
            [("Footpad", None), ("Cutpurse", None), ("Sneak", None),
             ("Pilferer", None), ("Burglar", None),
             ("Cracksman", "Crackswoman"), ("Shadow", None),
             ("Phantom", None), ("Ghost", None)],
            ("Syndicate", "_Nightshade", "Kingpin"), "underworld powers",
            None, None),
    "Sam": ("Ronin", None,
            [("Hatamoto", None), ("Kenshi", None), ("Ninja", "Kunoichi"),
             ("Joshu", None), ("Ryoshu", None), ("Kokushu", None),
             ("Daimyo", None), ("Kuge", None), ("Shogun", None)],
            None, None,
            "the Taro Corporation Tower", "the Shogun's Castle"),
    "Tou": None,  # Tourist is already perfect
    "Val": ("Valkyrie", None,
            [("Rookie", None), ("Skirmisher", None), ("Merc", None),
             ("Operator", None), ("Solo", None), ("Shock Trooper", None),
             ("Hero", "Heroine"), ("Champion", None), ("Lord", "Lady")],
            None, None, None, None),
    "Wiz": ("Netrunner", None,
            [("Script Kiddie", None), ("Coder", None), ("Phreaker", None),
             ("Cracker", None), ("Hacker", None), ("Decker", None),
             ("Console Cowboy", "Console Cowgirl"), ("Wizard", None),
             ("Technomancer", None)],
            ("Root", "_Ada", "Glitch"), "machine minds",
            "the Lonely Tower", "the Tower of Darkness"),
}

RACES = {
    # filecode: (noun, adj, collective)
    "Elf": ("synth", "synth", "synthkind"),
    "Dwa": ("cyborg", "cyborg", "cyborgkind"),
    "Gno": ("scav", "scav", "scavkind"),
    "Orc": ("mutant", "mutant", "mutantkind"),
}

def cstr(s):
    return "0" if s is None else '"%s"' % s

def main(path):
    src = open(path).read()
    for code, spec in ROLES.items():
        if spec is None:
            continue
        male, female, ranks, gods, gcomment, home, goal = spec
        # role block: from '    { { "' up to and including the filecode line
        m = re.search(r'\n    \{ \{ "[^\n]*\n(?:(?!\n    \{ \{ ")[\s\S])*?\n      "%s",\n' % code, src)
        if not m:
            sys.exit("role block %s not found" % code)
        block = m.group(0)
        new = block
        # role name
        new = re.sub(r'\n    \{ \{ "[^"]*", (?:0|"[^"]*") \},',
                     '\n    { { %s, %s },' % (cstr(male), cstr(female)), new, count=1)
        # rank titles: the 9 '{ "x", y }' entries following the name line
        rank_re = re.compile(r'\{ ("[^"]*"|0), ("[^"]*"|0) \}')
        start = new.index('\n', 1) + 1
        # skip any #if 0 alternative rank lists (Ranger has one)
        body = new[start:]
        alt = re.search(r'#if 0[\s\S]*?#endif\n', body)
        skip_end = alt.end() if alt else 0
        head, tail = body[:skip_end], body[skip_end:]
        count = [0]
        def rank_sub(mm):
            i = count[0]
            count[0] += 1
            if i >= 9:
                return mm.group(0)
            rm, rf = ranks[i]
            return '{ %s, %s }' % (cstr(rm), cstr(rf))
        tail = rank_re.sub(rank_sub, tail, 9)
        if count[0] != 9:
            sys.exit("role %s: found %d ranks" % (code, count[0]))
        new = new[:start] + head + tail
        if gods:
            new = re.sub(r'\n      "[^"\n]*", "[^"\n]*", "[^"\n]*", /\* [^*]* \*/\n',
                         '\n      %s, %s, %s, /* %s */\n'
                         % (cstr(gods[0]), cstr(gods[1]), cstr(gods[2]), gcomment),
                         new, count=1)
        src = src.replace(block, new, 1)
        # quest home/goal follow the filecode line
        if home or goal:
            m2 = re.search(r'\n      "%s",\n      ("[^"]*"),\n      ("[^"]*"),\n' % code, src)
            if not m2:
                sys.exit("quest places for %s not found" % code)
            h = cstr(home) if home else m2.group(1)
            g = cstr(goal) if goal else m2.group(2)
            src = src[:m2.start()] + '\n      "%s",\n      %s,\n      %s,\n' % (code, h, g) + src[m2.end():]
    for code, (noun, adj, coll) in RACES.items():
        m = re.search(r'\n    \{\n        "[^"]*",\n        "[^"]*",\n        "[^"]*",\n        "%s",\n' % code, src)
        if not m:
            sys.exit("race %s not found" % code)
        src = src[:m.start()] + '\n    {\n        "%s",\n        "%s",\n        "%s",\n        "%s",\n' % (noun, adj, coll, code) + src[m.end():]
    open(path, "w").write(src)

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "src/role.c")
