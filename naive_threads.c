#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

static const int TEXT_SIZE = 1 << 20;
struct timespec start, end;

typedef struct {
    const char *text;
    const char *pattern;
    size_t p_size;
    size_t beg;    
    size_t end;    
    size_t cnt;
    pthread_mutex_t *print_mutex;
} ThreadArgs;

static void *work(void *_args)
{
    ThreadArgs *args = (ThreadArgs*)_args;
    size_t cnt = 0;

    for (size_t i = args->beg; i < args->end; ++i) {
        size_t j = 0;
        while (j < args->p_size && args->text[i + j] == args->pattern[j])
            ++j;

        if (j == args->p_size) {
            pthread_mutex_lock(args->print_mutex);
            printf("%zu\n", i);
            pthread_mutex_unlock(args->print_mutex);
            ++cnt;
        }
    }
    args->cnt = cnt;
}


double get_working_time(struct timespec *start, struct timespec *end) {
    long seconds = end->tv_sec - start->tv_sec;
    long nanoseconds = end->tv_nsec - start->tv_nsec;
    double res = seconds + nanoseconds * 1e-9;
    return res;
}

int main(int argc, char *argv[])
{
    size_t n_threads = sysconf(_SC_NPROCESSORS_ONLN);
    int opt;

    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't': {
            char *endptr = NULL;
            long parsed = strtol(optarg, &endptr, 10);
            if (*optarg == '\0' || *endptr != '\0' || parsed <= 0) {
                fprintf(stderr, "Invalid thread count: '%s'\n", optarg);
                return EXIT_FAILURE;
            }
            n_threads = (size_t)parsed;
            break;
        }
        default:
            return EXIT_FAILURE;
        }
    }

    if (optind != argc - 1) {
        perror("Error args");
        return EXIT_FAILURE;
    }

    FILE *f = fopen(argv[optind], "r");
    if (!f) { 
        perror("Error on opening file"); 
        return EXIT_FAILURE; 
    }

    char pattern[TEXT_SIZE];
    char text[TEXT_SIZE]; 

    if (!fgets(pattern, sizeof(pattern), f)) { 
        perror("fgets pattern"); 
        fclose(f); 
        return EXIT_FAILURE; 
    }
    if (!fgets(text, sizeof(text), f)) { 
        perror("fgets text");    
        fclose(f); 
        return EXIT_FAILURE; 
    }

    pattern[strcspn(pattern, "\n")] = '\0';
    text[strcspn(text, "\n")] = '\0';
    size_t p_size  = strlen(pattern);
    size_t t_size = strlen(text);

    size_t positions = 0;
    if (p_size > 0 && p_size <= t_size)
        positions = t_size - p_size + 1;

    if (!positions) {
        printf("Result: 0\n");
        fclose(f);
        return EXIT_SUCCESS;
    }

    if (n_threads > positions) 
        n_threads = positions;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(n_threads * sizeof(ThreadArgs));
    pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

    size_t div = positions / n_threads;
    size_t mod  = positions % n_threads;
    size_t cur_pos  = 0;
    for (size_t i = 0; i < n_threads; ++i) {
        size_t shift = div + (i < mod ? 1 : 0);
        thread_args[i] = (ThreadArgs){
            .text = text,
            .pattern = pattern,
            .p_size = p_size,
            .beg = cur_pos,
            .end = cur_pos + shift,
            .cnt = 0,
            .print_mutex = &print_mutex,
        };
        cur_pos += shift;
        pthread_create(&threads[i], NULL, work, &thread_args[i]);
    }

    size_t total_found = 0;
    for (size_t i = 0; i < n_threads; ++i) {
        pthread_join(threads[i], NULL);
        total_found += thread_args[i].cnt;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double res = get_working_time(&start, &end);
    printf("Result: %zu (threads: %zu)\n", total_found, n_threads);
    printf("Working time: %.6f sec\n", res);
    free(threads);
    free(thread_args);
    fclose(f);
    return EXIT_SUCCESS;
}