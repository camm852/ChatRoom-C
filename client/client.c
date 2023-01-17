
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MAXLINE 4096
#define NAMELEN 64
#define TRUE 1
#define BLUE(string) "\x1b[34m" string "\x1b[0m"

/**
 * @brief Function that is executed when pressed ctrl+c
 *
 */
void ctrlC();

/**
 * @brief Return a int between 97 and 122 [a-z]
 *
 * @return int
 */
int randomInt();

/**
 * @brief print >
 *
 */
void prompt();

/**
 * @brief Eliminate characteres '\n'
 *
 * @param buffer
 * @param length
 */
void purgeBuffer(char *buffer, int length);

/**
 * @brief Enter name client
 *
 */
void enterName();

/**
 * @brief Check if the message for send is private
 *
 * @param buffer
 */
void isPrivate(char *buffer);

/**
 * @brief Check if the message received is for me
 *
 * @param buffer
 */
void isForMe(char *buffer);

/**
 * @brief Receive messages
 *
 * @param socketFileDescriptor
 */
void receiveMessage(int socketFileDescriptor);

/**
 * @brief Send messages
 *
 * @param socketFileDescriptor
 */
void sendMessage(int socketFileDescriptor);

char name[NAMELEN];
int flag = 0;

int main(int argc, char *argv[])
{
  int socketService; // descriptor socket

  struct sockaddr_in structSocket;

  /* address of the socket destination */
  struct hostent *hp, *gethostbyname();

  if (argc != 3)
  {
    fprintf(stderr, "Use: %s <host> <port>\n", argv[0]);
    exit(1);
  }

  /* Create socket */
  if ((socketService = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Imposible creacion del socket");
    exit(2);
  }

  /* Search address internet server*/
  if ((hp = gethostbyname(argv[1])) == NULL)
  {
    perror("Error: Nombre de la maquina desconocido");
    exit(3);
  }

  /* Preparation of the socket destination */
  structSocket.sin_family = PF_INET;
  structSocket.sin_addr.s_addr = htonl(argv[1]);
  structSocket.sin_port = htons(atoi(argv[2]));
  bcopy(hp->h_addr, &structSocket.sin_addr, hp->h_length);

  /* Conect socket server */
  if (connect(socketService, (struct sockaddr *)&structSocket, sizeof(structSocket)) == -1)
  {
    perror("Connect failed");
    exit(4);
  }

  signal(SIGINT, ctrlC);
  srand(time(NULL));

  while (TRUE)
  {
    enterName();
    purgeBuffer(name, NAMELEN);
    if (strlen(name) > 3)
    {
      char charRandom[2];
      sprintf(charRandom, "%c%c", randomInt(), randomInt());
      strcat(name, charRandom);
      printf("Your name will be %s\n", name);
      break;
    }
  }
  printf("Private message is _nickname_message\n");
  printf("/users for view users connected\n");

  fflush(stdin);
  send(socketService, name, NAMELEN, 0);

  pthread_t receiveMessageThread;

  if (pthread_create(&receiveMessageThread, NULL, (void *)receiveMessage, (void *)socketService) != 0)
  {
    printf("Error thread");
    exit(1);
  }

  pthread_t sendMessageThread;

  if (pthread_create(&sendMessageThread, NULL, (void *)sendMessage, (void *)socketService) != 0)
  {
    printf("Error thread");
    exit(1);
  }

  while (TRUE)
  {
    if (flag)
    {
      printf("\nCome Back Soon\n");
      break;
    }
  }
  close(socketService);
}

void ctrlC()
{
  flag = 1;
}
int randomInt()
{
  return 'a' + rand() % 26;
}
void prompt()
{
  printf("\r%s", ">");
  fflush(stdout);
}
void purgeBuffer(char *buffer, int length)
{
  for (int i = 0; i < length; i++)
  { // trim \n
    if (buffer[i] == '\n')
    {
      buffer[i] = '\0';
      break;
    }
  }
}
void enterName()
{
  printf("Enter your name: ");
  fgets(name, NAMELEN, stdin);
}
void isPrivate(char *buffer)
{
  int initName = 0;
  int initMessage = 0;
  size_t lengthBuffer = strlen(buffer);
  if (buffer[0] == '_')
  {
    for (int i = 1; i < lengthBuffer; i++)
    {
      if (buffer[i] == '_')
      {
        initName = i + 1;
        break;
      }
    }
  }
  if (initName != 0)
  {
    for (int i = initName; i < lengthBuffer; i++)
    {
      buffer[i] = buffer[i] + 1;
    }
  }
}
void isForMe(char *buffer)
{
  int initMessage = 0;
  char nameMessage[NAMELEN] = {};
  size_t lengthBuffer = strlen(buffer);
  for (int i = 1; i < lengthBuffer; i++)
  {
    if (buffer[i] == ':')
    {
      initMessage = i + 1;
      break;
    }
  }
  if (initMessage != 0 && buffer[initMessage] == '_')
  {
    int namePosition = 0;
    for (int i = initMessage + 1; i < lengthBuffer; i++)
    {
      if (buffer[i] == '_')
      {
        initMessage = i + 1;
        break;
      }
      nameMessage[namePosition] = buffer[i];
      namePosition++;
    }
  }
  if (strcmp(nameMessage, name) != 0)
    return;
  for (int i = initMessage; i < lengthBuffer; i++)
  {
    buffer[i] = buffer[i] - 1;
  }
}
void receiveMessage(int socketFileDescriptor)
{
  char message[MAXLINE + NAMELEN] = {};
  while (1)
  {
    int receive = recv(socketFileDescriptor, message, MAXLINE + NAMELEN, 0);
    if (receive > 0)
    {
      isForMe(message);
      printf("%s", message);
      prompt();
    }
    if (receive == 0)
    {
      break;
    }
    memset(message, 0, sizeof(message));
  }
}
void sendMessage(int socketFileDescriptor)
{

  char buffer[MAXLINE] = {};
  char message[MAXLINE + NAMELEN] = {};
  while (TRUE)
  {
    prompt();
    fgets(buffer, MAXLINE, stdin);
    purgeBuffer(buffer, MAXLINE);
    isPrivate(buffer);
    sprintf(message, BLUE("%s") ":%s\n", name, buffer);
    send(socketFileDescriptor, message, strlen(message), 0);
    bzero(message, MAXLINE + NAMELEN);
    bzero(buffer, MAXLINE);
  }
}