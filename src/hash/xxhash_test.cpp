#include <stdio.h>

#define XXH_STATIC_LINKING_ONLY   /* *_state_t */
#include "xxhash.h"

using namespace std;

int main() {
    const char* data = "hello world";
    cout << hex << get_hash((void*) data, strlen(data), 3);
    return 0;
}
