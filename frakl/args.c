// Copywrite (c) 2019 Dan Zimmerman

#include "args.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define asprintf(...)              \
  do {                             \
    int a = asprintf(__VA_ARGS__); \
    (void)a;                       \
  } while (0)
#define vasprintf(...)              \
  do {                              \
    int a = vasprintf(__VA_ARGS__); \
    (void)a;                        \
  } while (0)

char* pu32_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  char* result = NULL;
  const char* cresult = NULL;
  if (!arg) {
    cresult = "programmer error, u32 options require an arg";
    goto out;
  }
  if (*arg == '\0') {
    cresult = "unexpected empty string";
    goto out;
  }
  char* tmp;
  unsigned long x = strtoul(arg, &tmp, 0);
  if (*tmp != '\0') {
    asprintf(&result, "expected number, error at %c", *tmp);
    goto out;
  }
  if (x >= UINT32_MAX) {
    cresult = "too large";
    goto out;
  }
  if (x == 0) {
    cresult = "can't be 0";
    goto out;
  }
  *(uint32_t*)slot = (uint32_t)x;
out:
  if (cresult != NULL) {
    result = strdup(cresult);
  }
  return result;
}

char* bool_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  if (arg) {
    return strdup("programmer error, bool options can't take arguments");
  }
  *(bool*)slot = true;
  return NULL;
}

char* str_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  if (!arg) {
    return strdup("programmer error, str options require an argument");
  }
  *(const char**)slot = arg;
  return NULL;
}

char* pdbl_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  char* result = NULL;
  const char* cresult = NULL;
  if (!arg) {
    cresult = "programmer error, dbl options require an arg";
    goto out;
  }
  if (*arg == '\0') {
    cresult = "unexpected empty string";
    goto out;
  }
  char* tmp;
  double value = strtod(arg, &tmp);
  if (*tmp != '\0') {
    asprintf(&result, "expected number, error at %c", *tmp);
    goto out;
  }
  if (value <= 0.0) {
    cresult = "expected positive number";
    goto out;
  }

  *(double*)slot = value;

out:
  if (cresult != NULL) {
    result = strdup(cresult);
  }
  return result;
}

static char* strconcat(char* left, const char* right) {
  left = realloc(left, strlen(left) + strlen(right) + 1);
  return strcat(left, right);
}

static char* strconcat_enum_opts(char* result, arg_enum_opt_t opts) {
  do {
    result = strconcat(result, opts->option);
    if ((opts + 1)->option != NULL) {
      result = strconcat(result, ", ");
    }
  } while ((++opts)->option != NULL);
  return result;
}

char* enum_parser(const char* arg, void* slot, void* ctx) {
  arg_enum_opt_t opts = ctx;
  if (!opts || !opts->option) {
    return strdup(
        "programmer error, enum options require a parser_ctx (a "
        "arg_enum_opt_t)");
  }
  if (!arg) {
    return strdup("programmer error, enum options require an arg");
  }
  do {
    if (strcmp(opts->option, arg) == 0) {
      *(unsigned*)slot = opts->value;
      return NULL;
    }
  } while ((++opts)->option != NULL);
  return strconcat_enum_opts(strdup("expected one of "), ctx);
}

static void cp_num(const char** in_iter, char** out) {
  const char* const base = *in_iter;
  const char* iter = base;
  while (*iter != '\0' && *iter != ',') {
    iter++;
  }
  unsigned len = iter - base;
  if (*iter == ',') {
    iter += 1;
  }
  *in_iter = iter;
  char* res = malloc(len + 1);
  memcpy(res, base, len);
  res[len] = '\0';
  *out = res;
}

union value_holder {
  double dbl;
  long lng;
};

static union value_holder strtovh(const char* str, char** endptr, bool dbl) {
  if (dbl) {
    return (union value_holder){.dbl = strtod(str, endptr)};
  } else {
    return (union value_holder){.lng = strtol(str, endptr, 0)};
  }
}

static void bind_value_holder(void** io_slot, union value_holder vh, bool dbl) {
  void* slot = *io_slot;
  if (dbl) {
    *(double*)slot = vh.dbl;
    slot += sizeof(double);
  } else {
    *(long*)slot = vh.lng;
    slot += sizeof(long);
  }
  *io_slot = slot;
}

