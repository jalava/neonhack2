# NeonHack 2: Neo Yendor

Status (2026-10-12): phases P0-P8 done (P7 and P8 are planned in
doc/neonhack2-next-phases.md): the slice is playable in organic districts,
with gigs and city life.  Next: user playtest, then P9 (acts 2-4).

## Context

NeonHack (NetHack 5.0 + a cyberpunk layer) plays well, but it
is still NetHack's single dungeon descent. The user wants a sequel that
stands out: an open city, a quest journal, and a story that sends the player
from location to location. It must keep the familiar rendering (NeonGrid,
tty/curses, 24-bit colour) and keymap (arrows, Fn diagonals, mouse travel).

User decisions (2026-10-11):
- A new tree and branch, **neonhack2**.
- **District hubs + city map**: each district is one 80x21 level; travel
  between districts by metro or taxi from a city map.
- **Death mode chosen at character creation**: story mode (wake at the
  last clinic) or hardcore (one life).
- **City + deep Undercity**: all of NeonHack 1 stays under the city as the
  optional endgame; ascension remains the ultimate ending.
- Original setting: the city **Neo Yendor**, the sprawl grown over NeonHack
  1's sealed arcology Yendor (2188, a year after NeonHack 1).
- Story premise **The Splinter**, with **factions and 3 endings**.

Approach: fork NeonHack and keep the whole engine (monsters, items,
firearms, cyberware, Heat, NeonGrid, controls). Add a city layer on top:
district levels, building "sites", travel, and a story engine written in C
that reads declarative story data from `dat/story.lua`.

## Game design

### Districts
Each district is a hand-made 80x21 Lua map with `noflip`, like Minetown.
It has streets, shops, a clinic, a bar, a metro station, NPCs and the
lobbies of its sites. Each site is a building interior: its floors are a
separate dungeon (procedural rooms with themed rooms, plus one hand-made
key floor), reached by stairs or an elevator in the lobby.

| District | Feel | Danger | Sites (floors) | Unlocked |
|---|---|---|---|---|
| Neon Mile | nightlife strip, start, safehouse, Static Lounge | 1 | (none) | start |
| Lantern Market | night market, ripperdocs; Jade Circuit turf | 2 | Jade Circuit den (3) | prologue |
| The Stacks | megablocks, gang wars | 3 | Block 7 (4) | act 1 |
| Rustwater Docks | warehouses, smugglers | 4 | freighter *Mara Sun* (3), storm drains (3) | act 2 |
| The Fringe | city edge, nomad camp | 4 | the Boneyard (2) | act 2 |
| Chrome Heights | corporate plaza, corpsec everywhere | 6 | Halcyon tower (6) | act 2 |
| Ashfall | quake-ruined old downtown | 5 | Seal Vault (2); the Shaft to the Undercity | act 3 |
| Zenith | corporate island, finale | 8 | Zenith Spire (8) | act 4 |

The Undercity (all of NeonHack 1: Old Metro, Sokoban, Quest, Gehennom,
Planes) is reached through the Shaft in Ashfall.

### Travel and keys (existing keymap unchanged)
- **Metro**: every district has a station, drawn as a yellow branch `>`.
  Pressing `>` on it opens the city map; pick a district and you arrive at
  its station.
- **`M` / `#citymap`**: city map anywhere. It shows box-drawing art of Neo
  Yendor, then a list of districts with markers for "you are here", the
  tracked objective, other objectives and locked districts. On a street you
  can call a **taxi** (costs credits; refused while Wanted or with hostiles
  next to you).
- **`Tab` / `#journal`**: the journal. `#wizidentify` moves off `^I`.
- Story locks close the other ways between districts: level teleport,
  digging down, falling, and rising through the ceiling.

### Factions
Reputation runs from -100 to +100. It decides whether faction members are
peaceful (hook in `peace_minded`) and which quest options you get.

| Faction | Who | Home |
|---|---|---|
| **Halcyon Dynamics** | the corp that owns the Grid's backbone | Chrome Heights, Zenith |
| **Jade Circuit** | street syndicate, run by Matron Lin | Lantern Market, The Stacks |
| **The Long Road** | nomad clans, led by Old Juno | The Fringe, Rustwater Docks |

### Story: The Splinter (draft for review; names editable)
Background: the Amulet of Yendor at the bottom of the Undercity holds the
Grid's root keys (NeonHack 1). Decades ago Halcyon cut a copy and split it
into four shards ("splinters") for safety. Together the shards give control
of Neo Yendor's Grid. One shard is now in your head.

