#include <stdlib.h>
#include <string.h>
#include <bedd.h>
#include <io.h>

typedef struct bd_entry_t bd_entry_t;
typedef struct bd_explore_t bd_explore_t;

struct bd_entry_t {
  char name[256];
  int is_directory;
  
  int selected;
};

struct bd_explore_t {
  char path[256];
  
  bd_entry_t *entries;
  int count;
  
  int cursor, scroll;
};

// static functions

static void __bd_explore_update(bd_explore_t *explore);
static void __bd_explore_follow(bd_explore_t *explore);

static void __bd_explore_update(bd_explore_t *explore) {
  if (explore->entries) {
    free(explore->entries);
    explore->entries = NULL;
  }
  
  explore->count = 0;
  
  io_file_t directory = io_dopen(explore->path);
  bd_entry_t entry;
  
  while (io_dread(directory, entry.name)) {
    if (!strcmp(entry.name, ".")) continue;
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

// public functions

void bd_explore_draw(bd_view_t *view) {
  bd_explore_t *explore = view->data;
  int view_height = bd_height - 2;
  
  for (int i = 0; i < view_height; i++) {
    int index = i + explore->scroll;
    io_cursor(0, i + 2);
    
    if (index < explore->count) {
      bd_entry_t entry = explore->entries[index];
      
      if (index == explore->cursor) {
        io_printf(IO_INVERT "  >  ");
      } else {
        io_printf(IO_SHADOW_1 "  -  ");
      }
      
      io_printf(IO_NORMAL " %s(%s) %s", entry.selected ? IO_SHADOW_1 : "", entry.is_directory ? "DIR." : "FILE", entry.name);
    } else {
      io_printf(IO_NORMAL "    :");
    }
    
    io_printf(IO_NORMAL IO_CLEAR_LINE);
  }
  
  io_flush();
  
  strcpy(view->title, explore->path);
  
  view->cursor = (bd_cursor_t){-1, -1};
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
        if (explore->entries[i].selected) select_count++;
      }
      
      for (int i = 0; i < explore->count; i++) {
        if (!strcmp(explore->entries[i].name, "..")) continue;
        explore->entries[i].selected = (select_count < explore->count - 1);
      }
      
      __bd_explore_follow(explore);
      return 1;
    } else if (IO_UNALT(event.key) == IO_CTRL('M')) {
      bd_entry_t entry = explore->entries[explore->cursor];
      
      char new_path[256];
      strcpy(new_path, explore->path);
      
      int length = strlen(new_path);
      
      if (!strcmp(entry.name, "..")) {
        if (!strcmp(new_path, ".")) {
          io_dsolve(explore->path, new_path);
        }
        
        char *ptr = strrchr(new_path, '/');
        
        if (!ptr) return 0;
        if (ptr == new_path) ptr++;
        
        length = ptr - new_path;
        new_path[length] = '\0';
      } else {
        if (new_path[length - 1] != '/') {
          new_path[length++] = '/';
        }
        
        strcpy(new_path + length, entry.name);
      }
      
      if (entry.is_directory && event.key == IO_UNALT(event.key)) {
        strcpy(explore->path, new_path);
        __bd_explore_update(explore);
        
        __bd_explore_follow(explore);
        return 1;
      } else {
        bd_view = bd_view_add(new_path, entry.is_directory ? bd_view_explore : bd_view_text, new_path) - bd_views;
        return 0;
      }
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
    }
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
  __bd_explore_update(explore);
}
