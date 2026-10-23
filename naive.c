#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const int TEXT_SIZE = 1 << 20;
struct timespec start, end;

double get_working_time(struct timespec *start, struct timespec *end) {
    long seconds = end->tv_sec - start->tv_sec;
    long nanoseconds = end->tv_nsec - start->tv_nsec;
    double res = seconds + nanoseconds * 1e-9;
    return res;
}

static size_t naive_search(const char *t, size_t t_size, const char *p, size_t p_size) {
    size_t cnt = 0;
    if (p_size == 0 || p_size > t_size)
        return 0;

    for (size_t i = 0; i + p_size <= t_size; ++i) {
        size_t j = 0;
        while (j < p_size && t[i + j] == p[j])
            ++j;

        if (j == p_size) {
            printf("%zu\n", i);
            ++cnt;
        }
    }
    return cnt;
}
  

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Incorrect agrs");
        return EXIT_FAILURE;
    }

    FILE *f = fopen(argv[1], "r");
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

    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t total = naive_search(text, t_size, pattern, p_size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double res = get_working_time(&start, &end);
    printf("Result: %zu\n", total);
    printf("Working time: %.6f sec\n", res);

    fclose(f);
    return EXIT_SUCCESS;
}