| Act | Where | Beats |
|---|---|---|
| Prologue "Cold Start" | Neon Mile | You wake in Doc Okoye's clinic (your first respawn point) with the Splinter implanted and no memory of how. Mags Vance, the fixer at the Static Lounge, offers protection in exchange for work. You get the safehouse key and starter gigs. |
| 1 "Street Cred" | Neon Mile, Lantern Market, The Stacks | Pay off the Doc. Find a netrunner who can read the shard: Hex, under siege in Block 7. Climb Block 7 (4F). Hex reveals what the Splinter is and that the corp, the gang and the nomads each hold one of the other three shards. |
| 2 "Three Shards" | Lantern Market, Docks, Fringe, Chrome Heights | Three quest lines in any order. Each shard can be won by a deal, by force or by theft, and each choice moves reputation. Jade Circuit: do Matron Lin a job on the *Mara Sun*, or take her den. Long Road: recover a stolen convoy rig from the storm drains, or sell the nomads out to Halcyon. Halcyon: steal the shard from Halcyon tower (Heat!), or take Halcyon's offer. |
| 3 "Ashfall" | Ashfall | The shards resonate and the Shaft opens. Whichever faction you've angered most grabs Mags or Hex. You enter the Seal Vault (2F) and learn the full truth; a voice from the Undercity daemons (NeonHack 1's Wizard) offers a third way. |
| 4 "Zenith" | Zenith | Assault on the Zenith Spire (8F). Allies who show up depend on reputation. At the root console you choose an ending. |

Endings:
1. **Corporate order**: give the keys to Halcyon.
2. **Street crown**: give them to the Jade Circuit.
3. **Open Grid**: with the Long Road, broadcast the keys to everyone.

After the ending you choose to walk away (the game ends, recorded as a story
win) or stay. If you stay, the Undercity and the Amulet are still there to
go for.

Between story beats, fixers (Mags, then one per faction) offer
**procedural gigs** for credits and reputation: fetch a shard from site X
floor N, flatline target Y, deliver a package to Z, or hack a terminal.

### Journal
- Quests are grouped Main / Gigs / Side / Done. Each line shows the current
  objective and its district. Picking a quest shows its log (entries stamped
  with the turn) and lets you set it as the tracked quest.
- The tracked objective is printed when you arrive in a district and marked
  on the city map.
- The status line shows the place ("Neon Mile", "Block7 3F", at most 10
  characters) instead of `Dlvl:n`.
- "Holo-calls": story NPCs can reach you anywhere with a text window.

### Death
- Prompt at character creation (modelled on `ask_do_tutorial`), preset with
  `OPTIONS=hardcore` or `OPTIONS=!hardcore`.
- **Story mode**: when you flatline you wake at the last clinic or
  safehouse you used. The ripperdoc's bill takes 25% of your credits and
  Heat is reset. The flatline count appears in the ending and dumplog.
  Death is still final in the endgame (the Elemental Planes), because the
  other levels are gone by then.
- **Hardcore**: NetHack death.

## Engine work (verified against the code)

1. **Fork**
   - Fork NeonHack's `neonhack` branch as `neonhack2`, so NeonHack fixes can still be merged in.
   - Copy the hints file to `sys/unix/hints/neonhack2.500` with `GAME=neonhack2`.
   - Config file and options variable (`src/cfgfiles.c:128, 320-340, 1914`): read `~/.neonhack2rc`, falling back to `~/.neonhackrc` and then `.nethackrc`; options variable `NEONHACK2OPTIONS`.
   - `include/patchlevel.h`: `NEONHACK2`, title and version 0.1.0.
   - Disable the tutorial (`maybe_do_tutorial`, `allmain.c:568`).
   - Update `tools/fuzz` and `tools/nhdrive` for the new binary.
2. **Limits**
   - Ledger numbers: change `xint8` to `xint16` in `save.c` (savelev, the dosave0 loop, …), `restore.c` (getlev, dorecover, …), `extern.h`, `files.c` recover_savefile, `util/recover.c` and `util/sfctool.c`.
   - `MAXDUNGEON` 16 → 32 (`global.h:408`).
   - `LEV_LIMIT` 50 → 128 (`dgn_file.h:72`).
   - Single-level dungeons trigger an `impossible()` in `builds_up()` (`dungeon.c:1477`); patch it so one-floor sites work.
   - Level budget: about 84 existing levels plus about 40 new. The new levels alone would break the old 127 cap.
