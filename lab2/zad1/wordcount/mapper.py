import os
from collections import Counter

def worker_map(input_text, write_pipe):
    # Odczyt danych z pliku
    # with open(file_name, 'r') as f:
        # text = f.read()

    # Mapowanie: podziel na słowa i policz wystąpienia
    words = input_text.split()
    word_counts = Counter(words)

    # Wysłanie wyników do potoku
    with os.fdopen(write_pipe, 'w') as pipe:
        for word, count in word_counts.items():
            pipe.write(f"{word}\t{count}\n")
