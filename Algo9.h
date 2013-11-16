#ifndef ALGO9_H_
#define ALGO9_H_

#include <iostream>
#include <fstream>
#include <vector>
#include "MessageFormat.h"
#include "communication.h"
#include "LexiQueue.h"
#include <pthread.h>

 using namespace std;
class Torum
{
protected:

	long sequenceNo;
	int HOLDER;
	int **quorum;
	int quorumsize;
	int NumNodes;
	bool inCS;

	//vector<vector<int>> quorum;;
	pthread_mutex_t sharedQLock;
	LexiQueue *queue;


public:
	int ID;
	communication com;
	int flagforCS;
	char CS_FILENAME[25];

	static Torum* getInstance();
	void init();
	bool setID(int id);
	int getID();
	bool getQuorumTable(int **quorumtable,int qsize,int nnodes);
	bool requestCS();
	bool receiveRequest(Packet request);
	int receiveToken(Packet request);
	bool receiveRelease(Packet request);
	bool receiveHaveTkn(Packet request);
	bool sendToken();
	void displayStatus();

private:
	Torum(){};
	~Torum(void);
	Torum(Torum const& copy){};
	Torum& operator=(Torum const& copy){};
	static Torum* instance;
	bool EnterTheCS();
	void writeToFile(string filename,string line);
};

#endif;
