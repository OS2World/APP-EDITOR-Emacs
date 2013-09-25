/* pmemacs.h
   Copyright (C) 1993-1996 Eberhard Mattes.

This file is part of GNU Emacs.

GNU Emacs is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#define DEFAULT_FONT "-*-Courier-medium-r-normal--*-100-*-*-m-*-cp850"

typedef struct
{
  int portion, whole, position;
} slider;

/* Requests */

typedef enum
{
  PMR_CURSOR,                   /* Move the (visible) cursor */
  PMR_GLYPHS,                   /* Display glyphs */
  PMR_CLEAR,                    /* Clear a frame */
  PMR_CLREOL,                   /* Clear to end of line */
  PMR_BELL,                     /* Ring the bell */
  PMR_CREATE,                   /* Create a frame */
  PMR_DESTROY,                  /* Destroy a frame */
  PMR_VISIBLE,                  /* Make frame visible or invisible */
  PMR_NAME,                     /* Set frame name */
  PMR_FOCUS,                    /* Set focus to frame */
  PMR_ICONIFY,                  /* Turn window into an icon */
  PMR_SIZE,                     /* Set size of frame  */
  PMR_LINES,                    /* Insert or delete lines */
  PMR_POPUPMENU,                /* Popup menu */
  PMR_MODIFY,                   /* Modify parameters */
  PMR_MOUSEPOS,                 /* Query mouse position */
  PMR_PASTE,                    /* Get text from clipboard */
  PMR_CUT,                      /* Copy text to clipboard */
  PMR_MENUBAR,                  /* Set menubar */
  PMR_QUITCHAR,                 /* Set the quit character */
  PMR_CLOSE,                    /* Close connection */
  PMR_RAISE,                    /* Raise frame */
  PMR_LOWER,                    /* Lower frame */
  PMR_FACE,                     /* Define a face */
  PMR_FONTLIST,                 /* Get list of fonts */
  PMR_TRACKMOUSE,               /* Turn on/off mouse tracking */
  PMR_FRAMEPOS,                 /* Get position of frame window */
  PMR_SETPOS,                   /* Set positition of frame window */
  PMR_CONFIG,                   /* Get number of planes etc. */
  PMR_DIALOG,                   /* Display dialog box */
  PMR_DROP,                     /* Get name of dropped object */
  PMR_INITIALIZE,               /* Initialization */
  PMR_CPLIST,                   /* Get list of code pages */
  PMR_CODEPAGE,                 /* Set the code page */
  PMR_FILEDIALOG,               /* Show and process a file dialog */
  PMR_CREATEDONE,               /* Frame creation finished */
  PMR_CREATE_SCROLLBAR,         /* Create a scroll bar */
  PMR_UPDATE_SCROLLBAR,         /* Update slider of a scroll bar */
  PMR_MOVE_SCROLLBAR,           /* Move position of a scroll bar */
  PMR_DESTROY_SCROLLBAR         /* Destroy a scroll bar */
} pm_request_type;

typedef struct
{
  pm_request_type type;
  unsigned long frame;
} pmr_header;

typedef struct
{
  pmr_header header;
  int height, width;
} pmr_create;

typedef struct
{
  pmr_header header;
  int x, y, on;
} pmr_cursor;

typedef struct
{
  pmr_header header;
  int count, x, y, face;
} pmr_glyphs;

typedef struct
{
  pmr_header header;
  int x0, x1, y;
} pmr_clreol;

typedef struct
{
  pmr_header header;
  int visible;
} pmr_bell;

typedef struct
{
  pmr_header header;
  int visible;
} pmr_visible;

typedef struct
{
  pmr_header header;
  int count;
} pmr_name;

typedef struct
{
  pmr_header header;
  int width, height;
} pmr_size;

typedef struct
{
  pmr_header header;
  int y, max_y;
  int count;                    /* Insert this many lines if positive */
} pmr_lines;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
  int button;
  int align_top;
  int x, y;
  int count;
  int size;                     /* Size of data to follow */
} pmr_popupmenu;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
} pmr_mousepos;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
  int get_text;
} pmr_paste;

