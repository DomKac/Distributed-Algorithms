import multiprocessing
import threading
import time

# Stałe
EPOCH = 1735689600  # Początek epoki: 1 stycznia 2025 (w sekundach)
TIMESTAMP_BITS = 24
WORKER_ID_BITS = 8
THREAD_ID_BITS = 8
SEQUENCE_BITS = 24
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

            if timestamp == self.last_timestamp:
                self.sequence = (self.sequence + 1) & MAX_SEQUENCE
                if self.sequence == 0:
                    timestamp = self._wait_for_next_time_unit(self.last_timestamp)
            else:
                self.sequence = 0

            self.last_timestamp = timestamp

            # Struktura UID: [timestamp (24b)] [worker_id (8b)] [thread_id (8b)] [sequence (24b)]
            uid = ((timestamp & ((1 << TIMESTAMP_BITS) - 1)) << (WORKER_ID_BITS + THREAD_ID_BITS + SEQUENCE_BITS)) |\
                  ((self.worker_id & ((1 << WORKER_ID_BITS) - 1)) << (THREAD_ID_BITS + SEQUENCE_BITS)) |\
                  ((thread_id & ((1 << THREAD_ID_BITS) - 1)) << SEQUENCE_BITS) |\
                  (self.sequence & ((1 << SEQUENCE_BITS) - 1))

            return uid


# Funkcja wykonywana przez wątki
def thread_task(generator, thread_id, num_uids):
    for _ in range(num_uids):
        uid = generator.generate_uid(thread_id)
        print(hex(uid))
        

def main():
    generator = UIDGenerator(worker_id=1)
    num_uids = 1000000
    thread = threading.Thread(target=thread_task, args=(generator, 0, num_uids))
    start_time = time.time() * 1000
    thread.start()
    thread.join()
    end_time = time.time() * 1000

    print(f"Time: {end_time - start_time :.2f} ms")

    return 0


if __name__ == "__main__":
    main()
