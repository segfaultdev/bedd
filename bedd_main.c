#define CUPD_FILE "bedd_main.c"
#define CUPD_ARGS "-g -Iinclude"

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <bedd.h>
#include <cupd.h>

struct termios old_termios;

void raw_on(void) {
  tcgetattr(STDIN_FILENO, &old_termios);

  struct termios new_termios = old_termios;

  new_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  new_termios.c_oflag &= ~(OPOST);
  new_termios.c_cflag |= (CS8);
  new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  new_termios.c_cc[VMIN] = 0;
  new_termios.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
}

void raw_off(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios);
}

int get_winsize(int *width, int *height) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return 1;
  } else {
    *width = ws.ws_col;
    *height = ws.ws_row;

    return 0;
  }
}

int prompt_str(char *buffer, int length, const char *prompt) {
  int pos = 0;
  buffer[pos] = '\0';

  int first = 1;

  for (;;) {
    int width = 80, height = 25;
    get_winsize(&width, &height);

    char c;

    if (read(STDIN_FILENO, &c, 1) > 0) {
      if (c == BEDD_CTRL('q')) {
        return 0;
      } else if (c == BEDD_CTRL('m')) {
        return 1;
      } else if (c == '\x7F' || c == BEDD_CTRL('h')) {
        if (pos) {
          buffer[--pos] = '\0';
        }
      } else {
        buffer[pos++] = c;
        buffer[pos] = '\0';
      }
    } else if (first) {
      first = 0;
    } else {
      continue;
    }

    printf("\x1B[%d;%dH", height, 1);
    printf(BEDD_WHITE " %s " BEDD_BLACK " %s", prompt, buffer);

    printf("\x1B[K");
    printf("\x1B[%d;%dH", height, pos + strlen(prompt) + 4);

    fflush(stdout);
  }

  return 1;
}

