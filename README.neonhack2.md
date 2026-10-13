# NeonHack 2: Neo Yendor

NeonHack 2 is the sequel to NeonHack: the same NetHack 5.0 engine, the same
map look (NeonGrid) and controls, but set in an open city with a story.
Neo Yendor is the sprawl that grew over NeonHack 1's sealed arcology; the
whole of NeonHack 1 is still down there, under the Shaft in Ashfall.

Status: the whole story is playable, from the prologue to the three
endings, across eight districts and eight sites, with side jobs and
endless gigs from fixers. Balance comes next. The design is in
`doc/neonhack2-plan.md` and `doc/neonhack2-next-phases.md`.

## Building and playing

On Linux (other Unix-likes may need their own hints file):

```sh
git clone https://github.com/jalava/neonhack2.git
cd neonhack2
sh sys/unix/setup.sh sys/unix/hints/neonhack2.500   # once
make fetch-lua                                      # once; downloads Lua 5.4
make all                                            # builds into ./playground
./playground/neonhack2
```

You need a C compiler, make, curl and ncurses. On image-based distros
without ncurses headers, the hints file uses
Homebrew's ncurses headers; set `NHNCURSESINC` to use others.

Settings come from `~/.neonhack2rc`. If that file doesn't exist,
`~/.neonhackrc` is used, then `~/.nethackrc`.

## New keys (everything else is as in NeonHack)

| Key | What it does |
|---|---|
| `Tab` (`#journal`) | the journal: story missions, jobs and gigs with their objectives; pick one to read its log and track it |
| `M` (`#citymap`) | the city map; on a street, call a cab (costs credits; not while Wanted) |
| `>` on a metro station | ride the metro to another district |
| `#chat` (`M-c`) | talk to people; story characters have conversations with choices, and fixers (Mags Vance, at first) offer gigs |

## The city

| District | |
|---|---|
| Neon Mile | where you wake up; Doc Okoye, Mags at the Static Lounge |
| Lantern Market | the night market; Dr. Sato, Benny Two-Tone, the Jade Den below |
| The Stacks | megablocks; Block 7 and the Chrome Saints |
| Rustwater Docks | containers and the harbour; the freighter *Mara Sun*, the storm drains |
| The Fringe | the end of the line; the Long Road's camp, the Boneyard |
| Chrome Heights | Halcyon's plaza; Halcyon Tower, Adrian Vos |
| Ashfall | quake ruins; the Seal Vault, and the Shaft down to the Undercity |
| Zenith | the corporate island; the Zenith Spire |

The story opens districts as it goes. The Undercity (all of NeonHack 1)
waits under the Shaft in Ashfall, which the story opens in act 3.

## Factions

Halcyon Dynamics, the Jade Circuit and the Long Road each remember what
you do to them. On good terms (15 or more) their people in the city are
friendly; on bad terms (-15 or less) they attack on sight. Deals, gigs
from their fixers (Benny, Rook, Vos) and killing their people move it.
Who you've angered most, and who still likes you, changes how acts 3 and
4 go. At the end you choose who gets the keys to the Grid, then whether
to walk away (the game ends, as a story win) or stay and go after the
real thing at the bottom of the Undercity.

## Gigs and the street

- **Gigs:** fixers have repeatable jobs: deliveries, bounties and retrievals, anywhere you can reach. They pay more in more dangerous places. Up to three can run at once. To drop one, pick it in the journal.
- **City life:** each district has its own crowd. Citizens, salarymen and street vendors walk the streets (killing them is murder, and corpsec takes an interest). The Stacks belong to the Chrome Saints, and Lantern Market to pickpockets and the Jade Circuit. Now and then you'll notice something going on.

## Flatlining

A new game asks for story mode or hardcore. Put `OPTIONS=hardcore` or
`OPTIONS=!hardcore` in the config file to skip the question.
- **Story mode**: when you die you wake up at the last cyberware clinic
  you walked into (at the start, Doc Okoye's), a quarter of your credits
  poorer, with no Heat.
- **Hardcore**: death is final, as in NetHack.

## For developers

- Districts are hand-drawn maps in `tools/neon2/districts/*.py`, with a
  legend for shops, NPCs, signs and random variations. Run
  `tools/neon2/district.py --check` to validate them and see where the
  lighting falls, then `tools/neon2/district.py` to regenerate
  `dat/nc-*.lua`.
- Plans: `doc/neonhack2-plan.md` (phases 0–6) and `doc/neonhack2-next-phases.md` (7–10).
- Sites (buildings and tunnels) are dungeons in `dat/dungeon.lua` with a
  `parent` district: fill floors from `dat/nc-*fill.lua` and one
  hand-drawn key floor each.
- Story: `dat/story.lua`. The format is in `doc/story-format.md`. Check it
  with `#wizstorycheck` in a debug game (`./playground/neonhack2 -D`).
- Tests: `tools/neon2/slice-test` plays the prologue and act 1 through,
  `tools/neon2/story-test` plays acts 2 to 4 both ways round (deals,
  force, theft, betrayal, two endings), and `tools/neon2/gig-test` plays
  one gig of each kind. Debug-mode games all play as "wizard" in the
  same playground, so run these one at a time;
  `tools/fuzz SECONDS ROLE` runs the fuzzer.
- In debug mode:
  - `#wizcity` opens every district;
  - `#wizstory` inspects and steers the story;
  - level teleport (`^V`) accepts `Dungeon:level`, e.g. `Block 7:1`.

## Bugs

Report NeonHack 2's bugs at https://github.com/jalava/neonhack2/issues,
not to the NetHack DevTeam: they don't make it.

## License and credits

NeonHack 2 is a modified version of NetHack 5.0.0 (upstream tag
`NetHack-5.0.0_Released`), built on NeonHack, a cyberpunk variant of
NetHack by the same author. NetHack is by the NetHack DevTeam and many others; see
`dat/history`.

It is distributed under the NetHack General Public License, in
`dat/license` (and `LICENSE`). Like NetHack, it may be freely
redistributed. Every file changed from NetHack carries a notice saying
so, and `doc/NEONHACK-CHANGES.md` lists the changes.

Lua (MIT license) and, for some ports, PDCurses are fetched at build
time; see their own licenses.

