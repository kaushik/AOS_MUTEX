//============================================================================
// Name        : aos1.cpp
// Author      : kaushik sirineni
// Version     :
// Copyright   : all rights reserved @ kaushik.me
// Description : Mutual Exclusion
//============================================================================

#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include<fstream>
#include "Starter.h"
#include "LexiQueue.h"
using namespace std;

int main() {
	cout << "Mutual Exclusion" << endl; // prints Mutual Exclusion

	//Starter s;
	//s.init();

	//ifstream input("time.in", ios::in);

	struct timeval start, end;

	    long mtime, seconds, useconds;

	    gettimeofday(&start, NULL);
	    usleep(27200);
	    gettimeofday(&end, NULL);

	    seconds  = end.tv_sec  - start.tv_sec;
	    useconds = end.tv_usec - start.tv_usec;

	    mtime = ((seconds) * 1000000 + useconds/1.0) + 0.5;

	    printf("Elapsed time: %ld milliseconds\n", mtime);


	return 0;
}
/**/


