#include "matrix_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_NAME "/matrix_shm"
#define SEM_NAME "/sync_sem"

typedef struct {
    int size;
    int next_row;
    int next_col;
    double *A;
    double *B;
    double *C;
} SharedData;

void populate(int size, double* A, double* B) {
    for (int i = 0; i < size * size; i++) {
        A[i] = (double)rand() / RAND_MAX;
        B[i] = (double)rand() / RAND_MAX;
    }
}

void printm(int size, double* C) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%f ", C[i * size + j]);
        }
        printf("\n");
    }
}

double sequentialMultiply(int size) {
    double *A = (double*)malloc(size * size * sizeof(double));
    double *B = (double*)malloc(size * size * sizeof(double));
    double *C = (double*)malloc(size * size * sizeof(double));

    populate(size, A, B);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Actual multiplication
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            C[i * size + j] = 0;
            for (int k = 0; k < size; k++) {
                C[i * size + j] += A[i * size + k] * B[k * size + j];
            }
        }
    }

    gettimeofday(&end, NULL);

    double executionTime = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);

    if (size <= 10) {
        printf("Matrix A:\n");
        printm(size, A);
        printf("Matrix B:\n");
        printm(size, B);
        printf("Result Matrix C:\n");
        printm(size, C);
    }

    free(A);
    free(B);
    free(C);

    return executionTime;
}
void compute_row(SharedData *data, sem_t *sem) {
    while (1) {
        sem_wait(sem);
        int row = data->next_row++;
        sem_post(sem);

        if (row >= data->size) {
            break;
        }

        for (int j = 0; j < data->size; j++) {
            data->C[row * data->size + j] = 0;
            for (int k = 0; k < data->size; k++) {
                data->C[row * data->size + j] += data->A[row * data->size + k] * data->B[k * data->size + j];
            }
        }
    }
}

double parallelRowMultiply(int size, int num_processes) {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData) + 3 * size * size * sizeof(double));
    SharedData *shared_data = mmap(0, sizeof(SharedData) + 3 * size * size * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    shared_data->size = size;
    shared_data->next_row = 0;
    shared_data->A = (double*)((char*)shared_data + sizeof(SharedData));
    shared_data->B = shared_data->A + size * size;
    shared_data->C = shared_data->B + size * size;

    populate(size, shared_data->A, shared_data->B);

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < num_processes; i++) {
        if (fork() == 0) {
            compute_row(shared_data, sem);
            exit(0);
        }
    }

    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }

    gettimeofday(&end, NULL);
    double execution_time = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);

    if (size <= 10) {
        printf("Matrix A:\n");
        printm(size, shared_data->A);
        printf("Matrix B:\n");
        printm(size, shared_data->B);
        printf("Result Matrix C:\n");
        printm(size, shared_data->C);
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(shared_data, sizeof(SharedData) + 3 * size * size * sizeof(double));
    shm_unlink(SHM_NAME);

    return execution_time;
}

void compute_element(SharedData *data, sem_t *sem) {
    while (1) {
        sem_wait(sem);
        int row = data->next_row;
        int col = data->next_col;
        data->next_col++;
        if (data->next_col >= data->size) {
            data->next_row++;
            data->next_col = 0;
        }
        sem_post(sem);

        if (row >= data->size) {
            break;
        }

        double sum = 0;
        for (int k = 0; k < data->size; k++) {
            sum += data->A[row * data->size + k] * data->B[k * data->size + col];
        }
        data->C[row * data->size + col] = sum;
    }
}

double parallelElementMultiply(int size, int num_processes) {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData) + 3 * size * size * sizeof(double));
    SharedData *shared_data = mmap(0, sizeof(SharedData) + 3 * size * size * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    shared_data->size = size;
    shared_data->next_row = 0;
    shared_data->next_col = 0;
    shared_data->A = (double*)((char*)shared_data + sizeof(SharedData));
    shared_data->B = shared_data->A + size * size;
    shared_data->C = shared_data->B + size * size;

    populate(size, shared_data->A, shared_data->B);

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < num_processes; i++) {
        if (fork() == 0) {
            compute_element(shared_data, sem);
            exit(0);
        }
    }

    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }

    gettimeofday(&end, NULL);
    double execution_time = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);

    if (size <= 10) {
        printf("Matrix A:\n");
        printm(size, shared_data->A);
        printf("Matrix B:\n");
        printm(size, shared_data->B);
        printf("Result Matrix C:\n");
        printm(size, shared_data->C);
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(shared_data, sizeof(SharedData) + 3 * size * size * sizeof(double));
    shm_unlink(SHM_NAME);

    return execution_time;
}