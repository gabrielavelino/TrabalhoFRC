#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_ROOMS 10
#define MAX_CLIENTS_PER_ROOM 10

typedef struct
{
   char name[50];
   int clients[MAX_CLIENTS_PER_ROOM];
   int numClients;
} ChatRoom;

typedef struct
{
   int socket;
   int roomIndex;
} Client;

fd_set master, read_fds;
int fdmax, listener;
ChatRoom rooms[MAX_ROOMS];
Client clients[MAX_CLIENTS_PER_ROOM];

void initializeRooms()
{
   for (int i = 0; i < MAX_ROOMS; i++)
   {
      strcpy(rooms[i].name, "");
      rooms[i].numClients = 0;
   }
}

void joinRoom(int clientIndex, int roomIndex)
{
   clients[clientIndex].roomIndex = roomIndex;
   rooms[roomIndex].clients[rooms[roomIndex].numClients] = clientIndex;
   rooms[roomIndex].numClients++;
}

void leaveRoom(int clientIndex)
{
   int roomIndex = clients[clientIndex].roomIndex;
   int *clientsInRoom = rooms[roomIndex].clients;
   int numClients = rooms[roomIndex].numClients;

   for (int i = 0; i < numClients; i++)
   {
      if (clientsInRoom[i] == clientIndex)
      {
         for (int j = i; j < numClients - 1; j++)
         {
            clientsInRoom[j] = clientsInRoom[j + 1];
         }
         break;
      }
   }

   rooms[roomIndex].numClients--;
   clients[clientIndex].roomIndex = -1;
}

void broadcastMessage(int senderIndex, char *message)
{
   int roomIndex = clients[senderIndex].roomIndex;
   int *clientsInRoom = rooms[roomIndex].clients;
   int numClients = rooms[roomIndex].numClients;

   for (int i = 0; i < numClients; i++)
   {
      int clientSocket = clients[clientsInRoom[i]].socket;
      send(clientSocket, message, strlen(message), 0);
   }
}

int main(int argc, char *argv[])
{
   if (argc < 3)
   {
      printf("Digite IP e Porta para este servidor\n");
      exit(1);
   }
   printf("###### Servidor de Chat ######\n");
   printf("IP: %s\n", argv[1]);
   initializeRooms();

   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   listener = socket(AF_INET, SOCK_STREAM, 0);
   setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

   struct sockaddr_in serverAddress;
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
   serverAddress.sin_port = htons(atoi(argv[2]));
   memset(&(serverAddress.sin_zero), '\0', 8);

   bind(listener, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
   listen(listener, 10);

   FD_SET(listener, &master);
   fdmax = listener;

   for (;;)
   {
      read_fds = master;
      select(fdmax + 1, &read_fds, NULL, NULL, NULL);

      for (int i = 0; i <= fdmax; i++)
      {
         if (FD_ISSET(i, &read_fds))
         {
            if (i == listener)
            {
               struct sockaddr_in clientAddress;
               socklen_t addrlen = sizeof(clientAddress);
               int newFileDescriptor = accept(listener, (struct sockaddr *)&clientAddress, &addrlen);

               FD_SET(newFileDescriptor, &master);

               if (newFileDescriptor > fdmax)
               {
                  fdmax = newFileDescriptor;
               }

               int clientIndex = -1;
               for (int j = 0; j < MAX_CLIENTS_PER_ROOM; j++)
               {
                  if (clients[j].socket == 0)
                  {
                     clientIndex = j;
                     clients[j].socket = newFileDescriptor;
                     clients[j].roomIndex = -1;
                     break;
                  }
               }

               printf("Novo cliente conectado (socket %d)\n", newFileDescriptor);
            }
            else
            {
               char buffer[256];
               int nbytes = recv(i, buffer, sizeof(buffer), 0);

               if (nbytes <= 0)
               {
                  int clientIndex = -1;
                  for (int j = 0; j < MAX_CLIENTS_PER_ROOM; j++)
                  {
                     if (clients[j].socket == i)
                     {
                        clientIndex = j;
                        clients[j].socket = 0;
                        if (clients[j].roomIndex != -1)
                        {
                           leaveRoom(j);
                        }
                        break;
                     }
                  }

                  printf("Cliente desconectado (socket %d)\n", i);
                  close(i);
                  FD_CLR(i, &master);
               }
               else
               {
                  buffer[nbytes] = '\0';

                  int clientIndex = -1;
                  for (int j = 0; j < MAX_CLIENTS_PER_ROOM; j++)
                  {
                     if (clients[j].socket == i)
                     {
                        clientIndex = j;
                        break;
                     }
                  }

                  if (clients[clientIndex].roomIndex == -1)
                  {
                     // O cliente não está em uma sala, processar comando de sala
                     printf("Cliente %d enviou comando '%s'\n", i, buffer);
                     if (strncmp(buffer, "/create ", 8) == 0)
                     {
                        char roomName[50];
                        strncpy(roomName, buffer + 8, sizeof(roomName) - 1);
                        roomName[sizeof(roomName) - 1] = '\0';

                        int roomIndex = -1;
                        for (int i = 0; i < MAX_ROOMS; i++)
                        {
                           if (strlen(rooms[i].name) == 0)
                           {
                              roomIndex = i;
                              strcpy(rooms[i].name, roomName);
                              rooms[i].numClients = 0;
                              break;
                           }
                        }
                        if (roomIndex != -1)
                        {
                           joinRoom(clientIndex, roomIndex);
                           printf("Cliente %d criou a sala '%s'\n", i, roomName);
                        }
                        else
                        {
                           send(i, "Não é possível criar a sala. Limite máximo de salas atingido.\n", 61, 0);
                        }
                     }
                     if (strncmp(buffer, "/join ", 6) == 0)
                     {
                        int roomIndex = atoi(buffer + 6);
                        if (roomIndex >= 0 && roomIndex < MAX_ROOMS && strlen(rooms[roomIndex].name) > 0)
                        {
                           joinRoom(clientIndex, roomIndex);
                           printf("Cliente %d entrou na sala %d\n", i, roomIndex);
                        }
                        else
                        {
                           send(i, "Sala inválida\n", 14, 0);
                        }
                     }
                     else
                     {
                        send(i, "Comando inválido\n", 17, 0);
                     }
                  }
                  else
                  {
                     // O cliente está em uma sala, enviar mensagem para a sala
                     broadcastMessage(clientIndex, buffer);
                  }
               }
            }
         }
      }
   }

   return 0;
}