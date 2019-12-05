// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdlib.h>

struct arg_spec;
typedef char* (*arg_parser_t)(const char* arg, void* slot, void* ctx);
typedef char* (*arg_help_printer_t)(struct arg_spec const* const spec);

struct arg_spec {
  const char* flag;
  bool takes_arg;
  bool required;
  arg_parser_t parser;
  void* parser_ctx;
  size_t offset;
  const char* help;
  arg_help_printer_t help_printer;
};

char* pu32_parser(const char* arg, void* slot, void* ctx);
char* bool_parser(const char* arg, void* slot, void* ctx);
char* str_parser(const char* arg, void* slot, void* ctx);
char* pdbl_parser(const char* arg, void* slot, void* ctx);

typedef struct arg_enum_opt {
  const char* option;
  unsigned value;
} * arg_enum_opt_t;

// ctx is a arg_enum_opt_t, ending with a 0 entry
char* enum_parser(const char* arg, void* slot, void* ctx);

struct tuple_spec {
  uint16_t count;
  bool is_double;
};

char* tuple_parser(const char* arg, void* slot, void* ctx);

char* parse_args(int argc, const char* argv[],
                 struct arg_spec const* const specs, void (*initializer)(void*),
                 char* (*validator)(void*), void* ctx);

char* create_usage(const char* cmd, const char* description,
                   struct arg_spec const* const specs);
