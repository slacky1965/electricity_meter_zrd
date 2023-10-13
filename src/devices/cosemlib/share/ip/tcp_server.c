#include "tcp_server.h"
#include <stdio.h>
#include <string.h>

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

#define CRLF		"\r\n"
#define MAX_CLIENTS 10


typedef struct
{
   SOCKET sock;
   uint8_t connected; // 0 = not connected, otherwise identifier
} peer;

static void init(void)
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

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}



/*
static void broadcast(peer *clients, peer sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      // we don't send message to the sender
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
         //   strncpy(message, sender.name, BUF_SIZE - 1);
        //    strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
       //  strncat(message, buffer, sizeof message - strlen(message) - 1);
      //   write_peer(clients[i].sock, message);
      }
   }
}
*/

static int init_connection(int tcp_port)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(tcp_port);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_peer(SOCKET sock, char *buffer, int max_size)
{
   int n = 0;

   if((n = recv(sock, buffer, max_size, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disconnect the client */
      n = 0;
   }
   return n;
}

static void write_peer(SOCKET sock, const char *buffer, size_t size)
{
   if(send(sock, buffer, size, 0) < 0)
   {
      perror("send()");
    //  exit(errno);
   }
}

static void app(data_handler data_func, conn_handler conn_func, memory_t *b, int tcp_port)
{
   SOCKET sock = init_connection(tcp_port);

   char *buff = (char*)(b->data + b->offset);
   int max_size = b->max_size - b->offset;

   unsigned int max = sock;
   /* an array for all clients */
   peer peers[MAX_CLIENTS];

   // Init properly
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
       peers[i].connected = 0U;
   }

   fd_set working_set;
   fd_set master_set;

   FD_ZERO(&master_set);

   /* add the connection socket */
   FD_SET(sock, &master_set);

   puts("[TCP Server] TCP Server started");

    while(1)
    {
        // update max
        max = sock;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
           if (peers[i].connected)
           {
               /* what is the new maximum fd ? */
               max = peers[i].sock > max ? peers[i].sock : max;
           }
        }

        memcpy(&working_set, &master_set, sizeof(master_set));

        if(select(max + 1, &working_set, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }


        if(FD_ISSET(sock, &working_set))
        {
            /* new client */
            SOCKADDR_IN csin = { 0 };
            int sinsize = sizeof csin;
            SOCKET csock = accept(sock, (SOCKADDR *)&csin, &sinsize);

            if(csock == INVALID_SOCKET)
            {
                perror("accept()");
                continue;
            }

            // Grand access to the application layer
            uint8_t channel = conn_func(0U, CONN_NEW);
            if (channel > 0)
            {
                FD_SET(csock, &master_set);
                peer c = { csock, (uint8_t)channel };
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (!peers[i].connected)
                    {
                        peers[i] = c;
                        puts("[TCP server] New connection!");
                        break;
                    }
                }
            }
            else
            {
                // Reject connection
                end_connection(csock);
                puts("[TCP server] New connection rejected");
            }
        }

        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if (peers[i].connected)
            {
                /* a client is talking */
                if(FD_ISSET(peers[i].sock, &working_set))
                {
                   int size = read_peer(peers[i].sock, buff, max_size);
                   /* client disconnected */
                   if(size == 0)
                   {
                       FD_CLR(peers[i].sock, &master_set);
                      end_connection(peers[i].sock);

                      puts("Client disconnected !");
                      conn_func(peers[i].connected, CONN_DISCONNECTED);

                      // Make sure structure elements are cleared
                      peers[i].sock = INVALID_SOCKET;
                      peers[i].connected = 0U;
                   }
                   else
                   {
                       puts("[TCP server] New data received!");
                       if (data_func != NULL)
                       {
                           // FIXME: change channel number with instance number when multi threaded server is available
                           int ret = data_func(0U, b, size);
                           if (ret > 0)
                           {
                                write_peer(peers[i].sock, buff, ret);
                           }
                       }

                       //broadcast(peers, client, actual, buffer, 0);
                   }
                }
            }
        }
    }

   // Clear peers
   for(int i = 0; i < MAX_CLIENTS; i++)
   {
       if (peers[i].connected)
       {
            closesocket(peers[i].sock);
       }
   }
   // End server
   end_connection(sock);
}


int tcp_server_init(data_handler data_func, conn_handler conn_func, memory_t *buffer, int tcp_port)
{
   init();

   app(data_func, conn_func, buffer, tcp_port);

   end();

   return EXIT_SUCCESS;
}
