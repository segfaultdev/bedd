#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <bedd.h>

typedef struct bd_char_t bd_char_t;
typedef struct bd_line_t bd_line_t;

typedef struct bd_terminal_t bd_terminal_t;

struct bd_char_t {
  unsigned int code_point;
  
  int fore_color, back_color; // -1 = Default, 0-255 = xterm colors
  
  unsigned char bold:      1;
  unsigned char inverted:  1;
  unsigned char underline: 1;
};

struct bd_line_t {
  bd_char_t *data;
  int size;
  
  int dirty;
};

struct bd_terminal_t {
  char title[240];
  
  bd_line_t *lines;
  int count;
  
  bd_cursor_t cursor;
  int scroll_y, scroll_mouse_y;
  
  bd_char_t style;
  io_file_t file;
  
  int dirty;
};

static const bd_char_t __bd_empty = (bd_char_t) {
  .code_point = ' ',
  
  .fore_color = -1,
  .back_color = -1,
  
  .bold = 0,
  .inverted = 0,
  .underline = 0,
};

static void __bd_terminal_write(bd_terminal_t *terminal, int length, unsigned char buffer[length]);

static void __bd_terminal_write(bd_terminal_t *terminal, int length, unsigned char buffer[length]) {
  if (buffer[0] == '\b') {
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
    unsigned int code_point = buffer[0];
    
    if (code_point >= 0xF0) {
      code_point = ((unsigned int)(buffer[0] & 0x07) << 18) |
                   ((unsigned int)(buffer[1] & 0x3F) << 12) |
                   ((unsigned int)(buffer[2] & 0x3F) << 6) |
                   ((unsigned int)(buffer[3] & 0x3F) << 0);
    } else if (code_point >= 0xE0) {
      code_point = ((unsigned int)(buffer[0] & 0x0F) << 12) |
                   ((unsigned int)(buffer[1] & 0x3F) << 6) |
                   ((unsigned int)(buffer[2] & 0x3F) << 0);
    } else if (code_point >= 0xC0) {
      code_point = ((unsigned int)(buffer[0] & 0x1F) << 6) |
                   ((unsigned int)(buffer[1] & 0x3F) << 0);
    }
    
    terminal->cursor.x++;
    
    if (terminal->cursor.y >= terminal->count) {
      terminal->lines = realloc(terminal->lines, (terminal->cursor.y + 1) * sizeof(bd_line_t));
      
      for (int i = terminal->count; i <= terminal->cursor.y; i++) {
        terminal->lines[i] = (bd_line_t) {
          .data = NULL,
          .size = 0,
          
          .dirty = 1,
        };
      }
      
      terminal->count = terminal->cursor.y + 1;
    }
    
    if (terminal->lines[terminal->cursor.y].size < terminal->cursor.x) {
      terminal->lines[terminal->cursor.y].data = realloc(terminal->lines[terminal->cursor.y].data, terminal->cursor.x * sizeof(bd_char_t));
      
      for (int i = terminal->lines[terminal->cursor.y].size; i < terminal->cursor.x; i++) {
        terminal->lines[terminal->cursor.y].data[i] = __bd_empty;
      }
      
      terminal->lines[terminal->cursor.y].size = terminal->cursor.x;
    }
    
    terminal->lines[terminal->cursor.y].data[terminal->cursor.x - 1] = terminal->style;
    terminal->lines[terminal->cursor.y].data[terminal->cursor.x - 1].code_point = code_point;
    
    terminal->lines[terminal->cursor.y].dirty = 1;
  }
}

