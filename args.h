// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdlib.h>

struct arg_spec;
typedef const char* (*arg_parser_t)(const char* arg, void* slot);

struct arg_spec {
  const char* flag;
  bool takes_arg;
  bool required;
  arg_parser_t parser;
  size_t offset;
  const char* help;
};

const char* u32_parser(const char* arg, void* slot);
const char* bool_parser(const char* arg, void* slot);
const char* str_parser(const char* arg, void* slot);

char* parse_args(int argc, const char* argv[],
                 struct arg_spec const* const specs, void* ctx);

char* create_usage(const char* cmd, const char* description,
                   struct arg_spec const* const specs);
