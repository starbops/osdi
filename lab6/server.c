#include <stdio.h>          // for printf() and fprintf() 
#include <sys/socket.h>     // for socket(), bind(), and connect() 
#include <arpa/inet.h>      // for sockaddr_in and inet_ntoa() 
#include <stdlib.h>         // for atoi() and exit() 
#include <string.h>         // for memset() 
#include <unistd.h>         // for close() 
#include <sys/time.h>       // for struct timeval {} 
#include <fcntl.h>          // for fcntl() 
#include<sys/syscall.h>
#define MAXPENDING 5        // Maximum outstanding connection requests
#define MAXCLIENT  100      // Maximum client connections
#define RCVBUFSIZE 1024     // Size of receive buffer 

int     CreateTCPServerSocket( unsigned short );
int     AcceptTCPConnection( int );
int     HandleTCPClient( int );
int	mykey=0;
int main( int argc, char *argv[] )
{
    int             *servSock;        // Socket descriptors for server 
    int             maxDescriptor;    // Maximum socket descriptor value 
    fd_set          sockSet;          // Set of socket descriptors for select() 
    long            timeout=100;          // Timeout value given on command-line 
    struct timeval  selTimeout;       // Timeout for select() 
    
    int cliSock[MAXCLIENT];           // Client Socket Set
    
    int running = 1;                 // 1 if server should be running; 0 otherwise 
    
    int noPorts;                     // Number of port specified on command-line 
    int port;                        // Looping variable for ports 
    unsigned short portNo;           // Actual port number 
    
    int i;                           // For loop use
    
    // Test for correct number of arguments
    
    if ( argc < 3 )     
    {
        fprintf( stderr, "Usage:  %s <Socket Key> <Port 1> ...\n", argv[0]);
        exit(1);
    }

    mykey = atoi(argv[1]);        // First arg: Timeout 
    noPorts = argc - 2;             // Number of ports is argument count minus 2 

    // Allocate list of sockets for incoming connections 
    servSock = (int *) malloc( noPorts * sizeof(int) );    
  
    // Create list of ports and sockets to handle ports 
    for ( port = 0; port < noPorts; port++ )
    {
        // Add port to port list, skip first two arguments 
        portNo = atoi( argv[port + 2] ); 

        // Create port socket 
        servSock[port] = CreateTCPServerSocket( portNo );  
    }
    
    // Initialize the client socket pool
    for( i = 0 ; i < MAXCLIENT ; i++ )
    {
        cliSock[i] = -1;
    }

    printf( "Starting server:  Hit return to shutdown\n" );
    while ( running )
    {
        // Zero socket descriptor vector and set for server sockets 
        // This must be reset every time select() is called 
        FD_ZERO( &sockSet );
        
        // Add keyboard to descriptor vector 
        FD_SET( STDIN_FILENO, &sockSet );
        
        // Initialize maxDescriptor for use by select() 
        maxDescriptor = -1;
        
        // Add server sockets to descriptor vector 
        for ( port = 0; port < noPorts; port++ )
        {
            FD_SET( servSock[port], &sockSet );
            
            // Determine if new descriptor is the largest 
            if ( servSock[port] > maxDescriptor ) 
            {
                maxDescriptor = servSock[port];
            }
        }
        
        // Add client sockets to descriptor vector 
        for ( i = 0; i < MAXCLIENT; i++ )
        {
            if ( cliSock[i] > 0 ) 
            {
                FD_SET( cliSock[i], &sockSet );
            }
            
            if ( cliSock[i] > maxDescriptor ) 
            {
                maxDescriptor = cliSock[i];
            }
        }
        
        // Timeout specification 
        // This must be reset every time select() is called 
        selTimeout.tv_sec = timeout;       // timeout (secs.) 
        selTimeout.tv_usec = 0;            // 0 microseconds 

        // Suspend program until descriptor is ready or timeout 
        if ( select( maxDescriptor + 1, &sockSet, NULL, NULL, &selTimeout ) == 0 )
        {
            printf("No echo requests for %ld secs...Server still alive\n", timeout);
        }
        else 
        {
            // Check keyboard 
            if (FD_ISSET(0, &sockSet)) 
            {
                printf("Shutting down server\n");
                getchar();
                running = 0;
                continue;
            }
            
            // Check Listening Sockets
            for ( port = 0; port < noPorts; port++ )
            {
                if (FD_ISSET( servSock[port], &sockSet ) )
                {
                    printf("Request on port %d:  ", port);
                    for( i = 0 ; i < MAXCLIENT ; i++ )
                    {
                        if( cliSock[i] < 0 ) 
                        {
                            cliSock[i] = AcceptTCPConnection( servSock[port] );
                            i = MAXCLIENT;
                        }
                    }
                }
            }
            
            // Check Client Sockets
            for ( i = 0 ; i < MAXCLIENT ; i++ )
            {
                if ( FD_ISSET( cliSock[i], &sockSet ) )
                {
                    if( HandleTCPClient( cliSock[i] ) == 0 )
                    {
                        printf( "Connection %d Shudown.\n", cliSock[i] );
                        close( cliSock[i] );
                        cliSock[i] = -1;
                    }
                }
            }
        }
    }

    // Close sockets 
    for ( port = 0; port < noPorts; port++ )
    {
        close( servSock[port] );
    }
    
    for ( i = 0; i < MAXCLIENT; i++ )
    {
        if( cliSock[i] > 0 )
        {
            close( cliSock[i] );
        }
    }
    
    // Free list of sockets 
    free( servSock );

    return 0;
}

