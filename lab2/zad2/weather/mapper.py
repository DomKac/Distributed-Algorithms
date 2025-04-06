#!/usr/bin/env python3
import sys

_ = sys.stdin.readline()

for line in sys.stdin:
    if line.strip():
        date, _, _, temperature, _, _, _, _, _, _, _, _ = line.strip().split(",")
        date = date.split(" ")[0]
        year, month, day = list(map(int, date.split("-")))
        print(f"{month}\t{temperature}")