void bd_terminal_draw(bd_view_t *view) {
  bd_terminal_t *terminal = view->data;
  
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
      
      if (terminal->dirty) {
        line.dirty = 1;
      }
      
      for (int j = 0; line.dirty && j < line.size && j < bd_width - 2; j++) {
        bd_char_t chr = line.data[j];
        
        if (chr.fore_color < 0) {
          io_printf(IO_DEFAULT_FORE);
        } else {
          io_printf(IO_XTERM_FORE("%d"), chr.fore_color);
        }
        
        if (chr.back_color < 0) {
          io_printf(IO_DEFAULT_BACK);
        } else {
          io_printf(IO_XTERM_BACK("%d"), chr.back_color);
        }
        
        if (chr.bold) {
          io_printf("\x1B[1m");
        } else {
          io_printf("\x1B[21m");
        }
        
        if (chr.inverted) {
          io_printf("\x1B[7m");
        } else {
          io_printf("\x1B[27m");
        }
        
        if (chr.underline) {
          io_printf("\x1B[4m");
        } else {
          io_printf("\x1B[24m");
        }
        
        io_printf("%lc", chr.code_point);
      }
      
      if (line.dirty) {
        io_printf(IO_NORMAL IO_CLEAR_LINE);
        terminal->lines[y].dirty = 0;
      }
      
      io_cursor(bd_width - 2, i + 2);
      io_printf(IO_SHADOW_1 " ");
    }
    
    int scroll_start_y = (terminal->scroll_y * (bd_height - 2)) / terminal->count;
    int scroll_end_y = 1 + ((terminal->scroll_y + (bd_height - 2)) * (bd_height - 2)) / terminal->count;
    
    if (i >= scroll_start_y && i < scroll_end_y) {
      io_printf(IO_SHADOW_2 " ");
    } else {
      io_printf(IO_NORMAL "\u2502");
    }
    
    io_printf(IO_NORMAL);
  }
  
  terminal->dirty = 0;
  int cursor_y = (terminal->cursor.y - terminal->scroll_y) + 2;
  
  if (cursor_y < 2 || cursor_y >= bd_height) {
    cursor_y = -1;
  }
  
  sprintf(view->title, "%s (Terminal)", terminal->title);
  
  view->cursor = (bd_cursor_t) {
    terminal->cursor.x, cursor_y,
  };
}

