#ifndef __IO_H__
#define __IO_H__

#include <stdio.h>
#include <time.h>

// --- general functions ---

void io_init(void);
void io_exit(void);

// --- file/clipboard functions ---

typedef struct io_file_t io_file_t;

enum {
  io_file_file,
  io_file_directory,
  io_file_clipboard,
};

struct io_file_t {
  int type;
  void *data;
};

io_file_t io_fopen(const char *path, int write_mode);
int       io_fvalid(io_file_t file);
void      io_fclose(io_file_t file);
size_t    io_fwrite(io_file_t file, void *buffer, size_t count);
size_t    io_fread(io_file_t file, void *buffer, size_t count);
void      io_frewind(io_file_t file);
int       io_feof(io_file_t file);

void      io_dsolve(const char *path, char *buffer);
io_file_t io_dopen(const char *path);
int       io_dvalid(io_file_t file);
void      io_dclose(io_file_t file);
int       io_dread(io_file_t file, char *buffer);
void      io_drewind(io_file_t file);

io_file_t io_copen(void);
void      io_cclose(void);


// --- output functions ---

#define IO_NORMAL        "\x1B[0m\x1B[49m"
#define IO_INVERT        "\x1B[0m\x1B[1m\x1B[7m\x1B[49m"
#define IO_BOLD          "\x1B[0m\x1B[1m\x1B[49m"
#define IO_UNDERLINE     "\x1B[0m\x1B[4m\x1B[49m"
#define IO_NO_SHADOW     "\x1B[49m"
#define IO_SHADOW_1      "\x1B[22m\x1B[24m\x1B[27m\x1B[40m"
#define IO_SHADOW_2      "\x1B[22m\x1B[24m\x1B[27m\x1B[100m"
#define IO_BOLD_SHADOW_1 "\x1B[0m\x1B[1m\x1B[40m"
#define IO_BOLD_SHADOW_2 "\x1B[0m\x1B[1m\x1B[100m"

#define IO_WHITE     "\x1B[22m\x1B[97m"
#define IO_BLACK     "\x1B[22m\x1B[30m"
#define IO_RED       "\x1B[22m\x1B[31m"
#define IO_GREEN     "\x1B[22m\x1B[32m"
#define IO_YELLOW    "\x1B[22m\x1B[33m"
#define IO_BLUE      "\x1B[22m\x1B[34m"
#define IO_PURPLE    "\x1B[22m\x1B[35m"
#define IO_CYAN      "\x1B[22m\x1B[36m"
#define IO_GRAY      "\x1B[22m\x1B[37m"
#define IO_DARK_GRAY "\x1B[22m\x1B[90m"

#define IO_BOLD_WHITE     "\x1B[1m\x1B[97m"
#define IO_BOLD_BLACK     "\x1B[1m\x1B[30m"
#define IO_BOLD_RED       "\x1B[1m\x1B[31m"
#define IO_BOLD_GREEN     "\x1B[1m\x1B[32m"
#define IO_BOLD_YELLOW    "\x1B[1m\x1B[33m"
#define IO_BOLD_BLUE      "\x1B[1m\x1B[34m"
#define IO_BOLD_PURPLE    "\x1B[1m\x1B[35m"
#define IO_BOLD_CYAN      "\x1B[1m\x1B[36m"
#define IO_BOLD_GRAY      "\x1B[1m\x1B[37m"
#define IO_BOLD_DARK_GRAY "\x1B[1m\x1B[90m"

#define IO_CLEAR_LINE   "\x1B[K"
#define IO_CLEAR_SCREEN "\x1B[2J\x1B[H"
#define IO_CLEAR_CURSOR "\x1B[J"

#define IO_ARROW_UP    IO_EXTRA(28)
#define IO_ARROW_DOWN  IO_EXTRA(29)
#define IO_ARROW_RIGHT IO_EXTRA(30)
#define IO_ARROW_LEFT  IO_EXTRA(31)
#define IO_HOME        IO_EXTRA(32)
#define IO_END         IO_EXTRA(33)
#define IO_PAGE_UP     IO_EXTRA(34)
#define IO_PAGE_DOWN   IO_EXTRA(35)

#define IO_EXTRA(chr) ((chr) | 256)

#define IO_CTRL(chr)  ((chr) & (31 | ~511))
#define IO_SHIFT(chr) ((chr) | 512)
#define IO_ALT(chr)   ((chr) | 1024)

#define IO_UNSHIFT(chr) ((chr) & ~512)
#define IO_UNALT(chr)   ((chr) & ~1024)

void io_cursor(int x, int y);
void io_printf(const char *format, ...);
int  io_printf_wrap(int x, int width, int y, const char *format, ...); // returns y axis value
void io_flush(void);

// --- input functions ---

#define IO_EVENT_KEY_PRESS   1 // contains flags for Ctrl, Alt, Shift, etc.
#define IO_EVENT_MOUSE_DOWN  2
#define IO_EVENT_MOUSE_MOVE  3
#define IO_EVENT_MOUSE_UP    4
#define IO_EVENT_SCROLL      5
#define IO_EVENT_TIME_SECOND 6 // sent everytime the UNIX time changes
#define IO_EVENT_RESIZE      7

typedef struct io_event_t io_event_t;

struct io_event_t {
  int type;
  
  union {
    int key;
    
    struct {
      int x, y;
    } mouse;
    
    int scroll; // 1 if down, -1 if up
    
    time_t time;
    
    struct {
      int width, height;
    } size;
  };
};

io_event_t io_get_event(void);

#endif
