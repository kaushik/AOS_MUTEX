/*
 * Starter.h
 *
 *  Created on: Nov 6, 2013
 *      Author: kaushik sirineni
 */

#ifndef STARTER_H_
#define STARTER_H_

#include <vector>
#include <string>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <sys/time.h>
#include <unistd.h>
#include "Algo9.h"
#include "communication.h"

using namespace std;

class Starter {
private:
	Torum *node;
	int quorumSize;
	int NumNodes;
	int **Quorum;

	char **mapIDtoIP;
	char CS_FILENAME[25];
	void parseMsg(const string& s,const string& delim,std::vector<string>& tokens);
	
public:
	int id;

	struct timeval start, end;
	Starter();
	virtual ~Starter();
	void init();
	void registerAtController(char controllerIP[],int port);
	void decideAlgorithm();
	void Algorithm1();
	void Algorithm2();

};

#endif /* STARTER_H_ */
