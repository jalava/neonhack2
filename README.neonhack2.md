# NeonHack 2: Neo Yendor

NeonHack 2 is the sequel to NeonHack: the same NetHack 5.0 engine, the same
map look (NeonGrid) and controls, but set in an open city with a story.
Neo Yendor is the sprawl that grew over NeonHack 1's sealed arcology; the
whole of NeonHack 1 is still down there, under the Shaft in Ashfall.

Status: a playable slice. The prologue and act 1 run across Neon Mile,
Lantern Market, the Stacks and the Block 7 tower, with a side job and a
gig. The other districts are placeholders, and act 2 onwards isn't
written yet. The design is in `doc/neonhack2-plan.md`.

## Building and playing

```sh
cd neonhack2
sh sys/unix/setup.sh sys/unix/hints/neonhack2.500   # once
make all
./playground/neonhack2
```

Settings come from `~/.neonhack2rc`. If that file doesn't exist,
`~/.neonhackrc` is used, then `~/.nethackrc`.

## New keys (everything else is as in NeonHack)

| Key | What it does |
|---|---|
| `Tab` (`#journal`) | the journal: story missions, jobs and gigs with their objectives; pick one to read its log and track it |
| `M` (`#citymap`) | the city map; on a street, call a cab (costs credits; not while Wanted) |
| `>` on a metro station | ride the metro to another district |
| `#chat` (`M-c`) | talk to people; story characters have conversations with choices |

## Flatlining

A new game asks for story mode or hardcore. Put `OPTIONS=hardcore` or
`OPTIONS=!hardcore` in the config file to skip the question.
- **Story mode**: when you die you wake up at the last cyberware clinic
  you walked into (at the start, Doc Okoye's), a quarter of your credits
  poorer, with no Heat.
- **Hardcore**: death is final, as in NetHack.

## For developers

- Districts: edit the layouts in `tools/neon2/districts/`, then run
  `tools/neon2/district.py` to regenerate `dat/nc-*.lua`.
- Story: `dat/story.lua`. The format is in `doc/story-format.md`. Check it
  with `#wizstorycheck` in a debug game (`./playground/neonhack2 -D`).
- Tests: `tools/neon2/slice-test` plays the slice through and checks it;
  `tools/fuzz SECONDS ROLE` runs the fuzzer.
- In debug mode:
  - `#wizcity` opens every district;
  - `#wizstory` inspects and steers the story;
  - level teleport (`^V`) accepts `Dungeon:level`, e.g. `Block 7:1`.
