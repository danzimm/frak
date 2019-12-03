// Copywrite (c) 2019 Dan Zimmerman

#include <frakl/utils.h>

#include "tests.h"

TEST(RoundToNextPowerOfTwo) {
  EXPECT_EQ(round_to_next_power_of_two(0), 0);
  EXPECT_EQ(round_to_next_power_of_two(1), 1);
  EXPECT_EQ(round_to_next_power_of_two(2), 2);
  EXPECT_EQ(round_to_next_power_of_two(3), 4);
  EXPECT_EQ(round_to_next_power_of_two(4), 4);
  EXPECT_EQ(round_to_next_power_of_two(10), 16);
  EXPECT_EQ(round_to_next_power_of_two(32), 32);
  EXPECT_EQ(round_to_next_power_of_two(33), 64);
  EXPECT_EQ(round_to_next_power_of_two(70000), 131072);
  EXPECT_EQ(round_to_next_power_of_two(2500000001), 0x100000000);
}
