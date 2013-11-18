//
//  MaekawaAlgorithm.h
//  AOSProject
//
//  Created by Antony on 11/10/13.
//  Copyright (c) 2013 Antony. All rights reserved.
//

#ifndef __AOSProject__MaekawaAlgorithm__
#define __AOSProject__MaekawaAlgorithm__

#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include "MessageFormat.h"
#include "communication.h"
#include "LexiQueue.h"
#include <pthread.h>

class MaekawaAlgorithm
{
protected:
    

	long sequenceNo;
	int ** quorum;
	int quorumsize;
	int NoOfnodes;
    int NodesTotalNumber;
	bool hasFailed;
	bool hasCompletedCriticalSection;
    bool hasSentLockedMessage;
    bool hasSentLockedMessagetoItself;
    bool hasReceivedFailedMessage;
    int hasSentRequestMessage;
    int lockedBy;
    int hasReceivedLockedMessage;
    vector< vector<int> > quorumVote;
//    int hasLockedFor[16];
    vector< vector<int> > hasLockedFor;
    vector<int> relinquishList;
	LexiQueue *queue;
    pthread_mutex_t sharedLock;
    
public:
    int processID;
    communication com;
    char CS_FILENAME[25];
    int flagforCS;
    static MaekawaAlgorithm* getInstance();
    void receiveMessage(Packet msg);
    void receiveMakeRequest(Packet makeRequest);
    void initialization();
    bool setProcessID(int pid);
	bool getQuorumTable(int **quorumtable,int qsize,int nnodes);
	bool requestCriticalSection();
	bool receiveRequest(Packet request);
	bool receiveInquire(Packet inquire);
	bool receiveFailed(Packet failed);
	bool receiveRelease(Packet release);
	bool receiveRelinquish(Packet relinquish);
    bool receiveLocked(Packet locked);
	void enterCriticalSection();
	bool sendRelease();
	//bool sendRequest(Packet req);
	
    
private:
    MaekawaAlgorithm(){};
    MaekawaAlgorithm(MaekawaAlgorithm const& copy){};
    //MaekawaAlgorithm& operator=(MaekawaAlgorithm const& copy){};
    static MaekawaAlgorithm* instance;
    void writeToFile(string filename,string line);
};

#endif;

