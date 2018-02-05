#ifndef _IPC_HASH_H
#define _IPC_HASH_H

#include <netinet/in.h>
#include <pthread.h>
#include <sys/types.h>
#include "list.h"

#define HASH_LIST_SIZE 1024
#define HASH_NODE_COUNT_IN_SHM 10000
#define SERVER_ALIVE_SHM_BYTE 1
#define CLIENTS_LIMITATION 2
#define ADD 0
#define DEL 1
#define DUMP_ALL 2
#define QUE_ONE 3
#define WAIT 4
#define STRING_SIZE 128
#define TEST_TIME_OF_EACH_PROCESS 1000000

typedef enum {
	HASH_SHM_KEY = 1,
	HASH_INDEX_SHM_KEY = 2,
	SERVER_LIVE_SHM_KEY = 3,
	MSG_QUEUE_KEY = 4,
	// SHM_SEM_KEY for read-write lock
	SHM_SEM_KEY = 5,
	MSG_INQUEUE_SEM_KEY = 6	
}ipc_keys_e;

typedef struct node_in_shm_s {
	list_head_t free_node_link;
	int self_index;			//use to know the index in shm when processing pool
	int id;					
	int data;				
	int next_index;			//use to know where is the next node
}node_in_shm_t;

typedef struct msg_from_client_s {
	long type;				//let msg queue recognize message
	int operation;			//0:add,1:delete,2:dump all
	int id;
	int data;
	long process_id;			//being the specific msg type when server send back
}msg_from_client_t;

typedef struct msg_from_server_s {
	long type;				//client will know the type to receive according to process_id
	int operation_result;	//0:successful 1:failed
}msg_from_server_t;

typedef struct hlist_s {
	// [x][0] store the element count
	// [x][1] store the first hash node index in SHM
	int hash_array[HASH_LIST_SIZE][2];
}hlist_t;

typedef struct hlist_node_pool_s {
	list_head_t *first_free_node;
	int free_node_in_pool;
	node_in_shm_t node_memory_pool[HASH_NODE_COUNT_IN_SHM];
}hlist_node_pool_t;
#endif
