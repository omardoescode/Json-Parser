#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#define debug(...) printf("LOGS: " __VA_ARGS__)
#else
#define debug(...)
#endif

typedef enum {
  JSON_NULL,
  JSON_ARRAY,
  JSON_OBJECT,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOL
} JsonType;

/**
 * ignores space in the file pointer
 * returns the character after the space
 */
int ignore_space(FILE *f) {
  int c;
  do {
    c = fgetc(f);
  } while (isspace(c));
  return c;
}

typedef struct Json Json;

struct Json {
  JsonType type;
  union {
    struct {
      unsigned int length;
      Json **items;
    } array;
    struct {
      unsigned int count;
      char **keys;
      Json **values;
    } obj;
    char *str;
    long long number;
    bool bl;
  } cnt;
};

Json *json_parse(FILE *f);

Json *json_make_str(const char *cnt) {
  if (!cnt)
    return NULL;
  Json *result = malloc(sizeof(Json));
  if (!result) {
    perror("malloc");
    exit(1);
  }
  result->type = JSON_STRING;
  result->cnt.str = strdup(cnt);
  if (!result->cnt.str) {
    free(result);
    return NULL;
  }
  return result;
}

Json *json_make_number(long long number) {
  Json *result = malloc(sizeof(Json));
  if (!result) {
    perror("malloc");
    exit(1);
  }
  result->type = JSON_NUMBER;
  result->cnt.number = number;
  return result;
}

Json *json_make_boolean(bool bl) {
  Json *result = malloc(sizeof(Json));

  if (!result) {
    perror("malloc");
    exit(1);
  }
  result->type = JSON_BOOL;
  result->cnt.bl = bl;
  return result;
}

Json *json_make_null() {
  Json *result = malloc(sizeof(Json));
  if (!result) {
    perror("malloc");
    exit(1);
  }
  result->type = JSON_NULL;
  // no payload
  return result;
}

Json *json_make_array(unsigned int length) {
  Json *result = malloc(sizeof(Json));

  result->type = JSON_ARRAY;
  result->cnt.array.length = length;
  result->cnt.array.items = calloc(length, sizeof(Json *));

  if (!result->cnt.array.items) {
    free(result);
    return NULL;
  }

  return result;
}

Json *json_make_object(unsigned int length) {
  Json *result = malloc(sizeof(Json));
  if (!result) {
    perror("malloc");
    exit(1);
  }
  result->type = JSON_OBJECT;
  result->cnt.obj.count = length;

  if (length > 0) {
    result->cnt.obj.keys = calloc(length, sizeof(char *));
    result->cnt.obj.values = calloc(length, sizeof(Json *));
    if (!result->cnt.obj.keys || !result->cnt.obj.values) {
      free(result->cnt.obj.keys);
      free(result->cnt.obj.values);
      free(result);
      return NULL;
    }
  }
  return result;
}

void json_free(Json *json) {
  if (!json)
    return;
  switch (json->type) {
  case JSON_STRING:
    free(json->cnt.str);
    break;
  case JSON_ARRAY:
    for (unsigned int i = 0; i < json->cnt.array.length; i++)
      json_free(json->cnt.array.items[i]);
    free(json->cnt.array.items);
    break;
  case JSON_OBJECT:
    for (unsigned int i = 0; i < json->cnt.obj.count; i++) {
      free(json->cnt.obj.keys[i]);
      json_free(json->cnt.obj.values[i]);
    }
    free(json->cnt.obj.keys);
    free(json->cnt.obj.values);
    break;
  case JSON_NUMBER:
  case JSON_BOOL:
  case JSON_NULL:
    break;
  }

  free(json);
}

