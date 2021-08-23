#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bedd.h>

void bedd_init(bedd_t *tab, const char *path) {
  if (!tab) {
    return;
  }

  tab->lines = NULL;
  tab->line_cnt = 0;

  tab->lines = malloc(sizeof(bedd_line_t));

  tab->lines[0].buffer = NULL;
  tab->lines[0].length = 0;

  tab->line_cnt++;

  if (path) {
    tab->path = malloc(strlen(path) + 1);
    memcpy(tab->path, path, strlen(path) + 1);

    FILE *file = fopen(tab->path, "r");

    if (!file) {
      exit(1);
    }

    while (!feof(file)) {
      char c = fgetc(file);

      if ((c >= 32 && c != 127) || c == '\t') {
        tab->lines[tab->line_cnt - 1].buffer = realloc(tab->lines[tab->line_cnt - 1].buffer, tab->lines[tab->line_cnt - 1].length + 1);
        tab->lines[tab->line_cnt - 1].buffer[tab->lines[tab->line_cnt - 1].length++] = c;
      } else if (c == '\n') {
        tab->lines = realloc(tab->lines, (tab->line_cnt + 1) * sizeof(bedd_line_t));

        tab->lines[tab->line_cnt].buffer = NULL;
        tab->lines[tab->line_cnt].length = 0;

        tab->line_cnt++;
      }
    }
    
    fclose(file);
  } else {
    tab->path = NULL;
  }

  tab->dirty = 0;

  tab->row = 0;
  tab->col = 0;

  tab->sel_row = 0;
  tab->sel_col = 0;

  tab->off_row = 0;
}

int bedd_save(bedd_t *tab) {
  FILE *file = fopen(tab->path, "w");

  if (!file) {
    return 0;
  }

  for (int i = 0; i < tab->line_cnt; i++) {
    fwrite(tab->lines[i].buffer, 1, tab->lines[i].length, file);
    
    if (i < tab->line_cnt - 1 || tab->lines[i].length) {
      fputc('\n', file);
    }
  }

  fclose(file);
  return 1;
}

void bedd_tabs(bedd_t *tabs, int tab_pos, int tab_cnt, int width) {
  for (int i = 0; i < tab_cnt; i++) {
    // weird trick to overcome integer limits
    int tab_width = (((i + 1) * width) / tab_cnt) - ((i * width) / tab_cnt);

    if (i == tab_pos) {
      printf(BEDD_WHITE);
    } else {
      printf(BEDD_BLACK);
    }
    
    char *path_buffer = "no name";
    int   path_length = strlen(path_buffer);

    if (tabs[i].path) {
      path_buffer = tabs[i].path;
      path_length = strlen(tabs[i].path);
    }

    for (int j = 0; j < tab_width; j++) {
      if (j > 0 && j < tab_width - 1 && j - 1 < path_length) {
        printf("%c", path_buffer[j - 1]);
      } else {
        printf(" ");
      }
    }
  }
}

void bedd_stat(bedd_t *tab, const char *status) {
  printf(BEDD_WHITE " bedd r%s " BEDD_BLACK " %s%s (%d,%d) %s", BEDD_VER, tab->path ? tab->path : "no name", tab->dirty ? "*" : "", tab->row + 1, tab->col + 1, status);
}

void bedd_write(bedd_t *tab, char c) {
  tab->dirty = 1;

  if (tab->col > tab->lines[tab->row].length) {
    tab->col = tab->lines[tab->row].length;
  }

  if (c == BEDD_CTRL('m')) {
    tab->lines = realloc(tab->lines, (tab->line_cnt + 1) * sizeof(bedd_line_t));
    tab->row++;

    memmove(tab->lines + tab->row + 1, tab->lines + tab->row, (tab->line_cnt - tab->row) * sizeof(bedd_line_t));

    tab->lines[tab->row].buffer = NULL;
    tab->lines[tab->row].length = 0;

    int col = tab->col;
    tab->col = 0;

    for (int i = col; i < tab->lines[tab->row - 1].length; i++) {
      bedd_write(tab, tab->lines[tab->row - 1].buffer[i]);
    }

    if (col < tab->lines[tab->row - 1].length) {
      tab->lines[tab->row - 1].length = col;
      tab->lines[tab->row - 1].buffer = realloc(tab->lines[tab->row - 1].buffer, tab->lines[tab->row - 1].length);
    }

    tab->col = 0;
    tab->line_cnt++;

    tab->sel_row = tab->row;
    tab->sel_col = tab->col;

    return;
  }

  tab->lines[tab->row].buffer = realloc(tab->lines[tab->row].buffer, tab->lines[tab->row].length + 1);

  memmove(tab->lines[tab->row].buffer + tab->col + 1, tab->lines[tab->row].buffer + tab->col, tab->lines[tab->row].length - tab->col);

  tab->lines[tab->row].buffer[tab->col++] = c;
  tab->lines[tab->row].length++;

  if (tab->off_row > tab->row) {
    tab->off_row = tab->row;
  }

  if (tab->off_row < tab->row - (tab->height - 3)) {
    tab->off_row = tab->row - (tab->height - 3);
  }

  tab->sel_row = tab->row;
  tab->sel_col = tab->col;
}