char* tuple_parser(const char* arg, void* slot, void* ctx) {
  if (!arg) {
    return strdup("programmer error, tuple options require an argument");
  }
  if (!ctx) {
    return strdup(
        "programmer error, tuple options require a context (tuple_spec)");
  }

  const struct tuple_spec* spec = ctx;
  const uint16_t count = spec->count;
  const bool is_double = spec->is_double;
  char* res = NULL;
  char* num_buf = NULL;
  uint16_t i;

  for (i = 0; i < count && res == NULL; i++) {
    if (*arg == '\0') {
      break;
    }

    cp_num(&arg, &num_buf);
    if (*num_buf == '\0' || (*arg == '\0' && arg[-1] == ',')) {
      asprintf(&res, "expected non empty tuple item at index %d",
               i + (*num_buf == '\0' ? 0 : 1));
      goto loop_out;
    }

    char* tmp;
    union value_holder vh = strtovh(num_buf, &tmp, is_double);
    if (*tmp != '\0') {
      asprintf(&res, "expected %s, error at '%c'",
               is_double ? "double" : "long", *tmp);
      goto loop_out;
    } else {
      bind_value_holder(&slot, vh, is_double);
    }
    free(num_buf);
  }

  if (i != count || *arg != '\0') {
    asprintf(&res, "expected %d %s%s", count, is_double ? "double" : "long",
             count != 1 ? "s" : "");
  }

out:
  return res;

loop_out:
  if (num_buf) {
    free(num_buf);
  }
  goto out;
}

static unsigned get_specs_len(struct arg_spec const* const specs) {
  struct arg_spec const* iter = specs;
  while (iter && iter->flag) iter++;
  return iter - specs;
}

char* parse_args(int argc, const char* argv[],
                 struct arg_spec const* const specs, void (*initializer)(void*),
                 char* (*validator)(void*), void* ctx) {
  if (initializer) {
    initializer(ctx);
  }
  char* err = NULL;
  bool* did_parse = NULL;
  const char** iter = &argv[0];
  const char** end = iter + argc;
  if (specs == NULL || specs->flag == NULL) {
    if (iter != end) {
      asprintf(&err, "Unknown arg '%s'", *iter);
    }
    goto out;
  }

  struct arg_spec const* spec_iter;
  bool found;

  did_parse = calloc(get_specs_len(specs), sizeof(bool));

  if (iter == end) {
    goto check_required;
  }
  do {
    found = false;
    spec_iter = &specs[0];
    do {
      const char* arg;
      if (*spec_iter->flag == '-') {
        if (strcmp(*iter, spec_iter->flag) != 0) {
          continue;
        }

        if (spec_iter->takes_arg) {
          if (++iter == end) {
            asprintf(&err, "Expected argument following %s", spec_iter->flag);
            goto out;
          }
          arg = *iter;
        } else {
          arg = NULL;
        }
      } else if (**iter != '-') {
        arg = *iter;
      } else {
        continue;
      }
      found = true;
      did_parse[spec_iter - specs] = true;

      char* spec_err = spec_iter->parser(arg, ctx + spec_iter->offset,
                                         spec_iter->parser_ctx);
      if (spec_err) {
        asprintf(&err, "Unable to parse '%s%s%s': %s", spec_iter->flag,
                 arg ? " " : "", arg ?: "", spec_err);
        free(spec_err);
        goto out;
      }
      break;
    } while ((++spec_iter)->flag != NULL);
    if (!found) {
      asprintf(&err, "Unknown arg '%s'", *iter);
      goto out;
    }
  } while ((++iter) != end);

check_required:
  spec_iter = specs;
  do {
    if (spec_iter->required && !did_parse[spec_iter - specs]) {
      asprintf(&err, "Missing required arg: %s", spec_iter->flag);
      goto out;
    }
  } while ((++spec_iter)->flag != NULL);

out:
  if (did_parse) {
    free(did_parse);
  }
  return err ?: (validator ? validator(ctx) : NULL);
}

static const char* skip_dash(const char* str) {
  while (*str == '-' && *str != '\0') str++;
  return str;
}

static unsigned get_max_option_width(struct arg_spec const* const specs) {
  unsigned width = 0;
  if (!specs || !specs->flag) {
    return width;
  }
  struct arg_spec const* iter = specs;
  do {
    unsigned w;
    if (*iter->flag == '-') {
      w = strlen(iter->flag) + strlen(skip_dash(iter->flag)) + 3;
    } else {
      w = strlen(iter->flag) + 2;
    }
    if (w > width) {
      width = w;
    }
  } while ((++iter)->flag != NULL);
  return width;
}

static void asprintf_padded(char** out, unsigned width, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  char* tmp;
  vasprintf(&tmp, fmt, va);
  va_end(va);
  asprintf(out, "%-*s", width, tmp);
  free(tmp);
}

static const char* find_last_within(const char* str, char c, unsigned width) {
  unsigned slen = strlen(str);
  const char* iter = str + (slen < width ? slen : width);
  while (iter != str && *iter != c) iter--;
  return iter == str ? NULL : iter;
}

