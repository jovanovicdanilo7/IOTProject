#define  BSD                // WIN for Winsock and BSD for BSD sockets

//----- Include files -------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <stdlib.h>         // Needed for memcpy()
#include <time.h>
#include <stdio.h>
#include <mosquitto.h>
#include <string.h>
#ifdef WIN
  #include <winsock.h>      // Needed for all Windows stuff
#endif
#ifdef BSD
  #include <sys/types.h>    // Needed for system defined identifiers.
  #include <netinet/in.h>   // Needed for internet address structure.
  #include <sys/socket.h>   // Needed for socket(), bind(), etc...
  #include <arpa/inet.h>    // Needed for inet_ntoa()
  #include <fcntl.h>
  #include <netdb.h>
#endif

//----- Defines -------------------------------------------------------------
#define PORT_NUM0        12345             // Port number used
#define PORT_NUM3        12348             // Port number used
#define GROUP_ADDR "239.255.255.250"            // Address of the multicast group

//===== Main program ========================================================
void main(void)
{
#ifdef WIN
  WORD wVersionRequested = MAKEWORD(1,1); // Stuff for WSA functions
  WSADATA wsaData;                        // Stuff for WSA functions
#endif
  unsigned int         multi_server_sock; // Multicast socket descriptor
  struct ip_mreq       mreq;              // Multicast group structure
  struct sockaddr_in   client_addr;       // Client Internet address
  struct sockaddr_in   addr_dest;               // Multicast group address
  unsigned int         addr_len;          // Internet address length
  unsigned char        buffer[256];       // Datagram buffer
  unsigned char        buffer0[256];       // Datagram buffer
  unsigned char        buffer3[512];       // Datagram buffer
  int                  retcode;           // Return code
  int rc;
  struct mosquitto * mosq;


#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif

  //getDate
  time_t currentTime;
  struct tm* localTime;

  // Get the current time
  currentTime = time(NULL);

  // Convert the current time to the local time
  localTime = localtime(&currentTime);

  // Extract the individual components of the date
  int year = localTime->tm_year + 1900;
  int month = localTime->tm_mon + 1;
  int day = localTime->tm_mday;

  // Create a multicast socket and fill-in multicast address information
  multi_server_sock=socket(AF_INET, SOCK_DGRAM,0);
  mreq.imr_multiaddr.s_addr = inet_addr(GROUP_ADDR);
  mreq.imr_interface.s_addr = INADDR_ANY;

  // Create client address information and bind the multicast socket
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = INADDR_ANY;
  client_addr.sin_port = htons(PORT_NUM0);

  addr_dest.sin_family = AF_INET;
  addr_dest.sin_addr.s_addr = inet_addr(GROUP_ADDR);
  addr_dest.sin_port = htons(PORT_NUM3);

  retcode = bind(multi_server_sock,(struct sockaddr *)&client_addr,
                 sizeof(struct sockaddr));
  if (retcode < 0)
  {
    printf("*** ERROR - bind() failed with retcode = %d \n", retcode);
    return;
  }

  // Have the multicast socket join the multicast group
  retcode = setsockopt(multi_server_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
             (char*)&mreq, sizeof(mreq)) ;
  if (retcode < 0)
  {
    printf("*** ERROR - setsockopt() failed with retcode = %d \n", retcode);
    return;
  }

  // Set addr_len
  addr_len = sizeof(client_addr);

  puts("Listening for messages...");

  // Receive a datagram from the multicast server
  if( (retcode = recvfrom(multi_server_sock, buffer0, sizeof(buffer0), 0,
    (struct sockaddr *)&client_addr, &addr_len)) < 0){
    printf("*** ERROR - recvfrom() failed \n");
    exit(1);
  }

  // Output the received buffer to the screen as a string
  printf("%s\n",buffer0);

  memset(buffer3, 0, sizeof(buffer3));
  sprintf(buffer3,"RESPONSE * HTTP/1.1 200 OK\r\n"
 		  "CACHE-CONTROL: max-age = 3600\r\n"
		  "DATE: %02d-%02d-%04d\r\n"
		  "EXT: Empty\r\n"
		  "LOCATION: %s:%d\r\n"
		  "SERVER: LINUX\r\n"
		  "ST: senzor_temperature\r\n"
		  "USN: uuid:12345678-90ab-cdef-1234-567890abcdef::urn:schemas-upnp-org:device:senzor_temperature\r\n"
		  "\r\n", day, month, year, GROUP_ADDR, PORT_NUM0);

  puts("Sending response...");
  ssize_t bytes_sent = sendto(multi_server_sock, buffer3, sizeof(buffer3), 0, (struct sockaddr*)&addr_dest, addr_len);
  if (bytes_sent > 0) {
    puts("Message was sent successfully!");    
  }

  mosquitto_lib_init();

  mosq = mosquitto_new("publisher-test", true, NULL);

  rc = mosquitto_connect(mosq, "localhost", 1883, 60);
  if (rc != 0) {
      printf("Client could not connect to broker! Error Code: %d\n", rc);
      mosquitto_destroy(mosq);
      return -1;
  }

  printf("We are now connected to the broker!\n");

  char temperature[10];
  while (1) {
      printf("Enter temperature value (or 'q' to quit): ");
      fgets(temperature, sizeof(temperature), stdin);

      // Check if user entered 'q' to quit
      if (temperature[0] == 'q'){
          sprintf(buffer, "NOTIFY * HTTP/1.1\r\n"
			  "HOST: 239.255.255.250:1900\r\n"
			  "NT: Byebye\r\n"
			  "NTS: ssdp:byebye\r\n"
			  "USN: uuid:12345678-90ab-cdef-1234-567890abcdef::urn:schemas-upnp-org:device:senzor_temperature\r\n"
			  "CONFIGID.UPNP.ORG: 12345\r\n"
			  "\r\n");
          puts("Sending notify bye, bye...");
	  addr_dest.sin_port = htons(PORT_NUM3);
          ssize_t bytes_sent = sendto(multi_server_sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr_dest, addr_len);
          break;
      }
      // Publish temperature
      mosquitto_publish(mosq, NULL, "korisnik/temperatura", strlen(temperature), temperature, 0, false);
  }

  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  // Close and clean-up
#ifdef WIN
  closesocket(multi_server_sock);
  WSACleanup();
#endif
#ifdef BSD
  close(multi_server_sock);
#endif
}