3. **World plumbing**
   - **City dungeons** (`dat/dungeon.lua`, `src/dungeon.c`):
     - New dungeon flag `city` (list at `dungeon.c:748`).
     - New field `danger` for each dungeon and level, stored in `struct dungeon`.
     - New field `parent` for site dungeons.
     - "Neo Yendor" becomes an unconnected dungeon; its levels are the special levels `nc-<district>.lua`. Each site is an unconnected dungeon with a `city` flag; towers use `entry=-1`.
   - **Difficulty** (`level_difficulty`, `dungeon.c:2027`): city levels use `danger` plus the number of floors from the entry, and ignore the Amulet's deepest-level rule.
   - **Exits**: `des.stair` gets a `dest` option, e.g. `dest="Block 7"` or `dest="nc-ashfall"` (`l_create_stairway`, `sp_lev.c:4161`), which calls `stairway_add(..., &dest)`.
     - A site's entry floor gets its exit back to `parent` automatically, after `place_branch` (`mklev.c:1380`) and in `fixup_special`.
     - In city dungeons, a missing back-link puts you on a random spot (`u_on_rndspot`) and reports an `impossible()`, instead of picking the wrong stair.
   - **`goto_level`** (`do.c:1479`):
     - Between city dungeons, take "up" from `u.dz` instead of from `depth()`.
     - Add an "arrival spec" (metro station or exact x,y) that the placement block (`do.c:1720`) honours.
     - Add the story arrive hook near `onquest()` (`do.c:1890`), so it fires on every arrival.
   - **Travel locks** in city dungeons: `level_tele` behaves as in Sokoban (`teleport.c:1185`); `Can_dig_down`, `Can_fall_thru` and `Can_rise_up` are false; the potion case at `potion.c:1086` is covered too.
   - **Start in Neon Mile**:
     - After `u_init_misc()` (`allmain.c:790`), set `u.uz` and `dunlev_ureached`.
     - Replace `u_on_upstairs()` at `allmain.c:811` with `u_on_rndspot` and the district's arrival region.
   - **Undercity 1 up stairs**:
     - `prev_level` (`dungeon.c:1529`) currently ends the game when you climb out without the Amulet; instead it takes you to the Ashfall Shaft.
     - With the Amulet, ask whether to climb to the Planes (no return) or to Ashfall.
     - Gate the "no return" prompt at `do.c:1330` on carrying the Amulet.
     - Fix the "out of the dungeon" wording (`stairs.c:208`).
   - **Status line**: `describe_level` (`botl.c:448`) shows the place; `stairs_description` (`stairs.c:192`) names the destination.
4. **Metro and city map** (new `src/city.c`)
   - Add a `kind` field to `struct stairway` (`stairs.h:8`). Also copy it in `rest_stairs` (`restore.c:985`), which rebuilds stairs and would otherwise drop it.
   - Draw stations as branch stairs (`known_branch_stairs`, `stairs.c:181`). Pressing `>` on one is intercepted in `dodown` (`do.c:1226`). Monsters don't use stations (`muse.c:561, 1067`).
   - The city map is an `NHW_MENU`: art lines via `add_menu_str`, then the districts.
   - Travel goes through `schedule_goto` plus the arrival spec. A taxi leaves pets behind unless they're adjacent.
5. **Story engine** (new `src/story.c`, data in `dat/story.lua`)
   - **Content**: quests made of stages, each with an objective, a district, a completion condition and effects; NPCs (monster type, name, faction, home site and spot); dialogue nodes (text, plus choices with conditions and effects); and gig templates. Text uses quest-style `%`-substitutions (`questpgr.c` `convert_arg`).
   - **Loading**: `story.lua` is parsed at new game and on restore, the same way `dungeon.lua` is read. Only state goes in the save file, in a new chunk after `save_gamelog` (`save.c:327` / `restore.c:728`, plus the free path).
     - The state is keyed by string ids: quest stages, flags, reputation, the NPC-id → `m_id` registry, pending spawns, journal lines, tracked quest, respawn point, death mode and flatline count.
   - **Conditions**: talk to an NPC, arrive at a site or floor, kill, carry or give an item, pay credits, a flag value.
   - **Effects**: journal entry, credits, item (through the wish parser), spawn or despawn an NPC, unlock a district, holo-call, set a flag, change reputation, start or finish a quest, ending.
   - **Hooks**:
     - talk: `dochat` just before `domonnoise` (`sounds.c:1408`);
     - arrive: `goto_level`;
     - kill: `m_detach` / `xkilled` (`mon.c:2739, 3482`);
     - NPC registration: an `npc=` option in `des.monster`, inside `create_monster` (`sp_lev.c:1925`). Static NPCs come from the district Lua; dynamic ones spawn on arrival.
   - **Story items**: a new object type (data shard, keycard, package) whose story id is kept in `spe`/`corpsenm`, with merging turned off. Stacks split, so `o_id` can't be used.
   - **Why C, not Lua**: in NetHack 5.0, Lua callbacks stop firing after a restore, every Lua error is a panic, saved Lua variables are fragile, and Lua can't move the hero or look up monsters.
