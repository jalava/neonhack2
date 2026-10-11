# NeonHack — design and implementation plan

NeonHack is a cyberpunk variant of NetHack 5.0.0. It keeps NetHack's look
and feel: the same 80x24 terminal map, the same symbols and commands, the same
dungeon structure and the same depth of interaction. The game around those
mechanics is re-themed for a neon-lit megacity, and adds a set of
cyberpunk-native features that plug into NetHack's existing systems
(intrinsics, skills, shops, Kops, conducts) instead of bolting on a new game.

Base: upstream tag `NetHack-5.0.0_Released`, branch `neonhack`.
Player-facing overview: `README.neonhack.md`.

## Status (2026-10-11)

All twelve phases below are implemented and committed.  Notes on where
the result differs from the plan:

* **Racial names.** dwarf -> cyborg, gnome -> scav, orc -> mutant and
  elf -> synth as planned; kobolds became ratkin.  The army became
  corporate security ("corpsec trooper" ... "corpsec captain") instead of
  adding a separate corpsec monster.
* **New monster names.** The hostile netrunner is a "black hat" because
  "netrunner" is the player-monster name of the Netrunner role.
* **Item names.** "instant noodles" became "cup of ramen" (the wish parser
  singularizes "noodles").  Implant names are singular ("optic implant",
  "muscle graft").
