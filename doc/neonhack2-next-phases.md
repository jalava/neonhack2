# NeonHack 2: next phases (P7–P10)

Status (2026-10-12): P7 (organic districts, 8be51b252), P8 (economy and
city life, 18ab4f741) and P9 (acts 2-4) are done.  Next: user playtest,
then P10 (balance, hardening).

## Context

Phases P0–P6 are done: the city, metro, journal, story engine, death modes,
and a playable slice (prologue + act 1). Playtesting led to fixes in
fbbbb591c (dialogue answers) and 248f607c9 (balance). The scrap-bullet
poison fix is also in NeonHack 1 (5f4a7b653).

The user's feedback shapes what comes next:
- **Districts:** they look like grids of boxes. The user wants them tight
  and organic, like Minetown's Grotto Town (dat/minetn-5.lua): buildings
  wall to wall, twisting alleys, dead ends, courtyards.
- **Money:** the only ways to earn are the fixed jobs, so the user went
  to the Stacks for loot and died there.

User decisions (2026-10-12):
- **Maps:** hand-drawn in Grotto style, one per district, with small
  random variations each game.
- **Lighting:** neon pools. Shops, bars and the area around signs and
  the metro are lit; back alleys are dark.
- **Order:** organic layouts first, then economy and city life, then
  acts 2–4.

Rules for every phase:
- Commit each phase separately on `neonhack2`; push nothing.
- Keep `tools/neon2/slice-test` and the fuzzer passing.
- `make all` is safe while the user is playing (fbbbb591c).
- After P7 and after P8, stop for a user playtest.

## P7: Organic districts

### Map format: `tools/neon2/district.py` becomes an ASCII-art pipeline

Each `tools/neon2/districts/<name>.py` holds a full map (up to 79x21,
placed at level (1,0)) and a legend.

**Map characters**
- Terrain uses the des.map characters: ` # . - | + { T F K` and so on.
- Doors:
  - `+` is a door in a random state;
  - `/` open, `%` locked, `!` broken;
  - all of them become `+` in the map, with `des.door(state)` after the rooms.
- Markers are digits, lowercase letters except `w` and `x`, and `@ $ & * ^ = ?`. In the map they become `.`.

**Legend calls**
- `shop('1', "clinic")` (also temples and bars), `npc('o', "okoye")`, `mon('g', "street punk", peaceful=False)`.
- `sign('a', "TEXT")`, `metro('m')`, `stair('<', up=True, dest="Block 7")`, `arrive('s')`, `obj(...)`.
- `variant(pct, cells, group=None)` for Grotto-style random wall and alley changes.

**Shops and rooms**
- The tool flood-fills from the marker using 8 neighbours, as the engine does (`mkmap.c:208`).
- A rectangle becomes `des.region{region, type, lit=1, filled=1}`. Anything else becomes `irregular=1`, seeded at the marker (`sp_lev.c:5747`). Rectangles are preferred.
- Fountains and other features inside a room are placed with `des.feature` after the room.
- Only shops, temples and clinics are real rooms. The room limit is 40 (`global.h:385`).

