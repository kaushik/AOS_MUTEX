#include "Starter.h"

Starter::Starter() {
	printf("In Starter\n");
	id=-1;
	quorumSize=0;
	NumNodes=0;
	//decideAlgorithm();
}

Starter::~Starter() {
	for(int i = 0; i < NumNodes; ++i)
		delete Quorum[i];
	delete Quorum;
}

void Starter::init(){
	char controllerIP[MAXLENGTH_IP_ADDR] = CONTROLLER_IP;
	//messageCounter=0;
	//char controllerIP[16] = "10.176.67.88";
	int port = LISTEN_PORT;
	registerAtController(controllerIP, port);
	decideAlgorithm();
	
}

void Starter::parseMsg(const string& s,const string& delim,std::vector<string>& tokens)
{
        size_t s0 = 0, s1 = string::npos;
        string token;
        while(s0 != string::npos)
          {

            s1 = s.find_first_of(delim, s0);

                // if(s1 != s0 && s1 != string::npos) // the second check added for space
				if(s1 != s0) // no need check Defect FIX  1,2 -> 2 will not be added in the list
            {
                  token = s.substr(s0, s1 - s0);

                if( token != "") 	
                {
                    //cout<<"Token :"<<token<<" length:"<<token.length()<<endl;
						//cout<<token<<endl;
					   tokens.push_back(token);
                }
            }
            s0 = s.find_first_not_of(delim, s1);
        }
        //EXIT
        token.clear();
        return;

    }

/*
 * send a message to controller when a node starts on a amachine.
 */
void Starter::registerAtController(char controllerIP[15],int port){
	//communication

	communication com;
	int sockfd = com.connectToServer(controllerIP,port);

	int z;
	char buf[128]={'\0',};
	z = gethostname(buf,sizeof buf);
	if ( z == -1 ) {
		fprintf(stderr, "%s: gethostname(2)\n",
		strerror(errno));
		exit(1);
	}
	int sd = 999;
	printf("\nContacted Controller, sent my hostname:%s\n",buf);
	com.writeToSocket(sockfd,buf,128);
	com.readFromSocket(sockfd,&id,sizeof(int));
	int qsize,nsize;
	com.readFromSocket(sockfd,&qsize,sizeof(int));
	com.readFromSocket(sockfd,&nsize,sizeof(int));
	//int size;
	//com.readFromSocket(sockfd,&size,sizeof(int));
	printf("Received ID: %d, Qsize: %d, NumNodes: %d\n",id,qsize,nsize);
	NumNodes = nsize;
	quorumSize = qsize;
	Quorum = new int*[nsize];
	for(int i = 0; i < nsize; ++i)
		Quorum[i] = new int[qsize];
	
	for (int i = 0; i < nsize; ++i) {
        for (int j = 0; j < qsize; ++j) {
            Quorum[i][j] = 0;
        }
    }

	puts("Getting quorum table\n");
	char recvdMsg[4096] = {'\0',};
	com.readFromSocket(sockfd,recvdMsg,4095);
	string recvdStr = recvdMsg;
	vector<string> recvdValues;
	string delimiter = ":";
	parseMsg(recvdStr,delimiter,recvdValues);
	/*
	for (int i=0; i<recvdValues.size();i++)
	{
		cout<<recvdValues[i]<<endl; //<-- extract values from this vector and push into the attay in 123 line
		
	}*/
	for(int i=0; i<nsize;i++)
	{
		for(int j=0; j<qsize;j++)
		{
			//cout<<"Here"<<endl;
			Quorum[i][j] = atoi(recvdValues[i*qsize+j].c_str());
			//cout<<"Here1"<<endl;
		}
	}
	
	for(int i=0;i<nsize;i++){
		for(int j=0;j<qsize;j++){
	
			printf("%d\t",Quorum[i][j]);
		}
		printf("\n");	
		
	}
	shutdown(sockfd,2);
	int k = com.closeSocket(sockfd);
	if (k < 0) {
			printf("\nError in Closing");
			exit(0);
	}else
		printf("Node disconnected from Controller\n");

}

