/* Copyright 2021 <Anghel Andrei - Stelian> */

#include <stdlib.h>
#include <string.h>
#include "/home/student/load_balancer.h"
#include "/home/student/LinkedList.h"
#include "/home/student/server.h"
#include "/home/student/Hashtable.h"
#include "/home/student/utils.h"

#define MAX_SERVER_NUMBER 300000

/*Functia returneaza hash-ul etichetei unui server.*/
unsigned int hash_function_servers(void *a) {
    unsigned int uint_a = *((unsigned int *)a);

    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

/*Funtia returneaza hash-ul unei chei.*/
unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *) a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

/*Initializarea structurii de tipul load_balancer.*/
load_balancer* init_load_balancer() {
	load_balancer* main_server = malloc(sizeof(load_balancer));
    DIE(!main_server, "Malloc error.\n");

    /*Hash ring-ul este initializat cu valoarea 0.*/
    main_server->hash_ring = calloc(MAX_SERVER_NUMBER, sizeof(int));
    DIE(!main_server->hash_ring, "Malloc error.\n");

    main_server->server_number = 0;
    main_server->server_list = ll_create(sizeof(server_memory));

    return main_server;
}

/*Parcurge lista de servere si returneaza adresa server-ului
corespunzator id-ului primit ca parametru.*/
server_memory* get_server_from_list(load_balancer* main, int server_id)
{
    ll_node_t* current = main->server_list->head;
    while (current) {
        if (((server_memory*)(current->data))->id == server_id)
            return (server_memory*)(current->data);
        current = current->next;
    }
    return NULL;
}

/*Parcurge lista de servere si returneaza pozitia pe care se afla
server-ul primit ca parametru. Determinarea acestei pozitii se face
pentru ca ulterior sa poata fi aplicata functia de eliminare a unui
element din lista - ll_remove_nth_node().*/
int get_server_pos_from_list(load_balancer* main, server_memory* server)
{
    ll_node_t* current = main->server_list->head;
    int pos = 0;
    while (current) {
        if (((server_memory*)(current->data))->id == server->id)
            return pos;
        current = current->next;
        pos++;
    }
    return -1;
}

/*Adauga perechea cheie - valoare pe server-ul corespunzator, in functie de
valorile hash-urilor.*/
void loader_store(load_balancer* main, char* key, char* value, int* server_id) {
	if (hash_function_key(key) <=
    hash_function_servers(&main->hash_ring[0]) || hash_function_key(key) >
    hash_function_servers(&main->hash_ring[main->server_number - 1])) {
        server_memory* server =
                    get_server_from_list(main, (main->hash_ring[0]) % 100000);
        server_store(server, key, value);
        *server_id = (main->hash_ring[0]) % 100000;
        return;
    }

    for (int i = 0; i < main->server_number - 1; i++) {
        if (hash_function_key(key) >= hash_function_servers(&main->hash_ring[i])
        && hash_function_key(key) <
        hash_function_servers(&main->hash_ring[i + 1])) {
           server_memory* server =
                get_server_from_list(main, (main->hash_ring[i + 1]) % 100000);
           server_store(server, key, value);
           *server_id = (main->hash_ring[i + 1]) % 100000;
           return;
        }
    }
}

/*Determina server-ul pe care se afla cheia in functie de hash-ul acesteia
si extrage valoarea asociata cheii din hashtable.*/
char* loader_retrieve(load_balancer* main, char* key, int* server_id) {
	if (hash_function_key(key) <= hash_function_servers(&(main->hash_ring[0]))
    || hash_function_key(key) >
    hash_function_servers(&main->hash_ring[main->server_number - 1])) {
        server_memory* server =
                    get_server_from_list(main, (main->hash_ring[0]) % 100000);
        char* value = server_retrieve(server, key);
        *server_id = (main->hash_ring[0]) % 100000;
        return value;
    }

    for (int i = 0; i < main->server_number - 1; i++) {
        if (hash_function_key(key) >= hash_function_servers(&main->hash_ring[i])
        && hash_function_key(key) <
        hash_function_servers(&(main->hash_ring[i + 1]))) {
            server_memory* server =
                get_server_from_list(main, (main->hash_ring[i + 1]) % 100000);
           char* value = server_retrieve(server, key);
           *server_id = (main->hash_ring[i + 1]) % 100000;
           return value;
        }
    }

	return NULL;
}

