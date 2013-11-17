#ifndef LEXIQUEUE_H_
#define LEXIQUEUE_H_

#pragma once
#include "MessageFormat.h"
#include <cstdlib>
#include <queue>
#include<iostream>
#include<stdio.h>

using namespace std;

class CompareMessages{
public:
	bool operator()(Packet& m1,Packet& m2){
		if(m2.SEQ < m1.SEQ) return true;
		if(m2.SEQ == m1.SEQ && m2.ORIGIN < m1.ORIGIN) return true;
		return false;
	}
};
class LexiQueue
{
protected:
	priority_queue<Packet, vector<Packet>, CompareMessages> pq;
	priority_queue<Packet, vector<Packet>, CompareMessages> tempq;
public:
	LexiQueue(void);
	~LexiQueue(void);
	Packet top();
	Packet remove(int origin);
	Packet remove(int origin,long seq);
	bool add(Packet in);
	int size();
	bool isEmpty();
	bool updateTorumQ(int **quorum,int qsize,int ID);
	bool equalsTo(Packet m1, Packet m2);
	bool contains(int origin,long seq);
	void displayContents();
};

class BlockingQueue{


};
#endif
