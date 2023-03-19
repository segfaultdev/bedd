#include <stdlib.h>
#include <string.h>
#include <bedd.h>
#include <io.h>

typedef struct bd_entry_t bd_entry_t;
typedef struct bd_explore_t bd_explore_t;

struct bd_entry_t {
  char name[256];
  
  int is_directory;
  time_t mtime;
  
  int selected;
};

struct bd_explore_t {
  char path[256];
  
  bd_entry_t *entries;
  int count;
  
  int cursor, scroll;
  int scroll_mouse_y;
};

// Static functions

static void __bd_explore_update(bd_explore_t *explore);
static void __bd_explore_follow(bd_explore_t *explore);
static int  __bd_explore_enter(bd_view_t *view, int new_tab);

static void __bd_explore_update(bd_explore_t *explore) {
  if (explore->entries) {
    free(explore->entries);
    explore->entries = NULL;
  }
  
  explore->count = 0;
  
  io_file_t directory = io_dopen(explore->path);
  bd_entry_t entry;
  
  while (io_dread(directory, entry.name)) {
    if (!strcmp(entry.name, ".")) {
      continue;
    }
    
    char merge_path[256];
    
    strcpy(merge_path, explore->path);
    int length = strlen(merge_path);
    
    if (merge_path[length - 1] != '/') {
      merge_path[length++] = '/';
    }
    
    strcpy(merge_path + length, entry.name);
    io_file_t test = io_dopen(merge_path);
    
    if ((entry.is_directory = io_dvalid(test))) {
      io_dclose(test);
    }
    
    entry.mtime = io_ftime(merge_path);
    entry.selected = 0;
    
    explore->entries = realloc(explore->entries, (explore->count + 1) * sizeof(bd_entry_t));
    explore->entries[explore->count++] = entry;
  }
  
  for (int i = 0; i < explore->count; i++) {
    for (int j = i + 1; j < explore->count; j++) {
      if (strcmp(explore->entries[i].name, explore->entries[j].name) > 0) {
        bd_entry_t entry = explore->entries[i];
        
        explore->entries[i] = explore->entries[j];
        explore->entries[j] = entry;
      }
    }
  }
}

static void __bd_explore_follow(bd_explore_t *explore) {
  if (explore->cursor < 0) {
    explore->cursor = 0;
  }
  
  if (explore->cursor >= explore->count) {
    explore->cursor = explore->count - 1;
  }
  
  int view_height = bd_height - 2;
  
  if (explore->scroll > explore->cursor) {
    explore->scroll = explore->cursor;
  } else if (explore->scroll <= explore->cursor - view_height) {
    explore->scroll = (explore->cursor - view_height) + 1;
  }
}

static int __bd_explore_enter(bd_view_t *view, int new_tab) {
  bd_explore_t *explore = view->data;
  
  bd_entry_t entry = explore->entries[explore->cursor];
  
  char new_path[256];
  strcpy(new_path, explore->path);
  
  int length = strlen(new_path);
  
  if (!strcmp(entry.name, "..")) {
    if (!strcmp(new_path, ".")) {
      io_dsolve(explore->path, new_path);
    }
    
    char *ptr = strrchr(new_path, '/');
    
    if (!ptr) {
      return 0;
    }
    
    if (ptr == new_path) {
      ptr++;
    }
    
    length = ptr - new_path;
    new_path[length] = '\0';
  } else {
    if (new_path[length - 1] != '/') {
      new_path[length++] = '/';
    }
    
    strcpy(new_path + length, entry.name);
  }
  
  if (entry.is_directory && !new_tab) {
    strcpy(explore->path, new_path);
    __bd_explore_update(explore);
    
    __bd_explore_follow(explore);
    explore->cursor = 0;
    
    return 1;
  } else {
    bd_open(new_path);
    
    bd_view = bd_view_count - 1;
    return 0;
  }
}

// Public functions

