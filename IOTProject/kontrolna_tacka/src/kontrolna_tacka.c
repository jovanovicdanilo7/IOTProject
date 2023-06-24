#define  BSD                // WIN for Winsock and BSD for BSD sockets

//----- Include files -------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <stdlib.h>         // Needed for memcpy() and itoa()
#include <string.h>
#include <stdbool.h>
#ifdef WIN
  #pragma comment(lib,"Wsock32.lib")
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
#define PORT_NUM0         12345                  // Port number used
#define PORT_NUM1         12346                  // Port number used
#define PORT_NUM2         12347                  // Port number used
#define PORT_NUM3         12348                  // Port number used
#define PORT_NUM4         12349                  // Port number used
#define GROUP_ADDR "239.255.255.250"            // Address of the multicast group


char* getValueFromLabel(const char* input, const char* label) {
    char* value = NULL;
    const char* labelPtr = strstr(input, label);
    
    if (labelPtr != NULL) {
        const char* valuePtr = strchr(labelPtr, ':');
        
        if (valuePtr != NULL) {
            valuePtr++;
            while (*valuePtr == ' ') {
                valuePtr++;
            }
            
            size_t length = strcspn(valuePtr, "\r\n");
            value = (char*)malloc(length + 1);
            strncpy(value, valuePtr, length);
            value[length] = '\0';
        }
    }
    
    return value;
}

