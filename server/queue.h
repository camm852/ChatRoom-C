#define NAME 64

typedef struct // Estructura del cliente
{
  int socketFileDescriptor;
  int id;
  char name[NAME];
} ClientService;

typedef struct
{
  ClientService *queueClients[20];
  int init;
  int end;
  int size;
} Queue;

/**
 * @brief Start fields of the struct
 *
 * @param object type queue
 */
void queueConstructor(Queue *queue)
{
  queue->init = 0;
  queue->end = 0;
  queue->size = sizeof(queue->queueClients) / sizeof(*queue->queueClients);
}

/**
 * @brief add client to queue
 *
 * @param object type queue
 * @param object type ClientService
 */
void addQueue(Queue *queue, ClientService *client)
{
  if (queue->end > queue->size)
  {
    return;
  }
  queue->queueClients[queue->end] = client;
  queue->end++;
};

/**
 * @brief remove client of the queue
 *
 *
 * @param object type queue
 * @return ClientService*
 */
ClientService *removeQueue(Queue *queue)
{
  if (queue->end > 0)
  {
    ClientService *clientAux = queue->queueClients[0];
    for (int i = 0; i < queue->end; i++)
      queue->queueClients[i] = queue->queueClients[i + 1];
    queue->end--;

    return clientAux;
  }
}