static void str_word_chunk(const char** str, char* out, unsigned out_width) {
  if (**str == '\0') {
    *out = '\0';
    return;
  }
  unsigned out_len;
  unsigned space = 0;
  unsigned slen = strlen(*str);

  if (slen <= out_width) {
    out_len = slen;
    goto out;
  }

  const char* next_space = find_last_within(*str, ' ', out_width);
  if (next_space == NULL) {
    out_len = out_width;
  } else {
    out_len = next_space - (const char*)*str;
    space = 1;
  }

out:
  memcpy(out, *str, out_len);
  out[out_len] = '\n';
  out[out_len + 1] = '\0';
  *str += out_len + space;
}

char* compute_requireds_str(struct arg_spec const* const specs) {
  char* result = strdup("");
  struct arg_spec const* iter = specs;
  do {
    if (iter->required) {
      result = strconcat(strconcat(result, " "), iter->flag);
      if (*iter->flag == '-') {
        result = strconcat(strconcat(result, " "), skip_dash(iter->flag));
      }
    }
  } while ((++iter)->flag != NULL);
  return result;
}

static char* create_printer_prefix(struct arg_spec const* const spec) {
  char* result;
  if (spec->help) {
    asprintf(&result, "%s. ", spec->help);
  } else {
    result = strdup("");
  }
  return result;
}

static char* pu32_help_printer(struct arg_spec const* const spec) {
  return strconcat(create_printer_prefix(spec), "Must be a positive integer");
}

static char* pdbl_help_printer(struct arg_spec const* const spec) {
  return strconcat(create_printer_prefix(spec),
                   "Must be a positive floating point number");
}

static char* enum_help_printer(struct arg_spec const* const spec) {
  char* result = create_printer_prefix(spec);
  arg_enum_opt_t opts = spec->parser_ctx;
  if (!opts || !opts->option) {
    return strconcat(result, "No valid options, programmer error");
  }
  result = strconcat(result, "Valid options are: ");
  result = strconcat_enum_opts(result, opts);
  return result;
}

static arg_help_printer_t get_default_help_printer(
    struct arg_spec const* const spec) {
  arg_parser_t parser = spec->parser;
  if (parser == pu32_parser) {
    return pu32_help_printer;
  } else if (parser == enum_parser) {
    return enum_help_printer;
  } else if (parser == pdbl_parser) {
    return pdbl_help_printer;
  }
  return NULL;
}

static char* default_help_printer(struct arg_spec const* const spec) {
  return strdup(spec->help);
}

static char* get_help_summary(struct arg_spec const* const spec) {
  arg_help_printer_t help_printer =
      spec->help_printer ?: get_default_help_printer(spec);
  return (help_printer ?: default_help_printer)(spec);
}

char* create_usage(const char* cmd, const char* description,
                   struct arg_spec const* const specs) {
  char* prologue = NULL;
  char* args = strdup("");
  char* tmp = NULL;
  const char* ctmp = NULL;
  char* result = NULL;
  char* summary = NULL;
  char* requireds = NULL;
  char* flag_padding = NULL;
  struct arg_spec const* iter;
  unsigned arg_width;
  unsigned summary_width;
  bool print_required = true;

  if (!specs || !specs->flag) {
    return NULL;
  }
  iter = specs;

  arg_width = get_max_option_width(specs) + 1;
  summary_width = 80 - arg_width;

  asprintf(&flag_padding, "%-*s", arg_width, "");
  summary = malloc(summary_width + 2);

  // Prologue
  requireds = compute_requireds_str(specs);
  asprintf(&prologue, "%s: %s.\nUsage: %s [options]%s\n", cmd, description, cmd,
           requireds);
  bool has_requireds = *requireds != '\0';
  free(requireds);
  if (has_requireds) {
    args = strconcat(args, "Required:\n");
  } else {
    goto print_nonrequired;
  }

  // Options
print_opts:
  do {
    // Print the flag
    if (print_required ^ iter->required) {
      continue;
    }
    if (*iter->flag == '-' && iter->takes_arg) {
      asprintf_padded(&tmp, arg_width, "%s [%s]", iter->flag,
                      skip_dash(iter->flag));
    } else {
      asprintf_padded(&tmp, arg_width, "%s", iter->flag);
    }
    args = strconcat(args, tmp);
    free(tmp);

    // Print the summary
    tmp = get_help_summary(iter);
    ctmp = tmp;
    str_word_chunk(&ctmp, summary, summary_width);
    args = strconcat(args, summary);

    str_word_chunk(&ctmp, summary, summary_width);
    while (*summary != '\0') {
      args = strconcat(args, flag_padding);
      args = strconcat(args, summary);
      str_word_chunk(&ctmp, summary, summary_width);
    }
    free(tmp);
  } while ((++iter)->flag != NULL);

print_nonrequired:
  if (print_required) {
    print_required = false;
    iter = specs;
    args = strconcat(args, "Options:\n");
    goto print_opts;
  }

  asprintf(&result, "%s\n%s", prologue, args);
  free(prologue);
  free(args);
  free(summary);
  free(flag_padding);
  return result;
}