//===== Main program ========================================================
void main(void)
{
#ifdef WIN
  WORD wVersionRequested = MAKEWORD(1,1);       // Stuff for WSA functions
  WSADATA wsaData;                              // Stuff for WSA functions
#endif
  unsigned int         server_s;                // Server socket descriptor
  unsigned int         multi_server_sock;       // Multicast socket descriptor
  struct sockaddr_in   addr_dest;               // Multicast group address
  struct ip_mreq       mreq;                    // Multicast group descriptor
  unsigned char        TTL;                     // TTL for multicast packets
  struct in_addr       recv_ip_addr;            // Receive IP address
  struct sockaddr_in   client_addr;       // Client Internet address
  unsigned int         addr_len;                // Internet address length
  unsigned char        buffer_not[256];             // Datagram buffer
  unsigned char        buffer[512];             // Datagram buffer
  unsigned char        buffer0[256];             // Datagram buffer
  unsigned char        buffer1[256];             // Datagram buffer
  unsigned char        buffer2[256];             // Datagram buffer
  unsigned char        buffer3[256];             // Datagram buffer
  int                  count;                   // Loop counter
  int                  retcode;                 // Return code
  char                 senzor_pritiska[]         = "senzor_pritiska";
  char                 senzor_temperature[]      = "senzor_temperature";
  char                 aktuator_zvucnik_temp[]   = "aktuator_zvucnik_temp";
  char                 aktuator_zvucnik_prit[]   = "aktuator_zvucnik_prit";
  char                 notify_bye_bye[]   	 = "Byebye";
  bool 		       flag                      = false;

/*#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif*/

  // Create a multicast socket
  multi_server_sock=socket(AF_INET, SOCK_DGRAM,0);

  // Create multicast group address information
  addr_dest.sin_family = AF_INET;
  addr_dest.sin_addr.s_addr = inet_addr(GROUP_ADDR);
  addr_dest.sin_port = htons(PORT_NUM0);

  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = INADDR_ANY;
  client_addr.sin_port = htons(PORT_NUM3);

  // Set the TTL for the sends using a setsockopt()
  TTL = 1;
  retcode = setsockopt(multi_server_sock, IPPROTO_IP, IP_MULTICAST_TTL,
                       (char *)&TTL, sizeof(TTL));
  if (retcode < 0)
  {
    printf("*** ERROR - setsockopt() failed with retcode = %d \n", retcode);
    return;
  }

  // Set addr_len
  addr_len = sizeof(addr_dest);

  unsigned int receive_sock = socket(AF_INET, SOCK_DGRAM, 0);

  // Bind the receive socket to the local address and port
  bind(receive_sock, (struct sockaddr*)&client_addr, sizeof(client_addr));

// Set the receive socket to non-blocking mode
#ifdef WIN
  unsigned long non_blocking = 1;
  ioctlsocket(receive_sock, FIONBIO, &non_blocking);
#endif
#ifdef BSD
  fcntl(receive_sock, F_SETFL, O_NONBLOCK);
#endif

  // Multicast the message forever with a period of 1 second
  count = 0;
  puts("Sending M-SEARCH on multicast address to find all devices...\r\n");

  // Build the message in the buffer
  sprintf(buffer0,"M-SEARCH * HTTP/1.1\r\n"
 		  "Host: %s:%d\r\n"
		  "Man \ssdp:discover\r\n"
		  "MX: 1\r\n"
		  "ST:senzor_temperature\r\n"
		  "USER-AGENT: LINUX\r\n"
		  "CPFN.UPNP.ORG: kontrolna_tacka\r\n"
		  "CPUUID.UPNP.ORG: 6b73db9e-49e1-48d6-bf8a-d2c0f95d6e07\r\n"
		  "\r\n",GROUP_ADDR,PORT_NUM0);

  sprintf(buffer1,"M-SEARCH * HTTP/1.1\r\n"
 		  "Host: %s:%d\r\n"
		  "Man \ssdp:discover\r\n"
		  "MX: 1\r\n"
		  "ST:senzor_pritiska\r\n"
		  "USER-AGENT: LINUX\r\n"
		  "CPFN.UPNP.ORG: kontrolna_tacka\r\n"
		  "CPUUID.UPNP.ORG: 6b73db9e-49e1-48d6-bf8a-d2c0f95d6e07\r\n"
		  "\r\n",GROUP_ADDR,PORT_NUM1);

  sprintf(buffer2,"M-SEARCH * HTTP/1.1\r\n"
 		   "Host: %s:%d\r\n"
		   "Man \ssdp:discover\r\n"
		   "MX: 1\r\n"
		   "ST:aktuator_zvucnik\r\n"
		   "USER-AGENT: LINUX\r\n"
		   "CPFN.UPNP.ORG: kontrolna_tacka\r\n"
		   "CPUUID.UPNP.ORG: 6b73db9e-49e1-48d6-bf8a-d2c0f95d6e07\r\n"
		   "\r\n",GROUP_ADDR,PORT_NUM2);

  sprintf(buffer3,"M-SEARCH * HTTP/1.1\r\n"
 		   "Host: %s:%d\r\n"
		   "Man \ssdp:discover\r\n"
		   "MX: 1\r\n"
		   "ST:aktuator_zvucnik\r\n"
		   "USER-AGENT: LINUX\r\n"
		   "CPFN.UPNP.ORG: kontrolna_tacka\r\n"
		   "CPUUID.UPNP.ORG: 6b73db9e-49e1-48d6-bf8a-d2c0f95d6e07\r\n"
		   "\r\n",GROUP_ADDR,PORT_NUM4);

  sprintf(buffer_not, "One device discnonnected!");
  while(1)
  {
    addr_dest.sin_port = htons(PORT_NUM0); // Change the port to 12345
    sendto(multi_server_sock, buffer0, sizeof(buffer0), 0, (struct sockaddr*)&addr_dest, addr_len);

    addr_dest.sin_port = htons(PORT_NUM1); // Change the port to 12346
    sendto(multi_server_sock, buffer1, sizeof(buffer1), 0, (struct sockaddr*)&addr_dest, addr_len);

    addr_dest.sin_port = htons(PORT_NUM2); // Change the port to 12347
    sendto(multi_server_sock, buffer2, sizeof(buffer2), 0, (struct sockaddr*)&addr_dest, addr_len);

    addr_dest.sin_port = htons(PORT_NUM4); // Change the port to 12349
    sendto(multi_server_sock, buffer3, sizeof(buffer3), 0, (struct sockaddr*)&addr_dest, addr_len);


    while (1) {
    // Receive incoming message
    ssize_t received_bytes = recvfrom(receive_sock, buffer, sizeof(buffer), 0, NULL, NULL);

    // Check if receive was successful
    if (received_bytes > 0) {
      // Process the received message
      printf("Received message: %s\n", buffer);

      char* value = getValueFromLabel(buffer, "ST:");
      char* value_notify = getValueFromLabel(buffer, "NT:");

      // Reset the buffer
      memset(buffer, 0, sizeof(buffer));

      //printf("%s!\n", value_notify);
      if((strcmp(senzor_pritiska, value) == 0) || (strcmp(senzor_temperature, value) == 0) || (strcmp(aktuator_zvucnik_temp, value) == 0) || (strcmp(aktuator_zvucnik_prit, value) == 0)){
	puts("One device connected!");        
        ++count;
        if(count == 4){
	  flag = false;
	  //puts("Flag is true!");
	}else{
	  break;
	}
      }else if(strcmp(notify_bye_bye, value_notify) == 0){
	puts("One device disconnected!");        	 
      }else{
	 puts("Wrong device connected!");         
      }

    } else {
      // No more incoming messages, break the inner loop
      break;
    }
  }

  while(flag){
    ssize_t received_bytes = recvfrom(receive_sock, buffer, sizeof(buffer), 0, NULL, NULL);

    // Check if receive was successful
    if (received_bytes > 0) {
      //flag = false;
      // Process the received message
      printf("Received message: %s\n", buffer);

      char* value_notify = getValueFromLabel(buffer, "NT:");

      // Reset the buffer
      memset(buffer, 0, sizeof(buffer));

      printf("%s!\n", value_notify);
      if(strcmp(notify_bye_bye, value_notify) == 0){
        addr_dest.sin_port = htons(PORT_NUM4); // Change the port to 12349
        sendto(multi_server_sock, buffer_not, sizeof(buffer_not), 0, (struct sockaddr*)&addr_dest, addr_len);

        addr_dest.sin_port = htons(PORT_NUM2); // Change the port to 12347
        sendto(multi_server_sock, buffer_not, sizeof(buffer_not), 0, (struct sockaddr*)&addr_dest, addr_len);
	puts("One device disconnected!"); 	
      }
    }
  }
  

#ifdef WIN
    Sleep(1000);
#endif
#ifdef BSD
    sleep(1);
#endif
  }

  // Close and clean-up
#ifdef WIN
  closesocket(multi_server_sock);
  WSACleanup();
#endif
#ifdef BSD
  close(multi_server_sock);
#endif
}
