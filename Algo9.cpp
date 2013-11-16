#include "Algo9.h"

Torum* Torum::instance = NULL;

Torum::~Torum(){
	pthread_mutex_destroy(&sharedQLock);
}
void Torum::init()
{
	sequenceNo = 0;
	HOLDER = -1;
	inCS = false;
	queue = new LexiQueue();
	if(pthread_mutex_init(&sharedQLock,NULL)!=0){
		printf("\n mutex lock init failed\n");
	}
}

Torum* Torum::getInstance()
        {
            if(!instance){
            	instance = new Torum();
            }
            return instance;
        }

bool Torum::setID(int id){
	ID = id;
	return true;
}
int Torum::getID(){
	return ID;
}
bool Torum::getQuorumTable(int **quorumtable,int qsize,int Nnodes){
	quorum = quorumtable;
	quorumsize = qsize;
	NumNodes = Nnodes;
	return true;
}

bool Torum::requestCS(){
	sequenceNo++;

	struct Packet request;
	request.TYPE = REQUEST;
	request.ORIGIN = ID;
	request.sender = ID;
	request.SEQ = sequenceNo;

	/*
	pthread_mutex_lock(&sharedQLock);
	queue->add(request);
	pthread_mutex_unlock(&sharedQLock);
	*/
	if( HOLDER == -1 ){
		int i;
		for(i=0;i<quorumsize;i++){
			com.sendMessageToID(request,quorum[ID][i]);
		}
	}else{
		if(HOLDER != ID)
		queue->add(request);
		com.sendMessageToID(request,HOLDER);
	}
	return true;
}

bool Torum::receiveRequest(Packet request){
printf("in receive Request, request from %d\n",request.ORIGIN);
	if(sequenceNo<request.SEQ) sequenceNo = request.SEQ;
	//check if the token is with this node
	// token is not with this node
		//request from its master(within quorum request)
		if(request.ORIGIN == request.sender){

			//if(request.ORIGIN != ID && HOLDER != ID){//cos we already added request from yourself in requestCS() method
			//pthread_mutex_lock(&sharedQLock);
			queue->add(request);
			//pthread_mutex_unlock(&sharedQLock);
			//}
			if(HOLDER == ID){
				if(!inCS){// Node is idle after completing CS.
					Packet top = queue->top();
					if(top.ORIGIN == ID){
						queue->remove(top.ORIGIN);
						EnterTheCS();
					}else
						sendToken();
				}
			}else if(HOLDER != -1){
				request.sender=ID;
				com.sendMessageToID(request,HOLDER);
			}
		}else{// request originated from out of this quorum
			if(HOLDER==ID){
					//pthread_mutex_lock(&sharedQLock);
					queue->add(request);
					//pthread_mutex_unlock(&sharedQLock);
					if(!inCS){// Node is idle after completing CS.
						Packet top = queue->top();
						if(top.TYPE !=-1)
							sendToken();
					}
				}
			//else drop the request
		}


	return true;
}

/*
 * return >0 if successful
 * 1 if entered CS here
 */
int Torum::receiveToken(Packet token){
	if(sequenceNo<token.SEQ) sequenceNo = token.SEQ;
	//pthread_mutex_lock(&sharedQLock);
	Packet top = queue->top();
	if(top.TYPE == -1){
		printf("receiveToken: queue top returned empty\n");
	}
	//pthread_mutex_unlock(&sharedQLock);

	if(ID == top.ORIGIN || token.ORIGIN == CONTROLLER_ID){// if current node is top of the queue
		HOLDER = ID;
		struct Packet havetkn;
		havetkn.TYPE = HAVE_TOKEN;
		havetkn.ORIGIN = ID;
		havetkn.sender = ID;
		havetkn.SEQ = sequenceNo;
		for(int i=0;i<quorumsize;i++){
			com.sendMessageToID(havetkn,quorum[ID][i]);
		}
		if(ID == top.ORIGIN){
			EnterTheCS();
		}
	}else{
		// sends token to the request on top of the queue
		//Note: we are not passing the same packet that we received
		sendToken();
	}
	return true;
}

bool Torum::receiveRelease(Packet release){
	if(sequenceNo<release.SEQ) sequenceNo = release.SEQ;
	HOLDER = -1;
	//pthread_mutex_lock(&sharedQLock);
	queue->updateTorumQ(quorum,quorumsize,ID);
	//pthread_mutex_unlock(&sharedQLock);
	return true;
}

bool Torum::receiveHaveTkn(Packet havtkn){
	if(sequenceNo<havtkn.SEQ) sequenceNo = havtkn.SEQ;
	HOLDER = havtkn.ORIGIN;
	//pthread_mutex_lock(&sharedQLock);
	Packet ret = queue->remove(havtkn.ORIGIN);
	if(ret.TYPE != -1){
	Packet top = queue->top();
	if(top.TYPE == -1){
			printf("receiveToken: queue top returned empty\n");
					return false;
		}
	//pthread_mutex_unlock(&sharedQLock);
	if(top.TYPE!=-1)
		com.sendMessageToID(top,HOLDER);
	return true;
	}else{
		printf("receiveHaveTkn: queue top returned empty/not found\n");
		return false;
	}
}

bool Torum::sendToken(){
	Packet top = queue->top();
	if(top.TYPE == -1){
			printf("sendToken: queue top returned empty\n");
					//return false;
		}
	Packet ret = queue->remove(top.ORIGIN);
	if(ret.TYPE==-1){
		printf("sendToken: queue remove returned empty/not found\n");
		//return false;
	}else{
		if(HOLDER == ID){
		//token was with you and you are granting it to some one else
		//Release is sent only by such nodes and not arbitrators of Token
			struct Packet release;
			release.TYPE = RELEASE;
			release.ORIGIN = ID;
			release.sender = ID;
			release.SEQ = sequenceNo;
			for(int i=0;i<quorumsize;i++){
					com.sendMessageToID(release,quorum[ID][i]);
			}
			//pthread_mutex_lock(&sharedQLock);
			queue->updateTorumQ(quorum,quorumsize,ID);
			//pthread_mutex_unlock(&sharedQLock);
		}
	HOLDER = -1;
	struct Packet token;
		token.TYPE = SEND_TOKEN;
		token.ORIGIN = ID;
		token.sender = ID;
		token.SEQ = sequenceNo;
		com.sendMessageToID(token,top.sender);
		return true;
	}
}

void Torum::writeToFile(string filename,string line){
	ofstream myfile (filename.c_str(),ios::out | ios::app);
	  if (myfile.is_open())
	  {
	    myfile <<line<<endl;
	    myfile.close();
	  }
	  else cout << "Unable to open file";
}
bool Torum::EnterTheCS(){
	inCS = true;
	flagforCS =true;
	printf("\n******Node '%d' in CRITICAL SECTION******\n",ID);
	string str ="";
	char buff[4095];
	sprintf(buff,"Node %d entered CS, Seq: %ld \n",ID,sequenceNo);
	writeToFile("Resource.txt",buff);
	sleep(1);
	inCS = false;
}

void Torum::displayStatus(){
	printf("NODE STATUS: ID:%d, Seq=%d, Holder=%d, Queue Size=%d\n",ID,sequenceNo,HOLDER,queue->size());
}


