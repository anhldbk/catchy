#include <gtest/gtest.h>
#include <atomic>
#include "src/lock/SeqLock.h"
#include <stdint.h>
#include <string.h>
#include "src/utils.h"

using namespace std;

typedef SeqLock<uint32_t> SeqIntLock;

TEST(SeqLockTest, testGeneral) {
    SeqIntLock lock(0);
    
    uint32_t loop = 30, threads = 5;
    
    auto callback = [&]() {
        for (uint32_t i = 0; i < loop; i++) {
            lock.write(lock.read() + 1);
        }
    };
    utils::run_in_pool(callback, threads);    
    
    EXPECT_EQ(lock.read(), loop * threads);
}
