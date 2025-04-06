#!/usr/bin/env python3
import os
import sys

# Pobranie nazwy pliku wejściowego (doc_id)
doc_id = os.getenv('mapreduce_map_input_file', 'unknown_file')
doc_id = doc_id.split('/')[-1]

for line in sys.stdin:
    line = line.strip()
    words = line.split()
    for word in words:
        print(f"{word}\t{doc_id}")
