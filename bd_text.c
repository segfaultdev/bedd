#include <stdlib.h>
#include <string.h>
#include <syntax.h>
#include <ctype.h>
#include <match.h>
#include <bedd.h>
#include <io.h>

typedef struct bd_line_t bd_line_t;
typedef struct bd_text_t bd_text_t;

struct bd_line_t {
  unsigned char *data;
  int size, length;
  
  int syntax_state; // State *after* including the newline
};

struct bd_text_t {
  char path[256];
  
  bd_line_t *lines;
  int count;
  
  bd_cursor_t cursor, hold_cursor, scroll;
  int edit_count, dirty;
  
  int scroll_mouse_y;
  
  syntax_t syntax;
  
  bd_text_t *prev, *next;
};

// static functions

static int __bd_text_utf_8_size(unsigned char value);
static int __bd_text_utf_8_to_byte(bd_text_t *text, int x, int y);
static int __bd_text_utf_8_from_byte(bd_text_t *text, int byte_index, int y);

static bd_text_t __bd_text_clone(bd_text_t *text);
static void      __bd_text_free(bd_text_t *text, int recursive_prev, int recursive_next);

static void __bd_text_undo_save(bd_text_t *text);
static void __bd_text_undo(bd_text_t *text);
static void __bd_text_redo(bd_text_t *text);

static int   __bd_text_cursor(bd_text_t *text);
static void  __bd_text_backspace(bd_text_t *text, int fix_cursor);
static void  __bd_text_write(bd_text_t *text, unsigned int chr, int by_user);
static void  __bd_text_up(bd_text_t *text, int hold);
static void  __bd_text_down(bd_text_t *text, int hold);
static void  __bd_text_right(bd_text_t *text, int hold);
static void  __bd_text_left(bd_text_t *text, int hold);
static void  __bd_text_home(bd_text_t *text, int hold);
static void  __bd_text_end(bd_text_t *text, int hold);
static void  __bd_text_full_home(bd_text_t *text, int hold);
static void  __bd_text_full_end(bd_text_t *text, int hold);
static void  __bd_text_follow(bd_text_t *text);
static char *__bd_text_output(bd_text_t *text, int to_file, io_file_t file, bd_cursor_t start, bd_cursor_t end);
static void  __bd_text_syntax(bd_text_t *text, int start_line);
static int   __bd_text_spaces(bd_text_t *text, int y);

static int __bd_text_utf_8_size(unsigned char value) {
  if (value >= 0xF0) {
    return 4;
  } else if (value >= 0xE0) {
    return 3;
  } else if (value >= 0xC0) {
    return 2;
  }
  
  return 1;
}

static int __bd_text_utf_8_to_byte(bd_text_t *text, int x, int y) {
  bd_line_t *line = text->lines + y;
  
  for (int i = 0; i < line->size;) {
    if (!x) {
      return i;
    }
    
    i += __bd_text_utf_8_size(line->data[i]);
    x--;
  }
  
  return line->size;
}

static int __bd_text_utf_8_from_byte(bd_text_t *text, int byte_index, int y) {
  bd_line_t *line = text->lines + y;
  int temp_index = 0;
  
  for (int i = 0; i < line->length; i++) {
    if (temp_index >= byte_index) {
      return i;
    }
    
    temp_index += __bd_text_utf_8_size(line->data[temp_index]);
  }
  
  return line->length;
}

static bd_text_t __bd_text_clone(bd_text_t *text) {
  bd_text_t text_clone = *text;
  
  text_clone.lines = malloc(text_clone.count * sizeof(bd_line_t));
  memcpy(text_clone.lines, text->lines, text_clone.count * sizeof(bd_line_t));
  
  for (int i = 0; i < text_clone.count; i++) {
    if (text_clone.lines[i].size) {
      text_clone.lines[i].data = malloc(text_clone.lines[i].size);
      memcpy(text_clone.lines[i].data, text->lines[i].data, text_clone.lines[i].size);
    } else {
      text_clone.lines[i].data = NULL;
    }
  }
  
  return text_clone;
}

static void __bd_text_free(bd_text_t *text, int recursive_prev, int recursive_next) {
  for (int i = 0; i < text->count; i++) {
    free(text->lines[i].data);
  }
  
  free(text->lines);
  
  if (text->prev && recursive_prev) {
    __bd_text_free(text->prev, 1, 0);
    free(text->prev);
  }
  
  if (text->next && recursive_next) {
    __bd_text_free(text->next, 0, 1);
    free(text->next);
  }
}

static void __bd_text_undo_save(bd_text_t *text) {
  if (!text->edit_count) {
    return;
  }
  
  text->edit_count = 0;
  
  if (text->next) {
    __bd_text_free(text->next, 0, 1);
    free(text->next);
    
    text->next = NULL;
  }
  
  bd_text_t *temp = text;
  
  for (int i = 0; i < bd_config.undo_depth; i++) {
    if (!temp) {
      break;
    }
    
    if (temp->prev && i == bd_config.undo_depth - 1) {
      __bd_text_free(temp->prev, 1, 0);
      free(temp->prev);
      
      temp->prev = NULL;
    }
    
    temp = temp->prev;
  }
  
  bd_text_t text_clone = __bd_text_clone(text);
  
  text->prev = malloc(sizeof(bd_text_t));
  *(text->prev) = text_clone;
}

