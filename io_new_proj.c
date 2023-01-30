#ifdef __new_proj__

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <dirent.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <io.h>

static struct termios old_termios;
static int mouse_down = 0;

void io_init(void) {
  tcgetattr(STDIN_FILENO, &old_termios);
  struct termios new_termios = old_termios;

  new_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  new_termios.c_oflag &= ~(OPOST);
  new_termios.c_cflag |= (CS8);
  new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  new_termios.c_cc[VMIN] = 0;
  new_termios.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
  
  setlocale(LC_ALL, "");
  printf(IO_CLEAR_SCREEN "\x1B[?1000;1002;1006;1015h");
  
  fflush(stdout);
}

void io_exit(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios);
  printf("\x1B[?25h\x1B[?1000;1002;1006;1015l" IO_CLEAR_SCREEN);
  
  fflush(stdout);
}

io_file_t io_fopen(const char *path, int write_mode) {
  return (io_file_t){
    .data = fopen(path, write_mode ? "wb+" : "rb"),
    .type = io_file_file,
  };
}

int io_fvalid(io_file_t file) {
  return !!file.data;
}

void io_fclose(io_file_t file) {
  fclose(file.data);
}

size_t io_fwrite(io_file_t file, void *buffer, size_t count) {
  return fwrite(buffer, 1, count, file.data);
}

size_t io_fread(io_file_t file, void *buffer, size_t count) {
  return fread(buffer, 1, count, file.data);
}

void io_frewind(io_file_t file) {
  rewind(file.data);
}

int io_feof(io_file_t file) {
  return feof(file.data);
}

void io_dsolve(const char *path, char *buffer) {
  realpath(path, buffer);
}

io_file_t io_dopen(const char *path) {
  return (io_file_t){
    .data = opendir(path),
    .type = io_file_directory,
  };
}

int io_dvalid(io_file_t file) {
  return !!file.data;
}

void io_dclose(io_file_t file) {
  closedir(file.data);
}

int io_dread(io_file_t file, char *buffer) {
  struct dirent *entry = readdir(file.data);
  if (!entry) return 0;
  
  strcpy(buffer, entry->d_name);
  return 1;
}

void io_drewind(io_file_t file) {
  rewinddir(file.data);
}

io_file_t io_copen(int write_mode) {
  if (write_mode) {
    return (io_file_t){
      .data = popen("xclip -selection clipboard -i", "w"),
      .type = io_file_clipboard,
    };
  } else {
    return (io_file_t){
      .data = popen("xclip -selection clipboard -o", "r"),
      .type = io_file_clipboard,
    };
  }
}

void io_cclose(io_file_t file) {
  pclose(file.data);
}

void io_cursor(int x, int y) {
  if (x < 0 || y < 0) {
    printf("\x1B[?25l");
  } else {
    printf("\x1B[%d;%dH\x1B[?25h", y + 1, x + 1);
  }
}

void io_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  vprintf(format, args);
  va_end(args);
}

int io_printf_wrap(int x, int width, int y, const char *format, ...) {
  char buffer[8192];
  
  va_list args;
  va_start(args, format);
  
  vsprintf(buffer, format, args);
  va_end(args);
  
  int width_left = width;
  io_cursor(x, y);
  
  char word[256];
  int length = 0;
  
  for (int i = 0;; i++) {
    if (!buffer[i] || isspace(buffer[i])) {
      word[length] = '\0';
      
      if (width_left < length) {
        io_cursor(x, ++y);
        width_left = width;
      }
      
      printf("%s", word);
      
      width_left -= length;
      length = 0;
      
      if (buffer[i] == '\n') {
        io_cursor(x, ++y);
        width_left = width;
      } else if (buffer[i]) {
        if (width_left) {
          putchar(' ');
          width_left--;
        }
      } else {
        break;
      }
    } else {
      word[length++] = buffer[i];
    }
  }
  
  return y;
}

void io_flush(void) {
  fflush(stdout);
}