typedef struct
{
  pmr_header header;
  unsigned long size;           /* Size of data to follow */
} pmr_cut;

typedef struct
{
  pmr_header header;
  int entries;                  /* Number of pm_menu structures */
  int size;                     /* Size of data to follow */
} pmr_menubar;

typedef struct
{
  pmr_header header;
  int quitchar;                 /* The quit character */
} pmr_quitchar;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
  int foreground;               /* Foreground color */
  int background;               /* Background color */
  int name_length;              /* Length of font name to follow */
  char underline;               /* Underline */
} pmr_face;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
  int pattern_length;           /* Length of string to follow */
} pmr_fontlist;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
  int count;
  int size;                     /* Size of data to follow */
  int cookie;                   /* Magic cookie */
} pmr_menu;

typedef struct
{
  pmr_header header;
  int flag;
} pmr_track;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
} pmr_framepos;

typedef struct
{
  pmr_header header;
  int top, top_base, left, left_base;
} pmr_setpos;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
} pmr_config;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
  int buttons;
  int count;
  int size;                     /* Size of data to follow */
} pmr_dialog;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number of answer */
  int cookie;
} pmr_drop;

typedef struct
{
  pmr_header header;
  int codepage;
} pmr_initialize;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number for answer */
} pmr_cplist;

typedef struct
{
  pmr_header header;
  int serial;                   /* Serial number of answer */
  int codepage;
} pmr_codepage;

typedef struct
{
  pmr_header header;
  unsigned id;
  slider s;
} pmr_update_scrollbar;

typedef struct
{
  pmr_header header;
  unsigned id;
  int top, left, width, height;
} pmr_move_scrollbar;

typedef struct
{
  pmr_header header;
  unsigned id;
} pmr_destroy_scrollbar;

typedef union
{
  pmr_header header;
  pmr_create create;
  pmr_cursor cursor;
  pmr_glyphs glyphs;
  pmr_clreol clreol;
  pmr_bell bell;
  pmr_visible visible;
  pmr_name name;
  pmr_size size;
  pmr_lines lines;
  pmr_popupmenu popupmenu;
  pmr_mousepos mousepos;
  pmr_paste paste;
  pmr_cut cut;
  pmr_menubar menubar;
  pmr_quitchar quitchar;
  pmr_face face;
  pmr_fontlist fontlist;
  pmr_menu menu;
  pmr_track track;
  pmr_framepos framepos;
  pmr_setpos setpos;
  pmr_config config;
  pmr_dialog dialog;
  pmr_drop drop;
  pmr_initialize initialize;
  pmr_cplist cplist;
  pmr_codepage codepage;
  pmr_update_scrollbar update_scrollbar;
  pmr_move_scrollbar move_scrollbar;
  pmr_destroy_scrollbar destroy_scrollbar;
} pm_request;

/* Additional data for PMR_POPUPMENU and PMR_MENUBAR. */

typedef struct
{
  enum
    {
      PMMENU_END, PMMENU_PUSH, PMMENU_POP, PMMENU_ITEM,
      PMMENU_SUB, PMMENU_TITLE, PMMENU_SEP, PMMENU_TOP
    } type;
  int item;
  int str_offset;
  int enable;
} pm_menu;

/* Additional data for PMR_MODIFY */

#define CURSORTYPE_BOX          1
#define CURSORTYPE_BAR          2
#define CURSORTYPE_FRAME        3
#define CURSORTYPE_UNDERLINE    4
#define CURSORTYPE_HALFTONE     5

#define COLOR_NONE              (-99)

#define PMR_FALSE               1
#define PMR_TRUE                2

