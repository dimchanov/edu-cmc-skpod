#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int PROCESS_NUM;
int PROCESS_RANK;

int compare(const int* a, const int* b) {
  return *a >= *b;
}

void print_vector(long long size, int* vector) {
  for (long long i = 0; i < size; ++i) {
    printf("%d ", vector[i]);
  }
  printf("\n");
}

void random_initialization(long long size, int* p_vector) {
  for (long long i = 0; i < size; ++i) {
    p_vector[i] = (int)rand() % 10000;
  }
}

void parallel_initialization(long long size,
                             int** p_vector,
                             int** p_process_chunk,
                             long long* chunk_size) {
  if (PROCESS_RANK == 0) {
    *p_vector = (int*)malloc(size * sizeof(int));
    assert(*p_vector != NULL && "Malloc error");
    random_initialization(size, *p_vector);
  }

  long long new_chunk_size = size;
  for (int i = 0; i < PROCESS_RANK; ++i) {
    new_chunk_size -= new_chunk_size / (PROCESS_NUM - i);
  }
  *chunk_size = new_chunk_size / (PROCESS_NUM - PROCESS_RANK);
  *p_process_chunk = (int*)malloc(*chunk_size * sizeof(int));
  assert(*p_process_chunk != NULL && "Malloc error");
}

void vector_distribution(long long size,
                         int* p_vector,
                         int* p_process_chunk,
                         long long chunk_size) {
  int* p_send_num;
  int* p_send_ind;
  int new_chunk_size = size;
  p_send_num = (int*)malloc(PROCESS_NUM * sizeof(int));
  assert(p_send_num != NULL && "Malloc error");
  p_send_ind = (int*)malloc(PROCESS_NUM * sizeof(int));
  assert(p_send_ind != NULL && "Malloc error");

  p_send_num[0] = chunk_size;
  p_send_ind[0] = 0;
  chunk_size = size / PROCESS_NUM;
  for (int i = 1; i < PROCESS_NUM; ++i) {
    new_chunk_size -= chunk_size;
    chunk_size = new_chunk_size / (PROCESS_NUM - i);
    p_send_num[i] = chunk_size;
    p_send_ind[i] = p_send_ind[i - 1] + p_send_num[i - 1];
  }
  MPI_Scatterv(p_vector, p_send_num, p_send_ind, MPI_INT, p_process_chunk,
               p_send_num[PROCESS_RANK], MPI_INT, 0, MPI_COMM_WORLD);
  free(p_send_num);
  free(p_send_ind);
}

void parallel_calculation(int* p_process_chunk, long long chunk_size) {
  qsort(p_process_chunk, chunk_size, sizeof(int),
        (int (*)(const void*, const void*))compare);
}

void merge(int** a, long long a_size, int* b, long long b_size) {
  int* buffer = (int*)malloc((a_size + b_size) * sizeof(int));
  assert(buffer != NULL && "Malloc error in thread");
  long long a_iter = 0;
  long long b_iter = 0;
  long long idx = 0;
  while (a_iter < a_size || b_iter < b_size) {
    if ((*a)[a_iter] < b[b_iter]) {
      buffer[idx++] = (*a)[a_iter];
      ++a_iter;
    } else {
      buffer[idx++] = b[b_iter];
      ++b_iter;
    }
    if (a_iter == a_size) {
      while (b_iter < b_size) {
        buffer[idx++] = b[b_iter];
        ++b_iter;
      }
    }
    if (b_iter == b_size) {
      while (a_iter < a_size) {
        buffer[idx++] = (*a)[a_iter];
        ++a_iter;
      }
    }
  }
  *a = (int*)realloc((*a), (a_size + b_size) * sizeof(int));
  assert((*a) != NULL && "Malloc error in thread");
  for (long long i = 0; i < (a_size + b_size); ++i) {
    (*a)[i] = buffer[i];
  }
  free(buffer);
}

