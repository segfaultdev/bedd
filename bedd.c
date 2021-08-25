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
  tab->tmp_row = -1;

  tab->undo_cnt = 0;
  tab->undo_pos = -1;

  tab->step = 0;

  memset(tab->query, 0, 1024);

  bedd_push_undo(tab);
}

void bedd_free(bedd_t *tab) {
  for (int i = 0; i < tab->line_cnt; i++) {
    if (tab->lines[i].buffer) {
      free(tab->lines[i].buffer);
    }
  }

  free(tab->lines);

  while (tab->undo_cnt) {
    bedd_free_undo(tab->undo_buf + (--tab->undo_cnt));
  }
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
      printf(BEDD_INVERT);
    } else {
      printf(BEDD_NORMAL);
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
  printf(BEDD_INVERT " bedd r%s " BEDD_NORMAL " %s%s (%d,%d) (%d/%d) %s", BEDD_VER, tab->path ? tab->path : "no name", tab->dirty ? "*" : "", tab->row + 1, tab->col + 1, tab->undo_pos, tab->undo_cnt, status);
}

int bedd_color(bedd_t *tab, int state, int row, int col) {
  if (tab->path) {
    if (!strcmp(tab->path + (strlen(tab->path) - 2), ".c") ||
        !strcmp(tab->path + (strlen(tab->path) - 2), ".h") ||
        !strcmp(tab->path + (strlen(tab->path) - 3), ".cc") ||
        !strcmp(tab->path + (strlen(tab->path) - 3), ".hh") ||
        !strcmp(tab->path + (strlen(tab->path) - 4), ".cpp") ||
        !strcmp(tab->path + (strlen(tab->path) - 4), ".hpp") ||
        !strcmp(tab->path + (strlen(tab->path) - 4), ".cxx")) {
      tab->code = 1;
      return bedd_color_c(tab, state, row, col);
    } else if (!strcmp(tab->path + (strlen(tab->path) - 4), ".asm") ||
               !strcmp(tab->path + (strlen(tab->path) - 4), ".inc") ||
               !strcmp(tab->path + (strlen(tab->path) - 2), ".s")) {
      tab->code = 1;
      return bedd_color_asm(tab, state, row, col);
    } else if (!strcmp(tab->path + (strlen(tab->path) - 3), ".sh")) {
      tab->code = 1;
      return bedd_color_sh(tab, state, row, col);
    }
  }

  tab->code = 0;

  printf(BEDD_WHITE);
  return 0;
}

void bedd_indent(bedd_t *tab) {
  if (tab->path) {
    if (!strcmp(tab->path + (strlen(tab->path) - 2), ".c") ||
        !strcmp(tab->path + (strlen(tab->path) - 2), ".h") ||
        !strcmp(tab->path + (strlen(tab->path) - 3), ".cc") ||
        !strcmp(tab->path + (strlen(tab->path) - 3), ".hh") ||
        !strcmp(tab->path + (strlen(tab->path) - 4), ".cpp") ||
        !strcmp(tab->path + (strlen(tab->path) - 4), ".hpp") ||
        !strcmp(tab->path + (strlen(tab->path) - 4), ".cxx")) {
      bedd_indent_c(tab);
    } else if (!strcmp(tab->path + (strlen(tab->path) - 4), ".asm") ||
               !strcmp(tab->path + (strlen(tab->path) - 4), ".inc") ||
               !strcmp(tab->path + (strlen(tab->path) - 2), ".s")) {
      bedd_indent_asm(tab);
    } else if (!strcmp(tab->path + (strlen(tab->path) - 3), ".sh")) {
      bedd_indent_sh(tab);
    }
  }
}

