#include <string.h>
#include <syntax.h>
#include <ctype.h>

static int is_ident(char chr) {
  return isalnum(chr) || strchr("_", chr);
}

enum {
  st_rs_default,
  st_rs_ident,
  st_rs_number,
  st_rs_string,
  st_rs_char,
  st_rs_line_comment,
  st_rs_block_comment,
  st_rs_string_escape,
  st_rs_char_escape,
  st_rs_block_comment_exit,
};

int st_rs_color(int prev_state, int *state, const char *text, int length) {
  if (prev_state == st_rs_line_comment) {
    if (text[0] == '\n') {
      *state = st_rs_default;
      return st_color_default;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_rs_block_comment_exit) {
    *state = st_rs_default;
    return st_color_comment;
  }
  
  if (prev_state == st_rs_block_comment) {
    if (length >= 2 && text[0] == '*' && text[1] == '/') {
      *state = st_rs_block_comment_exit;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_rs_string_escape) {
    *state = st_rs_string;
    return st_color_string;
  }
  
  if (prev_state == st_rs_char_escape) {
    *state = st_rs_char;
    return st_color_string;
  }
  
  if (prev_state == st_rs_string) {
    if (text[0] == '\\') {
      *state = st_rs_string_escape;
    } else if (text[0] == '"') {
      *state = st_rs_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_rs_char) {
    if (text[0] == '\\') {
      *state = st_rs_char_escape;
    } else if (text[0] == '\'') {
      *state = st_rs_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_rs_line_comment) {
    if (text[0] == '\n') {
      *state = st_rs_default;
    }
    
    return st_color_comment;
  }
  
  if (length >= 2 && text[0] == '/') {
    if (text[1] == '/') {
      *state = st_rs_line_comment;
      return st_color_comment;
    } else if (text[1] == '*') {
      *state = st_rs_block_comment;
      return st_color_comment;
    }
  }
  
  if (strchr("+-*/%=&|^!?:.,;><\\~", text[0])) {
    *state = st_rs_default;
    return st_color_symbol;
  }
  
  if (isspace(text[0]) || strchr("()[]{}", text[0])) {
    *state = st_rs_default;
    return st_color_default;
  }
  
  if (text[0] == '"') {
    *state = st_rs_string;
    return st_color_string;
  }
  
  if (text[0] == '\'') {
    *state = st_rs_char;
    return st_color_string;
  }
  
  if (prev_state == st_rs_default) {
    if (isdigit(text[0])) {
      *state = st_rs_number;
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
      
      if (!isupper(text[0])) {
        is_type = 0;
      }
      
      char buffer[ident_length + 1];
      
      memcpy(buffer, text, ident_length);
      buffer[ident_length] = '\0';
      
      if (ident_length == 2 && strstr("as,fn,if,in", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 3 && strstr("for,let,mod,mut,pub,ref,use,dyn", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 4 && strstr("else,enum,impl,loop,move,self,Self,true,type", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 5 && strstr("break,const,crate,false,match,super,trait,where,while,async,await", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 6 && strstr("extern,return,static,struct,unsafe", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 8 && strstr("continue", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 2 && strstr("as,fn,if,in", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 3 && strstr("for,let,mod,mut,pub,ref,use,dyn", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 4 && strstr("else,enum,impl,loop,move,self,Self,true,type", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 5 && strstr("break,const,crate,false,match,super,trait,where,while,async,await", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 6 && strstr("extern,return,static,struct,unsafe", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 8 && strstr("continue", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 2 && strstr("u8,i8", buffer)) {
        is_type = 1;
      }
      
      if (ident_length == 3 && strstr("u16,i16,u32,i32,u64,i64,str", buffer)) {
        is_keyword = 1;
      }
      
      *state = st_rs_ident;
      
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
