#!/usr/bin/env python3
"""Replace role quest messages in dat/quest.lua.

    python3 questtext.py ROLEFILE.py

ROLEFILE.py defines ROLE (e.g. "Arc") and TEXTS, a dict mapping a message
key to either a string (replaces the key's text = [[...]] block), a list
of strings (replaces the key's list of one-liners), or a dict with
optional "text" and "synopsis" entries.
"""
import os, re, runpy, sys

TOP = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
QUEST = os.path.join(TOP, "dat", "quest.lua")

def lua_str(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

def role_span(src, role):
    m = re.search(r'\n   %s = \{\n' % role, src)
    if not m:
        sys.exit("role %s not found" % role)
    nxt = re.search(r'\n   [A-Z][a-z][a-z] = \{\n', src[m.end():])
    end = m.end() + nxt.start() if nxt else src.index('\n}', m.end())
    return m.end(), end

def key_span(block, key):
    m = re.search(r'\n      %s = \{\n' % re.escape(key), "\n" + block)
    if not m:
        sys.exit("key %s not found" % key)
    start = m.start()  # offset in "\n"+block == offset of key line in block
    # the key's block ends at the matching "\n      },"
    end = block.index("\n      },", start) + len("\n      },")
    return start, end

def apply(role, texts):
    src = open(QUEST).read()
    rs, re_ = role_span(src, role)
    block = src[rs:re_]
    for key, val in texts.items():
        ks, ke = key_span(block, key)
        kb = block[ks:ke]
        if isinstance(val, list):
            items = "".join('         %s,\n' % lua_str(v) for v in val)
            kb = "      %s = {\n%s      }," % (key, items)
        else:
            if isinstance(val, str):
                val = {"text": val}
            if "text" in val:
                if "text = [[" not in kb:
                    sys.exit("%s.%s has no text block" % (role, key))
                kb = re.sub(r'text = \[\[[\s\S]*?\]\]',
                            lambda m: 'text = [[%s]]' % val["text"].strip("\n"),
                            kb, count=1)
            if "synopsis" in val:
                kb = re.sub(r'synopsis = "(?:[^"\\]|\\.)*"',
                            lambda m: 'synopsis = %s' % lua_str(val["synopsis"]),
                            kb, count=1)
        block = block[:ks] + kb + block[ke:]
    src = src[:rs] + block + src[re_:]
    open(QUEST, "w").write(src)

if __name__ == "__main__":
    for path in sys.argv[1:]:
        ns = runpy.run_path(path)
        apply(ns["ROLE"], ns["TEXTS"])
        print("applied", path)