int bd_terminal_tick(bd_view_t *view) {
  bd_terminal_t *terminal = view->data;
  int do_draw = 0;
  
  unsigned char buffer[240];
  int length;
  
  while (io_fread(terminal->file, buffer, 1) > 0) {
    length = 1;
    
    if (buffer[0] >= 0xF0) {
      length = 4;
    } else if (buffer[0] >= 0xE0) {
      length = 3;
    } else if (buffer[0] >= 0xC0) {
      length = 2;
    }
    
    if (length > 1) {
      io_fread(terminal->file, buffer + 1, length - 1);
    }
    
    if (buffer[0] == '\x1B') {
      while (io_fread(terminal->file, buffer + length, 1) > 0) {
        length++;
        
        if (isalpha(buffer[length - 1]) || buffer[length - 1] == '\x07') {
          break;
        }
        
        if (buffer[length - 1] == '\x1B') {
          io_fread(terminal->file, buffer + length, 1);
          break;
        }
      }
      
      if (buffer[1] == ']' && buffer[2] == '0' && buffer[3] == ';') {
        buffer[length - 1] = '\0';
        strcpy(terminal->title, (char *)(buffer) + 4);
      }
      
      if (buffer[1] != '[') {
        continue;
      }
      
      // fprintf(stderr, "'%.*s'\n", length - 1, (char *)(buffer + 1));
      
      if (buffer[length - 1] == 'A') {
        int step = 1;
        
        if (isdigit(buffer[2])) {
          sscanf((char *)(buffer + 2), "%d", &step);
        }
        
        terminal->cursor.y -= step;
      } else if (buffer[length - 1] == 'B') {
        int step = 1;
        
        if (isdigit(buffer[2])) {
          sscanf((char *)(buffer + 2), "%d", &step);
        }
        
        terminal->cursor.y += step;
      } else if (buffer[length - 1] == 'C') {
        int step = 1;
        
        if (isdigit(buffer[2])) {
          sscanf((char *)(buffer + 2), "%d", &step);
        }
        
        terminal->cursor.x += step;
      } else if (buffer[length - 1] == 'D') {
        int step = 1;
        
        if (isdigit(buffer[2])) {
          sscanf((char *)(buffer + 2), "%d", &step);
        }
        
        terminal->cursor.x -= step;
      }
      
      if (buffer[length - 1] == 'H' || buffer[length - 1] == 'f') {
        int x = 1, y = 1;
        
        if (isdigit(buffer[2])) {
          sscanf((char *)(buffer + 2), "%d;%d", &y, &x);
        }
        
        if (x < 1) {
          x = 1;
        } else if (x > bd_width - 2) {
          x = bd_width - 2;
        }
        
        if (y < 1) {
          y = 1;
        } else if (y > bd_height - 2) {
          y = bd_height - 2;
        }
        
        terminal->cursor.x = x - 1;
        terminal->cursor.y = (y - 1) + terminal->scroll_y;
      }
      
      if (buffer[length - 1] == 'J') {
        int mode = 0;
        
        if (isdigit(buffer[2])) {
          sscanf((char *)(buffer + 2), "%d", &mode);
        }
        
        if (mode == 0) {
          if (terminal->cursor.y < terminal->count) {
            terminal->lines[terminal->cursor.y].data = realloc(terminal->lines[terminal->cursor.y].data, terminal->cursor.x * sizeof(bd_char_t));
            terminal->lines[terminal->cursor.y].size = terminal->cursor.x;
            
            for (int i = terminal->cursor.y + 1; i < terminal->count; i++) {
              free(terminal->lines[i].data);
            }
            
            terminal->count = terminal->cursor.y + 1;
            terminal->lines = realloc(terminal->lines, terminal->count * sizeof(bd_line_t));
          }
        } else if (mode == 1) {
          for (int i = 0; i < terminal->cursor.y && i < terminal->count; i++) {
            if (terminal->lines[terminal->cursor.y].data) {
              free(terminal->lines[terminal->cursor.y].data);
              
              terminal->lines[terminal->cursor.y].data = NULL;
              terminal->lines[terminal->cursor.y].size = 0;
            }
          }
          
          if (terminal->cursor.y < terminal->count) {
            for (int i = 0; i < terminal->cursor.x && i < terminal->lines[terminal->cursor.y].size; i++) {
              terminal->lines[terminal->cursor.y].data[i] = terminal->style;
            }
          }
        } else if (mode == 2) {
          for (int i = 0; i < terminal->count; i++) {
            free(terminal->lines[i].data);
          }
          
          terminal->count = 1;
          terminal->lines = realloc(terminal->lines, terminal->count * sizeof(bd_line_t));
          
          terminal->lines[0] = (bd_line_t) {
            .data = NULL,
            .size = 0,
            
            .dirty = 1,
          };
        }
        
        terminal->dirty = 1;
      }
      
      if (buffer[length - 1] == 'K') {
        int mode = 0;
        
        if (isdigit(buffer[2])) {
          sscanf((char *)(buffer + 2), "%d", &mode);
        }
        
        if (terminal->cursor.y < terminal->count) {
          if (mode == 0) {
            terminal->lines[terminal->cursor.y].data = realloc(terminal->lines[terminal->cursor.y].data, terminal->cursor.x * sizeof(bd_char_t));
            terminal->lines[terminal->cursor.y].size = terminal->cursor.x;
          } else if (mode == 1) {
            for (int i = 0; i < terminal->cursor.x && i < terminal->lines[terminal->cursor.y].size; i++) {
              terminal->lines[terminal->cursor.y].data[i] = terminal->style;
            }
          } else if (mode == 2) {
            if (terminal->lines[terminal->cursor.y].data != NULL) {
              free(terminal->lines[terminal->cursor.y].data);
            }
            
            terminal->lines[terminal->cursor.y].data = NULL;
            terminal->lines[terminal->cursor.y].size = 0;
          }
          
          terminal->lines[terminal->cursor.y].dirty = 1;
        }
      }
      
      if (buffer[length - 1] == 'm') {
        unsigned char *ptr;
        length = 2;
        
        while (!isalpha(buffer[length - 1])) {
          int code = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
          length = (ptr - buffer) + 1;
          
          if (!code) {
            terminal->style = __bd_empty;
          }
          
          if (code >= 30 && code <= 37) {
            terminal->style.fore_color = code - 30;
          } else if (code >= 90 && code <= 97) {
            terminal->style.fore_color = code - 82;
          } else if (code == 38) {
            int type = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
            length = (ptr - buffer) + 1;
            
            int color = 0;
            
            if (type == 2) {
              int red = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
              
              int green = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
              
              int blue = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
              
              red = (red * 6) / 256;
              green = (green * 6) / 256;
              blue = (blue * 6) / 256;
              
              color = 16 + blue + green * 6 + red * 36;
            } else if (type == 5) {
              color = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
            }
            
            terminal->style.fore_color = color;
          } else if (code == 39) {
            terminal->style.fore_color = -1;
          }
          
          if (code >= 40 && code <= 47) {
            terminal->style.back_color = code - 40;
          } else if (code >= 100 && code <= 107) {
            terminal->style.back_color = code - 92;
          } else if (code == 48) {
            int type = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
            length = (ptr - buffer) + 1;
            
            int color = 0;
            
            if (type == 2) {
              int red = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
              
              int green = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
              
              int blue = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
              
              red = (red * 6) / 256;
              green = (green * 6) / 256;
              blue = (blue * 6) / 256;
              
              color = 16 + blue + green * 6 + red * 36;
            } else if (type == 5) {
              color = strtol((char *)(buffer) + length, (char **)(&ptr), 10);
              length = (ptr - buffer) + 1;
            }
            
            terminal->style.back_color = color;
          } else if (code == 49) {
            terminal->style.back_color = -1;
          }
          
          if (code == 1) {
            terminal->style.bold = 1;
          } else if (code == 21) {
            terminal->style.bold = 0;
          }
          
          if (code == 4) {
            terminal->style.underline = 1;
          } else if (code == 24) {
            terminal->style.underline = 0;
          }
          
          if (code == 7) {
            terminal->style.inverted = 1;
          } else if (code == 27) {
            terminal->style.inverted = 0;
          }
        }
      }
    } else {
      __bd_terminal_write(terminal, length, buffer);
    }
    
    if (terminal->cursor.x < 0) {
      terminal->cursor.x = 0;
    }
    
    if (terminal->cursor.x >= bd_width - 2) {
      terminal->cursor.x = bd_width - 3;
    }
    
    if (terminal->cursor.y < 0) {
      terminal->cursor.y = 0;
    }
    
    if (terminal->scroll_y <= terminal->cursor.y - (bd_height - 2)) {
      terminal->scroll_y = terminal->cursor.y - (bd_height - 3);
      terminal->dirty = 1;
    }
    
    if (terminal->count > bd_config.terminal_count) {
      int offset = terminal->count - bd_config.terminal_count;
      memmove(terminal->lines, terminal->lines + offset, bd_config.terminal_count * sizeof(bd_line_t));
      
      terminal->lines = realloc(terminal->lines, terminal->count = bd_config.terminal_count);
      terminal->cursor.y -= offset, terminal->scroll_y -= offset;
    }
    
    do_draw = 1;
  }
  
  return do_draw;
}

