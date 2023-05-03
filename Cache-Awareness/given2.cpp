#include <iostream>
#include <sys/time.h>
#include "omp.h"

int main(int argc, char *argv[]) {
	int NUM_THREADS = 1;
	if (argc == 2) {
		NUM_THREADS = atoi(argv[1]);
	}

	if (NUM_THREADS == 0) {
		std::cerr << "ERROR: invalid number of threads" << std::endl;
		exit(1);
	}

	int *array = new int[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++)
		array[i] = 0;

	struct timeval tv1, tv2;

	gettimeofday(&tv1, 0);

#pragma omp parallel num_threads(NUM_THREADS)
	{
		int index = omp_get_thread_num();
		for(int i = 0; i < 1e9; i++) 
			array[index]++;
	}

	gettimeofday(&tv2, 0);

	std::cout << "Time: " << (double)(tv2.tv_usec - tv1.tv_usec) /
		1000000 + (double)(tv2.tv_sec - tv1.tv_sec) << std::endl;

	delete[] array;
	return 0;
}