#include <cstddef>
#include <cstdint>

#ifndef HASH_TABLE

#define TEMP_MAX 1000
#define TEMP_MIN -1000

struct ws_data {
    int sum = 0;
    int cnt = 0;
    int min = TEMP_MAX;
    int max = TEMP_MIN;
    char station[101] = "";
};

typedef struct hash_table_node {
    ws_data data;
    struct hash_table_node* next;
}
hash_table_node;

inline int min(int a, int b) { return a < b ? a : b; }
inline int max(int a, int b) { return a > b ? a : b; }

class HashTableWS {
public:
    HashTableWS(uint32_t hash_table_size = 1024);
    ~HashTableWS();

    uint32_t size();
    hash_table_node* find(const char* station, int len);
    void insert(const char station[101], int temperature, int len);
    void update(hash_table_node* node, int temperature);

    struct Iterator {
        hash_table_node** table;
        uint32_t table_size;
        uint32_t bucket;
        hash_table_node* node;

        void advance() {
            while (bucket < table_size) {
                if (node != NULL) return;
                ++bucket;
                node = (bucket < table_size) ? table[bucket] : NULL;
            }
        }

        Iterator (hash_table_node** table, uint32_t table_size, uint32_t bucket, hash_table_node* node)     
        : table(table), table_size(table_size), bucket(bucket), node(node) {
            advance();
        }

        hash_table_node& operator*() { return *node;}
        hash_table_node* operator->() { return node;}

        Iterator& operator++ () {
            node = node->next;
            advance();
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return node != other.node || bucket != other.bucket;
        }
    };

    Iterator begin() { 
        return Iterator(hash_table, hash_table_size, 0, hash_table ? hash_table[0] : NULL); 
    }

    Iterator end()   { 
        return Iterator(hash_table, hash_table_size, hash_table_size, NULL);
    }

private:
    uint32_t crc;
    uint32_t table[256];
    uint32_t hash_table_size = 0u;
    hash_table_node** hash_table = NULL;
    hash_table_node* free_hash_table_node = NULL;

    uint32_t hash_function(const char *str, const char* end);
    hash_table_node* allocate_hash_table_node(void);
};

#endif // !HASH_TABLE
#define HASH_TABLE
