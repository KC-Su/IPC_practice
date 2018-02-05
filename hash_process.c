#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "list.h"
#include "ipc_hash.h"

void init_pool(hlist_node_pool_t *pool_ds)
{
	int i;
	// set HASH_NODE_COUNT_IN_SHM to pool, represent how many nodes can add
	pool_ds->free_node_in_pool = HASH_NODE_COUNT_IN_SHM;

	// point first node in pool to be the first free node to add
	// and give it index 0 as in SHM
	INIT_LIST_HEAD(&(pool_ds->node_memory_pool[0].free_node_link));
	pool_ds->node_memory_pool[0].self_index = 0;
	pool_ds->node_memory_pool[1].next_index = -1;
	pool_ds->first_free_node = &(pool_ds->node_memory_pool[0].free_node_link);

	// initialize a link list through list.h API to have a list in which
	// all nodes can be used to add in SHM
	for (i=1; i<HASH_NODE_COUNT_IN_SHM; i++) {
		INIT_LIST_HEAD(&(pool_ds->node_memory_pool[i].free_node_link));
		// i is the index same as in SHM
		pool_ds->node_memory_pool[i].self_index = i;
		// -1 represents no node is linked as next node
		pool_ds->node_memory_pool[i].next_index = -1;
		// link node i to the tail of node i-1
		list_add_tail(&(pool_ds->node_memory_pool[i].free_node_link),
						&(pool_ds->node_memory_pool[i-1].free_node_link));
	}
}

void init_hlist(hlist_t *hash_list)
{
	int i;
	for (i=0; i<HASH_LIST_SIZE; i++) {
		// all list has 0 node
		hash_list->hash_array[i][0] = 0;
		// -1 represents no node is the first node of list
		hash_list->hash_array[i][1] = -1;
	}
}

int add_node_in_shm(hlist_node_pool_t *pool_ds, node_in_shm_t *first_shm_node_ptr,
					hlist_t *hash_list_ptr, int id, int data)
{
	node_in_shm_t temp_node;
	int node_count_in_list, id_hash_value;

	// Check if there is any node free to add
	if (pool_ds->free_node_in_pool == 1) {
		return 0;
	}

	// Check if there is a node existing with same id
	id_hash_value = id % HASH_LIST_SIZE;
	node_count_in_list = hash_list_ptr->hash_array[id_hash_value][0];

	// Jsut adding node if no node in list
	if (node_count_in_list != 0) {
		// temp_node now is the first node in list
		temp_node = *(first_shm_node_ptr + hash_list_ptr->hash_array[id_hash_value][1]);
		// Pass the while loop if no duplicated id value
		while (node_count_in_list) {
			// if id is already existing just return fail
			if (temp_node.id == id) {
				return 0;
			}
			node_count_in_list--;
			// temp_node will be the next node if not the last node in list
			if (node_count_in_list != 0) {
				temp_node = *(first_shm_node_ptr + temp_node.next_index);
			}
		}
	}

	// Pick one node from list and add it to SHM & hash list
	pool_ds->free_node_in_pool--;
	// Use temp_node to store the picked node
	temp_node = *list_entry(pool_ds->first_free_node, node_in_shm_t, free_node_link);
	// Change first_free_node and release the older one by list_del_init()
	pool_ds->first_free_node = pool_ds->first_free_node->next;
	list_del_init(pool_ds->first_free_node->prev);
	// Give the elements of temp_node value and put it in the hash_list_ptr
	temp_node.id = id;
	temp_node.data = data;
	temp_node.next_index = hash_list_ptr->hash_array[id_hash_value][1];
	hash_list_ptr->hash_array[id_hash_value][1] = temp_node.self_index;
	hash_list_ptr->hash_array[id_hash_value][0]++;
	// And the last one thing is put it in SHM
	memcpy(first_shm_node_ptr + temp_node.self_index, &(temp_node), sizeof(node_in_shm_t));
	return 1;
}

int del_node_in_shm(hlist_node_pool_t *pool_ds, node_in_shm_t *first_shm_node_ptr,
					hlist_t *hash_list_ptr, int id, int data)
{
	node_in_shm_t *temp_node, *node_before_temp_node;
	int i, node_count_in_list, id_hash_value, 
		index_before_temp_node = -1, del_node_not_exist = 1;

	// Check if there exists the node we want to delete
	// Attach the first node of list  to prepare the process
	id_hash_value = id % HASH_LIST_SIZE;
	node_count_in_list = hash_list_ptr->hash_array[id_hash_value][0];
	temp_node = first_shm_node_ptr + hash_list_ptr->hash_array[id_hash_value][1];
	// Check the whole list
	for (i=0; i<node_count_in_list; i++) {
		if (temp_node->id == id) {
			del_node_not_exist = 0;
			// temp_node is the delete node after break
			break;
		}
		// if there are more than one node in list, store the index of previous node
		if (temp_node->next_index != -1) {
			index_before_temp_node = temp_node->self_index;
			temp_node = first_shm_node_ptr + temp_node->next_index;
		}
	}

	if (del_node_not_exist) {
		return 0;
	}

	// if temp_node is the first node in hash_list
	if (hash_list_ptr->hash_array[id_hash_value][1] == temp_node->self_index) {
		// let the second node be the new first node in list
		// it will be -1 even no second node exist
		hash_list_ptr->hash_array[id_hash_value][1] = temp_node->next_index;
	}
	// 1. change the link in hash list and remove the delete node
	// if not the first node in the list, let previous node link to next node
	if (index_before_temp_node != -1) {
		node_before_temp_node = first_shm_node_ptr + index_before_temp_node;
		node_before_temp_node->next_index = temp_node->next_index;
	}
	// initialize the value of removed node
	temp_node->next_index = -1;
	// 2. add the temp_node into the free list 
	// temp_node is an initialized node is pool_ds of self index
	temp_node = &(pool_ds->node_memory_pool[temp_node->self_index]);
	// add the temp_node before the first node of free node list
	list_add_tail(&(temp_node->free_node_link), pool_ds->first_free_node);
	// let the temp_node be the new first free node of the list
	pool_ds->first_free_node = &(temp_node->free_node_link);
	// update the statistic information
	hash_list_ptr->hash_array[id_hash_value][0]--;
	pool_ds->free_node_in_pool++;
	return 1;
}

int dump_all_node_in_shm(node_in_shm_t *first_shm_node_ptr, hlist_t *hash_list_ptr)
{
	node_in_shm_t *temp_node;
	int i, j, node_count_in_shm = 0;
	char string[STRING_SIZE];
	FILE *file_ptr;

	file_ptr = fopen("dump_file.txt", "w+");
	// Calculate the total nodes number before write to txt file
	// and put string of node information to file simutaneously
	for (i=0; i<HASH_LIST_SIZE; i++) {
		node_count_in_shm += hash_list_ptr->hash_array[i][0];
		temp_node = first_shm_node_ptr + hash_list_ptr->hash_array[i][1];
		for (j=0; j<hash_list_ptr->hash_array[i][0]; j++) {
			sprintf(string, "Node in %d-th hash list with ID: %d, Data: %d\n",
					i, temp_node->id, temp_node->data);
			fputs(string, file_ptr);
			temp_node = first_shm_node_ptr + temp_node->next_index;
		}
	}
	fclose(file_ptr);
	return 1;
}
