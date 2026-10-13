#!/usr/bin/env python3
# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
"""Apply NeonHack object renames to include/objects.h and report/patch
references elsewhere.

  rename_objects.py apply     rewrite objects.h names/descriptions and
                              exact quoted object names in dat/*.lua
  rename_objects.py pairs     print old -> new full names
  rename_objects.py report    list old names still quoted in C sources
"""
import glob, os, re, subprocess, sys
sys.path.insert(0, os.path.dirname(__file__))
from object_names import RENAMES

TOP = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OBJH = os.path.join(TOP, "include", "objects.h")
MACROS = ("WEAPON|PROJECTILE|BOW|ARMOR|HELM|CLOAK|SHIELD|GLOVES|BOOTS|RING|"
          "AMULET|TOOL|CONTAINER|WEPTOOL|FOOD|POTION|SCROLL|SPELL|WAND|COIN|"
          "XTRA_SCROLL_LABEL")
# class prefixes used to make full names for the "of" classes
PREFIX = {"RING": "ring of ", "POTION": "potion of ", "SCROLL": "scroll of ",
          "SPELL": "spellbook of ", "WAND": "wand of "}

def entries(text):
    """yield (macro, enum, [(start, end, value)] for first two args)"""
    for m in re.finditer(r'(?m)^\s*(%s)\s*\(' % MACROS, text):
        macro = m.group(1)
        i = m.end()
        depth, j, args, astart = 1, i, [], i
        while depth:
            c = text[j]
            if c == '"':
                j = text.index('"', j + 1)
            elif c == '/' and text[j + 1] == '*':
                j = text.index('*/', j) + 1
            elif c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    args.append((astart, j))
            elif c == ',' and depth == 1:
                args.append((astart, j))
                astart = j + 1
            j += 1
        if macro == "XTRA_SCROLL_LABEL":
            # (text, sn): text is the description; name is NoDes
            fields = [None, args[0]]
        elif macro in ("FOOD", "COIN"):
            fields = [args[0], None]
        else:
            fields = [args[0], args[1]]
        enum = re.sub(r'/\*.*?\*/', '', text[args[-1][0]:args[-1][1]],
                      flags=re.S).strip()
        yield macro, enum, fields

def value(text, span):
    if span is None:
        return None
    v = text[span[0]:span[1]].strip()
    mm = re.match(r'"([^"]*)"$', v)
    return mm.group(1) if mm else None

def full_name(macro, name):
    if name is None:
        return None
    return PREFIX.get(macro, "") + name

def apply():
    text = open(OBJH).read()
    edits, seen = [], set()
    for macro, enum, fields in entries(text):
        if enum not in RENAMES:
            continue
        seen.add(enum)
        newname, newdesc = RENAMES[enum]
        for span, new in zip(fields, (newname, newdesc)):
            if new is None:
                continue
            if span is None:
                sys.exit("%s has no slot for %r" % (enum, new))
            old = text[span[0]:span[1]]
            lead = old[:len(old) - len(old.lstrip())]
            edits.append((span[0], span[1], lead + '"%s"' % new))
    missing = set(RENAMES) - seen
    if missing:
        sys.exit("unknown object enums: %s" % sorted(missing))
    for s, e, rep in sorted(edits, reverse=True):
        text = text[:s] + rep + text[e:]
    open(OBJH, "w").write(text)

def pairs():
    vanilla = subprocess.run(["git", "-C", TOP, "show",
                              "NetHack-5.0.0_Released:include/objects.h"],
                             capture_output=True, text=True).stdout
    out = []
    for macro, enum, fields in entries(vanilla):
        if enum not in RENAMES:
            continue
        newname, newdesc = RENAMES[enum]
        oname, odesc = value(vanilla, fields[0]), value(vanilla, fields[1])
        if newname and oname:
            out.append(("name", full_name(macro, oname),
                        full_name(macro, newname), enum))
        if newdesc and odesc:
            out.append(("desc", odesc, newdesc, enum))
    return out

def lua_apply(prs):
    changed = {}
    names = [(o, n) for kind, o, n, e in prs if kind == "name"]
    # bare names too, for the "of" classes, e.g. des.object("ring", ...)?
    for path in sorted(glob.glob(os.path.join(TOP, "dat", "*.lua"))):
        s = open(path).read()
        orig = s
        lines = s.split("\n")
        for i, line in enumerate(lines):
            # trap types share some words with objects ("arrow")
            if "trap" in line:
                continue
            for o, n in names:
                line = line.replace('"%s"' % o, '"%s"' % n)
            lines[i] = line
        s = "\n".join(lines)
        if s != orig:
            open(path, "w").write(s)
            changed[os.path.basename(path)] = sum(orig.count('"%s"' % o)
                                                  for o, n in names)
    return changed

def report(prs):
    files = sorted(glob.glob(os.path.join(TOP, "src", "*.c"))
                   + glob.glob(os.path.join(TOP, "include", "*.h")))
    for kind, old, new, enum in sorted(prs, key=lambda p: -len(p[1])):
        pat = re.compile(r'"[^"\n]*\b%s\b[^"\n]*"' % re.escape(old))
        for f in files:
            if f.endswith("objects.h"):
                continue
            for n, line in enumerate(open(f, errors="replace"), 1):
                if pat.search(line):
                    print("%s:%d: [%s %s -> %s] %s" % (
                        os.path.relpath(f, TOP), n, kind, old, new,
                        line.strip()))

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "report"
    prs = pairs()
    if cmd == "apply":
        apply()
        print("lua files changed:", lua_apply(prs))
    elif cmd == "pairs":
        for p in prs:
            print("\t".join(p))
    else:
        report(prs)
