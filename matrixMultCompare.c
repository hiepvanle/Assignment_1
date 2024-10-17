#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "matrix_operations.h"

void writeDataToFile(int size, double sequential, double* parallel_row, double* parallel_element, int* num_processes, int num_process_options) {
    char filename[50];
    sprintf(filename, "matrix_mult_data_%d.txt", size);
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(fp, "Processes Sequential ParallelRow ParallelElement\n");
    fprintf(fp, "1 %f %f %f\n", sequential, sequential, sequential);
    for (int i = 0; i < num_process_options; i++) {
        fprintf(fp, "%d %f %f %f\n", num_processes[i], sequential, parallel_row[i], parallel_element[i]);
    }

    fclose(fp);
}

void createGnuplotScript(int size) {
    char filename[50];
    sprintf(filename, "plot_script_%d.gp", size);
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(fp, "set title 'Matrix Multiplication Comparison (Size %dx%d)'\n", size, size);
    fprintf(fp, "set xlabel 'Number of Processes'\n");
    fprintf(fp, "set ylabel 'Execution Time (microseconds)'\n");
    fprintf(fp, "set key right bottom\n");
    fprintf(fp, "set logscale y\n");
    fprintf(fp, "plot 'matrix_mult_data_%d.txt' using 1:2 with linespoints title 'Sequential', \\\n", size);
    fprintf(fp, "     '' using 1:3 with linespoints title 'Parallel Row', \\\n");
    fprintf(fp, "     '' using 1:4 with linespoints title 'Parallel Element'\n");
    fprintf(fp, "pause -1 'Press any key to continue'\n");

    fclose(fp);
}

void runComparison(int size, int* num_processes, int num_process_options) {
    printf("Matrix size: %d x %d\n", size, size);
    double sequential = sequentialMultiply(size);
    printf("Sequential: %.2f microseconds\n", sequential);
    
    double* parallel_row = malloc(num_process_options * sizeof(double));
    double* parallel_element = malloc(num_process_options * sizeof(double));

    for (int i = 0; i < num_process_options; i++) {
        int processes = num_processes[i];
        parallel_row[i] = parallelRowMultiply(size, processes);
        printf("Parallel Row (%d processes): %.2f microseconds\n", processes, parallel_row[i]);
        parallel_element[i] = parallelElementMultiply(size, processes);
        printf("Parallel Element (%d processes): %.2f microseconds\n", processes, parallel_element[i]);
    }
    printf("\n");

    writeDataToFile(size, sequential, parallel_row, parallel_element, num_processes, num_process_options);
    createGnuplotScript(size);

    free(parallel_row);
    free(parallel_element);
}

int main() {
    srand(time(NULL));

    int sizes[] = {10, 100, 1000, 2000, 2500, 3000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    int num_processes[] = {10, 100, 1000};
    int num_process_options = sizeof(num_processes) / sizeof(num_processes[0]);

    for (int i = 0; i < num_sizes; i++) {
        runComparison(sizes[i], num_processes, num_process_options);
    }

    printf("Data files and gnuplot scripts have been generated.\n");
    printf("To create the plots, run the following commands:\n");
    for (int i = 0; i < num_sizes; i++) {
        printf("gnuplot plot_script_%d.gp\n", sizes[i]);
    }

    return 0;
}