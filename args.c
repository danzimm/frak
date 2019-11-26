// Copywrite (c) 2019 Dan Zimmerman

#include "args.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const char* pu32_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  if (*arg == '\0') {
    return "unexpected empty string";
  }
  char* tmp;
  unsigned long x = strtoul(arg, &tmp, 0);
  if (*tmp != '\0') {
    return "expected number";
  }
  if (x >= UINT32_MAX) {
    return "too large";
  }
  if (x == 0) {
    return "can't be 0";
  }
  *(uint32_t*)slot = (uint32_t)x;
  return NULL;
}

const char* bool_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  if (arg) {
    return "unexpected argument";
  }
  *(bool*)slot = true;
  return NULL;
}
const char* str_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  if (!arg) {
    return "expected argument";
  }
  *(const char**)slot = arg;
  return NULL;
}

static unsigned get_specs_len(struct arg_spec const* const specs) {
  struct arg_spec const* iter = specs;
  while (iter && iter->flag) iter++;
  return iter - specs;
}

char* parse_args(int argc, const char* argv[],
                 struct arg_spec const* const specs, void (*initializer)(void*),
                 char* (*validator)(void*), void* ctx) {
  initializer(ctx);
  char* err = NULL;
  bool* did_parse = NULL;
  if (specs == NULL || specs->flag == NULL) {
    goto out;
  }

  const char** iter = &argv[0];
  const char** end = iter + argc;
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

      const char* spec_err = spec_iter->parser(arg, ctx + spec_iter->offset,
                                               spec_iter->parser_ctx);
      if (spec_err) {
        asprintf(&err, "Unable to parse '%s%s%s': %s", spec_iter->flag,
                 arg ? " " : "", arg ?: "", spec_err);
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
  return err ?: validator(ctx);
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

static char* strconcat(char* left, const char* right) {
  left = realloc(left, strlen(left) + strlen(right) + 1);
  return strcat(left, right);
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

  if (slen < out_width) {
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

  arg_width = get_max_option_width(specs) + 2;
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
    if (*iter->flag == '-') {
      asprintf_padded(&tmp, arg_width, "%s [%s]", iter->flag,
                      skip_dash(iter->flag));
    } else {
      asprintf_padded(&tmp, arg_width, "%s", iter->flag);
    }
    args = strconcat(args, tmp);
    free(tmp);

    // Print the summary
    ctmp = iter->help;
    str_word_chunk(&ctmp, summary, summary_width);
    args = strconcat(args, summary);

    str_word_chunk(&ctmp, summary, summary_width);
    while (*summary != '\0') {
      args = strconcat(args, flag_padding);
      args = strconcat(args, summary);
      str_word_chunk(&ctmp, summary, summary_width);
    }
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
