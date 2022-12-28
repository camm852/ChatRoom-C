#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "queue.h"

#define MAXLINE 4150
#define MAXCLIENTS 2
#define TRUE 1
#define GREEN(string) "\x1b[32m" string "\x1b[0m"
#define RED(string) "\x1b[31m" string "\x1b[0m"

ClientService *clients[MAXCLIENTS];

Queue *queue;

pthread_mutex_t clientMutex = PTHREAD_MUTEX_INITIALIZER;

int conections = 0;
int id = 1;

/**
 * @brief Eliminate characteres '\n'
 *
 * @param buffer
 * @param length
 */
void purgeBuffer(char *buffer, int length);

/**
 * @brief Create a Socket object
 *
 * @param port
 * @param type
 * @return int socket Descriptor
 */
int createSocket(int *port, int type);

/**
 * @brief Insert a client information in clientService array
 *
 * @param client
 */
void addClient(ClientService *client);

/**
 * @brief Remove a client information of clientService array
 *
 * @param id
 */
void removeClient(int id);

/**
 * @brief Send a message to everyone online
 *
 * @param message
 * @param id
 */
void sendMessage(char *message, int id);

/**
 * @brief Where receive and send message, also add people in queue
 *
 * @param clientThread
 * @return void*
 */
void handleClient(ClientService *clientThread);

/**
 * @brief send message for people in queue for join
 *
 * @param clientThread
 */
void responseJoinChat(ClientService *clientThread);

int main(int argc, char *argv[])
{

  pthread_t thread;
  int socketListen, socketService; // Descriptores de sockets
  struct sockaddr_in socketStruct;

  queue = (Queue *)malloc(sizeof(Queue));
  queueConstructor(queue);

  int sizeSocketStruct; // tamaño del socket

  if (argc != 2) // verifico si se ingresa por argumentos el puerto
  {
    fprintf(stderr, "Use: %s [port]\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[1]);

  signal(SIGPIPE, SIG_IGN);

  if ((socketListen = createSocket(&port, SOCK_STREAM)) == -1) // Creacion del socket de escucha
  {
    fprintf(stderr, "Error: Can not create/connect socket\n");
    exit(2);
  }

  listen(socketListen, 1024);

  fprintf(stdout, "Chatroom run in port: %d\n", port);

  while (TRUE)
  {
    sizeSocketStruct = sizeof(socketStruct);
    socketService = accept(socketListen, (struct sockaddr *)&socketStruct, &sizeSocketStruct);

    ClientService *client = (ClientService *)malloc(sizeof(ClientService));
    client->socketFileDescriptor = socketService;
    client->id = id++;

    if (conections >= MAXCLIENTS)
    {
      char buffer[MAXLINE];

      addQueue(queue, client);
      sprintf(buffer, "You are in waiting line for join\n");
      send(client->socketFileDescriptor, buffer, MAXLINE, 0);

      continue;
    }

    addClient(client);

    if (pthread_create(&thread, NULL, (void *)handleClient, (void *)client) != 0)
    {
      fprintf(stdout, "Internal Server Error\n");
      exit(1);
    }
    sleep(1);
  }
}

void purgeBuffer(char *buffer, int length)
{
  int i;
  for (i = 0; i < length; i++)
  {
    if (buffer[i] == '\n')
    {
      buffer[i] = '\0';
      break;
    }
  }
}
int createSocket(int *port, int type)
{
  int newSocket;
  struct sockaddr_in socketStruct;
  int sizesocketStruct;

  if ((newSocket = socket(PF_INET, type, 0)) == -1)
  {
    perror("Error: Imposible crear socket");
    exit(2);
  }

  bzero((char *)&socketStruct, sizeof(socketStruct));

  socketStruct.sin_port = htons(*port);
  socketStruct.sin_addr.s_addr = htonl(INADDR_ANY);
  socketStruct.sin_family = PF_INET;

  if (bind(newSocket, (struct sockaddr *)&socketStruct, sizeof(socketStruct)) == -1)
  {
    perror("Binding was not possible");
    exit(3);
  }

  sizesocketStruct = sizeof(socketStruct);

  if (getsockname(newSocket, (struct sockaddr *)&socketStruct, &sizesocketStruct))
  {
    perror("Getting name host was not possible");
    exit(4);
  }

  return (newSocket);
}
void addClient(ClientService *client)
{
  pthread_mutex_lock(&clientMutex);
  for (int i = 0; i < MAXCLIENTS; ++i)
  {
    if (!clients[i])
    {
      clients[i] = client;
      break;
    }
  }
  pthread_mutex_unlock(&clientMutex);
}
void removeClient(int id)
{
  pthread_mutex_lock(&clientMutex);
  for (int i = 0; i < MAXCLIENTS; i++)
  {
    if (clients[i] && clients[i]->id == id)
    {
      clients[i] = NULL;
      break;
    }
  }
  pthread_mutex_unlock(&clientMutex);
}
void sendMessage(char *message, int id)
{
  pthread_mutex_lock(&clientMutex);
  for (int i = 0; i < MAXCLIENTS; ++i)
  {

    if (clients[i] && clients[i]->id != id)
    {
      if (send(clients[i]->socketFileDescriptor, message, strlen(message), 0) < 0)
      {
        perror("ERROR: write to descriptor failed");
        break;
      }
    }
  }
  pthread_mutex_unlock(&clientMutex);
}
void handleClient(ClientService *clientThread)
{
  char buffer[MAXLINE];
  char name[NAME];
  int isLeave = 0;

  ClientService *client = clientThread;

  // Recibe el Name
  if (recv(client->socketFileDescriptor, name, NAME, 0) <= 0)
  {
    isLeave = 1;
  }
  else
  {
    strcpy(client->name, name);
    sprintf(buffer, GREEN("%s") " has joined\n", client->name);
    printf("%s", buffer);
    sendMessage(buffer, client->id);
  }

  pthread_mutex_lock(&clientMutex);
  conections++;
  pthread_mutex_unlock(&clientMutex);

  bzero(buffer, MAXLINE);

  while (TRUE)
  {
    if (isLeave)
    {
      break;
    }

    int messageClient;

    if ((messageClient = recv(client->socketFileDescriptor, buffer, MAXLINE, 0)) > 0 && strlen(buffer) > 0)
    {
      sendMessage(buffer, client->id);
    }
    else if (messageClient <= 0)
    {
      sprintf(buffer, RED("%s") " has disconected\n", client->name);
      printf("%s", buffer);
      sendMessage(buffer, client->id);
      isLeave = 1;
    }

    bzero(buffer, MAXLINE);
  }

  removeClient(client->id);
  close(client->socketFileDescriptor);
  free(client);

  pthread_mutex_lock(&clientMutex);
  conections--;
  pthread_mutex_unlock(&clientMutex);

  if (queue->end > 0)
  {
    ClientService *clientWaiting = removeQueue(queue);
    addClient(clientWaiting);
    responseJoinChat(clientWaiting);
  }

  pthread_detach(pthread_self());
}
void responseJoinChat(ClientService *clientThread)
{
  char buffer[MAXLINE];

  bzero(buffer, MAXLINE);
  sprintf(buffer, "You have joined\n");
  send(clientThread->socketFileDescriptor, buffer, MAXLINE, 0);

  pthread_t thread;

  if (pthread_create(&thread, NULL, (void *)handleClient, (void *)clientThread) != 0)
  {
    fprintf(stdout, "Internal Server Error\n");
    exit(1);
  }
}