* **Firearms.** Rounds that hit are always spent; misses can be picked up.
  Guns add a small per-model damage bonus; shotguns hit hardest within two
  squares.  Monsters fire guns (via NetHack's launcher AI) but don't throw
  grenades.
* **Cyberware.** Implants can't be removed.  Reflex boosters make you
  very fast (FROMCYBER isn't part of INTRINSIC).
* **Hacking.** Uses cyberdeck battery charges rather than Pw.
* **Heat.** There's no bribe mechanic; heat decays over time and drops
  when a robbed shopkeeper is paid.  Squads don't deploy in Sokoban, Fort
  Ludios, Vlad's Tower, Gehennom or the endgame.
* **Display.** The default map is vanilla ASCII; the neon look is the
  opt-in `NeonGrid` symset.  Emoji are only used in tty messages on UTF-8
  locales.
* **Text.** Monk, Tourist and Valkyrie keep NetHack's quest stories (they
  fit); the Guidebook is not rewritten.

Testing: scripted tmux play-tests (`tools/nhdrive`) for every feature,
`#wizmondiff`, `#wizrumorcheck`, `luac -p` on every level file, and
multi-minute runs of NetHack's fuzzer for all 13 roles (`tools/fuzz`).
The fuzzer found and fixed: Lua levels using NetHack object names
(`obj:chest`), trap names caught by the object rename (`des.trap("arrow")`),
and a shapeshifter display update at <0,0> during level regeneration.

Ideas for later: monsters using grenades and cyberdecks, implant removal
at clinics, bribing corpsec, re-themed Fort Ludios/Medusa/Castle
descriptions, a NeonHack Guidebook, curses-port emoji support.
Every NeonHack change is a normal commit on top, so `git diff
NetHack-5.0.0_Released` always shows the complete variant.

## 1. Setting

Year 2187. The arcology-city of **Yendor** has grown kilometres down as well
as up. Its sealed lower levels — the **Undercity** — hold the ruins of
older cities, an abandoned metro, corporate black sites and, at the very
bottom, the wreckage of the first network: **Gehennom**, where rogue
**daemons** and the uploaded dead still run.

Somewhere below lies the **Amulet of Yendor** — a pre-collapse data amulet
said to hold the root keys of the Grid itself. Whoever offers it to their
patron (a megacorp AI, a street saint, an old god of the net) is uploaded
into godhood. The legendary hacker called **the Wizard of Yendor** wants it
too.

Tone: Gibson/Shadowrun with NetHack's dry humour. Genetech corporations
explain the dragons, unicorns and trolls; the iconic NetHack names stay
wherever they already fit ("grid bug", "quantum mechanic", "mail daemon",
"credit card", "Tourist").

### Naming principles

1. **Recognisable to NetHack veterans.** A renamed thing keeps its symbol,
   colour and behaviour, and its new name echoes the old role
   (leprechaun → *debt collector*, cornuthaum → *tinfoil hat*).
2. **Rename what is medieval or Tolkien, keep what is already timeless.**
   Animals, dragons and most iconic monsters stay; elves, dwarves, gnomes
   and orcs become cyberpunk factions.
3. **Old names still work for wishes.** Renamed items accept their vanilla
   names in wishes and the `#name`/discoveries prompts.
4. **Code identifiers never change.** Only display strings change
   (`PM_GNOME` is still `PM_GNOME`; it just prints as "scav"), so upstream
   fixes keep merging cleanly.

## 2. Look and feel

* Same ASCII map, same symbols, same default colours for monsters/objects.
  The map stays strictly single-width.
* New optional **neon** wall colouring: walls and doors default to bright
  blue/magenta instead of grey (`symset:plain` restores vanilla).
* **Emoji** appear only in single-line, non-map text: the new feature
  messages (hacking 💻, implants 🦾, gunfire 💥, heat 🚨), the tombstone and
  the end-of-game summary. They can be turned off with the `!emoji` option.
  The tty port is taught to measure UTF-8 by display width so emoji do not
  break wrapping or `--More--` placement.
* The game is called **NeonHack**: welcome line, version, tombstone
  ("FLATLINED"), death message ("You flatline..."), intro legacy text.

## 3. Races and roles

| Vanilla race | NeonHack race | Notes |
|---|---|---|
| human | human | |
| elf | **synth** | engineered androids; sleep res, infravision, hate mutants |
| dwarf | **cyborg** | heavily chromed miners; infravision, tough |
| gnome | **scav** | small tunnel scavengers of the Old Metro |
| orc | **mutant** | poison-resistant gang mutants; eat anything |

| Vanilla role | NeonHack role | Sample ranks | Pantheon (L / N / C) |
|---|---|---|---|
| Archeologist | **Salvager** | Scrapper … Curator | the Archive / Mother Scrap / the Rust King |
| Barbarian | **Nomad** | Drifter … Conqueror | Saint Diesel / Mother Highway / the Burning Man |
| Caveman | **Feral** | Gutter Rat … Pioneer | Old Concrete / the Rat Mother / the Big Noise |
| Healer | **Ripperdoc** | Orderly … Ripperdoc | Asklepia / the Good Nurse / the Plague Doctor |
| Knight | **Marshal** | Recruit … High Marshal | the Law / Lady Justice / the Black Badge |
| Monk | Monk | … Student of Circuits … Master | (vanilla) |
| Priest | **Techpriest** | Aspirant … Machine Pontiff | (random, as vanilla) |
| Ranger | **Gunslinger** | Plinker … Deadeye | the Sheriff / Lady Luck / the Outlaw |
| Rogue | **Infiltrator** | Footpad … Ghost | the Fence / Lady Shadow / the Kingpin |
| Samurai | **Ronin** | Hatamoto … Shogun | (vanilla) |
| Tourist | Tourist | (vanilla) | (vanilla, Discworld) |
| Valkyrie | Valkyrie | Rookie … Warlady | (vanilla, Norse) |
| Wizard | **Netrunner** | Script Kiddie … Technomancer | the Kernel / Ada / the Glitch |

Starting kits get cyberpunk equivalents automatically via the object reskin
(e.g. the Nomad's two-handed sword is a chainsword, the Gunslinger's bow and
arrows are a pistol and bullets, the Feral's club is a baseball bat). Kits
also gain a few new items: Netrunners start with a cyberdeck, Ripperdocs
with a medkit and one implant, Gunslingers with spare ammo.

## 4. Monsters

### 4.1 Renamed (behaviour unchanged)

| Class | Vanilla → NeonHack |
|---|---|
| `d` | warg → cyberwolf, winter wolf (cub) → cryowolf (pup) |
| `e` | freezing/flaming/shocking sphere → cryo/incendiary/shock mine |
| `f` | displacer beast → holo-panther |
| `h` | hobbit → urchin, dwarf → cyborg, dwarf lord/king → cyborg foreman/overseer, bugbear → juicer, (master) mind flayer → (master) brainjacker |
| `k` | kobold (large, lord, shaman) → ratkin (large, lord, shaman) |
| `l` | leprechaun → debt collector |
| `n` | wood/water/mountain nymph → grifter / cutpurse / cat burglar |
| `o` | goblin → mutant runt, hobgoblin → mutant brute, orc → mutant, hill orc → sewer mutant, Mordor orc → rad mutant, Uruk-hai → chrome mutant, orc shaman → mutant shaman, orc-captain → mutant warboss |
| `v` | fog cloud → smog cloud |
| `G` | gnome → scav, gnome lord → scav boss, gnomish wizard → scav techie, gnome king → scav king |
| `K` | Keystone Kop … Kop Kaptain → rent-a-cop … rent-a-cop captain |
| `T` | Olog-hai → war troll |
| `W` | Nazgul → black wraith |
| `'` | golems become **robots**: trash robot, printer drone, cable crawler, gilded robot, bouncer bot, wooden mannequin, meat puppet, ceramic robot, construction mech, glass robot, battle mech |
| `@` | elves → synths (scout, combat, infiltrator synth, synth lord, Synth King); watchman → security guard; watch captain → security chief; player monsters follow the role renames |
| `&` | "demon" → "daemon" throughout (water daemon, amorous daemon, …) |
| `Z`/`M` | zombies and mummies follow the race renames (scav zombie, cyborg mummy, …) |

### 4.2 New monsters

| Sym | Name | Lvl | Idea |
|---|---|---|---|
| `@` | street punk | 1 | early-game human, small gangs, knives and zip guns |
| `@` | yakuza | 5 | groups, katanas and pistols |
| `@` | netrunner | 7 | casts mage spells ("hacks"), carries a cyberdeck |
| `@` | corpsec trooper | 6 | spawned by Heat; assault rifle |
| `@` | cyberpsycho | 10 | fast berserker; drops an implant when killed |
| `'` | security drone | 3 | flies, shocks, sounds an alarm when it sees you |
| `'` | spider bot | 5 | fast multi-legged robot, shock bite |
| `'` | sentry bot | 8 | walking gun platform, assault rifle |
| `r` | robo-rat | 2 | augmented rat, shock bite, no corpse |
| `x` | glitch | 2 | flickering digital vermin; confusing touch |
| `x` | virus | 4 | infectious bite (illness) |
| `v` | nanite cloud | 6 | engulfs and corrodes gear |
| `Z` | cyberzombie | 8 | augmented corpse; claws plus weapon |
| `&` | black ICE | 14 | Gehennom daemon; drains energy and levels |

Robots (`'` plus the new drones and robo-rats) are mindless, poison and sleep
resistant, leave no corpse (salvage instead), and are the targets of
**hacking**.

## 5. Items

### 5.1 Class nouns

| Vanilla | NeonHack | Example |
|---|---|---|
| potion | **vial** | "a smoky vial", "vial of healing" |
| scroll | **script** | "a script labeled SUDO MAKE ME" |
| spellbook | **spellchip** | "a pirated spellchip", "spellchip of force bolt" |
| gold piece / zorkmid | **credit** | "53 credits" |

Wands, rings, amulets, tools, gems keep their nouns but get tech
appearances (LED, fiber-optic, QR-coded, barcode, holographic…). Script
labels become hacker strings (`SUDO MAKE ME`, `RM -RF /`, `HALT AND CATCH
FIRE`, `KIRJE` stays). Spellchip appearances become software terms
(pirated, shareware, cracked, enterprise, open-source, plaid…).

### 5.2 Renamed items (highlights)

* **Firearms reuse the launcher system.** bow → pistol, elven bow → smart
  pistol, orcish bow → zip gun, yumi → revolver; arrows → bullets (smart,
  scrap, silver, hollow-point); crossbow → shotgun, bolts → shotgun shells.
  Skills: "bow" → *firearms*, "crossbow" → *shotgun*.
* two-handed sword → chainsword, club → baseball bat, war hammer →
  sledgehammer, flail → nunchaku, dwarvish mattock → jackhammer, orcish
  dagger → shiv, dwarvish short sword → machete.
* cornuthaum → tinfoil hat, leather cloak → trench coat, alchemy smock → lab
  coat, dwarvish cloak → hoodie, cloak of magic resistance → faraday cloak,
  cloak of invisibility → optical camo cloak, cloak of displacement →
  holo-cloak, low boots → sneakers, iron shoes → steel-toe boots,
  levitation boots → antigrav boots, plate/splint/banded/chain/scale/ring
  mail → riot plate/ballistic armor/flak armor/mesh armor/plated
  vest/chainlink vest, mithril-coats → synthsilk coat / titanium mesh coat.
* crystal ball → satellite uplink, brass lantern → flashlight, ice box →
  fridge, chest → strongbox, skeleton key → master keycard, harp → keytar,
  drum → subwoofer, tooled horn → air horn, lembas → nutri-wafer, cram →
  protein brick.

### 5.3 New items

| Class | Items |
|---|---|
| Weapons | submachine gun (burst fire), assault rifle (two-handed, burst), stun baton (shock + stun), monofilament whip |
| Thrown | frag grenade, flashbang, EMP grenade (explode on impact) |
| Tools | cyberdeck (hacking), medkit (healing, safer surgery) |
| Implants | cybereyes, reflex booster, dermal plating, neural firewall, toxin filter, grounding mesh, threat sensor, neural uplink, muscle grafts, smartlink, regen nanites, sound dampener, rebreather |
| Vials | neuroblocker (suppresses cyberpsychosis) |
| Food | instant noodles |

## 6. New mechanics

### 6.1 Firearms
* Guns are launchers; bullets are ammo. Everything NetHack does for
  bows (multishot, skills, enchantment, breakage, quivering, `f`ire) works.
* **Noise:** every shot wakes monsters in a radius (`"BANG!"`, "You hear
  gunfire."). Melee and thrown weapons stay quiet — a real stealth tradeoff.
* **Burst fire:** SMGs and assault rifles add to multishot.
* Soldiers, corpsec, yakuza and sentry bots are equipped with guns, so the
  existing monster-launcher AI makes them shoot back.
* **Grenades** explode where they land: frag (blast damage), flashbang
  (blinds and stuns), EMP (disables robots, short-circuits implants, drains
  energy).

### 6.2 Cyberware
* Implants are tools; **apply** one to install it. Installation hurts
  (HP loss, chance of rejection based on Con); Ripperdocs and anyone
  applying a medkit-assisted install do much better. Cursed implants
  install glitched.
* Installed implants grant permanent intrinsics (speed, infravision,
  resistances, telepathy, warning, regeneration, stealth, magical
  breathing, AC, Str, firearm accuracy).
* **Cyberpsychosis:** each implant adds *cyber load*. Above your capacity
  (from Con, Wis and level) you get episodes: hallucination, confusion,
  rage, Charisma loss. Shown as the status condition **CyPsy**.
  Neuroblocker vials suppress it for a while.
* **EMP/shock:** lightning and EMP grenades can short out implants
  temporarily.
* New shop type: **cyberware clinic** (sells implants and medkits).
* New conduct: *never installed cyberware* ("You remained fully organic").
* `^X` / enlightenment lists installed implants.

### 6.3 Hacking
* `#hack` (or applying a cyberdeck) targets an adjacent robot or
  lock. Costs energy (Pw).
* Success chance from Int, experience level and role (Netrunners excel,
  the cyberdeck's enchantment helps).
* Robots: on success they are tamed (weak robots), pacified, or shut down;
  on failure the feedback shocks you and the robot turns hostile.
* Locks: electronically opens locked doors and boxes.

### 6.4 Heat (wanted level)
* Crimes raise **Heat**: shoplifting, murdering peacefuls, attacking
  security, robbing the vault. Heat decays slowly over time.
* At high Heat, **corpsec squads** (troopers with guns) periodically deploy
  near you — a generalisation of the Keystone Kops. Status condition
  **Wanted**.
* Paying off a shopkeeper or praying successfully cools things down.

## 7. Dungeon

| Vanilla | NeonHack |
|---|---|
| The Dungeons of Doom | **The Undercity** |
| The Gnomish Mines | **The Old Metro** (scav tunnels) |
| Minetown | **Metro Market** (station bazaar; the "Orcish Town" variant is a gang-raided market) |
| Mines' End | **End of the Line** |
| Sokoban | Sokoban (an automated warehouse) |
| Fort Ludios, Medusa, Castle, Gehennom, Vlad's Tower, Planes | names kept, flavour text re-themed |

Shops: bookstore → warez stall, liquor emporium → pharmacy, antique weapons
→ gun shop, delicatessen → noodle bar, wand shop → gadget store, rare books
→ software emporium, plus the new cyberware clinic. Izchak's lighting
store stays.

Level sounds, throne rooms (now boardrooms — "You're fired!"), barracks
("guns being cleaned"), and the castle drawbridge (the passtune is heard
as a modem handshake) get re-themed messages.

## 8. Text and data

* `rumors`, `oracles`, `engrave` (graffiti), `epitaph`, `bogusmon`
  (hallucinatory monster names): add a cyberpunk set and drop the most
  fantasy-specific lines.
* `data.base` (the `/` lookup encyclopedia): entries for every new monster
  and item, renamed keys for renamed things.
* `quest.lua`: role quest dialogue rewritten for the new roles.
* The tutorial, help files and Guidebook note the renames.

## 9. Implementation phases

Each phase is one or more commits that build and pass the smoke tests.

1. **Build and branding** — `neonhack.500` hints, binary name, version,
   welcome, intro, death and tombstone.
2. **Races and roles** — names, ranks, pantheons, quest names (`role.c`).
3. **Monster reskin** — `monsters.h` names plus every Lua, C and data
   reference, with a rename table driving a checked script.
4. **Object reskin** — names, appearances, class nouns, credits, wish
   aliases for vanilla names.
5. **New monsters and items** — monster and object table additions,
   starting inventories, monster equipment.
6. **Firearms** — noise, burst fire, grenades.
7. **Cyberware** — implants, cyber load, cyberpsychosis, clinic shop,
   conduct.
8. **Hacking** — `#hack`, cyberdeck.
9. **Heat** — crime tracking, corpsec squads, Wanted.
10. **Dungeon and text** — dungeon names, shops, sounds, rumors, oracles,
    engravings, epitaphs, bogusmon, data.base, quest text.
11. **Emoji and neon** — UTF-8-width-aware tty output, emoji option,
    neon colours.
12. **Hardening** — scripted tmux play-tests, debug-mode wishing and
    monster creation for every new thing, the built-in fuzzer, and a
    `sanity_check` run.

## 10. Testing

* `tools/nhdrive` drives the game in a detached tmux session
  (`start`, `keys`, `key`, `screen`, `stop`).
* Debug mode (`-D`) is enabled for the local playground build so tests
  can wish for items (`^W`), create monsters (`^G`), map levels (`^F`) and
  teleport between levels (`^V`).
* After every phase: clean build with no new warnings, start a game for
  several roles, then run targeted scripted checks for that phase's
  features.

## 11. Building

```sh
sh sys/unix/setup.sh sys/unix/hints/neonhack.500
make fetch-Lua        # once
make -j all           # builds and installs into ./playground
./playground/neonhack
```

On image-based distros without ncurses headers the hints file falls back
to Homebrew's ncurses headers and links the system's runtime libraries.
