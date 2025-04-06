#!/usr/bin/env python3
import sys
from collections import defaultdict

matrix_values = []
vector_value = None

for line in sys.stdin:
    # key is number of row
    key, values = line.strip().split("\t")
    try:
        vector_value = float(values)
    except ValueError:
        matrix_values = list(map(float, values.strip("[]").split(",")))

    if matrix_values and vector_value:
        print(f"{key}\t{sum(v * vector_value for v in matrix_values)}")
        matrix_values = []
        vector_value = None