int bd_terminal_event(bd_view_t *view, io_event_t event) {
  bd_terminal_t *terminal = view->data;
  
  if (event.type == IO_EVENT_KEY_PRESS) {
    unsigned char buffer[4];
    int length = 1, echo = 0;
    
    buffer[0] = event.key & 0xFF;
    buffer[1] = (event.key >> 8) & 0xFF;
    buffer[2] = (event.key >> 16) & 0xFF;
    buffer[3] = (event.key >> 24) & 0x7F;
    
    if (buffer[0] >= 0xF0) {
      length = 4;
    } else if (buffer[0] >= 0xE0) {
      length = 3;
    } else if (buffer[0] >= 0xC0) {
      length = 2;
    }
    
    if ((event.key >= 32 && event.key < 128) || strchr("\b\n\t", event.key)) {
      io_fwrite(terminal->file, &event.key, 1);
      echo = 1;
    } else if (event.key & 0x80000000) {
      io_fwrite(terminal->file, buffer, length);
      echo = 1;
    } else if (IO_UNALT(event.key) < 32) {
      unsigned char key = IO_UNALT(event.key);
      io_fwrite(terminal->file, &key, 1);
    } else if (IO_EXTRA(event.key) == event.key) {
      io_tsend(terminal->file, event.key);
    }
    
    if (echo && io_techo(terminal->file)) {
      __bd_terminal_write(terminal, length, buffer);
    }
  } else if (event.type == IO_EVENT_RESIZE) {
    io_tresize(terminal->file, bd_width - 2, bd_height - 2);
  }
  
  return 0;
}

void bd_terminal_load(bd_view_t *view) {
  bd_terminal_t *terminal = (view->data = malloc(sizeof(bd_terminal_t)));
  strcpy(terminal->title, bd_config.shell_path);
  
  terminal->lines = calloc(sizeof(bd_line_t), 1);
  terminal->count = 1;
  
  terminal->cursor = (bd_cursor_t) {
    0, 0,
  };
  
  terminal->scroll_y = 0;
  terminal->scroll_mouse_y = -1;
  
  terminal->style = __bd_empty;
  terminal->file = io_topen(bd_config.shell_path);
  
  terminal->dirty = 0;
  
  io_tresize(terminal->file, bd_width - 2, bd_height - 2);
}

void bd_terminal_kill(bd_view_t *view) {
  bd_terminal_t *terminal = view->data;
  io_tclose(terminal->file);
}
