# dat/story.lua: the story format

NeonHack 2's story is data. `dat/story.lua` defines one global table,
`story`, which `src/story.c` reads at the start of every game and again when
a game is restored. Only the state goes into the save file, and it is keyed
by the ids used here. So the story can be edited between sessions, as long
as ids that are in play keep existing.

Check the file in a debug-mode game (`./neonhack2 -D`) with
`#wizstorycheck`. It reports every reference to a quest, NPC, item, faction,
dialogue node or place that doesn't exist.

## Sections

```lua
story = {
   factions = { jade = "the Jade Circuit", ... },  -- id = name
   items = { shard = { name = "Hex's data shard" }, ... },
   npcs = { id = { ... }, ... },
   quests = { { ... }, ... },                        -- in journal order
   dialogue = { id = { ... }, ... },
   gigs = { { ... }, ... },                          -- repeatable jobs
   districts = { ["nc-stacks"] = { ... }, ... },     -- city life
}
```

### Places

A place is written as one of:
- a district level, e.g. `"nc-stacks"`;
- a dungeon name, meaning its entrance floor, e.g. `"Block 7"`;
- a dungeon name and level, e.g. `"Block 7:1"` (the top floor of a tower)
  or `"The Undercity:3"`.

### NPCs

```lua
okoye = {
   name = "Doc Okoye",       -- shown as the monster's name
   monster = "ripperdoc",    -- any monster name
   female = true,
   home = "nc-neonmile",     -- where "spawn" puts it
   stay = true,              -- never wanders off
   hostile = false,          -- targets are hostile
   level = 5,                -- experience level (default: the monster's)
   faction = "jade",
   chat = "okoye_chat",      -- dialogue for small talk (optional)
   fixer = true,             -- offers gigs when there's nothing else to say
   idle = { "Doc Okoye wipes down her tools." },  -- one-liners otherwise
}
```

There are two ways an NPC gets into the world:
- A level file places it with `des.monster({ npc = "okoye", x = 8, y = 4 })`.
  The type, name and peacefulness come from story.lua.
- A `spawn` effect makes it on its home level: at once if you are there,
  otherwise when you arrive.

The story recognizes the NPC by its monster id, so it may move around.

### Quests

```lua
{ id = "act1", title = "Street Cred", kind = "main",  -- main | side | gig
  start = true,                -- active from the start of the game
  stages = { { ... }, ... },
  done = { <effects> } }       -- after the last stage
```

A stage:

```lua
{ goal = "Find Hex in Block 7.",  -- the objective shown everywhere
  where = "Block 7:1",            -- for the city map and "Objective (...)"
  log = [[Journal text written when the stage begins.]],
  start = { <effects> },          -- when the stage begins
  done = { <effects> },           -- when it is finished
  -- finished by ONE of:
  talk = "hex",                   -- talking to the NPC
  give = "shard",                 -- (with talk) only while carrying it;
                                  -- the NPC takes it
  dialogue = "hex_meet",          -- (with talk) the conversation to run
  remind = "...",                 -- (with give) said when you lack it
  arrive = "nc-stacks",           -- arriving at the place
  kill = "ratking",               -- the NPC dying
  carry = "shard",                -- having the item in your pack
  flag = "door_open",             -- the story flag becoming non-zero
}
```

A stage with none of these is finished by an `advance` effect, usually from
a dialogue choice. If an NPC you must talk to dies, the quest fails.

### Gigs

Gigs are repeatable jobs made from templates. When you talk to a fixer and no
story stage needs them, they offer up to three. You can have three running
at once, and you can drop one from the journal.

```lua
{ id = "courier", title = "Delivery to %T",
  fixers = { "mags" },          -- who offers it (all fixers if missing)
  place = "district",           -- or "site": a random floor of a site
  spot = "metro",               -- the target waits by the metro
  hostile = true,               -- the target is a hostile (bounties)
  drop = true,                  -- the item lies somewhere at the place
  target = { "citizen", ... },  -- monster types for the target
  names = { "Juno Reyes", ... },
  items = { "sealed package", ... },
  reward = 30,                  -- credits, plus 10 per point of danger
  stages = { ... },             -- as quest stages
  done = { credits = "$reward", call = "$fixer", text = [[...]] } }
```

