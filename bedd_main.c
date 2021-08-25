#define CUPD_FILE "bedd_main.c"
#define CUPD_ARGS "-Os -Iinclude"

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

int prompt_str(char *buffer, int length, int clear, const char *prompt) {
  int pos = 0;

  if (!clear) {
    pos = strlen(buffer);
  }

  buffer[pos] = '\0';

  int first = 1;

  printf("\x1B[?25h");
  printf("\x1B[3 q");

  for (;;) {
    int width = 80, height = 25;
    get_winsize(&width, &height);

    char c = '\0';

    if (read(STDIN_FILENO, &c, 1) > 0) {
      if (c == BEDD_CTRL('q')) {
        return 0;
      } else if (c == BEDD_CTRL('m')) {
        return 1;
      } else if (c == '\x7F' || c == BEDD_CTRL('h')) {
        if (pos) {
          buffer[--pos] = '\0';
        }
      } else if (c >= 32 && c < 127) {
        buffer[pos++] = c;
        buffer[pos] = '\0';
      }
    } else if (first) {
      first = 0;
    } else {
      continue;
    }

    printf("\x1B[%d;%dH", height, 1);
    printf(BEDD_INVERT " %s " BEDD_NORMAL " %s", prompt, buffer);

    printf("\x1B[K");
    printf("\x1B[%d;%dH", height, pos + strlen(prompt) + 4);

    fflush(stdout);
  }

  return 1;
}

int dir_tree(int row, int col, int height, const char *path) {
  struct dirent *entry;
  DIR *dir;

  if (!(dir = opendir(path))) {
    return row;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.') {
      continue;
    }

    char new_path[strlen(path) + strlen(entry->d_name) + 2];  
    sprintf(new_path, "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      if (row > 1 && row < height) {
        printf("\x1B[%d;%dH+ ", row, col);

        int pos = 0;

        for (int i = col; i < BEDD_TREE; i++) {
          if (pos < strlen(entry->d_name)) {
            printf("%c", entry->d_name[pos]);
          } else {
            printf(" ");
          }

          pos++;
        }
      }

      row = dir_tree(row + 1, col + 2, height, new_path);
    } else {
      if (row > 1 && row < height) {
        printf("\x1B[%d;%dH- ", row, col);

        int pos = 0;

        for (int i = col; i < BEDD_TREE; i++) {
          if (pos < strlen(entry->d_name)) {
            printf("%c", entry->d_name[pos]);
          } else {
            printf(" ");
          }

          pos++;
        }
      }

      row++;
    }
  }

  closedir(dir);
  return row;
}


