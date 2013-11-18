//
//  MaekawaAlgorithm.cpp
//  AOSProject
//
//  Created by Antony on 11/10/13.
//  Copyright (c) 2013 Antony. All rights reserved.
//

#include "MaekawaAlgorithm.h"
#include "wqueue.h"
#include <pthread.h>
#include "communication.h"

MaekawaAlgorithm* MaekawaAlgorithm::instance = NULL;

MaekawaAlgorithm* MaekawaAlgorithm::getInstance()
{
    if(!instance){
        instance = new MaekawaAlgorithm();
    }
    return instance;
}

void MaekawaAlgorithm::initialization(){
	sequenceNo = 0;
	queue = new LexiQueue();
	hasFailed = false;
    
    //the current node has finished entering the critical section. It needs to send RELEASE messages.
	hasCompletedCriticalSection = false;
    
    //the current node has ever sent a locked message to other nodes. It has been lock by some nodes. When it receives another request, it needs to send INQUIRE messages.
    hasSentLockedMessage = false;
    hasSentLockedMessagetoItself = false;
    hasReceivedFailedMessage = false;
    
    hasSentRequestMessage = -1;
    hasReceivedLockedMessage = 0;
    NodesTotalNumber = 16;
	pthread_mutex_init(&sharedLock, NULL);
    
    printf("**** hasLockedFor Table ****\n");
    //initialize hasLockedFor table
    for(int i = 0; i < NodesTotalNumber; i++){
        vector<int> temp;
        temp.push_back(i);
        temp.push_back(0);
        hasLockedFor.push_back(temp);
        
        printf("%d | %d",hasLockedFor[i][0],hasLockedFor[i][1]);
    }
}

bool MaekawaAlgorithm::setProcessID(int pid){
	processID = pid;
	return true;
    
}

bool MaekawaAlgorithm::getQuorumTable(int **quorumtable,int qsize,int Nnodes){
    quorum = quorumtable;
    quorumsize = qsize;
    NoOfnodes = Nnodes;
    
    printf("--^^--quorumVote Table--^^-- \n");
    //initialize quorumVote
    for(int i = 0; i < quorumsize; i++){
        vector<int> temp;
        temp.push_back(quorum[processID][i]);
        temp.push_back(0);
        quorumVote.push_back(temp);
        //quorumVote[i].assign(temp[0],temp[1]);
        printf("%d , %d \n",quorumVote[i][0],quorumVote[i][1]);
        
//        quorumVote[i][1] = 100000;
//        printf("%d , %d \n",quorumVote[i][0],quorumVote[i][1]);
        
    }
    
    
    
    return true;
}

bool MaekawaAlgorithm::requestCriticalSection(){
    
    printf("----Node %d is trying to request critical section\n",processID);
    sequenceNo++;
	struct Packet request;
	request.TYPE = REQUEST;
	request.ORIGIN = processID;
    request.SEQ = sequenceNo;
    request.sender = -1;
    //reset relinquishList
    for(int i = 0; i < relinquishList.size(); i++){
        printf("----relinquishList size: %lu. Member removed: %d \n",relinquishList.size(),relinquishList.back());
        relinquishList.pop_back();
    }
    
    //add request to the node queue
	queue->add(request);
    printf("----The request is: (%lu , %d) \n",sequenceNo,processID);
    
//    //broadcast request to all processes in its quorum
//	for(int j = 0; j < quorumsize; j++){
//        com.sendMessageToID(request,quorum[processID][j]);
//        printf("----Node %d has sent REQUEST message to %d \n",processID,quorum[processID][j]);
//	}
    
    //broadcast request to all other process in its quorum
    for(int j = 0; j < quorumsize; j++){
        if(quorum[processID][j] != processID){
            com.sendMessageToID(request,quorum[processID][j]);
            printf("----Node %d has sent REQUEST message to %d \n",processID,quorum[processID][j]);
        }
        else if(quorum[processID][j] == processID){
            //The node itself will immediately receive it's request
            printf("----Node %d has sent REQUEST message to itself \n",processID);
            receiveRequest(request);
        }
	}
    
    //hasSentLockedMessagetoItself = true;
	return true;
}

void MaekawaAlgorithm::receiveMakeRequest(Packet makeRequest){
    
    printf("----Node %d has received MAKE_REQUEST message from %d \n",processID,makeRequest.ORIGIN);
    //Compare and maximize the sequence number
    if(sequenceNo < makeRequest.SEQ)
		sequenceNo = makeRequest.SEQ - 1;
    
    //
    hasSentRequestMessage ++;
    printf("----Node %d has %d request to send.\n",processID,hasSentRequestMessage);
    
    //Call requestCriticalSection to broadcast request
    if(hasSentRequestMessage == 0)
        requestCriticalSection();
    
}

