import multiprocessing
import threading
import time
import math

# UID 64-bit
# +-----------+-----------+-----------+-------------+
# | TIMESTAMP | WORKER ID | THREAD ID | SEQENCE NUM |
# +-----------+-----------+-----------+-------------+

# Stałe
EPOCH = 1735689600  # Początek epoki: 1 stycznia 2025 (w sekundach)
TIMESTAMP_BITS = 24
WORKER_ID_BITS = 12
THREAD_ID_BITS = 8
SEQUENCE_BITS = 20
MAX_SEQUENCE = (1 << SEQUENCE_BITS) - 1

class UIDGenerator:
    def __init__(self, worker_id):
        self.worker_id = worker_id
        self.lock = threading.Lock()
        self.last_timestamp = -1
        self.sequence = 0

    def _current_timestamp(self):
        return int(time.time()) - EPOCH

    def _wait_for_next_time_unit(self, last_timestamp):
        timestamp = self._current_timestamp()
        while timestamp <= last_timestamp:
            timestamp = self._current_timestamp()
        return timestamp

    def generate_uid(self, thread_id):
        with self.lock:
            timestamp = self._current_timestamp()

            if timestamp < self.last_timestamp:
                raise Exception("Clock moved backwards. Refusing to generate UID.")

            if timestamp != self.last_timestamp:
                self.sequence = 0

            # if timestamp == self.last_timestamp:
            #     self.sequence = (self.sequence + 1) & MAX_SEQUENCE
            #     if self.sequence == 0:
            #         timestamp = self._wait_for_next_time_unit(self.last_timestamp)
            # else:
            #     self.sequence = 0

            self.last_timestamp = timestamp

            # Struktura UID: [timestamp (24b)] [worker_id (10b)] [thread_id (10b)] [sequence (20b)]
            uid = ((timestamp & ((1 << TIMESTAMP_BITS) - 1)) << (WORKER_ID_BITS + THREAD_ID_BITS + SEQUENCE_BITS)) |\
                  ((self.worker_id & ((1 << WORKER_ID_BITS) - 1)) << (THREAD_ID_BITS + SEQUENCE_BITS)) |\
                  ((thread_id & ((1 << THREAD_ID_BITS) - 1)) << SEQUENCE_BITS) |\
                  (self.sequence & ((1 << SEQUENCE_BITS) - 1))

            return uid

# Funkcja wykonywana przez wątki
def thread_task(generator, thread_id, num_uids):
    for _ in range(num_uids):
        uid = generator.generate_uid(thread_id)
        # print(uid)

# Funkcja wykonywana przez procesy (pracowników)
def worker_task(worker_id, num_threads, num_uids):
    # print(f"Worker {worker_id} started")
    generator = UIDGenerator(worker_id)

    threads = []
    for thread_id in range(num_threads):
        t = threading.Thread(target=thread_task, args=(generator, thread_id, num_uids))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    # print(f"Worker {worker_id} finished")


# Główny program (Dozorca)
def main():

    total_uids = 10000000.0
    num_workers = (1<<12) - 1  # Liczba pracowników (procesów)
    num_threads = 1  # Liczba wątków na pracownika
    num_uids = math.ceil(total_uids / (num_workers * num_threads)) # Liczba UID-ów generowanych przez każdy wątek
    print(f"Workers: {num_workers}")
    print(f"Threads per Worker: {num_threads}")
    print(f"num_uids: {num_uids}")

    # num_workers = 16  # Liczba pracowników (procesów)
    # num_threads = 16  # Liczba wątków na pracownika
    # num_uids = 39063     # Liczba UID-ów generowanych przez każdy wątek

    processes = []

    time_start = time.time() * 1000
    for worker_id in range(num_workers):
        p = multiprocessing.Process(target=worker_task, args=(worker_id, num_threads, num_uids))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()
    time_end = time.time() * 1000

    print(f"Total time: {time_end - time_start:.2f} ms")

if __name__ == "__main__":
    main()
