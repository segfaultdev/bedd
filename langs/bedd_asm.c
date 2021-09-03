#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bedd.h>

int bedd_color_asm(bedd_t *tab, int state, int row, int col) {
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

  if (tab->lines[row].buffer[col] == ';') {
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

  if (tab->lines[row].buffer[col] == '%') {
    printf(BEDD_MAGENTA);
    return 1;
  }

  int length = 0;
  char last = ' ';

  for (int i = col; i < tab->lines[row].length; i++) {
    if (BEDD_ISIDENT(tab->lines[row].buffer[i])) {
      length++;
    } else {
      last = tab->lines[row].buffer[i];
      break;
    }
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
    if (!memcmp(tab->lines[row].buffer + col, "db", 2) ||
        !memcmp(tab->lines[row].buffer + col, "dw", 2) ||
        !memcmp(tab->lines[row].buffer + col, "dd", 2) ||
        !memcmp(tab->lines[row].buffer + col, "dq", 2)) {
      printf(BEDD_RED);
      return 1;
    }
  }

  if (state == 0 && length == 4) {
    if (!memcmp(tab->lines[row].buffer + col, "byte", 4) ||
        !memcmp(tab->lines[row].buffer + col, "word", 4)) {
      printf(BEDD_RED);
      return 1;
    }
  }

  if (state == 0 && length == 5) {
    if (!memcmp(tab->lines[row].buffer + col, "dword", 5) ||
        !memcmp(tab->lines[row].buffer + col, "qword", 5)) {
      printf(BEDD_RED);
      return 1;
    }
  }

  if (state == 0 && length == 2) {
    if (!memcmp(tab->lines[row].buffer + col, "ah", 2) ||
        !memcmp(tab->lines[row].buffer + col, "al", 2) ||
        !memcmp(tab->lines[row].buffer + col, "bh", 2) ||
        !memcmp(tab->lines[row].buffer + col, "bl", 2) ||
        !memcmp(tab->lines[row].buffer + col, "ch", 2) ||
        !memcmp(tab->lines[row].buffer + col, "cl", 2) ||
        !memcmp(tab->lines[row].buffer + col, "dh", 2) ||
        !memcmp(tab->lines[row].buffer + col, "dl", 2) ||
        !memcmp(tab->lines[row].buffer + col, "ax", 2) ||
        !memcmp(tab->lines[row].buffer + col, "bx", 2) ||
        !memcmp(tab->lines[row].buffer + col, "cx", 2) ||
        !memcmp(tab->lines[row].buffer + col, "dx", 2) ||
        !memcmp(tab->lines[row].buffer + col, "ip", 2) ||
        !memcmp(tab->lines[row].buffer + col, "sp", 2) ||
        !memcmp(tab->lines[row].buffer + col, "bp", 2) ||
        !memcmp(tab->lines[row].buffer + col, "si", 2) ||
        !memcmp(tab->lines[row].buffer + col, "di", 2) ||
        !memcmp(tab->lines[row].buffer + col, "cs", 2) ||
        !memcmp(tab->lines[row].buffer + col, "ds", 2) ||
        !memcmp(tab->lines[row].buffer + col, "es", 2) ||
        !memcmp(tab->lines[row].buffer + col, "fs", 2) ||
        !memcmp(tab->lines[row].buffer + col, "gs", 2) ||
        !memcmp(tab->lines[row].buffer + col, "ss", 2)) {
      printf(BEDD_BLUE);
      return 1;
    }
  }

  if (state == 0 && length == 3) {
    if (!memcmp(tab->lines[row].buffer + col, "eax", 3) ||
        !memcmp(tab->lines[row].buffer + col, "ebx", 3) ||
        !memcmp(tab->lines[row].buffer + col, "ecx", 3) ||
        !memcmp(tab->lines[row].buffer + col, "edx", 3) ||
        !memcmp(tab->lines[row].buffer + col, "eip", 3) ||
        !memcmp(tab->lines[row].buffer + col, "esp", 3) ||
        !memcmp(tab->lines[row].buffer + col, "ebp", 3) ||
        !memcmp(tab->lines[row].buffer + col, "esi", 3) ||
        !memcmp(tab->lines[row].buffer + col, "edi", 3) || 
        !memcmp(tab->lines[row].buffer + col, "cr0", 3) || 
        !memcmp(tab->lines[row].buffer + col, "cr1", 3) || 
        !memcmp(tab->lines[row].buffer + col, "cr2", 3) || 
        !memcmp(tab->lines[row].buffer + col, "cr3", 3)) {
      printf(BEDD_BLUE);
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
      printf(BEDD_MAGENTA);
    } else {
      printf(BEDD_WHITE);
    }

    return 1;
  }

  return state;
}

void bedd_indent_asm(bedd_t *tab, int col, int on_block) {
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
