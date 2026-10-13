#!/usr/bin/env python3
# Copyright (c) 2026 jalava
# NetHack may be freely redistributed.  See license for details.
"""Convert tmux capture-pane -e output (ANSI SGR) to a standalone HTML page."""
import html, re, sys

BASIC = ["#000000", "#cd3131", "#0dbc79", "#e5e510", "#2472c8", "#bc3fbc",
         "#11a8cd", "#e5e5e5"]
BRIGHT = ["#666666", "#f14c4c", "#23d18b", "#f5f543", "#3b8eea", "#d670d6",
          "#29b8db", "#ffffff"]

def xterm256(n):
    if n < 8: return BASIC[n]
    if n < 16: return BRIGHT[n - 8]
    if n < 232:
        n -= 16; r, g, b = n // 36, (n // 6) % 6, n % 6
        f = lambda v: 0 if v == 0 else 55 + 40 * v
        return "#%02x%02x%02x" % (f(r), f(g), f(b))
    v = 8 + 10 * (n - 232); return "#%02x%02x%02x" % (v, v, v)

def convert(text):
    out, fg, bg, bold, rev = [], None, None, False, False
    pos = 0
    for m in re.finditer(r'\x1b\[([0-9;]*)m', text):
        out.append((text[pos:m.start()], fg, bg, bold, rev))
        pos = m.end()
        codes = [int(c) if c else 0 for c in m.group(1).split(';')] or [0]
        i = 0
        while i < len(codes):
            c = codes[i]
            if c == 0: fg, bg, bold, rev = None, None, False, False
            elif c == 1: bold = True
            elif c == 7: rev = True
            elif c == 22: bold = False
            elif c == 27: rev = False
            elif 30 <= c <= 37: fg = BASIC[c - 30]
            elif 90 <= c <= 97: fg = BRIGHT[c - 90]
            elif 40 <= c <= 47: bg = BASIC[c - 40]
            elif c == 39: fg = None
            elif c == 49: bg = None
            elif c in (38, 48) and i + 1 < len(codes):
                if codes[i + 1] == 5:
                    col = xterm256(codes[i + 2]); i += 2
                else:
                    col = "#%02x%02x%02x" % tuple(codes[i + 2:i + 5]); i += 4
                if c == 38: fg = col
                else: bg = col
            i += 1
    out.append((text[pos:], fg, bg, bold, rev))
    parts = []
    for s, fg, bg, bold, rev in out:
        s = re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', s)
        if not s: continue
        f, b = fg or "#c8c8d0", bg
        if bold and fg in BASIC: f = BRIGHT[BASIC.index(fg)]
        if rev: f, b = (b or "#0b0b14"), f
        style = "color:%s;" % f + ("background:%s;" % b if b else "")
        parts.append('<span style="%s">%s</span>' % (style, html.escape(s)))
    return "".join(parts)

title = sys.argv[1] if len(sys.argv) > 1 else "NeonHack"
screens = sys.stdin.read().split("\f")
body = "".join('<h2>%s</h2><pre>%s</pre>' % (html.escape(s.split("\n",1)[0]), convert(s.split("\n",1)[1])) for s in screens if s.strip())
print("""<!doctype html><html><head><meta charset="utf-8"><title>%s</title>
<style>body{background:#05050a;color:#ddd;font-family:sans-serif;margin:24px}
h1{color:#ff2bd6;letter-spacing:2px} h2{color:#29d8ff;font-size:15px;font-weight:normal;margin:22px 0 6px}
pre{background:#0b0b14;border:1px solid #2a2a44;padding:12px 14px;display:inline-block;
font-family:'DejaVu Sans Mono','Menlo',monospace;font-size:14px;line-height:1.25;border-radius:6px;
box-shadow:0 0 24px rgba(41,216,255,.15)}</style></head><body><h1>%s</h1>%s</body></html>""" % (title, title, body))