int main(int argc, const char **argv) {
  cupd_init(argc, argv);

  atexit(raw_off);
  raw_on();

  bedd_t *tabs = NULL;
  int tab_pos = 0, tab_cnt = 0;

  struct stat file;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i] + (strlen(argv[i]) -  5), ".java") &&
        strcmp(argv[i] + (strlen(argv[i]) -  3), ".py") &&
        strcmp(argv[i] + (strlen(argv[i]) -  7), ".python") &&
        strcmp(argv[i] + (strlen(argv[i]) - 10), ".gordavaca") &&
        strcmp(argv[i] + (strlen(argv[i]) -  3), ".gv")) {
      if (stat(argv[i], &file) >= 0) {
        tabs = realloc(tabs, (++tab_cnt) * sizeof(bedd_t));
        bedd_init(tabs + (tab_cnt - 1), argv[i]);
      }
    }
  }

  if (!tab_cnt) {
    tabs = realloc(tabs, (++tab_cnt) * sizeof(bedd_t));
    bedd_init(tabs + tab_pos, NULL);
  }

  int first = 1;
  int show_tree = 0;

  int off_dir = 0;
  int tmp_dir = -1;
  int dir_len = 25;

  char status[1024] = {0};

  printf("\x1B[?47h");
  printf("\x1B[?1000;1002;1006;1015h");
  printf(BEDD_WHITE);

  for (;;) {
    int width = 80, height = 25;
    get_winsize(&width, &height);

    tabs[tab_pos].height = height;

    char c;

    if (read(STDIN_FILENO, &c, 1) > 0) {
      if (c == BEDD_CTRL('q')) {
        int do_exit = 0;

        if (tabs[tab_pos].dirty) {
          char buffer[1024];

          if (prompt_str(buffer, sizeof(buffer), 1, "there are unsaved changes, sure? (y/n)")) {
            if (buffer[0] == 'y' || buffer[0] == 'Y') {
              do_exit = 1;
            }
          }
        } else {
          do_exit = 1;
        }

        if (do_exit) {
          bedd_free(tabs + tab_pos);

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
        }
      } else if (c == BEDD_CTRL('t')) {
        show_tree = 1 - show_tree;
      } else if (c == BEDD_CTRL('d')) {
        char buffer[1024];

        if (prompt_str(buffer, 1024, 1, "path:")) {
          if (strlen(buffer)) {
            char command[1280];
            sprintf(command, "rm -rf %s", buffer);

            system(command);
            sprintf(status, "| file/directory deleted successfully");
          }
        }
      } else if (c == BEDD_CTRL('k')) {
        char buffer[1024];

        if (prompt_str(buffer, 1024, 1, "path:")) {
          if (strlen(buffer)) {
            char command[1280];
            sprintf(command, "mkdir -p %s", buffer);

            system(command);
            sprintf(status, "| directory created successfully");
          }
        }
      } else if (c == BEDD_CTRL('z')) {
        if (tabs[tab_pos].step) {
          bedd_push_undo(tabs + tab_pos);
          tabs[tab_pos].step = 0;
        }

        if (tabs[tab_pos].undo_pos) {
          bedd_peek_undo(tabs + tab_pos, --tabs[tab_pos].undo_pos);
        }
      } else if (c == BEDD_CTRL('y')) {
        if (tabs[tab_pos].step) {
          bedd_push_undo(tabs + tab_pos);
          tabs[tab_pos].step = 0;
        }

        if (tabs[tab_pos].undo_pos < tabs[tab_pos].undo_cnt - 1) {
          bedd_peek_undo(tabs + tab_pos, ++tabs[tab_pos].undo_pos);
        }
      } else if (c == BEDD_CTRL('a')) {
        tabs[tab_pos].sel_row = 0;
        tabs[tab_pos].sel_col = 0;

        tabs[tab_pos].row = tabs[tab_pos].line_cnt - 1;
        tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
      } else if (c == BEDD_CTRL('c')) {
        if (tabs[tab_pos].row != tabs[tab_pos].sel_row || tabs[tab_pos].col != tabs[tab_pos].sel_col) {
          bedd_copy(tabs + tab_pos);
        }
      } else if (c == BEDD_CTRL('x')) {
        if (tabs[tab_pos].row != tabs[tab_pos].sel_row || tabs[tab_pos].col != tabs[tab_pos].sel_col) {
          bedd_copy(tabs + tab_pos);
          bedd_delete(tabs + tab_pos);
        }
      } else if (c == BEDD_CTRL('v')) {
        if (tabs[tab_pos].row != tabs[tab_pos].sel_row || tabs[tab_pos].col != tabs[tab_pos].sel_col) {
          bedd_delete(tabs + tab_pos);
        }

        bedd_paste(tabs + tab_pos);
      } else if (c == BEDD_CTRL('f')) {
        // we preserve the last query, as this is going to be
        // called a lot of times consecutively

        int whole_word = 0;

        if (prompt_str(tabs[tab_pos].query, 1024, 0, "query:")) {
          if (tabs[tab_pos].query[0] == '~') {
            whole_word = 1;
          }

          if (strlen(tabs[tab_pos].query)) {
            if (bedd_find(tabs + tab_pos, tabs[tab_pos].query + whole_word, whole_word)) {
              sprintf(status, "| query found(Ctrl+F, then Enter to get next)");
            } else {
              sprintf(status, "| query not found(go to top with Ctrl+Up first)");
            }
          }
        }
      } else if (c == BEDD_CTRL('g')) {
        char buffer_1[1024];
        char buffer_2[1024];

        int whole_word = 0;

        if (prompt_str(buffer_1, sizeof(buffer_1), 1, "query:")) {
          if (buffer_1[0] == '~') {
            whole_word = 1;
          }

          if (strlen(buffer_1 + whole_word)) {
            if (prompt_str(buffer_2, sizeof(buffer_2), 1, "replace with:")) {
              int count = bedd_replace(tabs + tab_pos, buffer_1 + whole_word, buffer_2, whole_word);

              sprintf(status, "| replaced %d occurence%s", count, count == 1 ? "" : "s");

              if (count) {
                bedd_push_undo(tabs + tab_pos);
              }
            }
          }
        }
      } else if (c == BEDD_CTRL('n')) {
        tabs = realloc(tabs, (tab_cnt + 1) * sizeof(bedd_t));
        bedd_init(tabs + tab_cnt, NULL);

        tab_pos = tab_cnt++;
      } else if (c == BEDD_CTRL('o')) {
        char buffer[1024];

        if (prompt_str(buffer, sizeof(buffer), 1, "path:")) {
          if (strlen(buffer)) {
            if (stat(buffer, &file) < 0) {
              sprintf(status, "| cannot open file: \"%s\"", buffer);
            } else if (!strcmp(buffer + (strlen(buffer) -  5), ".java") ||
                       !strcmp(buffer + (strlen(buffer) -  3), ".py") ||
                       !strcmp(buffer + (strlen(buffer) -  7), ".python") ||
                       !strcmp(buffer + (strlen(buffer) - 10), ".gordavaca") ||
                       !strcmp(buffer + (strlen(buffer) -  3), ".gv")) {
              sprintf(status, "| file too dangerous: \"%s\"", buffer);
            } else {
              tabs = realloc(tabs, (tab_cnt + 1) * sizeof(bedd_t));
              bedd_init(tabs + tab_cnt, buffer);

              tab_pos = tab_cnt++;
            }
          }
        }
      } else if (c == BEDD_CTRL('s')) {
        char buffer[1024];
        int prompted = 0;

        if (tabs[tab_pos].step) {
          bedd_push_undo(tabs + tab_pos);
          tabs[tab_pos].step = 0;
        }

        if (!tabs[tab_pos].path) {
          if (prompt_str(buffer, sizeof(buffer), 1, "path:")) {
            if (strlen(buffer)) {
              tabs[tab_pos].path = malloc(strlen(buffer) + 1);
              strcpy(tabs[tab_pos].path, buffer);

              prompted = 1;
            }
          }
        }

        if (!bedd_save(tabs + tab_pos) ||
            !strcmp(tabs[tab_pos].path + (strlen(tabs[tab_pos].path) -  5), ".java") ||
            !strcmp(tabs[tab_pos].path + (strlen(tabs[tab_pos].path) -  3), ".py") ||
            !strcmp(tabs[tab_pos].path + (strlen(tabs[tab_pos].path) -  7), ".python") ||
            !strcmp(tabs[tab_pos].path + (strlen(tabs[tab_pos].path) - 10), ".gordavaca") ||
            !strcmp(tabs[tab_pos].path + (strlen(tabs[tab_pos].path) -  3), ".gv")) {
          if (prompted) {
            sprintf(status, "| cannot save file: \"%s\"", tabs[tab_pos].path);

            free(tabs[tab_pos].path);
            tabs[tab_pos].path = NULL;
          }
        } else {
          sprintf(status, "| file saved successfully");
          tabs[tab_pos].dirty = 0;
        }
      } else if (c == '\t') {
        for (int i = 0; i < 2 - (tabs[tab_pos].col + 1) % 2; i++) {
          bedd_write(tabs + tab_pos, ' ');
        }

        tabs[tab_pos].step++;

        if (tabs[tab_pos].step >= BEDD_STEP) {
          bedd_push_undo(tabs + tab_pos);
          tabs[tab_pos].step = 0;
        }
      } else if (c == '\x7F' || c == BEDD_CTRL('h')) {
        bedd_delete(tabs + tab_pos);
        
        tabs[tab_pos].step++;

        if (tabs[tab_pos].step >= BEDD_STEP) {
          bedd_push_undo(tabs + tab_pos);
          tabs[tab_pos].step = 0;
        }
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
                    tabs[tab_pos].sel_col = tabs[tab_pos].col;
                  } else if (seq[1] == '4' || seq[1] == '8') {
                    tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
                    tabs[tab_pos].sel_col = tabs[tab_pos].col;
                  } else if (seq[1] == '3') {
                    if (tabs[tab_pos].sel_row == tabs[tab_pos].row && tabs[tab_pos].sel_col == tabs[tab_pos].col) {
                      bedd_right(tabs + tab_pos, 0);
                    }

                    bedd_delete(tabs + tab_pos);
                    
                    tabs[tab_pos].step++;

                    if (tabs[tab_pos].step >= BEDD_STEP) {
                      bedd_push_undo(tabs + tab_pos);
                      tabs[tab_pos].step = 0;
                    }
                  } else if (seq[1] == '5') {
                    for (int i = 0; i < (height - 2) / 2; i++) {
                      bedd_up(tabs + tab_pos, 0);
                    }
                  } else if (seq[1] == '6') {
                    for (int i = 0; i < (height - 2) / 2; i++) {
                      bedd_down(tabs + tab_pos, 0);
                    }
                  }
                } else if (seq[2] == ';') {
                  if (read(STDIN_FILENO, &seq[3], 1) >= 1 && read(STDIN_FILENO, &seq[4], 1) >= 1) {
                    if (seq[3] == '2') {
                      if (seq[4] == 'A') {
                        bedd_up(tabs + tab_pos, 1);
                      } else if (seq[4] == 'B') {
                        bedd_down(tabs + tab_pos, 1);
                      } else if (seq[4] == 'C') {
                        bedd_right(tabs + tab_pos, 1);
                      } else if (seq[4] == 'D') {
                        bedd_left(tabs + tab_pos, 1);
                      }
                    } else if (seq[3] == '5') {
                      if (seq[4] == 'A') {
                        tabs[tab_pos].row = 0;
                        tabs[tab_pos].sel_row = tabs[tab_pos].row;

                        if (tabs[tab_pos].off_row > tabs[tab_pos].row) {
                          tabs[tab_pos].off_row = tabs[tab_pos].row;
                        }

                        if (tabs[tab_pos].off_row < tabs[tab_pos].row - (height - 3)) {
                          tabs[tab_pos].off_row = tabs[tab_pos].row - (height - 3);
                        }
                      } else if (seq[4] == 'B') {
                        tabs[tab_pos].row = tabs[tab_pos].line_cnt - 1;
                        tabs[tab_pos].sel_row = tabs[tab_pos].row;

                        if (tabs[tab_pos].off_row > tabs[tab_pos].row) {
                          tabs[tab_pos].off_row = tabs[tab_pos].row;
                        }

                        if (tabs[tab_pos].off_row < tabs[tab_pos].row - (height - 3)) {
                          tabs[tab_pos].off_row = tabs[tab_pos].row - (height - 3);
                        }
                      } else if (seq[4] == 'C') {
                        if (tab_pos < tab_cnt - 1) {
                          tab_pos++;
                        }
                      } else if (seq[4] == 'D') {
                        if (tab_pos) {
                          tab_pos--;
                        }
                      }
                    }
                  }
                }
              }
            } else {
              if (seq[1] == 'A') {
                bedd_up(tabs + tab_pos, 0);
              } else if (seq[1] == 'B') {
                bedd_down(tabs + tab_pos, 0);
              } else if (seq[1] == 'C') {
                bedd_right(tabs + tab_pos, 0);
              } else if (seq[1] == 'D') {
                bedd_left(tabs + tab_pos, 0);
              } else if (seq[1] == 'H') {
                tabs[tab_pos].col = 0;
              } else if (seq[1] == 'F') {
                tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
              } else if (seq[1] == '<') {
                if (read(STDIN_FILENO, &seq[2], 1) >= 1) {
                  if (seq[2] == '0') {
                    if (read(STDIN_FILENO, &seq[3], 1) >= 1) {
                      if (seq[3] == ';') {
                        int row = 0;
                        int col = 0;

                        for (;;) {
                          if (!read(STDIN_FILENO, &c, 1)) {
                            break;
                          }

                          if (!(c >= '0' && c <= '9')) {
                            break;
                          }

                          col *= 10;
                          col += (c - '0');
                        }

                        for (;;) {
                          if (!read(STDIN_FILENO, &c, 1)) {
                            break;
                          }

                          if (!(c >= '0' && c <= '9')) {
                            break;
                          }

                          row *= 10;
                          row += (c - '0');
                        }

                        if (row == 1) {
                          tab_pos = (col * tab_cnt) / width;

                          if (tab_pos >= tab_cnt) {
                            tab_pos = tab_cnt - 1;
                          }
                        }

                        if (col == width && row > 1 && row < height) {
                          int scroll_start = ((tabs[tab_pos].off_row) * (height - 2)) / tabs[tab_pos].line_cnt;
                          int scroll_end   = ((tabs[tab_pos].off_row + (height - 2)) * (height - 2)) / tabs[tab_pos].line_cnt;

                          if (row - 2 >= scroll_start && row - 2 <= scroll_end) {
                            tabs[tab_pos].tmp_row = (row - 1) - scroll_start;
                          } else {
                            tabs[tab_pos].tmp_row = (scroll_end - scroll_start) / 2;

                            int scroll = (row - 1) - tabs[tab_pos].tmp_row;
                            tabs[tab_pos].off_row = (tabs[tab_pos].line_cnt * scroll) / (height - 2);
                          }
                        } else {
                          tabs[tab_pos].tmp_row = -1;
                        }

                        if (show_tree && col <= BEDD_TREE && row > 1 && row < height) {
                          int scroll_start = (off_dir * (height - 2)) / dir_len;
                          int scroll_end   = ((off_dir + (height - 2)) * (height - 2)) / dir_len;

                          if (row - 2 >= scroll_start && row - 2 <= scroll_end) {
                            tmp_dir = (row - 1) - scroll_start;
                          } else {
                            tmp_dir = (scroll_end - scroll_start) / 2;

                            int scroll = (row - 1) - tmp_dir;
                            off_dir = (dir_len * scroll) / (height - 2);
                          }
                        } else {
                          tmp_dir = -1;
                        }

                        if (!(show_tree && col <= BEDD_TREE) && col < width && row > 1 && row < height) {
                          tabs[tab_pos].row = (row - 2) + tabs[tab_pos].off_row;

                          if (tabs[tab_pos].row < 0) {
                            tabs[tab_pos].row = 0;
                          }

                          if (tabs[tab_pos].row >= tabs[tab_pos].line_cnt) {
                            tabs[tab_pos].row = tabs[tab_pos].line_cnt - 1;
                          }    

                          int line_len = 0;
                          int line_tmp = tabs[tab_pos].line_cnt;

                          while (line_tmp) {
                            line_len++;
                            line_tmp /= 10;
                          }

                          if (line_len < 0) {
                            line_len++;
                          }

                          tabs[tab_pos].col = col - (line_len + (show_tree * BEDD_TREE) + 6);

                          if (tabs[tab_pos].col < 0) {
                            tabs[tab_pos].col = 0;
                          }

                          if (tabs[tab_pos].col > tabs[tab_pos].lines[tabs[tab_pos].row].length) {
                            tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
                          }
                        }
                      }
                    }
                  } else if (seq[2] == '3') {
                    if (read(STDIN_FILENO, &seq[3], 1) >= 1 && read(STDIN_FILENO, &seq[4], 1) >= 1) {
                      if (seq[4] == ';') {
                        int row = 0;
                        int col = 0;

                        for (;;) {
                          if (!read(STDIN_FILENO, &c, 1)) {
                            break;
                          }

                          if (!(c >= '0' && c <= '9')) {
                            break;
                          }

                          col *= 10;
                          col += (c - '0');
                        }

                        for (;;) {
                          if (!read(STDIN_FILENO, &c, 1)) {
                            break;
                          }

                          if (!(c >= '0' && c <= '9')) {
                            break;
                          }

                          row *= 10;
                          row += (c - '0');
                        }

                        if (tabs[tab_pos].tmp_row != -1) {
                          int scroll = (row - 1) - tabs[tab_pos].tmp_row;
                          tabs[tab_pos].off_row = (tabs[tab_pos].line_cnt * scroll) / (height - 2);
                        } else if (tmp_dir != -1) {
                          int scroll = (row - 1) - tmp_dir;
                          off_dir = (dir_len * scroll) / (height - 2);
                        } else {
                          if (col < width && row > 1 && row < height) {
                            tabs[tab_pos].row = (row - 2) + tabs[tab_pos].off_row;

                            if (tabs[tab_pos].row < 0) {
                              tabs[tab_pos].row = 0;
                            }

                            if (tabs[tab_pos].row >= tabs[tab_pos].line_cnt) {
                              tabs[tab_pos].row = tabs[tab_pos].line_cnt - 1;
                            }    

                            int line_len = 0;
                            int line_tmp = tabs[tab_pos].line_cnt;

                            while (line_tmp) {
                              line_len++;
                              line_tmp /= 10;
                            }

                            if (line_len < 0) {
                              line_len++;
                            }

                            tabs[tab_pos].col = col - (line_len + (show_tree * BEDD_TREE) + 6);

                            if (tabs[tab_pos].col < 0) {
                              tabs[tab_pos].col = 0;
                            }

                            if (tabs[tab_pos].col > tabs[tab_pos].lines[tabs[tab_pos].row].length) {
                              tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
                            }

                            if (tabs[tab_pos].sel_row > tabs[tab_pos].row || (tabs[tab_pos].sel_row == tabs[tab_pos].row && tabs[tab_pos].sel_col > tabs[tab_pos].col)) {
                              tabs[tab_pos].sel_row = tabs[tab_pos].row;
                              tabs[tab_pos].sel_col = tabs[tab_pos].col;
                            }
                          } else if (row == 1) {
                            tabs[tab_pos].off_row--;

                            if (tabs[tab_pos].off_row < 0) {
                              tabs[tab_pos].off_row = 0;
                            } else if (tabs[tab_pos].off_row >= tabs[tab_pos].line_cnt - (height - 2)) {
                              if (tabs[tab_pos].line_cnt - (height - 2) > 0) {
                                tabs[tab_pos].off_row = tabs[tab_pos].line_cnt - (height - 2);
                              }
                            }
                          } else if (row == height) {
                            tabs[tab_pos].off_row++;
                            
                            if (tabs[tab_pos].off_row < 0) {
                              tabs[tab_pos].off_row = 0;
                            } else if (tabs[tab_pos].off_row >= tabs[tab_pos].line_cnt - (height - 2)) {
                              if (tabs[tab_pos].line_cnt - (height - 2) > 0) {
                                tabs[tab_pos].off_row = tabs[tab_pos].line_cnt - (height - 2);
                              }
                            }
                          }
                        }
                      }
                    }
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

                  while (c != 'M' && c != 'm') {
                    if (read(STDIN_FILENO, &c, 1) < 1) {
                      break;
                    }
                  }

                  if (tmp_dir == -1 && tabs[tab_pos].tmp_row == -1 && seq[2] == '0' && c == 'M') {
                    tabs[tab_pos].sel_row = tabs[tab_pos].row;
                    tabs[tab_pos].sel_col = tabs[tab_pos].col;
                  }
                }
              }
            }
          } else if (seq[0] == 'O') {
            if (seq[1] == 'H') {
              tabs[tab_pos].col = 0;
              tabs[tab_pos].sel_col = tabs[tab_pos].col;
            } else if (seq[1] == 'F') {
              tabs[tab_pos].col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
              tabs[tab_pos].sel_col = tabs[tab_pos].col;
            }
          }
        }
      } else {
        if (tabs[tab_pos].sel_row != tabs[tab_pos].row || tabs[tab_pos].sel_col != tabs[tab_pos].col) {
          bedd_delete(tabs + tab_pos);
        }

        bedd_write(tabs + tab_pos, c);
        
        tabs[tab_pos].step++;

        if (tabs[tab_pos].step >= BEDD_STEP) {
          bedd_push_undo(tabs + tab_pos);
          tabs[tab_pos].step = 0;
        }
      }
    } else if (first) {
      first = 0;
    } else {
      continue;
    }

    printf("\x1B[?25l");
    printf("\x1B[H" BEDD_NORMAL);

    bedd_tabs(tabs, tab_pos, tab_cnt, width);

    if (tabs[tab_pos].off_row < 0) {
      tabs[tab_pos].off_row = 0;
    } else if (tabs[tab_pos].off_row >= tabs[tab_pos].line_cnt - (height - 2)) {
      if (tabs[tab_pos].line_cnt - (height - 2) > 0) {
        tabs[tab_pos].off_row = tabs[tab_pos].line_cnt - (height - 2);
      } else {
        tabs[tab_pos].off_row = 0;
      }
    }

    if (off_dir < 0) {
      off_dir = 0;
    } else if (off_dir >= dir_len - (height - 2)) {
      if (dir_len - (height - 2) > 0) {
        off_dir = dir_len - (height - 2);
      } else {
        off_dir = 0;
      }
    }

    int scroll_start = ((tabs[tab_pos].off_row) * (height - 2)) / tabs[tab_pos].line_cnt;
    int scroll_end   = ((tabs[tab_pos].off_row + (height - 2)) * (height - 2)) / tabs[tab_pos].line_cnt;

    int dir_start = (off_dir * (height - 2)) / dir_len;
    int dir_end   = ((off_dir + (height - 2)) * (height - 2)) / dir_len;

    int line_len = 0;
    int line_tmp = tabs[tab_pos].line_cnt;

    while (line_tmp) {
      line_len++;
      line_tmp /= 10;
    }

    if (line_len < 0) {
      line_len++;
    }

    int row = tabs[tab_pos].off_row;
    int pos = 0;

    int state = 0;

    printf(BEDD_NORMAL BEDD_WHITE);

    if (show_tree) {
      dir_len = dir_tree(2 - off_dir, 1, height, ".") - (2 - off_dir);

      for (int i = dir_len; i < height - 1; i++) {
        printf("\x1B[%d;%dH", i + 2, 1);

        for (int j = 0; j < BEDD_TREE - 1; j++) {
          printf(" ");
        }
      }
    }

    for (int i = 0; i < height - 2; i++, row++) {
      printf("\x1B[%d;%dH", i + 2, 1 + (show_tree * (BEDD_TREE - 1)));
      printf(BEDD_NORMAL BEDD_WHITE);

      if (show_tree) {
        if (i >= dir_start && i <= dir_end) {
          printf(BEDD_INVERT);
        } else {
          printf(BEDD_NORMAL);
        }

        printf("|" BEDD_NORMAL);
      }

      if (row >= 0 && row < tabs[tab_pos].line_cnt) {
        if (tabs[tab_pos].row == row) {
          printf(BEDD_INVERT "  %*d  " BEDD_NORMAL " ", line_len, row + 1);
          pos = i + 2;
        } else {
          printf(BEDD_NORMAL "  %*d |" BEDD_NORMAL " ", line_len, row + 1);
        }

        for (int j = 0; j < tabs[tab_pos].lines[row].length && j < width - (line_len + (show_tree * BEDD_TREE) + 7); j++) {
          printf(BEDD_SELECT);

          if (row == tabs[tab_pos].sel_row) {
            if (j < tabs[tab_pos].sel_col) {
              printf(BEDD_DEFAULT);
            }
          }

          if (row == tabs[tab_pos].row) {
            if (j >= tabs[tab_pos].col) {
              printf(BEDD_DEFAULT);
            }
          }

          if (row < tabs[tab_pos].sel_row || row > tabs[tab_pos].row) {
            printf(BEDD_DEFAULT);
          }

          state = bedd_color(tabs + tab_pos, state, row, j);

          printf("%c", tabs[tab_pos].lines[row].buffer[j]);
        }

        printf(BEDD_WHITE BEDD_SELECT);

        if (row == tabs[tab_pos].sel_row) {
          if (tabs[tab_pos].lines[row].length < tabs[tab_pos].sel_col) {
            printf(BEDD_DEFAULT);
          }
        }

        if (row == tabs[tab_pos].row) {
          if (tabs[tab_pos].lines[row].length >= tabs[tab_pos].col) {
            printf(BEDD_DEFAULT);
          }
        }

        if (row < tabs[tab_pos].sel_row || row > tabs[tab_pos].row) {
          printf(BEDD_DEFAULT);
        }

        printf(" ");
      } else {
        printf(BEDD_NORMAL "  %*s :" BEDD_NORMAL " ", line_len, "");
      }

      printf(BEDD_DEFAULT "\x1B[K");
      printf("\x1B[%d;%dH", i + 2, width);

      if (i >= scroll_start && i <= scroll_end) {
        printf(BEDD_INVERT);
      } else {
        printf(BEDD_NORMAL);
      }

      printf("|");
      printf(BEDD_NORMAL "\r\n");
    }

    printf("\x1B[%d;%dH", height, 0);
    bedd_stat(tabs + tab_pos, status);

    if (strlen(status)) {
      status[0] = '\0';
    }

    int col = tabs[tab_pos].col;

    if (col > tabs[tab_pos].lines[tabs[tab_pos].row].length) {
      col = tabs[tab_pos].lines[tabs[tab_pos].row].length;
    }

    if (pos) {
      printf("\x1B[3 q");
      printf("\x1B[?25h");
      printf("\x1B[%d;%dH", pos, col + line_len + (show_tree * BEDD_TREE) + 6);
    }

    fflush(stdout);
  }

  printf("\x1B[?25h");
  printf("\x1B[?1000;1002;1006;1015l");
  printf("\x1B[2J\x1B[H" BEDD_NORMAL);
  printf("\x1B[?47l");
  printf("\x1B[1 q");

  return 0;
}