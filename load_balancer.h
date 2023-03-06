/* Copyright 2021 <Anghel Andrei - Stelian> */
#ifndef LOAD_BALANCER_H_
#define LOAD_BALANCER_H_

#include "/home/student/server.h"

struct load_balancer;
typedef struct load_balancer load_balancer;

struct load_balancer {
	int *hash_ring;
    int server_number;
    linked_list_t* server_list;
};

unsigned int hash_function_servers(void *a);

unsigned int hash_function_key(void *a);

load_balancer* init_load_balancer();

server_memory* get_server_from_list(load_balancer* main, int server_id);

int get_server_pos_from_list(load_balancer* main, server_memory* server);

void add_elem_vect(load_balancer* main, int pos, int elem);

void elim_elem_vect(load_balancer* main, int pos);

void transfer_values(server_memory* server, int label,
    server_memory* next_server, load_balancer* main);

void free_load_balancer(load_balancer* main);

/**
 * loader_store() - Stores the key-value pair inside the system.
 * @arg1: Load balancer which distributes the work.
 * @arg2: Key represented as a string.
 * @arg3: Value represented as a string.
 * @arg4: This function will RETURN via this parameter 
 *        the server ID which stores the object.
 *
 * The load balancer will use Consistent Hashing to distribute the 
 * load across the servers. The chosen server ID will be returned 
 * using the last parameter.
 */

void loader_add_server_empty_hash_ring(load_balancer* main,
                        int server_id, server_memory* server);

void loader_store(load_balancer* main, char* key, char* value, int* server_id);

/**
 * loader_retrieve() - Gets a value associated with the key.
 * @arg1: Load balancer which distributes the work.
 * @arg2: Key represented as a string.
 * @arg3: This function will RETURN the server ID 
          which stores the value via this parameter.
 *
 * The load balancer will search for the server which should posess the 
 * value associated to the key. The server will return NULL in case 
 * the key does NOT exist in the system.
 */
char* loader_retrieve(load_balancer* main, char* key, int* server_id);

/**
 * loader_add_server() - Adds a new server to the system.
 * @arg1: Load balancer which distributes the work.
 * @arg2: ID of the new server.
 *
 * The load balancer will generate 3 replica TAGs and it will
 * place them inside the hash ring. The neighbor servers will 
 * distribute some the objects to the added server.
 */
void loader_add_server(load_balancer* main, int server_id);

/**
 * loader_remove_server() - Removes a specific server from the system.
 * @arg1: Load balancer which distributes the work.
 * @arg2: ID of the removed server.
 *
 * The load balancer will distribute ALL objects stored on the
 * removed server and will delete ALL replicas from the hash ring.
 */
void loader_remove_server(load_balancer* main, int server_id);


#endif  /* LOAD_BALANCER_H_ */
