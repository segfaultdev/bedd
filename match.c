#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <match.h>

int mt_match(const char *text, int length, const char *query, const char *replace_input, char *replace_output) {
  int offset = 0;
  char chr = '\0';
  
  char **matches = NULL;
  int match_count = 0;
  
#define return_free(value) ({while (match_count--) free(matches[match_count]); if (matches) free(matches); return value;})

  while ((chr = *(query++)) && offset < length) {
    if (chr == '[') {
      int is_format = 0;
      chr = *(query++);
      
      if (chr == '^') {
        is_format = 1;
        chr = *(query++);
      }
      
      if (*query == '/') {
        query++;
      } else {
        return_free(-1);
      }
      
      if (chr == 'l') {
        int done = 0;
        
        for (;;) {
          int query_length = 0;
          
          for (;;) {
            if (query[query_length] == ',' || query[query_length] == ']') {
              break;
            } else if (query[query_length] == '\\') {
              query_length++;
            }
            
            query_length++;
          }
          
          if (!done) {
            char new_query[query_length + 1];
            
            memcpy(new_query, query, query_length);
            new_query[query_length] = '\0';
            
            int match_length = mt_match(text + offset, length - offset, new_query, NULL, NULL);
            
            if (match_length >= 0) {
              offset += match_length;
              done = 1;
            }
          }
          
          query += query_length;
          
          if (*(query++) == ']') {
            break;
          }
        }
        
        if (!done) {
          return_free(-1);
        }
      } else if (chr == 'g') {
        char char_set[512] = {0};
        
        int min_count = 0;
        int max_count = INT_MAX;
        
        for (;;) {
          chr = *(query++);
          
          if (chr == '/' || chr == ']') {
            break;
          }
          
          if (chr == 'A' || chr == '*') {
            strcat(char_set, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
          }
          
          if (chr == 'a' || chr == '*') {
            strcat(char_set, "abcdefghijklmnopqrstuvwxyz");
          }
          
          if (chr == '0' || chr == '*') {
            strcat(char_set, "0123456789");
          }
          
          if (chr == '+' || chr == '*') {
            strcat(char_set, "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}");
          }
          
          if (chr == '_' || chr == '*') {
            strcat(char_set, " \t\r\n");
          }
          
          if (chr == '\\') {
            char temp[2] = {chr = *(query++), '\0'};
            strcat(char_set, temp);
          }
        }
        
        if (chr == '/') {
          min_count = 0;
          
          for (;;) {
            chr = *(query++);
            
            if (isdigit(chr)) {
              min_count = (min_count * 10) + (chr - '0');
            } else {
              break;
            }
          }
          
          if (chr == ',') {
            max_count = 0;
            
            for (;;) {
              chr = *(query++);
              
              if (isdigit(chr)) {
                max_count = (max_count * 10) + (chr - '0');
              } else {
                break;
              }
            }
          }
        }
        
        if (chr != ']') {
          return_free(-1);
        }
        
        int best_match = 0;
        
        for (int i = offset; i < length && i - offset <= max_count; i++) {
          if (!strchr(char_set, text[i])) {
            break;
          }
          
          if (mt_match(text + (i + 1), length - (i + 1), query, NULL, NULL) >= 0) {
            best_match = (i + 1) - offset;
          }
        }
        
        if (best_match < min_count) {
          return_free(-1);
        }
        
        if (is_format) {
          char *match_str = calloc(best_match + 1, 1);
          memcpy(match_str, text + offset, best_match);
          
          matches = realloc(matches, (match_count + 1) * sizeof(char *));
          matches[match_count++] = match_str;
        }
        
        offset += best_match;
      } else if (chr == 'r') {
        int index = 0;
        
        for (;;) {
          chr = *(query++);
          
          if (isdigit(chr)) {
            index = (index * 10) + (chr - '0');
          } else {
            break;
          }
        }
        
        const char *prev_match = matches[index];
        
        while (*prev_match) {
          if (text[offset] == *(prev_match++)) {
            offset++;
          } else {
            return_free(-1);
          }
        }
      }
      
      continue;
    } else if (chr == '\\') {
      chr = *(query++);
    }
    
    if (text[offset] == chr) {
      offset++;
    } else {
      return_free(-1);
    }
  }
  
  if (chr) {
    return_free(-1);
  }
  
  while (replace_input && *replace_input) {
    if (*replace_input == '\\') {
      *(replace_output++) = *(++replace_input);
    } else if (*replace_input == '[') {
      replace_input++;
      int index = 0;
      
      while (isdigit(*replace_input)) {
        index = (index * 10) + (*(replace_input++) - '0');
      }
      
      for (int i = 0; matches[index][i]; i++) {
        *(replace_output++) = matches[index][i];
      }
    } else {
      *(replace_output++) = *replace_input;
    }
    
    replace_input++;
  }
  
  if (replace_input) {
    *replace_output = '\0';
  }
  
  return_free(offset);
#undef return_free
}