void bd_explore_draw(bd_view_t *view) {
  bd_explore_t *explore = view->data;
  int view_height = bd_height - 2;
  
  for (int i = 0; i < view_height; i++) {
    int index = i + explore->scroll;
    io_cursor(0, i + 2);
    
    if (index < explore->count) {
      bd_entry_t entry = explore->entries[index];
      struct tm *tm_time = localtime(&entry.mtime);
      
      char buffer[100];
      strftime(buffer, 100, "%d %b %Y, %H:%M:%S", tm_time);
      
      if (index == explore->cursor) {
        io_printf(IO_INVERT "  >  ");
      } else {
        io_printf(IO_SHADOW_1 "  -  ");
      }
      
      if (entry.is_directory) {
        io_printf(IO_NORMAL " %s(%s) " IO_BOLD_GREEN "%s/", entry.selected ? IO_SHADOW_1 : "", buffer, entry.name);
      } else {
        io_printf(IO_NORMAL " %s(%s) %s%s", entry.selected ? IO_SHADOW_1 : "", buffer, entry.name[0] == '.' ? IO_BOLD_BLUE : IO_BOLD_WHITE, entry.name);
      }
    } else {
      io_printf(IO_NORMAL "    :");
    }
    
    io_printf(IO_CLEAR_LINE IO_NORMAL);
    io_cursor(bd_width - 1, i + 2);
    
    int scroll_start_y = (explore->scroll * (bd_height - 2)) / (explore->count + (bd_height - 2));
    int scroll_end_y = 1 + ((explore->scroll + (bd_height - 2)) * (bd_height - 2)) / (explore->count + (bd_height - 2));
    
    if (i >= scroll_start_y && i < scroll_end_y) {
      io_printf(IO_SHADOW_2 " ");
    } else {
      io_printf(IO_NORMAL "\u2502");
    }
    
    io_printf(IO_NORMAL);
  }
  
  strcpy(view->title, explore->path);
  
  view->cursor = (bd_cursor_t) {
    -1, -1,
  };
}