**The tool rejects a map when** (the engine doesn't check these for special levels)
- rows have different widths, or a marker sits on a terrain character;
- a shop has more or fewer than one door, or its door is at a corner (`shknam.c:634`);
- the cell just inside a shop door has fewer than 2 room neighbours, so the shopkeeper would block the way (`mkroom.c:1050-1093`, `invalid_shop_shape`);
- a variant touches a room, room wall, door or key cell;
- some key point is unreachable from the metro.
  - **Key points:** NPC markers, shop doors, stairs and the arrival area.
  - **How reachability is checked:** with every combination of variants, using 8-way moves, no diagonal step through a doorway, and locked doors counted as walls.

**Lighting (neon pools)**
- The whole map starts unlit.
- Pools are the floor cells within radius 3 of a sign, the metro or a shop door, reachable without crossing walls. The tool emits them as one `selection.area` per horizontal run, marked "lit".
- The engine grows lit selections by one cell (`sp_lev.c:5696`), which lights the bordering walls. Circles would also light the floor behind walls.
- Rooms are lit.

**Order in the generated .lua**
`des.map` → variants (they must come before rooms, `minetn-5.lua:36`) → "unlit" → pools → rooms → doors → features → metro/stairs → NPCs/monsters/objects → signs → arrival regions.

**`district.py --check`** validates every layout and prints each map with its pools shown, for review.

### Redraw (same landmarks, NPCs and story)

| Map | Landmarks |
|---|---|
| **Neon Mile** | Okoye's clinic and operating room (the start), the Static Lounge with Mags, the safehouse, the shops, the little park, the metro |
| **Lantern Market** | the market as a maze of stalls; Sato's clinic and back room, Benny's stall, the Jade Circuit den (locked; stairs down), the Church of the Signal, the metro |
| **The Stacks** | megablock walls with narrow cuts between them; Block 7's lobby (Chrome Saints, stairs up), the free clinic, the kebab stand, the wreck lot, the metro |
| **Block 7 roof** | cluttered rooftop paths around Hex's server room |

- The other five districts are drawn in P9 with the same tool.
- A save that has already visited a district keeps its old map; a new game gets the new maps.
- Not changed: the procedural Block 7 floors and the Jade Den filler.

**Playtest checkpoint:** the user plays the three districts.

## P8: Economy and city life

### Procedural gigs (`src/story.c`, `dat/story.lua`)

**Templates**
- A `gigs = { ... }` section in story.lua is read into C at load (the Lua state is closed afterwards).
- Each template has a type, a title and goal with `%T` (target) and `%P` (place), stages that refer to `$target` and `$item`, and a reward formula.
- Types:
  - **courier:** carry a package to a recipient in another district;
  - **bounty:** kill a named ganger on a random floor of a site;
  - **retrieval:** bring back a data shard from a site floor.

**Fixed slots**
- At load, add K=8 inactive quests of kind "gig", 2K NPC slots and K item slots.
- The arrays are never reallocated: `st_finish_stage` and `story_chat` hold pointers into them across effects (`story.c:1416`, `1568`).
- Instances get ever-increasing ids (`gig17`, `gig17_t`, `gig17_i`); the counter is saved.
- A slot is reused only after the quest that held it ended on an earlier turn.

**Saving**
- A `gig <id> <template> <fixer> <pm> <dnum> <dlevel> <reward> <name>` line is saved before the quest and NPC lines.
- On restore the gig is rebuilt from its template, then its state is applied.
- Fix the existing mismatch: lines are saved at up to BUFSZ*4+64 characters, but restore panics above 1023 (`restore.c:1435`).

**Offering gigs**
- Each fixer's dialogue gets a "Got any work?" choice. It offers up to 3 generated gigs, in districts that are open, with rewards scaled by danger.
- Mags is the first fixer; act 2 adds faction fixers, whose gigs also move reputation.

**Where targets and items appear**
- Courier recipients spawn next to the destination district's metro, found the same way as in `city_place_arrival`.
- Bounty targets use the existing spawn-on-arrival.
- Retrieval items are spawned in `story_arrived`, on a floor cell outside shops. Not by migration, which drops items in the arrival region (`teleport.c:2137`).
- A holo-call from the fixer arrives when a gig is done.

### District monsters
- Hook the random branch of `makemon` (`makemon.c:1312`) with `city_rndmonst()`. Not `rndmonst_adj`: that also picks corpse and statue types.
- Weighted tables per district come from a story.lua `districts` section. Sites use their own table or their parent district's.
- Use `level_difficulty()` and `monmax_difficulty` (`monst.h:264`); skip G_GONE and G_UNIQ; return 0 to fall back to normal generation.

### Civilians
- New monster entries after "cyberpsycho" (`monsters.h` ~2921), all G_NOGEN, M2_PEACEFUL, M2_HUMAN: e.g. a citizen, a street vendor and a salaryman.
- Their gear goes in `makemon.c`.
- Bump EDITLEVEL (`patchlevel.h:20`): old saves won't load.
- Spawn them with a population cap per district from `maybe_generate_rnd_mon` (`allmain.c:166`).
- They swap places with you, so they don't plug alleys (`hack.c:2154`).
- Killing one adds Heat and carries the normal penalties for killing a peaceful human.

### Ambience
- `city_ambience()` runs next to `dosounds` (`allmain.c:360`):
  - only in districts;
  - about 1 turn in 150, with a cooldown;
  - sound lines are skipped when you're deaf and sight lines when you're blind.
- The lines come from the story.lua `districts` section: holo-ads, sirens, rain on neon.

**Playtest checkpoint:** earn credits through gigs, and check that the districts feel alive.

## P9: Acts 2–4

- **Five districts** drawn with the P7 tool: Rustwater Docks, The Fringe, Chrome Heights, Ashfall, Zenith.
- **Their sites:** freighter *Mara Sun*, storm drains, the Boneyard, Halcyon tower, Seal Vault, Zenith Spire. Each has fill floors and one hand-made key floor.
- **Story:** act 2 has three shard lines (deal, force or theft) with faction choices. Act 3 is Ashfall, the Seal Vault and the daemon's offer. Act 4 is the Zenith Spire.
- **Factions:** reputation sets peacefulness in `peace_minded` (monster types mapped to factions: yakuza → jade, corpsec → halcyon, nomads → longroad), and faction fixers offer gigs.
- **Endings:** three of them, plus walk away (the game ends as a story win) or stay (keep playing, with the Undercity and Amulet still open).

## P10: Hardening and documentation

- Balance pass with an XL1–XL8 character through every district.
- Long fuzzer runs, and save/restore across everything.
- Docs: `README.neonhack2.md`, `dat/help`, `doc/story-format.md` (gigs and districts), memory file.

## Verification

- **Every layout change:** `tools/neon2/district.py --check` passes, and each district gets a visual check in tmux with `tools/nhdrive`.
- **Story:** `#wizstorycheck` reports 0 problems.
- **Each phase:**
  - `tools/neon2/slice-test` passes, including save/restore;
  - `tools/fuzz` runs with 3 roles for 120 s each and leaves no paniclog lines.
- **P7:**
  - Shops greet you, and clinics sign you up for respawn.
  - Pools are lit and alleys dark (check the remembered-floor glyphs).
  - The metro is reachable in every variant.
- **P8:**
  - A new `tools/neon2/gig-test` accepts a gig of each type, finishes it, saves and restores in the middle, and checks the payout.
  - District spawn counts are sampled with `#wizstory mons`.
