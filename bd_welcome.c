#include <string.h>
#include <bedd.h>
#include <io.h>

void bd_welcome_draw(bd_view_t *view) {
  io_cursor(0, 2);
  io_printf(IO_CLEAR_CURSOR);
  
  const char *title = "Welcome to the bedd editor!";
  
  io_cursor((bd_width - strlen(title)) / 2, 3);
  io_printf(IO_UNDERLINE "%s" IO_NORMAL, title);
  
  int y = io_printf_wrap(2, bd_width - 4, 5,
    "bedd is an open source, multifunctional editor and enviroment, with purposes ranging from file exploring to coding and note taking. "
    "Its name comes from 'bar editor', as everything revolves around the tabbed bar.\n\n"
  );
  
  io_printf_wrap(2, bd_width, y,
    IO_UNDERLINE "General:" IO_NORMAL "\n"
    IO_BOLD " Ctrl+Q " IO_NORMAL "Close tab/Exit\n"
    IO_BOLD " Ctrl+W " IO_NORMAL "Welcome tab\n"
    IO_BOLD " Ctrl+T " IO_NORMAL "Terminal tab\n"
    IO_BOLD " Ctrl+N " IO_NORMAL "Empty tab\n"
    IO_BOLD " Ctrl+O " IO_NORMAL "Open file/dir.\n"
    IO_BOLD " Ctrl+A " IO_NORMAL "Select all\n"
    IO_BOLD " Ctrl+Up " IO_NORMAL "Go top\n"
    IO_BOLD " Ctrl+Down " IO_NORMAL "Go bottom\n"
    IO_BOLD " Ctrl+Left " IO_NORMAL "Prev. tab\n"
    IO_BOLD " Ctrl+Right " IO_NORMAL "Next tab\n"
    IO_BOLD " [\u2190\u2191\u2192\u2193] " IO_NORMAL "Move\n\n"
    IO_UNDERLINE "Dialog box:" IO_NORMAL "\n"
    IO_BOLD " Enter " IO_NORMAL "Accept\n"
  );
  
  io_printf_wrap(2 + (bd_width / 3), bd_width, y,
    IO_UNDERLINE "Text tab:" IO_NORMAL "\n"
    IO_BOLD " Ctrl+S " IO_NORMAL "Save file\n"
    IO_BOLD " Ctrl+C " IO_NORMAL "Copy\n"
    IO_BOLD " Ctrl+X " IO_NORMAL "Cut\n"
    IO_BOLD " Ctrl+V " IO_NORMAL "Paste\n"
    IO_BOLD " Ctrl+F " IO_NORMAL "Find/Replace\n"
    IO_BOLD " Ctrl+Z " IO_NORMAL "Undo\n"
    IO_BOLD " Ctrl+Y " IO_NORMAL "Redo\n"
    IO_BOLD " Ctrl+K " IO_NORMAL "Comment/Uncomment\n"
    IO_BOLD " Shift+[\u2190\u2191\u2192\u2193] " IO_NORMAL "Select\n\n"
    IO_UNDERLINE "Terminal tab:" IO_NORMAL "\n"
    IO_BOLD " Ctrl+Shift+C " IO_NORMAL "Copy\n"
    IO_BOLD " Ctrl+Shift+V " IO_NORMAL "Paste\n"
  );
  
  io_printf_wrap(2 + ((2 * bd_width) / 3), bd_width, y,
    IO_UNDERLINE "File explorer tab:" IO_NORMAL "\n"
    IO_BOLD " Enter " IO_NORMAL "Open\n"
    IO_BOLD " Alt+Enter " IO_NORMAL "Open new tab\n"
    IO_BOLD " Space " IO_NORMAL "Select/Deselect\n"
    IO_BOLD " Delete " IO_NORMAL "Delete\n"
    IO_BOLD " Ctrl+U " IO_NORMAL "Refresh dir.\n"
    IO_BOLD " Ctrl+C " IO_NORMAL "Copy\n"
    IO_BOLD " Ctrl+X " IO_NORMAL "Cut\n"
    IO_BOLD " Ctrl+V " IO_NORMAL "Paste\n"
    IO_BOLD " Ctrl+R " IO_NORMAL "Rename\n"
    IO_BOLD " Ctrl+K " IO_NORMAL "New directory\n"
    IO_BOLD " Ctrl+L " IO_NORMAL "New file\n"
  );
  
  io_flush();
  
  view->cursor = (bd_cursor_t){-1, -1};
}
