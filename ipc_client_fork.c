#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include "ipc_hash.h"

#define PAUSE printf("Press Enter key to continue..."); fgetc(stdin)
static int __read_specific_node(int id, node_in_shm_t *first_shm_node_ptr, hlist_t *hlist)
{
	int i, query_node_exist = 0;
	node_in_shm_t *temp_node;
	// know what list should be lookup
	int id_hash_value = id % HASH_LIST_SIZE;
	// know how many nodes in the list
	int node_in_list = hlist->hash_array[id_hash_value][0];
	// search from the first node in the list
	temp_node = first_shm_node_ptr + hlist->hash_array[id_hash_value][1];
	
	for (i=0; i<node_in_list; i++) {
		if (temp_node->id == id) {
			query_node_exist = 1;
			printf("Node is in %d-th list with ID: %d DATA: %d \n", 
					id_hash_value, temp_node->id, temp_node->data);
		}
		temp_node = first_shm_node_ptr + temp_node->next_index;
	}
	
	// return 0: successful query
	if (query_node_exist) {
		return 0;
	} else {
		return 1;
	}
}

int main(void)
{
	// for random
	srand(time(NULL));
	// pid_t is int use to get pid to be the message type in queue
	pid_t process_id;
	int server_is_alive, times_to_run, insert_in_shm = 0;
	int node_shm_id, hlist_shm_id, server_live_shm_id, msg_queue_id,
		shm_sem_id, msg_inqueue_sem_id;
		
	// three pointer point to SHM data address 
	node_in_shm_t *first_shm_node_ptr;
	hlist_t *hlist_shm_ptr;
	short *server_live_byte_ptr;

	// message buffer for inqueue (msgsnd)
	msg_from_server_t server_msg_buf;
	// message buffer for dequeue (msgrcv)
	msg_from_client_t client_msg_buf;

	// IPC elements initialization
	// Semaphore
	// shm_read_sem is a read/write lock for SHM implemented by counting semaaphore
	// msg_inqueue_sem is for the clients limitation purpose implemented by counting semaphore
	shm_sem_id = semget((ipc_keys_e)SHM_SEM_KEY, 1, IPC_CREAT | 0666);

	msg_inqueue_sem_id = semget((ipc_keys_e)MSG_INQUEUE_SEM_KEY, 1, IPC_CREAT | 0666);

	// SHM 
	node_shm_id = shmget((ipc_keys_e)HASH_SHM_KEY, 
					HASH_NODE_COUNT_IN_SHM*sizeof(node_in_shm_t), IPC_CREAT | 0666);
	first_shm_node_ptr = (node_in_shm_t *) shmat(node_shm_id, NULL, 0);
	
	hlist_shm_id = shmget((ipc_keys_e)HASH_INDEX_SHM_KEY,
					sizeof(hlist_t), IPC_CREAT | 0666);
	hlist_shm_ptr = (hlist_t *)shmat(hlist_shm_id, NULL, 0);

	server_live_shm_id = shmget((ipc_keys_e)SERVER_LIVE_SHM_KEY,
					SERVER_ALIVE_SHM_BYTE, IPC_CREAT | 0666);
	server_live_byte_ptr = (short *) shmat(server_live_shm_id, NULL, 0);
	// Message Queue
	msg_queue_id = msgget((ipc_keys_e)MSG_QUEUE_KEY, IPC_CREAT | 0666);

	// Semaphore buffer initialization for semop()
	struct sembuf get_sem_buf, release_sem_buf;

	get_sem_buf.sem_num = 0;
	get_sem_buf.sem_op = -1;						// sem_value += sem_op
	get_sem_buf.sem_flg = SEM_UNDO;					// SEM_UNDO release while process break down

	release_sem_buf.sem_num = 0;
	release_sem_buf.sem_op = 1;						// sem_value += sem_op
	release_sem_buf.sem_flg = SEM_UNDO;

	// get the process id to saperate the message in queue
	process_id = getpid();
	times_to_run = TEST_TIME_OF_EACH_PROCESS;
	while (times_to_run) {
		times_to_run--;
		// get the shm_sem to know server is alive or not
		server_is_alive = 0;
		semop(shm_sem_id, &(get_sem_buf), 1);
		// critical section
		if (*server_live_byte_ptr) {
			server_is_alive = 1;
		} else {
			printf("Server is down !!\n");
			break;
		}
		// release shm_sem
		semop(shm_sem_id, &(release_sem_buf), 1);	
		// client will do the process if server is alive
		if (server_is_alive) {
			// encapsulate the message
			// set the type = 1 to send to server and process_id is the type this client receive
			client_msg_buf.operation = rand() % 3;			// 3 type operations
			client_msg_buf.type = 1;				
			client_msg_buf.process_id = (long) process_id;
			client_msg_buf.id = rand() % (HASH_NODE_COUNT_IN_SHM + 1);
			client_msg_buf.data = rand();
		
			// switch case to send message to server or even read the shm to query
			switch (client_msg_buf.operation) {
				case ADD:
					// get message sem
					semop(msg_inqueue_sem_id, &(get_sem_buf), 1);
					// send message to queue and wait for feedback
					msgsnd(msg_queue_id, &(client_msg_buf), sizeof(msg_from_client_t), 0);
					msgrcv(msg_queue_id, &(server_msg_buf), sizeof(msg_from_server_t), 
							process_id, 0);
					// release message sem
					semop(msg_inqueue_sem_id, &(release_sem_buf), 1);
					if (server_msg_buf.operation_result) {
						printf("ADD operation successful!\n");
						insert_in_shm++;
					} else {
						printf("ADD operation failed!\n");
					}
					break;
				case DEL:
					// get message sem
					semop(msg_inqueue_sem_id, &(get_sem_buf), 1);
					// send message to queue and wait for feedback
					msgsnd(msg_queue_id, &(client_msg_buf), sizeof(msg_from_client_t), 0);
					msgrcv(msg_queue_id, &(server_msg_buf), sizeof(msg_from_server_t),
							process_id, 0);
					// release message sem
					semop(msg_inqueue_sem_id, &(release_sem_buf), 1);
					if (server_msg_buf.operation_result) {
						printf("DEL operation successful!\n");
						insert_in_shm--;
					} else {
						printf("DEL operation failed!\n");
					}
					break;
				case QUE_ONE:
					// query specific one node is a special case
					// and needs get shm semaphore to read 
					semop(shm_sem_id, &(get_sem_buf), 1);
					// critical section to read specific node
					if (__read_specific_node(client_msg_buf.id, first_shm_node_ptr, hlist_shm_ptr)) {
						// function return none zero means error
						printf("Query failed!\n");
					}
					// release shm semaphore
					semop(shm_sem_id, &(release_sem_buf), 1);
					break;
			}
		}
	}
	printf("%d node in SHM counting by this process\n", insert_in_shm);
	// detach SHM
	shmdt((char *)first_shm_node_ptr);
	shmdt((char *)hlist_shm_ptr);
	shmdt((char *)server_live_byte_ptr);
	return 0;
}
