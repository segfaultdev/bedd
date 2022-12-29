#include <stdlib.h>
#include <string.h>
#include <syntax.h>
#include <bedd.h>
#include <io.h>

typedef struct bd_line_t bd_line_t;
typedef struct bd_text_t bd_text_t;

struct bd_line_t {
  char *data;
  int length;
  
  int syntax_state; // State *after* including the newline
};

struct bd_text_t {
  char path[256];
  
  bd_line_t *lines;
  int count;
  
  bd_cursor_t cursor, hold_cursor, scroll;
  int dirty;
  
  int scroll_mouse_y;
  
  syntax_t syntax;
};

// static functions

static int  __bd_text_cursor(bd_text_t *text);
static void __bd_text_backspace(bd_text_t *text, int fix_cursor);
static void __bd_text_write(bd_text_t *text, char chr, int by_user);
static void __bd_text_up(bd_text_t *text, int hold);
static void __bd_text_down(bd_text_t *text, int hold);
static void __bd_text_right(bd_text_t *text, int hold);
static void __bd_text_left(bd_text_t *text, int hold);
static void __bd_text_home(bd_text_t *text, int hold);
static void __bd_text_end(bd_text_t *text, int hold);
static void __bd_text_full_home(bd_text_t *text, int hold);
static void __bd_text_full_end(bd_text_t *text, int hold);
static void __bd_text_follow(bd_text_t *text);
static void __bd_text_output(bd_text_t *text, io_file_t file, bd_cursor_t start, bd_cursor_t end);
static void __bd_text_syntax(bd_text_t *text, int start_line);

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
    memmove(line->data + (text->cursor.x - 1), line->data + text->cursor.x, line->length - text->cursor.x);
    
    text->cursor.x--;
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
    
    prev_line->data = realloc(prev_line->data, prev_line->length + line->length);
    
    if (line->length) {
      memcpy(prev_line->data + prev_line->length, line->data, line->length);
    }
    
    text->cursor.x = prev_line->length;
    
    prev_line->length += line->length;
    free(line->data);
    
    for (int i = text->cursor.y; i < text->count - 1; i++) {
      text->lines[i] = text->lines[i + 1];
    }
    
    text->lines = realloc(text->lines, (text->count - 1) * sizeof(bd_line_t));
    
    text->cursor.y--;
    text->count--;
  }
  
  if (fix_cursor) text->hold_cursor = text->cursor;
  text->dirty = 1;
  
  __bd_text_syntax(text, text->cursor.y);
  __bd_text_follow(text);
}

