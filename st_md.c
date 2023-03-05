#include <string.h>
#include <syntax.h>
#include <ctype.h>

enum {
  st_md_default,
  st_md_middle,
  st_md_title,
  st_md_code_inline,
  st_md_code_block,
  st_md_tag_first,
  st_md_tag_second,
  st_md_url,
};

int st_md_color(int prev_state, int *state, const char *text, int length) {
  if (prev_state == st_md_url) {
    if (!isalnum(text[0]) && !strchr(":/.-_%&=?@+", text[0])) {
      *state = st_md_middle;
      return st_color_default;
    }
    
    return st_color_function;
  }
  
  if (prev_state == st_md_code_block) {
    if (length >= 3 && text[0] == '`' && text[1] == '`' && text[2] == '`') {
      *state = st_md_middle;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_md_code_inline) {
    if (text[0] == '`') {
      *state = st_md_middle;
    }
    
    return st_color_comment;
  }
  
  if (text[0] == '\n') {
    *state = st_md_default;
    return st_color_default;
  }
  
  if (prev_state == st_md_tag_first) {
    if (text[0] == ']') {
      if (length >= 2 && text[1] == '(') {
        *state = st_md_tag_second;
      } else {
        *state = st_md_middle;
      }
      
      return st_color_default;
    }
    
    return st_color_comment;
  }
  
  if (prev_state == st_md_tag_second) {
    if (text[0] == '(') {
      return st_color_default;
    } else if (text[0] == ')') {
      *state = st_md_middle;
      return st_color_default;
    }
    
    return st_color_function;
  }
  
  if (prev_state == st_md_default) {
    if (text[0] == '#') {
      *state = st_md_title;
      return st_color_type;
    } else if (text[0] == '-' || text[0] == '*') {
      *state = st_md_middle;
      return st_color_symbol;
    } else if (!isspace(text[0])) {
      *state = st_md_middle;
    }
  }
  
  if (length >= 3 && text[0] == '`' && text[1] == '`' && text[2] == '`') {
    *state = st_md_code_block;
    return st_color_comment;
  }
  
  if (text[0] == '`') {
    *state = st_md_code_inline;
    return st_color_comment;
  }
  
  if (text[0] == '[') {
    *state = st_md_tag_first;
    return st_color_default;
  }
  
  if (prev_state == st_md_middle && isspace(text[0])) {
    return st_color_default;
  }
  
  if (isalpha(text[0])) {
    int start_length = 1;
    
    for (int i = 1; i < length; i++) {
      if (!isalpha(text[i])) {
        break;
      }
      
      start_length++;
    }
    
    if (length >= start_length + 3 && text[start_length + 0] == ':' && text[start_length + 1] == '/' && text[start_length + 2] == '/') {
      *state = st_md_url;
      return st_color_function;
    }
  }
  
  return st_color_none;
}