static void __bd_text_undo(bd_text_t *text) {
  if (!text->prev) {
    return;
  }
  
  bd_text_t *text_clone = malloc(sizeof(bd_text_t));
  memcpy(text_clone, text, sizeof(bd_text_t));
  
  memcpy(text, text->prev, sizeof(bd_text_t));
  text->next = text_clone;
}

static void __bd_text_redo(bd_text_t *text) {
  if (!text->next) {
    return;
  }
  
  bd_text_t *next = text->next;
  memcpy(text, next, sizeof(bd_text_t));
  
  free(next);
}

static int __bd_text_cursor(bd_text_t *text) {
  // cursor.x must not exceed the length of the line, but *can* be equal to it
  
  if (text->cursor.x < 0) {
    text->cursor.x = 0;
  }
  
  if (text->cursor.x > text->lines[text->cursor.y].length) {
    text->cursor.x = text->lines[text->cursor.y].length;
  }
  
  // Same logic applies to hold_cursor.x
  
  if (text->hold_cursor.x < 0) {
    text->hold_cursor.x = 0;
  }
  
  if (text->hold_cursor.x > text->lines[text->hold_cursor.y].length) {
    text->hold_cursor.x = text->lines[text->hold_cursor.y].length;
  }
  
  // Then, cursor must be equal to hold_cursor, so delete chars in between!
  
  if (text->cursor.x == text->hold_cursor.x && text->cursor.y == text->hold_cursor.y) {
    return 0;
  }
  
  // Make sure hold_cursor is behind and not after cursor, to make the following code simpler
  
  if (BD_CURSOR_GREAT(text->hold_cursor, text->cursor)) {
    bd_cursor_t temp = text->cursor;
    
    text->cursor = text->hold_cursor;
    text->hold_cursor = temp;
  }
  
  while (BD_CURSOR_GREAT(text->cursor, text->hold_cursor)) {
    __bd_text_backspace(text, 0);
  }
  
  __bd_text_follow(text);
  return 1;
}

static void __bd_text_backspace(bd_text_t *text, int fix_cursor) {
  if (fix_cursor) {
    if (__bd_text_cursor(text)) {
      return;
    }
  }
  
  if (text->cursor.x) {
    bd_line_t *line = text->lines + text->cursor.y;
    int byte_index = __bd_text_utf_8_to_byte(text, text->cursor.x - 1, text->cursor.y);
    
    unsigned char value = line->data[byte_index];
    int utf_8_size = __bd_text_utf_8_size(value);
    
    memmove(line->data + byte_index, line->data + byte_index + utf_8_size, line->size - (byte_index + utf_8_size));
    text->cursor.x--;
    
    line->size -= utf_8_size;
    line->length--;
    
    __bd_text_syntax(text, text->cursor.y);
    __bd_text_follow(text);
  } else {
    if (!text->cursor.y) {
      __bd_text_follow(text);
      return;
    }
    
    bd_line_t *prev_line = text->lines + (text->cursor.y - 1);
    bd_line_t *line = text->lines + text->cursor.y;
    
    prev_line->data = realloc(prev_line->data, prev_line->size + line->size);
    
    if (line->size) {
      memcpy(prev_line->data + prev_line->size, line->data, line->size);
    }
    
    text->cursor.x = prev_line->length;
    
    prev_line->length += line->length;
    prev_line->size += line->size;
    
    free(line->data);
    
    for (int i = text->cursor.y; i < text->count - 1; i++) {
      text->lines[i] = text->lines[i + 1];
    }
    
    text->lines = realloc(text->lines, (text->count - 1) * sizeof(bd_line_t));
    
    text->cursor.y--;
    text->count--;
  }
  
  if (fix_cursor) {
    text->hold_cursor = text->cursor;
  }
  
  text->edit_count++, text->dirty = 1;
  
  __bd_text_syntax(text, text->cursor.y);
  __bd_text_follow(text);
}

