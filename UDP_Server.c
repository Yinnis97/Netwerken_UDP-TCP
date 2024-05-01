#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	
	#include <time.h>
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <sys/select.h> 
	#include <time.h>
	int OSInit( void ) {}
	int OSCleanup( void ) {}
#endif

//-------------------------------------------------------------------------------

int initialization();
void execution( int internet_socket , long int RandomNumber );
void cleanup( int internet_socket );

//-------------------------------------------------------------------------------

int main( int argc, char * argv[] )
{
	while(1)
	{
	OSInit();

	int internet_socket = initialization();
	
	srand((unsigned)time(NULL));
	long int RandomNumber = rand()%99; //Random number
	
	execution( internet_socket, RandomNumber );

	cleanup( internet_socket );

	OSCleanup();
    }
	return 0;
}

//-------------------------------------------------------------------------------

int initialization()
{
	
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "24042", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				close( internet_socket );
				perror( "bind" );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

//-------------------------------------------------------------------------------

void execution( int internet_socket , long int RandomNumber )
{   int i = 8;
	fd_set rfds;
	int val1 = 1;
	int val2 = 0;
	int verschil = 0;
	struct timeval tv;
	int number_of_bytes_send = 0;
	int number_of_bytes_received = 0;
	char buffer[1000];
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	
   while(1)
   {
	  FD_ZERO(&rfds);
	  FD_SET(internet_socket,&rfds);
	
       number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
	  if( number_of_bytes_received == -1 )
	  {
	   perror( "recvfrom" );
	  }
	  else
	  {
	   buffer[number_of_bytes_received] = '\0';
	   printf( "Received : %s\n", buffer );
	   printf("Random : %d\n", RandomNumber );
	  }
	  
	  //wnr timeout niet 0 is.
	 if( val1 != 0 )
	 {
	   int ClientNumber = atoi(buffer);
	
	   //Verschil berekenen.
	   if(RandomNumber > ClientNumber)
	   {
	    verschil = RandomNumber - ClientNumber;
	   }
	   else
	   {
	    verschil = ClientNumber - RandomNumber;
	   }
	   printf("Verschil : %d\n", verschil);
	  
	   //Time-out delen door 2 bij elke gok.
	   tv.tv_sec = i;
	   if(number_of_bytes_received >= 1 ) 
	   {
		 number_of_bytes_received = 0;
		 i = i/2;
	   }
	  
	   val1 = select(1,&rfds,NULL,NULL,&tv);
	   printf("val1 = %d\n",val1);
	   if(val1 == -1)
	   {
	    perror();
	   }
	    number_of_bytes_received = 0;
	 }
	 
	  // wnr time-out = 0.
     if( val1 == 0 )
	 { 
       
	   FD_ZERO(&rfds);
	   FD_SET(internet_socket,&rfds);

	 
	   //Checken of er een bericht gestuurd is na of tijdens de time-out --> "You Lost!".
	   
       if( (number_of_bytes_received != 0) )
	   {
	    
	    printf("%d", number_of_bytes_received);
	    number_of_bytes_send = sendto( internet_socket, "You Lost!", 9, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
	    if( number_of_bytes_send == -1 )
	    {
		 perror( "sendto" );
	    }
		
		//Resetten van val1 om terug te kunnen starten.
		if(val2 == 0)
		{
		break;
		i = 8 , val1 = 8;
		}
       }
	   
	   //bericht sturen "You Won?".
	   if( (val2 ==0) && (number_of_bytes_received == 0) )
	   {
		
	    number_of_bytes_send = sendto( internet_socket, "You Won?", 8, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
	    if( number_of_bytes_send == -1 )
	    {
	     perror( "sendto" );
	    }
	   }
	   
	   //16 seconden time-out.
	   tv.tv_sec = 16;
       val2 = select(1,&rfds,NULL,NULL,&tv);
    
	   if(val2 == -1)
	   {
		perror();
	   }
	
	
	   //time-out is 0 en er is geen bericht gestuurd --> "You Won!".
	   if( (val2 == 0) && (number_of_bytes_received == 0) )
	   {
		number_of_bytes_received = 0;
	    number_of_bytes_send = sendto( internet_socket, "You Won!", 8, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
	    if( number_of_bytes_send == -1 )
	    {
		 perror( "sendto" );
	    }
	    break;
		
		i = 8 , val1 = 8;  //Resetten van val1 om terug te kunnen starten.
	   }
	  
     }
	
   }	
}
	
//-------------------------------------------------------------------------------

void cleanup( int internet_socket )
{
	close( internet_socket );
}