#define SHORTCUT_SET            0x80000000
#define SHORTCUT_ALT            0x00000001
#define SHORTCUT_ALTGR          0x00000002
#define SHORTCUT_F1             0x00000004
#define SHORTCUT_F10            0x00000008
#define SHORTCUT_ALT_F4         0x00000010
#define SHORTCUT_ALT_F5         0x00000020
#define SHORTCUT_ALT_F6         0x00000040
#define SHORTCUT_ALT_F7         0x00000080
#define SHORTCUT_ALT_F8         0x00000100
#define SHORTCUT_ALT_F9         0x00000200
#define SHORTCUT_ALT_F10        0x00000400
#define SHORTCUT_ALT_F11        0x00000800
#define SHORTCUT_ALT_SPACE      0x00001000

typedef struct
{
  int width, height;
  int top, top_base, left, left_base;
  int background_color;
  int cursor_type, cursor_width, cursor_blink;
  int alt_modifier, altgr_modifier;
  unsigned shortcuts;
  char buttons[3];
  char default_font[100];
  char menu_font[60];           /* see also pmemacs.c */
  int menu_font_set;
  int sb_width;
  int var_width_fonts;
} pm_modify;

/* Additional data for PMR_FILEDIALOG */

typedef struct
{
  int serial;
  int save_as;
  int must_match;
  char title[80];
  char ok_button[40];
  char dir[260];                /* CCHMAXPATH */
  char defalt[260];             /* CCHMAXPATH */
} pm_filedialog;

/* Additional data for PMR_CREATE_SCROLLBAR */

typedef struct
{
  pmr_header header;
  int top, left, width, height;
  int serial;
  slider s;
} pm_create_scrollbar;


/* Events */

typedef enum
{
  PME_PAINT,                    /* Repaint a frame */
  PME_KEY,                      /* Key pressed */
  PME_BUTTON,                   /* Mouse button */
  PME_SIZE,                     /* Size of frame changed */
  PME_RESTORE,                  /* Frame window restored  */
  PME_MENUBAR,                  /* Menubar selection */
  PME_MOUSEMOVE,                /* Mouse moved */
  PME_MINIMIZE,                 /* Frame window minimized */
  PME_FRAMEMOVE,                /* Frame window moved */
  PME_SCROLLBAR,                /* Scrollbar event occured */
  PME_WINDOWDELETE,             /* Close a window */
  PME_ANSWER                    /* Answer for a request */
} pm_event_type;

typedef enum
{
  PMK_ASCII,
  PMK_VIRTUAL
} pm_key_type;

typedef struct
{
  pm_event_type type;
  unsigned long frame;
} pme_header;

typedef struct
{
  pme_header header;
  int x0, x1, y0, y1;
} pme_paint;

typedef struct
{
  pme_header header;
  pm_key_type type;
  int code, modifiers;
} pme_key;

typedef struct
{
  pme_header header;
  int width, height;
  int pix_width, pix_height;
} pme_size;

typedef struct
{
  pme_header header;
  int button, modifiers, x, y;
  unsigned long timestamp;
} pme_button;

typedef struct
{
  pme_header header;
  int number;
} pme_menubar;

typedef struct
{
  pme_header header;
  int x, y;
} pme_mouse;

typedef struct
{
  pme_header header;
  int top, left;
} pme_framemove;

typedef struct
{
  pme_header header;
  unsigned id;
  int button, modifiers, part, x, y;
  unsigned long timestamp;
} pme_scrollbar;

typedef struct
{
  pme_header header;
  int serial, size, one_word;
} pme_answer;

typedef union
{
  pme_header header;
  pme_paint paint;
  pme_key key;
  pme_size size;
  pme_button button;
  pme_menubar menubar;
  pme_mouse mouse;
  pme_framemove framemove;
  pme_scrollbar scrollbar;
  pme_answer answer;
} pm_event;


/* Answers */

typedef struct
{
  unsigned long frame;
  int x, y;
} pmd_mousepos;

typedef struct
{
  int count;
} pmd_fontlist;

typedef struct
{
  int top, left;
  int pix_width, pix_height;
} pmd_framepos;

typedef struct
{
  int planes;
  int color_cells;
  int width;
  int height;
  int width_mm;
  int height_mm;
} pmd_config;

#define SB_INVALID_ID   ((unsigned)(-1))

typedef struct
{
  unsigned id;
} pmd_create_scrollbar;