int main(int argc, const char **argv) {
  cupd_init(argc, argv);

  atexit(raw_off);
  raw_on();

  bedd_t *tabs = malloc(sizeof(bedd_t));
  int tab_pos = 0, tab_cnt = 1;

  bedd_init(tabs + tab_pos, "bar.txt");

  int first = 1;

  printf("\x1B[?1000;1002;1006;1015h");

  for (;;) {
    int width = 80, height = 25;
    get_winsize(&width, &height);

    char c;

    if (read(STDIN_FILENO, &c, 1) > 0) {
      if (c == BEDD_CTRL('q')) {
        if (tab_cnt == 1) {
          break;
        }

        for (int i = tab_pos; i < tab_cnt - 1; i++) {
          tabs[i] = tabs[i + 1];
        }

        tabs = realloc(tabs, (--tab_cnt) * sizeof(bedd_t));

        if (tab_pos >= tab_cnt) {
          tab_pos = tab_cnt - 1;
        }
      } else if (c == BEDD_CTRL('n')) {
        tabs = realloc(tabs, (tab_cnt + 1) * sizeof(bedd_t));
        bedd_init(tabs + tab_cnt, NULL);

        tab_pos = tab_cnt++;
      } else if (c == BEDD_CTRL('o')) {
        char buffer[1024];

        if (prompt_str(buffer, sizeof(buffer), "path:")) {
          if (strlen(buffer)) {
            tabs = realloc(tabs, (tab_cnt + 1) * sizeof(bedd_t));
            bedd_init(tabs + tab_cnt, buffer);

            tab_pos = tab_cnt++;
          }
        }
      } else if (c == '\x7F' || c == BEDD_CTRL('h')) {
        bedd_delete(tabs + tab_pos);
      } else if (c == BEDD_CTRL('b')) {
        tabs[tab_pos].col = 0;
      } else if (c == '\x1B') {
        char seq[6] = {0};

        if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
          if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
              if (read(STDIN_FILENO, &seq[2], 1) >= 1) {
                if (seq[2] == '~') {
                  if (seq[1] == '1' || seq[1] == '7') {
                    tabs[tab_pos].col = 0;
                  } else if (seq[1] == '4' || seq[1] == '8') {
                    tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
                  } else if (seq[1] == '3') {
                    bedd_right(tabs + tab_pos);
                    bedd_delete(tabs + tab_pos);
                  } else if (seq[1] == '5') {
                    for (int i = 0; i < (height - 2) / 2; i++) {
                      bedd_up(tabs + tab_pos);
                    }
                  } else if (seq[1] == '6') {
                    for (int i = 0; i < (height - 2) / 2; i++) {
                      bedd_down(tabs + tab_pos);
                    }
                  }
                } else if (seq[2] == ';') {
                  if (read(STDIN_FILENO, &seq[3], 1) >= 1 && read(STDIN_FILENO, &seq[4], 1) >= 1) {
                    if (seq[4] == 'A') {
                      tabs[tab_pos].row = 0;
                    } else if (seq[4] == 'B') {
                      tabs[tab_pos].row = tabs[tab_pos].line_cnt - 1;
                    } else if (seq[4] == 'C') {
                      if (tab_pos < tab_cnt - 1) {
                        tab_pos++;
                      }

                      continue;
                    } else if (seq[4] == 'D') {
                      if (tab_pos) {
                        tab_pos--;first = 0;
                      }

                      continue;
                    }
                  }
                }
              }
            } else {
              if (seq[1] == 'A') {
                bedd_up(tabs + tab_pos);
              } else if (seq[1] == 'B') {
                bedd_down(tabs + tab_pos);
              } else if (seq[1] == 'C') {
                bedd_right(tabs + tab_pos);
              } else if (seq[1] == 'D') {
                bedd_left(tabs + tab_pos);
              } else if (seq[1] == 'H') {
                tabs[tab_pos].col = 0;
              } else if (seq[1] == 'F') {
                tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
              } else if (seq[1] == '<') {
                if (read(STDIN_FILENO, &seq[2], 1) >= 1) {
                  if (seq[2] == '0') {
                    break;
                  } else if (seq[2] == '6') {
                    if (read(STDIN_FILENO, &seq[3], 1) >= 1) {
                      if (seq[3] == '4') {
                        for (int i = 0; i < 2; i++) {
                          if (tabs[tab_pos].off_row == 0) {
                            break;
                          }

                          tabs[tab_pos].off_row--;
                        }
                      } else if (seq[3] == '5') {
                        for (int i = 0; i < 2; i++) {
                          if (tabs[tab_pos].off_row >= tabs[tab_pos].line_cnt - 1) {
                            break;
                          }
                          
                          tabs[tab_pos].off_row++;
                        }
                      }
                    }
                  }

                  while (c != 'M') {
                    if (read(STDIN_FILENO, &c, 1) < 1) {
                      break;
                    }
                  }
                }
              }
            }
          } else if (seq[0] == 'O') {
            if (seq[1] == 'H') {
              tabs[tab_pos].col = 0;
            } else if (seq[1] == 'F') {
              tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
            }
          }
        }
      } else {
        bedd_write(tabs + tab_pos, c);
      }
    } else if (first) {
      first = 0;
    } else {
      continue;
    }

    printf("\x1B[2J\x1B[H" BEDD_BLACK);

    bedd_tabs(tabs, tab_pos, tab_cnt, width);

    int line_len = 0;
    int line_tmp = tabs[tab_pos].line_cnt;

    while (line_tmp) {
      line_len++;
      line_tmp /= 10;
    }

    if (line_len < 0) {
      line_len++;
    }

    if (tabs[tab_pos].off_row > tabs[tab_pos].row) {
      tabs[tab_pos].off_row = tabs[tab_pos].row;
    }

    if (tabs[tab_pos].off_row < tabs[tab_pos].row - (height - 3)) {
      tabs[tab_pos].off_row = tabs[tab_pos].row - (height - 3);
    }

    int row = tabs[tab_pos].off_row;
    int pos = 2;

    for (int i = 0; i < height - 2; i++, row++) {
      if (row >= 0 && row < tabs[tab_pos].line_cnt) {
        if (tabs[tab_pos].row == row) {
          printf(BEDD_WHITE "  %*d  " BEDD_BLACK " ", line_len, row + 1);
          pos = i + 2;
        } else {
          printf(BEDD_BLACK "  %*d |" BEDD_BLACK " ", line_len, row + 1);
        }

        if (tabs[tab_pos].lines[row].buffer) {
          fwrite(tabs[tab_pos].lines[row].buffer, 1, tabs[tab_pos].lines[row].length, stdout);
        }
      } else {
        printf(BEDD_BLACK "  %*s :" BEDD_BLACK " ", line_len, "");
      }

      printf("\r\n");
    }

    bedd_stat(tabs + tab_pos);

    int col = tabs[tab_pos].col;

    if (col > tabs[tab_pos].lines[tabs[tab_pos].row].length) {
      col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
    }

    printf("\x1B[%d;%dH", pos, col + line_len + 6);
    fflush(stdout);
  }

  printf("\x1B[?1000;1002;1006;1015l");
  printf("\x1B[2J\x1B[H" BEDD_BLACK);

  return 0;
}