void Starter::decideAlgorithm(){
	printf("\nIn decideAlgorithm() of Node\n");
	communication com;
	int serfd;
	int clifd = com.OpenListener(serfd,LISTEN_PORT2);
	//char buffer[4095]={'\0',};
	int intbuf;
	com.readFromSocket(clifd,&intbuf,sizeof(int));
	printf("recvd: %d\n",intbuf);
	puts("Getting ID to IP map\n");
	//assigining memory to mapIDtoIP
	mapIDtoIP = new char*[NumNodes];
	for(int i = 0; i < NumNodes; ++i){
		mapIDtoIP[i] = new char[MAXLENGTH_IP_ADDR];
	}

	char recvdMsg[4096] = {'\0',};
	strcpy(mapIDtoIP[0],"hello");
	com.readFromSocket(clifd,recvdMsg,4095);
	string recvdStr = recvdMsg;
	vector<string> recvdValues;
	string delimiter = ":";
	parseMsg(recvdStr,delimiter,recvdValues);
	for(int i=0;i<NumNodes;i++){
		strcpy(mapIDtoIP[i],recvdValues[i].c_str());
		printf("%d\t%s\n",i,mapIDtoIP[i]);
	}
	com.readFromSocket(clifd,CS_FILENAME,25);
	printf("File Name of Critical section resource is %s\n",CS_FILENAME);
	shutdown(clifd,2);
	com.closeSocket(clifd);
	com.closeSocket(serfd);
	int algo = intbuf;
	if(algo == 1){
		Algorithm1();
	}else if(algo == 2){
		Algorithm2();

	}else
		printf("Invalid input\n");

}

void Starter::Algorithm1(){
	printf("Starting Algorithm 1: Maekawa\n");
	mnode = MaekawaAlgorithm::getInstance();
	mnode->initialization();
	mnode->setProcessID(id);
	mnode->getQuorumTable(Quorum,quorumSize, NumNodes);

	mnode->com.setMapIDtoIP(mapIDtoIP);

	wqueue<Packet*> queue;
	pthread_t ListenerThread;
	pthread_create(&ListenerThread, NULL, MaekawaListen, (void *)&queue);

	sleep(1);
	pthread_t ProcessingThread;
	pthread_create(&ProcessingThread, NULL, MaekawaProcess,(void *)&queue);

	pthread_join(ListenerThread, NULL);
	pthread_join(ProcessingThread, NULL);

	printf("Maekawa Algorithm execution done\n");
}

void Starter::Algorithm2(){
	printf("Starting Algorithm 2: Torum\n");
	node = Torum::getInstance();
	node->init();
	node->setID(id);
	node->getQuorumTable(Quorum,quorumSize,NumNodes);
	//000
	node->com.setMapIDtoIP(mapIDtoIP);
	strcpy(node->CS_FILENAME,CS_FILENAME);

	wqueue<Packet*> mqueue;

	pthread_t ListenerThread;
	pthread_create(&ListenerThread, NULL, TorumListen, (void *)&mqueue);

	sleep(1);
	pthread_t ProcessingThread;
	pthread_create(&ProcessingThread, NULL, TorumProcess,(void *)&mqueue);

	pthread_join(ListenerThread, NULL);
	pthread_join(ProcessingThread, NULL);

	printf("Torum execution done\n");
}

void endProcessing(int id, long messageCounter,long timeCounter){
	printf("Ending...\nID:%d, MessageCount:%ld, TimeCount:%ld",id,messageCounter,timeCounter);

	sleep(3);
	communication com;
	char cIP[MAXLENGTH_IP_ADDR]=CONTROLLER_IP;
	int sockFd=com.connectToServer(cIP,LISTEN_PORT_END);
	int nodeid = id;
	com.writeToSocket(sockFd,&nodeid,sizeof(int));
	long counter=messageCounter;
	com.writeToSocket(sockFd,&counter,sizeof(long));
	counter = timeCounter;
	com.writeToSocket(sockFd,&counter,sizeof(long));

	shutdown(sockFd,0);
	close(sockFd);
	printf("Bye!!!");
	exit(0);
}

void *TorumListen(void* mqueue) {
	printf("TorumListener Thread created\n");
	wqueue<Packet*> *m_queue=((wqueue<Packet*>*)mqueue);
	communication com;
	com.serverListen(LISTEN_PORT3,m_queue);
	return NULL;
}


