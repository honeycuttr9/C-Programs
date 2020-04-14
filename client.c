#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>

#define SERVER_PORT 7902
#define MAX_LINE 256

int main(int argc, char* argv[])
{
	// structure of packets
	struct packet
    {
		short type; 		    // type of packet ie 121, 221 etc 
        short seqNumber;        // sequence number for a packet
		char uName[MAX_LINE];	// user name
		char mName[MAX_LINE];	// machine name
		char data[MAX_LINE];	// data sent
	};
	
	// declare variables
	struct packet packet_reg;	// packet to be sent 
    struct packet confirm;      // packet for registration confirmation                                    	
    struct packet chat;         // initialize chat packet    
    struct hostent *hp;		    // host IP
	struct sockaddr_in sin;		// socket 
	char *host;			        // hostname
	char buf[MAX_LINE];		    // buffer to send text
	int s;				        // variable to hold socket id
	int len;			        // length of buffer
    short seqNumChat;           // sequence number for chat data

	// check arguments
	if(argc == 3)
    {
		host = argv[1]; 		            // host = first argument
		strcpy(packet_reg.uName,argv[2]);	// username = second argument
	}
	else
    {
		// print error and exit
		fprintf(stderr, "usage:newclient server\n");
		exit(1);
	}

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if(!hp)
    {
		// if cant translate to IP, print error and exit
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}

	/* active open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
		perror("tcpclient: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));			            // initialize socket to 0
	sin.sin_family = AF_INET;				                // set socket to address family
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);	// copy host IP to socket address
	sin.sin_port = htons(SERVER_PORT);			            // set socket port to the port of the server

	// connect to host
	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}
	
	// construct registration packet
	packet_reg.type = htons(121);			    // set type to 121
    packet_reg.seqNumber = htons(1);
	gethostname(packet_reg.mName, MAX_LINE); 	// get host name

	
	// send registration packet 1st time                        
	if(send(s,&packet_reg,sizeof(packet_reg),0) < 0)
    {
		printf("\nError could not send packet \n");
		exit(1);
	}

    
    // send registration packet 2nd time
	if(send(s,&packet_reg,sizeof(packet_reg),0) < 0)
    {
		printf("\nError could not send packet \n");
		exit(1);
	}

    // send registration packet 3rd time
	if(send(s,&packet_reg,sizeof(packet_reg),0) < 0)
    {
		printf("\nError could not send packet \n");
		exit(1);
	}

    // print registration request
    //printf("Registration Request\nUser: %s\tMachine: %s\tType: %d\n\n", packet_reg.uName, packet_reg.mName, ntohs(packet_reg.type));

    // receive registration confirmation
    if(recv(s,&confirm,sizeof(confirm),0) <0)
    {          
        printf("\nCould not receive registration packet\n");    
        exit(1);
    }

    // print confirmation
    printf("Registration Confirm\nUser: %s\tMachine: %s\tType: %d\n\n", confirm.uName, confirm.mName, ntohs(confirm.type));
    
   	/* main loop: get and send lines of text */
    while(len = recv(s,&chat,sizeof(chat), 0))
    {                    			
            // print chat info
            printf("\nChat received\nUser: %s\tMachine: %s\tType: %d\n", chat.uName, chat.mName, ntohs(chat.type));
            printf("Sequence Number [%d] \n%s\n\n", ntohs(chat.seqNumber), chat.data);
	}
}
