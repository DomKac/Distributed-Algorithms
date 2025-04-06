#!/usr/bin/env python3

import sys
from collections import defaultdict

# Słownik do przechowywania listy dokumentów dla każdego słowa
word_to_docs = defaultdict(set)

# Odczyt linii wejściowych
for line in sys.stdin:
    line = line.strip()
    word, doc_id = line.split('\t', 1)
    word_to_docs[word].add(doc_id)

# Emisja wyniku: słowo -> lista dokumentów
for word, docs in word_to_docs.items():
    print(f"{word}\t{','.join(sorted(docs))}")
