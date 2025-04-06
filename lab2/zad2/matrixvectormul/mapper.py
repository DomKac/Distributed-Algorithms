#!/usr/bin/env python3
import sys

i = 0
for line in sys.stdin:
    if line.strip():
        if len(line.split()) > 1:
            values = list(map(float, line.strip().split()))
            print(f"{i}\t{values}")
        else:
            value = float(line.strip())
            print(f"{i}\t{value}")
        i += 1
