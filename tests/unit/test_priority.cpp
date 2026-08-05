#include<gtest/gtest.h>
#include "../../src/common/Priority.h"

TEST(priorityfromString,ReturnOneForHigh ){
    EXPECT_EQ(priority_from_string("High"),1);
}
TEST(priorityfromString,ReturnTwoForMed ){
    EXPECT_EQ(priority_from_string("Med"),2);
}
TEST(priorityfromString,ReturnThreeForLow ){
    EXPECT_EQ(priority_from_string("Low"),3);
}
TEST(priorityfromString,ReturnThreeForGV ){
    EXPECT_EQ(priority_from_string("Abc"),3);
}
TEST(priorityfromString,ReturnThreeForES ){
    EXPECT_EQ(priority_from_string(""),3);
}