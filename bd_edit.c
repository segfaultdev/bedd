#include <string.h>
#include <theme.h>
#include <bedd.h>
#include <io.h>

static const int __edit_limits[] = {
  1, 16,
  0, 1,
  0, 1,
  1, 32,
  1, 32,
  0, 40,
  16, 256,
  4, 128,
  0, theme_count - 1,
  0, 1,
};

static const int __edit_count = 10;

void bd_edit_draw(bd_view_t *view) {
  io_cursor(0, 2);
  io_printf(IO_CLEAR_CURSOR);
  
  const char *title = "Edit configuration (Ctrl+S: Save, [\u2191\u2193]: Move, [\u2190\u2192]: Change)";
  
  io_cursor((bd_width - strlen(title)) / 2, 3);
  io_printf(IO_UNDERLINE "%s" IO_NORMAL, title);
  
  int index = (int)(view->data);
  
  io_cursor(2, 5);
  io_printf(IO_BOLD "%sTab width (1-16):" IO_NORMAL " %d", (index == 0 ? IO_INVERT : ""), bd_config.indent_width);
  
  io_cursor(2, 6);
  io_printf(IO_BOLD "%sTabs or spaces:" IO_NORMAL " %s", (index == 1 ? IO_INVERT : ""), bd_config.indent_spaces ? "Spaces" : "Tabs");
  
  io_cursor(2, 7);
  io_printf(IO_BOLD "%sShow guides:" IO_NORMAL " %s", (index == 2 ? IO_INVERT : ""), bd_config.indent_guides ? "Yes" : "No");
  
  io_cursor(2, 9);
  io_printf(IO_BOLD "%sScroll step (with mouse wheel, default 2):" IO_NORMAL " %d", (index == 3 ? IO_INVERT : ""), bd_config.scroll_step);
  
  io_cursor(2, 10);
  io_printf(IO_BOLD "%sScroll image step (with Shift+arrows, default 4):" IO_NORMAL " %d", (index == 4 ? IO_INVERT : ""), bd_config.scroll_image_step);
  
  io_cursor(2, 11);
  io_printf(IO_BOLD "%sHorizontal scroll margin (default 0):" IO_NORMAL " %d", (index == 5 ? IO_INVERT : ""), bd_config.scroll_width_margin);
  
  io_cursor(2, 13);
  io_printf(IO_BOLD "%sCharacters per undo entry (default 48):" IO_NORMAL " %d", (index == 6 ? IO_INVERT : ""), bd_config.undo_edit_count);
  
  io_cursor(2, 14);
  io_printf(IO_BOLD "%sMaximum undo entries (default 64):" IO_NORMAL " %d", (index == 7 ? IO_INVERT : ""), bd_config.undo_depth);
  
  io_cursor(2, 16);
  io_printf(IO_BOLD "%sTheme:" IO_NORMAL " \"%s\" (%d/%d)", (index == 8 ? IO_INVERT : ""), theme_names[bd_config.theme], bd_config.theme + 1, theme_count);
  
  io_cursor(2, 17);
  io_printf(IO_BOLD "%sxterm 256-color mode:" IO_NORMAL " %s", (index == 9 ? IO_INVERT : ""), bd_config.xterm_colors ? "Yes" : "No");
  
  io_flush();
  
  view->cursor = (bd_cursor_t) {
    -1, -1
    };
}

int bd_edit_event(bd_view_t *view, io_event_t event) {
  if (event.type == IO_EVENT_KEY_PRESS) {
    if (event.key == IO_ARROW_UP) {
      view->data = (void *)(((int)(view->data) + (__edit_count - 1)) % __edit_count);
      return 1;
    } else if (event.key == IO_ARROW_DOWN) {
      view->data = (void *)(((int)(view->data) + 1) % __edit_count);
      return 1;
    } else if (event.key == IO_ARROW_LEFT) {
      int index = (int)(view->data);
      
      bd_config.raw_data[index] -= __edit_limits[index * 2];
      bd_config.raw_data[index] += __edit_limits[index * 2 + 1] - __edit_limits[index * 2];
      bd_config.raw_data[index] %= (__edit_limits[index * 2 + 1] - __edit_limits[index * 2]) + 1;
      bd_config.raw_data[index] += __edit_limits[index * 2];
      
      strcpy(view->title, "Edit configuration*");
      return 1;
    } else if (event.key == IO_ARROW_RIGHT) {
      int index = (int)(view->data);
      
      bd_config.raw_data[index] -= __edit_limits[index * 2];
      bd_config.raw_data[index]++;
      bd_config.raw_data[index] %= (__edit_limits[index * 2 + 1] - __edit_limits[index * 2]) + 1;
      bd_config.raw_data[index] += __edit_limits[index * 2];
      
      strcpy(view->title, "Edit configuration*");
      return 1;
    } else if (event.key == IO_CTRL('S')) {
      io_file_t config = io_fopen(io_config, 1);
      
      if (io_fvalid(config)) {
        io_fwrite(config, (void *)(&bd_config), sizeof(bd_config_t));
        io_fclose(config);
        
        strcpy(view->title, "Edit configuration");
      }
      
      return 1;
    }
  }
  
  return 0;
}