/*Adauga un element pe pozitia data in vectorul ce stocheaza
etichetele. Tot aici se face incrementarea numarului de
elemente ale hash ring-ului.*/
void add_elem_vect(load_balancer* main, int pos, int elem)
{
    for (int i = main->server_number; i > pos; i--)
        main->hash_ring[i] = main->hash_ring[i - 1];
    main->hash_ring[pos] = elem;
    (main->server_number)++;
}

/*Elimina elementul de la pozitia data din vectorul ce stocheaza
etichetele. Tot aici se face decrementarea numarului de
elemente ale hash ring-ului.*/
void elim_elem_vect(load_balancer* main, int pos)
{
    for (int i = pos; i < main->server_number - 1; i++) {
        main->hash_ring[i] = main->hash_ring[i + 1];
    }
    (main->server_number)--;
}

/*Functia redistribuie elementele stocate pe "next_server" si le transfera,
daca este necesar, pe server-ul "server" (ce reprezinta serverul nou
adaugat in sistem). Toate cheile stocate pe next_server sunt hash-uite
si comparate cu hash-ul variabilei "label" (eticheta corespunzatoare uneia 
dintre replicile noului server).*/
void transfer_values(server_memory* server, int label,
    server_memory* next_server, load_balancer* main)
{
    unsigned int max_hash =
            hash_function_servers(&main->hash_ring[main->server_number - 1]);
    for (unsigned int i = 0; i < next_server->ht->hmax; i++) {
        ll_node_t* current = next_server->ht->buckets[i]->head;
        while (current) {
            void* key = ((info*)(current->data))->key;
            if (hash_function_key(key) <= hash_function_servers(&label) ||
            hash_function_key(key) > max_hash) {
                void* value = ((info*)(current->data))->value;
                server_remove(server, key);
                ht_put(server->ht, key, strlen(key) + 1, value,
                    strlen(value) + 1);
            }
            current = current->next;
        }
    }
}

/*Aceasta functie adauga in hash ring etichetele corespunzatoare primului
server adaugat. (cazul initial, cand hash ring-ul este gol).*/
void loader_add_server_empty_hash_ring(load_balancer* main,
    int server_id, server_memory* server)
{
    main->hash_ring[0] = server_id;
        (main->server_number)++;

        /*Formula de calcul a etichetei.*/
        int label_1 = 100000 + server_id;
        if (hash_function_servers(&label_1) >=
            hash_function_servers(&main->hash_ring[0])) {
            add_elem_vect(main, 1, label_1);
        } else if (hash_function_servers(&label_1) <
        hash_function_servers(&main->hash_ring[0])) {
            add_elem_vect(main, 0, label_1);
        }

        int label_2 = 200000 + server_id;
        if (hash_function_servers(&label_2) >=
        hash_function_servers(&main->hash_ring[1])) {
            add_elem_vect(main, 2, label_2);
        } else if (hash_function_servers(&label_2) <
        hash_function_servers(&main->hash_ring[0])) {
            add_elem_vect(main, 0, label_2);
        } else if (hash_function_servers(&label_2) ==
        hash_function_servers(&main->hash_ring[0])) {
            add_elem_vect(main, 1, label_2);
        } else {
            add_elem_vect(main, 1, label_2);
        }
        free(server);
}

