#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h>
#include <assert.h>
#include <omp.h> 

int compare(const int *a, const int *b) 
{
    return *a >= *b;
}

void single_chunk_sort(int32_t* array, int64_t size) 
{
    qsort(array, size, sizeof(int32_t), (int32_t(*) (const void *, const void *))compare);
}

void merge(int32_t* a, int64_t a_size, int64_t b_size) 
{
    int32_t* b = a + a_size;
    int32_t* buffer = (int32_t*)malloc((a_size + b_size) * sizeof(int32_t));
    assert(buffer != NULL && "Malloc error in thread");
    int64_t a_iter = 0;
    int64_t b_iter = 0;
    int64_t idx = 0;
    while (a_iter < a_size || b_iter < b_size) 
    {
        if (a[a_iter] < b[b_iter])
        {
            buffer[idx++] = a[a_iter];
            ++a_iter;
        } else {
            buffer[idx++] = b[b_iter];
            ++b_iter;
        }
        if (a_iter == a_size) 
        {
            while (b_iter < b_size)
            {
                buffer[idx++] = b[b_iter];
                ++b_iter;
            }
            
        }
        if (b_iter == b_size) 
        {
            while (a_iter < a_size)
            {
                buffer[idx++] = a[a_iter];
                ++a_iter;
            }
            
        }
    }   
    for (int64_t i = 0; i < (a_size + b_size); ++i) 
    {
        a[i] = buffer[i];
    }
    free(buffer);
}

void parallel_merge_sort(int32_t* array, int64_t size, int32_t p)  
{
    int64_t part_size = 0;
    if (p > 1) {
        part_size = size / p;
    } else {
        part_size = size;
    }
    double parallel_start_time = omp_get_wtime(); 
    int32_t chunk_count = p;
#pragma omp parallel shared(part_size, size, array, chunk_count) num_threads(p)
    {
#pragma omp single
            {
                for (int i = 0; i < chunk_count; ++i) 
                {
#pragma omp task 
                    if (i == chunk_count - 1) {
                        single_chunk_sort(
                            array + part_size * i, 
                            size - part_size * i);
                    } else {

                        single_chunk_sort(
                            array + part_size * i, 
                            part_size);
                    }
                }
            }
        }
    int parity_flag = 0;
    while (chunk_count > 1) 
    {
        parity_flag = chunk_count % 2;
        chunk_count = chunk_count / 2;
#pragma omp parallel shared(part_size, size, array, chunk_count, parity_flag) num_threads(chunk_count)
        {
#pragma omp single
            {
                for (int i = 0; i < chunk_count; ++i) 
                {
#pragma omp task 
                    if (i == (chunk_count - 1) && (!parity_flag)) {
                        merge(
                            array + part_size * 2 * i, 
                            part_size,
                            size - part_size * (2 * i + 1));
                    } else {
                        merge(
                            array + part_size * 2 * i, 
                            part_size,
                            part_size);
                    }
                }
            }
#pragma omp taskwait
        }
        part_size *= 2;
        chunk_count += parity_flag;
    }

    double parallel_end_time = omp_get_wtime();
    printf("Parallel sort {\n");
    printf("\tElapsed time: %f s,\n", parallel_end_time - parallel_start_time);
    printf("\tArray size: %ld,\n", size);
    printf("\tNumber if treads: %d\n}\n", p);
}

int main(int argc, char *argv[]) 
{
    srand(42);
    assert(argc == 3 && "Invalid command line arguments");
    int64_t  n = (int64_t)atoll(argv[1]);
    assert(n > 0 && "Invalid array size");
    int32_t num_threads = (int32_t)atoi(argv[2]);
    assert(num_threads > 0 && "Invalid number of threads");
    int32_t* array_to_parallel_sorted = (int32_t*)malloc(n * sizeof(int32_t));
    assert(array_to_parallel_sorted != NULL && "Malloc error");
   for (int64_t i = 0; i < n; ++i) 
    {
        int32_t tmp = (int32_t)rand()%100;
        array_to_parallel_sorted[i] = tmp;
    }
    parallel_merge_sort(array_to_parallel_sorted, n, num_threads);
    free(array_to_parallel_sorted);
    return 0;
} 