int CreateTCPServerSocket(unsigned short port)
{
    int sock;                        // socket to create 
    struct sockaddr_in echoServAddr; // Local address 

    // Create socket for incoming connections 
    if ( ( sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        perror( "socket() failed" );
        exit(1);
    }
    
    // Construct local address structure 
    memset( &echoServAddr, 0, sizeof( echoServAddr ) );     // Zero out structure 
    echoServAddr.sin_family = AF_INET;                      // Internet address family 
    echoServAddr.sin_addr.s_addr = htonl( INADDR_ANY );     // Any incoming interface 
    echoServAddr.sin_port = htons( port );                  // Local port 

    // Bind to the local address 
    if ( bind(sock, (struct sockaddr *) &echoServAddr, sizeof( echoServAddr ) ) < 0 )
    {
        perror( "bind() failed" );
        exit(1);
    }
    
    // Mark the socket so it will listen for incoming connections 
    if ( listen( sock, MAXPENDING ) < 0 )
    {
        perror( "listen() failed" );
        exit(1);
    }

    /* Uncomment next line when you finished the system call */

    //syscall(SYS_setsockopt, sock, mykey);	//set server socket's fd,
    return sock;
}

int AcceptTCPConnection( int servSock )
{
    int                 clntSock;     // Socket descriptor for client 
    struct sockaddr_in  echoClntAddr; // Client address 
    unsigned int        clntLen;      // Length of client address data structure 

    // Set the size of the in-out parameter 
    clntLen = sizeof( echoClntAddr );
    
    // Wait for a client to connect 
    if ( ( clntSock = accept( servSock, (struct sockaddr *) &echoClntAddr, &clntLen ) ) < 0 )
    {
        perror("accept() failed");
        exit(1);
    }
    
    // clntSock is connected to a client! 
    
    printf("Handling client %s(%d)\n", inet_ntoa( echoClntAddr.sin_addr ), clntSock );

    /* Uncomment next line when you finished the system call */
    //syscall(SYS_setsockopt, clntSock, mykey); //set client socket's fd

    return clntSock;
}

int HandleTCPClient( int clntSocket )
{
    char    echoBuffer[RCVBUFSIZE];        // Buffer for echo string 
    int     recvMsgSize;                   // Size of received message 
    
    bzero( echoBuffer, RCVBUFSIZE );
    // Receive message from client 
    if ( ( recvMsgSize = recv( clntSocket, echoBuffer, RCVBUFSIZE, 0 ) ) < 0 )
    {
        perror("recv() failed");
        exit(1);
    }

    // Send received string and receive again until end of transmission 
    if ( recvMsgSize > 0 )      // zero indicates end of transmission 
    {
				printf("%s\n",echoBuffer);
        // Echo message back to client 
        if ( send( clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize )
        {
            perror( "send() failed" );
            exit(1);
        }
	
    }
    
    return recvMsgSize;
}
