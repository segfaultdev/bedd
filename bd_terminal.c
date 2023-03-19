#include <bedd.h>

typedef struct bd_terminal_t bd_terminal_t;

struct bd_line_t {
  unsigned char *data;
  int size;
};

struct bd_terminal_t {

};

void bd_terminal_draw(bd_view_t *view) {

}

int bd_terminal_event(bd_view_t *view, io_event_t event) {
  return 0;
}

void bd_terminal_load(bd_view_t *view) {

}
