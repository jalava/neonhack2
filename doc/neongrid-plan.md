# NeonGrid: a character set for playability

Status: implemented (2026-10-11).  NeonGrid is the default map on UTF-8
terminals; `NeonGrid+` is opt-in.  Preview them in the terminal you play in:

```sh
tools/neon/neongrid.py preview   # sample level, 3 ways
tools/neon/neongrid.py glyphs    # font check
```

## 1. Where the game is drawn

NeonHack is meant to be playable on a Linux host from **Windows** over
PowerShell → ssh, `TERM=xterm-256color`, UTF-8.  The characters are drawn
by the Windows window PowerShell runs in, with *its* font:

| Window | VT colours | UTF-8 | 24-bit colour | Missing glyphs |
|---|---|---|---|---|
| Windows Terminal (default on Windows 11) | yes | yes | yes | taken from a fallback font |
| Classic console (conhost) | yes | yes | yes | shown as boxes |

Consequences:

* **IBMgraphics can't work over ssh as it is.**  It writes raw code page 437
  bytes (`\xb3` for `│`), which is what "extended ASCII" means on a Windows
  console.  Inside a UTF-8 ssh stream those bytes are invalid and come out
  as garbage.  The same shapes have to be sent as Unicode.
* **Glyphs must come from the terminal's own font.**  A glyph borrowed from
  a fallback font has different metrics, which is the "kerning is a bit
  off" seen when pasting emoji.  On the map that would break the grid.
  The baseline is **WGL4**, the set every Windows font has (Consolas,
  Cascadia Mono, Lucida Console).  It contains all of code page 437, so the
  IBM look is fully available.
* **24-bit colour is squeezed to 256.**  tty uses the 256-colour palette
  whenever terminfo says 256 colours (`xterm-256color` does).  NetHack has a
  `use_truecolor` option, but tty ignores it.

## 2. Design rules

1. **One cell, one font.**  Map glyphs come from WGL4 only (extras beyond it
   are optional, §5).  No emoji, no double-width or ambiguous characters.
2. **Shape carries meaning; colour reinforces it.**  Anything you must tell
   apart at a glance differs in *shape*, so it still reads in 16 colours or
   with colour blindness: water/lava, ice/floor, open/closed door,
   sink/fountain, altar/iron chain, web/amulet, grave/wall, tree/corridor,
   boulder/engraving.
3. **Keep the muscle memory that matters.**  Monster letters, object class
   symbols, `@`, `^` for traps, and `<` `>` for stairs (they are the keys
   you press) stay as in NetHack.
4. **Quiet background, loud foreground.**  Walls, floors and corridors are
   dim; dungeon features are mid-bright; monsters and objects keep
   NetHack's own colours, so they stand out.
5. **Show where you are.**  Wall colour per branch: Undercity steel cyan,
   Old Metro amber, Sokoban blue with *double* lines (puzzle rules apply),
   Gehennom red, Fort Ludios gold.
6. **Show what NetHack already tells you, without farlook.**  Altar
   alignment by colour, lit vs remembered floor, lit vs dark corridor,
   branch stairs.

## 3. The set

What NetHack's extended sets get wrong, and what NeonGrid does instead:

| Feature | ASCII | IBMgraphics | DECgraphics | **NeonGrid** | Colour |
|---|---|---|---|---|---|
| walls | `- \|` | `─│┌┼…` | `─│┌┼…` | `─│┌┼…` | per branch |
| Sokoban walls | `- \|` | single | single | `═║╔╬…` | blue |
| floor / remembered | `.` | `·` | `·` | `·` | grey / dim |
| doorway, broken door | `.` | `·` | `·` | `·` | amber |
| open door | `- \|` | `■` | `▒` | `─ │` (perpendicular to the wall, as NetHack) | amber |
| closed door | `+` (= spellchip) | `+` | `+` | `■` | amber |
| corridor dark / lit | `#` | `░ ▒` | `#` | `░ ▒` | dim / brighter |
| stairs | `< >` | `< >` | `< >` | `< >` | white, branch yellow |
| ladders | `< >` | `< >` | `≤ ≥` | `≤ ≥` | brown, branch yellow |
| fountain | `{` | `⌠` | `{` | `⌠` | blue |
| sink | `{` (= fountain) | `⌠` (= fountain) | `{` | `#` | white |
| altar | `_` (= iron chain) | `_` | `π` | `Ω` | lawful white, neutral grey, chaotic red, Moloch magenta |
| grave | `\|` (= wall) | `\|` | `\|` | `†` | white |
| throne | `\` | `\` | `\` | `\` | gold |
| tree | `#` (= corridor) | `±` | `±` | `♣` | green |
| iron bars | `#` | `≡` | `≠` | `≡` | steel |
| pool, moat, water | `}` | `≈` | `◆` | `≈` | blue |
| lava | `}` (= pool) | `≈` (= pool) | `◆` (= pool) | `▓` | orange-red |
| wall of lava | `}` | `≈` | `◆` | `█` | orange |
| ice | `.` (= floor) | `·` (= floor) | `·` (= floor) | `◦` | ice blue |
| drawbridge down / up | `. #` | `· #` | `· #` | `═ ║` / `■` | brown |
| engraving | `` ` `` (= boulder) | `ε` | `` ` `` | `§` | lavender |
| web | `"` (= amulet) | `"` | `"` | `¤` | grey |
| boulder | `` ` `` | `` ` `` | `` ` `` | `0` | grey |
| cloud / poison cloud | `#` | `#` | `#` | `▒` / `▓` | white / green |
| zap beams | `\| - \ /` | `│ ─ \ /` | `│ ─ \ /` | `│ ─ \ /` | by zap type |
| traps, monsters, objects | NetHack | NetHack | NetHack | NetHack | NetHack |

`#` for sinks is NetHack 3.6's old sink symbol; it is free again because
corridors, trees and iron bars no longer use `#`.

## 4. What was built

1. **The sets**: `tools/neon/neongrid.py` holds the table above and
   generates the `NeonGrid` and `NeonGrid+` blocks in `dat/symbols`
   (`neongrid.py install`): `S_` lines for shapes and colours, `G_` lines
   for branch walls, altar alignments and the boulder (UTF8 symsets ignore
   `S_boulder`).  The preview reads the same table, and `install` refuses
   to write a set in which any pair from rule 2 shares a character.  The
   old hand-written NeonGrid (rounded magenta corners) is gone.
2. **24-bit colour in tty**: tty now honours `use_truecolor` instead of
   always squeezing custom colours into the 256-colour palette, and
   NeonHack turns it on by default (`!use_truecolor` for terminals without
   24-bit colour).
3. **Default**: `initoptions_init()` loads NeonGrid when the locale is
   UTF-8 and `TERM` isn't the Linux console, before the config file is
   read, so `symset:` in the config (`symset:default` for ASCII) wins.
   This also replaces curses' own default ("curses", DEC line drawing).
   Emoji use the same terminal check.
4. **Docs**: README, `dat/help`, `neonhackrc.sample`, NEONHACK.md.

Not done: translating IBMgraphics to UTF-8 (NeonGrid covers the IBM
look), and honouring `COLORTERM` (24-bit is on by default instead).

## 5. Extras (NeonGrid+)

Outside WGL4, so only for fonts that have them (Cascadia Mono, the
Windows Terminal default, does): rounded room corners `╭╮╰╯`, dashed open
doors `╌ ╎`, diagonal beams `╱ ╲`, web `╳`, engulf and explosion edges
`▔▁▏▕`.  `OPTIONS=symset:NeonGrid+`.

## 6. Testing

* `neongrid.py glyphs` on a Windows client (Windows Terminal, Cascadia Mono):
  every NeonGrid and NeonGrid+ character renders in line.
* `tools/neon/showcase.lua`, loaded with `#wizloaddes` in wizard mode,
  builds a room with every feature (door states, altars of every
  alignment, liquids, ice, clouds, traps, web, engraving, boulder,
  ladders, lit and unlit floor); checked in tty with NeonGrid and
  NeonGrid+, and the 24-bit colour escapes inspected.
* Defaults: NeonGrid in tty and curses on UTF-8; ASCII with
  `symset:default`, `LANG=C` and `TERM=linux`.
* The fuzzer.

Branch wall colours are generated like the walls of the Undercity but
weren't visited in-game.

## 7. Decisions

1. Played in Windows Terminal with Cascadia Mono, over ssh.
2. The glyph check showed no missing or misaligned characters, so
   NeonGrid+ ships too.
3. Corridors: `░ ▒`, the IBM look.
4. NeonGrid is the default on UTF-8 terminals.
