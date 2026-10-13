#!/usr/bin/env python3
# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
"""Apply NeonHack monster renames.

  rename_monsters.py apply     rewrite include/monsters.h and dat/*.lua
  rename_monsters.py report    list remaining old names in C sources
  rename_monsters.py aliases   print C alt_spl lines for vanilla names

Monster entries are identified by their PM_ enum name, so running this
again after upstream merges is safe.
"""
import glob, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from monster_names import RENAMES

TOP = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MONH = os.path.join(TOP, "include", "monsters.h")
SLOTS = ("MALE", "FEMALE", "NEUTRAL")

def mon_entries(text):
    """yield (start, end, enum, name_expr_start, name_expr_end, names)"""
    for m in re.finditer(r'\bMON\(', text):
        i = m.end()
        depth = 1
        j = i
        while depth:
            c = text[j]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            j += 1
        body = text[i:j - 1]
        enum = body.rsplit(',', 1)[1].strip()
        nm = re.match(r'\s*(NAMS?\((?:\s*"[^"]*"\s*,?)+\))', body)
        if not nm:
            continue
        expr = nm.group(1)
        names = re.findall(r'"([^"]*)"', expr)
        yield (i + nm.start(1), i + nm.end(1), enum, names)

def old_new_pairs(text):
    pairs = []
    for s, e, enum, names in mon_entries(text):
        if enum not in RENAMES:
            continue
        new = RENAMES[enum]
        new = [new] if isinstance(new, str) else list(new)
        if len(names) == 1 and len(new) == 1:
            pairs.append((names[0], new[0], enum, "NEUTRAL"))
        elif len(names) == 3 and len(new) == 3:
            for slot, o, n in zip(SLOTS, names, new):
                pairs.append((o, n, enum, slot))
        else:
            sys.exit("arity mismatch for %s: %r -> %r" % (enum, names, new))
    return pairs

def apply():
    text = open(MONH).read()
    if "NeonHack monster names" not in text:
        pass
    out, last = [], 0
    seen = set()
    for s, e, enum, names in mon_entries(text):
        if enum not in RENAMES:
            continue
        seen.add(enum)
        new = RENAMES[enum]
        if isinstance(new, str):
            expr = 'NAM("%s")' % new
        else:
            expr = 'NAMS("%s", "%s", "%s")' % new
        out.append(text[last:s])
        out.append(expr)
        last = e
    out.append(text[last:])
    missing = set(RENAMES) - seen
    if missing:
        sys.exit("unknown monster enums: %s" % sorted(missing))
    vanilla = text
    newtext = "".join(out)
    pairs = old_new_pairs(vanilla) if vanilla != newtext else None
    open(MONH, "w").write(newtext)
    return pairs

def lua_apply(pairs):
    changed = {}
    for path in sorted(glob.glob(os.path.join(TOP, "dat", "*.lua"))):
        # dungeon.lua names levels ("rogue"), not monsters
        if os.path.basename(path) == "dungeon.lua":
            continue
        s = open(path).read()
        orig = s
        for old, new, enum, slot in pairs:
            if old == new:
                continue
            s = s.replace('"%s"' % old, '"%s"' % new)
        if s != orig:
            open(path, "w").write(s)
            changed[os.path.basename(path)] = sum(
                orig.count('"%s"' % o) for o, n, _, _ in pairs if o != n)
    return changed

def report(pairs):
    files = sorted(glob.glob(os.path.join(TOP, "src", "*.c"))
                   + glob.glob(os.path.join(TOP, "include", "*.h")))
    for old, new, enum, slot in sorted(pairs, key=lambda p: -len(p[0])):
        if old == new:
            continue
        pat = re.compile(r'"[^"\n]*\b%s\b[^"\n]*"' % re.escape(old), re.I)
        for f in files:
            if f.endswith("monsters.h"):
                continue
            for n, line in enumerate(open(f, errors="replace"), 1):
                if pat.search(line):
                    print("%s:%d: [%s -> %s] %s" % (os.path.relpath(f, TOP), n,
                          old, new, line.strip()))

def alias_block(pairs):
    """C lines for mondata.c's alt_spl table: every NetHack name of a
    renamed monster, plus protective entries for current names that
    start with such an alias followed by a space (matching is by
    whole-word prefix, first match wins)"""
    # ignore obsolete entries kept under '#if 0'
    cur = re.sub(r'#if 0[\s\S]*?#endif', '', open(MONH).read())
    allnames = []
    for s, e, enum, names in mon_entries(cur):
        if len(names) == 1:
            allnames.append((names[0], enum, "NEUTRAL"))
        else:
            allnames.extend((n, enum, slot) for slot, n in zip(SLOTS, names))
    aliases = {}
    for old, new, enum, slot in pairs:
        if old.lower() != new.lower():
            aliases[old.lower()] = (enum, slot)
    protect = {}
    for a in aliases:
        for n, enum, slot in allnames:
            if n.lower().startswith(a + " ") and n.lower() not in aliases:
                protect[n.lower()] = (enum, slot)
    entries = sorted(list(aliases.items()) + list(protect.items()),
                     key=lambda kv: (-len(kv[0]), kv[0]))
    out = [BEGIN + "\n",
           "            /* NetHack names of renamed monsters (longest first,\n",
           "               since matching is by whole-word prefix) */\n"]
    for name, (enum, slot) in entries:
        tag = "" if name in aliases else "  /* not the alias */"
        out.append('            { "%s", PM_%s, %s },%s\n' % (name, enum, slot, tag))
    out.append(END + "\n")
    return "".join(out)

BEGIN = "            /* BEGIN NeonHack monster aliases (rename_monsters.py) */"
END = "            /* END NeonHack monster aliases */"

def update_aliases(pairs):
    p = os.path.join(TOP, "src", "mondata.c")
    s = open(p).read()
    b, e = s.index(BEGIN), s.index(END) + len(END) + 1
    open(p, "w").write(s[:b] + alias_block(pairs) + s[e:])


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "report"
    if cmd == "apply":
        import subprocess
        vanilla = subprocess.run(["git", "-C", TOP, "show",
                                  "NetHack-5.0.0_Released:include/monsters.h"],
                                 capture_output=True, text=True).stdout
        pairs = old_new_pairs(vanilla)
        apply()
        print("lua files changed:", lua_apply(pairs))
        update_aliases(pairs)
    elif cmd in ("report", "aliases", "pairs"):
        import subprocess
        vanilla = subprocess.run(["git", "-C", TOP, "show",
                                  "NetHack-5.0.0_Released:include/monsters.h"],
                                 capture_output=True, text=True).stdout
        pairs = old_new_pairs(vanilla)
        if cmd == "report":
            report(pairs)
        elif cmd == "pairs":
            for p in pairs:
                print("\t".join(p))
        else:
            print(alias_block(pairs), end="")

