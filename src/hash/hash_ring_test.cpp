#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <map>
#include <iostream>
#include <string.h>
#include <thread>
#include <vector>
#include <functional>


#include "hash/xxhash_wrapper.h"
#include "hash/hash_ring.h"
using namespace std;

vector<string> PERMS;

/* Function to swap values at two pointers */
void swap(char *x, char *y)
{
    char temp;
    temp = *x;
    *x = *y;
    *y = temp;
}
 
/* Function to print permutations of string
   This function takes three parameters:
   1. String
   2. Starting index of the string
   3. Ending index of the string. */
void permute(char *a, int l, int r)
{
   int i;
   if (l == r){
       std::string perm(a);
        PERMS.push_back(move(perm));
   }
   else
   {
       for (i = l; i <= r; i++)
       {
          swap((a+l), (a+i));
          permute(a, l+1, r);
          swap((a+l), (a+i)); //backtrack
       }
   }
}

void generate(){
    char str[] = "ABCDEFGH";    
    int n = strlen(str);
    PERMS.clear();
    permute(str, 0, n-1);    
}

int main() {
    printf("Test getting known nodes on ring...\n");
    const int N = 48;
//    hash_ring_t *ring = hash_ring_create(N, HASH_FUNCTION_XXHASH);
//    hash_ring_t *ring = hash_ring_create(N, HASH_FUNCTION_MD5);
    hash_ring_t *ring = hash_ring_create(N, HASH_FUNCTION_SHA1);
    const char *slotA = "slotA";
    const char *slotB = "slotB";
    
    hash_ring_node_t *node;
    
    assert(hash_ring_add_node(ring, (uint8_t*)slotA, strlen(slotA)) == HASH_RING_OK);
    assert(hash_ring_add_node(ring, (uint8_t*)slotB, strlen(slotB)) == HASH_RING_OK);

    generate();
    map <string, int> counter;
    counter[slotA] = 0;
    counter[slotB] = 0;
    for(string key:PERMS){
        node = hash_ring_find_node(ring, (uint8_t*)key.c_str(), key.length());
        counter[(char*)node->name] += 1;        
    }
    
    cout << "Slot A = " << counter[slotA] << endl;
    cout << "Slot B = " << counter[slotB] << endl;

    hash_ring_free(ring);
    return 0;
}
