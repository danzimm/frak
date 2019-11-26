// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdlib.h>

struct arg_spec;
typedef const char* (*arg_parser_t)(const char* arg, void* slot, void* ctx);

struct arg_spec {
  const char* flag;
  bool takes_arg;
  bool required;
  arg_parser_t parser;
  void* parser_ctx;
  size_t offset;
  const char* help;
};

const char* pu32_parser(const char* arg, void* slot, void* ctx);
const char* bool_parser(const char* arg, void* slot, void* ctx);
const char* str_parser(const char* arg, void* slot, void* ctx);

char* parse_args(int argc, const char* argv[],
                 struct arg_spec const* const specs, void (*initializer)(void*),
                 char* (*validator)(void*), void* ctx);

char* create_usage(const char* cmd, const char* description,
                   struct arg_spec const* const specs);
