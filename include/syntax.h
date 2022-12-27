#ifndef __SYNTAX_H__
#define __SYNTAX_H__

typedef struct syntax_t syntax_t;

struct syntax_t {
  char lang[32];
  
  int (*f_depth)(const char *line, int length);
  char (*f_pair)(const char *line, int length, char chr);
};

syntax_t st_init(const char *filename);

#endif