void parallel_result_connection(int process_num,
                                int** p_process_chunk,
                                long long* chunk_size) {
  if (process_num == 1) {
    return;
  }
  int new_process_num = (process_num / 2) + (process_num % 2);
  if (PROCESS_RANK < (process_num / 2)) {
    long long tmp_chunk_size;
    MPI_Recv(&tmp_chunk_size, 1, MPI_LONG_LONG_INT,
             PROCESS_RANK + new_process_num, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);

    int* tmp_chunk = (int*)malloc(tmp_chunk_size * sizeof(int));
    assert(tmp_chunk != NULL && "Malloc error");
    MPI_Recv(tmp_chunk, tmp_chunk_size, MPI_INT, PROCESS_RANK + new_process_num,
             1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    merge(p_process_chunk, *chunk_size, tmp_chunk, tmp_chunk_size);
    *chunk_size += tmp_chunk_size;
    free(tmp_chunk);
  }
  if ((PROCESS_RANK >= new_process_num) && (PROCESS_RANK < process_num)) {
    MPI_Send(chunk_size, 1, MPI_LONG_LONG_INT, PROCESS_RANK - new_process_num,
             0, MPI_COMM_WORLD);
    MPI_Send(*p_process_chunk, *chunk_size, MPI_INT,
             PROCESS_RANK - new_process_num, 1, MPI_COMM_WORLD);
  }
  parallel_result_connection(new_process_num, p_process_chunk, chunk_size);
}

void test_parallel_result(long long size, int* p_vector, int* p_result) {
  if (PROCESS_RANK == 0) {
    int* s_result = (int*)malloc(size * sizeof(int));
    assert(s_result != NULL && "Malloc error");
    for (long long i = 0; i < size; ++i) {
      s_result[i] = p_vector[i];
    }
    double time_start, time_finish;
    time_start = MPI_Wtime();
    qsort(s_result, size, sizeof(int),
          (int (*)(const void*, const void*))compare);
    time_finish = MPI_Wtime();

    printf("Elapsed time for Sequential algorithm = %06.6fs\n",
           time_finish - time_start);
    int equal = 0;
    for (long long i = 0; i < size; ++i) {
      if (p_result[i] != s_result[i]) {
        printf("ERR %d != %d\n", p_result[i], s_result[i]);
        equal = 1;
      }
    }
    if (equal) {
      printf("Sequential and Parallel results are NOT equal\n");
    } else {
      printf("Sequential and Parallel results are equal\n");
    }
    free(s_result);
  }
}

void parallel_termination(int** p_vector, int** p_process_chunk) {
  if (PROCESS_RANK == 0) {
    free(*p_vector);
  }
  free(*p_process_chunk);
}

int main(int argc, char** argv) {
  int err;
  srand(42);
  if ((err = MPI_Init(&argc, &argv))) {
    printf("MPI startup error!\n");
    MPI_Abort(MPI_COMM_WORLD, err);
  }
  MPI_Comm_size(MPI_COMM_WORLD, &PROCESS_NUM);
  MPI_Comm_rank(MPI_COMM_WORLD, &PROCESS_RANK);
  if (PROCESS_RANK == 0) {
    if (argc != 2) {
      exit(11);
    }
  }
  long long size;
  sscanf(argv[1], "%lld", &size);

  int* p_vector;
  int* p_process_chunk;
  long long chunk_size;

  parallel_initialization(size, &p_vector, &p_process_chunk, &chunk_size);

  double time_start, time_finish;
  time_start = MPI_Wtime();
  vector_distribution(size, p_vector, p_process_chunk, chunk_size);
  parallel_calculation(p_process_chunk, chunk_size);
  parallel_result_connection(PROCESS_NUM, &p_process_chunk, &chunk_size);
  time_finish = MPI_Wtime();

  if (PROCESS_RANK == 0) {
    printf("Size %lld\n", size);
    printf("Process number %d\n", PROCESS_NUM);
    printf("Elapsed time for Parallel algorithm = %06.6fs\n",
           time_finish - time_start);
  }

  test_parallel_result(size, p_vector, p_process_chunk);
  parallel_termination(&p_vector, &p_process_chunk);

  MPI_Finalize();
}
