#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>


typedef enum {FIFO, LRU, Random} replacement_p;

const char* get_replacement_policy(uint32_t p) {
    switch(p) {
    case FIFO: return "FIFO";
    case LRU: return "LRU";
    case Random: return "Random";
    default: assert(0); return "";
    };
    return "";
}

typedef struct {
    uint32_t address;
} mem_access_t;


typedef struct {
    uint32_t cache_hits;
    uint32_t cache_misses;
} result_t;


/*
 * Parameters for the cache that will be populated by the provided code skeleton.
 */

replacement_p replacement_policy = FIFO;
uint32_t associativity = 0;
uint32_t number_of_cache_blocks = 0;
uint32_t cache_block_size = 0;



uint32_t g_num_cache_tag_bits = 0;
uint32_t g_cache_offset_bits= 0;
result_t g_result;


/* Reads a memory access from the trace file and returns
 * 32-bit physical memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1002];
    char* token = NULL;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf, 1000, ptr_file)!= NULL) {
        /* Get the address */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtoul(token, NULL, 16);
        return access;
    }

    /* If there are no more entries in the file return an address 0 */
    access.address = 0;
    return access;
}

void print_statistics(uint32_t num_cache_tag_bits, uint32_t cache_offset_bits, result_t* r) {

    uint32_t cache_total_hits = r->cache_hits;
    uint32_t cache_total_misses = r->cache_misses;
    printf("CacheTagBits:%u\n", num_cache_tag_bits);
    printf("CacheOffsetBits:%u\n", cache_offset_bits);
    printf("Cache:hits:%u\n", r->cache_hits);
    printf("Cache:misses:%u\n", r->cache_misses);
    printf("Cache:hit-rate:%2.1f%%\n", cache_total_hits / (float)(cache_total_hits + cache_total_misses) * 100.0);
}


typedef struct {
    uint32_t tag;
    int accessCounter;
} cacheEntry;
void lru(cacheEntry **cache, uint32_t tag, uint32_t index, uint32_t set_size, result_t *res);
void fifo(cacheEntry **cache, uint32_t tag, uint32_t index, uint32_t set_size, result_t *res);
void randomPolicy(cacheEntry **cache, uint32_t tag, uint32_t index, uint32_t set_size, result_t *res);

u_int32_t num_set_index_bits = 0;
int num_sets = 1;

void lru(cacheEntry **cache, uint32_t tag, uint32_t index, uint32_t set_size, result_t *res){
    uint16_t isHit=0;
    int hitIndex;
    for(uint32_t j=0; j<set_size; j++){
        if(cache[index][j].tag == tag){
            isHit=1;
            hitIndex=j;
            break;
        }     
    }
    if(isHit){
        // printf("Cache Hit \n");
        res->cache_hits++;
        for(uint32_t j=0; j<set_size; j++)
            cache[index][j].accessCounter++;
        cache[index][hitIndex].accessCounter=0;
    } else{
        // printf("Cache Miss \n");
        res->cache_misses++;
        int leastAccessed = cache[index][0].accessCounter;
        int placeToreplace = 0;
        for(uint32_t i =0;i < set_size;i++){
            if( cache[index][i].accessCounter > leastAccessed){
                leastAccessed = cache[index][i].accessCounter; 
                placeToreplace = i; 
            }
        }
        for(uint32_t j=0; j<set_size; j++)
            cache[index][j].accessCounter++;
        cache[index][placeToreplace].accessCounter=0;
        cache[index][placeToreplace].tag=tag;

    }
    
}

void fifo(cacheEntry **cache, uint32_t tag, uint32_t index, uint32_t set_size, result_t *res){
     uint16_t isHit=0;
    int hitIndex;
    for(uint32_t j=0; j<set_size; j++){
        if(cache[index][j].tag == tag){
            isHit=1;
            hitIndex=j;
            break;
        }     
    }
     if(!isHit){
        res->cache_misses++;
        // printf("Cache Miss \n");

        int firstIn=cache[index][0].accessCounter;
        uint32_t replaceIndex=0;
        int lastIn = cache[index][0].accessCounter;
        for(uint32_t i = 0;i < set_size;i++){
            if(cache[index][i].accessCounter < firstIn){
                firstIn = cache[index][i].accessCounter; 
                replaceIndex=i;
            }
            if(cache[index][i].accessCounter > lastIn){
                lastIn = cache[index][i].accessCounter; 
            }
        }
        cache[index][replaceIndex].tag = tag;
        cache[index][replaceIndex].accessCounter = ++lastIn; 
     }else{
        // printf("Cache Hit \n");
        res->cache_hits++;

     }
}

void randomPolicy(cacheEntry **cache, uint32_t tag, uint32_t index, uint32_t set_size, result_t *res){
    uint16_t isHit=0;
    uint32_t hitIndex;
    for(uint32_t j=0; j<set_size; j++){
        if(cache[index][j].tag == tag){
            isHit=1;
            hitIndex=j;
            break;
        }     
    }
     if(isHit){
         res->cache_hits++;
         //printf("Cache Hit \n");
     }else{
        res->cache_misses++;
        //printf("Cache Miss \n");
        if(cache[index][0].accessCounter <= set_size)
            cache[index][cache[index][0].accessCounter++].tag = tag;
        int randomPlace= rand() % set_size;
        cache[index][randomPlace].tag = tag;

        
     }
}


