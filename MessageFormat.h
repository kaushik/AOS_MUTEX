#ifndef MESSAGEFORMAT_H
#define MESSAGEFORMAT_H

#pragma once

#define REQUEST 0
#define HAVE_TOKEN 1
#define RELEASE 2
#define SEND_TOKEN 3
#define MAKE_REQUEST 11
#define END_PROCESS 12

#define CONTROLLER_ID 45
#define CONTROLLER_IP "10.176.67.108"

#define LISTEN_PORT 3492
#define LISTEN_PORT2 3592
#define LISTEN_PORT3 3692
#define LISTEN_PORT_END 3793


#define MAXLENGTH_IP_ADDR 16

	/*struct AlgoMsg{
		int TYPE; // 0: request_token, 1: have-token, 2: release, 3:send-token
		int ORIGIN;
		long SEQ;
		int sender;
	};*/
	
	struct Packet{
		int TYPE; // 0: request_token, 1: have-token, 2: release, 3:send-token
		int ORIGIN;
		long SEQ;
		int sender;
	}__attribute__((packed));

#endif