bool MaekawaAlgorithm::receiveRequest(Packet request){
	
    printf("----Node %d has received REQUEST message from %d \n",processID,request.ORIGIN);
    //Compare and maximize the sequence number
    if(sequenceNo < request.SEQ)
		sequenceNo = request.SEQ;
    
    //add request to the node queue
    if(request.ORIGIN != processID){
        queue->add(request);
    }

    
    //(i) Check if it has been locked
    //(ii)Check if current input message is the top in the queue
    if(hasSentLockedMessage == false){
        
        //send LOCKED message back to sender of request
        struct Packet locked;
        locked.TYPE = LOCKED;
        locked.ORIGIN = processID;
        locked.SEQ = sequenceNo;
        locked.sender = -1;
        
        printf("----Node %d has sent LOCKED message to %d \n",processID,request.ORIGIN);
        
        if(request.ORIGIN != processID){
            com.sendMessageToID(locked, request.ORIGIN);
            lockedBy = request.ORIGIN;
            hasSentLockedMessage = true;
            
            //The node will know that it has sent locked message to a certain node, so it will not send duplicate locked message.
            hasLockedFor[request.ORIGIN][1] = 1;
            printf("**** hasLockedFor Table ****\n");
            for(int k = 0; k < NodesTotalNumber; k++){
                printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
            }
        }
        else if(request.ORIGIN == processID){
            lockedBy = request.ORIGIN;
            hasSentLockedMessage = true;
            printf("----Node %d has sent LOCKED message to itself\n",processID);
            
            //The node will know that it has sent locked message to a certain node, so it will not send duplicate locked message.
            hasLockedFor[request.ORIGIN][1] = 1;
            printf("**** hasLockedFor Table ****\n");
            for(int k = 0; k < NodesTotalNumber; k++){
                printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
            }
            receiveLocked(locked);
            printf("!!!!This message is supposed to show up after LOCK TO ITSELF message\n");
        }
    }
    else if(queue->equalsTo(queue->top(), request) == true){
        
        //send INQUIRE message to the process that locked the current process
        struct Packet inquire;
        inquire.TYPE = INQUIRE;
        inquire.ORIGIN = processID;
        inquire.SEQ = sequenceNo;
        inquire.sender = -1;
        
        printf("----Node %d has sent INQUIRE message to %d \n",processID,lockedBy);
        
        if(lockedBy != processID){
            com.sendMessageToID(inquire, lockedBy);
        }
        else if(lockedBy == processID){
            printf("----Node %d has sent INQUIRE message to itself\n",processID);
            receiveInquire(inquire);
        }
    }
    else if(queue->equalsTo(queue->top(), request) == false){
        
        //send FAILED message to sender of request
        struct Packet failed;
        failed.TYPE = FAILED;
        failed.ORIGIN = processID;
        failed.SEQ = sequenceNo;
        failed.sender = -1;
        
        printf("----Node %d has sent FAILED message to %d \n",processID,request.ORIGIN);
        
        if(request.ORIGIN != processID){
            com.sendMessageToID(failed, request.ORIGIN);
        }
        else if(request.ORIGIN == processID){
            printf("----Node %d has sent FAILED message to itself\n",processID);
            receiveFailed(failed);
        }
    }
    return true;
}

bool MaekawaAlgorithm::receiveInquire(Packet inquire) {
    
    printf("----Node %d has received INQUIRE message from %d \n",processID,inquire.ORIGIN);
//    //Compare and maximize the sequence number
//	if(sequenceNo<inquire.SEQ)
//		sequenceNo = inquire.SEQ;
    
    if(hasSentLockedMessage == true || hasCompletedCriticalSection == true)
        //Put the ORIGIN of inquire into list
        relinquishList.push_back(inquire.ORIGIN);
    
    if(hasSentLockedMessage == true && hasReceivedFailedMessage == true){
        struct Packet relinquish;
        relinquish.TYPE = RELINQUISH;
        relinquish.ORIGIN = processID;
        relinquish.SEQ = sequenceNo;
        relinquish.sender = -1;
        
        //send(relinquish);
        if(inquire.ORIGIN != processID){
            com.sendMessageToID(relinquish, inquire.ORIGIN);
            printf("----Node %d has sent RELINQUISH message to %d \n",processID,inquire.ORIGIN);
        }
        else if(inquire.ORIGIN == processID){
            printf("----Node %d has sent RELINQUISH message to itself\n",processID);
            receiveRelinquish(relinquish);
        }
    }
    
	return true;
}

