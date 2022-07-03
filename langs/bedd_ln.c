#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bedd.h>

#define is_word(word) (state == 0 && length == strlen((word)) && !memcmp(tab->lines[row].buffer + col, (word), length))

int bedd_color_ln(bedd_t *tab, int state, int row, int col) {
  // 0 = neutral state
  // 1 = inside keyword/identifier
  // 2 = inside number
  // 3 = inside monoline comment
  // 4 = inside string
  // 5 = inside escape code
  
  if (col == 0) {
    printf(BEDD_WHITE);
    state = 0;
  }

  if (state == 3) {
    printf(BEDD_GRAY);
    return 3;
  }
  
  if (state == 5) {
    return 4;
  }

  if (state == 4) {
    printf(BEDD_GREEN);
    
    if (tab->lines[row].buffer[col] == '\\') {
      return 5;
    }
    
    if (tab->lines[row].buffer[col] == '"') {
      return 0;
    } else {
      return 4;
    }
  } else if (tab->lines[row].buffer[col] == '"') {
    printf(BEDD_GREEN);
    return 4;
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
      tab->lines[row].buffer[col] == ';' ||
      tab->lines[row].buffer[col] == '>' ||
      tab->lines[row].buffer[col] == '<' ||
      tab->lines[row].buffer[col] == '%' ||
      tab->lines[row].buffer[col] == '~') {
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
  
  if (is_word("and") || is_word("or") || is_word("xor")) {
    printf(BEDD_RED);
    return 1;
  }
  
  if (is_word("begin") || is_word("end") || is_word("func") || is_word("let") || is_word("if") || is_word("else") || is_word("array") || is_word("while") ||
      is_word("give") || is_word("break") || is_word("type_of") || is_word("size_of") || is_word("to_type") || is_word("get") || is_word("set") ||
      is_word("mem_read") || is_word("mem_write") || is_word("mem_copy") || is_word("mem_test") || is_word("str_copy") || is_word("str_test") ||
      is_word("str_size") || is_word("eval") || is_word("block") || is_word("ref")) {
    printf(BEDD_MAGENTA);
    return 1;
  }

  if (state == 0 && length > 2) {
    if (is_word("number") || is_word("pointer") || is_word("error") || is_word("null") || is_word("true") || is_word("false") ||
        !memcmp(tab->lines[row].buffer + col + (length - 2), "_t", 2)) {
      printf(BEDD_CYAN);
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

void bedd_indent_ln(bedd_t *tab, int col, int on_block) {
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
    if (!strcmp(tab->lines[tab->row - 1].buffer + i, "begin")) {
      step++;
    } else if (!strcmp(tab->lines[tab->row - 1].buffer + i, "end")) {
      step--;
    }
    
    if (step < 0) {
      step = 0;
    }
  }

  if (!on_block) {
    level += 2 * step;
  }

  for (int i = 0; i < level; i++) {
    bedd_write(tab, ' ');
  }
}
