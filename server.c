/* Copyright 2021 <Anghel Andrei - Stelian> */
#include <stdlib.h>
#include <string.h>

#include "/home/student/server.h"
#include "/home/student/utils.h"

#define BUCKET_NUMBER 50

/*Initializarea structurii de tipul server_memory.*/
server_memory* init_server_memory() {
	server_memory* server = malloc(sizeof(server_memory));
	DIE(!server, "Malloc error.\n");

	server->id = 0;
	server->ht = ht_create(BUCKET_NUMBER, hash_function_string,
				compare_function_strings);

	return server;
}

/*Adaugarea unei perechi cheie - valoare folosind
functia specifica structurii de hashtable.*/
void server_store(server_memory* server, char* key, char* value) {
	ht_put(server->ht, key, strlen(key) + 1, value, strlen(value) + 1);
}

/*Eliminarea unei perechi cheie - valoare folosind
functia specifica structurii de hashtable.*/
void server_remove(server_memory* server, char* key) {
	ht_remove_entry(server->ht, key);
}

/*Gasirea valorii asociate unei chei folosind
functia specifica structurii de hashtable.*/
char* server_retrieve(server_memory* server, char* key) {
	char* value = NULL;
	value =  (char*)ht_get(server->ht, key);

	if (value)
		return value;
	return NULL;
}

/*Eliberarea memoriei alocate pentru stocarea
structurii de tip server_memory.*/
void free_server_memory(server_memory* server) {
	ht_free(server->ht);
	free(server);
}
