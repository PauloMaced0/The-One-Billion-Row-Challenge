#include "hash_table.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

HashTableWS::HashTableWS(uint32_t desired_hash_table_size) {
    uint32_t i, j;

    hash_table_size = desired_hash_table_size;

    hash_table = (hash_table_node **)malloc((size_t)hash_table_size * sizeof(hash_table_node *));

    for (i = 0u; i < hash_table_size; i++) {
        hash_table[i] = NULL;
    }

    for(i = 0u;i < 256u;i++)
        for(table[i] = i,j = 0u;j < 8u;j++)
            if(table[i] & 1u)
                table[i] = (table[i] >> 1) ^ 0x17FD6B6Du; // "magic" constant
            else
                table[i] >>= 1;
}

HashTableWS::~HashTableWS() {
    uint i;
    hash_table_node *hd, *next;

    for (i = 0u; i < hash_table_size; i++) {
        hd = hash_table[i];
        while (hd != NULL) {
            next = hd->next;
            free(hd);
            hd = next;
        }

        hash_table[i] = NULL;
    }

    hd = free_hash_table_node;
    while (hd != NULL) {
        next = hd->next;
        free(hd);
        hd = next;
    }

    free(hash_table);
}

uint32_t HashTableWS::size() {
    return hash_table_size;
}

uint32_t HashTableWS::hash_function(const char *str, const char* end) {
    crc = 0x67DFEA2Bu;

    while(str < end) {
        crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
        str++;
    }
    return crc % hash_table_size;
}

hash_table_node* HashTableWS::allocate_hash_table_node(void) {
    hash_table_node* n; 
    int i;

    if (free_hash_table_node == NULL) {
        // allocate 500 nodes at a time
        free_hash_table_node = (hash_table_node* )malloc((size_t) 500 * sizeof(hash_table_node));

        for (i = 0; i < 499; i++)
            free_hash_table_node[i].next = &free_hash_table_node[i+1];
        free_hash_table_node[i].next = NULL;
    }

    n = free_hash_table_node;
    free_hash_table_node = free_hash_table_node->next;
    n->next = NULL;

    return n;
}

hash_table_node* HashTableWS::find(const char* key, int len) {
    uint32_t index = hash_function(key, key + len);
    hash_table_node* hd = hash_table[index];
    while (hd != NULL && memcmp(key, hd->data.station, len) != 0) {
        hd = hd->next;
    }
    
    return hd;
}

void HashTableWS::insert(const char* station, int temperature, int len) {
    uint32_t hash = hash_function(station, station + len);
    hash_table_node* node = allocate_hash_table_node();
    node->next = hash_table[hash];
    memcpy(node->data.station, station, len);
    node->data.station[len] = '\0';
    node->data.cnt = 1;
    node->data.sum = temperature;
    node->data.max = temperature;
    node->data.min = temperature;
    hash_table[hash] = node;
}

void HashTableWS::update(hash_table_node* node, int temperature) {
    node->data.cnt++;
    node->data.sum += temperature;
    node->data.max = max(node->data.max, temperature);
    node->data.min = min(node->data.min, temperature);
}
