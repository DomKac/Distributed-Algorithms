#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

// Stałe
#define EPOCH 1735689600 // Początek epoki: 1 stycznia 2025 (w sekundach)
#define TIMESTAMP_BITS 24
#define WORKER_ID_BITS 8
#define THREAD_ID_BITS 8
#define SEQUENCE_BITS 24
#define MAX_SEQUENCE ((1 << SEQUENCE_BITS) - 1)

// Struktura UID Generatora
typedef struct {
    uint64_t worker_id;
    uint64_t last_timestamp;
    uint64_t sequence;
    pthread_mutex_t lock;
} UIDGenerator;

typedef struct {
    uint64_t last_timestamp;
    uint64_t worker_id;
    uint64_t thread_id;
    uint64_t seq_num;
} MyUIDGenerator;

// Funkcje pomocnicze
static uint64_t current_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec - EPOCH);
}

// static long wait_for_next_time_unit(long last_timestamp) {
//     long timestamp = current_timestamp();
//     while (timestamp <= last_timestamp) {
//         timestamp = current_timestamp();
//     }
//     return timestamp;
// }

static uint64_t my_generate_uid_2 (MyUIDGenerator *generator) {
 
    uint64_t timestamp = current_timestamp();

    if (timestamp < generator->last_timestamp) {
        fprintf(stderr, "Clock moved backwards. Refusing to generate UID.\n");
        exit(1);
    }

    if (timestamp != generator->last_timestamp) {
        generator->seq_num = 0;
    }

    generator->last_timestamp = timestamp;

    uint64_t uid = ((timestamp & ((1 << TIMESTAMP_BITS) - 1)) << (WORKER_ID_BITS + THREAD_ID_BITS + SEQUENCE_BITS)) |
                   ((generator->worker_id & ((1 << WORKER_ID_BITS) - 1)) << (THREAD_ID_BITS + SEQUENCE_BITS)) |
                   ((generator->thread_id & ((1 << THREAD_ID_BITS) - 1)) << SEQUENCE_BITS) |
                   (generator->seq_num & ((1 << SEQUENCE_BITS) - 1));

    generator->seq_num += 1;
    // printf("|%zu|%zu|%zu|%zu|\t", timestamp, generator->worker_id, generator->thread_id, generator->seq_num);
    // printf("UID: %zu\n", uid);

    return uid;
}

static uint64_t my_generate_uid(uint64_t worker_id, uint64_t thread_id, uint64_t seq_num) {
    uint64_t timestamp = current_timestamp();
    uint64_t uid = ((timestamp & ((1 << TIMESTAMP_BITS) - 1)) << (WORKER_ID_BITS + THREAD_ID_BITS + SEQUENCE_BITS)) |
               ((worker_id & ((1 << WORKER_ID_BITS) - 1)) << (THREAD_ID_BITS + SEQUENCE_BITS)) |
               ((thread_id & ((1 << THREAD_ID_BITS) - 1)) << SEQUENCE_BITS) |
               (seq_num & ((1 << SEQUENCE_BITS) - 1));

    printf("|%zu|%zu|%zu|%zu|\t", timestamp, worker_id, thread_id, seq_num);
    printf("UID: %zu\n", uid);

    return uid;
}

static uint64_t generate_uid(UIDGenerator *generator, uint64_t thread_id) {

    pthread_mutex_lock(&generator->lock);

    uint64_t timestamp = current_timestamp();

    if (timestamp < generator->last_timestamp) {
        fprintf(stderr, "Clock moved backwards. Refusing to generate UID.\n");
        pthread_mutex_unlock(&generator->lock);
        exit(1);
    }

    if (timestamp != generator->last_timestamp) {
        generator->sequence = 0;
    }

    generator->last_timestamp = timestamp;

    uint64_t uid = ((timestamp & ((1 << TIMESTAMP_BITS) - 1)) << (WORKER_ID_BITS + THREAD_ID_BITS + SEQUENCE_BITS)) |
               ((generator->worker_id & ((1 << WORKER_ID_BITS) - 1)) << (THREAD_ID_BITS + SEQUENCE_BITS)) |
               ((thread_id & ((1 << THREAD_ID_BITS) - 1)) << SEQUENCE_BITS) |
               (generator->sequence & ((1 << SEQUENCE_BITS) - 1));

    generator->sequence = (generator->sequence + 1) & MAX_SEQUENCE;
    printf("|%zu|%zu|%zu|%zu|\t", timestamp, generator->worker_id, thread_id, generator->sequence);
    printf("UID: %zu\n", uid);
    pthread_mutex_unlock(&generator->lock);
    return uid;
}

