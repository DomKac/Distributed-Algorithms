#!/usr/bin/env python3
import sys
from collections import defaultdict

A_id = None
B_id = None
A = {}
B = {}

max_col_A, max_row_A = 0, 0
max_col_B, max_row_B = 0, 0

# Zakładam że wykonujemy mnożenie A * B

for line in sys.stdin:
    # key is number of row
    key, values = line.strip().split("\t")
    row, col, val = values.split(',')
    
    row = int(row)
    col = int(col)
    val = float(val)

    if A_id == None:
        A_id = key
    elif key != A_id:
        B_id = key

    if key == A_id:
        A[(row, col)] = val
        max_col_A = max(max_col_A, col)
        max_row_A = max(max_row_A, row)
    elif key == B_id:
        B[(row, col)] = val
        max_col_B = max(max_col_B, col)
        max_row_B = max(max_row_B, row)


if max_col_A != max_row_B:
    print("Wrong matrices sizes")
else:
    M = [[0 for _ in range(max_row_A + 1)] for _ in range(max_col_B + 1)]

    for row in range(max_row_A + 1):
        for col in range(max_col_B + 1):
            for k in range(max_col_A + 1):
                M[row][col] += A[(row, k)] * B[(k, col)]
            print(f"{M[row][col]}", end=' ')
        print()
    

#     A
# | 1 2 3 |     *     = | 22 28 | = | 1*1+2*3+3*5 1*2+2*4+3*6 |
# | 4 5 6 |             | 49 64 |   | 4*1+5*3+6*5 4*2+5*4+6*6 |
        #   | 1 2 |
        #   | 3 4 |
        #   | 5 6 |
        #      B

# | 1 2 3 |     *     = | 22 28 | = | 1*1+2*3+3*5 1*2+2*4+3*6 |
# | 4 5 6 |             | 49 64 |   | 4*1+5*3+6*5 4*2+5*4+6*6 |
        #   | 1 2 3 |
        #   | 4 5 6 |
        #   | 7 8 9 |
