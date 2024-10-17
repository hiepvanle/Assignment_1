#ifndef MATRIX_OPERATIONS_H
#define MATRIX_OPERATIONS_H

void populate(int size, double* A, double* B);
void printm(int size, double* C);
double sequentialMultiply(int size);
double parallelRowMultiply(int size, int num_processes);
double parallelElementMultiply(int size, int num_processes);

#endif // MATRIX_OPERATIONS_H