#include <string.h>
#include <syntax.h>
#include <ctype.h>

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
    
    if (depth < 0) {
      depth = 0;
    }
  }
  
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
  
  if (in_string) {
    return '\0';
  }
  
  if (chr == '(') {
    return ')';
  }
  
  if (chr == '[') {
    return ']';
  }
  
  if (chr == '{') {
    return '}';
  }
  
  if (chr == '"') {
    return '"';
  }
  
  if (chr == '\'') {
    return '\'';
  }
  
  return '\0';
}

static int is_ident(char chr) {
  return isalnum(chr) || strchr("_$", chr);
}

enum {
  st_c_default,
  st_c_ident,
  st_c_number,
  st_c_string,
  st_c_char,
  st_c_line_comment,
  st_c_block_comment,
  st_c_string_escape,
  st_c_char_escape,
  st_c_block_comment_exit,
};

int st_c_color(int prev_state, int *state, const char *text, int length) {
  if (prev_state == st_c_line_comment) {
    if (text[0] == '\n') {
      *state = st_c_default;
      return st_color_default;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_c_block_comment_exit) {
    *state = st_c_default;
    return st_color_comment;
  }
  
  if (prev_state == st_c_block_comment) {
    if (length >= 2 && text[0] == '*' && text[1] == '/') {
      *state = st_c_block_comment_exit;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_c_string_escape) {
    *state = st_c_string;
    return st_color_string;
  }
  
  if (prev_state == st_c_char_escape) {
    *state = st_c_char;
    return st_color_string;
  }
  
  if (prev_state == st_c_string) {
    if (text[0] == '\\') {
      *state = st_c_string_escape;
    } else if (text[0] == '"') {
      *state = st_c_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_c_char) {
    if (text[0] == '\\') {
      *state = st_c_char_escape;
    } else if (text[0] == '\'') {
      *state = st_c_default;
    }
    
    return st_color_string;
  }
  
  if (prev_state == st_c_line_comment) {
    if (text[0] == '\n') {
      *state = st_c_default;
    }
    
    return st_color_comment;
  }
  
  if (length >= 2 && text[0] == '/') {
    if (text[1] == '/') {
      *state = st_c_line_comment;
      return st_color_comment;
    } else if (text[1] == '*') {
      *state = st_c_block_comment;
      return st_color_comment;
    }
  }
  
  if (strchr("+-*/%=&|^!?:.,;><\\~", text[0])) {
    *state = st_c_default;
    return st_color_symbol;
  }
  
  if (isspace(text[0]) || strchr("()[]{}\n", text[0])) {
    *state = st_c_default;
    return st_color_default;
  }
  
  if (text[0] == '"') {
    *state = st_c_string;
    return st_color_string;
  }
  
  if (text[0] == '\'') {
    *state = st_c_char;
    return st_color_string;
  }
  
  if (text[0] == '#') {
    *state = st_c_ident;
    return st_color_keyword;
  }
  
  if (prev_state == st_c_default) {
    if (isdigit(text[0])) {
      *state = st_c_number;
      return st_color_number;
    } else if (is_ident(text[0])) {
      int is_func = 0, is_type = 1, is_keyword = 0;
      int ident_length = 1;
      
      for (int i = 1; i < length; i++) {
        if (text[i] == '(') {
          is_func = 1;
        }
        
        if (!is_ident(text[i])) {
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
      
      if (ident_length == 2 && strstr("do,if", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 3 && strstr("for,int,new", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 4 && strstr("auto,bool,case,char,else,enum,goto,long,true,void", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 5 && strstr("break,class,const,false,float,short,union,using,while", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 6 && strstr("delete,double,extern,inline,public,return,signed,sizeof,static,struct,switch", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 7 && strstr("default,private,typedef", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 8 && strstr("continue,register,unsigned,volatile", buffer)) {
        is_keyword = 1;
      }
      
      if (ident_length == 9 && strstr("constexpr,namespace,protected", buffer)) {
        is_keyword = 1;
      }
      
      *state = st_c_ident;
      
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