int main(int argc, char** argv) {
    time_t t;
    /* Intializes random number generator */
    /* Important: *DO NOT* call this function anywhere else. */
    srand((unsigned) time(&t));
    /* ----------------------------------------------------- */
    /* ----------------------------------------------------- */

    /*
     *
     * Read command-line parameters and initialize configuration variables.
     *
     */
    int improper_args = 0;
    char file[10000];
    if (argc < 6) {
        improper_args = 1;
        printf("Usage: ./mem_sim [replacement_policy: FIFO LRU Random] [associativity: 1 2 4 8 ...] [number_of_cache_blocks: 16 64 256 1024] [cache_block_size: 32 64] mem_trace.txt\n");
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */
        if (strcmp(argv[1], "FIFO") == 0) {
            replacement_policy = FIFO;
        } else if (strcmp(argv[1], "LRU") == 0) {
            replacement_policy = LRU;
        } else if (strcmp(argv[1], "Random") == 0) {
            replacement_policy = Random;
        } else {
            improper_args = 1;
            printf("Usage: ./mem_sim [replacement_policy: FIFO LRU Random] [associativity: 1 2 4 8 ...] [number_of_cache_blocks: 16 64 256 1024] [cache_block_size: 32 64] mem_trace.txt\n");
        }
        associativity = atoi(argv[2]);
        number_of_cache_blocks = atoi(argv[3]);
        cache_block_size = atoi(argv[4]);
        strcpy(file, argv[5]);
    }
    if (improper_args) {
        exit(-1);
    }
    assert(number_of_cache_blocks == 16 || number_of_cache_blocks == 64 || number_of_cache_blocks == 256 || number_of_cache_blocks == 1024);
    assert(cache_block_size == 32 || cache_block_size == 64);
    assert(number_of_cache_blocks >= associativity);
    assert(associativity >= 1);

    printf("input:trace_file: %s\n", file);
    printf("input:replacement_policy: %s\n", get_replacement_policy(replacement_policy));
    printf("input:associativity: %u\n", associativity);
    printf("input:number_of_cache_blocks: %u\n", number_of_cache_blocks);
    printf("input:cache_block_size: %u\n", cache_block_size);
    printf("\n");

    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file = fopen(file,"r");
    if (!ptr_file) {
        printf("Unable to open the trace file: %s\n", file);
        exit(-1);
    }

    /* result structure is initialized for you. */
    memset(&g_result, 0, sizeof(result_t));

    /* Do not delete any of the lines below.
     * Use the following snippet and add your code to finish the task. */
    num_sets= (int) number_of_cache_blocks / associativity;
    g_cache_offset_bits = log(cache_block_size)/log(2);
    num_set_index_bits = log(num_sets)/log(2);
    g_num_cache_tag_bits= 32 - num_set_index_bits - g_cache_offset_bits;

    /* You may want to setup your Cache structure here. */
    cacheEntry **cacheTable=(cacheEntry **)malloc(num_sets*sizeof(cacheEntry));
    for(int i=0; i<num_sets; i++){
        cacheTable[i]=(cacheEntry *)calloc(associativity, sizeof(cacheEntry));
    }



    int count=0;
    mem_access_t access;
    /* Loop until the whole trace file has been read. */
    while(1) {
        access = read_transaction(ptr_file);
        // If no transactions left, break out of loop.
        if (access.address == 0)
            break;

        

        uint32_t tag = access.address >> (num_set_index_bits + g_cache_offset_bits);
        uint32_t set_index = (access.address >> g_cache_offset_bits ) % (tag << num_set_index_bits);
        uint32_t offset = access.address <<(g_num_cache_tag_bits+ num_set_index_bits)>> (g_num_cache_tag_bits+ num_set_index_bits);
        
        // printf("address: 0x%x, tag: 0x%x, index:%d , offset: %d ",access.address,tag,set_index, offset);
        // if (count == 20) break;
        // count++;

        if(replacement_policy == LRU){
            lru(cacheTable, tag, set_index, associativity, &g_result);
        }else if (replacement_policy == FIFO){
            fifo(cacheTable, tag, set_index, associativity, &g_result);
        }else if(replacement_policy == Random){
            randomPolicy(cacheTable, tag, set_index, associativity, &g_result);
        }else{
            printf("the cache replacement policy you are using is not supportted");
        }

    }
    /* Freeing the allocated memory for cache table */
    for(int i = 0; i < num_sets; i++)
        free(cacheTable[i]);
    free(cacheTable);
    

    print_statistics(g_num_cache_tag_bits, g_cache_offset_bits, &g_result);

    /* Close the trace file. */
    fclose(ptr_file);
    return 0;
}
