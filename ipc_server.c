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
#include <pthread.h>
#include "ipc_hash.h"

#define PAUSE printf("Press Enter key to continue..."); fgetc(stdin)
extern void init_pool(hlist_node_pool_t *pool_ds);
extern void init_hlist(hlist_t *hash_list);
extern int add_node_in_shm(hlist_node_pool_t *pool_ds, node_in_shm_t *first_shm_node_ptr, 
							hlist_t *hash_list_ptr, int id, int data);
extern int del_node_in_shm(hlist_node_pool_t *pool_ds, node_in_shm_t *first_shm_node_ptr, 
					hlist_t *hash_list_ptr, int id, int data);
extern int dump_all_node_in_shm(node_in_shm_t *first_shm_node_ptr, hlist_t *hash_list_ptr);

int g_server_keep_working = 1;

static void __turn_off_server(void)
{
	int input, server_run = 1;
	while (server_run) {
		// 69 == 'E'
		input = fgetc(stdin);
		if (input == 69) {
			server_run = 0;
		}
	}
	g_server_keep_working = 0;
}

int main(void)
{
	int i, last_message_in_queue;
	int node_shm_id, hlist_shm_id, server_live_shm_id, msg_queue_id,
		shm_sem_id, msg_inqueue_sem_id;
	hlist_node_pool_t pool_ds;
	// thread creating	
	pthread_t tid;
	
	pthread_create(&tid, NULL, (void*) __turn_off_server, NULL);

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
	// shm_sem is a read/write lock for SHM implemented by counting semaaphore
	// msg_inqueue_sem is for the clients limitation purpose implemented by counting semaphore
	// the initial value of semaphore is 0 !!!
	shm_sem_id = semget((ipc_keys_e)SHM_SEM_KEY, 1, IPC_CREAT | 0666);
	semctl(shm_sem_id, 0, SETVAL, (int) CLIENTS_LIMITATION);

	msg_inqueue_sem_id = semget((ipc_keys_e)MSG_INQUEUE_SEM_KEY, 1, IPC_CREAT | 0666);
	semctl(msg_inqueue_sem_id, 0, SETVAL, (int) CLIENTS_LIMITATION);

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
	release_sem_buf.sem_op = CLIENTS_LIMITATION;	// sem_value += sem_op
	release_sem_buf.sem_flg = SEM_UNDO;
	
	// Initialization of SHM and memory pool (pool_ds)
	init_pool(&(pool_ds));

	// get semaphore
	for (i=0; i<CLIENTS_LIMITATION; i++) {
		semop(shm_sem_id, &(get_sem_buf), 1);
	}
	// critical section begin
	*server_live_byte_ptr =(short)1;				// 1 means server is live now!
	init_hlist(hlist_shm_ptr);
	// critical section end
	// Release the semaphore
	semop(shm_sem_id, &(release_sem_buf), 1);

	printf("%d node in SHM now\n", HASH_NODE_COUNT_IN_SHM - pool_ds.free_node_in_pool);

	while (g_server_keep_working) {
		// First of all, receive the message from message queu
		if(msgrcv(msg_queue_id, &client_msg_buf, sizeof(msg_from_client_t), 1, IPC_NOWAIT) == -1) {
			sleep(0.001);
			continue;
		}
		// Then process the message through switch() 
		switch (client_msg_buf.operation) {
			case ADD:
				// get semaphore
				for (i=0; i<CLIENTS_LIMITATION; i++) {
						semop(shm_sem_id, &(get_sem_buf), 1);
				}
				// critical section begin
				server_msg_buf.operation_result = add_node_in_shm(&(pool_ds), 
													first_shm_node_ptr, hlist_shm_ptr,
													client_msg_buf.id, client_msg_buf.data);
				// critical section end
				// release semaphore
				semop(shm_sem_id, &(release_sem_buf), 1);
				
				// send feeback to message queue
				server_msg_buf.type = client_msg_buf.process_id;
				msgsnd(msg_queue_id, &(server_msg_buf), sizeof(msg_from_server_t), 0);
				printf("%d node in SHM now\n", HASH_NODE_COUNT_IN_SHM - pool_ds.free_node_in_pool);
				break;
			case DEL:
				// get semaphore
				for (i=0; i<CLIENTS_LIMITATION; i++) {
						semop(shm_sem_id, &(get_sem_buf), 1);
				}
				// critical section begin
				server_msg_buf.operation_result = del_node_in_shm(&(pool_ds), 
													first_shm_node_ptr, hlist_shm_ptr,
													client_msg_buf.id, client_msg_buf.data);
				// critical section end
				// release semaphore
				semop(shm_sem_id, &(release_sem_buf), 1);
				
				// send feeback to message queue
				server_msg_buf.type = client_msg_buf.process_id;
				msgsnd(msg_queue_id, &(server_msg_buf), sizeof(msg_from_server_t), 0);
				printf("%d node in SHM now\n", HASH_NODE_COUNT_IN_SHM - pool_ds.free_node_in_pool);
				break;
			case DUMP_ALL:
				// no need to get semaphore beacuse DUMP_ALL just reading
				server_msg_buf.operation_result = dump_all_node_in_shm(first_shm_node_ptr, 
													hlist_shm_ptr);
				server_msg_buf.type = client_msg_buf.process_id;
				msgsnd(msg_queue_id, &(server_msg_buf), sizeof(msg_from_server_t), 0);
				break;
		}
	}

	// Here is outside the while loop, it means server is going to turn off
	// get the semaphore
	for (i=0; i<CLIENTS_LIMITATION; i++) {
		semop(shm_sem_id, &(get_sem_buf), 1);	
	}
	// critical section begin
	// turn off server
	*server_live_byte_ptr = (short) 0;
	// critical section end
	semop(shm_sem_id, &(release_sem_buf), 1);

	// use OS maintain structure to get msg queue information
	struct msqid_ds msg_queue_information;
	msgctl(msg_queue_id, IPC_STAT, &(msg_queue_information));
	// to know how many messages send by client remaining
	last_message_in_queue = msg_queue_information.msg_qnum;
	// send all feedbacks with operation_result failed (0)
	for (i=0; i<last_message_in_queue; i++) {
		msgrcv(msg_queue_id, &client_msg_buf, sizeof(msg_from_client_t), 1, 0);
		server_msg_buf.type = client_msg_buf.process_id;
		server_msg_buf.operation_result = 0;
		msgsnd(msg_queue_id, &(server_msg_buf), sizeof(msg_from_server_t), 0);
	}
	// wait for 3 seconds to let client receive last message
	sleep(1);
	// Now server can really turn off!!	
	// IPC elements detach & remove operation
	// remove semaphores 
	semctl(shm_sem_id, 1, IPC_RMID, NULL);
	semctl(msg_inqueue_sem_id, 1, IPC_RMID, NULL);
	// detach & remove two SHM
	shmdt((char *)first_shm_node_ptr);
	shmdt((char *)hlist_shm_ptr);
	shmdt((char *)server_live_byte_ptr);
	shmctl(node_shm_id, IPC_RMID, NULL);
	shmctl(hlist_shm_id, IPC_RMID, NULL);
	shmctl(server_live_shm_id, IPC_RMID, NULL);
	// remove message queue
	msgctl(msg_queue_id, IPC_RMID, NULL);
	return 0;
}
