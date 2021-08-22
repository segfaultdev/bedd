#ifndef __CUPD_H__
#define __CUPD_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#ifndef CUPD_FILE
#define CUPD_FILE NULL
#endif

#ifndef CUPD_ARGS
#define CUPD_ARGS ""
#endif

static char **cupd_stack;
static int    cupd_count;
static int    cupd_total;
 
static int cupd_check(const char *path, time_t time) {
  struct dirent *entry;
  struct stat file;
  DIR *dir;

  if (!(dir = opendir(path))) {
    return 0;
  }

  while ((entry = readdir(dir)) != NULL) {
    char new_path[strlen(path) + strlen(entry->d_name) + 2];  
    sprintf(new_path, "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
        continue;
      }

      if (cupd_check(new_path, time)) {
        return 1;
      }
    } else {
      if (stat(new_path, &file) < 0) {
        continue;
      }

      if (strcmp(new_path + (strlen(new_path) - 2), ".c") && strcmp(new_path + (strlen(new_path) - 2), ".h")) {
        continue;
      }

      if (file.st_mtime < time) {
        continue;
      }

      return 1;
    }
  }

  closedir(dir);
  return 0;
}

static void cupd_build(const char *path) {
  DIR *dir;

  if (!(dir = opendir(path))) {
    return;
  }

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    char new_path[strlen(path) + strlen(entry->d_name) + 2];  
    sprintf(new_path, "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
        continue;
      }

      cupd_build(new_path);
    } else {
      size_t length = strlen(new_path);

      if (strcmp(new_path + (length - 2), ".c")) {
        continue;
      }  

      char out_path[length + 1];
      strcpy(out_path, new_path);

      out_path[length - 1] = 'o';

      char buffer[strlen(CUPD_ARGS) + (2 * length) + 12];
      sprintf(buffer, "cc -c %s%s%s -o %s", CUPD_ARGS, strlen(CUPD_ARGS) ? " " : "", new_path, out_path);

      printf("[cupd] %s\n", buffer);
      system(buffer); // ik, system iz bad, but hey, it works!

      // add to the file stack
      cupd_stack = realloc(cupd_stack, (cupd_count + 1) * sizeof(char *));
      
      cupd_stack[cupd_count] = malloc(length + 1);
      strcpy(cupd_stack[cupd_count], out_path);

      cupd_total += length + 1;
      cupd_count++;
    }
  }

  closedir(dir);
}

static void cupd_clean(const char *path) {
  DIR *dir;

  if (!(dir = opendir(path))) {
    return;
  }

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    char new_path[strlen(path) + strlen(entry->d_name) + 2];  
    sprintf(new_path, "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
        continue;
      }

      cupd_clean(new_path);
    } else {
      size_t length = strlen(new_path);

      if (strcmp(new_path + (length - 2), ".o")) {
        continue;
      }

      char buffer[length + 4];
      sprintf(buffer, "rm %s", new_path);

      printf("[cupd] %s\n", buffer);
      system(buffer); // as i said, ik system bad
    }
  }

  closedir(dir);
}

static void cupd_init(int argc, const char **argv) {
  if (CUPD_FILE == NULL) {
    printf("[cupd] CUPD_FILE is not defined, set it to the filename of\n");
    printf("       the main file of the project to build(e.g. \"main.c\")\n");
    exit(1);
  }

  if (CUPD_FILE == __FILE__) {
    printf("[cupd] CUPD_FILE cannot be __FILE__, use filename instead\n");
    exit(1);
  }

  struct stat code, prog;

  if (stat(CUPD_FILE, &code) < 0) {
    // the main file is not in the current folder
    return;
  }

  if (stat(argv[0], &prog) < 0) {
    // the program is not in the current folder
    return;
  }

  if (cupd_check(".", prog.st_mtime)) {
    cupd_stack = NULL;
    cupd_count = 0;
    cupd_total = 0;

    cupd_build(".");

    if (cupd_count) {
      int length = 0;

      for (int i = 0; i < argc; i++) {
        length += strlen(argv[i] + 1);
      }

      char buffer_1[(strlen(argv[0]) * 2) + 9];
      char buffer_2[cupd_total + strlen(argv[0]) + strlen(CUPD_ARGS) + 8];
      char buffer_3[(strlen(argv[0]) * 2) + 9];
      char buffer_4[(strlen(argv[0]) * 2) + 42];
      char buffer_5[strlen(argv[0]) + 8];
      char buffer_6[length];

      sprintf(buffer_1, "cp %s %s.old", argv[0], argv[0]);
      sprintf(buffer_2, "cc -o %s%s%s", argv[0], strlen(CUPD_ARGS) ? " " : "", CUPD_ARGS);
      sprintf(buffer_3, "mv %s.old %s", argv[0], argv[0]);
      sprintf(buffer_4, "touch -d \"$(date -R -r %s) - 1 minute\" %s", argv[0], argv[0]);
      sprintf(buffer_5, "rm %s.old", argv[0]);

      for (int i = 0; i < cupd_count; i++) {
        strcat(buffer_2, " ");
        strcat(buffer_2, cupd_stack[i]);
      }

      buffer_6[0] = '\0';

      for (int i = 0; i < argc; i++) {
        if (i) {
          strcat(buffer_6, " ");
        }

        strcat(buffer_6, argv[i]);
      }

      printf("[cupd] %s\n", buffer_1);
      system(buffer_1); // SHUTUP

      printf("[cupd] %s\n", buffer_2);
      system(buffer_2); // NO

      cupd_clean(".");
      free(cupd_stack);

      if (stat(argv[0], &prog) < 0) {
        printf("[cupd] %s\n", buffer_3);
        system(buffer_3); // i will use it no matter how bad it is

        printf("[cupd] %s\n", buffer_4);
        system(buffer_4); // don't even try

        printf("[cupd] %s could not be updated\n", argv[0]);
      } else {
        printf("[cupd] %s\n", buffer_4);
        system(buffer_5); // system gud

        printf("[cupd] %s succefully updated\n", argv[0]);

        printf("[cupd] %s\n", buffer_5);
        system(buffer_6); // no i won't change it
      }
    }
    
    exit(0);
  }
}

#endif