// for debugging
void json_print(Json *json) {
  assert(json);

  switch (json->type) {
  case JSON_NULL:
    printf("null");
    break;

  case JSON_BOOL:
    printf(json->cnt.bl ? "true" : "false");
    break;

  case JSON_NUMBER:
    printf("%lld", json->cnt.number);
    break;

  case JSON_STRING:
    printf("\"%s\"", json->cnt.str);
    break;

  case JSON_ARRAY:
    printf("[");
    for (unsigned int i = 0; i < json->cnt.array.length; i++) {
      if (i > 0)
        printf(", ");
      json_print(json->cnt.array.items[i]);
    }
    printf("]");
    break;

  case JSON_OBJECT:
    printf("{");
    for (unsigned int i = 0; i < json->cnt.obj.count; i++) {
      if (i > 0)
        printf(", ");
      printf("\"%s\": ", json->cnt.obj.keys[i]);
      json_print(json->cnt.obj.values[i]);
    }
    printf("}");
    break;
  }
}

Json *parse_string(FILE *f) {
  assert(f);
  debug("parse_string: entering\n");
  int capacity = 1024, length = 0;
  char *buffer = malloc(capacity * sizeof(char));
  if (!buffer) {
    perror("malloc");
    exit(1);
  }
  int c;
  while ((c = fgetc(f)) != '"') {
    if (c == EOF) {
      fprintf(stderr, "Invalid Json Syntax");
      exit(1);
    }

    if (length == capacity - 1) {
      capacity *= 2;
      char *new_buf = realloc(buffer, capacity);
      if (!new_buf) {
        perror("realloc");
        free(buffer);
        exit(1);
      }
      buffer = new_buf;
    }

    buffer[length++] = c;
  }

  buffer[length] = '\0';
  Json *result = json_make_str(buffer);
  free(buffer);
  debug("parse_string: returning string result\n");
  return result;
}

Json *parse_number(FILE *f, int first_char) {
  debug("parse_number: entering\n");
  bool negative = first_char == '-';
  long long number = 0;
  int c = first_char;

  if (negative) {
    c = fgetc(f);
    if (!isdigit(c)) {
      fprintf(stderr, "Invalid number\n");
      exit(1);
    }
  }

  number = c - '0';

  while (c = fgetc(f), isdigit(c))
    number = number * 10 + (c - '0');

  if (c != EOF)
    ungetc(c, f);
  debug("parse_number: returning number result\n");
  return json_make_number(negative ? -number : number);
}

Json *parse_null(FILE *f, char first_char) {
  debug("parse_null: entering\n");
  assert(first_char == 'n');
  char buf[4];
  size_t n = fread(buf, 1, 3, f);
  buf[n] = '\0';
  if (n != 3 || strcmp(buf, "ull") != 0) {
    fprintf(stderr, "Invalid JSON\n");
    exit(1);
  }
  debug("parse_null: returning null\n");
  return json_make_null();
}
Json *parse_bool(FILE *f, char first_char) {
  debug("parse_bool: entering\n");
  assert(first_char == 't' || first_char == 'f');
  if (first_char == 't') {
    char buf[4];
    size_t n = fread(buf, 1, 3, f);
    buf[n] = '\0';
    if (n != 3 || strcmp(buf, "rue") != 0) {
      fprintf(stderr, "Invalid JSON\n");
      exit(1);
    }
    debug("parse_bool: returning boolean result\n");
    return json_make_boolean(true);
  } else {
    char buf[5];
    size_t n = fread(buf, 1, 4, f);
    buf[n] = '\0';
    if (n != 4 || strcmp(buf, "alse") != 0) {
      fprintf(stderr, "Invalid JSON\n");
      exit(1);
    }
    debug("parse_bool: returning boolean result\n");
    return json_make_boolean(false);
  }
}

Json *parse_array(FILE *f) {
  debug("parse_array: entering\n");
  int c;
  int length = 0;
  int capacity = 0;
  Json **items = NULL;
  for (;;) {
    c = ignore_space(f);
    if (c == EOF) {
      fprintf(stderr, "Invalid Json");
      exit(1);
    }

    if (c == ']')
      break;

    ungetc(c, f);
    Json *new_item = json_parse(f);

    if (length == capacity) {
      capacity = 2 * capacity + 1;
      items = realloc(items, capacity * sizeof(Json *));
      if (!items) {
        perror("realloc");
        exit(1);
      }
    }

    items[length++] = new_item;
    int c = ignore_space(f);
    if (c == ',') {
      continue;
    } else if (c == ']') {
      break;
    } else {
      fprintf(stderr, "Expected ',' or ']', but got '%c'\n", c);
      exit(1);
    }
  }

  if (length > 0) {
    items = realloc(items, length * sizeof(Json *));
    if (!items) {
      perror("realloc");
      exit(1);
    }
  }

  Json *result = json_make_array(length);
  result->cnt.array.items = items;
  debug("parse_array: returning array result\n");
  return result;
}

