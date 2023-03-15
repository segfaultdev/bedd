#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR
#define STBI_NO_STDIO
#define STBI_NO_HDR

#include <stb_image.h>
#include <stdint.h>
#include <bedd.h>

typedef struct bd_image_t bd_image_t;

struct bd_image_t {
  char path[256];
  
  uint32_t *data;
  int width, height;
  
  int scale;
  
  int scroll_x, scroll_y;
  int scroll_mouse_x, scroll_mouse_y; // Only one means scroll bar, both mean drag
};

static int __bd_image_rgb_to_index(int r, int g, int b);

static int __bd_image_rgb_to_index(int r, int g, int b) {
  if (bd_config.xterm_colors) {
    r = (r * 6) / 256;
    g = (g * 6) / 256;
    b = (b * 6) / 256;
    
    return 16 + b + g * 6 + r * 36;
  }
  
  int i = (r >= 85 && g >= 85 && b >= 85);
  
  r /= 128;
  g /= 128;
  b /= 128;
  
  return r + g * 2 + b * 4 + i * 8;
}

void bd_image_draw(bd_view_t *view) {
  bd_image_t *image = (bd_image_t *)(view->data);
  int view_width = (image->width * image->scale) / 10000;
  
  if (image->scale < 0) {
    view_width = bd_width - 1;
  }
  
  int start_x = (image->scroll_x < 0 ? 0 : image->scroll_x);
  int start_y = (image->scroll_y < 0 ? 0 : image->scroll_y);
  
  for (int i = 0; i < bd_height - 3; i++) {
    io_cursor(0, i + 2);
    io_printf(IO_NORMAL);
    
    int y = (i - image->scroll_y) * 2;
    
    int y_0 = (y * image->width) / view_width;
    int y_1 = ((y + 1) * image->width) / view_width;
    
    if (y_0 >= 0 && y_0 < image->height) {
      for (int j = 0; j < bd_width - 1; j++) {
        int x = ((j - image->scroll_x) * image->width) / view_width;
        
        if (x < 0) {
          io_printf(" ");
          continue;
        }
        
        if (x >= image->width) {
          break;
        }
        
        uint32_t rgb_0 = image->data[x + y_0 * image->width];
        
        if (!(rgb_0 >> 24)) {
          rgb_0 = (j % 2) ? 0xFF3F3F3F : 0xFF6F6F6F;
        }
        
        int color_0 = __bd_image_rgb_to_index((rgb_0 >> 0) & 0xFF, (rgb_0 >> 8) & 0xFF, (rgb_0 >> 16) & 0xFF);
        
        if (y_1 < image->height) {
          uint32_t rgb_1 = image->data[x + y_1 * image->width];
          
          if (!(rgb_1 >> 24)) {
            rgb_1 = ((j + 1) % 2) ? 0xFF3F3F3F : 0xFF6F6F6F;
          }
          
          int color_1 = __bd_image_rgb_to_index((rgb_1 >> 0) & 0xFF, (rgb_1 >> 8) & 0xFF, (rgb_1 >> 16) & 0xFF);
          
          io_printf(IO_XTERM_BACK("%d"), color_1);
        } else {
          io_printf(IO_NORMAL);
        }
        
        io_printf(IO_XTERM_FORE("%d") "\u2580", color_0);
      }
    }
    
    io_printf(IO_NORMAL IO_CLEAR_CURSOR);
  }
}

int bd_image_event(bd_view_t *view, io_event_t event) {
  bd_image_t *image = (bd_image_t *)(view->data);
  
  if (event.type == IO_EVENT_KEY_PRESS) {
    if (event.key == IO_CTRL('U')) {
      image->scale = -1;
      return 1;
    } else if (event.key == '+') {
      if (image->scale < 0) {
        image->scale = ((bd_width - 1) * 10000) / image->width;
      }
      
      if (image->scale < 20000) {
        image->scale += 1000;
      }
      
      return 1;
    } else if (event.key == '-') {
      if (image->scale < 0) {
        image->scale = ((bd_width - 1) * 10000) / image->width;
      }
      
      if (image->scale > 1000) {
        image->scale -= 1000;
      }
      
      return 1;
    }
  }
  
  return 0;
}

void bd_image_load(bd_view_t *view, const char *path) {
  bd_image_t *image = (bd_image_t *)(view->data = malloc(sizeof(bd_image_t)));
  
  strcpy(image->path, path);
  strcpy(view->title, path);
  
  image->scroll_x = 0;
  image->scroll_y = 0;
  
  io_file_t file = io_fopen(path, 0);
  
  if (!io_fvalid(file)) {
    return;
  }
  
  void *data = NULL;
  size_t used = 0, size = 0;
  
  while (!io_feof(file)) {
    if (used >= size) {
      size += 4096;
      data = realloc(data, size);
    }
    
    if (io_fread(file, data + used, 1) <= 0) {
      break;
    }
    
    used++;
  }
  
  io_fclose(file);
  int bpp;
  
  image->data = (uint32_t *)(stbi_load_from_memory(data, used, &(image->width), &(image->height), &bpp, 4));
  image->scale = -1;
  
  free(data);
  
  if (bpp != 4) {
    return;
  }
}
