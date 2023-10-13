
#include "tcp_client.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef USE_WINDOWS_OS

#include <winsock2.h>
#endif


#ifdef USE_UNIX_OS

#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <errno.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#endif

#define CRLF        "\r\n"
#define MAX_CLIENTS 10


typedef struct
{
   SOCKET sock;
   uint8_t connected; // 0 = not connected, otherwise identifier
} peer;

void tcp_initialize(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

void tcp_end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}