// Funkcja wykonywana przez wątki
static void *thread_task(void *arg) {
    UIDGenerator *generator = ((UIDGenerator **)arg)[0];
    uint64_t thread_id = *((uint64_t *)((UIDGenerator **)arg)[1]);
    uint64_t num_uids = *((uint64_t *)((UIDGenerator **)arg)[2]);

    for (uint64_t i = 0; i < num_uids; i++) {
        uint64_t uid = generate_uid(generator, thread_id);
        // UID można wypisywać tutaj, jeśli jest potrzebne
        // printf("Worker Thread %d generated UID: %X\n", thread_id, uid);
    }

    pthread_exit(NULL);
}

static void *my_thread_task(void *arg) {

    uint64_t worker_id = ((uint64_t *)arg)[0];
    uint64_t thread_id = ((uint64_t *)arg)[1];
    uint64_t num_uids =  ((uint64_t *)arg)[2];

    for (uint64_t seq_num = 0; seq_num < num_uids; seq_num++) {
        uint64_t uid = my_generate_uid(worker_id, thread_id, seq_num);
        // UID można wypisywać tutaj, jeśli jest potrzebne
        // printf("Worker Thread %d generated UID: %X\n", thread_id, uid);
    }

    pthread_exit(NULL);
}

// Funkcja wykonywana przez wątki
static void *my_thread_task_2(void *arg) {

    uint64_t worker_id = ((uint64_t *)arg)[0];
    uint64_t thread_id = ((uint64_t *)arg)[1];
    uint64_t num_uids = ((uint64_t *)arg)[2];

    MyUIDGenerator generator = {.worker_id = worker_id, .thread_id = thread_id, .last_timestamp = 0, .seq_num = 0};

    for (uint64_t i = 0; i < num_uids; i++) {
        uint64_t uid = my_generate_uid_2(&generator);
        // UID można wypisywać tutaj, jeśli jest potrzebne
        // printf("Worker Thread %d generated UID: %X\n", thread_id, uid);
    }

    pthread_exit(NULL);
}

// Funkcja wykonywana przez procesy (pracowników)
static void worker_task(uint64_t worker_id, uint64_t num_threads, uint64_t num_uids) {
    UIDGenerator generator;
    generator.worker_id = worker_id;
    pthread_mutex_init(&generator.lock, NULL);
    generator.last_timestamp = 0;
    generator.sequence = 0;

    pthread_t threads[num_threads];
    uint64_t thread_ids[num_threads];
    for (size_t i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        void *args[] = {&generator, &thread_ids[i], &num_uids};
        pthread_create(&threads[i], NULL, thread_task, args);
    }

    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&generator.lock);
}

// Funkcja wykonywana przez procesy (pracowników)
static void my_worker_task(uint64_t worker_id, uint64_t num_threads, uint64_t num_uids) {

    pthread_t threads[num_threads];
    uint64_t thread_ids[num_threads];

    for (size_t i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        uint64_t args[] = {worker_id, thread_ids[i], num_uids};
        pthread_create(&threads[i], NULL, my_thread_task_2, args);
    }

    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

}

// Główny program (Dozorca)
int main(void) {
    uint64_t total_uids = 10000000;
    uint64_t num_workers = (1 << 6) - 1;                        // Liczba pracowników (procesów)
    uint64_t num_threads = (1 << 6) - 1;                                                // Liczba wątków na pracownika
    uint64_t num_uids = (total_uids / (num_workers * num_threads)) + 1; // Liczba UID-ów generowanych przez każdy wątek

    printf("Workers: %zu\n", num_workers);
    printf("Threads per Worker: %zu\n", num_threads);
    printf("UIDs per Thread: %zu\n", num_uids);

    pid_t pids[num_workers];

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (size_t i = 0; i < num_workers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // worker_task(i, num_threads, num_uids);
            my_worker_task(i, num_threads, num_uids);
            exit(0);
        } else {
            pids[i] = pid;
        }
    }

    for (size_t i = 0; i < num_workers; i++) {
        waitpid(pids[i], NULL, 0);
    }

    gettimeofday(&end, NULL);

    double elapsed_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    printf("Total time: %.2f ms\n", elapsed_time);

    return 0;
}
