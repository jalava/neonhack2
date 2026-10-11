#!/usr/bin/env python3
"""List C string literals (outside comments) that contain a word.

  find_strings.py WORDREGEX FILE...
"""
import re, sys

def literals(src):
    """yield (lineno, literal_text) for string literals not in comments"""
    i, n, line = 0, len(src), 1
    while i < n:
        c = src[i]
        if c == '\n':
            line += 1; i += 1
        elif src.startswith('/*', i):
            j = src.index('*/', i) + 2
            line += src.count('\n', i, j); i = j
        elif src.startswith('//', i):
            j = src.find('\n', i); i = n if j < 0 else j
        elif c == "'":
            j = i + 1
            while src[j] != "'":
                j += 2 if src[j] == '\\' else 1
            i = j + 1
        elif c == '"':
            j = i + 1
            while src[j] != '"':
                j += 2 if src[j] == '\\' else 1
            yield line, src[i:j + 1]
            line += src.count('\n', i, j); i = j + 1
        else:
            i += 1

if __name__ == "__main__":
    pat = re.compile(sys.argv[1])
    for f in sys.argv[2:]:
        src = open(f, errors="replace").read()
        for ln, lit in literals(src):
            if pat.search(lit):
                print("%s:%d: %s" % (f, ln, lit))
