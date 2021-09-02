#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bedd.h>

int bedd_color_sh(bedd_t *tab, int state, int row, int col) {
  // 0 = neutral state
  // 1 = inside keyword/identifier
  // 2 = inside number
  // 3 = inside monoline comment
  // 4 = inside string
  // 5 = inside char

  if (col == 0) {
    printf(BEDD_WHITE);
    state = 0;
  }

  if (state == 3) {
    printf(BEDD_GRAY);
    return 3;
  }

  if (state == 4) {
    printf(BEDD_GREEN);

    if (tab->lines[row].buffer[col] == '"') {
      return 0;
    } else {
      return 4;
    }
  } else if (state != 5 && tab->lines[row].buffer[col] == '"') {
    printf(BEDD_GREEN);
    return 4;
  }

  if (state == 5) {
    printf(BEDD_GREEN);

    if (tab->lines[row].buffer[col] == '\'') {
      return 0;
    } else {
      return 5;
    }
  } else if (state != 4 && tab->lines[row].buffer[col] == '\'') {
    printf(BEDD_GREEN);
    return 5;
  }

  if (tab->lines[row].buffer[col] == '#') {
    printf(BEDD_GRAY);
    return 3;
  }

  if (tab->lines[row].buffer[col] == ' ' ||
      tab->lines[row].buffer[col] == '(' ||
      tab->lines[row].buffer[col] == ')' ||
      tab->lines[row].buffer[col] == '{' ||
      tab->lines[row].buffer[col] == '}' ||
      tab->lines[row].buffer[col] == '[' ||
      tab->lines[row].buffer[col] == ']') {
    printf(BEDD_WHITE);
    return 0;
  }

  if (tab->lines[row].buffer[col] == '+' ||
      tab->lines[row].buffer[col] == '-' ||
      tab->lines[row].buffer[col] == '*' ||
      tab->lines[row].buffer[col] == '/' ||
      tab->lines[row].buffer[col] == '=' ||
      tab->lines[row].buffer[col] == '&' ||
      tab->lines[row].buffer[col] == '|' ||
      tab->lines[row].buffer[col] == '^' ||
      tab->lines[row].buffer[col] == '!' ||
      tab->lines[row].buffer[col] == '?' ||
      tab->lines[row].buffer[col] == ':' ||
      tab->lines[row].buffer[col] == '.' ||
      tab->lines[row].buffer[col] == ',' ||
      tab->lines[row].buffer[col] == '>' ||
      tab->lines[row].buffer[col] == '<' ||
      tab->lines[row].buffer[col] == '\\') {
    printf(BEDD_RED);
    return 0;
  }

  int length = 0;
  char last = ' ';
  char prev = ' ';

  for (int i = col; i < tab->lines[row].length; i++) {
    if (BEDD_ISIDENT(tab->lines[row].buffer[i])) {
      length++;
    } else {
      last = tab->lines[row].buffer[i];
      break;
    }
  }

  if (col > 0) {
    prev = tab->lines[row].buffer[col - 1];
  }

  if (state == 0 && BEDD_ISNUMBR(tab->lines[row].buffer[col])) {
    printf(BEDD_YELLOW);
    return 2;
  }

  if (state == 0 && last == ':') {
    printf(BEDD_CYAN);
    return 1;
  }

  if (state == 0 && length == 2) {
    if (!memcmp(tab->lines[row].buffer + col, "if", 2) ||
        !memcmp(tab->lines[row].buffer + col, "fi", 2) ||
        !memcmp(tab->lines[row].buffer + col, "do", 2)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 3) {
    if (!memcmp(tab->lines[row].buffer + col, "set", 3) ||
        !memcmp(tab->lines[row].buffer + col, "for", 3)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 4) {
    if (!memcmp(tab->lines[row].buffer + col, "echo", 4) ||
        !memcmp(tab->lines[row].buffer + col, "read", 4) ||
        !memcmp(tab->lines[row].buffer + col, "else", 4) ||
        !memcmp(tab->lines[row].buffer + col, "done", 4) ||
        !memcmp(tab->lines[row].buffer + col, "case", 4) ||
        !memcmp(tab->lines[row].buffer + col, "esac", 4) ||
        !memcmp(tab->lines[row].buffer + col, "exit", 4) ||
        !memcmp(tab->lines[row].buffer + col, "trap", 4) ||
        !memcmp(tab->lines[row].buffer + col, "wait", 4) ||
        !memcmp(tab->lines[row].buffer + col, "eval", 4) ||
        !memcmp(tab->lines[row].buffer + col, "exec", 4)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 5) {
    if (!memcmp(tab->lines[row].buffer + col, "unset", 5) ||
        !memcmp(tab->lines[row].buffer + col, "shift", 5) ||
        !memcmp(tab->lines[row].buffer + col, "while", 5) ||
        !memcmp(tab->lines[row].buffer + col, "until", 5) ||
        !memcmp(tab->lines[row].buffer + col, "break", 5) ||
        !memcmp(tab->lines[row].buffer + col, "umask", 5)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 6) {
    if (!memcmp(tab->lines[row].buffer + col, "export", 6) ||
        !memcmp(tab->lines[row].buffer + col, "return", 6) ||
        !memcmp(tab->lines[row].buffer + col, "ulimit", 6)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 8) {
    if (!memcmp(tab->lines[row].buffer + col, "readonly", 8) ||
        !memcmp(tab->lines[row].buffer + col, "continue", 8)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length > 0) {
    int is_first = 1;

    for (int i = 0; i < col; i++) {
      if (tab->lines[row].buffer[i] != ' ') {
        is_first = 0;
        break;
      }
    }

    if (is_first) {
      printf(BEDD_BLUE);
    } else if (prev == '-') {
      printf(BEDD_RED);
    } else {
      printf(BEDD_WHITE);
    }

    return 1;
  }



  return state;
}

void bedd_indent_sh(bedd_t *tab, int col) {
  int level = 0;

  for (int i = 0; i < tab->lines[tab->row - 1].length; i++) {
    if (tab->lines[tab->row - 1].buffer[i] == ' ') {
      level++;
    } else {
      break;
    }
  }

  for (int i = 0; i < level; i++) {
    bedd_write(tab, ' ');
  }
}
