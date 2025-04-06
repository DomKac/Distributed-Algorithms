#!/usr/bin/env python3
import sys
import os

doc_id = os.getenv('mapreduce_map_input_file', 'unknown_file')
doc_id = doc_id.split('/')[-1].rstrip(".txt")

row = 0
for line in sys.stdin:
    if line.strip():
        values = line.strip().split()
        for col, val in enumerate(values):
            print(f"{doc_id}\t{row},{col},{val}")
        row += 1   
                


# | 1 2 3 |     *     = | 22 28 | = | 1*1+2*3+3*5 1*2+2*4+3*6 |
# | 4 5 6 |             | 49 64 |   | 4*1+5*3+6*5 4*2+5*4+6*6 |
        #   | 1 2 |
        #   | 3 4 |
        #   | 5 6 |
