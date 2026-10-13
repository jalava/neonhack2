# NeonHack 2: Neo Yendor

A cyberpunk variant of [NetHack](https://www.nethack.org/) 5.0 with an
open city and a story. Same terminal map, same keys, same deep game
underneath; but instead of one long descent you work the streets of Neo
Yendor, district by district, with a quest journal, fixers, factions and
three endings. All of NeonHack's Undercity (the whole NetHack dungeon,
re-themed) still waits under the Shaft in Ashfall.

```
 You fire 3 bullets.  Rat-a-tat-tat!  The 1st bullet hits the corpsec trooper.
                                                   ╭──·─────╮
                                                   │···f@···│
                                                   │·$·····<│
                                                   ╰────────╯
 Tester the Script Kiddie   St:10 Dx:16 Co:13 In:18 Wi:10 Ch:9 Neutral
 Neon Mile $:120 HP:17(17) Pw:12(12) AC:9 Xp:2 Wanted
```

- **The city:** eight hand-drawn districts (Neon Mile, Lantern Market, the
  Stacks, Rustwater Docks, the Fringe, Chrome Heights, Ashfall, Zenith),
  linked by metro and taxi, and buildings and tunnels to climb and
  descend: Block 7, the Jade Den, the freighter *Mara Sun*, the storm
  drains, the Boneyard, Halcyon Tower, the Seal Vault, the Zenith Spire.
- **The story, *The Splinter*:** you wake on a ripperdoc's table with a
  shard of the city's root keys in your head. Three factions hold the
  other shards; get them by a deal or the hard way.
- **Factions:** Halcyon Dynamics, the Jade Circuit and the Long Road
  remember what you do. Their people are friends or enemies accordingly.
- **Death:** story mode (wake up at the last clinic, poorer) or hardcore.
- **Everything from NeonHack:** firearms, cyberware, hacking, corporate
  Heat, and the NeonGrid look for UTF-8 terminals.

## Build and play (Linux)

```sh
git clone https://github.com/jalava/neonhack2.git
cd neonhack2
sh sys/unix/setup.sh sys/unix/hints/neonhack2.500
make fetch-lua
make all
./playground/neonhack2
```

More in [`README.neonhack2.md`](../README.neonhack2.md): keys, the city,
factions, story mode, and notes for developers.

## Bugs

Please report them in this repository's
[issues](https://github.com/jalava/neonhack2/issues), not to the NetHack
DevTeam. NeonHack isn't theirs.

## License

NeonHack 2 is a modified version of NetHack 5.0.0, distributed under the
[NetHack General Public License](../LICENSE). NetHack is by the NetHack
DevTeam and a great many contributors (see `dat/history`). The changes
are listed in [`doc/NEONHACK-CHANGES.md`](../doc/NEONHACK-CHANGES.md),
and every changed file says so.
