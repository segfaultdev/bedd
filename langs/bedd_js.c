#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bedd.h>

int bedd_color_js(bedd_t *tab, int state, int row, int col) {
  // 0 = neutral state
  // 1 = inside keyword/identifier
  // 2 = inside number
  // 3 = inside multiline comment
  // 4 = inside monoline comment
  // 5 = inside string
  // 6 = inside char
  // 7 = inside escape code(string)
  // 8 = inside escape code(char)
  // 9 = ending multiline comment

  if (col == 0) {
    if (state == 1 || state == 2 || state == 4 || state == 5 || state == 6 || state == 7 || state == 8) {
      printf(BEDD_WHITE);
      state = 0;
    }
  }

  if (state == 3) {
    printf(BEDD_GRAY);

    if (tab->lines[row].buffer[col] == '*' && col < tab->lines[row].length) {
      if (tab->lines[row].buffer[col + 1] == '/') {
        return 9;
      }
    }
    
    return 3;
  }

  if (state == 4) {
    printf(BEDD_GRAY);
    return 4;
  }

  if (state == 7) {
    return 5;
  }

  if (state == 8) {
    return 6;
  }

  if (state == 9) {
    return 0;
  }

  if (state == 5) {
    printf(BEDD_GREEN);

    if (tab->lines[row].buffer[col] == '\\') {
      return 7;
    }

    if (tab->lines[row].buffer[col] == '"') {
      return 0;
    } else {
      return 5;
    }
  } else if (state != 6 && tab->lines[row].buffer[col] == '"') {
    printf(BEDD_GREEN);
    return 5;
  }

  if (state == 6) {
    printf(BEDD_GREEN);

    if (tab->lines[row].buffer[col] == '\\') {
      return 8;
    }

    if (tab->lines[row].buffer[col] == '\'') {
      return 0;
    } else {
      return 6;
    }
  } else if (state != 5 && tab->lines[row].buffer[col] == '\'') {
    printf(BEDD_GREEN);
    return 6;
  }

  if (tab->lines[row].buffer[col] == '/' && col < tab->lines[row].length) {
    if (tab->lines[row].buffer[col + 1] == '/') {
      printf(BEDD_GRAY);
      return 4;
    }

    if (tab->lines[row].buffer[col + 1] == '*') {
      printf(BEDD_GRAY);
      return 3;
    }
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
      tab->lines[row].buffer[col] == ';' ||
      tab->lines[row].buffer[col] == '>' ||
      tab->lines[row].buffer[col] == '<' ||
      tab->lines[row].buffer[col] == '\\') {
    printf(BEDD_RED);
    return 0;
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

  if (state == 0 && length == 2) {
    if (!memcmp(tab->lines[row].buffer + col, "do", 2) ||
        !memcmp(tab->lines[row].buffer + col, "if", 2) ||
        !memcmp(tab->lines[row].buffer + col, "in", 2) ||
        !memcmp(tab->lines[row].buffer + col, "of", 2)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 3) {
    if (!memcmp(tab->lines[row].buffer + col, "for", 3) ||
        !memcmp(tab->lines[row].buffer + col, "let", 3) ||
        !memcmp(tab->lines[row].buffer + col, "try", 3) ||
        !memcmp(tab->lines[row].buffer + col, "var", 3)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 4) {
    if (!memcmp(tab->lines[row].buffer + col, "case", 4) ||
        !memcmp(tab->lines[row].buffer + col, "else", 4) ||
        !memcmp(tab->lines[row].buffer + col, "enum", 4) ||
        !memcmp(tab->lines[row].buffer + col, "null", 4) ||
        !memcmp(tab->lines[row].buffer + col, "this", 4) ||
        !memcmp(tab->lines[row].buffer + col, "true", 4) ||
        !memcmp(tab->lines[row].buffer + col, "void", 4) ||
        !memcmp(tab->lines[row].buffer + col, "with", 4)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 5) {
    if (!memcmp(tab->lines[row].buffer + col, "await", 5) ||
        !memcmp(tab->lines[row].buffer + col, "break", 5) ||
        !memcmp(tab->lines[row].buffer + col, "catch", 5) ||
        !memcmp(tab->lines[row].buffer + col, "class", 5) ||
        !memcmp(tab->lines[row].buffer + col, "const", 5) ||
        !memcmp(tab->lines[row].buffer + col, "false", 5) ||
        !memcmp(tab->lines[row].buffer + col, "super", 5) ||
        !memcmp(tab->lines[row].buffer + col, "throw", 5) ||
        !memcmp(tab->lines[row].buffer + col, "while", 5) ||
        !memcmp(tab->lines[row].buffer + col, "yield", 5)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 6) {
    if (!memcmp(tab->lines[row].buffer + col, "delete", 6) ||
        !memcmp(tab->lines[row].buffer + col, "export", 6) ||
        !memcmp(tab->lines[row].buffer + col, "import", 6) ||
        !memcmp(tab->lines[row].buffer + col, "public", 6) ||
        !memcmp(tab->lines[row].buffer + col, "return", 6) ||
        !memcmp(tab->lines[row].buffer + col, "static", 6) ||
        !memcmp(tab->lines[row].buffer + col, "switch", 6) ||
        !memcmp(tab->lines[row].buffer + col, "typeof", 6)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 7) {
    if (!memcmp(tab->lines[row].buffer + col, "default", 7) ||
        !memcmp(tab->lines[row].buffer + col, "extends", 7) ||
        !memcmp(tab->lines[row].buffer + col, "finally", 7) ||
        !memcmp(tab->lines[row].buffer + col, "package", 7) ||
        !memcmp(tab->lines[row].buffer + col, "private", 7)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length == 8) {
    if (!memcmp(tab->lines[row].buffer + col, "continue", 8) ||
        !memcmp(tab->lines[row].buffer + col, "debugger", 8) ||
        !memcmp(tab->lines[row].buffer + col, "function", 8)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }
  
  if (state == 0 && length == 9) {
    if (!memcmp(tab->lines[row].buffer + col, "interface", 9) ||
        !memcmp(tab->lines[row].buffer + col, "protected", 9)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }
  
  if (state == 0 && length == 10) {
    if (!memcmp(tab->lines[row].buffer + col, "implements", 10) ||
        !memcmp(tab->lines[row].buffer + col, "instanceof", 10)) {
      printf(BEDD_MAGENTA);
      return 1;
    }
  }

  if (state == 0 && length > 0) {
    if (last == '(') {
      printf(BEDD_BLUE);
    } else {
      printf(BEDD_WHITE);
    }

    return 1;
  }

  return state;
}

void bedd_indent_js(bedd_t *tab, int col, int on_block) {
  int level = 0;
  int step = 0;

  for (int i = 0; i < tab->lines[tab->row - 1].length; i++) {
    if (tab->lines[tab->row - 1].buffer[i] == ' ') {
      level++;
    } else {
      break;
    }
  }

  for (int i = level; i < col; i++) {
    if (tab->lines[tab->row - 1].buffer[i] == '{' || tab->lines[tab->row - 1].buffer[i] == '(' || tab->lines[tab->row - 1].buffer[i] == '[') {
      step++;
    } else if (tab->lines[tab->row - 1].buffer[i] == '}' || tab->lines[tab->row - 1].buffer[i] == ')' || tab->lines[tab->row - 1].buffer[i] == ']') {
      step--;
    }
  }

  if (step < 0) {
    step = 0;
  }

  if (!on_block) {
    level += 2 * step;
  }

  for (int i = 0; i < level; i++) {
    bedd_write(tab, ' ');
  }
}
