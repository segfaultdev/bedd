#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <bedd.h>
#include <io.h>

bd_view_t *bd_view_add(const char *title, int type, ...) {
  bd_views = realloc(bd_views, (bd_view_count + 1) * sizeof(bd_view_t));
  
  strcpy(bd_views[bd_view_count].title, title);
  bd_views[bd_view_count].title_dirty = 1;
  
  bd_views[bd_view_count].type = type;
  
  va_list args;
  va_start(args, type);
  
  if (type == bd_view_text) {
    bd_text_load(bd_views + bd_view_count, va_arg(args, const char *));
  } else if (type == bd_view_explore) {
    bd_explore_load(bd_views + bd_view_count, va_arg(args, const char *));
  } else if (type == bd_view_terminal) {
    bd_terminal_load(bd_views + bd_view_count);
  } else {
    bd_views[bd_view_count].data = NULL;
  }
  
  va_end(args);
  return bd_views + bd_view_count++;
}

void bd_view_remove(bd_view_t *view) {
  if (view->type == bd_view_text) {
    if (!bd_text_save(view, 1)) {
      return;
    }
  }
  
  if (view->data && view->type != bd_view_edit) {
    free(view->data);
  }
  
  int index = view - bd_views;
  
  for (int i = index; i < bd_view_count - 1; i++) {
    bd_views[i] = bd_views[i + 1];
  }
  
  bd_view_count--;
  
  if (!bd_view_count) {
    return;
  }
  
  bd_views = realloc(bd_views, bd_view_count * sizeof(bd_view_t));
  
  if (bd_view >= bd_view_count) {
    bd_view = bd_view_count - 1;
  }
}

void bd_view_draw(bd_view_t *view) {
  if (view->type == bd_view_welcome) {
    bd_welcome_draw(view);
  } else if (view->type == bd_view_edit) {
    bd_edit_draw(view);
  } else if (view->type == bd_view_text) {
    bd_text_draw(view);
  } else if (view->type == bd_view_explore) {
    bd_explore_draw(view);
  } else if (view->type == bd_view_terminal) {
    bd_terminal_draw(view);
  }
}

int bd_view_event(bd_view_t *view, io_event_t event) {
  if (view->type == bd_view_edit) {
    return bd_edit_event(view, event);
  } else if (view->type == bd_view_text) {
    return bd_text_event(view, event);
  } else if (view->type == bd_view_explore) {
    return bd_explore_event(view, event);
  } else if (view->type == bd_view_terminal) {
    return bd_terminal_event(view, event);
  }
  
  return 0;
}