bool MaekawaAlgorithm::receiveFailed(Packet failed) {

    printf("----Node %d has received FAILED message from %d \n",processID,failed.ORIGIN);
//    //Compare and maximize the sequence number
//    if(sequenceNo<failed.SEQ)
//		sequenceNo = failed.SEQ;
    hasReceivedFailedMessage = true;
    int temp = -1;
    for(int i = 0; i < quorumsize; i++){
        if(quorumVote[i][0] == failed.ORIGIN) temp = i;
    }
    quorumVote[temp][1] = 0;
    for(int i = 0; i < quorumsize; i++){
        printf("%d , %d \n",quorumVote[i][0],quorumVote[i][1]);
    }
    
    //Send RELINQUISH message to all the members in relinquishList
    for(int i = 0; i < relinquishList.size(); i++){
        struct Packet relinquish;
        relinquish.TYPE = RELINQUISH;
        relinquish.ORIGIN = processID;
        relinquish.SEQ = sequenceNo;
        relinquish.sender = -1;
        
        //send(relinquish);
        if(relinquishList.back() != failed.ORIGIN){
            if(relinquishList.back() != processID){
                com.sendMessageToID(relinquish, relinquishList.back());
                printf("----Node %d has sent RELINQUISH message to %d \n",processID,relinquishList.back());
                relinquishList.pop_back();
            }
            else if(relinquishList.back() == processID){
                printf("----Node %d has sent RELINQUISH message to itself\n",processID);
                relinquishList.pop_back();
                receiveRelinquish(relinquish);
            }
        }

    }
    return true;
}

bool MaekawaAlgorithm::receiveRelinquish(Packet relinquish){
    
    printf("----Node %d has received RELINQUISH message from %d \n",processID,relinquish.ORIGIN);
//    //Compare and maximize the sequence number
//	if(sequenceNo < relinquish.SEQ)
//        sequenceNo = relinquish.SEQ;

    //Send LOCKED message to the top priority member in queue and send message to its ORIGIN
    struct Packet locked;
    locked.TYPE = LOCKED;
    locked.ORIGIN = processID;
    locked.SEQ = sequenceNo;
    locked.sender = -1;
    
    //Update hasLockedFor Table
    hasLockedFor[relinquish.ORIGIN][1] = 0;
    printf("----Node %d has release LOCK from %d \n",relinquish.ORIGIN,processID);
    printf("**** Sent LOCKED Table ****\n");
    for(int k = 0; k < NodesTotalNumber; k++){
        printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
    }
    
    //Update quorumVote Table
    printf("--^^--quorumVote Table--^^-- \n");
    

    

    
    if(relinquish.ORIGIN == processID){
        for(int i = 0; i < quorumsize; i++){
            printf("%d , %d \n",quorumVote[i][0],quorumVote[i][1]);
            if(quorumVote[i][0] == processID){
                quorumVote[i][1] = 0;
            }
        }
        printf("----Node %d has received %d LOCKED messages \n",processID,hasReceivedLockedMessage);
    }
    
    if(queue->top().ORIGIN != processID){
        //check if it has sent locked message to this node before
        if(hasLockedFor[queue->top().ORIGIN][1] == 0){
            com.sendMessageToID(locked, queue->top().ORIGIN);
            
            //The node will know that it has sent locked message to a certain node, so it will not send duplicate locked message.
            hasLockedFor[queue->top().ORIGIN][1] = 1;
            printf("**** Sent LOCKED Table ****\n");
            for(int k = 0; k < NodesTotalNumber; k++){
                printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
            }
            printf("----Node %d has sent LOCKED message to %d \n",processID,queue->top().ORIGIN);
        }
        else if(hasLockedFor[queue->top().ORIGIN][1] == 1){
            printf("----Node %d has already sent LOCKED message to %d \n ",processID,queue->top().ORIGIN);
            printf("----It will not resend\n");
        }

        
    }
    else if(queue->top().ORIGIN == processID){
        //check if it has sent locked message to this node before
        if(hasLockedFor[queue->top().ORIGIN][1] == 0){
            printf("----Node %d has sent LOCKED message to itself\n",processID);
            
            //The node will know that it has sent locked message to a certain node, so it will not send duplicate locked message.
            hasLockedFor[queue->top().ORIGIN][1] = 1;
            printf("**** Sent LOCKED Table ****\n");
            for(int k = 0; k < NodesTotalNumber; k++){
                printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
            }
            receiveLocked(locked);
        }
        else if(hasLockedFor[queue->top().ORIGIN][1] == 1){
            printf("----Node %d has already sent LOCKED message to itself \n ",processID);
            printf("----It will not resend\n");
        }

    }

	return true;
}

