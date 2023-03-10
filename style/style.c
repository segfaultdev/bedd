// Simple empty line stylizer, keeps/adds indentation to empty lines,
// as most code formatters automatically remove said indentation, which
// is "no bueno".

// TODO: Properly sort include headers too!

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void code_format(const char *path, FILE *file) {
  char **lines = calloc(sizeof(char *), 1);
  int line_count = 0;
  
  size_t length;
  
  while (getline(lines + line_count, &length, file) > 0) {
    line_count++;
    
    lines = realloc(lines, (line_count + 1) * sizeof(char *));
    lines[line_count] = NULL;
  }
  
  int prev_indent = 0;
  
  for (int i = 0; i < line_count; i++) {
    int curr_indent = 0;
    
    for (int j = 0; lines[i][j]; j++) {
      if (lines[i][j] != ' ') {
        break;
      }
      
      curr_indent++;
    }
    
    int is_empty = 1, is_open = 0, has_newline = 0;
    
    for (int j = 0; lines[i][j]; j++) {
      if (!isspace(lines[i][j])) {
        is_empty = 0;
      }
      
      is_open = !(!strchr("({[<", lines[i][j]));
      has_newline = (lines[i][j] == '\n');
    }
    
    if (is_open) {
      prev_indent += 2;
    }
    
    if (is_empty && !curr_indent) {
      free(lines[i]);
      
      char buffer[prev_indent + has_newline + 1];
      memset(buffer, ' ', prev_indent);
      
      buffer[prev_indent] = '\n';
      buffer[prev_indent + has_newline] = '\0';
      
      lines[i] = strdup(buffer);
    }
    
    prev_indent = curr_indent;
  }
  
  freopen(path, "w", file);
  
  for (int i = 0; i < line_count; i++) {
    fwrite(lines[i], 1, strlen(lines[i]), file);
  }
}

int main(int argc, const char **argv) {
  for (int i = 1; i < argc; i++) {
    FILE *file = fopen(argv[i], "r+");
    
    if (!file) {
      printf("? %s\n", argv[i]);
      continue;
    }
    
    code_format(argv[i], file);
    fclose(file);
    
    printf("OK %s\n", argv[i]);
  }
  
  return 0;
}
