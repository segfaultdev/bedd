#include <stdlib.h>
#include <ctype.h>
#include <bedd.h>

typedef struct bd_char_t bd_char_t;
typedef struct bd_line_t bd_line_t;

typedef struct bd_terminal_t bd_terminal_t;

struct bd_char_t {
  unsigned int code_point;
  
  unsigned char *ansi_data;
  int ansi_size;
};

struct bd_line_t {
  bd_char_t *data;
  int size;
};

struct bd_terminal_t {
  bd_line_t *lines;
  int count;
  
  bd_cursor_t cursor;
  int scroll_y, scroll_mouse_y;
  
  io_file_t file;
};

void bd_terminal_draw(bd_view_t *view) {
  bd_terminal_t *terminal = view->data;
  
  io_cursor(0, 2);
  io_printf(IO_CLEAR_CURSOR);
  
  for (int i = 0; i < bd_height - 2; i++) {
    int y = i + terminal->scroll_y;
    io_cursor(0, i + 2);
    
    io_printf(IO_NORMAL);
    
    if (y < 0 || y >= terminal->count) {
      io_printf(IO_CLEAR_LINE);
      
      io_cursor(bd_width - 2, i + 2);
      io_printf(IO_SHADOW_1 ":");
    } else {
      bd_line_t line = terminal->lines[y];
      
      for (int j = 0; j < line.size && j < bd_width - 2; j++) {
        bd_char_t chr = line.data[j];
        io_printf("%.*s%lc", chr.ansi_size, chr.ansi_data, chr.code_point);
      }
      
      io_printf(IO_CLEAR_LINE);
      
      io_cursor(bd_width - 2, i + 2);
      io_printf(IO_SHADOW_1 " ");
    }
  }
  
  int cursor_y = (terminal->cursor.y - terminal->scroll_y) + 2;
  
  if (cursor_y < 2 || cursor_y >= bd_height) {
    cursor_y = -1;
  }
  
  view->cursor = (bd_cursor_t) {
    terminal->cursor.x, cursor_y,
  };
}

int bd_terminal_tick(bd_view_t *view) {
  bd_terminal_t *terminal = view->data;
  int do_draw = 0;
  
  unsigned char buffer[256];
  int length;
  
  while (io_fread(terminal->file, buffer, 1) > 0) {
    length = 1;
    // TODO: UTF-8
    
    if (buffer[0] == '\x1B') {
      while (io_fread(terminal->file, buffer + length, 1) > 0) {
        length++;
        
        if (isalpha(buffer[length - 1])) {
          break;
        }
      }
      
      if (buffer[length - 1] == 'H') {
        int x, y;
        sscanf(buffer + 2, "%d;%d", &y, &x);
        
        terminal->cursor.x = x - 1;
        terminal->cursor.y = y - 1;
      } else {
        // TODO: Add to buffer
      }
    } else if (buffer[0] == '\b') {
      if (terminal->cursor.x) {
        terminal->cursor.x--;
      } else if (terminal->cursor.y) {
        terminal->cursor.y--;
        terminal->cursor.x = terminal->lines[terminal->cursor.y].size;
      }
    } else if (buffer[0] == '\r') {
      terminal->cursor.x = 0;
    } else if (buffer[0] == '\n') {
      terminal->cursor.x = 0;
      terminal->cursor.y++;
    } else {
      unsigned int code_point = buffer[0]; // TODO: UTF-8
      terminal->cursor.x++;
      
      if (terminal->cursor.y >= terminal->count) {
        terminal->lines = realloc(terminal->lines, (terminal->cursor.y + 1) * sizeof(bd_line_t));
        
        for (int i = terminal->count; i <= terminal->cursor.y; i++) {
          terminal->lines[i] = (bd_line_t) {
            .data = NULL,
            .size = 0,
          };
        }
        
        terminal->count = terminal->cursor.y + 1;
      }
      
      if (terminal->lines[terminal->cursor.y].size < terminal->cursor.x) {
        terminal->lines[terminal->cursor.y].data = realloc(terminal->lines[terminal->cursor.y].data, terminal->cursor.x * sizeof(bd_char_t));
        
        for (int i = terminal->lines[terminal->cursor.y].size; i < terminal->cursor.x; i++) {
          terminal->lines[terminal->cursor.y].data[i] = (bd_char_t) {
            .code_point = ' ',
            
            .ansi_data = NULL,
            .ansi_size = 0,
          };
        }
        
        terminal->lines[terminal->cursor.y].size = terminal->cursor.x;
      }
      
      terminal->lines[terminal->cursor.y].data[terminal->cursor.x - 1].code_point = code_point;
    }
    
    do_draw = 1;
  }
  
  return do_draw;
}

int bd_terminal_event(bd_view_t *view, io_event_t event) {
  bd_terminal_t *terminal = view->data;
  
  if (event.type == IO_EVENT_KEY_PRESS) {
    if (event.key < 128) {
      io_fwrite(terminal->file, &event.key, 1);
    }
  }
  
  return 0;
}

void bd_terminal_load(bd_view_t *view) {
  bd_terminal_t *terminal = (view->data = malloc(sizeof(bd_terminal_t)));
  
  terminal->lines = calloc(sizeof(bd_line_t), 1);
  terminal->count = 1;
  
  terminal->cursor = (bd_cursor_t) {
    0, 0,
  };
  
  terminal->scroll_y = 0;
  terminal->scroll_mouse_y = -1;
  
  terminal->file = io_topen(bd_config.shell_path);
}

void bd_terminal_kill(bd_view_t *view) {
  bd_terminal_t *terminal = view->data;
  io_tclose(terminal->file);
}
