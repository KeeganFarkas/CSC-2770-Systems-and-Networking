#include <iostream>
#include <sstream>
#include "sys/time.h"

int SIZE = 20000;

int main(int argc, char *argv[]) {
	if (argc == 2) {
		std::stringstream ss(argv[1]);
		ss >> SIZE;
		if (ss.fail()) {
			std::cerr << "Canot conver command line number"
				<< std::endl;
			exit(1);
		}
	}

	int *array = new int[SIZE*SIZE];
	struct timeval tv1, tv2;

	gettimeofday(&tv1,NULL); //get the current time


	for (int i = 0; i < SIZE; i++)
		for (int j = 0; j < SIZE; j++)
			array[i*SIZE+j]=0;


	gettimeofday(&tv2,NULL);

     //this prints out the time in seconds between
     //the two calls to gettimeofday

	std::cout << "Time: " << (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
		(double)(tv2.tv_sec - tv1.tv_sec) << std::endl;

	delete[] array;
	return 0;
}