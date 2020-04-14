#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<fcntl.h>

#define SERVER_PORT 7902
#define MAX_LINE 256
#define MAX_PENDING 5

 // structure of packets
struct packet
{
	short type; 		    // type of packet ie 121, 221 etc
    short seqNumber;        // sequence number for packets 
    char uName[MAX_LINE];	// user name
	char mName[MAX_LINE];	// machine name
	char data[MAX_LINE];	// data sent        
};
// structure for registration table
struct registrationTable 
{
    int port;               // log port number
    int sockid;             // log socket identification
    char mName[MAX_LINE];   // name of client machine
    char uName[MAX_LINE];   // name of user
    char client_info;
};
// table index to hold multiple clients
int tableIndex = 0;
// registration table
struct registrationTable table[10]; 
// global mutex variable to lock table for access
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
            Method to fill out registration table 
*/
void *join_handler(struct registrationTable *clientData)
{
    int newsock;
    struct packet packet_reg;
    struct packet confirm;              // packet expected for confirmation
    struct sockaddr_in clientAddr;	    // client address
    newsock = clientData->sockid;
    
    // receive registration packet 1
    if(recv(newsock,&packet_reg,sizeof(packet_reg),0) <0)
    {
        printf("\nCould not receive registration packet 1 (Join Handler)\n");
        exit(1);
    }
    // receive registration packet 2 & update table
    if(recv(newsock,&packet_reg,sizeof(packet_reg),0) <0)
    {
        printf("\nCould not receive registration packet 2 (Join Handler)\n");
        exit(1);
    } 
    else
    {
        // lock mutex variable to access or manipulate table
        pthread_mutex_lock(&my_mutex);
        // copy information to registration table
        table[tableIndex].port = clientAddr.sin_port;
        table[tableIndex].sockid = newsock;
        strcpy(table[tableIndex].mName, packet_reg.mName);
        strcpy(table[tableIndex].uName, packet_reg.uName);
        printf("Registration received (join_handler)\nUser: %s\nMachine: %s\nType: %d\nSocket ID: %d\n\n", table[tableIndex].uName, table[tableIndex].mName, ntohs(packet_reg.type), newsock);
        // construct confirmation packet
        confirm.type = htons(221);			        // set type to 22
        confirm.seqNumber = htons(0);               // set sequence number
        strcpy(confirm.mName, table[tableIndex].mName);  // set machine name
        strcpy(confirm.uName, table[tableIndex].uName);  // set user name  
        // unlock mutex variable so multicaster can access
        pthread_mutex_unlock(&my_mutex);
        // send back confirmation                            
        if(send(newsock,&confirm,sizeof(confirm),0) < 0)
        {
		    printf("\nError could not send confirmation packet (Join Handler)\n");
		    exit(1);
	    }
        // increment tableIndex for next registration
        tableIndex = tableIndex + 1;     
    }
    // exit the thread
    pthread_exit(NULL);
}






/*
            Method to multicast chat to registered participants
*/
void *multicaster(void *arg)
{
    char *file;                 // file to read from
    char text[100];             // string to hold characters being sent
    struct packet chat;         // chat packet to be sent to client
    int fd;                     // file number
    int i;                      // counter to send to all registered machines
    int this_socket;             // integer to hold the socket id
    file = "input.txt";         // file name
    fd = open(file,O_RDONLY,0); // open the file
    int nread;
    int seqNum = 0;

    /*
            While true, check if the table has anyone registered
    */
    while(1)
    {
        // if the table has len > 0
        if (tableIndex > 0)
        {
            // lock mutex variable to read from table
            pthread_mutex_lock(&my_mutex);            
            // read the file and store contents in "text"
            nread = read(fd, text, 100);
            for (i = 0; i < tableIndex; i++)
            {
                this_socket = table[i].sockid;
                chat.type = htons(131);
                chat.seqNumber = htons(seqNum);
                strcpy(chat.data, text);
                strcpy(chat.uName, table[i].uName);
                strcpy(chat.mName, table[i].mName); 
                if(send(this_socket,&chat,sizeof(chat),0) < 0)
                {
		            printf("\nError could not send confirmation packet (Multicaster)\n");
		            exit(1);
	            }
                else
                {
                    printf("\nPacket sent:\nUser %s\nMachine: %s\nSocket: %d", chat.uName, chat.mName, this_socket);
                    seqNum = seqNum + 1;
                }             
            }
            // unlock mutex variable so join handler can add to table if new clients register
            pthread_mutex_unlock(&my_mutex);
        }
        sleep(5);
    }
}

int main(int argc, char* argv[])
{
	// declare variables
	struct packet packet_reg;	            // packet to be sent 
    struct registrationTable client_info;   // client info
    struct packet chat; 	                // initialize chat packet
    struct sockaddr_in sin;		            // initialize socket
	struct sockaddr_in clientAddr;	        // client address
    pthread_t threads[2];                   // array of threads
	char buf[MAX_LINE];		                // buffer to store data
    char clientname[MAX_LINE];
	int s, new_s;			                // used to store socket IDs
	int len;			                    // used to store length of buffer
    void *exit_value;
    

    /*
                      Setup For connections    
    */
	/* setup passive open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
		perror("tcpserver: socket (Main)");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));	// initialize sin to 0
	sin.sin_family = AF_INET;		    // set address family
	sin.sin_addr.s_addr = INADDR_ANY;	// get IPs of the machine
	sin.sin_port = htons(SERVER_PORT);	// set port to server port
	
	// bind port to server
	if(bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
		perror("tcpclient: bind (Main)");
		exit(1);
	}
	listen(s, MAX_PENDING);
    // create thread for chat multicasting
    pthread_create(&threads[1],NULL,multicaster,NULL);


    /*
                    Main loop to accept connections
    */
	/* wait for connection, then receive and print text */
	while(1)
    {
        // new socket accept
		if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0)
        {
			perror("tcpserver: accept (Main)");
			exit(1);
		}
        gethostname(clientname, 256);
        // receive registration packet
        if(recv(new_s,&packet_reg,sizeof(packet_reg),0) <0)
        {
            printf("\nCould not receive registration packet (Main)\n");
            exit(1);
        }
        else
        {
            printf("\nRegistration Received (main)\n");
            client_info.sockid = new_s;
            client_info.port = clientAddr.sin_port;
            strcpy(client_info.mName, packet_reg.mName);
            strcpy(client_info.uName, packet_reg.uName);
            // create join handler
            pthread_create(&threads[0],NULL,(void *)join_handler,&client_info);   
            pthread_join(threads[0],&exit_value);
        }		
		//close(new_s);
	}
}











