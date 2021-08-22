#ifndef __BEDD_H__
#define __BEDD_H__

#define BEDD_VER "01"

#define BEDD_WHITE "\033[7m"
#define BEDD_BLACK "\033[m"

#define BEDD_CTRL(k) ((k) & 0x1F)

typedef struct bedd_line_t bedd_line_t;
typedef struct bedd_t bedd_t;

struct bedd_line_t {
  char *buffer;
  int length;
};

struct bedd_t {
  char *path;
  int dirty;

  bedd_line_t *lines;
  int line_cnt;

  int row, col;
  int off_row;
};

void bedd_init(bedd_t *tab, const char *path);

void bedd_tabs(bedd_t *tabs, int tab_pos, int tab_cnt, int width);
void bedd_stat(bedd_t *tab);

void bedd_write(bedd_t *tab, char c);
void bedd_delete(bedd_t *tab);

void bedd_up(bedd_t *tab);
void bedd_down(bedd_t *tab);
void bedd_left(bedd_t *tab);
void bedd_right(bedd_t *tab);

#endif