bool MaekawaAlgorithm::receiveLocked(Packet locked){
    
    printf("----Node %d has received LOCKED message from %d \n",processID,locked.ORIGIN);
//    //Compare and maximize the sequence number
//    if(sequenceNo < locked.SEQ)
//        sequenceNo = locked.SEQ;
    
    //Increase hasReceivedLockedMessage by 1. If this variable reaches K-1, then the node can enter the critical section.
    //hasReceivedLockedMessage++;
    
    //Update quorumVote table
    
    int temp = -1;
    for(int i = 0; i < quorumsize; i++){
        if(quorumVote[i][0] == locked.ORIGIN) temp = i;
    }
    quorumVote[temp][1] = 1;
    
    hasReceivedLockedMessage = 0;
    
    printf("--^^--quorumVote Table--^^-- \n");
    
    //Check if the current node has received all the locked messages
    for(int i = 0; i < quorumsize; i++){
        printf("%d , %d \n",quorumVote[i][0],quorumVote[i][1]);
        if(quorumVote[i][1] == 1){
            hasReceivedLockedMessage++;
        }
    }
    printf("----Node %d has received %d LOCKED messages \n",processID,hasReceivedLockedMessage);
    
    if(hasReceivedLockedMessage == quorumsize){
        
        printf("----Node %d has entered critical section \n",processID);
        enterCriticalSection();
        //printf("Node %d has exited critical section \n",processID);
        return true;
    }
    return true;
}

bool MaekawaAlgorithm::receiveRelease(Packet release){
    
    printf("----Node %d has received RELEASE message from %d \n",processID,release.ORIGIN);
    
//    //Compare and maximize the sequence number
//	if(sequenceNo < release.SEQ)
//        sequenceNo = release.SEQ;
    
//    //Discard the release message from itself
//    if(release.ORIGIN == processID){
//        printf("----Node %d has received RELEASE message from %d. The message is discarded \n",processID,release.ORIGIN);
//        hasCompletedCriticalSection = false;
//        return true;
//    }
    
    //(i)       deletes release message's node from the queue
    //(ii) a)   lock for the most preceding request in the queue. It sends LOCK message.
    //(ii) b)   No request in the queue. Unlock itself. Do nothing.
    if(release.ORIGIN != processID){
        queue->remove(release.ORIGIN);
    }
    
    //Update hasLockedFor table
    hasLockedFor[release.ORIGIN][1] = 0;
    printf("----Node %d has release LOCK from %d \n",release.ORIGIN,processID);
    printf("**** Sent LOCKED Table ****\n");
    hasSentLockedMessage = false;
    for(int k = 0; k < NodesTotalNumber; k++){
        if(hasLockedFor[k][1] == 1)
            hasSentLockedMessage = true;
        printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
    }
    

    
    
    if(queue->top().TYPE == -1 ){ //-1 means nothing in the queue
        
        printf("----After Node %d received release message, there is no more request in the queue \n",processID);
        hasSentLockedMessage = false;
    }
    else{
        
//        //check if it has sent locked message to this node before
//        if(hasLockedFor[queue->top().ORIGIN == 0]){
//            com.sendMessageToID(locked, queue->top().ORIGIN);
//            
//            //The node will know that it has sent locked message to a certain node, so it will not send duplicate locked message.
//            hasLockedFor[queue->top().ORIGIN] = 1;
//            printf("**** Sent LOCKED Table ****\n");
//            for(int k = 0; k < NodesTotalNumber; k++){
//                printf("%d | %d \n",k,hasLockedFor[k]);
//            }
//            printf("----Node %d has sent LOCKED message to %d \n",processID,queue->top().ORIGIN);
//        }
//        else{
//            printf("----Node %d has already sent LOCKED message to %d \n ",processID,queue->top().ORIGIN);
//            printf("----It will not resend\n");
//        }
        

        
        struct Packet locked;
        locked.TYPE = LOCKED;
        locked.ORIGIN = processID;
        locked.SEQ = sequenceNo;
        locked.sender = -1;
        
        printf("----The next request in the queue: %d \n",queue->top().ORIGIN);
        printf("----After Node %d received release message, there is at least one request in the queue \n",processID);
        //print the request in the queue
        printf("----The request is: (%lu , %d) \n",queue->top().SEQ,queue->top().ORIGIN);
        if(hasSentLockedMessage == false){
            if(queue->top().ORIGIN != processID){
                
                //check if it has sent locked message to this node before
                if(hasLockedFor[queue->top().ORIGIN][1] == 0){
                    
                    hasLockedFor[queue->top().ORIGIN][1] = 1;
                    
                    com.sendMessageToID(locked, queue->top().ORIGIN);
                    printf("----Node %d has sent LOCKED message to %d \n",processID,queue->top().ORIGIN);
                    
                    printf("**** Sent LOCKED Table ****\n");
                    for(int k = 0; k < NodesTotalNumber; k++){
                        printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
                    }
                    lockedBy = queue->top().ORIGIN;
                    hasSentLockedMessage = true;
                }
                
            }
            else if(queue->top().ORIGIN == processID){
                if(hasLockedFor[queue->top().ORIGIN][1] == 0){
                    
                    hasLockedFor[queue->top().ORIGIN][1] = 1;
                    
                    lockedBy = queue->top().ORIGIN;
                    hasSentLockedMessage = true;
                    printf("----Node %d has sent LOCKED message to itself\n",processID);
                    
                    printf("**** Sent LOCKED Table ****\n");
                    for(int k = 0; k < NodesTotalNumber; k++){
                        printf("%d | %d \n",hasLockedFor[k][0],hasLockedFor[k][1]);
                    }
                    receiveLocked(locked);
                }
                
            }
        }

        
    }
    if(release.ORIGIN == processID){
        //Call requestCriticalSection to broadcast request
        if(hasSentRequestMessage > 0){
            requestCriticalSection();
        }
        hasSentRequestMessage--;
        printf("----Node %d has %d request left to send.\n",processID,hasSentRequestMessage);
    }
    
	return true;
}
void MaekawaAlgorithm::writeToFile(string filename,string line){
	ofstream myfile (filename.c_str(),ios::out | ios::app);
	  if (myfile.is_open())
	  {
	    myfile <<line<<endl;
	    myfile.close();
	  }
	  else cout << "Unable to open file";
}

