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
	hqueue = new LexiQueue();
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
			if(ID!=quorum[ID][i])
				com.sendMessageToID(request,quorum[ID][i]);
		}
		receiveRequest(request);
	}else{
		queue->add(request);
		if(HOLDER != ID){
			temppk = request;
			tempDest = HOLDER;
			hqueue->add(request);
			com.sendMessageToID(request,HOLDER);
		}else if(HOLDER == ID){
			Packet top = queue->top();
			if(top.TYPE == -1){//queue is empty
				printf("requestCS: queue top empty\n");
			}
			if(top.ORIGIN == ID){
				if(temppk.ORIGIN == top.ORIGIN && temppk.SEQ==top.SEQ){
					tempDest = 999;
					temppk.ORIGIN = 999;
				}
				queue->displayContents();
				queue->remove(top.ORIGIN);
				queue->displayContents();
				EnterTheCS();
				Packet top2 = queue->top();
				if(top2.TYPE!=-1){
				Packet next = queue->remove(top2.ORIGIN,top2.SEQ);
				receiveRequest(next);
				}
			}else{//if there is another nodes req on top of queue
				for(int i=0;i<quorumsize;i++){
					if(ID !=quorum[ID][i])
						com.sendMessageToID(request,quorum[ID][i]);
				}
				sendToken();
			}
		}
	}
	return true;
}
bool Torum::isMaster(int master,int slave){
	bool ret = false;
	for(int i =0;i<quorumsize;i++){
		if(quorum[master][i] == slave){
			ret = true;
		}
	}
	return ret;
}
bool Torum::receiveRequest(Packet request){
printf("request from %d, sender:%d, seq:%d\n",request.ORIGIN,request.sender,request.SEQ);
	if(sequenceNo<request.SEQ) sequenceNo = request.SEQ;
	//check if the token is with this node
	// token is not with this node
	//request from its master(within quorum request)
	if(request.ORIGIN == request.sender && isMaster(request.sender,ID)){

		//if(request.ORIGIN != ID && HOLDER != ID){//cos we already added request from yourself in requestCS() method
		//pthread_mutex_lock(&sharedQLock);
		queue->add(request);
		//pthread_mutex_unlock(&sharedQLock);
		//}
		if(HOLDER == ID){
			if(!inCS){// Node is idle after completing CS.
				Packet top = queue->top();
				if(top.ORIGIN == ID){
					if(temppk.ORIGIN == top.ORIGIN && temppk.SEQ==top.SEQ){
						tempDest = 999;
						temppk.ORIGIN = 999;
					}
					if(hqueue->contains(top.ORIGIN,top.SEQ))
						hqueue->remove(top.ORIGIN,top.SEQ);
					queue->remove(top.ORIGIN);
					EnterTheCS();
					Packet top2 = queue->top();
					if(top2.TYPE!=-1){
						Packet next = queue->remove(top2.ORIGIN,top2.SEQ);
						receiveRequest(next);
						}
				}else{
					/*for(int i=0;i<quorumsize;i++){
						if(ID !=quorum[ID][i])
							com.sendMessageToID(request,quorum[ID][i]);
					}*/
					sendToken();
				}
			}
		}else if(HOLDER != -1 && HOLDER != request.ORIGIN){
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
	printf("In receive token, Token. origin:%d, sender:%d,seq:%d\n",token.ORIGIN,token.sender,token.SEQ);
	if(sequenceNo<token.SEQ) sequenceNo = token.SEQ;
	//pthread_mutex_lock(&sharedQLock);
	Packet top = queue->top();
	if(top.TYPE == -1){//if queue empty just keep the token with you
		printf("receiveToken: queue top returned empty\n");
		HOLDER = ID;
		struct Packet havetkn;
		havetkn.TYPE = HAVE_TOKEN;
		havetkn.ORIGIN = ID;
		havetkn.sender = ID;
		havetkn.SEQ = sequenceNo;
		for(int i=0;i<quorumsize;i++){
			if(ID!=quorum[ID][i])
				com.sendMessageToID(havetkn,quorum[ID][i]);
		}
		receiveHaveTkn(havetkn);
	}else if(ID == top.ORIGIN || token.ORIGIN == CONTROLLER_ID){// if current node is top of the queue
		HOLDER = ID;
		struct Packet havetkn;
		havetkn.TYPE = HAVE_TOKEN;
		havetkn.ORIGIN = ID;
		havetkn.sender = ID;
		havetkn.SEQ = sequenceNo;
		for(int i=0;i<quorumsize;i++){
			if(ID!=quorum[ID][i])
				com.sendMessageToID(havetkn,quorum[ID][i]);
		}
		if(token.ORIGIN == CONTROLLER_ID){
			HOLDER = ID;
		}
		if(ID == top.ORIGIN){
			if(temppk.ORIGIN == top.ORIGIN && temppk.SEQ==top.SEQ){
				tempDest = 999;
				temppk.ORIGIN = 999;
			}
			if(hqueue->contains(top.ORIGIN,top.SEQ))
				hqueue->remove(top.ORIGIN,top.SEQ);
			queue->remove(top.ORIGIN,top.SEQ);
			EnterTheCS();
			Packet top2 = queue->top();
			if(top2.TYPE!=-1){
				Packet next = queue->remove(top2.ORIGIN,top2.SEQ);
				receiveRequest(next);
				}
		}
	}else{
		// sends token to the request on top of the queue
		//Note: we are passing the same packet that we received, just update sender variable
		token.sender = ID;
		com.sendMessageToID(token,top.sender);
		//sendToken();
	}
	return true;
}

bool Torum::receiveHaveTkn(Packet havtkn){
	printf("In reveiveHavTkn: Origin:%d\n");
	if(sequenceNo<havtkn.SEQ) sequenceNo = havtkn.SEQ;
	HOLDER = havtkn.ORIGIN;
	//if the queue has request from the havetoken origin
	//remove it as the origins request has been satisfied
	Packet ret = queue->remove(havtkn.ORIGIN);
	if(ret.TYPE == -1){
		printf("receiveHaveTkn: queue remove returned empty/not found\n");
	}
	//now as we know the token holder, send the request
	//waiting on top of our queue to holder

	LexiQueue temp;
	int size = queue->size();
	for(int i=0;i<size;i++){
		Packet top = queue->top();
			if(HOLDER != top.ORIGIN){//dont sent origins own request to origin
				com.sendMessageToID(top,HOLDER);
				break;
			}else{
				temp.add(queue->remove(top.ORIGIN));
			}
	}
	while(!temp.isEmpty()){
		queue->add(temp.remove(temp.top().ORIGIN));
	}
	return true;
}

bool Torum::receiveRelease(Packet release){
	if(sequenceNo<release.SEQ) sequenceNo = release.SEQ;
	/*if(HOLDER == tempDest){
		Packet temp = queue->remove(ID,temppk.SEQ);
		if(temp.TYPE !=-1){
			queue->add(temp);
			for(int i=0;i<quorumsize;i++){
				if(ID !=quorum[ID][i])
					com.sendMessageToID(temp,quorum[ID][i]);
			}
		}
	}
	tempDest = 999;
	temppk.ORIGIN = 999;*/
	//pthread_mutex_lock(&sharedQLock);
	//queue->updateTorumQ(quorum,quorumsize,ID);
	//pthread_mutex_unlock(&sharedQLock);
	HOLDER = -1;
	return true;
}

bool Torum::sendToken(){
	printf("In sendToken(), ");
	Packet top = queue->top();
	if(top.TYPE == -1){
			printf("\nsendToken: queue top returned empty\n");
					//return false;
	}else{//if queue not empty
		printf("top of queue is Origin:%d, Sender:%d\n",top.ORIGIN,top.sender);
		if(HOLDER == ID){
		//token is with you and you are granting it to some one else
		//Release is sent only by such nodes and not arbitrators of Token

			//first remove the request from he queue and then
			//send token to its origin
			Packet ret = queue->remove(top.ORIGIN);
				if(ret.TYPE==-1){
					printf("sendToken: queue remove returned empty/not found\n");
					//return false;
				}
			queue->updateTorumQ(quorum,quorumsize,ID);
			struct Packet release;
			release.TYPE = RELEASE;
			release.ORIGIN = ID;
			release.sender = ID;
			release.SEQ = sequenceNo;
			for(int i=0;i<quorumsize;i++){
				if(ID!=quorum[ID][i])
					com.sendMessageToID(release,quorum[ID][i]);
			}
			receiveRelease(release);
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
	printf("\n******Node '%d' in CRITICAL SECTION******\n\n",ID);
	int sockfd = com.connectToServer(CONTROLLER_IP,LISTEN_PORT_CS);
	Packet p;
	p.TYPE = ENTER_CS;
	p.ORIGIN = ID;
	p.sender = ID;
	com.writeToSocket(sockfd,&p,sizeof(p));
	com.closeSocket(sockfd);
	sleep(60);
	p.TYPE = END_CS;
	sockfd = com.connectToServer(CONTROLLER_IP,LISTEN_PORT_CS);
	com.writeToSocket(sockfd,&p,sizeof(p));
	int confirmation=0;
	com.readFromSocket(sockfd,&confirmation,sizeof(int));
	if(confirmation != 1){
		printf("Mutex voilated\n");
	}
	com.closeSocket(sockfd);

	char buff[4095];
	sprintf(buff,"Node %d entered CS, Seq: %ld \n",ID,sequenceNo);
	writeToFile(CS_FILENAME,buff);
	sleep(1);
	inCS = false;
}

void Torum::displayStatus(){
	printf("NODE STATUS: ID:%d, Seq=%d, Holder=%d, Queue Size=%d\n",ID,sequenceNo,HOLDER,queue->size());
	queue->displayContents();
}