static void __bd_text_write(bd_text_t *text, unsigned int chr, int by_user) {
  __bd_text_cursor(text);
  text->edit_count++, text->dirty = 1;
  
  if (chr == '\t') {
    do {
      __bd_text_write(text, ' ', by_user);
    } while (text->cursor.x % bd_config.indent_width);
    
    return;
  } else if (chr == '\n') {
    text->lines = realloc(text->lines, (text->count + 1) * sizeof(bd_line_t));
    
    if (text->cursor.y < text->count - 1) {
      memmove(text->lines + text->cursor.y + 1, text->lines + text->cursor.y, (text->count - text->cursor.y) * sizeof(bd_line_t));
    }
    
    text->count++;
    
    int space_count = 0;
    bd_line_t line;
    
    while (by_user && space_count < text->lines[text->cursor.y].length && text->lines[text->cursor.y].data[space_count] == ' ') {
      space_count++;
    }
    
    int old_count = space_count;
    unsigned char last = '\0', pair = '\0';
    
    int byte_index = __bd_text_utf_8_to_byte(text, text->cursor.x, text->cursor.y);
    
    if (by_user) {
      space_count += !!text->syntax.f_depth((const char *)(text->lines[text->cursor.y].data), byte_index) * bd_config.indent_width;
      
      if (text->cursor.x) {
        int prev_index = __bd_text_utf_8_to_byte(text, text->cursor.x - 1, text->cursor.y);
        
        last = text->lines[text->cursor.y].data[prev_index];
        pair = text->syntax.f_pair((const char *)(text->lines[text->cursor.y].data), prev_index, last);
      }
    }
    
    line.length = space_count + (text->lines[text->cursor.y].length - text->cursor.x),
    line.size = space_count + (text->lines[text->cursor.y].size - byte_index);
    
    line.data = line.size ? malloc(line.size) : NULL;
    
    if (by_user && space_count) {
      memset(line.data, ' ', space_count);
    }
    
    if (line.size - space_count) {
      memcpy(line.data + space_count, text->lines[text->cursor.y].data + byte_index, line.size - space_count);
    }
    
    text->lines[text->cursor.y].length = text->cursor.x;
    text->lines[text->cursor.y].size = byte_index;
    
    text->lines[text->cursor.y].data = realloc(text->lines[text->cursor.y].data, byte_index);
    text->lines[text->cursor.y + 1] = line;
    
    text->cursor.x = space_count;
    text->cursor.y++;
    
    text->hold_cursor = text->cursor;
    
    if (pair) {
      int byte_index = __bd_text_utf_8_to_byte(text, text->cursor.x, text->cursor.y);
      
      if (text->cursor.x < text->lines[text->cursor.y].length && text->lines[text->cursor.y].data[byte_index] == pair) {
        bd_cursor_t old_cursor = text->cursor;
        __bd_text_write(text, '\n', 0);
        
        while (old_count--) {
          __bd_text_write(text, ' ', 0);
        }
        
        text->cursor = old_cursor;
      }
    }
    
    text->hold_cursor = text->cursor;
    __bd_text_follow(text);
    
    __bd_text_syntax(text, text->cursor.y);
    return;
  }
  
  int byte_index = __bd_text_utf_8_to_byte(text, text->cursor.x, text->cursor.y);
  int utf_8_size = __bd_text_utf_8_size(chr & 0xFF);
  
  bd_line_t *line = text->lines + text->cursor.y;
  line->data = realloc(line->data, line->size + utf_8_size);
  
  if (line->length - text->cursor.x) {
    memmove(line->data + byte_index + utf_8_size, line->data + byte_index, line->size - byte_index);
  }
  
  memcpy(line->data + byte_index, &chr, utf_8_size);
  text->cursor.x++;
  
  line->size += utf_8_size;
  line->length++;
  
  text->hold_cursor = text->cursor;
  __bd_text_follow(text);
  
  if (!by_user) {
    return;
  }
  
  char pair = text->syntax.f_pair((const char *)(line->data), byte_index, (char)(chr));
  
  if (pair) {
    __bd_text_write(text, pair, 0);
    __bd_text_left(text, 0);
  }
  
  __bd_text_syntax(text, text->cursor.y);
}

