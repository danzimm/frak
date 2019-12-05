// Copywrite (c) 2019 Dan Zimmerman

#include <frakl/args.h>
#include <stddef.h>
#include <stdint.h>
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

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", ""}, specs, NULL, NULL,
                             ctx));
  EXPECT_EQ(ctx[0], NULL);
  EXPECT_STREQ(ctx[1], "");

  char* err = parse_args(1, (const char*[]){"--noarg"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--noarg': programmer error, str options "
               "require an argument");
  free(err);
}

TEST(ArgsPDbl) {
  double ctx[2];
  const struct arg_spec specs[] = {
      {
          .flag = "--noarg",
          .parser = pdbl_parser,
          .offset = 0,
      },
      {
          .flag = "--arg",
          .takes_arg = true,
          .parser = pdbl_parser,
          .offset = sizeof(double),
      },
      {.flag = NULL},
  };

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", "3"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], 0.0);
  EXPECT_EQ(ctx[1], 3.0);

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", "3.0"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], 0.0);
  EXPECT_EQ(ctx[1], 3.0);

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", "3.00"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], 0.0);
  EXPECT_EQ(ctx[1], 3.0);

  char* err =
      parse_args(2, (const char*[]){"--arg", ""}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg ': unexpected empty string");
  free(err);

  err =
      parse_args(2, (const char*[]){"--arg", "3.00."}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--arg 3.00.': expected number, error at .");
  free(err);

  err = parse_args(2, (const char*[]){"--arg", "foo"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg foo': expected number, error at f");
  free(err);

  err = parse_args(2, (const char*[]){"--arg", "0.0"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg 0.0': expected positive number");
  free(err);

  err =
      parse_args(2, (const char*[]){"--arg", "-10.2"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg -10.2': expected positive number");
  free(err);

  err = parse_args(1, (const char*[]){"--noarg"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--noarg': programmer error, dbl options "
               "require an arg");
  free(err);
}

TEST(ArgsPU32) {
  uint32_t ctx[2];
  const struct arg_spec specs[] = {
      {
          .flag = "--noarg",
          .parser = pu32_parser,
          .offset = 0,
      },
      {
          .flag = "--arg",
          .takes_arg = true,
          .parser = pu32_parser,
          .offset = sizeof(uint32_t),
      },
      {.flag = NULL},
  };

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", "3"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], 0);
  EXPECT_EQ(ctx[1], 3);

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", "012345"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], 0);
  EXPECT_EQ(ctx[1], 012345);

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--arg", "0x10"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], 0);
  EXPECT_EQ(ctx[1], 16);

  char* err =
      parse_args(2, (const char*[]){"--arg", ""}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg ': unexpected empty string");
  free(err);

  err = parse_args(2, (const char*[]){"--arg", "3.00"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--arg 3.00': expected number, error at .");
  free(err);

  err = parse_args(2, (const char*[]){"--arg", "foo"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg foo': expected number, error at f");
  free(err);

  err = parse_args(2, (const char*[]){"--arg", "0"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg 0': can't be 0");
  free(err);

  err = parse_args(2, (const char*[]){"--arg", "4294967296"}, specs, NULL, NULL,
                   ctx);
  EXPECT_STREQ(err, "Unable to parse '--arg 4294967296': too large");
  free(err);

  err = parse_args(1, (const char*[]){"--noarg"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--noarg': programmer error, u32 options "
               "require an arg");
  free(err);
}

TEST(ArgsEnum) {
  unsigned ctx[2];
  const struct arg_enum_opt pets[] = {
      {"cat", 1}, {"fish", 2},  {"turtle", 3},
      {"dog", 4}, {"puppy", 5}, {NULL, 0},
  };
  const struct arg_enum_opt fish[] = {
      {"bass", 1},
      {"cat", 2},
      {"carp", 3},
      {NULL, 0},
  };
  const struct arg_spec specs[] = {
      {
          .flag = "--pet",
          .takes_arg = true,
          .parser = enum_parser,
          .parser_ctx = (void*)pets,
          .offset = 0,
      },
      {
          .flag = "--fish",
          .takes_arg = true,
          .parser = enum_parser,
          .parser_ctx = (void*)fish,
          .offset = sizeof(unsigned),
      },
      {
          .flag = "--fish2",
          .parser = enum_parser,
          .parser_ctx = (void*)fish,
          .offset = sizeof(unsigned),
      },
      {
          .flag = "--fish3",
          .takes_arg = true,
          .parser = enum_parser,
          .offset = sizeof(unsigned),
      },
      {.flag = NULL},
  };

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--pet", "turtle"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[0], 3);

  bzero(ctx, sizeof(ctx));
  EXPECT_EQ(NULL, parse_args(2, (const char*[]){"--fish", "bass"}, specs, NULL,
                             NULL, ctx));
  EXPECT_EQ(ctx[1], 1);

  char* err = parse_args(1, (const char*[]){"--fish2"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--fish2': programmer error, enum options "
               "require an arg");
  free(err);

  err =
      parse_args(2, (const char*[]){"--fish3", "bass"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--fish3 bass': programmer error, enum options "
               "require a parser_ctx (a arg_enum_opt_t)");
  free(err);

  err =
      parse_args(2, (const char*[]){"--fish", "shark"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(
      err, "Unable to parse '--fish shark': expected one of bass, cat, carp");
  free(err);
}

struct tuple_ctx {
  // Padding just to force extra padding for alignment
  uint8_t padding;
  long lngs[3];
  // Padding just to force extra padding for alignment
  uint8_t padding2;
  double dbls[2];
  // Padding just to force extra padding for alignment
  uint8_t padding3;
};

TEST(ArgsTuple) {
  struct tuple_spec dbl_spec = {
      .count = 2,
      .is_double = true,
  };
  struct tuple_spec lng_spec = {
      .count = 3,
      .is_double = false,
  };
  const struct arg_spec specs[] = {
      {
          .flag = "--noarg",
          .parser = tuple_parser,
          .offset = 0,
      },
      {
          .flag = "--arg-noctx",
          .takes_arg = true,
          .parser = tuple_parser,
          .offset = 0,
      },
      {
          .flag = "--twodbl",
          .takes_arg = true,
          .parser = tuple_parser,
          .offset = offsetof(struct tuple_ctx, dbls),
          .parser_ctx = &dbl_spec,
      },
      {
          .flag = "--threelng",
          .takes_arg = true,
          .parser = tuple_parser,
          .offset = offsetof(struct tuple_ctx, lngs),
          .parser_ctx = &lng_spec,
      },
      {.flag = NULL},
  };

  struct tuple_ctx ctx;
  bzero(&ctx, sizeof(ctx));

  char* err =
      parse_args(1, (const char*[]){"--noarg"}, specs, NULL, NULL, &ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--noarg': programmer error, tuple options "
               "require an argument");
  free(err);

  err = parse_args(2, (const char*[]){"--arg-noctx", "arg"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--arg-noctx arg': programmer error, tuple "
               "options require a context (tuple_spec)");
  free(err);

  bzero(&ctx, sizeof(ctx));
  EXPECT_STREQ(NULL, parse_args(2, (const char*[]){"--twodbl", "1.0,2.0"},
                                specs, NULL, NULL, &ctx));
  EXPECT_EQ(ctx.dbls[0], 1.0);
  EXPECT_EQ(ctx.dbls[1], 2.0);

  bzero(&ctx, sizeof(ctx));
  EXPECT_STREQ(NULL, parse_args(2, (const char*[]){"--twodbl", "0.0,-2.2"},
                                specs, NULL, NULL, &ctx));
  EXPECT_EQ(ctx.dbls[0], 0.0);
  EXPECT_EQ(ctx.dbls[1], -2.2);

  bzero(&ctx, sizeof(ctx));
  EXPECT_STREQ(NULL, parse_args(2, (const char*[]){"--twodbl", "-3,4"}, specs,
                                NULL, NULL, &ctx));
  EXPECT_EQ(ctx.dbls[0], -3.0);
  EXPECT_EQ(ctx.dbls[1], 4.0);

  bzero(&ctx, sizeof(ctx));
  EXPECT_STREQ(NULL, parse_args(2, (const char*[]){"--twodbl", "3e5,4"}, specs,
                                NULL, NULL, &ctx));
  EXPECT_EQ(ctx.dbls[0], 3e5);
  EXPECT_EQ(ctx.dbls[1], 4.0);

  bzero(&ctx, sizeof(ctx));
  EXPECT_STREQ(NULL, parse_args(2, (const char*[]){"--threelng", "3,4,5"},
                                specs, NULL, NULL, &ctx));
  EXPECT_EQ(ctx.lngs[0], 3);
  EXPECT_EQ(ctx.lngs[1], 4);
  EXPECT_EQ(ctx.lngs[2], 5);

  bzero(&ctx, sizeof(ctx));
  EXPECT_STREQ(NULL, parse_args(2, (const char*[]){"--threelng", "-3,0,255"},
                                specs, NULL, NULL, &ctx));
  EXPECT_EQ(ctx.lngs[0], -3);
  EXPECT_EQ(ctx.lngs[1], 0);
  EXPECT_EQ(ctx.lngs[2], 255);

  err = parse_args(2, (const char*[]){"--threelng", "3.0,4,5"}, specs, NULL,
                   NULL, &ctx);
  EXPECT_STREQ(
      err, "Unable to parse '--threelng 3.0,4,5': expected long, error at '.'");
  free(err);

  err = parse_args(2, (const char*[]){"--twodbl", "3a,4"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(
      err, "Unable to parse '--twodbl 3a,4': expected double, error at 'a'");
  free(err);

  err = parse_args(2, (const char*[]){"--twodbl", "3a,4"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(
      err, "Unable to parse '--twodbl 3a,4': expected double, error at 'a'");
  free(err);

  err =
      parse_args(2, (const char*[]){"--twodbl", "3,"}, specs, NULL, NULL, &ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--twodbl 3,': expected non empty tuple item "
               "at index 1");
  free(err);

  err = parse_args(2, (const char*[]){"--threelng", "3,,5"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--threelng 3,,5': expected non empty tuple "
               "item at index 1");
  free(err);

  err = parse_args(2, (const char*[]){"--threelng", ",1,5"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(err,
               "Unable to parse '--threelng ,1,5': expected non empty tuple "
               "item at index 0");
  free(err);

  err = parse_args(2, (const char*[]){"--threelng", "1,5"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(err, "Unable to parse '--threelng 1,5': expected 3 longs");
  free(err);

  err = parse_args(2, (const char*[]){"--threelng", "1,5,9,10"}, specs, NULL,
                   NULL, &ctx);
  EXPECT_STREQ(err, "Unable to parse '--threelng 1,5,9,10': expected 3 longs");
  free(err);

  err = parse_args(2, (const char*[]){"--twodbl", "1,5,5"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(err, "Unable to parse '--twodbl 1,5,5': expected 2 doubles");
  free(err);

  err = parse_args(2, (const char*[]){"--twodbl", "1.0"}, specs, NULL, NULL,
                   &ctx);
  EXPECT_STREQ(err, "Unable to parse '--twodbl 1.0': expected 2 doubles");
  free(err);
}

TEST(ArgsExpectedArgs) {
  const char* ctx[2];
  const struct arg_spec specs[] = {
      {
          .flag = "--arg",
          .takes_arg = true,
          .parser = str_parser,
          .offset = sizeof(const char*),
      },
      {.flag = NULL},
  };

  char* err = parse_args(1, (const char*[]){"--arg"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Expected argument following --arg");
  free(err);
}

TEST(ArgsUnexpectedArg) {
  const char* ctx[2];
  const struct arg_spec specs[] = {
      {
          .flag = "--arg",
          .takes_arg = true,
          .parser = str_parser,
          .offset = sizeof(const char*),
      },
      {.flag = NULL},
  };

  char* err = parse_args(1, (const char*[]){"--arg2"}, specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unknown arg '--arg2'");
  free(err);

  const struct arg_spec empty_specs[] = {
      {.flag = NULL},
  };
  err = parse_args(1, (const char*[]){"--arg"}, empty_specs, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unknown arg '--arg'");
  free(err);

  err = parse_args(1, (const char*[]){"--arg"}, NULL, NULL, NULL, ctx);
  EXPECT_STREQ(err, "Unknown arg '--arg'");
  free(err);
}

struct test_ctx {
  const char* str;
  double dbl;
  uint32_t i;
  bool b;
};

static void init_test_ctx(struct test_ctx* ctx) {
  bzero(ctx, sizeof(struct test_ctx));
}

static char* validate_test_ctx(struct test_ctx* ctx) {
  if (ctx->b && ctx->str && strcmp(ctx->str, "nobool") == 0) {
    return strdup("Cannot pass bool and nobool");
  }
  return NULL;
}

TEST(ArgsParsesHetrogenousSpec) {
  const struct arg_spec specs[] = {
      {
          .flag = "--str",
          .takes_arg = true,
          .parser = str_parser,
          .offset = offsetof(struct test_ctx, str),
      },
      {
          .flag = "--dbl",
          .takes_arg = true,
          .parser = pdbl_parser,
          .offset = offsetof(struct test_ctx, dbl),
      },
      {
          .flag = "--u32",
          .takes_arg = true,
          .parser = pu32_parser,
          .offset = offsetof(struct test_ctx, i),
      },
      {
          .flag = "--bool",
          .parser = bool_parser,
          .offset = offsetof(struct test_ctx, b),
      },
      {.flag = NULL},
  };
  struct test_ctx ctx;

  ctx.str = "hello";
  EXPECT_EQ(NULL, parse_args(7,
                             (const char*[]){"--bool", "--dbl", "3.0", "--str",
                                             "foo", "--u32", "10"},
                             specs, (void*)init_test_ctx,
                             (void*)validate_test_ctx, &ctx));
  EXPECT_STREQ(ctx.str, "foo");
  EXPECT_EQ(ctx.i, 10);
  EXPECT_EQ(ctx.dbl, 3.0);
  EXPECT_TRUE(ctx.b);

  ctx.str = "hello";
  EXPECT_EQ(NULL,
            parse_args(4, (const char*[]){"--dbl", "4.0", "--u32", "11"}, specs,
                       (void*)init_test_ctx, (void*)validate_test_ctx, &ctx));
  EXPECT_EQ(ctx.str, NULL);
  EXPECT_EQ(ctx.i, 11);
  EXPECT_EQ(ctx.dbl, 4.0);
  EXPECT_FALSE(ctx.b);

  EXPECT_EQ(NULL, parse_args(0, NULL, specs, (void*)init_test_ctx,
                             (void*)validate_test_ctx, &ctx));
  EXPECT_EQ(ctx.str, NULL);
  EXPECT_EQ(ctx.i, 0);
  EXPECT_EQ(ctx.dbl, 0.0);
  EXPECT_FALSE(ctx.b);

  char* err = parse_args(3, (const char*[]){"--str", "nobool", "--bool"}, specs,
                         (void*)init_test_ctx, (void*)validate_test_ctx, &ctx);
  EXPECT_STREQ(err, "Cannot pass bool and nobool");
  free(err);
  EXPECT_EQ(ctx.str, "nobool");
  EXPECT_EQ(ctx.i, 0);
  EXPECT_EQ(ctx.dbl, 0);
  EXPECT_TRUE(ctx.b);
}

TEST(ArgsParsesHetrogenousSpecRequired) {
  const struct arg_spec specs[] = {
      {
          .flag = "--str",
          .takes_arg = true,
          .required = true,
          .parser = str_parser,
          .offset = offsetof(struct test_ctx, str),
      },
      {
          .flag = "--dbl",
          .takes_arg = true,
          .parser = pdbl_parser,
          .offset = offsetof(struct test_ctx, dbl),
      },
      {
          .flag = "--u32",
          .takes_arg = true,
          .required = true,
          .parser = pu32_parser,
          .offset = offsetof(struct test_ctx, i),
      },
      {
          .flag = "--bool",
          .parser = bool_parser,
          .offset = offsetof(struct test_ctx, b),
      },
      {.flag = NULL},
  };
  struct test_ctx ctx;

  EXPECT_EQ(NULL, parse_args(7,
                             (const char*[]){"--bool", "--dbl", "3.0", "--str",
                                             "foo", "--u32", "10"},
                             specs, (void*)init_test_ctx,
                             (void*)validate_test_ctx, &ctx));
  EXPECT_STREQ(ctx.str, "foo");
  EXPECT_EQ(ctx.i, 10);
  EXPECT_EQ(ctx.dbl, 3.0);
  EXPECT_TRUE(ctx.b);

  EXPECT_EQ(NULL,
            parse_args(4, (const char*[]){"--u32", "4", "--str", "11"}, specs,
                       (void*)init_test_ctx, (void*)validate_test_ctx, &ctx));
  EXPECT_STREQ(ctx.str, "11");
  EXPECT_EQ(ctx.i, 4);

  char* err =
      parse_args(4, (const char*[]){"--dbl", "4.0", "--u32", "11"}, specs,
                 (void*)init_test_ctx, (void*)validate_test_ctx, &ctx);
  EXPECT_STREQ(err, "Missing required arg: --str");
  free(err);

  err = parse_args(4, (const char*[]){"--dbl", "4.0", "--str", "11"}, specs,
                   (void*)init_test_ctx, (void*)validate_test_ctx, &ctx);
  EXPECT_STREQ(err, "Missing required arg: --u32");
  free(err);

  err = parse_args(0, (const char*[]){}, specs, (void*)init_test_ctx,
                   (void*)validate_test_ctx, &ctx);
  EXPECT_STREQ(err, "Missing required arg: --str");
  free(err);
}
