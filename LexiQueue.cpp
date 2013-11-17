#include "LexiQueue.h"


LexiQueue::LexiQueue(void)
{

}


LexiQueue::~LexiQueue(void)
{

}

Packet LexiQueue::remove(int origin){
	tempq = pq;
	Packet top;
	Packet ret;
	ret.TYPE = -1;
	bool removed = false;
	while(!tempq.empty())
		tempq.pop();
	while(!pq.empty()){
		top = pq.top();
		pq.pop();
		if(top.ORIGIN == origin && removed == false){
			ret =top;
			removed = true;
		}else
			tempq.push(top);
	}
	pq = tempq;

	return ret;
}

Packet LexiQueue::remove(int origin, long seq){
	tempq = pq;
	Packet top;
	Packet ret;
	ret.TYPE = -1;
	while(!tempq.empty())
		tempq.pop();
	while(!pq.empty()){
		top = pq.top();
		pq.pop();
		if(top.ORIGIN == origin && top.SEQ == seq){
			ret =top;
		}else
			tempq.push(top);
	}
	pq = tempq;

	return ret;
}

Packet LexiQueue::top(){
	Packet top;
	top.TYPE = -1;
	if(!pq.empty()){
		top = pq.top();
		return top;
	}
	return top;
}

/* adds to the queue
 * handles duplicate entries by checking before adding
 */
bool LexiQueue::add(Packet in){
	/*if(pq.empty()){
			pq.push(in);
			return true;
	}*/
	Packet temp = remove(in.ORIGIN,in.SEQ);//if element already there remove it and then replace
	pq.push(in);
	return true;
}

bool LexiQueue::updateTorumQ(int **quorum,int qsize,int ID){


	while(!pq.empty()){
		Packet top = pq.top();
		pq.pop();
		for(int j=0;j<qsize;j++){
			if(quorum[top.ORIGIN][j] == ID){
				tempq.push(top);
				break;
			}
		}

	}
	pq = tempq;
	return true;
}

bool LexiQueue::contains(int origin,long seq){

	Packet p = remove(origin,seq);
	if(p.TYPE == -1){
		return false;
	}else{
		add(p);
		return true;
	}
}
int LexiQueue::size(){
	int ret = pq.size();
	return ret;
}

bool LexiQueue::isEmpty(){
	return pq.empty();
}

bool LexiQueue::equalsTo(Packet m1, Packet m2){
    if(m1.ORIGIN == m2.ORIGIN && m1.SEQ == m2.SEQ && m1.TYPE == m2.TYPE)
        return true;
    else
        return false;
   }

void LexiQueue::displayContents(){
	tempq = pq;
	printf("Queue:");
	while(!tempq.empty()){
		printf("%d(%d,s:%d),",(tempq.top()).ORIGIN,(tempq.top()).sender,(tempq.top()).SEQ);
		tempq.pop();
	}
	printf("\n");
}