The template's text has placeholders that are filled in when a gig is made:
- in any text, `%T` is the target's name, `%P` the place, `%I` the item and `%F` the fixer;
- as ids or numbers, `$target`, `$item`, `$place`, `$fixer`, `$fixerhome` and `$reward`.

Only places the story has opened are used.

### Districts

```lua
["nc-stacks"] = {
   monsters = { ["street punk"] = 5, ["giant rat"] = 2 },  -- random ones
   people = { citizen = 3 },                  -- civilians kept around
   sights = { "..." }, sounds = { "..." },    -- ambient messages
},
["Block 7"] = { monsters = { ... } },         -- a site, by dungeon name
```

- **`monsters`:** random monsters on the level are picked from this list, by weight, as long as they suit the difficulty. Otherwise NetHack picks as usual.
- **`people`:** civilians (citizen, salaryman, street vendor) are topped up out of sight.
- **`sights` and `sounds`:** now and then one of these is shown. Sights are skipped while you're blind and sounds while you're deaf.

### Dialogue

```lua
hex_meet = {
   text = [[Paragraphs, word-wrapped.

A blank line starts a new paragraph.  %n is your name, %r your role.]],
   choices = {
      { "Who are you?", go = "hex_who" },
      { "Here, take the money.", need = { credits = 50 },
        credits = -50, rep = { jade = 5 } },
      { "Not yet.", wait = true },  -- the talk stage isn't finished
   },
}
```

- A node without choices is just shown, and the conversation ends.
- A choice can have `need` conditions; it is hidden unless they all hold.
- Its effects run when it is picked, and `go` continues to another node.
- Escape walks away. The stage stays unfinished.

### Effects

Effects can appear in `start`, `done` and dialogue choices. They run in
this order:

| Key | Effect |
|---|---|
| `say = "text"` | a message |
| `call = "npc"`, `text = [[...]]` | a text window; with `call`, titled as a holo-call from the NPC |
| `log = "text"` | a journal entry |
| `credits = n` | give (or take, if negative) credits |
| `object = "2 blessed medkits"` | an ordinary object, by wish syntax |
| `item = "id"`, `take = "id"` | give or take a story item |
| `rep = { jade = 5 }` | change reputation (-100..100) |
| `flag = "x"`, `flags = { x = 3 }`, `unflag = "x"` | story flags |
| `spawn = "npc"`, `despawn = "npc"` | bring an NPC in, or send it away |
| `unlock = "nc-lantern"` | open a district for travel |
| `advance = "quest"` | finish that quest's current stage |
| `fail = "quest"`, `finish = "quest"`, `start = "quest"` | quest state |
| `track = "quest"` | make it the tracked quest |
| `xp = n` | experience |

### Conditions (`need`)

| Key | Holds when |
|---|---|
| `flag` / `noflag` | the flag is set / not set |
| `item` | you carry the story item |
| `credits` | you have at least that much |
| `rep = { jade = 10 }` / `norep` | reputation is at least / below |
| `active` / `done` / `notdone` | the quest's state |

## Debug-mode commands

- `#wizstory` takes one of these commands:
  - `list` shows quests, reputation, flags and NPCs;
  - `start`, `next`, `finish`, `fail` or `track` followed by a quest id;
  - `flag <name> <n>` or `rep <faction> <n>`;
  - `spawn <npc>`, `goto <npc>` (teleport next to it), `kill <npc>` or `item <id>`;
  - `tp <x> <y>` (teleport to level coordinates) or `mons` (list monsters);
  - `gig <kind>` (start a gig from the first fixer; says where) or `fetch <item>` (stand on a story item lying on this level).
- `#wizstorycheck` checks every reference in story.lua.
- `#wizcity` opens every district.
