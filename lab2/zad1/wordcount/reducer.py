import os
from collections import Counter

def worker_reduce(key, value, write_pipe):
    # Redukcja: suma wartości dla klucza
    
    total_count = sum(map(int, value))

    # Wysłanie wyników do potoku
    with os.fdopen(write_pipe, 'w') as pipe:
        pipe.write(f"{key}\t{total_count}")