io_event_t io_get_event(void) {
  static time_t old_time = 0;
  static int old_width = 0, old_height = 0;
  
  struct winsize ws;

  if (!(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)) {
    int width = ws.ws_col;
    int height = ws.ws_row;
    
    if (width != old_width || height != old_height) {
      old_width = width;
      old_height = height;
      
      return (io_event_t){
        .type = IO_EVENT_RESIZE,
        .size = {width, height},
      };
    }
  }
  
  time_t new_time = time(NULL);
  
  if (new_time != old_time) {
    old_time = new_time;
    
    return (io_event_t){
      .type = IO_EVENT_TIME_SECOND,
      .time = new_time,
    };
  }
  
  char ansi_buffer[256];
  char chr;
  
  if (read(STDIN_FILENO, &chr, 1) > 0) {
    int key = chr;
    
    if (chr == '\x1B') {
      read(STDIN_FILENO, ansi_buffer, 1);
      
      if (ansi_buffer[0] == '[') {
        read(STDIN_FILENO, ansi_buffer + 1, 1);
      }
      
      if (ansi_buffer[0] < 32) {
        key = IO_ALT(ansi_buffer[0]);
      } else if (ansi_buffer[1] == 'A') {
        key = IO_ARROW_UP;
      } else if (ansi_buffer[1] == 'B') {
        key = IO_ARROW_DOWN;
      } else if (ansi_buffer[1] == 'C') {
        key = IO_ARROW_RIGHT;
      } else if (ansi_buffer[1] == 'D') {
        key = IO_ARROW_LEFT;
      } else if (isdigit(ansi_buffer[1])) {
        read(STDIN_FILENO, ansi_buffer + 2, 1);
        
        if (ansi_buffer[2] == ';') {
          read(STDIN_FILENO, ansi_buffer + 3, 2);
          
          if (ansi_buffer[4] == 'A') {
            key = IO_ARROW_UP;
          } else if (ansi_buffer[4] == 'B') {
            key = IO_ARROW_DOWN;
          } else if (ansi_buffer[4] == 'C') {
            key = IO_ARROW_RIGHT;
          } else if (ansi_buffer[4] == 'D') {
            key = IO_ARROW_LEFT;
          }
          
          if (ansi_buffer[3] == '5' || ansi_buffer[3] == '6') {
            key = IO_CTRL(key);
          }
          
          if (ansi_buffer[3] == '2' || ansi_buffer[3] == '6' || ansi_buffer[3] == '4') {
            key = IO_SHIFT(key);
          }
          
          if (ansi_buffer[3] == '3' || ansi_buffer[3] == '4') {
            key = IO_ALT(key);
          }
        } else if (ansi_buffer[2] == '~') {
          if (ansi_buffer[1] == '3') {
            key = '\x7F';
          } else if (ansi_buffer[1] == '5') {
            key = IO_PAGE_UP;
          } else if (ansi_buffer[1] == '6') {
            key = IO_PAGE_DOWN;
          }
        }
      } else if (ansi_buffer[1] == 'H') {
        key = IO_HOME;
      } else if (ansi_buffer[1] == 'F') {
        key = IO_END;
      } else if (ansi_buffer[1] == 'Z') {
        key = IO_SHIFT('\t');
      } else if (ansi_buffer[1] == '<') {
        int mouse_type = 0, mouse_x = 0, mouse_y = 0;
        int offset = 2;
        
        for (;;) {
          read(STDIN_FILENO, ansi_buffer + offset++, 1);
          if (isalpha(ansi_buffer[offset - 1])) break;
        }
        
        int mouse_state = isupper(ansi_buffer[offset - 1]);
        offset = 2;
        
        while (isdigit(ansi_buffer[offset])) {
          mouse_type = (mouse_type * 10) + (ansi_buffer[offset] - '0');
          offset++;
        }
        
        mouse_type &= ~32;
        offset++;
        
        while (isdigit(ansi_buffer[offset])) {
          mouse_x = (mouse_x * 10) + (ansi_buffer[offset] - '0');
          offset++;
        }
        
        offset++;
        
        while (isdigit(ansi_buffer[offset])) {
          mouse_y = (mouse_y * 10) + (ansi_buffer[offset] - '0');
          offset++;
        }
        
        mouse_x--; mouse_y--;
        
        // 0 = left click, 1 = middle click, 2 = right click
        
        if (mouse_type == 0) {
          if (mouse_state) {
            io_event_t event = (io_event_t){
              .type = mouse_down ? IO_EVENT_MOUSE_MOVE : IO_EVENT_MOUSE_DOWN,
              .mouse = {.x = mouse_x, .y = mouse_y},
            };
            
            mouse_down = mouse_state;
            return event;
          } else {
            mouse_down = 0;
            
            return (io_event_t){
              .type = IO_EVENT_MOUSE_UP,
              .mouse = {.x = mouse_x, .y = mouse_y},
            };
          }
        } else if (mouse_type == 64) {
          return (io_event_t){
            .type = IO_EVENT_SCROLL,
            .scroll = -1,
          };
        } else if (mouse_type == 65) {
          return (io_event_t){
            .type = IO_EVENT_SCROLL,
            .scroll = 1,
          };
        }
      }
    } else if (chr == '\x7F') {
      key = IO_CTRL('H');
    }
    
    return (io_event_t){
      .type = IO_EVENT_KEY_PRESS,
      .key = key,
    };
  }
  
  return (io_event_t){
    .type = 0,
  };
}

#endif