int bd_explore_event(bd_view_t *view, io_event_t event) {
  bd_explore_t *explore = view->data;
  
  if (event.type == IO_EVENT_KEY_PRESS) {
    if (event.key == IO_CTRL('U')) {
      explore->cursor = 0;
      __bd_explore_update(explore);
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == IO_CTRL('A')) {
      int select_count = 0;
      
      for (int i = 0; i < explore->count; i++) {
        if (explore->entries[i].selected) {
          select_count++;
        }
      }
      
      for (int i = 0; i < explore->count; i++) {
        if (!strcmp(explore->entries[i].name, "..")) {
          continue;
        }
        
        explore->entries[i].selected = (select_count < explore->count - 1);
      }
      
      __bd_explore_follow(explore);
      return 1;
    } else if (IO_UNALT(event.key) == IO_CTRL('M')) {
      return __bd_explore_enter(view, event.key != IO_UNALT(event.key));
    } else if (event.key == IO_ARROW_UP) {
      explore->cursor--;
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == IO_ARROW_DOWN) {
      explore->cursor++;
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == IO_CTRL(IO_ARROW_UP)) {
      explore->cursor = 0;
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == IO_CTRL(IO_ARROW_DOWN)) {
      explore->cursor = explore->count - 1;
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == IO_PAGE_UP) {
      explore->cursor -= (bd_height - 2) / 2;
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == IO_PAGE_DOWN) {
      explore->cursor += (bd_height - 2) / 2;
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == ' ') {
      if (strcmp(explore->entries[explore->cursor].name, "..")) {
        explore->entries[explore->cursor].selected = !explore->entries[explore->cursor].selected;
      }
      
      __bd_explore_follow(explore);
      return 1;
    } else if (event.key == IO_CTRL('C') || event.key == IO_CTRL('X')) {
      io_file_t clip = io_copen(1);
      
      char buffer[8192];
      strcpy(buffer, (event.key == IO_CTRL('C')) ? "Copy" : "Move");
      
      for (int i = 0; i < explore->count; i++) {
        if (explore->entries[i].selected) {
          char merge_path[256];
          strcat(buffer, " ");
          
          strcpy(merge_path, explore->path);
          int length = strlen(merge_path);
          
          if (merge_path[length - 1] != '/') {
            merge_path[length++] = '/';
          }
          
          strcpy(merge_path + length, explore->entries[i].name);
          strcat(buffer, merge_path);
        }
      }
      
      io_fwrite(clip, buffer, strlen(buffer));
      io_cclose(clip);
      
      return 1;
    } else if (event.key == IO_CTRL('V')) {
      io_file_t clip = io_copen(0);
      
      char buffer[8192];
      int length = 0;
      
      while (!io_feof(clip)) {
        if (io_fread(clip, buffer + length, 1) <= 0) {
          break;
        }
        
        length++;
      }
      
      io_cclose(clip);
      
      // TODO: Copy and cut
      return 1;
    }
  } else if (event.type == IO_EVENT_MOUSE_DOWN || event.type == IO_EVENT_MOUSE_MOVE) {
    if (explore->scroll_mouse_y >= 0) {
      explore->scroll = ((event.mouse.y - (2 + explore->scroll_mouse_y)) * (explore->count + (bd_height - 2))) / (bd_height - 2);
      
      if (explore->scroll < 0) {
        explore->scroll = 0;
      } else if (explore->scroll >= explore->count) {
        explore->scroll = explore->count - 1;
      }
    } else if (event.type == IO_EVENT_MOUSE_DOWN && event.mouse.x >= bd_width - 1) {
      int scroll_start_y = 2 + (explore->scroll * (bd_height - 2)) / (explore->count + (bd_height - 2));
      int scroll_end_y = 3 + ((explore->scroll + (bd_height - 2)) * (bd_height - 2)) / (explore->count + (bd_height - 2));
      
      if (event.mouse.y >= scroll_start_y && event.mouse.y < scroll_end_y) {
        explore->scroll_mouse_y = event.mouse.y - scroll_start_y;
      } else {
        explore->scroll_mouse_y = (scroll_end_y - scroll_start_y) / 2;
        explore->scroll = ((event.mouse.y - (2 + explore->scroll_mouse_y)) * (explore->count + (bd_height - 2))) / (bd_height - 2);
        
        if (explore->scroll < 0) {
          explore->scroll = 0;
        } else if (explore->scroll >= explore->count) {
          explore->scroll = explore->count - 1;
        }
      }
    } else if (event.type == IO_EVENT_MOUSE_DOWN) {
      if (explore->cursor == explore->scroll + (event.mouse.y - 2)) {
        return __bd_explore_enter(view, 0);
      }
      
      explore->cursor = explore->scroll + (event.mouse.y - 2);
      __bd_explore_follow(explore);
    }
    
    return 1;
  } else if (event.type == IO_EVENT_MOUSE_UP) {
    explore->scroll_mouse_y = -1;
  } else if (event.type == IO_EVENT_SCROLL) {
    explore->scroll += event.scroll * bd_config.scroll_step;
    
    if (explore->scroll < 0) {
      explore->scroll = 0;
    } else if (explore->scroll >= explore->count) {
      explore->scroll = explore->count - 1;
    }
    
    return 1;
  }
  
  return 0;
}

void bd_explore_load(bd_view_t *view, const char *path) {
  bd_explore_t *explore = malloc(sizeof(bd_explore_t));
  view->data = explore;
  
  strcpy(explore->path, path);
  explore->entries = NULL;
  
  int length = strlen(explore->path);
  
  if (explore->path[length - 1] == '/') {
    explore->path[length - 1] = '\0';
  }
  
  explore->cursor = explore->scroll = 0;
  explore->scroll_mouse_y = -1;
  
  __bd_explore_update(explore);
}