void MaekawaAlgorithm::enterCriticalSection(){
	printf("\n******Node '%d' in CRITICAL SECTION******\n",processID);
	flagforCS =true;
    char buff[4095];
	sprintf(buff,"Node %d entered CS, Seq: %ld \n",processID,sequenceNo);
	writeToFile("Resource.txt",buff);

    //sleep(15);
    hasCompletedCriticalSection = true;
    hasSentLockedMessage = false;
    
    printf("----Queue size before remove is: %d\n", queue->size());
    queue->remove(processID);
    
    printf("----Queue size after remove is: %d\n", queue->size());
    printf("----Node %d has delete itself from the queue\n",processID);
    printf("----Node %d has received 0 locked message\n",processID);
    printf("----Node %d has exited its critical section\n",processID);
    printf("********************************************\n");
    
    //reset quorumVote Table
    printf("--^^--quorumVote Table--^^-- \n");
    
    for(int i = 0; i < quorumsize; i++){
        quorumVote[i][1] = 0;
        printf("%d , %d \n",quorumVote[i][0],quorumVote[i][1]);
    }
    sendRelease();
    

}

bool MaekawaAlgorithm::sendRelease(){
    
	//Packet top = queue->top();
	if(hasCompletedCriticalSection == true){
        
		struct Packet release;
		release.TYPE = RELEASE;
		release.ORIGIN = processID;
        release.sender = -1;
        release.SEQ = sequenceNo;
        
		for(int j = 0 ; j < quorumsize ; j++){
            if(quorum[processID][j] != processID){
                printf("----Node %d has sent RELEASE message to node %d \n",processID,quorum[processID][j]);
                com.sendMessageToID(release, quorum[processID][j]);
                hasCompletedCriticalSection = false;
            }
            else if(quorum[processID][j] == processID){
                printf("----Node %d has sent RELEASE message to itself\n",processID);
                hasCompletedCriticalSection = false;
                receiveRelease(release);
            }
		}
        printf("----Node %d has sent all the RELEASE messages to its quorum members\n",processID);
	}
    
	return true;
    
}




