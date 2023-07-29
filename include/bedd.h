#ifndef __BEDD_H__
#define __BEDD_H__

#include <syntax.h>
#include <time.h>
#include <io.h>

typedef struct bd_config_t bd_config_t;
typedef struct bd_cursor_t bd_cursor_t;
typedef struct bd_view_t bd_view_t;

struct bd_config_t {
  union {
    struct {
      int indent_width;        // Anything other than 2 or 4 would be cringe af, but we must still provide room for customization and blah blah blah :p.
      int indent_spaces;       // Afects saving/loading, as in editing they're still shown as spaces no matter what.
      int indent_guides;       // Show indentation guides.
      int scroll_step;         // Lines to move per scroll step.
      int scroll_image_step;   // Blocks to move per image scroll step.
      int scroll_width_margin; // When scrolling sideways, how much margin to left to the right.
      int undo_edit_count;     // Character insertions/deletions per undo save.
      int undo_depth;          // Maximum amount of undoable steps.
      int theme;               // Selected theme.
      int xterm_colors;        // Enable xterm-like 256-color mode.
      int terminal_count;      // Maximum terminal line count.
      int column_guide;        // Distance to show the guide at (or 0 if disabled).
      int column_tiny;         // Enables smaller UI, designed for terminals with less columns (makes tabs smaller by path reduction, and reduces line number margins).
    };
    
    int raw_data[13];
  };
  
  char shell_path[256];
  char syntax_colors[64][st_color_count];
};

struct bd_cursor_t {
  int x, y;
};

#define BD_CURSOR_MIN(a, b)   ((a).y < (b).y ? (a) : ((a).y == (b).y ? ((a).x < (b).x ? (a) : (b)) : (b)))
#define BD_CURSOR_MAX(a, b)   ((a).y > (b).y ? (a) : ((a).y == (b).y ? ((a).x > (b).x ? (a) : (b)) : (b)))
#define BD_CURSOR_LESS(a, b)  ((a).y < (b).y || ((a).y == (b).y && (a).x < (b).x))
#define BD_CURSOR_GREAT(a, b) ((a).y > (b).y || ((a).y == (b).y && (a).x > (b).x))

enum {
  bd_view_welcome,
  bd_view_edit,
  bd_view_text,
  bd_view_explore,
  bd_view_terminal,
  bd_view_image,
};

struct bd_view_t {
  char title[256];
  int title_dirty;
  
  int type;
  
  bd_cursor_t cursor;
  void *data;
};

extern bd_config_t bd_config;

extern bd_view_t *bd_views;
extern int bd_view_count, bd_view;

extern int bd_width, bd_height;
extern time_t bd_time;

int bd_open(const char *path);

bd_view_t *bd_view_add(const char *title, int type, ...);
void       bd_view_remove(bd_view_t *view);
void       bd_view_draw(bd_view_t *view);
int        bd_view_tick(bd_view_t *view);
int        bd_view_event(bd_view_t *view, io_event_t event);

void bd_welcome_draw(bd_view_t *view);

void bd_edit_draw(bd_view_t *view);
int  bd_edit_event(bd_view_t *view, io_event_t event);

void bd_text_draw(bd_view_t *view);
int  bd_text_event(bd_view_t *view, io_event_t event);
void bd_text_load(bd_view_t *view, const char *path);
int  bd_text_save(bd_view_t *view, int closing); // Returns 1 if should close (when closing)

void bd_explore_draw(bd_view_t *view);
int  bd_explore_event(bd_view_t *view, io_event_t event);
void bd_explore_load(bd_view_t *view, const char *path);

void bd_terminal_draw(bd_view_t *view);
int  bd_terminal_tick(bd_view_t *view);
int  bd_terminal_event(bd_view_t *view, io_event_t event);
void bd_terminal_load(bd_view_t *view);
void bd_terminal_kill(bd_view_t *view);

void bd_image_draw(bd_view_t *view);
int  bd_image_event(bd_view_t *view, io_event_t event);
void bd_image_load(bd_view_t *view, const char *path);

void bd_global_draw(void);
int  bd_global_click(int x, int y);

// Dialog formatting:
// - "(1-9)[...]" -> Informational text (with number specifying length)
// - "i[...]" -> Text input (with prompt inside) -> Expects (char *)
// - "b[count;button_1;button_2;...]" -> Button line (with button count and titles inside, returned number corresponds to button index)
//
// NOTE: Ctrl+Q returns 0, while Enter returns the index of the selected button

int bd_dialog(const char *title, int width, const char *format, ...);

#endif