void bedd_delete(bedd_t *tab) {
  if (tab->sel_row != tab->row || tab->sel_col != tab->col) {
    int row = tab->sel_row;
    int col = tab->sel_col;

    tab->sel_row = tab->row;
    tab->sel_col = tab->col;

    while (row != tab->row || col != tab->col) {
      bedd_delete(tab);
    }

    return;
  }

  if (tab->col == 0) {
    if (tab->row == 0) {
      return;
    }

    tab->row--;
    tab->col = tab->lines[tab->row].length;

    int col = tab->col;

    for (int i = 0; i < tab->lines[tab->row + 1].length; i++) {
      bedd_write(tab, tab->lines[tab->row + 1].buffer[i]);
    }

    memmove(tab->lines + tab->row + 1, tab->lines + tab->row + 2, (tab->line_cnt - (tab->row + 1)) * sizeof(bedd_line_t));

    tab->col = col;
    tab->line_cnt--;
    tab->lines = realloc(tab->lines, tab->line_cnt * sizeof(bedd_line_t));
  } else {
    tab->col--;

    memmove(tab->lines[tab->row].buffer + tab->col, tab->lines[tab->row].buffer + tab->col + 1, tab->lines[tab->row].length - tab->col);
    
    tab->lines[tab->row].length--;
    tab->lines[tab->row].buffer = realloc(tab->lines[tab->row].buffer, tab->lines[tab->row].length);
  }

  tab->sel_row = tab->row;
  tab->sel_col = tab->col;

  if (tab->off_row > tab->row) {
    tab->off_row = tab->row;
  }

  if (tab->off_row < tab->row - (tab->height - 3)) {
    tab->off_row = tab->row - (tab->height - 3);
  }

  tab->dirty = 1;
}

void bedd_up(bedd_t *tab, int select) {
  if (tab->row > 0) {
    tab->row--;
  } else {
    tab->col = 0;
  }

  if (!select || tab->sel_row > tab->row || (tab->sel_row == tab->row && tab->sel_col > tab->col)) {
    tab->sel_row = tab->row;
    tab->sel_col = tab->col;
  }

  if (tab->off_row > tab->row) {
    tab->off_row = tab->row;
  }

  if (tab->off_row < tab->row - (tab->height - 3)) {
    tab->off_row = tab->row - (tab->height - 3);
  }
}

void bedd_down(bedd_t *tab, int select) {
  if (tab->row < tab->line_cnt - 1) {
    tab->row++;
  } else {
    tab->col = tab->lines[tab->row].length;
  }

  if (!select || tab->sel_row > tab->row || (tab->sel_row == tab->row && tab->sel_col > tab->col)) {
    tab->sel_row = tab->row;
    tab->sel_col = tab->col;
  }

  if (tab->off_row > tab->row) {
    tab->off_row = tab->row;
  }

  if (tab->off_row < tab->row - (tab->height - 3)) {
    tab->off_row = tab->row - (tab->height - 3);
  }
}

void bedd_left(bedd_t *tab, int select) {
  if (tab->col > tab->lines[tab->row].length) {
    tab->col = tab->lines[tab->row].length;
  }

  if (tab->col > 0) {
    tab->col--;
  } else if (tab->row > 0) {
    tab->row--;
    tab->col = tab->lines[tab->row].length;
  }

  if (!select || tab->sel_row > tab->row || (tab->sel_row == tab->row && tab->sel_col > tab->col)) {
    tab->sel_row = tab->row;
    tab->sel_col = tab->col;
  }

  if (tab->off_row > tab->row) {
    tab->off_row = tab->row;
  }

  if (tab->off_row < tab->row - (tab->height - 3)) {
    tab->off_row = tab->row - (tab->height - 3);
  }
}

void bedd_right(bedd_t *tab, int select) {
  if (tab->col < tab->lines[tab->row].length) {
    tab->col++;
  } else if (tab->row < tab->line_cnt - 1) {
    tab->row++;
    tab->col = 0;
  }

  if (!select || tab->sel_row > tab->row || (tab->sel_row == tab->row && tab->sel_col > tab->col)) {
    tab->sel_row = tab->row;
    tab->sel_col = tab->col;
  }

  if (tab->off_row > tab->row) {
    tab->off_row = tab->row;
  }

  if (tab->off_row < tab->row - (tab->height - 3)) {
    tab->off_row = tab->row - (tab->height - 3);
  }
}