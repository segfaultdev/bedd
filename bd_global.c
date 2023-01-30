#include <string.h>
#include <bedd.h>
#include <time.h>
#include <io.h>

void bd_global_draw(void) {
  io_cursor(0, 0);
  io_printf(IO_INVERT " bedd r02 " IO_SHADOW_2 " ");
  
  struct tm *tm_time = localtime(&bd_time);
  
  char buffer[100];
  strftime(buffer, 100, " %d %b %Y, %H:%M:%S ", tm_time);
  
  for (int i = 0; i < bd_width - (12 + (int)(strlen(buffer))); i++) {
    io_printf("\u2550");
  }
  
  io_printf(" " IO_INVERT "%s" IO_SHADOW_1, buffer);
  io_cursor(0, 1);
  
  for (int i = 0; i < bd_view_count; i++) {
    int width = ((bd_width * (i + 1)) / bd_view_count) - ((bd_width * i) / bd_view_count);
    
    if (i == bd_view) {
      io_printf(IO_INVERT);
    }
    
    io_printf("\u2576");
    width -= 4;
    
    char title[256];
    strcpy(title, bd_views[i].title);
    
    int length = strlen(title);
    
    if (length > width) {
      memset(title + (width - 3), '.', 3);
      title[width] = '\0';
      
      width = 0;
    } else {
      width -= length;
    }
    
    io_printf("%s", title);
    
    while (width--) {
      io_printf("\u2500");
    }
    
    io_printf("\u2500\u00D7\u2574");
    
    if (i == bd_view) {
      io_printf(IO_SHADOW_1);
    }
  }
  
  io_printf(IO_NORMAL);
  io_cursor(0, 2);
}

int bd_global_click(int x, int y) {
  if (y < 1) return 0;
  int start_x = 0;
  
  for (int i = 0; i < bd_view_count; i++) {
    int width = ((bd_width * (i + 1)) / bd_view_count) - ((bd_width * i) / bd_view_count);
    int end_x = start_x + width;
    
    if (x >= start_x && x < end_x) {
      // TODO: X close button
      
      if (i == bd_view) {
        return 0;
      } else {
        bd_view = i;
        return 1;
      }
    }
    
    start_x = end_x;
  }
  
  return 0;
}