Json *parse_object(FILE *f) {
  debug("parse_object: entering\n");
  int c;
  unsigned int capacity = 1, count = 0;
  char **keys = malloc(sizeof(char *) * capacity);
  Json **values = malloc(sizeof(Json *) * capacity);

  if (!keys || !values) {
    free(keys);
    free(values);
    perror("malloc");
    exit(1);
  }

  while ((c = ignore_space(f)) != '}') {
    if (c != '"') {
      for (unsigned int i = 0; i < count; i++)
        free(keys[i]), json_free(values[i]);
      free(keys);
      free(values);
      fprintf(stderr, "Invalid JSON: Expected string key\n");
      exit(1);
    }

    Json *key_json = parse_string(f);
    char *k = strdup(key_json->cnt.str);
    json_free(key_json);

    int colon = ignore_space(f);
    if (colon != ':') {
      for (unsigned int i = 0; i < count; i++)
        free(keys[i]), json_free(values[i]);
      free(keys);
      free(values);
      fprintf(stderr, "Invalid JSON: Expected ':' after key\n");
      exit(1);
    }

    Json *value = json_parse(f);

    if (count == capacity) {
      capacity *= 2;
      keys = realloc(keys, capacity * sizeof(char *));
      values = realloc(values, capacity * sizeof(Json *));
      if (!keys || !values) {
        for (unsigned int i = 0; i < count; i++)
          free(keys[i]), json_free(values[i]);
        free(keys);
        free(values);
        perror("realloc");
        exit(1);
      }
    }

    keys[count] = k;
    values[count] = value;
    count++;

    c = ignore_space(f);
    if (c == ',')
      continue;
    else if (c == '}')
      break;
    else {
      for (unsigned int i = 0; i < count; i++)
        free(keys[i]), json_free(values[i]);
      free(keys);
      free(values);
      fprintf(stderr, "Invalid JSON: Expected ',' or '}'\n");
      exit(1);
    }
  }

  Json *result = json_make_object(count);
  result->cnt.obj.keys = keys;
  result->cnt.obj.values = values;
  debug("parse_object: returning object result\n");
  return result;
}

Json *json_parse(FILE *f) {
  debug("json_parse: entering\n");
  assert(f);

  // this first character defines everything
  int c = ignore_space(f);

  if (c == EOF) {
    fprintf(stderr, "Invalid json");
    exit(1);
  } else if (c == '"') {
    debug("json_parse: dispatching to parse_string\n");
    return parse_string(f);
  } else if (isdigit(c) || c == '-') {
    debug("json_parse: dispatching to parse_number\n");
    return parse_number(f, c);
  } else if (c == 't' || c == 'f') {
    debug("json_parse: dispatching to parse_bool\n");
    return parse_bool(f, c);
  } else if (c == '[') {
    debug("json_parse: dispatching to parse_array\n");
    return parse_array(f);
  } else if (c == '{') {
    debug("json_parse: dispatching to parse_object\n");
    return parse_object(f);
  } else if (c == 'n') {
    return parse_null(f, c);
  }

  fprintf(stderr, "Invalid Json");
  exit(1);
}

int main(int argc, char **argv) {
  FILE *f = stdin;
  if (argc > 1) {
    f = fopen(argv[1], "r");
    if (f == NULL) {
      perror("fopen");
      return 1;
    }
  }

  Json *j = json_parse(f);
  json_print(j);
  puts("");

  if (f != stdin)
    fclose(f);
  return 0;
}
