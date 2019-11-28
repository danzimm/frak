// Copywrite (c) 2019 Dan Zimmerman

#include "tests.h"

TEST(MetaTests) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
  EXPECT_TRUE(0x3);
  EXPECT_FALSE(false);
  EXPECT_FALSE(0);
}

TEST_FAIL(ExpectEQFail) {
  EXPECT_EQ(1, 2);
  EXPECT_EQ(2, 3);
}

TEST_FAIL(ExpectTrueFail) {
  EXPECT_TRUE(false);
  EXPECT_TRUE(true && false);
}

TEST_FAIL(ExpectFalseFail) {
  EXPECT_FALSE(true);
  EXPECT_FALSE(true || false);
}
