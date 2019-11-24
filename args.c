// Copywrite (c) 2019 Dan Zimmerman

#include "args.h"

#include <stdio.h>
#include <string.h>

const char* u32_parser(const char* arg, void* slot) {
  if (*arg == '\0') {
    return "unexpected empty string";
  }
  char* tmp;
  unsigned long x = strtoul(arg, &tmp, 0);
  if (*tmp != '\0') {
    return "expected number";
  }
  if (x >= UINT32_MAX) {
    return "number to large";
  }
  *(uint32_t*)slot = (uint32_t)x;
  return NULL;
}

const char* bool_parser(const char* arg, void* slot) {
  if (arg) {
    return "unexpected argument";
  }
  *(bool*)slot = true;
  return NULL;
}
const char* str_parser(const char* arg, void* slot) {
  if (!arg) {
    return "expected argument";
  }
  *(const char**)slot = arg;
  return NULL;
}

char* parse_args(int argc, const char* argv[],
                 struct arg_spec const* const specs, void* ctx) {
  if (specs == NULL || specs->flag == NULL) {
    return NULL;
  }
  const char** iter = &argv[0];
  const char** end = iter + argc;

  struct arg_spec const* spec_iter;
  char* err = NULL;
  bool found;

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

      const char* spec_err = spec_iter->parser(arg, ctx + spec_iter->offset);
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

out:
  return err;
}
