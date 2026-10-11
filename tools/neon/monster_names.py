"""NeonHack monster display names, keyed by the PM_ enum suffix.

A value is either a single name (NAM) or a (male, female, neutral) tuple
(NAMS).  Monsters not listed keep their NetHack names.
"""

RENAMES = {
    # d
    "WARG": "cyberwolf",
    "WINTER_WOLF_CUB": "cryowolf pup",
    "WINTER_WOLF": "cryowolf",
    # e
    "FREEZING_SPHERE": "cryo mine",
    "FLAMING_SPHERE": "incendiary mine",
    "SHOCKING_SPHERE": "shock mine",
    # f
    "DISPLACER_BEAST": "holo-panther",
    # h
    "HOBBIT": "urchin",
    "DWARF": "cyborg",
    "DWARF_LEADER": ("cyborg foreman", "cyborg forewoman", "cyborg foreperson"),
    "DWARF_RULER": ("cyborg overlord", "cyborg overlady", "cyborg overseer"),
    "BUGBEAR": "juicer",
    "MIND_FLAYER": "brainjacker",
    "MASTER_MIND_FLAYER": "master brainjacker",
    # k
    "KOBOLD": "ratkin",
    "LARGE_KOBOLD": "large ratkin",
    "KOBOLD_LEADER": ("ratkin lord", "ratkin lady", "ratkin leader"),
    "KOBOLD_SHAMAN": "ratkin shaman",
    # l
    "LEPRECHAUN": "debt collector",
    # n
    "WOOD_NYMPH": "grifter",
    "WATER_NYMPH": "pickpocket",
    "MOUNTAIN_NYMPH": "cat burglar",
    # o
    "GOBLIN": "mutant runt",
    "HOBGOBLIN": "mutant brute",
    "ORC": "mutant",
    "HILL_ORC": "sewer mutant",
    "MORDOR_ORC": "rad mutant",
    "URUK_HAI": "chrome mutant",
    "ORC_SHAMAN": "mutant shaman",
    "ORC_CAPTAIN": "mutant warboss",
    # v
    "FOG_CLOUD": "smog cloud",
    # G
    "GNOME": "scav",
    "GNOME_LEADER": ("scav boss", "scav matriarch", "scav chief"),
    "GNOMISH_WIZARD": "scav techie",
    "GNOME_RULER": ("scav king", "scav queen", "scav ruler"),
    # K
    "KEYSTONE_KOP": "rent-a-cop",
    "KOP_SERGEANT": "rent-a-cop sergeant",
    "KOP_LIEUTENANT": "rent-a-cop lieutenant",
    "KOP_KAPTAIN": "rent-a-cop captain",
    # M
    "KOBOLD_MUMMY": "ratkin mummy",
    "GNOME_MUMMY": "scav mummy",
    "ORC_MUMMY": "mutant mummy",
    "DWARF_MUMMY": "cyborg mummy",
    "ELF_MUMMY": "synth mummy",
    # T
    "OLOG_HAI": "war troll",
    # W
    "NAZGUL": "black wraith",
    # Z
    "KOBOLD_ZOMBIE": "ratkin zombie",
    "GNOME_ZOMBIE": "scav zombie",
    "ORC_ZOMBIE": "mutant zombie",
    "DWARF_ZOMBIE": "cyborg zombie",
    "ELF_ZOMBIE": "synth zombie",
    # ' golems are robots
    "STRAW_GOLEM": "trash robot",
    "PAPER_GOLEM": "printer drone",
    "ROPE_GOLEM": "cable crawler",
    "GOLD_GOLEM": "gilded robot",
    "LEATHER_GOLEM": "bouncer bot",
    "WOOD_GOLEM": "wooden mannequin",
    "FLESH_GOLEM": "meat puppet",
    "CLAY_GOLEM": "ceramic robot",
    "STONE_GOLEM": "construction mech",
    "GLASS_GOLEM": "glass robot",
    "IRON_GOLEM": "battle mech",
    # @ synths (elves)
    "ELF": "synth",
    "WOODLAND_ELF": "scout synth",
    "GREEN_ELF": "combat synth",
    "GREY_ELF": "stealth synth",
    "ELF_NOBLE": ("synth lord", "synth lady", "synth noble"),
    "ELVEN_MONARCH": ("Synth King", "Synth Queen", "synth monarch"),
    # @ security
    "WATCHMAN": "security guard",
    "WATCH_CAPTAIN": "security chief",
    # @ player monsters follow the role renames
    "ARCHEOLOGIST": "salvager",
    "BARBARIAN": "nomad",
    "CAVE_DWELLER": ("feral", "feral", "feral"),
    "HEALER": "ripperdoc",
    "KNIGHT": "marshal",
    "CLERIC": ("techpriest", "techpriestess", "techpriest"),
    "RANGER": "gunslinger",
    "ROGUE": "infiltrator",
    "SAMURAI": "ronin",
    "WIZARD": "netrunner",
    # quest leaders, guardians and nemeses
    "KING_ARTHUR": "Commissioner Arthur",
    "HIPPOCRATES": "Doc Hippocrates",
    "MINION_OF_HUHETOTL": "Minion of Entropy",
    "STUDENT": "intern",
    "CHIEFTAIN": "clan rider",
    "NEANDERTHAL": "feral elder",
    "ATTENDANT": "orderly",
    "PAGE": "cadet",
    # & daemons
    "WATER_DEMON": "water daemon",
    "AMOROUS_DEMON": ("incubus", "succubus", "amorous daemon"),
}
