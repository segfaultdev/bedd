#include <string.h>
#include <syntax.h>
#include <ctype.h>

static int is_ident(char chr) {
  return isalnum(chr) || strchr("_$", chr);
}

enum {
  st_js_default,
  st_js_ident,
  st_js_number,
  st_js_string,
  st_js_char,
  st_js_line_comment,
  st_js_block_comment,
  st_js_string_escape,
  st_js_char_escape,
  st_js_block_comment_exit,
};

int st_js_color(int prev_state, int *state, const char *text, int length) {
  if (prev_state == st_js_line_comment) {
    if (text[0] == '\n') {
      *state = st_js_default;
      return st_color_default;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_js_block_comment_exit) {
    *state = st_js_default;
    return st_color_comment;
  }
  
  if (prev_state == st_js_block_comment) {
    if (length >= 2 && text[0] == '*' && text[1] == '/') {
      *state = st_js_block_comment_exit;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_js_string_escape) {
    *state = st_js_string;
    return st_color_string;
  }
  
  if (prev_state == st_js_char_escape) {
    *state = st_js_char;
    return st_color_string;
  }
  
  if (prev_state == st_js_string) {
    if (text[0] == '\\') {
      *state = st_js_string_escape;
    } else if (text[0] == '"') {
      *state = st_js_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_js_char) {
    if (text[0] == '\\') {
      *state = st_js_char_escape;
    } else if (text[0] == '\'') {
      *state = st_js_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_js_line_comment) {
    if (text[0] == '\n') {
      *state = st_js_default;
    }
    
    return st_color_comment;
  }
  
  if (length >= 2 && text[0] == '/') {
    if (text[1] == '/') {
      *state = st_js_line_comment;
      return st_color_comment;
    } else if (text[1] == '*') {
      *state = st_js_block_comment;
      return st_color_comment;
    }
  }
  
  if (strchr("+-*/%=&|^!?:.,;><\\~", text[0])) {
    *state = st_js_default;
    return st_color_symbol;
  }
  
  if (isspace(text[0]) || strchr("()[]{}", text[0])) {
    *state = st_js_default;
    return st_color_default;
  }
  
  if (text[0] == '"') {
    *state = st_js_string;
    return st_color_string;
  }
  
  if (text[0] == '\'') {
    *state = st_js_char;
    return st_color_string;
  }
  
  if (prev_state == st_js_default) {
    if (isdigit(text[0])) {
      *state = st_js_number;
      return st_color_number;
    } else if (is_ident(text[0])) {
      int is_func = 1, is_type = 1, is_keyword = 0;
      int ident_length = 1;
      
      for (int i = 1; i < length; i++) {
        if (text[i] == '(') {
          break;
        }
        
        if (!is_ident(text[i])) {
          is_func = 0;
          break;
        }
        
        ident_length++;
      }
      
      if (ident_length < 2) {
        is_type = 0;
      } else if (text[ident_length - 1] != 't') {
        is_type = 0;
      } else if (text[ident_length - 2] != '_') {
        is_type = 0;
      }
      
      char buffer[ident_length + 1];
      
      memcpy(buffer, text, ident_length);
      buffer[ident_length] = '\0';
      
      if (ident_length == 2 && strstr("do,if,in,of", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 3 && strstr("for,let,new,try,var", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 4 && strstr("case,else,enum,null,this,true,void,with", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 5 && strstr("async,await,break,catch,class,const,false,super,throw,while,yield", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 6 && strstr("delete,export,import,public,return,static,switch,typeof", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 7 && strstr("default,extends,finally,package,private", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 8 && strstr("continue,debugger,function", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 9 && strstr("interface,protected", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 10 && strstr("implements,instanceof", buffer)) {
        is_keyword = 1;
      }
      
      *state = st_js_ident;
      
      if (is_type) {
        return st_color_type;
      } else if (is_keyword) {
        return st_color_keyword;
      } else if (is_func) {
        return st_color_function;
      } else {
        return st_color_default;
      }
    }
  }
  
  return st_color_none;
}
