import os
import sys
import multiprocessing
from collections import defaultdict
# from Worker import worker_map, worker_reduce
from weather.mapper import worker_map
from weather.reducer import worker_reduce
import argparse

def master(file_names, output_file):
    # Utwórz potoki komunikacyjne dla workerów
    pipes = [os.pipe() for _ in range(len(file_names))]
    workers = []

    # Inicjalizacja procesów workerów do fazy MAP
    for idx, file_name in enumerate(file_names):
        # Możemy wysyłać nazwę pliku do otwarcia w WORKER 
        # lub wczytać zawartość pliku w MASTER i przekazać tekst do WORKER
        with open(file_name, 'r') as f:
            input_text = f.read()
        read_pipe, write_pipe = pipes[idx]
        worker = multiprocessing.Process(target=worker_map, args=(input_text, write_pipe))
        workers.append(worker)
        worker.start()
        print(f"Worker {idx} (pid: {worker.pid}) will process file: {file_name}")
        os.close(write_pipe)  # Master nie potrzebuje write_pipe

    # Zbieranie wyników od workerów z fazy MAP w postaci (key, value)
    map_results = []
    for idx, (read_pipe, _) in enumerate(pipes):
        with os.fdopen(read_pipe, 'r') as pipe:
            map_results.append(pipe.read())
            print(f"Worker {idx} (pid: {workers[idx].pid}) finished processing file")
            print(f"Results: {map_results[-1]}")
        workers[idx].join()  # Czekaj na zakończenie workerów

    # Przekazanie wyników do fazy REDUCE
    reduce_input = defaultdict(list)
    for result in map_results:
        for line in result.splitlines():
            key, value = line.strip().split("\t", 1)
            print(f"Adding to reduce_input: {key} -> {value}")
            reduce_input[key].append(value)

    print("reduce_input:", reduce_input)
    # Uruchom procesy workerów do fazy REDUCE
    pipes = [os.pipe() for _ in reduce_input]
    workers = []
    for idx, (key, values) in enumerate(reduce_input.items()):
        read_pipe, write_pipe = pipes[idx]
        print(f"Starting worker for key: {key} with values: {values}")
        worker = multiprocessing.Process(target=worker_reduce, args=(key, values, write_pipe))
        workers.append(worker)
        worker.start()
        os.close(write_pipe)

    # Zbieranie wyników od workerów z fazy REDUCE
    final_results = {}
    for idx, (read_pipe, _) in enumerate(pipes):
        with os.fdopen(read_pipe, 'r') as pipe:
            key, value = pipe.read().strip().split("\t")
            final_results[key] = value
        workers[idx].join()

    # Zapis wyników do pliku
    with open(output_file, 'w') as f:
        for key, value in sorted(final_results.items()):
            f.write(f"{key}\t{value}\n")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process some files.')
    parser.add_argument('-input', required=True, help='Directory with input files')
    parser.add_argument('-output', required=True, help='Output file')

    args = parser.parse_args()
    input_directory = args.input
    output_file = args.output

    file_names = [os.path.join(input_directory, f) for f in os.listdir(input_directory) if os.path.isfile(os.path.join(input_directory, f))]
    
    # output_file = sys.argv[1]
    # file_names = sys.argv[2:]
    master(file_names, output_file)
