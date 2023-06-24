#define  BSD                // WIN for Winsock and BSD for BSD sockets

//----- Include files -------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <stdlib.h>         // Needed for memcpy()
#include <time.h>
#include <mosquitto.h>
#include <stdio.h>
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
#define PORT_NUM2        12347             // Port number used
#define PORT_NUM3        12348             // Port number used
#define GROUP_ADDR "239.255.255.250"            // Address of the multicast group

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    if (rc == 0) {
        printf("Subscribing to topic -t korisnik/temperatura \n");
        mosquitto_subscribe(mosq, NULL, "korisnik/temperatura", 0);
    } else {
        mosquitto_disconnect(mosq);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    // Check if the message payload is not NULL
    if (msg->payload) {
        // Print the topic and message payload received
        printf("Topic: %s\n", msg->topic);
        printf("Temperature: %s\n", (char *)msg->payload);

	int temperature = atoi((char *)msg->payload); // Convert payload to an integer

        if (temperature > 37) {
            printf("Temperature is higher than 37!\n");
        } else {
            printf("Temperature is within normal limits!\n");
        }
    }
    mosquitto_disconnect(mosq);
}

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
  unsigned char        buffer2[256];       // Datagram buffer
  unsigned char        buffer3[512];       // Datagram buffer
  int                  retcode;           // Return code
  struct mosquitto *mosq;
  int rc;
  unsigned char        buffer[256];       // Datagram buffer

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
  client_addr.sin_port = htons(PORT_NUM2);

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
  if( (retcode = recvfrom(multi_server_sock, buffer2, sizeof(buffer2), 0,
    (struct sockaddr *)&client_addr, &addr_len)) < 0){
      printf("*** ERROR - recvfrom() failed \n");
      exit(1);
    }

  // Output the received buffer to the screen as a string
  printf("%s\n",buffer2);

  sprintf(buffer3,"RESPONSE * HTTP/1.1 200 OK\r\n"
 		  "CACHE-CONTROL: max-age = 3600\r\n"
		  "DATE: %02d-%02d-%04d\r\n"
		  "EXT: Empty\r\n"
		  "LOCATION: %s:%d\r\n"
		  "SERVER: LINUX\r\n"
		  "ST: aktuator_zvucnik_temp\r\n"
		  "USN: uuid:12345678-90ab-cdef-1234-567890abcdef::urn:schemas-upnp-org:device:aktuator_zvucnik\r\n"
		  "\r\n", day, month, year, GROUP_ADDR, PORT_NUM2);

  puts("Sending response...");
  ssize_t bytes_sent = sendto(multi_server_sock, buffer3, sizeof(buffer3), 0, (struct sockaddr*)&addr_dest, addr_len);
  if (bytes_sent > 0) {
    puts("Message was sent successfully!");     
  }

  mosquitto_lib_init();

  mosq = mosquitto_new(NULL, true, NULL);
  if (mosq == NULL) {
      printf("Failed to create client instance.\n");
      return 1;
  }
  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_message_callback_set(mosq, on_message);

  rc = mosquitto_connect(mosq, "localhost", 1883, 60);
  if (rc != MOSQ_ERR_SUCCESS) {
      printf("Connect failed: %s\n", mosquitto_strerror(rc));
      return 1;
  }

  mosquitto_loop_start(mosq);  // Start the MQTT network loop in a separate thread

  // Wait for messages until a key is pressed
  printf("Press Enter to quit...\n");
  getchar();

  mosquitto_loop_stop(mosq, true);  // Stop the network loop
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
