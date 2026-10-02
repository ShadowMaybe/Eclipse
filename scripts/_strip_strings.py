#!/usr/bin/env python3
"""Remove named <string .../> entries from every strings.xml, then validate.

Line-based for surgical removal (preserves Android resource formatting,
escaping and namespace prefixes that a full XML round-trip would destroy).
Automatically backs out any edit that would leave the file malformed.
"""
import os
import sys
import xml.etree.ElementTree as ET

RES = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..',
                   'app_eclipselauncher', 'src', 'main', 'res')


def string_files(root):
    for dp, dn, fn in os.walk(root):
        for f in fn:
            if f == 'strings.xml':
                yield os.path.join(dp, f)


def is_valid(path):
    try:
        ET.parse(path)
        return True
    except Exception:
        return False


def strip(names, root=RES):
    names = set(names)
    removed = 0
    touched = 0
    for path in sorted(string_files(root)):
        with open(path, encoding='utf-8') as fh:
            lines = fh.readlines()
        keep = [l for l in lines if not any(f'name="{n}"' in l for n in names)]
        if len(keep) == len(lines):
            continue
        with open(path, 'w', encoding='utf-8') as fh:
            fh.writelines(keep)
        if not is_valid(path):
            # back out - the entry spans multiple lines, handle it manually
            with open(path, 'w', encoding='utf-8') as fh:
                fh.writelines(lines)
            print(f"  ! {os.path.relpath(path, root)}: MULTILINE entry, needs "
                  f"manual handling", file=sys.stderr)
            continue
        removed += len(lines) - len(keep)
        touched += 1
    return removed, touched


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("usage: _strip_strings.py NAME [NAME...]", file=sys.stderr)
        sys.exit(2)
    r, t = strip(sys.argv[1:])
    print(f"stripped {r} line(s) from {t} file(s): {' '.join(sys.argv[1:])}")
