#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <bedd.h>
#include <io.h>

int bd_dialog(const char *title, int __width, const char *format, ...) {
  int dialog_draw = 1;
  int height = 2;
  
  char **select_buffers = NULL;
  int *select_indexes = NULL;
  
  int select_count = 0, button_count = 0;
  
  va_list args;
  va_start(args, format);
  
  const char *ptr = format;
  
  while (*ptr) {
    char chr = *ptr;
    
    if (chr == 'i') {
      while (*(ptr++) != ']');
      
      select_buffers = realloc(select_buffers, (select_count + 1) * sizeof(char **));
      select_indexes = realloc(select_indexes, (select_count + 1) * sizeof(int));
      
      select_buffers[select_count] = va_arg(args, char *);
      select_indexes[select_count] = 0;
      
      select_count++;
      height++;
    } else if (chr == 'b') {
      int count = 0;
      ptr += 2;
      
      while (isdigit(*ptr)) {
        count = (count * 10) + (*(ptr++) - '0');
      }
      
      while (*(ptr++) != ']');
      
      select_buffers = realloc(select_buffers, (select_count + count) * sizeof(char **));
      select_indexes = realloc(select_indexes, (select_count + count) * sizeof(int));
      
      while (count--) {
        select_buffers[select_count] = NULL;
        select_indexes[select_count] = ++button_count;
        
        select_count++;
      }
      
      height++;
    } else if (isdigit(chr)) {
      height += chr - '0';
    }
    
    height++;
  }
  
  va_end(args);
  
  int select_index = 0;  // index of selected element
  int select_offset = 0; // offset in text input boxes
  
  if (select_buffers[select_index]) {
    select_offset = strlen(select_buffers[select_index]);
  }
  
  for (;;) {
    if (dialog_draw) {
      int width = __width;
      
      if (__width > bd_width) {
        width = bd_width;
      } else if (__width <= 0) {
        width = bd_width + __width;
      }
      
      int start_x = (bd_width - width) / 2;
      int cursor_x = -1, cursor_y = -1;
      
      bd_global_draw();
      bd_view_draw(bd_views + bd_view);
      
      int start_y = (bd_height - height) / 2;
      int real_height = 0;
      
      int index = 0;
      
      io_cursor(start_x, start_y + real_height++);
      io_printf(IO_INVERT " %s " IO_SHADOW_2 " ", title);
      
      for (int i = 0; i < width - ((int)(strlen(title)) + 4); i++) {
        io_printf("\u2550");
      }
      
      io_printf(" " IO_NORMAL);
      
      io_cursor(start_x, start_y + real_height++);
      io_printf(IO_SHADOW_1 "%*c" IO_NORMAL, width, ' ');
      
      ptr = format;
      
      while (*ptr) {
        io_cursor(start_x, start_y + real_height++);
        
        char chr = *(ptr++);
        ptr++;
        
        char buffer[256];
        int length = 0;
        
        while (*ptr != ']') {
          buffer[length++] = *(ptr++);
        }
        
        buffer[length] = '\0';
        ptr++;
        
        if (chr == 'i') {
          const char *format_str;
          
          if (index == select_index) {
            format_str = IO_BOLD_SHADOW_1 "  %s " IO_NORMAL "%-*s" IO_SHADOW_1 "  " IO_NORMAL;
          } else {
            format_str = IO_SHADOW_1 "  %s " IO_NORMAL "%-*s" IO_SHADOW_1 "  " IO_NORMAL;
          }
          
          io_printf(format_str, buffer, width - (5 + strlen(buffer)), select_buffers[index]);
          
          if (index == select_index) {
            cursor_x = start_x + 3 + strlen(buffer) + select_offset;
            cursor_y = start_y + (real_height - 1);
          }
          
          index++;
        } else if (chr == 'b') {
          int total_width = 1;
          
          io_printf(IO_SHADOW_1 " ");
          int offset = 0, count = 0;
          
          while (isdigit(buffer[offset])) {
            count = (count * 10) + (buffer[offset++] - '0');
          }
          
          offset++; // skip semicolon
          
          for (int i = 0; i < count; i++) {
            char buffer_2[64];
            int length_2 = 0;
            
            while (buffer[offset] && buffer[offset] != ';') {
              buffer_2[length_2++] = buffer[offset++];
            }
            
            buffer_2[length_2] = '\0';
            offset++; // skip semicolon or bracket
            
            io_printf(IO_WHITE);
            
            if (index == select_index) {
              io_printf(" " IO_INVERT " %s " IO_SHADOW_1, buffer_2);
            } else {
              io_printf(" " IO_SHADOW_2 " %s " IO_SHADOW_1, buffer_2);
            }
            
            total_width += length_2 + 3;
            index++;
          }
          
          io_printf("%*c" IO_NORMAL, width - total_width, ' ');
        } else if (isdigit(chr)) {
          int line_count = chr - '0';
          real_height--;
          
          for (int i = 0; i < line_count; i++) {
            io_cursor(start_x, start_y + real_height++);
            io_printf(IO_SHADOW_1 "%*c" IO_NORMAL, width, ' ');
          }
          
          io_printf(IO_SHADOW_1);
          io_printf_wrap(start_x + 2, width - 4, (start_y + real_height) - line_count, buffer);
        } else {
          io_printf(IO_SHADOW_1 "%*c" IO_NORMAL, width, chr);
        }
        
        io_cursor(start_x, start_y + real_height++);
        io_printf(IO_SHADOW_1 "%*c" IO_NORMAL, width, ' ');
      }
      
      dialog_draw = 0;
      
      io_cursor(cursor_x, cursor_y);
      io_flush();
    }
    
    io_event_t event;
    
    if ((event = io_get_event()).type) {
      if (event.type == IO_EVENT_TIME_SECOND) {
        bd_time = event.time;
      } else if (event.type == IO_EVENT_RESIZE) {
        bd_width = event.size.width;
        bd_height = event.size.height;
        
        dialog_draw = 1;
      } else if (event.type == IO_EVENT_KEY_PRESS) {
        if (event.key >= 32 && event.key < 127 && select_buffers[select_index]) {
          int length = strlen(select_buffers[select_index]);
          
          memmove(select_buffers[select_index] + select_offset + 1, select_buffers[select_index] + select_offset, (length - select_offset) + 1);
          select_buffers[select_index][select_offset++] = event.key;
          
          dialog_draw = 1;
        } else if (event.key == IO_CTRL('H') || event.key == '\x7F') {
          int length = strlen(select_buffers[select_index]);
          
          if (event.key == IO_CTRL('H')) {
            select_offset--;
            
            if (select_offset < 0) {
              select_offset = 0;
            }
          }
          
          if (length - select_offset) {
            memmove(select_buffers[select_index] + select_offset, select_buffers[select_index] + select_offset + 1, length - select_offset);
          }
          
          dialog_draw = 1;
        } else if (event.key == IO_CTRL('Q')) {
          return 0;
        } else if (event.key == IO_CTRL('M')) {
          if (select_buffers[select_index]) {
            select_index++;
            
            if (select_index == select_count) {
              return 1;
            } else {
              dialog_draw = 1;
            }
          } else {
            return select_indexes[select_index];
          }
        } else if (event.key == IO_ARROW_LEFT && select_buffers[select_index] && select_offset > 0) {
          select_offset--;
          dialog_draw = 1;
        } else if (event.key == IO_ARROW_RIGHT && select_buffers[select_index] && select_offset < (int)(strlen(select_buffers[select_index]))) {
          select_offset++;
          dialog_draw = 1;
        } else if (event.key == IO_ARROW_UP || event.key == IO_ARROW_LEFT) {
          select_index = (select_index + (select_count - 1)) % select_count;
          
          if (select_buffers[select_index]) {
            select_offset = strlen(select_buffers[select_index]);
          } else {
            select_offset = 0;
          }
          
          dialog_draw = 1;
        } else if (event.key == IO_ARROW_DOWN || event.key == IO_ARROW_RIGHT) {
          select_index = (select_index + 1) % select_count;
          
          if (select_buffers[select_index]) {
            select_offset = strlen(select_buffers[select_index]);
          } else {
            select_offset = 0;
          }
          
          dialog_draw = 1;
        }
      }
    }
  }
}
