#!/usr/bin/env python3
"""Rewrite NetHack terminology to NeonHack terminology in prose data
files (rumors, oracles, ...).  Matches whole words, keeps the case of
the first letter, and prints a diff-like summary.

  retext.py FILE...
"""
import re, sys

# (pattern, replacement); patterns are regexes matched case-insensitively
# on word boundaries, longest first
TERMS = [
    (r"Gnomish Mines", "Old Metro"),
    (r"Dungeons of Doom", "Undercity"),
    (r"Mines' End", "End of the Line"),
    (r"Minetown", "Metro Market"),
    (r"Keystone Kops?", "rent-a-cops"),
    (r"master mind flayers?", "master brainjacker"),
    (r"mind flayers", "brainjackers"),
    (r"mind flayer", "brainjacker"),
    (r"elven mithril-coats?", "synthsilk coat"),
    (r"dwarvish mithril-coats?", "titanium mesh coat"),
    (r"mithril-coats?", "titanium mesh coat"),
    (r"mithril", "titanium"),
    (r"crystal balls", "satellite uplinks"),
    (r"crystal ball", "satellite uplink"),
    (r"cornuthaums?", "tinfoil hat"),
    (r"brass lanterns?", "flashlight"),
    (r"oil lamps", "kerosene lamps"),
    (r"oil lamp", "kerosene lamp"),
    (r"ice box(es)?", "fridge"),
    (r"skeleton keys?", "master keycard"),
    (r"lembas wafers?", "nutri-wafer"),
    (r"lembas", "nutri-wafers"),
    (r"cram rations?", "protein brick"),
    (r"two-handed swords?", "chainsword"),
    (r"dwarvish mattocks?", "jackhammer"),
    (r"crossbow bolts", "shotgun shells"),
    (r"crossbows", "shotguns"),
    (r"crossbow", "shotgun"),
    (r"iron golems?", "battle mech"),
    (r"flesh golems?", "meat puppet"),
    (r"clay golems?", "ceramic robot"),
    (r"stone golems?", "construction mech"),
    (r"paper golems?", "printer drone"),
    (r"straw golems?", "trash robot"),
    (r"gold golems?", "gilded robot"),
    (r"golems", "robots"),
    (r"golem", "robot"),
    (r"spellbooks", "spellchips"),
    (r"spellbook", "spellchip"),
    (r"potions", "vials"),
    (r"potion", "vial"),
    (r"scrolls", "scripts"),
    (r"scroll", "script"),
    (r"gold pieces", "credits"),
    (r"zorkmids", "credits"),
    (r"zorkmid", "credit"),
    (r"leprechauns", "debt collectors"),
    (r"leprechaun", "debt collector"),
    (r"nymphs", "grifters"),
    (r"nymph", "grifter"),
    (r"kobolds", "ratkin"),
    (r"kobold", "ratkin"),
    (r"gnomes", "scavs"),
    (r"gnome", "scav"),
    (r"gnomish", "scav"),
    (r"dwarves", "cyborgs"),
    (r"dwarf", "cyborg"),
    (r"dwarvish", "cyborg"),
    (r"elves", "synths"),
    (r"elf", "synth"),
    (r"elven", "synth"),
    (r"orcs", "mutants"),
    (r"orc", "mutant"),
    (r"orcish", "scrap"),
    (r"watchmen", "security guards"),
    (r"watchman", "security guard"),
    (r"Archeologists", "Salvagers"),
    (r"Archeologist", "Salvager"),
    (r"Barbarians", "Nomads"),
    (r"Barbarian", "Nomad"),
    (r"Cavemen", "Ferals"),
    (r"Caveman", "Feral"),
    (r"Healers", "Ripperdocs"),
    (r"Healer", "Ripperdoc"),
    (r"Rangers", "Gunslingers"),
    (r"Ranger", "Gunslinger"),
    (r"Samurai", "Ronin"),
]

def keepcase(src, rep):
    if src[:1].isupper():
        return rep[:1].upper() + rep[1:]
    return rep

def convert(text):
    for pat, rep in TERMS:
        text = re.sub(r"(?<![\w-])%s(?![\w-])" % pat,
                      lambda m: keepcase(m.group(0), rep), text,
                      flags=re.IGNORECASE)
    return text

if __name__ == "__main__":
    for path in sys.argv[1:]:
        lines = open(path).read().split("\n")
        out = []
        n = 0
        for line in lines:
            if line.startswith("#"):
                out.append(line)
                continue
            new = convert(line)
            if new != line:
                n += 1
                print("%s:\n  - %s\n  + %s" % (path, line, new))
            out.append(new)
        open(path, "w").write("\n".join(out))
        print("== %s: %d lines changed" % (path, n), file=sys.stderr)
