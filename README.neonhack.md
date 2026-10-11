# NeonHack

A cyberpunk variant of **NetHack 5.0.0**. Same terminal map, same commands,
same deep game, set in the Undercity of a neon-lit arcology instead of a
fantasy dungeon. You still go down for the Amulet of Yendor; you just do it
with a pistol, a cyberdeck and too much chrome.

```
 You fire 3 bullets.  💥 Rat-a-tat-tat!  The 1st bullet hits the corpsec trooper.
                                                   ╭──·─────╮
                                                   │···f@···│
                                                   │·$·····<│
                                                   ╰────────╯
 Tester the Script Kiddie   St:10 Dx:16 Co:13 In:18 Wi:10 Ch:9 Neutral
 Dlvl:3 $:120 HP:17(17) Pw:12(12) AC:9 Xp:2 Wanted
```

## Build and play

```sh
sh sys/unix/setup.sh sys/unix/hints/neonhack.500
make fetch-Lua        # once; downloads Lua 5.4
make all              # builds and installs into ./playground
./playground/neonhack
```

* Interfaces: tty (default) and curses (`OPTIONS=windowtype:curses`).
* Config: `~/.neonhackrc` (an existing `~/.nethackrc` is used if there is no
  `~/.neonhackrc`), or the `NEONHACKOPTIONS` / `NETHACKOPTIONS` environment
  variables. See `neonhackrc.sample`.
* The playground build allows debug mode for the local user:
  `./playground/neonhack -D`.
* On distros that ship ncurses without its headers the
  hints file uses Homebrew's ncurses headers and links the system's
  runtime libraries; nothing extra needs to be installed.

## Controls

NetHack's keys all still work (`hjklyubn`, or digits with `number_pad`),
but you don't need them to move:

| Key | Laptop | Moves |
|---|---|---|
| Arrow keys | | west, east, north, south |
| Home | Fn+Left | northwest |
| PgUp | Fn+Up | northeast |
| End | Fn+Right | southeast |
| PgDn | Fn+Down | southwest |
| Shift + any of those | | run |
| Ctrl + any of those | | rush |
| Left click on the map | | travel there (next to you: step, open, kick) |
| Right click | | look at what's there |

Fn turns an arrow 45° clockwise. (That puts End and PgDn the other way
round from a numpad's 1 and 3; with a numpad, use `number_pad`.) The keys
work wherever NetHack asks for a direction or a map position (firing,
farlook `;`, travel `_`); in menus they turn pages.

The mouse is on by default (`mouse_support` option) in both tty and curses.
While it is on, most terminals select text with Shift+drag. Turn it off
with `OPTIONS=!mouse_support`; get NetHack's right-click menu back with
`BIND=mouse1:therecmdmenu`.

## What's different

**World.** The Dungeons of Doom are the Undercity, the Gnomish Mines are the
Old Metro (with the Metro Market and the End of the Line), and Gehennom is
where the old network's daemons still run. Shops include gun shops,
pharmacies, noodle bars and cyberware clinics.

**Roles.** Salvager, Nomad, Feral, Ripperdoc, Marshal, Monk, Techpriest,
Gunslinger, Infiltrator, Ronin, Tourist, Valkyrie and Netrunner. Races:
human, synth, cyborg, scav, mutant. NetHack names still work in options
(`role:Wizard` gives a Netrunner).

**Monsters.** Factions replace fantasy races (synths, cyborgs, scavs,
mutant gangs, ratkin), golems are robots, Kops are rent-a-cops, the army is
corporate security. New: street punks, yakuza, black hats, cyberpsychos,
security drones, spider bots, sentry bots, robo-rats, glitches, viruses,
nanite clouds, cyberzombies and black ICE.

**Items.** Potions are vials, scrolls are scripts (labeled `SUDO MAKE ME`,
`RM -RF /`...), spellbooks are spellchips, gold is credits. Bows are
pistols, arrows are bullets, crossbows are shotguns; plus submachine guns,
assault rifles, stun batons, monofilament whips, grenades, cyberdecks,
medkits and sixteen implants. NetHack names still work when wishing.

**Mechanics.**

| | |
|---|---|
| Firearms | NetHack's launcher system: wield, quiver, fire. Guns are loud (wake the level), automatics fire bursts, rounds that hit are spent, shotguns are brutal up close. |
| Grenades | Frag, flashbang and EMP, thrown with `t`. |
| Cyberware | Apply an implant for a permanent ability: speed, resistances, telepathy, warning, regeneration, AC, Str, firearm accuracy, mantis blades... Each one adds cyber load; past your limit comes cyberpsychosis (`CyPsy`). Lightning and EMPs short implants out. |
| Hacking | `#hack` / M-h / apply a cyberdeck: tame, pacify or shut down robots, open maglocks. |
| Heat | Crimes make you `Wanted`; corporate security squads come for you. |
| Conduct | "You have remained fully organic." |

**Look.** The default map is classic NetHack ASCII. `OPTIONS=symset:NeonGrid`
switches to neon box-drawing walls; the `emoji` option (on by default,
UTF-8 terminals only) decorates the new feature messages.

## For developers

* Design and plan: `NEONHACK.md`.
* Every NeonHack change is a commit on top of the `NetHack-5.0.0_Released`
  tag: `git diff NetHack-5.0.0_Released` shows the whole variant.
* New mechanics live in `src/neon.c`; the NetHack sources get small hooks.
* Renames are data-driven: `tools/neon/monster_names.py` and
  `object_names.py` (applied by `rename_monsters.py` / `rename_objects.py`),
  `roles.py`, `quest/*.py` (via `questtext.py`), `retext.py` for prose.
* `tools/nhdrive` drives the game in tmux for scripted tests;
  `tools/fuzz SECONDS ROLE` runs NetHack's fuzzer in a private playground.
* Debug-mode helpers: `#wizheat`, plus NetHack's `#wizmondiff`,
  `#wizrumorcheck`, `^W`, `^G`, `^V`.

## License

NeonHack is distributed under the NetHack General Public License (see
`dat/license`), like the game it is built on. NetHack is copyright 1985-2026
Stichting Mathematisch Centrum and M. Stephenson; see `dat/history` for the
people who made it.