static void __bd_text_up(bd_text_t *text, int hold) {
  text->cursor.y--;
  
  if (text->cursor.y < 0) {
    text->cursor.x = 0;
    text->cursor.y = 0;
  }
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_down(bd_text_t *text, int hold) {
  text->cursor.y++;
  
  if (text->cursor.y >= text->count) {
    text->cursor.y = text->count - 1;
    text->cursor.x = text->lines[text->cursor.y].length;
  }
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_right(bd_text_t *text, int hold) {
  text->cursor.x++;
  
  if (text->cursor.x > text->lines[text->cursor.y].length) {
    text->cursor.y++;
    
    if (text->cursor.y >= text->count) {
      text->cursor.y = text->count - 1;
      text->cursor.x = text->lines[text->cursor.y].length;
    } else {
      text->cursor.x = 0;
    }
  }
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_left(bd_text_t *text, int hold) {
  if (text->cursor.x > text->lines[text->cursor.y].length) {
    text->cursor.x = text->lines[text->cursor.y].length;
  }
  
  text->cursor.x--;
  
  if (text->cursor.x < 0) {
    text->cursor.y--;
    
    if (text->cursor.y < 0) {
      text->cursor.x = 0;
      text->cursor.y = 0;
    } else {
      text->cursor.x = text->lines[text->cursor.y].length;
    }
  }
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_home(bd_text_t *text, int hold) {
  text->cursor.x = 0;
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_end(bd_text_t *text, int hold) {
  text->cursor.x = text->lines[text->cursor.y].length;
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_full_home(bd_text_t *text, int hold) {
  text->cursor.x = 0;
  text->cursor.y = 0;
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_full_end(bd_text_t *text, int hold) {
  text->cursor.y = text->count - 1;
  text->cursor.x = text->lines[text->cursor.y].length;
  
  if (!hold) {
    text->hold_cursor = text->cursor;
  }
  
  __bd_text_follow(text);
}

static void __bd_text_follow(bd_text_t *text) {
  int view_height = bd_height - 2;
  
  if (text->scroll.y > text->cursor.y) {
    text->scroll.y = text->cursor.y;
  } else if (text->scroll.y <= text->cursor.y - view_height) {
    text->scroll.y = (text->cursor.y - view_height) + 1;
  }
  
  int lind_size = 1, lind_max = 10;
  
  while (text->count >= lind_max) {
    lind_size++;
    lind_max *= 10;
  }
  
  text->scroll.x = text->cursor.x - (bd_width - 8 - lind_size - bd_config.scroll_width_margin);
  
  if (text->scroll.x < 0) {
    text->scroll.x = 0;
  }
}

static char *__bd_text_output(bd_text_t *text, int to_file, io_file_t file, bd_cursor_t start, bd_cursor_t end) {
  char *buffer = NULL;
  int buffer_length = 0;
  
  if (start.x > text->lines[start.y].length) {
    start.x = text->lines[start.y].length;
  }
  
  if (end.x > text->lines[end.y].length) {
    end.x = text->lines[end.y].length;
  }
  
  int space_count = __bd_text_spaces(text, start.y);
  
  while (start.y < end.y || (start.y == end.y && start.x < end.x)) {
    unsigned char utf_8_buffer[4];
    int utf_8_size;
    
    if (start.x >= text->lines[start.y].length) {
      utf_8_buffer[0] = '\n';
      utf_8_size = 1;
      
      space_count = __bd_text_spaces(text, ++start.y);
      start.x = 0;
    } else if (!bd_config.indent_spaces && start.x < space_count && !(start.x % bd_config.indent_width)) {
      utf_8_buffer[0] = '\t';
      utf_8_size = 1;
      
      start.x += bd_config.indent_width;
    } else {
      int byte_index = __bd_text_utf_8_to_byte(text, start.x, start.y);
      
      utf_8_buffer[0] = text->lines[start.y].data[byte_index];
      utf_8_size = __bd_text_utf_8_size(utf_8_buffer[0]);
      
      memcpy(utf_8_buffer + 1, text->lines[start.y].data + byte_index + 1, utf_8_size - 1);
      start.x++;
    }
    
    if (to_file) {
      io_fwrite(file, utf_8_buffer, utf_8_size);
    } else {
      buffer = realloc(buffer, buffer_length + utf_8_size);
      
      memcpy(buffer + buffer_length, utf_8_buffer, utf_8_size);
      buffer_length += utf_8_size;
    }
  }
  
  if (!to_file) {
    buffer = realloc(buffer, buffer_length + 1);
    buffer[buffer_length] = '\0';
  }
  
  return buffer;
}

static void __bd_text_syntax(bd_text_t *text, int start_line) {
  int state = start_line ? text->lines[start_line - 1].syntax_state : 0;
  
  for (int i = 0; i < text->lines[start_line].size; i++) {
    text->syntax.f_color(state, &state, (const char *)(text->lines[start_line].data + i), text->lines[start_line].size - i);
  }
  
  text->syntax.f_color(state, &state, "\n", 1);
  
  if (state == text->lines[start_line].syntax_state) {
    return;
  }
  
  text->lines[start_line++].syntax_state = state;
  
  if (start_line >= text->count) {
    return;
  }
  
  __bd_text_syntax(text, start_line);
}

static int __bd_text_spaces(bd_text_t *text, int y) {
  bd_line_t *line = text->lines + y;
  int space_count = 0;
  
  while (space_count < line->size && line->data[space_count] == ' ') {
    space_count++;
  }
  
  return space_count;
}

// public functions

void bd_text_draw(bd_view_t *view) {
  bd_text_t *text = view->data;
  int lind_size = 1, lind_max = 10, lind_left = 2, lind_right = 2, lind_post = 1;
  
  while (text->count >= lind_max) {
    lind_size++;
    lind_max *= 10;
  }
  
  if (bd_config.column_tiny) {
    lind_size = 4, lind_left = 1, lind_right = 1, lind_post = 0;
  }
  
  int view_width = bd_width - (lind_size + lind_left + lind_right + lind_post + 2);
  int view_height = bd_height - 2;
  
  bd_cursor_t min_cursor = BD_CURSOR_MIN(text->cursor, text->hold_cursor);
  bd_cursor_t max_cursor = BD_CURSOR_MAX(text->cursor, text->hold_cursor);
  
  int high_count = __bd_text_spaces(text, text->cursor.y);
  int high_tabs = high_count / bd_config.indent_width;
  
  int high_starts[high_tabs], high_ends[high_tabs];
  
  int high_start_y = text->cursor.y;
  int high_end_y = text->cursor.y + 1;
  
  for (int i = high_count / bd_config.indent_width; i > 0; i--) {
    while (high_start_y > 0) {
      int space_count = __bd_text_spaces(text, high_start_y - 1);
      
      if (space_count >= i * bd_config.indent_width) {
        high_start_y--;
      } else {
        break;
      }
    }
    
    while (high_end_y < text->count) {
      int space_count = __bd_text_spaces(text, high_end_y);
      
      if (space_count >= i * bd_config.indent_width) {
        high_end_y++;
      } else {
        break;
      }
    }
    
    high_starts[i - 1] = high_start_y;
    high_ends[i - 1] = high_end_y;
  }
  
  for (int i = 0; i < view_height; i++) {
    io_cursor(0, 2 + i);
    
    int y = i + text->scroll.y;
    int is_selected = 0;
    
    int guide_done = 0;
    
    if (y < text->count) {
      io_printf("%s%*s%*d%*s%c" IO_NORMAL "%*s", (y == text->cursor.y ? IO_INVERT : IO_SHADOW_1), lind_left, "", lind_size, y + 1, lind_right - 1, "", text->scroll.x ? '<' : ' ', lind_post, "");
      
      bd_line_t *line = text->lines + y;
      int space_count = __bd_text_spaces(text, y);
      
      int state = y ? text->lines[y - 1].syntax_state : 0;
      int last_color = st_color_none;
      
      for (int j = 0; j < view_width; j++) {
        int x = j + text->scroll.x;
        
        bd_cursor_t cursor = (bd_cursor_t) {
          x, y
        };
        
        if (!BD_CURSOR_LESS(cursor, min_cursor) && BD_CURSOR_LESS(cursor, max_cursor)) {
          if (!is_selected) {
            io_printf(IO_SHADOW_1);
            is_selected = 1;
          }
        } else {
          if (is_selected) {
            io_printf(IO_NO_SHADOW);
            is_selected = 0;
          }
        }
        
        if (x <= line->length && bd_config.column_guide > 0 && x == bd_config.column_guide) {
          guide_done = 1;
        }
        
        if (x == line->length) {
          if (bd_config.column_guide > 0 && x == bd_config.column_guide) {
            if (is_selected) {
              io_printf(IO_DARK_GRAY "\u2551");
            } else {
              io_printf(IO_BLACK "\u2551");
            }
          }
          
          text->syntax.f_color(state, &state, "\n", 1);
          
          if (!bd_config.column_guide || x != bd_config.column_guide) {
            io_printf(" ");
          }
        } else if (x > line->length) {
          break;
        } else {
          if (x < space_count) {
            text->syntax.f_color(state, &state, " ", 1);
            
            int high_index = x / bd_config.indent_width;
            
            if (bd_config.indent_guides && !(x % bd_config.indent_width) && x < high_count && high_index < high_tabs &&
                (!(space_count % bd_config.indent_width) || high_index < space_count / bd_config.indent_width - 2) &&
                y >= high_starts[high_index] && y < high_ends[high_index]) {
              io_printf("%s\u2502" IO_WHITE, (is_selected || y == text->cursor.y) ? IO_DARK_GRAY : IO_BLACK);
            } else {
              io_printf("%s\u00B7" IO_WHITE, (is_selected || y == text->cursor.y) ? IO_DARK_GRAY : IO_BLACK);
            }
          } else {
            int byte_index = __bd_text_utf_8_to_byte(text, x, y);
            int utf_8_size = __bd_text_utf_8_size(line->data[byte_index]);
            
            int color = text->syntax.f_color(state, &state, (const char *)(line->data + byte_index), line->size - byte_index);
            
            if (color == st_color_none) {
              color = last_color;
            }
            
            int temp_color = color;
            
            if (color == last_color) {
              temp_color = st_color_none;
            }
            
            unsigned char utf_8_buffer[5];
            
            memcpy(utf_8_buffer, line->data + byte_index, utf_8_size);
            utf_8_buffer[utf_8_size] = '\0';
            
            if (bd_config.column_guide > 0 && x >= bd_config.column_guide) {
              io_printf(is_selected ? IO_DARK_GRAY : IO_BLACK);
            } else {
              io_printf(bd_config.syntax_colors[temp_color]);
            }
            
            if (utf_8_buffer[0] == ' ' && (bd_config.column_guide > 0 && x == bd_config.column_guide)) {
              io_printf("\u2551");
            } else {
              io_printf("%s", utf_8_buffer);
            }
            
            last_color = color;
          }
        }
        
        if (bd_config.column_guide > 0 && x == bd_config.column_guide) {
          if (is_selected) {
            io_printf(IO_SHADOW_1);
          } else {
            io_printf(IO_NO_SHADOW);
          }
        }
      }
      
      io_printf(IO_NORMAL IO_CLEAR_LINE);
      
      if (!guide_done && bd_config.column_guide > 0) {
        io_cursor(lind_size + lind_left + lind_right + lind_post + bd_config.column_guide, 2 + i);
        io_printf(IO_BLACK "\u2551" IO_NORMAL);
      }
      
      io_cursor(bd_width - 2, 2 + i);
      io_printf(IO_SHADOW_1);
      
      for (int j = view_width + text->scroll.x; j < line->length; j++) {
        text->syntax.f_color(state, &state, (const char *)(line->data + j), line->length - j);
      }
      
      if (text->scroll.x + view_width < line->length) {
        text->syntax.f_color(state, &state, "\n", 1);
      }
      
      io_printf((text->scroll.x + view_width < line->length) ? ">" : " ");
      line->syntax_state = state;
    } else {
      io_printf("%*s:%*s", (lind_size + lind_left + lind_right) - 1, "", lind_post, "");
      io_printf(IO_NORMAL IO_CLEAR_LINE);
      
      if (!guide_done && bd_config.column_guide > 0) {
        io_cursor(lind_size + lind_left + lind_right + lind_post + bd_config.column_guide, 2 + i);
        io_printf(IO_BLACK "\u2551" IO_NORMAL);
      }
      
      io_cursor(bd_width - 2, 2 + i);
      io_printf(IO_SHADOW_1 ":");
    }
    
    int scroll_start_y = (text->scroll.y * (bd_height - 2)) / (text->count + (bd_height - 2));
    int scroll_end_y = 1 + ((text->scroll.y + (bd_height - 2)) * (bd_height - 2)) / (text->count + (bd_height - 2));
    
    if (i >= scroll_start_y && i < scroll_end_y) {
      io_printf(IO_SHADOW_2 " ");
    } else {
      io_printf(IO_NORMAL "\u2502");
    }
    
    io_printf(IO_NORMAL);
  }
  
  int temp_x = text->cursor.x > text->lines[text->cursor.y].length ? text->lines[text->cursor.y].length : text->cursor.x;
  
  int cursor_x = (temp_x - text->scroll.x) + (lind_size + lind_left + lind_right + lind_post);
  int cursor_y = (text->cursor.y - text->scroll.y) + 2;
  
  if (cursor_x < lind_size + 5 || cursor_x >= bd_width - 2) {
    cursor_x = -1;
  }
  
  if (cursor_y < 2 || cursor_y >= bd_height) {
    cursor_y = -1;
  }
  
  char new_title[257];
  sprintf(new_title, "%s%s", text->path[0] ? text->path : "(no name)", text->dirty ? "*" : "");
  
  view->title_dirty = strcmp(view->title, new_title);
  strcpy(view->title, new_title);
  
  view->cursor = (bd_cursor_t) {
    cursor_x, cursor_y,
  };
}

int bd_text_event(bd_view_t *view, io_event_t event) {
  bd_text_t *text = view->data;
  int lind_size = 1, lind_max = 10, lind_left = 2, lind_right = 2, lind_post = 1;
  
  while (text->count >= lind_max) {
    lind_size++;
    lind_max *= 10;
  }
  
  if (bd_config.column_tiny) {
    lind_size = 4, lind_left = 1, lind_right = 1, lind_post = 0;
  }
  
  if (event.type == IO_EVENT_KEY_PRESS) {
    if (IO_UNSHIFT(event.key) == '\t' && memcmp(&(text->cursor), &(text->hold_cursor), sizeof(bd_cursor_t))) {
      __bd_text_undo_save(text);
      
      bd_cursor_t min_cursor = BD_CURSOR_MIN(text->cursor, text->hold_cursor);
      bd_cursor_t max_cursor = BD_CURSOR_MAX(text->cursor, text->hold_cursor);
      
      bd_cursor_t old_cursor = text->cursor;
      bd_cursor_t old_hold_cursor = text->hold_cursor;
      
      for (int i = min_cursor.y; i <= max_cursor.y; i++) {
        if (event.key == IO_UNSHIFT(event.key)) {
          text->cursor = text->hold_cursor = (bd_cursor_t) {
            0, i
          };
          
          __bd_text_write(text, '\t', 1);
        } else {
          bd_line_t *line = text->lines + i;
          int space_count = 0;
          
          while (space_count < line->size && line->data[space_count] == ' ' && space_count < bd_config.indent_width) {
            space_count++;
          }
          
          text->cursor = text->hold_cursor = (bd_cursor_t) {
            space_count, i
          };
          
          while (space_count--) {
            __bd_text_backspace(text, 1);
          }
        }
      }
      
      text->cursor = old_cursor;
      text->hold_cursor = old_hold_cursor;
      
      if (event.key == IO_UNSHIFT(event.key)) {
        text->cursor.x += bd_config.indent_width;
        text->hold_cursor.x += bd_config.indent_width;
      } else {
        text->cursor.x -= bd_config.indent_width;
        
        if (text->cursor.x < 0) {
          text->cursor.x = 0;
        }
        
        text->hold_cursor.x -= bd_config.indent_width;
        
        if (text->hold_cursor.x < 0) {
          text->hold_cursor.x = 0;
        }
      }
      
      __bd_text_undo_save(text);
      return 1;
    } else if ((event.key >= 32 && event.key < 127) || event.key == '\t' || event.key & 0x80000000) {
      __bd_text_write(text, event.key & 0x7FFFFFFF, 1);
      
      if (text->edit_count >= bd_config.undo_edit_count) {
        __bd_text_undo_save(text);
      }
      
      return 1;
    } else if (event.key == IO_CTRL('M')) {
      __bd_text_write(text, '\n', 1);
      
      if (text->edit_count >= bd_config.undo_edit_count) {
        __bd_text_undo_save(text);
      }
      
      return 1;
    } else if (event.key == IO_CTRL('H')) {
      __bd_text_backspace(text, 1);
      
      if (text->edit_count >= bd_config.undo_edit_count) {
        __bd_text_undo_save(text);
      }
      
      return 1;
    } else if (event.key == '\x7F') {
      __bd_text_right(text, 0);
      __bd_text_backspace(text, 1);
      
      if (text->edit_count >= bd_config.undo_edit_count) {
        __bd_text_undo_save(text);
      }
      
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_ARROW_UP) {
      __bd_text_up(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_ARROW_DOWN) {
      __bd_text_down(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_ARROW_RIGHT) {
      __bd_text_right(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_ARROW_LEFT) {
      __bd_text_left(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_HOME) {
      __bd_text_home(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_END) {
      __bd_text_end(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_PAGE_UP) {
      for (int i = 0; i < (bd_height - 2) / 2; i++) {
        __bd_text_up(text, event.key != IO_UNSHIFT(event.key));
      }
      
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_PAGE_DOWN) {
      for (int i = 0; i < (bd_height - 2) / 2; i++) {
        __bd_text_down(text, event.key != IO_UNSHIFT(event.key));
      }
      
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_CTRL(IO_ARROW_UP)) {
      __bd_text_full_home(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (IO_UNSHIFT(event.key) == IO_CTRL(IO_ARROW_DOWN)) {
      __bd_text_full_end(text, event.key != IO_UNSHIFT(event.key));
      return 1;
    } else if (event.key == IO_CTRL('A')) {
      bd_cursor_t scroll = text->scroll;
      
      __bd_text_full_home(text, 0);
      __bd_text_full_end(text, 1);
      
      text->scroll = scroll;
      return 1;
    } else if (event.key == IO_CTRL('S')) {
      bd_text_save(view, 0);
      return 1;
    } else if (event.key == IO_CTRL('C') || event.key == IO_CTRL('X')) {
      if (!memcmp(&(text->cursor), &(text->hold_cursor), sizeof(bd_cursor_t))) {
        return 0;
      }
      
      io_file_t clipboard = io_copen(1);
      
      if (!io_fvalid(clipboard)) {
        return 0;
      }
      
      __bd_text_output(text, 1, clipboard, BD_CURSOR_MIN(text->cursor, text->hold_cursor), BD_CURSOR_MAX(text->cursor, text->hold_cursor));
      io_cclose(clipboard);
      
      if (event.key == IO_CTRL('X')) {
        __bd_text_undo_save(text);
        __bd_text_cursor(text);
        
        __bd_text_undo_save(text);
      }
      
      return 1;
    } else if (event.key == IO_CTRL('V')) {
      io_file_t clipboard = io_copen(0);
      
      if (!io_fvalid(clipboard)) {
        return 0;
      }
      
      __bd_text_undo_save(text);
      __bd_text_cursor(text);
      
      unsigned char buffer[4];
      
      while (io_fread(clipboard, buffer, 1)) {
        int utf_8_size = __bd_text_utf_8_size(buffer[0]);
        io_fread(clipboard, buffer + 1, utf_8_size - 1);
        
        unsigned int value = 0;
        
        for (int i = 0; i < utf_8_size; i++) {
          value |= ((unsigned int)(buffer[i]) << (i * 8));
        }
        
        __bd_text_write(text, value, 0);
      }
      
      io_cclose(clipboard);
      
      __bd_text_undo_save(text);
      return 1;
    } else if (event.key == IO_CTRL('F')) {
      char query[256] = {0}, replace[256] = {0};
      __bd_text_undo_save(text);
      
      for (;;) {
        int result = bd_dialog("Find/Replace in text (Ctrl+Q to exit)", -16, "i[Query:]i[Replace with:]b[3;Find next;Replace next;Replace all]", query, replace);
        
        if (!result) {
          break; // Ctrl+Q
        }
        
        bd_cursor_t cursor = BD_CURSOR_MAX(text->cursor, text->hold_cursor);
        
        if (result == 3) {
          cursor = (bd_cursor_t) {
            0, 0
          };
        }
        
        int done = 0;
        
        while (!done && cursor.y < text->count) {
          while (!done && cursor.x <= text->lines[cursor.y].length) {
            char replace_copy[256];
            
            int byte_index = __bd_text_utf_8_to_byte(text, cursor.x, cursor.y);
            int match_length = mt_match((const char *)(text->lines[cursor.y].data + byte_index), text->lines[cursor.y].size - byte_index, query, replace, replace_copy);
            
            if (match_length >= 0) {
              text->hold_cursor = cursor;
              
              text->cursor = (bd_cursor_t) {
                cursor.x + __bd_text_utf_8_from_byte(text, match_length, cursor.y), cursor.y
              };
              
              if (result >= 2) {
                __bd_text_cursor(text);
                
                for (int i = 0; replace_copy[i]; i++) {
                  __bd_text_write(text, replace_copy[i], 0);
                }
                
                text->hold_cursor = cursor;
              }
              
              if (result <= 2) {
                done = 1;
              }
            } else {
              cursor.x++;
            }
          }
          
          cursor.x = 0;
          cursor.y++;
        }
        
        __bd_text_undo_save(text);
        __bd_text_follow(text);
        
        bd_text_draw(view);
      }
      
      return 1;
    } else if (event.key == IO_CTRL('Z')) {
      __bd_text_undo(text);
      return 1;
    } else if (event.key == IO_CTRL('Y')) {
      __bd_text_redo(text);
      return 1;
    } else if (event.key == IO_ALT(IO_CTRL('M'))) {
      const unsigned char *line = text->lines[text->cursor.y].data;
      int start = __bd_text_utf_8_to_byte(text, text->cursor.x, text->cursor.y);
      
      if (start >= text->lines[text->cursor.y].size) {
        start = text->lines[text->cursor.y].size - 1;
      }
      
      while (start && (isalnum(line[start]) || strchr(".:/-_&+=?", line[start]))) {
        start--;
      }
      
      if (!isalnum(line[start])) {
        start++;
      }
      
      int end = start;
      
      while (end < text->lines[text->cursor.y].size && (isalnum(line[end]) || strchr(".:/-_&+=?", line[end]))) {
        end++;
      }
      
      char buffer[(end - start) + 1];
      buffer[end - start] = '\0';
      
      memcpy(buffer, line + start, end - start);
      bd_view = bd_view_add(buffer, bd_view_text, buffer) - bd_views;
      
      return 1;
    }
  } else if (event.type == IO_EVENT_MOUSE_DOWN || event.type == IO_EVENT_MOUSE_MOVE) {
    if (text->scroll_mouse_y >= 0) {
      text->scroll.y = ((event.mouse.y - (2 + text->scroll_mouse_y)) * (text->count + (bd_height - 2))) / (bd_height - 2);
      
      if (text->scroll.y < 0) {
        text->scroll.y = 0;
      } else if (text->scroll.y >= text->count) {
        text->scroll.y = text->count - 1;
      }
    } else if (event.type == IO_EVENT_MOUSE_DOWN && event.mouse.x >= bd_width - 1) {
      int scroll_start_y = 2 + (text->scroll.y * (bd_height - 2)) / (text->count + (bd_height - 2));
      int scroll_end_y = 3 + ((text->scroll.y + (bd_height - 2)) * (bd_height - 2)) / (text->count + (bd_height - 2));
      
      if (event.mouse.y >= scroll_start_y && event.mouse.y < scroll_end_y) {
        text->scroll_mouse_y = event.mouse.y - scroll_start_y;
      } else {
        text->scroll_mouse_y = (scroll_end_y - scroll_start_y) / 2;
        text->scroll.y = ((event.mouse.y - (2 + text->scroll_mouse_y)) * (text->count + (bd_height - 2))) / (bd_height - 2);
        
        if (text->scroll.y < 0) {
          text->scroll.y = 0;
        } else if (text->scroll.y >= text->count) {
          text->scroll.y = text->count - 1;
        }
      }
    } else {
      int cursor_x = (event.mouse.x - (lind_size + lind_left + lind_right + lind_post)) + text->scroll.x;
      int cursor_y = (event.mouse.y - 2) + text->scroll.y;
      
      if (cursor_y < 0) {
        cursor_y = 0;
      } else if (cursor_y >= text->count) {
        cursor_y = text->count - 1;
      }
      
      if (cursor_x < 0) {
        cursor_x = 0;
      } else if (cursor_x > text->lines[cursor_y].length) {
        cursor_x = text->lines[cursor_y].length;
      }
      
      text->cursor = (bd_cursor_t) {
        cursor_x, cursor_y
      };
      
      if (event.type == IO_EVENT_MOUSE_DOWN) {
        text->hold_cursor = text->cursor;
      }
      
      __bd_text_follow(text);
    }
    
    return 1;
  } else if (event.type == IO_EVENT_MOUSE_UP) {
    text->scroll_mouse_y = -1;
  } else if (event.type == IO_EVENT_SCROLL) {
    text->scroll.y += event.scroll * bd_config.scroll_step;
    
    if (text->scroll.y < 0) {
      text->scroll.y = 0;
    } else if (text->scroll.y >= text->count) {
      text->scroll.y = text->count - 1;
    }
    
    return 1;
  }
  
  return 0;
}

void bd_text_load(bd_view_t *view, const char *path) {
  bd_text_t *text = malloc(sizeof(bd_text_t));
  view->data = text;
  
  text->lines = malloc(sizeof(bd_line_t));
  text->count = 1;
  
  text->cursor = text->hold_cursor = text->scroll = (bd_cursor_t) {
    0, 0
  };
  
  text->path[0] = '\0';
  
  text->lines[0].data = NULL;
  text->lines[0].size = text->lines[0].length = 0;
  text->lines[0].syntax_state = -1;
  
  text->scroll_mouse_y = -1; // not scrolling
  
  text->prev = text->next = NULL;
  
  text->edit_count = 1;
  text->dirty = 0;
  
  text->syntax = st_init("");
  __bd_text_syntax(text, 0);
  
  if (!path) {
    __bd_text_undo_save(text);
    return;
  }
  
  io_file_t file = io_fopen(path, 0);
  
  if (!io_fvalid(file)) {
    __bd_text_undo_save(text);
    return;
  }
  
  text->syntax = st_init(path);
  strcpy(text->path, path);
  
  if (file.read_only) {
    strcat(text->path, " (read only)");
  }
  
  unsigned char buffer[4];
  
  while (io_fread(file, buffer, 1)) {
    int utf_8_size = __bd_text_utf_8_size(buffer[0]);
    io_fread(file, buffer + 1, utf_8_size - 1);
    
    unsigned int value = 0;
    
    for (int i = 0; i < utf_8_size; i++) {
      value |= ((unsigned int)(buffer[i]) << (i * 8));
    }
    
    __bd_text_write(text, value, 0);
  }
  
  text->cursor = text->hold_cursor = text->scroll = (bd_cursor_t) {
    0, 0
  };
  
  text->dirty = 0;
  
  for (int i = 0; i < text->count; i++) {
    text->lines[i].syntax_state = -1;
  }
  
  __bd_text_syntax(text, 0);
  io_fclose(file);
  
  __bd_text_undo_save(text);
}

int bd_text_save(bd_view_t *view, int closing) {
  bd_text_t *text = view->data;
  
  if (closing && !text->dirty) {
    return 1;  // won't write to disk if it has a path and isn't dirty
  }
  
  for (;;) {
    if (closing) {
      int result = bd_dialog("Close file (Ctrl+Q to not close)", -16, "i[Path:]b[2;Save;Do not save]", text->path);
      
      if (result == 0) {
        return 0;
      } else if (result == 2) {
        return 1;
      }
    }
    
    if (text->path[0]) {
      io_file_t file = io_fopen(text->path, 1);
      
      if (io_fvalid(file)) {
        io_frewind(file);
        
        bd_cursor_t end = (bd_cursor_t) {
          0, text->count - 1
        };
        
        end.x = text->lines[end.y].length;
        
        __bd_text_output(text, 1, file, (bd_cursor_t) {
          0, 0
        }, end);
        
        io_fclose(file);
        
        break;
      }
    }
    
    if (!closing) {
      if (!bd_dialog("Save file (Ctrl+Q to cancel)", -16, "i[Path:]b[1;Save]", text->path)) {
        return 1;
      }
    }
  }
  
  strcpy(view->title, text->path);
  text->syntax = st_init(text->path);
  
  view->title_dirty = 1;
  text->dirty = 0;
  
  for (int i = 0; i < text->count; i++) {
    text->lines[i].syntax_state = -1;
  }
  
  __bd_text_syntax(text, 0);
  return 1;
}