static void bedd_write_raw(bedd_t *tab, char c) {
  if (c < 32 || c >= 127) {
    return;
  }

  if (tab->col > tab->lines[tab->row].length) {
    tab->col = tab->lines[tab->row].length;
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

void bedd_write(bedd_t *tab, char c) {
  tab->dirty = 1;

  if (tab->col > tab->lines[tab->row].length) {
    tab->col = tab->lines[tab->row].length;
  }

  if (c == '\n' || c == BEDD_CTRL('m')) {
    int on_block = 0;

    if (tab->col > 0 && tab->col < tab->lines[tab->row].length) {
      if (tab->lines[tab->row].buffer[tab->col - 1] == '{' && tab->lines[tab->row].buffer[tab->col] == '}') {
        on_block = 1;
      } else if (tab->lines[tab->row].buffer[tab->col - 1] == '(' && tab->lines[tab->row].buffer[tab->col] == ')') {
        on_block = 1;
      } else if (tab->lines[tab->row].buffer[tab->col - 1] == '[' && tab->lines[tab->row].buffer[tab->col] == ']') {
        on_block = 1;
      } else if (tab->lines[tab->row].buffer[tab->col - 1] == '"' && tab->lines[tab->row].buffer[tab->col] == '"') {
        on_block = 1;
      } else if (tab->lines[tab->row].buffer[tab->col - 1] == '\'' && tab->lines[tab->row].buffer[tab->col] == '\'') {
        on_block = 1;
      }
    }

    tab->lines = realloc(tab->lines, (tab->line_cnt + 1) * sizeof(bedd_line_t));
    tab->row++;

    memmove(tab->lines + tab->row + 1, tab->lines + tab->row, (tab->line_cnt - tab->row) * sizeof(bedd_line_t));

    tab->lines[tab->row].buffer = NULL;
    tab->lines[tab->row].length = 0;

    int col = tab->col;
    tab->col = 0;

    if (tab->code) {
      bedd_indent(tab);
    }

    int new_col = tab->col;

    for (int i = col; i < tab->lines[tab->row - 1].length; i++) {
      bedd_write_raw(tab, tab->lines[tab->row - 1].buffer[i]);
    }

    if (col < tab->lines[tab->row - 1].length) {
      tab->lines[tab->row - 1].length = col;
      tab->lines[tab->row - 1].buffer = realloc(tab->lines[tab->row - 1].buffer, tab->lines[tab->row - 1].length);
    }

    tab->col = new_col;
    tab->line_cnt++;

    tab->sel_row = tab->row;
    tab->sel_col = tab->col;

    if (on_block) {
      bedd_write(tab, BEDD_CTRL('m'));

      tab->row--;
      tab->col = tab->lines[tab->row].length;

      tab->sel_row = tab->row;
      tab->sel_col = tab->col;

      bedd_write_raw(tab, ' ');
      bedd_write_raw(tab, ' ');
    }

    return;
  }

  if (tab->code) {
    if (c == ')' && tab->col < tab->lines[tab->row].length) {
      if (tab->lines[tab->row].buffer[tab->col] == ')') {
        bedd_right(tab, 0);
        return;
      }
    }

    if (c == '}' && tab->col < tab->lines[tab->row].length) {
      if (tab->lines[tab->row].buffer[tab->col] == '}') {
        bedd_right(tab, 0);
        return;
      }
    }

    if (c == ']' && tab->col < tab->lines[tab->row].length) {
      if (tab->lines[tab->row].buffer[tab->col] == ']') {
        bedd_right(tab, 0);
        return;
      }
    }

    if (c == '"' && tab->col < tab->lines[tab->row].length) {
      if (tab->lines[tab->row].buffer[tab->col] == '"') {
        bedd_right(tab, 0);
        return;
      }
    }

    if (c == '\'' && tab->col < tab->lines[tab->row].length) {
      if (tab->lines[tab->row].buffer[tab->col] == '\'') {
        bedd_right(tab, 0);
        return;
      }
    }
  }

  bedd_write_raw(tab, c);

  if (tab->code) {
    if (c == '(') {
      bedd_write_raw(tab, ')');
      bedd_left(tab, 0);
    }

    if (c == '{') {
      bedd_write_raw(tab, '}');
      bedd_left(tab, 0);
    }

    if (c == '[') {
      bedd_write_raw(tab, ']');
      bedd_left(tab, 0);
    }

    if (c == '"') {
      bedd_write_raw(tab, '"');
      bedd_left(tab, 0);
    }

    if (c == '\'') {
      bedd_write_raw(tab, '\'');
      bedd_left(tab, 0);
    }
  }
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

  if (tab->col > tab->lines[tab->row].length) {
    tab->col = tab->lines[tab->row].length;
  }

  if (tab->col == 0) {
    if (tab->row == 0) {
      return;
    }

    tab->row--;
    tab->col = tab->lines[tab->row].length;

    int col = tab->col;

    for (int i = 0; i < tab->lines[tab->row + 1].length; i++) {
      bedd_write_raw(tab, tab->lines[tab->row + 1].buffer[i]);
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

int bedd_find(bedd_t *tab, const char *query, int whole_word) {
  if (!query) {
    return 0;
  }

  if (tab->col > tab->lines[tab->row].length) {
    tab->col = tab->lines[tab->row].length;
  }

  if (tab->col < 0) {
    tab->col = 0;
  }

  for (int i = tab->row; i < tab->line_cnt; i++) {
    if (tab->lines[i].length < strlen(query)) {
      continue;
    }

    for (int j = ((i == tab->row) ? tab->col : 0); j <= (tab->lines[i].length - strlen(query)); j++) {
      if (!tab->lines[i].buffer) {
        break;
      }

      if (memcmp(tab->lines[i].buffer + j, query, strlen(query))) {
        continue;
      }

      if (whole_word) {
        if (j > 0) {
          if (BEDD_ISIDENT(tab->lines[i].buffer[j - 1])) {
            continue;
          }
        }

        if (j < (tab->lines[i].length - (strlen(query) + 1))) {
          if (BEDD_ISIDENT(tab->lines[i].buffer[j + strlen(query)])) {
            continue;
          }
        }
      }

      tab->row = i;
      tab->col = j;

      tab->sel_row = tab->row;
      tab->sel_col = tab->col;

      tab->col += strlen(query);

      if (tab->off_row > tab->row) {
        tab->off_row = tab->row;
      }

      if (tab->off_row < tab->row - (tab->height - 3)) {
        tab->off_row = tab->row - (tab->height - 3);
      }

      return 1;
    }
  }

  return 0;
}

int bedd_replace(bedd_t *tab, const char *query, const char *replace, int whole_word) {
  int count = 0;

  if (!query || !replace) {
    return count;
  }

  for (int i = 0; i < tab->line_cnt; i++) {
    if (tab->lines[i].length < strlen(query)) {
      continue;
    }

    for (int j = 0; j <= (tab->lines[i].length - strlen(query)); j++) {
      if (!tab->lines[i].buffer) {
        break;
      }

      if (memcmp(tab->lines[i].buffer + j, query, strlen(query))) {
        continue;
      }

      if (whole_word) {
        if (j > 0) {
          if (BEDD_ISIDENT(tab->lines[i].buffer[j - 1])) {
            continue;
          }
        }

        if (j < (tab->lines[i].length - (strlen(query) + 1))) {
          if (BEDD_ISIDENT(tab->lines[i].buffer[j + strlen(query)])) {
            continue;
          }
        }
      }

      tab->row = i;
      tab->col = j + strlen(query);

      tab->sel_row = tab->row;
      tab->sel_col = tab->col;

      for (int k = 0; k < strlen(query); k++) {
        bedd_delete(tab);
      }

      for (int k = 0; k < strlen(replace); k++) {
        bedd_write_raw(tab, replace[k]);
      }

      j = tab->col - 1;
      count++;
    }
  }

  return count;
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

void bedd_free_undo(bedd_undo_t *undo) {
  for (int i = 0; i < undo->line_cnt; i++) {
    if (undo->lines[i].buffer) {
      free(undo->lines[i].buffer);
    }
  }

  free(undo->lines);
}

void bedd_push_undo(bedd_t *tab) {
  while ((tab->undo_cnt - 1) > tab->undo_pos) {
    bedd_free_undo(tab->undo_buf + (--tab->undo_cnt));
  }

  if (tab->undo_cnt >= BEDD_UNDO) {
    tab->undo_pos = BEDD_UNDO - 2;
    tab->undo_cnt = BEDD_UNDO - 1;
    memmove(tab->undo_buf, tab->undo_buf + 1, tab->undo_cnt * sizeof(bedd_undo_t));
  }

  tab->undo_buf[tab->undo_cnt].lines = malloc(tab->line_cnt * sizeof(bedd_line_t));
  tab->undo_buf[tab->undo_cnt].line_cnt = tab->line_cnt;

  for (int i = 0; i < tab->line_cnt; i++) {
    if (tab->lines[i].length) {
      tab->undo_buf[tab->undo_cnt].lines[i].buffer = malloc(tab->lines[i].length);
      memcpy(tab->undo_buf[tab->undo_cnt].lines[i].buffer, tab->lines[i].buffer, tab->lines[i].length);
    } else {
      tab->undo_buf[tab->undo_cnt].lines[i].buffer = NULL;
    }

    tab->undo_buf[tab->undo_cnt].lines[i].length = tab->lines[i].length;
  }

  tab->undo_buf[tab->undo_cnt].row = tab->row;
  tab->undo_buf[tab->undo_cnt].col = tab->col;

  tab->undo_cnt++;
  tab->undo_pos++;
}

void bedd_peek_undo(bedd_t *tab, int pos) {
  for (int i = 0; i < tab->line_cnt; i++) {
    if (tab->lines[i].buffer) {
      free(tab->lines[i].buffer);
    }
  }

  free(tab->lines);

  tab->lines = malloc(tab->undo_buf[pos].line_cnt * sizeof(bedd_undo_t));
  tab->line_cnt = tab->undo_buf[pos].line_cnt;

  for (int i = 0; i < tab->line_cnt; i++) {
    if (tab->undo_buf[pos].lines[i].length) {
      tab->lines[i].buffer = malloc(tab->undo_buf[pos].lines[i].length);
      memcpy(tab->lines[i].buffer, tab->undo_buf[pos].lines[i].buffer, tab->undo_buf[pos].lines[i].length);
    } else {
      tab->lines[i].buffer = NULL;
    }

    tab->lines[i].length = tab->undo_buf[pos].lines[i].length;
  }

  tab->row = tab->undo_buf[pos].row;
  tab->col = tab->undo_buf[pos].col;

  tab->sel_row = tab->row;
  tab->sel_col = tab->col;

  if (tab->off_row > tab->row) {
    tab->off_row = tab->row;
  }

  if (tab->off_row < tab->row - (tab->height - 3)) {
    tab->off_row = tab->row - (tab->height - 3);
  }
}

void bedd_copy(bedd_t *tab) {
  FILE *xclip = popen("xclip -selection clipboard -i", "w");
  
  if (!xclip) {
    return;
  }

  int row = tab->sel_row;
  int col = tab->sel_col;

  if (!tab->lines) {
    return;
  }

  if (row < 0) {
    row = 0;
  }

  if (row >= tab->line_cnt) {
    row = tab->line_cnt - 1;
  }

  if (col > tab->lines[row].length) {
    col = tab->lines[row].length;
  }

  while (row < tab->row || (row == tab->row && col < tab->col)) {
    if (col >= tab->lines[row].length) {
      col = 0;
      row++;

      fputc('\n', xclip);
    }

    if (col < tab->lines[row].length) {
      fputc(tab->lines[row].buffer[col], xclip);
    }

    col++;
  }

  pclose(xclip);
}

void bedd_paste(bedd_t *tab) {
  FILE *xclip = popen("xclip -selection clipboard -o", "r");
  if (!xclip) return;

  int code = tab->code;
  tab->code = 0;

  while (!feof(xclip)) {
    bedd_write(tab, fgetc(xclip));
  }

  tab->code = code;
  pclose(xclip);
}
