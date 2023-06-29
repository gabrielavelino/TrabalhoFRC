#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
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
void createNewRoomCommand(char buffer[256], int client_file_decriptor)
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
      // joinRoom(clientIndex, roomIndex);
      printf("Cliente %d criou a sala %s\n", client_file_decriptor, roomName);
   }
   else
   {
      send(client_file_decriptor, "Não é possível criar a sala. Limite máximo de salas atingido.\n", 61, 0);
   }
}


void showHelpMessageCommand(int client_file_decriptor)
{
   char helpMessage[] = "\n########### COMANDOS DE AJUDA ############\n"
                        "Comandos disponíveis:\n"
                        "/help - Exibe a lista de comandos disponíveis\n"
                        "/join <sala> - Entra na sala especificada\n"
                        "/create <nome> - Cria uma nova sala com o nome especificado\n"
                        "/list - Lista as salas disponíveis\n"
                        "/exit - Sai do chat\n"
                        "##########################################\n\n";
   send(client_file_decriptor, helpMessage, sizeof(helpMessage), 0);
}

void joinRoom(int clientIndex, int roomIndex)
{
   clients[clientIndex].roomIndex = roomIndex;
   rooms[roomIndex].clients[rooms[roomIndex].numClients] = clientIndex;
   rooms[roomIndex].numClients++;
}

void joinRoomCommand(char buffer[256], int client_file_decriptor, int clientIndex)
{
   printf("Entrou na sala criada %s\n", buffer);
   int roomIndex = atoi(buffer + 6);
   if (roomIndex >= 0 && roomIndex < MAX_ROOMS && strlen(rooms[roomIndex].name) > 0)
   {
      joinRoom(clientIndex, roomIndex);
      //printf("Cliente %d entrou na sala %s\n", client_file_decriptor, roomName);
   }
   else
   {
      send(client_file_decriptor, "Sala inválida\n", 16, 0);
   }
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

void listRooms(int client_file_descriptor)
{
   for (size_t i = 0; i < MAX_ROOMS; i++)
   {
      printf("%s\n", rooms[i].name);
      char roomList[256];
      sprintf(roomList, "%s %d/10\n", rooms[i].name, rooms[i].numClients);
      if(strcmp(rooms[i].name, "") == 0){
         break;
      }
      send(client_file_descriptor, roomList, strlen(roomList), 0);
   }
}

void broadcastMessage(int senderIndex, char *message)
{
   int roomIndex = clients[senderIndex].roomIndex;
   int *clientsInRoom = rooms[roomIndex].clients;
   int numClients = rooms[roomIndex].numClients;
   char senderName[256];
   sprintf(senderName, "%d: %s", senderIndex, message);

   for (int i = 0; i < numClients; i++)
   {
      if(strncmp(message, "/exit", 5) == 0){
         leaveRoom(senderIndex);
         break;
      }
      int clientSocket = clients[clientsInRoom[i]].socket;
      if(senderIndex != clientsInRoom[i]){
         send(clientSocket, senderName, strlen(senderName), 0);
      }
   }
}

void acceptNewClient()
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

int main(int argc, char *argv[])
{
   if (argc < 3)
   {
      printf("Digite IP e Porta para este servidor\n");
      exit(1);
   }
   printf("########### SERVIDOR CHAT FRC ############\n");
   printf("Servidor rodando no IP: %s\n", argv[1]);
   printf("Digite /help para ver os comandos disponíveis\n");
   printf("##########################################\n");
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
               acceptNewClient();
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
               // REALIZA OS COMANDOS DO SERVIDOR
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
                     printf("Cliente %d enviou comando %s\n", i, buffer);
                     if (strncmp(buffer, "/help", 5) == 0)
                        showHelpMessageCommand(i);
                     else if (strncmp(buffer, "/create ", 8) == 0)
                        createNewRoomCommand(buffer, i);
                     else if (strncmp(buffer, "/join ", 6) == 0)
                        joinRoomCommand(buffer, i, clientIndex);
                     else if (strncmp(buffer, "/list", 5) == 0)
                        listRooms(i);
                     else
                     {
                        send(i, "Comando inválido\n", 19, 0);
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