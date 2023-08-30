#ifndef __THEME_H__
#define __THEME_H__

enum {
  theme_17,
  theme_16,
  theme_16_tty,
  theme_cga_black,
  theme_cga_blue,
  theme_crt_green,
  theme_crt_amber,
  theme_solarized_dark,
  
  theme_count,
};

enum {
  theme_back_0, // "\x0E"
  theme_back_1,
  theme_back_2,
  
  theme_fore_0,
  theme_fore_1,
  theme_fore_2,
  theme_fore_3,
  theme_fore_4,
  theme_fore_5,
  theme_fore_6,
  theme_fore_7,
  theme_fore_8,
  theme_fore_9, // "\x1A"
  
  theme_back_3, // "\x1C" (jump)
};

extern const char *theme_names[];

void theme_apply(char *buffer);

#endif
