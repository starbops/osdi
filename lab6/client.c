#include <stdio.h>      // for printf() and fprintf() 
#include <sys/socket.h> // for socket(), connect(), send(), and recv() 
#include <arpa/inet.h>  // for sockaddr_in and inet_addr() 
#include <stdlib.h>     // for atoi() and exit() 
#include <string.h>     // for memset() 
#include <unistd.h>     // for close() 
#include<sys/syscall.h>
#define STRBUFSIZE 1024 // Size of receive buffer 

void DieWithError(char *errorMessage);  // Error handling function 
int mykey=0;
int main(int argc, char *argv[])
{
    int sock;                        // Socket descriptor 
    struct sockaddr_in echoServAddr; // Echo server address 
    unsigned short echoServPort;     // Echo server port 
    char *servIP;                    // Server IP address (dotted quad)
    char echoString[STRBUFSIZE];     // Buffer for echo string  
    char echoBuffer[STRBUFSIZE];     // Buffer for echo string 
    unsigned int echoStringLen;      // Length of string to echo 
    int bytesRcvd, totalBytesRcvd;   // Bytes read in single recv() and total bytes read 
    int running = 1;                 // Check the loop state

    if ( ( argc < 3 ) || (argc > 4 ) )    // Test for correct number of arguments 
    {
       fprintf( stderr, "Usage: %s <Socket key> <Server IP> [<Echo Port>]\n", argv[0] );
       exit(1);
    }
    mykey = atoi(argv[1]); 
    servIP = argv[2];             // First arg: server IP address (dotted quad) 

    if ( argc == 4 )
    {
        // Use given port, if any 
        echoServPort = atoi(argv[3]); 
    }
    else
    {
        // 7 is the well-known port for the echo service
        echoServPort = 7;  
    } 

    // Create a reliable, stream socket using TCP 
    if ( ( sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        perror("socket() failed");
        exit(1);
    }
		
		/* Uncomment next line when you finished the system call */
    //syscall(SYS_setsockopt, sock, mykey);	//set client socket's fd
		
    // Construct the server address structure 
    memset(&echoServAddr, 0, sizeof(echoServAddr));     // Zero out structure 
    echoServAddr.sin_family      = AF_INET;             // Internet address family 
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   // Server IP address 
    echoServAddr.sin_port        = htons(echoServPort); // Server port 

    // Establish the connection to the echo server 
    if ( connect( sock, (struct sockaddr *) &echoServAddr, sizeof( echoServAddr ) ) < 0 )
    {
        perror( "connect() failed" );
        exit(1);
    }
    
    while ( running )
    {
        bzero( echoString, STRBUFSIZE );
        bzero( echoBuffer, STRBUFSIZE );
        
        printf("Enter(q for exit): \n"); 
        
        fgets( echoString, STRBUFSIZE, stdin ); 
        
        echoStringLen = strlen( echoString );
        
        if ( echoStringLen == 2 && echoString[0] == 'q' && echoString[1] == '\n' )
        {
            running = 0;
            continue;
        }
        
        if ( send(sock, echoString, echoStringLen, 0) != echoStringLen )
        {
            perror( "send() sent a different number of bytes than expected");
            exit(1);
        }
        
        printf("Received"); 
        
        if ( ( bytesRcvd = recv( sock, echoBuffer, STRBUFSIZE - 1, 0) ) <= 0 )
        {
            perror("recv() failed or connection closed prematurely");
            exit(1);
        }
        printf( "(%d): ", bytesRcvd );
        printf( "%s", echoBuffer);      // Print the echo buffer 
        
    }
    
    printf("\n");    // Print a final linefeed 

    close(sock);
    return 0;
}
