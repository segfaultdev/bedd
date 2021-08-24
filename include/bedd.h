#ifndef __BEDD_H__
#define __BEDD_H__

#define BEDD_VER "01"

#define BEDD_INVERT "\x1B[7m"
#define BEDD_NORMAL "\x1B[27m"

#define BEDD_BLACK   "\x1B[30m"
#define BEDD_RED     "\x1B[31m"
#define BEDD_GREEN   "\x1B[32m"
#define BEDD_YELLOW  "\x1B[33m"
#define BEDD_BLUE    "\x1B[34m"
#define BEDD_MAGENTA "\x1B[35m"
#define BEDD_CYAN    "\x1B[36m"
#define BEDD_WHITE   "\x1B[97m"
#define BEDD_GRAY    "\x1B[37m"

#define BEDD_SELECT  "\x1B[100m"
#define BEDD_DEFAULT "\x1B[49m"

#define BEDD_CTRL(k) ((k) & 0x1F)

#define BEDD_ISNUMBR(c) ((c) >= '0' && (c) <= '9')
#define BEDD_ISUPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define BEDD_ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define BEDD_ISALPHA(c) (BEDD_ISUPPER((c)) || BEDD_ISLOWER((c)))
#define BEDD_ISALNUM(c) (BEDD_ISALPHA((c)) || BEDD_ISNUMBR((c)))
#define BEDD_ISIDENT(c) (BEDD_ISALNUM((c)) || (c) == '_')

typedef struct bedd_line_t bedd_line_t;
typedef struct bedd_t bedd_t;

typedef struct bedd_word_t bedd_word_t;

struct bedd_line_t {
  char *buffer;
  int length;
};

struct bedd_t {
  char *path;
  int dirty;

  bedd_line_t *lines;
  int line_cnt;

  // cursor position
  int row, col;

  // temp. offset
  int tmp_row;

  // start of selection
  int sel_row, sel_col;

  // scroll offset
  int off_row;

  // screen height
  int height;

  // code flag
  int code;
};

void bedd_init(bedd_t *tab, const char *path);
int  bedd_save(bedd_t *tab);

void bedd_tabs(bedd_t *tabs, int tab_pos, int tab_cnt, int width);
void bedd_stat(bedd_t *tab, const char *status);
int  bedd_color(bedd_t *tab, int state, int row, int col);
void bedd_indent(bedd_t *tab);

void bedd_write(bedd_t *tab, char c);
void bedd_delete(bedd_t *tab);

int  bedd_replace(bedd_t *tab, const char *query, const char *replace, int whole_word);

void bedd_up(bedd_t *tab, int select);
void bedd_down(bedd_t *tab, int select);
void bedd_left(bedd_t *tab, int select);
void bedd_right(bedd_t *tab, int select);

// langs

int  bedd_color_c(bedd_t *tab, int state, int row, int col);
void bedd_indent_c(bedd_t *tab);

int  bedd_color_asm(bedd_t *tab, int state, int row, int col);
void bedd_indent_asm(bedd_t *tab);

int  bedd_color_sh(bedd_t *tab, int state, int row, int col);
void bedd_indent_sh(bedd_t *tab);

#endif