#include <string.h>
#include <syntax.h>

int st_c_depth(const char *line, int length) {
  char in_string = '\0';
  int depth = 0;
  
  for (int i = 0; i < length; i++) {
    if (in_string) {
      if (line[i] == '\\') {
        i++;
      } else if (line[i] == in_string) {
        in_string = '\0';
      }
    } else if (strchr("\"'", line[i])) {
      in_string = line[i];
    } else if (strchr("([{", line[i])) {
      depth++;
    } else if (strchr(")]}", line[i])) {
      depth--;
    }
  }
  
  if (depth < 0) depth = 0;
  return depth;
}

char st_c_pair(const char *line, int length, char chr) {
  char in_string = '\0';
  
  for (int i = 0; i < length; i++) {
    if (in_string) {
      if (line[i] == '\\') {
        i++;
      } else if (line[i] == in_string) {
        in_string = '\0';
      }
    } else if (strchr("\"'", line[i])) {
      in_string = line[i];
    }
  }
  
  if (in_string) return '\0';
  
  if (chr == '(') return ')';
  if (chr == '[') return ']';
  if (chr == '{') return '}';
  if (chr == '"') return '"';
  if (chr == '\'') return '\'';
  
  return '\0';
}