/*Principalul rol al functiei este de a adauga in hash ring
etichetele corespunzatoare server-ului nou. Se realizeaza si
adaugarea la lista a noului server si "rebalansarea" datelor.*/
void loader_add_server(load_balancer* main, int server_id) {
	/*Noul server este creat si adaugat in lista.*/
    server_memory* server = init_server_memory();
    server->id = server_id;
    ll_add_nth_node(main->server_list, 0, server);

    /*Cazul in care hash ring-ul este gol.*/
    if (main->server_number == 0) {
        loader_add_server_empty_hash_ring(main, server_id, server);
        return;
    }

    /*Retin pozitia pe care este adaugata eticheta in hash
    ring pentru a putea determina eticheta urmatoare.*/
    int pos = 0;
    for (int j = 0; j <= 2; j++) {
        /*Formula de calcul a etichetei.*/
        int label = j * 100000 + server_id;
        if (hash_function_servers(&label) >
        hash_function_servers(&main->hash_ring[main->server_number - 1])) {
            pos = main->server_number;
            add_elem_vect(main, main->server_number, label);
        } else if (hash_function_servers(&label) <
        hash_function_servers(&main->hash_ring[0])) {
            add_elem_vect(main, 0, label);
            pos = 0;
        } else {
            int added = 0;
            for (int i = 0; i < main->server_number - 1; i++) {
                if (hash_function_servers(&label) ==
                hash_function_servers(&main->hash_ring[i])) {
                    /*In caz ca hash-urile sunt egale
                    sortez dupa valoarea etichetei.*/
                    if (label > main->hash_ring[i]) {
                        add_elem_vect(main, i + 1, label);
                        pos = i + 1;
                        added = 1;
                        break;
                    }
                    if (label < main->hash_ring[i]) {
                        add_elem_vect(main, i, label);
                        pos = i;
                        added = 1;
                        break;
                    }
                }
                if (hash_function_servers(&label) >
                hash_function_servers(&main->hash_ring[i]) &&
                hash_function_servers(&label) <
                hash_function_servers(&main->hash_ring[i + 1])) {
                        add_elem_vect(main, i + 1, label);
                        pos = i + 1;
                        added = 1;
                        break;
                    }
            }
            if (added == 0 && hash_function_servers(&label) ==
            hash_function_servers(&main->hash_ring[main->server_number - 1])) {
                if (label > main->hash_ring[main->server_number - 1]) {
                        pos = main->server_number;
                        add_elem_vect(main, main->server_number, label);
                    }

                    if (label < main->hash_ring[main->server_number - 1]) {
                        pos = main->server_number - 1;
                        add_elem_vect(main, main->server_number - 1, label);
                    }
            }
        }
        /*Determinarea urmatorului server cu
        ajutorul circularitatii vectorului hash ring.*/
        int next_label = main->hash_ring[(pos + 1) % (main->server_number)];
        server_memory* next_server =
                            get_server_from_list(main, next_label % 100000);
        /*Daca urmatoarea eticheta in sens orar corespunde
        unui server diferit, are loc "rebalansarea".*/
        if ((next_server->id) != server_id)
            transfer_values(server, label, next_server, main);
    }
    free(server);
}

/*Eliminarea din hash ring a replicilor unui server,
redistribuirea perechilor cheie - valoare si eliminarea
server-ului din lista.*/
void loader_remove_server(load_balancer* main, int server_id) {
	/*Eliminarea din hash-ring a tuturor
    replicilor asociate serverului.*/
    for (int j = 0; j <= 2; j++) {
        int label = j * 100000 + server_id;
        for (int i = 0; i < main->server_number; i++) {
            if (main->hash_ring[i] == label) {
                elim_elem_vect(main, i);
                break;
            }
        }
    }

    /*Parcurgerea hashtable-ului asociat server-ului.*/
    server_memory* server = get_server_from_list(main, server_id);
    for (unsigned int i = 0; i < server->ht->hmax; i++) {
        ll_node_t* current = server->ht->buckets[i]->head;
        while (current) {
            void* key = ((info*)(current->data))->key;
            void* value = ((info*)(current->data))->value;
            /*Variabila auxiliara al carei
            rezultat nu il vom folosi.*/
            int trash = 0;
            /*Toate perechile cheie - valoare din 
            serverul ce va fi eliminat sunt redistribuite prin 
            intermediul functiei loader_store(). Acest lucru
            functioneaza doar daca replicile server-ului eliminat
            au fost eliminate din hash ring in prealabil.*/
            loader_store(main, key, value, &trash);
            current = current->next;
        }
    }
    /*Eliminarea server-ului din lista.*/
    int pos = get_server_pos_from_list(main, server);
    ll_node_t* node = ll_remove_nth_node(main->server_list, pos);
    free_server_memory((server_memory*)(node->data));
    free(node);
}

/*Eliberarea memoriei alocate pentru stocarea
structurii de tip load_balaner.*/
void free_load_balancer(load_balancer* main) {
    ll_free(&(main->server_list));
    free(main->hash_ring);
    free(main);
}
