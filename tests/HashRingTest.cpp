/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <gtest/gtest.h>
#include "src/HashRing.h"
#include <string.h>

using namespace std;
typedef HashNode<int> HashIntNode;
typedef HashRing<int> HashIntRing;

TEST(HashRingTest, testGeneral) {
    HashIntRing hr;
    string node_a = "node-A", node_b = "node-B";
    HashIntNode node(node_a.c_str(), 3);
    EXPECT_EQ(hr.add_node(node), true);

    node = HashIntNode(node_b.c_str(), 4);
    EXPECT_EQ(hr.add_node(node), true);

    HashIntNode* ptr = nullptr;

    ptr = hr.find_node("node-C");
    EXPECT_NE(ptr, nullptr);

    ptr = hr.find_node("node-D");
    EXPECT_NE(ptr, nullptr);

    EXPECT_EQ(hr.remove_node("node-D"), false);
    // remove node A
    EXPECT_EQ(hr.remove_node(node_a.c_str()), true);
    
    ptr = hr.find_node("node-E");
    EXPECT_NE(ptr, nullptr);  
    EXPECT_EQ(ptr->get_name(), node_b);
}
