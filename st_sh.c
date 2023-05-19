#include <string.h>
#include <syntax.h>
#include <ctype.h>

static int is_ident(char chr) {
  return isalnum(chr) || strchr("_$", chr);
}

enum {
  st_sh_default,
  st_sh_ident,
  st_sh_number,
  st_sh_string,
  st_sh_char,
  st_sh_line_comment,
  st_sh_string_escape,
  st_sh_char_escape,
};

int st_sh_color(int prev_state, int *state, const char *text, int length) {
  if (prev_state == st_sh_line_comment) {
    if (text[0] == '\n') {
      *state = st_sh_default;
      return st_color_default;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_sh_string_escape) {
    *state = st_sh_string;
    return st_color_string;
  }
  
  if (prev_state == st_sh_char_escape) {
    *state = st_sh_char;
    return st_color_string;
  }
  
  if (prev_state == st_sh_string) {
    if (text[0] == '\\') {
      *state = st_sh_string_escape;
    } else if (text[0] == '"') {
      *state = st_sh_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_sh_char) {
    if (text[0] == '\\') {
      *state = st_sh_char_escape;
    } else if (text[0] == '\'') {
      *state = st_sh_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_sh_line_comment) {
    if (text[0] == '\n') {
      *state = st_sh_default;
    }
    
    return st_color_comment;
  }
  
  if (text[0] == '#') {
    *state = st_sh_line_comment;
    return st_color_comment;
  }
  
  if (strchr("+-*/%=&|^!?:.,;><\\~", text[0])) {
    *state = st_sh_default;
    return st_color_symbol;
  }
  
  if (isspace(text[0]) || strchr("()[]{}\n", text[0])) {
    *state = st_sh_default;
    return st_color_default;
  }
  
  if (text[0] == '"') {
    *state = st_sh_string;
    return st_color_string;
  }
  
  if (text[0] == '\'') {
    *state = st_sh_char;
    return st_color_string;
  }
  
  if (text[0] == '#') {
    *state = st_sh_ident;
    return st_color_keyword;
  }
  
  if (prev_state == st_sh_default) {
    if (isdigit(text[0])) {
      *state = st_sh_number;
      return st_color_number;
    } else if (is_ident(text[0])) {
      int is_keyword = 0;
      int ident_length = 1;
      
      for (int i = 1; i < length; i++) {
        if (!is_ident(text[i])) {
          break;
        }
        
        ident_length++;
      }
      
      char buffer[ident_length + 1];
      
      memcpy(buffer, text, ident_length);
      buffer[ident_length] = '\0';
      
      if (ident_length == 2 && strstr("do,fi,if,in", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 3 && strstr("set,for", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 4 && strstr("case,done,echo,esac,eval,exec,exit,read,trap,wait", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 5 && strstr("break,shift,umask,unset,while", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 6 && strstr("export,return,ulimit", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 8 && strstr("continue,readonly", buffer)) {
        is_keyword = 1;
      }
      
      *state = st_sh_ident;
      
      if (is_keyword) {
        return st_color_keyword;
      } else {
        return st_color_default;
      }
    }
  }
  
  return st_color_none;
}