6. **Journal**: `Tab` / `#journal` shows a menu built with `add_menu_heading`/`add_menu_str` (`windows.c:1816`); the log is a text window modelled on `show_gamelog` (`insight.c:2578`).
7. **Death modes**:
   - The creation prompt is modelled on `ask_do_tutorial` (`options.c:430`).
   - The respawn hook goes in `done()` after the life-saving block (`end.c:1081`): `savelife()`, then the bill, then `schedule_goto` to the respawn point.
   - It never applies in the endgame or to `GENOCIDED`.
8. **City life** (later phase):
   - **Pedestrians**: peaceful civilians, exempt from the "killed a peaceful human" luck rule (`mon.c:3653`) but still adding Heat.
   - **District spawn tables** in `rndmonst`.
   - **Ambient district messages** modelled on `dosounds`.
   - **Neon signs and ads** as engravings.
   - **Faction peacefulness** in `peace_minded`.

## Phases

| # | Phase | Done when |
|---|---|---|
| P0 | Fork, rename, build | `./playground/neonhack2` runs; it is NeonHack 1 under a new name |
| P1 | Limits (`xint16` ledgers, `MAXDUNGEON`, `LEV_LIMIT`, one-floor sites) | a wizard-mode game visits 100+ levels and survives save/restore; fuzzer clean |
| P2 | World plumbing, with 3 stub districts and 1 stub site | start in Neon Mile; walk in and out of the site; Shaft to Undercity 1 and back; travel locks hold |
| P3 | Metro, city map, taxi | ride between the 3 districts; markers shown; taxi blocked while Wanted |
| P4 | Story engine, journal, `#wizstory` (jump to a stage / dump state), `#wizstorycheck` (checks every id and every stair back-link) | stub quest of talk → arrive → kill → give completes and survives save/restore |
| P5 | Death modes | the edge cases below pass |
| **P6** | **Vertical slice: prologue + act 1** (Neon Mile, Lantern Market, The Stacks, Block 7) with real maps, NPCs and dialogue | **user playtest checkpoint** |
| P7 | City life: pedestrians, spawn tables, ambience, signs, holo-calls | |
| P8 | Acts 2–4: Docks, Fringe, Chrome Heights, Ashfall, Zenith; their sites; factions; 3 endings and walk-away | full story playable |
| P9 | Procedural fixer gigs | |
| P10 | Hardening and docs: `NEONHACK2.md`, `README.neonhack2.md`, `dat/help`, Guidebook notes, memory file | |

Each phase is committed separately on the local branch `neonhack2`. Nothing is
pushed.

## Verification

- **Build**: `sh sys/unix/setup.sh sys/unix/hints/neonhack2.500 && make all` (serial make). Test with `tools/nhdrive` (tmux) and `tools/fuzz SECONDS ROLE`.
- **Fuzzer**: teach the fuzzer's level teleport (`teleport.c:1176`) to reach the city and site dungeons, then run each phase with several roles.
- **Save/restore**: in wizard mode, visit every district, site and Undercity branch, save, restore, and compare. Repeat in the middle of a quest with `nhdrive`.
- **Story**: `#wizstorycheck` must pass in CI-style runs. A scripted `nhdrive` playthrough of the prologue and act 1 checks the journal and city-map text at each stage.
- **Respawn edge cases** (wizard mode): drowning, lava, stoning, sliming, choking, being engulfed, dying while punished, dying in Gehennom, dying with the Amulet, dying in the Planes (must be final), and hardcore mode (must be final).
- **Controls**: arrows, Fn diagonals, mouse travel, `Tab`, `M`, and `>` at a station, in both tty and curses, over ssh from Windows Terminal.
- **Menus under the fuzzer**: dialogue and city-map menus can't loop forever.
