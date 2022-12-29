#ifndef __SYNTAX_H__
#define __SYNTAX_H__

typedef struct syntax_t syntax_t;

enum {
  st_color_none,
  
  st_color_default,
  st_color_keyword,
  st_color_number,
  st_color_function,
  st_color_type,
  st_color_string,
  st_color_comment,
  st_color_symbol,
  
  st_color_count,
};

struct syntax_t {
  char lang[32];
  
  int (*f_depth)(const char *line, int length);
  char (*f_pair)(const char *line, int length, char chr);
  int (*f_color)(int prev_state, int *state, const char *text, int length);
};

syntax_t st_init(const char *filename);

#endif
