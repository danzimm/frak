// Copywrite (c) 2019 Dan Zimmerman

#include <frakl/args.h>
#include <strings.h>

#include "tests.h"

TEST(ArgsBool) {
  bool ctx[2];
  const struct arg_spec specs[] = {
      {
          .flag = "--noarg",
          .parser = bool_parser,
          .offset = 0,
      },
      {
          .flag = "--arg",
          .takes_arg = true,
          .parser = bool_parser,
          .offset = &ctx[1] - &ctx[0],
      },
      {.flag = NULL},
  };

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(0, NULL, specs, NULL, NULL, ctx));
  EXPECT_FALSE(ctx[0]);
  EXPECT_FALSE(ctx[1]);

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL,
            parse_args(1, (const char*[]){"--noarg"}, specs, NULL, NULL, ctx));
  EXPECT_TRUE(ctx[0]);
  EXPECT_FALSE(ctx[1]);

  char* err = parse_args(1, (const char*[]){"--arg"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Expected argument following --arg");
  free(err);

  err = parse_args(2, (const char*[]){"--arg", "bar"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--arg bar': programmer error, bool options "
               "can't take arguments");
  free(err);
}

TEST(ArgsString) {
  const char* ctx[2];
  const struct arg_spec specs[] = {
      {
          .flag = "--noarg",
          .parser = str_parser,
          .offset = 0,
      },
      {
          .flag = "--arg",
          .takes_arg = true,
          .parser = str_parser,
          .offset = sizeof(const char*),
      },
      {.flag = NULL},
  };

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", "bar"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], NULL);
  EXPECT_STREQ(ctx[1], "bar");

  char* err = parse_args(1, (const char*[]){"--noarg"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--noarg': programmer error, str options "
               "require an argument");
  free(err);
}
