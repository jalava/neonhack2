# NeonGrid 2: a character set for playability (plan)

Status: proposal, not implemented.  Preview it in the terminal you play in:

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

What NetHack's extended sets get wrong today, and the proposal:

| Feature | ASCII | IBMgraphics | DECgraphics | **NeonGrid 2** | Colour |
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

## 4. Changes

All small, and most are data.

1. **The set itself**: `tools/neon/neongrid.py` holds the table above; a
   `symset` command generates the `NeonGrid` block in `dat/symbols` from
   it (`S_` lines for shapes and colours, `G_` lines for branch walls and
   altar alignments), so the preview and the game can't drift apart.  It
   replaces the current NeonGrid; its rounded magenta corners move to the
   extras (§5).
2. **Real 24-bit colour in tty**: honour `use_truecolor` (and
   `COLORTERM=truecolor` when the client sends it) instead of always
   squeezing to the 256-colour palette.  One condition in
   `win/tty/termcap.c`.
3. **IBMgraphics over ssh** (optional): when the locale is UTF-8, translate
   code page 437 to UTF-8 as tty writes it, so IBMgraphics, RogueIBM and
   friends work on any modern terminal.  About 30 lines in
   `win/tty/wintty.c`.
4. **Default**: optionally start in NeonGrid when the locale is UTF-8
   (plain ASCII otherwise, `symset:plain` to opt out).  Today the default
   is NetHack ASCII.
5. **Docs**: README, `dat/help`, `neonhackrc.sample` (recommended options:
   `symset:NeonGrid`, `use_truecolor`, `dark_room`, `lit_corridor`,
   `hilite_pile`).

## 5. Extras (NeonGrid+, only if the glyph check passes)

Outside WGL4, so only for Windows Terminal with Cascadia Mono (or another
font that has them): rounded room corners `╭╮╰╯`, dashed open doors `╌ ╎`,
diagonal beams `╱ ╲`, web `╳`, engulf and explosion edges `▔▁▏▕`.  These
ship as a second symset, `NeonGrid+`, never as the default.

## 6. Testing

* Run `neongrid.py glyphs` in each terminal that matters (Windows Terminal
  with Cascadia Mono; classic console with Consolas; Linux) before any
  glyph is final.
* A wizard-mode showcase level (Lua) with every feature, every branch's
  walls and all altar alignments, captured with `tools/nhdrive` in tty and
  curses.
* The same captures in 16-colour mode and with a colour-blindness filter
  (deuteranopia and protanopia): every pair in rule 2 must still read.
* The fuzzer, as after every change.

## 7. Questions before building

1. Which window does PowerShell open in on the Windows client: Windows Terminal (tabs
   in the title bar) or the classic console?  Which font?
2. What does `neongrid.py glyphs` show: any boxes, question marks or bars
   out of line?
3. Corridors as `░ ▒` (IBM look) or NetHack's `#`?
4. NeonGrid as the default on UTF-8 terminals, or keep ASCII as default?
