#include <string.h>
#include <syntax.h>

// COBOL

int st_cobol_depth(const char *line, int length);
char st_cobol_pair(const char *line, int length, char chr);
int st_cobol_color(int prev_state, int *state, const char *text, int length);

// C/C++

int st_c_depth(const char *line, int length);
char st_c_pair(const char *line, int length, char chr);
int st_c_color(int prev_state, int *state, const char *text, int length);

// JavaScript/JSON

int st_js_color(int prev_state, int *state, const char *text, int length);

// Markdown

int st_md_color(int prev_state, int *state, const char *text, int length);

// sh/bash

int st_sh_color(int prev_state, int *state, const char *text, int length);

// Rust

int st_rs_color(int prev_state, int *state, const char *text, int length);

// Default

static int st_depth(const char *line, int length);
static char st_pair(const char *line, int length, char chr);
static int st_color(int prev_state, int *state, const char *text, int length);

// syntax.h stuff

syntax_t st_init(const char *filename) {
  filename = strrchr(filename, '.');
  
  if (filename) {
    if (!strcasecmp(filename, ".h") || !strcasecmp(filename, ".c") ||
        !strcasecmp(filename, ".hh") || !strcasecmp(filename, ".cc") ||
        !strcasecmp(filename, ".hpp") || !strcasecmp(filename, ".cpp") ||
        !strcasecmp(filename, ".ij")) {
      return (syntax_t) {
        .lang = "C/C++",
        .f_depth = st_c_depth,
        .f_pair = st_c_pair,
        .f_color = st_c_color,
      };
    }
    
    if (!strcasecmp(filename, ".cbl") || !strcasecmp(filename, ".cob") || !strcasecmp(filename, ".cobol")) {
      return (syntax_t) {
        .lang = "COBOL",
        .f_depth = st_cobol_depth,
        .f_pair = st_cobol_pair,
        .f_color = st_cobol_color,
      };
    }
    
    if (!strcasecmp(filename, ".js")) {
      return (syntax_t) {
        .lang = "JavaScript",
        .f_depth = st_c_depth,
        .f_pair = st_c_pair,
        .f_color = st_js_color,
      };
    }
    
    if (!strcasecmp(filename, ".json")) {
      return (syntax_t) {
        .lang = "JSON",
        .f_depth = st_c_depth,
        .f_pair = st_c_pair,
        .f_color = st_js_color,
      };
    }
    
    if (!strcasecmp(filename, ".md")) {
      return (syntax_t) {
        .lang = "Markdown",
        .f_depth = st_depth,
        .f_pair = st_pair,
        .f_color = st_md_color,
      };
    }
    
    if (!strcasecmp(filename, ".rs")) {
      return (syntax_t) {
        .lang = "Rust",
        .f_depth = st_c_depth,
        .f_pair = st_c_pair,
        .f_color = st_rs_color,
      };
    }
    
    if (!strcasecmp(filename, ".sh")) {
      return (syntax_t) {
        .lang = "sh/bash script",
        .f_depth = st_depth,
        .f_pair = st_c_pair,
        .f_color = st_sh_color,
      };
    }
  }
  
  return (syntax_t) {
    .lang = "Text/Unknown",
    .f_depth = st_depth,
    .f_pair = st_pair,
    .f_color = st_color,
  };
}

static int st_depth(const char *line, int length) {
  line;
  length;
  return 0;
}

static char st_pair(const char *line, int length, char chr) {
  line;
  length;
  chr;
  return '\0';
}

static int st_color(int prev_state, int *state, const char *text, int length) {
  prev_state;
  text;
  length;
  *state = 0;
  
  return st_color_default;
}