static void __bd_text_write(bd_text_t *text, char chr, int by_user) {
  __bd_text_cursor(text);
  text->dirty = 1;
  
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
    char last = '\0', pair = '\0';
    
    if (by_user) {
      space_count += !!text->syntax.f_depth(text->lines[text->cursor.y].data, text->cursor.x) * bd_config.indent_width;
      
      if (text->cursor.x) {
        last = text->lines[text->cursor.y].data[text->cursor.x - 1];
        pair = text->syntax.f_pair(text->lines[text->cursor.y].data, text->cursor.x - 1, last);
      }
    }
    
    line.length = space_count + (text->lines[text->cursor.y].length - text->cursor.x),
    line.data = line.length ? malloc(line.length) : NULL;
    
    if (by_user && space_count) {
      memset(line.data, ' ', space_count);
    }
    
    if (line.length - space_count) {
      memcpy(line.data + space_count, text->lines[text->cursor.y].data + text->cursor.x, line.length - space_count);
    }
    
    text->lines[text->cursor.y].data = realloc(text->lines[text->cursor.y].data, text->lines[text->cursor.y].length = text->cursor.x);
    text->lines[text->cursor.y + 1] = line;
    
    text->cursor.x = space_count;
    text->cursor.y++;
    
    if (pair) {
      if (!(text->cursor.x < text->lines[text->cursor.y].length && text->lines[text->cursor.y].data[text->cursor.x] != pair)) {
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
  
  bd_line_t *line = text->lines + text->cursor.y;
  line->data = realloc(line->data, line->length + 1);
  
  if (line->length - text->cursor.x) {
    memmove(line->data + text->cursor.x + 1, line->data + text->cursor.x, line->length - text->cursor.x);
  }
  
  line->data[text->cursor.x++] = chr;
  line->length++;
  
  text->hold_cursor = text->cursor;
  __bd_text_follow(text);
  
  if (!by_user) return;
  char pair = text->syntax.f_pair(line->data, text->cursor.x - 1, chr);
  
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
}

static void __bd_text_output(bd_text_t *text, io_file_t file, bd_cursor_t start, bd_cursor_t end) {
  if (start.x > text->lines[start.y].length) {
    start.x = text->lines[start.y].length;
  }
  
  if (end.x > text->lines[end.y].length) {
    end.x = text->lines[end.y].length;
  }
  
  int space_count = 0;
  
  while (space_count < text->lines[start.y].length && text->lines[start.y].data[space_count] == ' ') {
    space_count++;
  }
  
  while (start.y < end.y || (start.y == end.y && start.x < end.x)) {
    char chr;
    
    if (start.x >= text->lines[start.y].length) {
      chr = '\n';
      
      start.x = 0;
      start.y++;
      
      space_count = 0;
      
      while (space_count < text->lines[start.y].length && text->lines[start.y].data[space_count] == ' ') {
        space_count++;
      }
    } else if (!bd_config.indent_spaces && start.x < space_count && !(start.x % bd_config.indent_width)) {
      chr = '\t';
      start.x += bd_config.indent_width;
    } else {
      chr = text->lines[start.y].data[start.x];
      start.x++;
    }
    
    io_fwrite(file, &chr, 1);
  }
}

static void __bd_text_syntax(bd_text_t *text, int start_line) {
  int state = start_line ? text->lines[start_line - 1].syntax_state : 0;
  
  for (int i = 0; i < text->lines[start_line].length; i++) {
    text->syntax.f_color(state, &state, text->lines[start_line].data + i, text->lines[start_line].length - i);
  }
  
  text->syntax.f_color(state, &state, "\n", 1);
  if (state == text->lines[start_line].syntax_state) return;
  
  text->lines[start_line++].syntax_state = state;
  if (start_line >= text->count) return;
  
  __bd_text_syntax(text, start_line);
}

// public functions

void bd_text_draw(bd_view_t *view) {
  bd_text_t *text = view->data;
  int lind_size = 1, lind_max = 10;
  
  while (text->count >= lind_max) {
    lind_size++;
    lind_max *= 10;
  }
  
  int view_width = bd_width - ((lind_size + 5) + 2);
  int view_height = bd_height - 2;
  
  bd_cursor_t min_cursor = BD_CURSOR_MIN(text->cursor, text->hold_cursor);
  bd_cursor_t max_cursor = BD_CURSOR_MAX(text->cursor, text->hold_cursor);
  
  for (int i = 0; i < view_height; i++) {
    io_cursor(0, 2 + i);
    int y = i + text->scroll.y;
    
    int is_selected = 0;
    
    if (y < text->count) {
      if (y == text->cursor.y) {
        io_printf(IO_INVERT "  %*d %c" IO_NORMAL " ", lind_size, y + 1, text->scroll.x ? '<' : ' ');
      } else {
        io_printf(IO_SHADOW_1 "  %*d %c" IO_NORMAL " ", lind_size, y + 1, text->scroll.x ? '<' : ' ');
      }
      
      bd_line_t *line = text->lines + y;
      
      int space_count = 0;
      
      while (space_count < line->length && line->data[space_count] == ' ') {
        space_count++;
      }
      
      int state = y ? text->lines[y - 1].syntax_state : 0;
      int last_color = st_color_none;
      
      for (int j = 0; j < view_width; j++) {
        int x = j + text->scroll.x;
        bd_cursor_t cursor = (bd_cursor_t){x, y};
        
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
        
        if (x == line->length) {
          io_printf(" ");
        } else if (x > line->length) {
          break;
        } else {
          if (x < space_count) {
            text->syntax.f_color(state, &state, "\n", 1);
            io_printf("%s\u00B7" IO_WHITE, y == text->cursor.y ? IO_DARK_GRAY : IO_BLACK);
          } else {
            int color = text->syntax.f_color(state, &state, line->data + x, line->length - x);
            if (color == st_color_none) color = last_color;
            
            int temp_color = color;
            if (color == last_color) temp_color = st_color_none;
            
            io_printf("%s%c", bd_config.syntax_colors[temp_color], line->data[x]);
            last_color = color;
          }
        }
      }
      
      io_printf(IO_NORMAL IO_CLEAR_LINE IO_SHADOW_1);
      io_cursor(bd_width - 2, 2 + i);
      
      io_printf((text->scroll.x + view_width < line->length) ? ">" : " ");
      line->syntax_state = state;
    } else {
      io_printf("  %*c : ", lind_size, ' ');
      
      io_printf(IO_CLEAR_LINE IO_SHADOW_1);
      io_cursor(bd_width - 2, 2 + i);
      
      io_printf(":");
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
  
  io_flush();
  
  int temp_x = text->cursor.x > text->lines[text->cursor.y].length ? text->lines[text->cursor.y].length : text->cursor.x;
  
  int cursor_x = (temp_x - text->scroll.x) + (lind_size + 5);
  int cursor_y = (text->cursor.y - text->scroll.y) + 2;
  
  if (cursor_x < lind_size + 5 || cursor_x >= bd_width - 2) cursor_x = -1;
  if (cursor_y < 2 || cursor_y >= bd_height) cursor_y = -1;
  
  char new_title[257];
  sprintf(new_title, "%s%s", text->path[0] ? text->path : "(no name)", text->dirty ? "*" : "");
  
  view->title_dirty = strcmp(view->title, new_title);
  strcpy(view->title, new_title);
  
  view->cursor = (bd_cursor_t){cursor_x, cursor_y};
}

int bd_text_event(bd_view_t *view, io_event_t event) {
  bd_text_t *text = view->data;
  
  if (event.type == IO_EVENT_KEY_PRESS) {
    if (IO_UNSHIFT(event.key) == '\t' && memcmp(&(text->cursor), &(text->hold_cursor), sizeof(bd_cursor_t))) {
      bd_cursor_t min_cursor = BD_CURSOR_MIN(text->cursor, text->hold_cursor);
      bd_cursor_t max_cursor = BD_CURSOR_MAX(text->cursor, text->hold_cursor);
      
      bd_cursor_t old_cursor = text->cursor;
      bd_cursor_t old_hold_cursor = text->hold_cursor;
      
      for (int i = min_cursor.y; i <= max_cursor.y; i++) {
        if (event.key == IO_UNSHIFT(event.key)) {
          text->cursor = text->hold_cursor = (bd_cursor_t){0, i};
          __bd_text_write(text, '\t', 1);
        } else {
          bd_line_t *line = text->lines + i;
          int space_count = 0;
          
          while (space_count < line->length && line->data[space_count] == ' ' && space_count < bd_config.indent_width) {
            space_count++;
          }
          
          text->cursor = text->hold_cursor = (bd_cursor_t){space_count, i};
          
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
        if (text->cursor.x < 0) text->cursor.x = 0;
        
        text->hold_cursor.x -= bd_config.indent_width;
        if (text->hold_cursor.x < 0) text->hold_cursor.x = 0;
      }
      
      return 1;
    } else if ((event.key >= 32 && event.key < 127) || event.key == '\t') {
      __bd_text_write(text, event.key, 1);
      return 1;
    } else if (event.key == IO_CTRL('M')) {
      __bd_text_write(text, '\n', 1);
      return 1;
    } else if (event.key == IO_CTRL('H')) {
      __bd_text_backspace(text, 1);
      return 1;
    } else if (event.key == '\x7F') {
      __bd_text_right(text, 0);
      __bd_text_backspace(text, 1);
      
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
      int lind_size = 1, lind_max = 10;
      
      while (text->count >= lind_max) {
        lind_size++;
        lind_max *= 10;
      }
      
      int cursor_x = (event.mouse.x - (5 + lind_size)) + text->scroll.x;
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
      
      text->cursor = (bd_cursor_t){cursor_x, cursor_y};
      
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
  
  text->cursor = text->hold_cursor = text->scroll = (bd_cursor_t){0, 0};
  text->path[0] = '\0';
  
  text->lines[0].data = NULL;
  text->lines[0].length = 0;
  text->lines[0].syntax_state = -1;
  
  text->scroll_mouse_y = -1; // not scrolling
  
  text->dirty = 0;
  
  text->syntax = st_init("");
  __bd_text_syntax(text, 0);
  
  if (!path) return;
  
  io_file_t file = io_fopen(path, 0);
  if (!io_fvalid(file)) return;
  
  text->syntax = st_init(path);
  
  strcpy(text->path, path);
  char chr;
  
  while (io_fread(file, &chr, 1)) {
    __bd_text_write(text, chr, 0);
  }
  
  text->cursor = text->hold_cursor = text->scroll = (bd_cursor_t){0, 0};
  text->dirty = 0;
  
  for (int i = 0; i < text->count; i++) {
    text->lines[i].syntax_state = -1;
  }
  
  __bd_text_syntax(text, 0);
  io_fclose(file);
}

int bd_text_save(bd_view_t *view, int closing) {
  bd_text_t *text = view->data;
  if (text->path[0] && !text->dirty) return 1; // won't write to disk if it has a path and isn't dirty
  
  for (;;) {
    if (closing) {
      int result = bd_dialog("Close file (Ctrl+Q to not close)", -16, "i[Path:]b[2;Save;Do not save]", text->path);
      
      if (result == 0) return 0;
      else if (result == 2) return 1;
    }
    
    if (text->path[0]) {
      io_file_t file = io_fopen(text->path, 1);
      
      if (io_fvalid(file)) {
        io_frewind(file);
        
        bd_cursor_t end = (bd_cursor_t){0, text->count - 1};
        end.x = text->lines[end.y].length;
        
        __bd_text_output(text, file, (bd_cursor_t){0, 0}, end);
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