void *TorumProcess(void* mqueue) {
	printf("TorumProcess Thread created\n");
	long messageCounter = 0;
	long timeCounter = 0;
	wqueue<Packet*>*m_queue=(wqueue<Packet*>*)mqueue;

	//getting time for CS grant
	long utime, seconds, useconds;
	queue<struct timeval> timequeue;


	Torum *node = Torum::getInstance();
	sleep(5);
	// Remove 1 item at a time and process it. Blocks if no items are
	// available to process.
	for (int i = 0;; i++) {
		node->displayStatus();
		printf("TorumProcessing Thread , loop %d - waiting for item...\n", i);
		Packet* item = m_queue->remove();
		//printf("thread  loop %d - got one item\n", i);
		//printf("Received: messageType - %d, SEQ number - %ld\n",item->TYPE, item->SEQ);

		if (item->TYPE == SEND_TOKEN){
			printf("SEND_TOKEN recieved from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
			node->flagforCS = false;

			node->receiveToken(*item);

			if(node->flagforCS){//node entered CS using this Token
				struct timeval end;
				gettimeofday(&end, NULL);
				struct timeval start = timequeue.front();
				timequeue.pop();
				seconds  = end.tv_sec  - start.tv_sec;
				useconds = end.tv_usec - start.tv_usec;
				utime = ((seconds) * 1000000 + useconds) + 0.5;
				timeCounter += utime;
			}
			if(item->sender != node->ID)
			messageCounter++;
		}else if (item->TYPE == MAKE_REQUEST){
			printf("MAKE_REQUEST recieved from Controller %d and packet type is %d\n",item->ORIGIN,item->TYPE);

			struct timeval start;
			gettimeofday(&start, NULL);
			timequeue.push(start);

			node->requestCS();
		}else if (item->TYPE == HAVE_TOKEN){
			if(item->sender != node->ID)
						messageCounter++;
			printf("HAVE_TOKEN recieved from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
			node->receiveHaveTkn(*item);
		}else if (item->TYPE == RELEASE){
			if(item->sender != node->ID)
						messageCounter++;
			printf("RELEASE recieved from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
			node->receiveRelease(*item);
		}else if (item->TYPE == REQUEST){
			if(item->sender != node->ID)
						messageCounter++;
			printf("REQUEST recieved from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
			node->receiveRequest(*item);
		}
		else if(item->TYPE == END_PROCESS)
		{
			printf("END_PROCESS recieved from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
			endProcessing(node->ID,messageCounter,timeCounter);
		}
		delete item;
	}
	return NULL;
}

void *MaekawaListen(void* iqueue) {
	printf("MaekawaListener Thread created\n");
	wqueue<Packet*> *m_queue=((wqueue<Packet*>*)iqueue);
	communication com;
	com.serverListen(LISTEN_PORT3,m_queue);
	return NULL;
}


void *MaekawaProcess(void* iqueue) {
	printf("MaekawaProcess Thread created\n");

	long messageCounter = 0;
	long timeCounter = 0;
	//getting time for CS grant
	long utime, seconds, useconds;
	queue<struct timeval> timequeue;

	wqueue<Packet*>*m_queue=(wqueue<Packet*>*)iqueue;
	MaekawaAlgorithm *mnode = MaekawaAlgorithm::getInstance();
	sleep(5);
	// Remove 1 item at a time and process it. Blocks if no items are
	// available to process.
	for (int i = 0;; i++) {
		printf("MaekawaProcessing Thread , loop %d - waiting for item...\n", i);
		Packet* item = m_queue->remove();
		//printf("thread  loop %d - got one item\n", i);
		printf("Received: messageType - %d, SEQ number - %ld\n",item->TYPE, item->SEQ);

        switch (item->TYPE) {
            case MAKE_REQUEST:
                printf(" ## MAKE_REQUEST received from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
                struct timeval start;
                gettimeofday(&start, NULL);
                timequeue.push(start);
                mnode->receiveMakeRequest(*item);

                break;
            case REQUEST:
                printf(" ## REQUEST received from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
                mnode->receiveRequest(*item);
                if(item->ORIGIN != mnode->processID)
                	messageCounter++;
                break;
            case INQUIRE:
                printf(" ## INQUIRE received from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
                mnode->receiveInquire(*item);
                if(item->ORIGIN != mnode->processID)
                    messageCounter++;
                break;
            case FAILED:
                printf(" ## FAILED received from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
                mnode->receiveFailed(*item);
                if(item->ORIGIN != mnode->processID)
                    messageCounter++;
                break;
            case RELEASE:
                printf(" ## RELEASE received from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
                mnode->receiveRelease(*item);
                if(item->ORIGIN != mnode->processID)
                    messageCounter++;
                break;
            case RELINQUISH:
                printf(" ## RELINQUISH received from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
                mnode->receiveRelinquish(*item);
                if(item->ORIGIN != mnode->processID)
                    messageCounter++;
                break;
            case LOCKED:
                printf(" ## LOCKED received from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
                mnode->flagforCS = false;
                mnode->receiveLocked(*item);
                if(mnode->flagforCS){//node entered CS using this Token
					struct timeval end;
					gettimeofday(&end, NULL);
					struct timeval start = timequeue.front();
					timequeue.pop();
					seconds  = end.tv_sec  - start.tv_sec;
					useconds = end.tv_usec - start.tv_usec;
					utime = ((seconds) * 1000000 + useconds) + 0.5;
					timeCounter += utime;
				}
				if(item->ORIGIN != mnode->processID)
				messageCounter++;
                break;
            case END_PROCESS:
            	printf("END_PROCESS recieved from Node %d and packet type is %d\n",item->ORIGIN,item->TYPE);
            	delete item;
            	endProcessing(mnode->processID,messageCounter,timeCounter);
            	break;
            default:
                break;
        }
        delete item;
	}
	return NULL;
}

