#include <stdlib.h>
#include <string.h>
#include <bedd.h>
#include <io.h>

typedef struct bd_line_t bd_line_t;
typedef struct bd_text_t bd_text_t;

struct bd_line_t {
  char *data;
  int length;
};

struct bd_text_t {
  char path[256];
  
  bd_line_t *lines;
  int count;
  
  bd_cursor_t cursor, hold_cursor, scroll;
  int dirty;
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

static int __bd_text_cursor(bd_text_t *text) {
  // cursor.x must not exceed the length of the line, but *can* be equal to it
  
  if (text->cursor.x > text->lines[text->cursor.y].length) {
    text->cursor.x = text->lines[text->cursor.y].length;
  }
  
  // same logic applies to hold_cursor.x
  
  if (text->hold_cursor.x > text->lines[text->hold_cursor.y].length) {
    text->hold_cursor.x = text->lines[text->hold_cursor.y].length;
  }
  
  // make sure hold_cursor is behind and not after cursor, to make the following code simpler
  
  if (text->hold_cursor.y > text->cursor.x || (text->hold_cursor.y == text->cursor.y && text->hold_cursor.x > text->cursor.x)) {
    bd_cursor_t temp = text->cursor;
    
    text->cursor = text->hold_cursor;
    text->hold_cursor = temp;
  }
  
  // then, cursor must be equal to hold_cursor, so delete chars in between!
  
  if (text->cursor.x == text->hold_cursor.x && text->cursor.y == text->hold_cursor.y) {
    return 0;
  }
  
  while (text->cursor.x != text->hold_cursor.x || text->cursor.y != text->hold_cursor.y) {
    __bd_text_backspace(text, 0);
  }
  
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
  } else {
    if (!text->cursor.y) {
      if (fix_cursor) text->hold_cursor = text->cursor;
      __bd_text_follow(text);
      
      return;
    }
    
    bd_line_t *prev_line = text->lines + (text->cursor.y - 1);
    bd_line_t *line = text->lines + text->cursor.y;
    
    prev_line->data = realloc(prev_line->data, prev_line->length + line->length);
    memcpy(prev_line->data + prev_line->length, line->data, line->length);
    
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
      memmove(text->lines + text->cursor.y + 1, text->lines + text->cursor.y, (text->count - 1) - text->cursor.y);
    }
    
    text->count++;
    
    int space_count = 0;
    bd_line_t line;
    
    while (by_user && space_count < text->lines[text->cursor.y].length && text->lines[text->cursor.y].data[space_count] == ' ') {
      space_count++;
    }
    
    line.length = space_count + (text->lines[text->cursor.y].length - text->cursor.x),
    line.data = malloc(line.length);
    
    if (by_user) {
      memset(line.data, ' ', space_count);
    }
    
    memcpy(line.data + space_count, text->lines[text->cursor.y].data + text->cursor.x, line.length);
    text->lines[text->cursor.y].data = realloc(text->lines[text->cursor.y].data, text->lines[text->cursor.y].length = text->cursor.x);
    
    text->lines[text->cursor.y + 1] = line;
    
    text->cursor.x = space_count;
    text->cursor.y++;
    
    text->hold_cursor = text->cursor;
    __bd_text_follow(text);
    
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

// public functions

void bd_text_draw(bd_view_t *view) {
  bd_text_t *text = view->data;
  
  int lind_size = 1;
  int lind_max = 10;
  
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
            io_printf(IO_NORMAL);
            is_selected = 0;
          }
        }
        
        if (x == line->length) {
          io_printf(" ");
        } else if (x > line->length) {
          break;
        } else {
          if (x < space_count) {
            io_printf("%s\u00B7" IO_WHITE, y == text->cursor.y ? IO_DARK_GRAY : IO_BLACK);
          } else {
            io_printf("%c", line->data[x]);
          }
        }
      }
      
      io_printf(IO_NORMAL IO_CLEAR_LINE IO_SHADOW_1);
      io_cursor(bd_width - 2, 2 + i);
      
      io_printf((text->scroll.x + view_width < line->length) ? ">" : " ");
    } else {
      io_printf("  %*c : ", lind_size, ' ');
      
      io_printf(IO_CLEAR_LINE IO_SHADOW_1);
      io_cursor(bd_width - 2, 2 + i);
      
      io_printf(":");
    }
    
    io_printf(IO_NORMAL IO_CLEAR_LINE);
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
    }
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
  
  text->dirty = 0;
  if (!path) return;
  
  io_file_t file = io_fopen(path, 0);
  if (!io_fvalid(file)) return;
  
  strcpy(text->path, path);
  char chr;
  
  while (io_fread(file, &chr, 1)) {
    __bd_text_write(text, chr, 0);
  }
  
  text->cursor = text->hold_cursor = text->scroll = (bd_cursor_t){0, 0};
  text->dirty = 0;
  
  io_fclose(file);
}

void bd_text_save(bd_view_t *view) {
  bd_text_t *text = view->data;
  
  if (text->path[0] && !text->dirty) return; // won't save if it has a path and isn't dirty
  
  for (;;) {
    if (text->path[0]) {
      io_file_t file = io_fopen(text->path, 1);
      
      if (io_fvalid(file)) {
        io_frewind(file);
        
        io_fwrite(file, NULL, 0); // TODO: get data to be saved
        io_fclose(file);
        
        break;
      }
    }
    
    if (!bd_dialog("Save file (Ctrl+Q to cancel)", -16, "i[Path:]b[1;Save]", text->path)) {
      return;
    }
  }
  
  strcpy(view->title, text->path);
  
  view->title_dirty = 1;
  text->dirty = 0;
}
