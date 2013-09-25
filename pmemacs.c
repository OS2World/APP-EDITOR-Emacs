/* pmemacs.c -- OS/2 Presentation Manager interface for GNU Emacs.
   Copyright (C) 1993-1996 Eberhard Mattes.
   Copyright (C) 1995 Patrick Nadeau (scroll bar code).

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


#include <config.h>
#include <stdio.h>
#define _UCHAR_T
#include "lisp.h"
#include "termhooks.h"

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_GPI
#define INCL_DEV
#include <os2.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "pmemacs.h"

#define HASH_SIZE 53

#define DIALOG_CODE_PAGE 850

#define UWM_CREATE              (WM_USER+1000) /* Create a window */
#define UWM_DESTROY             (WM_USER+1001) /* Destroy a window */
#define UWM_POPUPMENU           (WM_USER+1002) /* Create a popup menu */
#define UWM_DIALOG              (WM_USER+1003) /* Create a dialog box */
#define UWM_MENUBAR             (WM_USER+1004) /* Update the menubar */
#define UWM_FILEDIALOG          (WM_USER+1005) /* Create a file dialog box */
#define UWM_CREATE_SCROLLBAR    (WM_USER+1006) /* Create a scrollbar */
#define UWM_DESTROY_SCROLLBAR   (WM_USER+1007) /* Destroy a scrollbar */
#define UWM_INITIAL_SHOW        (WM_USER+1008) /* Call initial_show_frame() */

/* Cf. lisp/term/pm-win.el. */
#define VK_CLOSE_FRAME          57 /* 0x39 */
#define VK_KP_CENTER            58
#define VK_KP_ADD               59
#define VK_KP_SUBTRACT          60
#define VK_KP_MULTIPLY          61
#define VK_KP_DIVIDE            62
#define VK_KP_0                 63
#define VK_KP_1                 64
#define VK_KP_2                 65
#define VK_KP_3                 66
#define VK_KP_4                 67
#define VK_KP_5                 68
#define VK_KP_6                 69
#define VK_KP_7                 70
#define VK_KP_8                 71
#define VK_KP_9                 72
#define VK_KP_SEPARATOR         73
#define VK_KP_DECIMAL           74

#define BUTTON_DROP_FILE        5    /* See keyboard.c */
#define BUTTON_DROP_COLOR       6

#define KC_ALTGR                0x10000

#define IDM_MENU                    1
#define IDM_MENU_LAST            9999
#define IDM_MENUBAR             10000
#define IDM_MENUBAR_LAST        19999
#define IDM_MENUBAR_TOP         20000
#define IDM_SUBMENU             21000

/* Maximum number of scrollbars */
#define MAX_SCROLLBARS          128

/* Window ID of first scrollbar.  IDs FID_SCROLLBAR through
   FID_SCROLLBAR + MAX_SCROLLBARS - 1 are reserved for scrollbars.
   Most of our windows just use an ID of zero.  */
#define FID_SCROLLBAR           256

/* The font_spec structure contains a font specification derived from
   a font name string. */
typedef struct
{
  /* The size of the font, in printer's points multipled by 10. */
  int size;

  /* Font selection: FATTR_SEL_UNDERSCORE etc. */
  int sel;

  /* The face name of the font. */
  char name[FACESIZE];
} font_spec;

/* The face_data structure defines a face (a font with attributes). */
typedef struct
{
  COLOR foreground;
  COLOR background;
  int font_id;                  /* Zero if no font associated */
  font_spec spec;
} face_data;

/* The font_data structure defines a font. */
typedef struct
{
  /* Face name, size and selection of the font. */
  font_spec spec;

  /* Size of character box, for outline fonts.  This value is used for
     both cx and cy.  This value is undefined for bitmap fonts. */
  FIXED cbox_size;

  /* This flag is non-zero if an outline font is selected. */
  char outline;

  /* This flag is non-zero if the width of the font is wrong or if the
     font is an outline font -- we have to use GpiCharStringPosAt
     instead of GpiCharStringAt in that case. */
  char use_incr;
} font_data;


/* The frame_data structure holds all the data for a frame.  A pointer
   to this structure is stored at position 0 of the client window data
   area. */
typedef struct frame_data
{

    /* This member is used for chaining frames that share the same hash
       code. */
    struct frame_data *next;

    /* The width and height of the client window. */
    LONG cxClient, cyClient;

    /* Some font metrics: the character width, character height and
       descender height of the font.  All values are given in pixels. */
    LONG cxChar, cyChar, cyDesc;

    /* The frame window handle. */
    HWND hwndFrame;

    /* The client window handle.  The client window is a child window of
       the frame window HWNDFRAME. */
    HWND hwndClient;

    /* This presentation space handle is used for asynchronous painting
       of the client window. */
    HPS hpsClient;

    /* The background color of the default face of this frame, */
    COLOR background_color;

    /* The position of the cursor, in characters. */
    int cursor_x, cursor_y;

    /* This member is non-zero if the cursor should be visible. */
    int cursor_on;

    /* The type of the cursor, one of the CURSORTYPE_whatever constants. */
    int cursor_type;

    /* The width of the cursor, for CURSORTYPE_BAR. */

    int cursor_width;

    /* Whether the cursor is blinking or not. */
    int cursor_blink;

    /* The width and height, respectively, in characters of this frame. */
    int width, height;

    /* The width of the vertical scrollbars, in characters. */
    int sb_width;

    /* The lower 3 bits of this member are set if WM_BUTTONxUP should be
       ignored until WM_BUTTONxDOWN is received.  This member is set to 7
       (all three bits set) when a button message is received while the
       window doesn't have the focus.  A bit is cleared if a
       WM_BUTTONxDOWN message is received.  Bit 0 is used for button 1,
       bit 1 is used for button 2, and bit 2 is used for button 3. */
    int ignore_button_up;

    /* The identity of the frame.  In Emacs, this is the frame pointer.
       As we do hashing on this value, it is cast to unsigned long in
       pmterm.c before passing to this module. */
    ULONG id;

    /* If there is a popup menu, this member holds the handle for that
       menu.  Otherwise, it is NULL. */
    HWND hwndPopupMenu;

    /* If there is a menubar, this member holds the handle for that
       menu.  Otherwise, it is NULL. */
    HWND hwndMenubar;

    /* This array is used for mapping menu IDs of the menubar to
       selection indices used by Emacs. */
    int *menubar_id_map;

    /* The number of elements allocated for `menubar_id_map'. */
    int menubar_id_allocated;

    /* The number of elements currently used in `menubar_id_map'. */
    int menubar_id_used;

    /* The following members hold the pm_menu structures and strings
       which were used most recently for building the menubar.  This
       data is used to decide whether the menubar has changed. */
    int cur_menubar_el_allocated;
    pm_menu *cur_menubar_el;
    int cur_menubar_str_allocated;
    char *cur_menubar_str;

    /* The following fields are used for recording the menubar contents
       while a frame has not yet been completely created. */
    int initial_menubar_el_count;
    int initial_menubar_str_bytes;

    /* This flag is set if a popup menu is active. */
    int popup_active;

    /* This is the default font of this frame. */
    font_spec font;

    /* This is the font (size.name, for instance "8.Helv") for menus of
       this frame.  See also pmemacs.h. */
    char menu_font[64];

    /* This field records the stage of frame creation. */
    enum
    {
        STAGE_CREATING,             /* pm-create-frame in progress */
        STAGE_INVISIBLE,            /* pm-create-frame completed, invisible */
        STAGE_GETTING_READY,        /* Transition from INVISIBLE to READY */
        STAGE_READY                 /* Frame creation completed */
    } stage;

    /* This flag is valid only if the STAGE field above is less than
       STAGE_READY.  It indicates whether the frame is visible (1),
       invisible (0), or iconified (-1). */
    int initial_visibility;

    /* While creating the frame, the position is kept here.  These
       fields are valid while STAGE is less than STAGE_READY. */
    int initial_left, initial_left_base;
    int initial_top, initial_top_base;

    /* This flag is non-zero when the window is in the process of being
       minimized.  It is set when a WM_MINMAXFRAME sent to the frame
       window indicates that the window is being minimized.  The flag is
       used when processing the WM_SIZE message: if it is set, WM_SIZE
       is ignored and the flag is cleared. */
    int minimizing;

    /* This flag is non-zero when the window is in the process of being
       restored from minimized state.  It is set when a WM_MINMAXFRAME
       sent to the frame window indicates that the window is being
       restored.  The flag is used when processing the WM_SIZE message:
       if it is set, WM_SIZE clears this flag and calls set_size() in
       case the size has been changed while the window was minimized. */
    int restoring;

    /* This flag is non-zero if the window is minimized. */
    int minimized;

    /* Ignore WM_SIZE if this flag is non-zero. */
    int ignore_wm_size;

    /* The Emacs modifier bit to be sent if the left Alt key is
       pressed. */
    int alt_modifier;

    /* The Emacs modifier bit to be sent if the right Alt key (AltGr on
       most non-US keyboards) is pressed. */
    int altgr_modifier;

    /* The most recently pressed deadkey.  When receiving an invalid
       composition message, both the deadkey and the next key are sent
       to Emacs.  If this member is zero, no deadkey has been pressed
       recently. */
    int deadkey;

    /* Disable PM-defined shortcuts (accelerators) such as F1, F10,
       Alt+F4, for which the associated bit in this member is zero.  */
    unsigned shortcuts;

    /* This array maps OS/2 mouse button numbers to Emacs mouse button
       numbers.  The values are either 0 (if the button is to be
       ignored) or are in 1 through 3. */
    char buttons[3];

    /* This array contains non-zero values for all logical fonts defined
       for this frame. */
    char font_defined[255];

    /* List of character increments, based on the default font.  This is
       used to override non-integer character increments when using an
       outline font and to override wrong character increments when
       using a font of a size differing from the size of the default
       font. */
    LONG increments[512];

    /* Accept variable-width fonts if `var_width_fonts' is non-zero. */
    char var_width_fonts;

#ifdef USE_PALETTE_MANAGER
    /* The palette. */
    HPAL hpal;
#endif

    /* Current state of various PM attributes.  This is used to speed up
       text output by avoiding GPI calls. */
    COLOR cur_color, cur_backcolor;
    LONG cur_charset, cur_backmix;
    FIXED cur_cbox_size;
} frame_data;


typedef struct
{
  int x, y, button, align_top;
  pm_menu *data;
  char *str;
} menu_data;

typedef struct
{
  pm_menu *data;
  char *str;
  int el_count;
  int str_bytes;
} menubar_data;

struct drop
{
  unsigned long cookie;
  unsigned len;
  char str[CCHMAXPATH];
};

struct scrollbar_entry
{
  HWND hwnd;
  slider async;                 /* Used while dragging */
  short range;
};

/* The original window procedure of frame windows.  Frame windows are
   subclassed by FrameWndProc, which calls this window procedure. */
static PFNWP old_frame_proc = NULL;

/* The anchor-block handle.  It is shared by all threads.  This should
   be fixed, each thread that has a message queue should have its own
   anchor-block handle.  Currently, OS/2 ignores the anchor-block
   handle. */
static HAB hab;

/* The message queue handle of the PM thread. */
static HMQ hmq;

/* The size of the screen. */
static SWP swpScreen;

/* Requests from Emacs arrive in this pipe. */
static int inbound_pipe;

/* Events are sent through this pipe to Emacs. */
static int outbound_pipe;

/* The handle of the object window of the PM thread.  See
   ObjectWndProc() for details. */
static HWND hwndObject;

/* This hash table is used for finding the frame_data structure for a
   frame ID. */
static frame_data *hash_table[HASH_SIZE];

/* Pointer to frame_data structure used while creating a new frame.
   This is dirty. */
static frame_data *new_frame;

/* Window class names. */
static const char szFrameClass[] = "pmemacs.frame";
static const char szClientClass[] = "pmemacs.client";
static const char szObjectClass[] = "pmemacs.object";

/* This flag is non-zero if a WM_MENUSELECT message has been
   received. */
static int menu_selected = FALSE;

/* This variable is used for setting the IDs of submenus.  */
static int submenu_id;

/* Map scrollbar IDs to window handles.  Unused entries contain
   NULLHANDLE in the `hwnd' member.  This table is shared by all
   frames.  Per-scrollbar data is also stored in this table. */

static struct scrollbar_entry scrollbar_table[MAX_SCROLLBARS];

/* While dragging the slider of a scrollbar, we should not set the
   position or size of the slider (that would confuse the PM).  As the
   scrollbar code captures the mouse, only one scrollbar can be
   dragged at any time, therefore a global variable is sufficient for
   recording whether the slider of the current scrollbar is being
   dragged or not.  As that scrollbar also has the keyboard focus,
   keyboard commands cannot be used to cause changes to the
   scrollbars.  For now, we ignore the problem of Lisp code causing
   changes to the scrollbar during dragging. */
static int dragging_scrollbar_slider = FALSE;

/* For positioning popup menus so that the first item is at the
   position of the mouse, we need to remember the first ID.  */
static int first_id;

/* This is a presentation space for the screen.  It is used by
   dialog_string_width() which assumes that the default font is
   selected. */
static HPS hpsScreen;

/* This is the width of an icon, in pixels. */
static LONG cxIcon;

/* This is the maximum width and height, respectively, of the system
   font.  These values are used for converting to and from dialog box
   units. */
static LONG default_font_width;
static LONG default_font_height;

/* Post this event semaphore to beep. */
static HEV hevBeep;

/* Post this event after processing a message sent by pipe_thread(). */
static HEV hevDone;

/* The process ID of Emacs proper.  This is taken from the command
   line.  We cannot use getppid(), as PMSHELL is the parent process of
   all PM sessions. */
static int emacs_pid;

/* The quit character.  If this character is typed, send SIGINT to
   Emacs instead of a keyboard event.  The default is C-G. */
static int quit_char = 0x07;

/* This variable is TRUE after receiving a PMR_CLOSE request.  This is
   for avoiding errors due to broken pipes etc. */
static int terminating = FALSE;

/* The table of faces.  Element 0 of face_vector is not used. */
static face_data *face_vector = NULL;
static int nfaces = 0;
static int nfaces_allocated = 0;

/* The table of fonts.  Local font IDs are shared by all frames to
   simplify things.  The Presentation Manager supports local font IDs
   1 through 254 and 0 (the default font). */
static font_data font_vector[255];
static int nfonts = 0;

/* If this variable is true, send PME_MOUSEMOVE events for
   WM_MOUSEMOVE messages.  As Emacs 19.23 uses mouse movement events
   for highlighting mouse-sensitive areas, track_mouse is now always
   true.  Perhaps later versions of Emacs want to turn off mouse
   movement events, therefore we keep this variable and the
   PMR_TRACKMOUSE request. */

static int track_mouse = TRUE;

/* The glyph coordinates of the previous PME_MOUSEMOVE event. */
static int track_x = -1;
static int track_y = -1;

/* This flag indicates whether the mouse is captured or not. */
static int capture_flag = FALSE;

#ifdef USE_PALETTE_MANAGER

/* This variable is TRUE if the palette manager is available. */
static int has_palette_manager = FALSE;

/* The color table for the palette manager */
static ULONG color_table[1024];
static int colors = 0;

#endif

#ifdef USE_PALETTE_MANAGER
#define GET_COLOR(hps,x) (has_palette_manager ? \
                          GpiQueryColorIndex (hps, 0, (x)) : (x))
#else
#define GET_COLOR(hps,x) (x)
#endif

/* This vector tells DlgProc which buttons are to be disabled. */

static char disabled_buttons[80];

/* The serial number of the last PMR_POPUPMENU or PMR_MENU request. */

static int menu_serial;

/* The serial number of the last PMR_DIALOG request. */

static int dialog_serial;

/* Circular buffer of objects recently dropped.  */

#define DROP_MAX 8
static int drop_in = 0;
static int drop_out = 0;
static int drop_count = 0;
static HMTX drop_mutex;
static struct drop *drop_list;

/* Each drop action is assigned a number, for relating drop events
   with PMR_DROP requests. */

static int drop_cookie = 0;

/* Current code page. */

static int code_page = 850;


/* This table maps keypad keys. */

static const struct
{
  unsigned ch;                  /* Character code (input) */
  unsigned sc;                  /* Scan code (input) */
  unsigned vk;                  /* Virtual key (output) */
  unsigned clr;                 /* Flags to clear (output) */
} keypad[] =
  {
    {'*',  0x37, VK_KP_MULTIPLY,  0},
    {'7',  0x47, VK_KP_7,         KC_SHIFT},
    {'8',  0x48, VK_KP_8,         KC_SHIFT},
    {'9',  0x49, VK_KP_9,         KC_SHIFT},
    {'-',  0x4a, VK_KP_SUBTRACT,  0},
    {'4',  0x4b, VK_KP_4,         KC_SHIFT},
    {'5',  0x4c, VK_KP_5,         KC_SHIFT},
    {'6',  0x4d, VK_KP_6,         KC_SHIFT},
    {'+',  0x4e, VK_KP_ADD,       0},
    {'1',  0x4f, VK_KP_1,         KC_SHIFT},
    {'2',  0x50, VK_KP_2,         KC_SHIFT},
    {'3',  0x51, VK_KP_3,         KC_SHIFT},
    {'0',  0x52, VK_KP_0,         KC_SHIFT},
    {'.',  0x53, VK_KP_DECIMAL,   KC_SHIFT},
    {',',  0x53, VK_KP_SEPARATOR, KC_SHIFT},
    {'/',  0x5c, VK_KP_DIVIDE,    0},
  };


/* Prototypes for functions which are used before defined. */

static void terminate_process (void);


/* Convert between dialog box units and pixels. */

#define DLGUNIT_TO_PIX_X(x) (((x) * default_font_width) / 4)
#define DLGUNIT_TO_PIX_Y(y) (((y) * default_font_height) / 8)
#define PIX_TO_DLGUNIT_X(x) (((x) * 4 + default_font_width - 1) \
                             / default_font_width)
#define PIX_TO_DLGUNIT_Y(y) (((y) * 8 + default_font_height - 1) \
                             / default_font_height)

/* Convert character coordinates to pixel coordinates (reference
   point!). */

#define make_x(f,x) ((x) * (f)->cxChar)
#define make_y(f,y) ((f)->cyClient + (f)->cyDesc - ((y) + 1) * (f)->cyChar)

/* Convert pixel coordinates to character coordinates (character
   box!). */

#define charbox_x(f,x) ((x) / (f)->cxChar)
#define charbox_y(f,y) (((f)->cyClient - 1 - (y)) / (f)->cyChar)



#ifdef TRACE

#define TRACE_FNAME             "pmemacs.log"
#define TRACE_OFF               ((HFILE)-1)

static HFILE trace_handle = TRACE_OFF;

static void trace_str (const char *s, size_t len)
{
  ULONG n;

  if (trace_handle != TRACE_OFF &&
      DosWrite (trace_handle, s, len, &n) != 0)
    trace_handle = TRACE_OFF;
}

#define TRACE_STR(S,LEN) trace_str ((S), (LEN))
#define TRACE_PSZ(S)     trace_str ((S), strlen (S))

#else

#define TRACE_STR(S,LEN) ((void)0)
#define TRACE_PSZ(S)     ((void)0)

#endif


/* An error occured.  Display MSG in a message box, then quit. */

static void pm_error (const char *msg)
{
  WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, msg, "PM Emacs error", 0,
                 MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
  /*...*/
  DosExit (EXIT_PROCESS, 1);
}


/* Display a message.  This is used for debugging. */

static void pm_message (HWND hwndOwner, const char *fmt, ...)
{
  va_list arg_ptr;
  char tmp[200];

  va_start (arg_ptr, fmt);
  vsprintf (tmp, fmt, arg_ptr);
  WinMessageBox (HWND_DESKTOP, hwndOwner, tmp, "PM Emacs", 0,
                 MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
}

/* Allocate memory.  Note that DosAllocMem fails if SIZE is 0. */

static void *allocate (ULONG size)
{
  ULONG rc;
  void *p;

  if (size == 0)
    return (NULL);
  rc = DosAllocMem ((PVOID)&p, size, PAG_COMMIT | PAG_READ | PAG_WRITE);
  if (rc != 0)
    pm_error ("Out of memory.");
  return (p);
}


/* Deallocate memory allocated by allocate(). */

static void deallocate (void *p)
{
  if (p != NULL)
    DosFreeMem (p);
}


/* Send data to Emacs. */

static void send (int fd, const void *src, size_t size)
{
  const char *s;
  ULONG rc, n;

  if (terminating)
    return;
  s = src;
  while (size != 0)
    {
      rc = DosWrite (fd, s, size, &n);
      if (rc != 0 || n == 0)
        pm_message (HWND_DESKTOP, "Cannot write to pipe, rc=%lu, n=%lu",
                    rc, n);
      size -= n;
      s += n;
    }
}


/* Send an event to Emacs. */

static void send_event (const pm_event *src)
{
  send (outbound_pipe, src, sizeof (pm_event));
}

/* Send answer to Emacs.  SERIAL is the serial number, taken from the
   request.  SRC points to SIZE bytes of data.  If SRC is NULL, the
   word ONE_WORD is sent instead. */

static void send_answer (int serial, const void *src, unsigned size,
                         int one_word)
{
  pm_event pme;

  pme.answer.header.type = PME_ANSWER;
  pme.answer.header.frame = 0;
  pme.answer.serial = serial;
  pme.answer.one_word = one_word;
  pme.answer.size = size;
  if (src == NULL)
    {
      pme.answer.size = -1;
      send_event (&pme);
    }
  else if (size == 0)
    send_event (&pme);
  else
    {
      char *tem, *alloc;
      unsigned total;

      /* Send the answer atomically to avoid interference with events
         sent by other threads. */

      alloc = NULL;
      total = size + sizeof (pme);
      if (total <= 0x2000)
        tem = alloca (total);
      else
        tem = alloc = allocate (total);
      memcpy (tem, &pme, sizeof (pme));
      memcpy (tem + sizeof (pme), src, size);
      send (outbound_pipe, tem, total);
      if (alloc != NULL) deallocate (alloc);
    }
}


/* Clear all fonts of all the faces and reset the font IDs in the face
   table. */

static void clear_fonts (void)
{
  frame_data *f;
  int i;

  for (i = 1; i <= nfaces; ++i)
    face_vector[i].font_id = 0;
  for (i = 0; i < HASH_SIZE; ++i)
    for (f = hash_table[i]; f != NULL; f = f->next)
      memset (f->font_defined, 0, sizeof (f->font_defined));
  nfonts = 0;
}


/* Compare two font specifications. */

static int equal_font_spec (const font_spec *f1, const font_spec *f2)
{
  return (f1->size == f2->size
          && f1->sel == f2->sel
          && strcmp (f1->name, f2->name) == 0);
}


/* Parse a font string into a font specification. */

#define SKIP_FIELD while (*str != 0 && *str != '-') ++str; \
		   if (*str == '-') ++str; else return (FALSE)

static int parse_font_spec (font_spec *dst, const unsigned char *str)
{
  int i;

  dst->size = 0;
  dst->sel = 0;
  dst->name[0] = 0;
  if (str[0] == '-')
    {
      ++str;
      SKIP_FIELD;               /* Foundry */
      i = 0;
      while (*str != 0 && *str != '-')
        {
          if (i < sizeof (dst->name) - 1)
            dst->name[i++] = *str;
          ++str;
        }
      dst->name[i] = 0;
      SKIP_FIELD;               /* Family */
      if (*str == 'b')
        dst->sel |= FATTR_SEL_BOLD;
      SKIP_FIELD;               /* Weight */
      if (*str == 'i')
        dst->sel |= FATTR_SEL_ITALIC;
      SKIP_FIELD;               /* Slant */
      SKIP_FIELD;               /* Width */
      SKIP_FIELD;               /* Additional style */
      SKIP_FIELD;               /* Pixel size */
      i = 0;
      while (*str >= '0' && *str <= '9')
        {
          i = i * 10 + (*str - '0');
          ++str;
        }
      if (i == 0)
        return (FALSE);
      dst->size = i;
      SKIP_FIELD;               /* Point size */
      SKIP_FIELD;               /* X resolution */
      SKIP_FIELD;               /* Y resolution */
      SKIP_FIELD;               /* Spacing */
      SKIP_FIELD;               /* Registry */
      return (TRUE);
    }
  else
    {
      while (*str >= '0' && *str <= '9')
        {
          dst->size = dst->size * 10 + (*str - '0');
          ++str;
        }
      dst->size *= 10;
      if (dst->size == 0)
        return (FALSE);
      for (;;)
        {
          if (strncmp (str, ".bold", 5) == 0)
            {
              dst->sel |= FATTR_SEL_BOLD;
              str += 5;
            }
          else if (strncmp (str, ".italic", 7) == 0)
            {
              dst->sel |= FATTR_SEL_ITALIC;
              str += 7;
            }
          else if (*str == '.')
            {
              ++str;
              break;
            }
          else
            return (FALSE);
        }
      if (*str == 0)
        return (FALSE);
      _strncpy (dst->name, str, sizeof (dst->name));
    }
  return (TRUE);
}


/* Create a logical font (using ID) matching PFM.  SELECTION contains
   font selection bits such as FATTR_SEL_UNDERSCORE. */

static int use_font (frame_data *f, int id, PFONTMETRICS pfm, int selection)
{
  FATTRS font_attrs;

  font_attrs.usRecordLength = sizeof (font_attrs);
  font_attrs.fsSelection = selection;
  font_attrs.lMatch = pfm->lMatch;
  strcpy (font_attrs.szFacename, pfm->szFacename);
  font_attrs.idRegistry = 0;
  font_attrs.usCodePage = code_page;
  font_attrs.lMaxBaselineExt = 0;
  font_attrs.lAveCharWidth = 0;
  font_attrs.fsType = 0;
  font_attrs.fsFontUse = 0;
  if (GpiCreateLogFont (f->hpsClient, NULL, id, &font_attrs) == GPI_ERROR)
    return (FALSE);
  GpiSetCharSet (f->hpsClient, id);
  f->cur_charset = id;
  return (TRUE);
}


/* Select a font matching FONT_NAME and FONT_SIZE (10 * pt).  If there
   is such a font, return TRUE.  Otherwise, no font is selected and
   FALSE is returned. */

static int select_font_internal (frame_data *f, int id, char *font_name,
                                 int font_size, int selection,
                                 font_data *font)
{
  ULONG fonts, temp, i;
  LONG xRes;
  USHORT size;
  PFONTMETRICS pfm;
  HDC hdc;

  temp = 0;
  fonts = GpiQueryFonts (f->hpsClient, QF_PUBLIC | QF_PRIVATE, font_name,
                         &temp, sizeof (FONTMETRICS), NULL);
  pfm = allocate (fonts * sizeof (FONTMETRICS));
  GpiQueryFonts (f->hpsClient, QF_PUBLIC | QF_PRIVATE, font_name,
                 &fonts, sizeof (FONTMETRICS), pfm);
  for (i = 0; i < fonts; ++i)
    if ((f->var_width_fonts || (pfm[i].fsType & FM_TYPE_FIXED))
        && !(pfm[i].fsDefn & FM_DEFN_OUTLINE)
        && pfm[i].sNominalPointSize == font_size
        && use_font (f, id, pfm+i, selection))
      {
        font->outline = FALSE;
        font->use_incr = (selection != f->font.sel
                          || !(pfm[i].fsType & FM_TYPE_FIXED)
                          || pfm[i].lMaxCharInc != f->cxChar
                          || pfm[i].lMaxBaselineExt != f->cyChar
                          || pfm[i].lMaxDescender != f->cyDesc);
        deallocate (pfm);
        return (TRUE);
      }
  for (i = 0; i < fonts; ++i)
    if ((f->var_width_fonts || (pfm[i].fsType & FM_TYPE_FIXED))
        && (pfm[i].fsDefn & FM_DEFN_OUTLINE)
        && use_font (f, id, pfm+i, selection))
      {
        font->outline = TRUE;
        font->use_incr = TRUE;
        hdc = GpiQueryDevice (f->hpsClient);
        DevQueryCaps (hdc, CAPS_HORIZONTAL_FONT_RES, 1, &xRes);
        size = (USHORT)((double)font_size * (double)xRes / 720.0 + 0.5);
        font->cbox_size = MAKEFIXED (size, 0);
        deallocate (pfm);
        return (TRUE);
      }
  deallocate (pfm);
  return (FALSE);
}


/* Select a font matching FONT_NAME and FONT_SIZE (10 * pt).  If this
   fails, use the default font of the frame.  If there is such a font,
   return TRUE.  Otherwise, no font is selected and FALSE is
   returned. */

static int select_font (frame_data *f, int id, char *font_name,
                        int font_size, int selection, font_data *font)
{
  int ok;

  ok = select_font_internal (f, id, font_name, font_size, selection, font);
  if (!ok)
    ok = select_font_internal (f, id, f->font.name, f->font.size, selection,
                               font);
  return (ok);
}


/* Create a font for a face. */

static int make_font (frame_data *f, face_data *face)
{
  int i;
  font_data *font;
  font_spec spec;

  spec = face->spec;
  if (spec.size == 0)
    spec.size = f->font.size;
  if (spec.name[0] == 0)
    strcpy (spec.name, f->font.name);
  if (face->font_id == 0)
    for (i = 1; i <= nfonts; ++i)
      if (equal_font_spec (&font_vector[i].spec, &spec))
        {
          face->font_id = i;
          break;
        }
  if (face->font_id == 0)
    {
      if (nfonts >= 254)
        face->font_id = 1;
      else
        {
          /* Note: Font ID 0 is not used, it means no font ID */
          face->font_id = ++nfonts;
          font_vector[face->font_id].spec = spec;
        }
    }
  font = &font_vector[face->font_id];
  select_font (f, face->font_id, font->spec.name, font->spec.size,
               spec.sel, font);
  f->font_defined[face->font_id] = 1;
  return (face->font_id);
}


/* Set the default font of the frame F, according to the font_name and
   font_size fields of *F. */

static void set_default_font (frame_data *f)
{
  FONTMETRICS fm;
  SIZEF cbox;
  int i;
  font_data font;

  clear_fonts ();
  if (!select_font_internal (f, 1, f->font.name, f->font.size, 0, &font))
    {
      parse_font_spec (&f->font, DEFAULT_FONT);
      select_font_internal (f, 1, f->font.name, f->font.size, 0, &font);
    }
  if (font.outline)
    {
      cbox.cx = cbox.cy = font.cbox_size;
      GpiSetCharBox (f->hpsClient, &cbox);
      f->cur_cbox_size = font.cbox_size;
    }
  GpiQueryFontMetrics (f->hpsClient, (LONG)sizeof (fm), &fm);
  f->cxChar = fm.lMaxCharInc;
  f->cyChar = fm.lMaxBaselineExt;
  f->cyDesc = fm.lMaxDescender;
  for (i = 0; i < 512; ++i)
    f->increments[i] = fm.lMaxCharInc;
}


/* Add a color to the color table. */

#ifdef USE_PALETTE_MANAGER
static void add_color (COLOR color)
{
  int i;

  for (i = 0; i < colors; ++i)
    if (color_table[i] == color)
      return;
  color_table[colors++] = color;
}
#endif


/* Create palette.  Call this after adding a color. */

#ifdef USE_PALETTE_MANAGER
static void make_palette (frame_data *f)
{
  LONG size;

  if (has_palette_manager)
    {
      if (f->hpal != 0)
        GpiDeletePalette (f->hpal);
      f->hpal = GpiCreatePalette (hab, LCOL_PURECOLOR, LCOLF_CONSECRGB,
                                  colors, color_table);
      if (f->hpal == 0 || f->hpal == GPI_ERROR)
        pm_message (HWND_DESKTOP, "GpiCreatePalette failed");
      if (GpiSelectPalette (f->hpsClient, f->hpal) == PAL_ERROR)
        pm_message (HWND_DESKTOP, "GpiSelectPalette failed");
      if (WinRealizePalette (f->hwndClient, f->hpsClient, &size) == PAL_ERROR)
        pm_message (HWND_DESKTOP, "WinRealizePalette failed");
    }
}
#endif


/* Change the size of a frame.  The new size is taken from the WIDTH
   and HEIGHT fields of the frame data structure.  This function is
   also called to resize the PM window if the font has changed. */

static void set_size (frame_data *f)
{
  RECTL rcl;
  SWP swp;
  POINTL ptl;

  /* Don't call WinSetWindowPos if the frame is minimized or not yet
     completely created. */

  if (f->minimized || f->stage < STAGE_READY)
    return;

  /* Compute the rectangle of the frame window from the current
     position and the new size of the client window.  The upper left
     corner of the window is kept in place. */

  WinQueryWindowPos (f->hwndClient, &swp);
  ptl.x = swp.x;                /* Upper left-hand corner */
  ptl.y = swp.y + swp.cy;
  WinMapWindowPoints (f->hwndFrame, HWND_DESKTOP, &ptl, 1);
  rcl.xLeft = ptl.x;
  rcl.xRight = rcl.xLeft + (f->width + f->sb_width) * f->cxChar;
  rcl.yTop = ptl.y;
  rcl.yBottom = rcl.yTop - f->height * f->cyChar;
  WinCalcFrameRect (f->hwndFrame, &rcl, FALSE);

  /* When keeping the lower left-hand corner in place it might happen
     that the titlebar is moved out of the screen.  If this happens,
     move the window to keep the titlebar visible. */

  if (rcl.yTop > swpScreen.cy)
    {
      LONG offset = rcl.yTop - swpScreen.cy;
      rcl.yBottom -= offset;
      rcl.yTop -= offset;
    }

  /* Now resize and move the window. */

  WinSetWindowPos (f->hwndFrame, HWND_TOP, rcl.xLeft, rcl.yBottom,
                   rcl.xRight - rcl.xLeft, rcl.yTop - rcl.yBottom,
                   SWP_SIZE | SWP_MOVE);
}


/* Report the size of frame F to Emacs using the cxClient and cyClient
   fields. */

static void report_size (frame_data *f)
{
  pm_event pme;

  pme.size.header.type = PME_SIZE;
  pme.size.header.frame = f->id;
  pme.size.width = f->cxClient / f->cxChar - f->sb_width;
  pme.size.height = f->cyClient / f->cyChar;
  if (pme.size.width < 1) pme.size.width = 1;
  if (pme.size.height < 1) pme.size.height = 1;
  pme.size.pix_width = pme.size.width; /* TODO */
  pme.size.pix_height = pme.size.height; /* TODO */
  f->width = pme.size.width;
  f->height = pme.size.height;
  send_event (&pme);
}


/* Show the frame F for the first time. */

static void initial_show_frame (frame_data *f)
{
  LONG cx, cy;
  RECTL rcl;
  POINTL ptl;
  SWP swp;
  menubar_data mbd;

  /* Avoid recursion. */

  if (f->stage >= STAGE_GETTING_READY)
    return;
  f->stage = STAGE_GETTING_READY;

  /* First compute the size of the frame window. */

  cx = (f->width + f->sb_width) * f->cxChar;
  cy = f->height * f->cyChar;
  rcl.xLeft = 0; rcl.yBottom = 0;
  rcl.xRight = rcl.xLeft + cx;
  rcl.yTop = rcl.yBottom + cy;
  WinCalcFrameRect (f->hwndFrame, &rcl, FALSE);

  WinSetWindowPos (f->hwndFrame, NULLHANDLE, 0, 0,
                   rcl.xRight - rcl.xLeft, rcl.yTop - rcl.yBottom,
                   SWP_SIZE);

  /* Next, set the position of the frame window by computing the
     position of the client window in rcl.xLeft and rcl.yBottom,
     computing the frame window position from the client window
     position, and moving the frame window. */

  if (f->initial_left_base == 0 || f->initial_top_base == 0)
    {
      /* Unfortunately, FCF_SHELLPOSITION does not take effect until
         the window is made visible.  That is, we have to show the
         window to be able to obtain the position. */

      WinSetWindowPos (f->hwndFrame, HWND_TOP, 0, 0, 0, 0,
                       SWP_SHOW | SWP_ZORDER);
      WinQueryWindowPos (f->hwndClient, &swp);
      ptl.x = swp.x; /* Lower left-hand corner */
      ptl.y = swp.y;
      WinMapWindowPoints (f->hwndFrame, HWND_DESKTOP, &ptl, 1);
      rcl.xLeft = ptl.x;
      rcl.yBottom = ptl.y;
    }

  if (f->initial_left_base > 0)
    rcl.xLeft = f->initial_left;
  else if (f->initial_left_base < 0)
    rcl.xLeft = swpScreen.cx - cx + f->initial_left;

  if (f->initial_top_base > 0)
    rcl.yBottom = swpScreen.cy - 1 - (f->initial_top + cy);
  else if (f->initial_top_base < 0)
    rcl.yBottom = -f->initial_top;

  rcl.xRight = rcl.xLeft + cx;
  rcl.yTop = rcl.yBottom + cy;
  WinCalcFrameRect (f->hwndFrame, &rcl, FALSE);
  WinSetWindowPos (f->hwndFrame, HWND_TOP,
                   rcl.xLeft, rcl.yBottom,
                   rcl.xRight - rcl.xLeft, rcl.yTop - rcl.yBottom,
                   SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER);
  WinSetActiveWindow (HWND_DESKTOP, f->hwndFrame);
  f->cxClient = cx;
  f->cyClient = cy;
  f->stage = STAGE_READY;
  report_size (f);

  mbd.data = NULL; mbd.str = NULL; mbd.el_count = 0; mbd.str_bytes = 0;
  WinSendMsg (f->hwndClient, UWM_MENUBAR, MPFROMP (f), MPFROMP (&mbd));

  WinInvalidateRect (f->hwndClient, NULL, FALSE);
}


/* Send a list of supported code pages to Emacs. */

static void cplist (int serial)
{
  ULONG acp[32];
  ULONG n;

  n = WinQueryCpList (hab, sizeof (acp) / sizeof (acp[0]), acp);
  send_answer (serial, acp, n * sizeof (acp[0]), 0);
}


/* Set the code page of the message queue and store it into
   `code_page', for font creation.  Note that all fonts will be
   recreated as all frames will be redrawn.  If answer is true, send
   an answer of 0 (failure) or 1 (success) to Emacs. */

static void set_code_page (int cp, int serial, int answer)
{
  int ok;

  ok = WinSetCp (hmq, cp);
  if (ok)
    {
      code_page = cp;
      clear_fonts ();
    }
  if (answer)
    send_answer (serial, NULL, 0, ok);
}


/* Translate PM shift key bits to Emacs modifier bits. */

static int convert_modifiers (frame_data *f, int shift)
{
  int result;

  result = 0;
  if (shift & KC_ALT)
    result |= f->alt_modifier;
  if (shift & KC_CTRL)
    result |= ctrl_modifier;
  if (shift & KC_SHIFT)
    result |= shift_modifier;
  if (shift & KC_ALTGR)         /* KC_ALTGR is a mock modifier */
    result |= f->altgr_modifier;
  return (result);
}


/* Send a keyboard event to Emacs.  F is the frame to which the event
   is sent, TYPE is the event subtype (PMK_ASCII or PMK_VIRTUAL).
   CODE is the character code, virtual key code or the symbol.
   MODIFIERS contains PM keyboard status bits for the shift keys, REP
   is the repeat count.  If the key matches quit_char, a SIGINT signal
   is sent instead of a keyboard event. */

static void send_key (frame_data *f, pm_key_type type, int code,
                      int modifiers, int rep)
{
  pm_event pme;
  int i;

  if (type == PMK_ASCII && code == quit_char && modifiers == 0)
    kill (emacs_pid, SIGINT);
  else
    {
      pme.key.header.type = PME_KEY;
      pme.key.header.frame = f->id;
      pme.key.type = type;
      pme.key.code = code;
      pme.key.modifiers = convert_modifiers (f, modifiers);
      for (i = 0; i < rep; ++i)
        send_event (&pme);
    }
}


/* Send a mouse click event to Emacs. */

static void send_button (frame_data *f, int x, int y, int button,
                         int modifiers, unsigned long timestamp)
{
  pm_event pme;

  pme.button.header.type = PME_BUTTON;
  pme.button.header.frame = f->id;
  pme.button.button = button;
  pme.button.modifiers = modifiers;
  pme.button.x = charbox_x (f, x);
  pme.button.y = charbox_y (f, y);
  if (pme.button.x < 0) pme.button.x = 0;
  if (pme.button.y < 0) pme.button.y = 0;
  pme.button.timestamp = timestamp;
  send_event (&pme);
}


/* Create a cursor for frame F, according to the cursor_type,
   cursor_width, and cursor_blink fields of *F. */

static void create_cursor (frame_data *f)
{
  LONG cx, cy;
  ULONG flags;

  if (f->cxChar != 0 && f->cyChar != 0)
    {
      cx = f->cxChar;
      cy = f->cyChar;
      flags = CURSOR_SOLID;
      switch (f->cursor_type)
        {
        case CURSORTYPE_BAR:
          cx = f->cursor_width;
          break;
        case CURSORTYPE_FRAME:
          flags = CURSOR_FRAME;
          break;
        case CURSORTYPE_UNDERLINE:
          cy = 0;
          break;
        case CURSORTYPE_HALFTONE:
          flags = CURSOR_HALFTONE;
          break;
        case CURSORTYPE_BOX:
          break;
        }
      if (f->cursor_blink)
        flags |= CURSOR_FLASH;
      WinCreateCursor (f->hwndClient, f->cursor_x, f->cursor_y, cx, cy,
                       flags, NULL);
      if (f->cursor_on)
        WinShowCursor (f->hwndClient, TRUE);
    }
}


/* Return true iff F's client window (or one if its children such as a
   scrollbar) has the focus. */

static int has_focus (frame_data *f)
{
  HWND hwndFocus;

  hwndFocus = WinQueryFocus (HWND_DESKTOP);
  return (hwndFocus == f->hwndClient
          || WinQueryWindow (hwndFocus, QW_PARENT) == f->hwndClient);
}


/* Create a new cursor for the frame F if F has the input focus. */

static void set_cursor (frame_data *f)
{
  if (has_focus (f))
    {
      WinDestroyCursor (f->hwndClient);
      create_cursor (f);
    }
}


/* Set the font of menu HWNDMENU to NAME.  Trying to use a
   non-existing font selects the default font.  Therefore, we can use
   the empty string to switch to the default font.  This function
   should be called after adding all the menu items; otherwise the
   height of the menu items will be wrong.  For the same reason,
   setting the presentation parameter with WinCreateWindow does not
   work. */

static void set_menu_font (HWND hwndMenu, char *name)
{
  WinSetPresParam (hwndMenu, PP_FONTNAMESIZE, strlen (name) + 1, name);
}


/* Update the font for all menus of frame F. */

static void update_menu_font (frame_data *f)
{
  HWND hwnd;

  hwnd = WinWindowFromID (f->hwndFrame, FID_SYSMENU);
  if (hwnd != NULLHANDLE)
    set_menu_font (hwnd, f->menu_font);
  hwnd = WinWindowFromID (f->hwndFrame, FID_MENU);
  if (hwnd != NULLHANDLE)
    set_menu_font (hwnd, f->menu_font);
}


/* Create a menu and return the window handle.  HWNDOWNER will be the
   owner of the menu.  ID is the window ID. */

static HWND create_menu_handle (HWND hwndOwner, ULONG id)
{
  return (WinCreateWindow (hwndOwner, WC_MENU, "", 0, 0, 0, 0, 0,
                           hwndOwner, HWND_TOP, id, NULL, NULL));
}


/* Add the value SEL to `menubar_id_map' of frame F and return the
   index of the new entry. */

static int add_menubar_id_map (frame_data *f, int sel)
{
  int id;

  if (f->menubar_id_used >= f->menubar_id_allocated)
    {
      int *new_map, new_alloc;

      new_alloc = f->menubar_id_allocated + 1024; /* One page */
      new_map = allocate (new_alloc * sizeof (*new_map));
      if (f->menubar_id_map != NULL)
        {
          memcpy (new_map, f->menubar_id_map,
                  f->menubar_id_used * sizeof (*new_map));
          deallocate (f->menubar_id_map);
        }
      f->menubar_id_map = new_map;
      f->menubar_id_allocated = new_alloc;
    }
  id = f->menubar_id_used++;
  f->menubar_id_map[id] = sel;
  return (id);
}


static void create_submenu (frame_data *f, HWND hwndMenu, HWND hwndOwner,
                            pm_menu **p, char *str, int sub, int menubarp)
{
  MENUITEM mi;

  for (;;)
    switch ((*p)->type)
      {
      case PMMENU_END:
        return;

      case PMMENU_POP:
        ++(*p);
        return;

      case PMMENU_ITEM:
        if ((*p)[1].type == PMMENU_PUSH)
          {
            mi.hwndSubMenu = create_menu_handle (hwndOwner, 0);
            mi.afStyle = MIS_SUBMENU | MIS_TEXT;
            mi.iPosition = MIT_END;
            mi.afAttribute = 0;
            mi.hItem = 0;
            mi.id = submenu_id++;
            if (first_id == 0) first_id = mi.id;
            WinSendMsg (hwndMenu, MM_INSERTITEM, MPFROMP (&mi),
                        MPFROMP (str + (*p)->str_offset));
            (*p) += 2;
            create_submenu (f, mi.hwndSubMenu, hwndOwner, p, str, sub,
                            menubarp);
          }
        else
          {
            mi.iPosition = MIT_END;
            mi.afStyle = MIS_TEXT;
            mi.afAttribute = ((*p)->enable ? 0 : MIA_DISABLED);
            mi.hwndSubMenu = NULLHANDLE;
            mi.hItem = 0;
            if (menubarp)
              mi.id = IDM_MENUBAR + add_menubar_id_map (f, (*p)->item);
            else
              mi.id = IDM_MENU + (*p)->item;
            if (first_id == 0) first_id = mi.id;
            WinSendMsg (hwndMenu, MM_INSERTITEM, MPFROMP (&mi),
                        MPFROMP (str + (*p)->str_offset));
            ++(*p);
          }
        break;

      case PMMENU_SEP:
        mi.iPosition = MIT_END;
        mi.afStyle = MIS_SEPARATOR;
        mi.afAttribute = 0;
        mi.hwndSubMenu = NULLHANDLE;
        mi.hItem = 0;
        mi.id = 0;
        WinSendMsg (hwndMenu, MM_INSERTITEM, MPFROMP (&mi), NULL);
        ++(*p);
        break;

      case PMMENU_SUB:
        if (sub)
          return;
        mi.hwndSubMenu = create_menu_handle (hwndOwner, 0);
        mi.afStyle = MIS_SUBMENU | MIS_TEXT;
        mi.iPosition = MIT_END;
        mi.afAttribute = 0;
        mi.hItem = 0;
        mi.id = submenu_id++;
        if (first_id == 0) first_id = mi.id;
        WinSendMsg (hwndMenu, MM_INSERTITEM, MPFROMP (&mi),
                    MPFROMP (str + (*p)->str_offset));
        ++(*p);
        create_submenu (f, mi.hwndSubMenu, hwndOwner, p, str, TRUE, menubarp);
        break;

      default:
        abort ();
      }
}


/* Create a menu, adding submenus to HWNDMENU.  The owner of
   submenus will be HWNDOWNER. */

static void create_menu (frame_data *f, HWND hwndMenu, HWND hwndOwner,
                         pm_menu **p, char *str, int menubarp)
{
  f->popup_active = FALSE;
  first_id = 0;
  create_submenu (f, hwndMenu, hwndOwner, p, str, FALSE, menubarp);
  if (f->menu_font[0] != 0)
    set_menu_font (hwndMenu, f->menu_font);
}


/* Create a popup menu and return its window handle. */

static HWND create_popup_menu (frame_data *f, pm_menu *p, char *str)
{
  HWND hwnd;

  hwnd = create_menu_handle (f->hwndClient, 0);
  submenu_id = IDM_SUBMENU;
  create_menu (f, hwnd, f->hwndClient, &p, str, FALSE);
  f->popup_active = TRUE;
  return (hwnd);
}


/* Update a menu of the menubar. */

static int update_menu (frame_data *f, HWND hwnd, pm_menu **p, char *str,
                        pm_menu **p0, char *str0, int sub)
{
  MENUITEM mi;
  MRESULT mr;
  int id;

  for (;;)
    switch ((*p)->type)
      {
      case PMMENU_END:
        return (TRUE);

      case PMMENU_POP:
        ++(*p); ++(*p0);
        return (TRUE);

      case PMMENU_ITEM:
        if ((*p)[1].type == PMMENU_PUSH)
          {
            id = submenu_id++;
            mr = WinSendMsg (hwnd, MM_QUERYITEM,
                             MPFROM2SHORT (id, FALSE), MPFROMP (&mi));
            if (!SHORT1FROMMR (mr)) return (FALSE);
            if (strcmp (str + (*p)->str_offset, str0 + (*p0)->str_offset) != 0)
              WinSendMsg (hwnd, MM_SETITEMTEXT, MPFROMSHORT (id),
                          MPFROMP (str + (*p)->str_offset));
            (*p) += 2; (*p0) += 2;
            update_menu (f, mi.hwndSubMenu, p, str, p0, str0, sub);
          }
        else
          {
            id = IDM_MENUBAR + add_menubar_id_map (f, (*p)->item);
            if ((*p)->enable != (*p0)->enable)
              {
                WinSendMsg (hwnd, MM_SETITEMATTR, MPFROM2SHORT (id, FALSE),
                            MPFROM2SHORT (MIA_DISABLED,
                                          (*p)->enable ? 0 : MIA_DISABLED));
              }
            if (strcmp (str + (*p)->str_offset, str0 + (*p0)->str_offset) != 0)
              WinSendMsg (hwnd, MM_SETITEMTEXT, MPFROMSHORT (id),
                          MPFROMP (str + (*p)->str_offset));
            ++(*p); ++(*p0);
          }
        break;

      case PMMENU_SEP:
        ++(*p); ++(*p0);
        break;

      case PMMENU_SUB:
        if (sub)
          return (TRUE);
        id = submenu_id++;
        mr = WinSendMsg (hwnd, MM_QUERYITEM,
                         MPFROM2SHORT (id, FALSE), MPFROMP (&mi));
        if (!SHORT1FROMMR (mr)) return (FALSE);
        if (strcmp (str + (*p)->str_offset, str0 + (*p0)->str_offset) != 0)
          WinSendMsg (hwnd, MM_SETITEMTEXT, MPFROMSHORT (id),
                      MPFROMP (str + (*p)->str_offset));
        ++(*p); ++(*p0);
        update_menu (f, mi.hwndSubMenu, p, str, p0, str0, TRUE);
        break;

      default:
        abort ();
      }
}


/* Update the menubar. */

static void create_menubar (frame_data *f, pm_menu *els, char *str,
                            int el_count, int str_bytes)
{
  HWND hwnd;
  MENUITEM mi;
  SWP swp;
  POINTL aptl[2];
  RECTL rcl;
  MRESULT mr;
  pm_menu *p, *p0;
  char *str0;
  int i;

  if (els == NULL)
    {
      if (f->initial_menubar_el_count == 0)
        return;
      els = f->cur_menubar_el;
      str = f->cur_menubar_str;
      el_count = f->initial_menubar_el_count;
      str_bytes = f->initial_menubar_str_bytes;
    }
  else if (f->stage < STAGE_READY)
    {
      f->initial_menubar_el_count = el_count;
      f->initial_menubar_str_bytes = str_bytes;
      goto update_cur_menubar;
    }
  /* Quickly check if anything changed. */
  else if (f->cur_menubar_el != NULL && f->cur_menubar_str != NULL
      && el_count <= f->cur_menubar_el_allocated)
    {
      if (str_bytes <= f->cur_menubar_str_allocated
          && memcmp (els, f->cur_menubar_el, el_count * sizeof (pm_menu)) == 0
          && memcmp (str, f->cur_menubar_str, str_bytes) == 0)
        {
          /* Nothing changed at all.  This is the most common case. */

          return;
        }

      /* Now check if everything is unchanged except for the `enabled'
         state and texts.  If only the `enabled' state and texts have
         changed, modifying the menubar in place is very simple.  If
         the title of any menubar item (top level menu) has changed,
         we rebuild the menubar. */

      p = els; p0 = f->cur_menubar_el; str0 = f->cur_menubar_str;
      for (i = 0; i < el_count && p->type == p0->type; ++i, ++p, ++p0)
        {
          if (p->type == PMMENU_TOP
              && strcmp (str + p->str_offset, str0 + p0->str_offset) != 0)
            break;
        }
      if (i >= el_count)
        {
          /* Only simple changes were made, the submenu tree is
             unchanged.  Update the menubar in place without deleting
             or creating windows. */

          f->menubar_id_used = 0;
          submenu_id = IDM_SUBMENU;
          p = els; p0 = f->cur_menubar_el; i = IDM_MENUBAR_TOP;
          while (p->type == PMMENU_TOP)
            {
              ++p; ++p0;
              mr = WinSendMsg (f->hwndMenubar, MM_QUERYITEM,
                               MPFROM2SHORT (i, FALSE), MPFROMP (&mi));
              ++i;
              if (!SHORT1FROMMR (mr))
                {
                  DosBeep (100, 500); goto replace_menubar;
                }
              if (!update_menu (f, mi.hwndSubMenu, &p, str, &p0, str0, FALSE))
                {
                  DosBeep (600, 500); goto replace_menubar;
                }
              if (p->type != PMMENU_END)
                break;
              ++p; ++p0;        /* Skip over PMMENU_END record */
            }
          goto update_cur_menubar;
        }
    }

replace_menubar:
  WinQueryWindowPos (f->hwndClient, &swp);
  aptl[0].x = swp.x;
  aptl[0].y = swp.y;
  aptl[1].x = swp.x + swp.cx;
  aptl[1].y = swp.y + swp.cy;
  WinMapWindowPoints (f->hwndFrame, HWND_DESKTOP, aptl, 2);
  rcl.xLeft = aptl[0].x; rcl.yBottom = aptl[0].y;
  rcl.xRight = aptl[1].x; rcl.yTop = aptl[1].y;

  hwnd = WinWindowFromID (f->hwndFrame, FID_MENU);
  if (hwnd != NULLHANDLE)
    WinDestroyWindow (hwnd);

  hwnd = WinCreateWindow (f->hwndFrame, WC_MENU, "", MS_ACTIONBAR, 0, 0, 0, 0,
                          f->hwndFrame, HWND_TOP, FID_MENU, NULL, NULL);
  f->menubar_id_used = 0;
  submenu_id = IDM_SUBMENU;
  p = els; i = IDM_MENUBAR_TOP;
  while (p->type == PMMENU_TOP)
    {
      mi.iPosition = MIT_END;
      mi.afStyle = MIS_SUBMENU | MIS_TEXT;
      mi.hwndSubMenu = create_menu_handle (f->hwndFrame, 0);
      mi.afAttribute = 0;
      mi.hItem = 0;
      mi.id = i++;
      WinSendMsg (hwnd, MM_INSERTITEM, MPFROMP (&mi),
                  MPFROMP (str + p->str_offset));
      ++p;
      create_menu (f, mi.hwndSubMenu, f->hwndFrame, &p, str, TRUE);
      if (p->type != PMMENU_END)
        break;
      ++p;                      /* Skip over PMMENU_END record */
    }
  set_menu_font (hwnd, f->menu_font);
  f->ignore_wm_size = TRUE;
  WinSendMsg (f->hwndFrame, WM_UPDATEFRAME, MPFROMSHORT (FCF_MENU), 0);
  f->ignore_wm_size = FALSE;

  WinCalcFrameRect (f->hwndFrame, &rcl, FALSE);
  if (rcl.yTop > swpScreen.cy)
    {
      LONG offset = rcl.yTop - swpScreen.cy;
      rcl.yBottom -= offset;
      rcl.yTop -= offset;
    }
  WinSetWindowPos (f->hwndFrame, HWND_TOP, rcl.xLeft, rcl.yBottom,
                   rcl.xRight - rcl.xLeft, rcl.yTop - rcl.yBottom,
                   SWP_MOVE | SWP_SIZE);
  f->hwndMenubar = hwnd;

update_cur_menubar:
  if (el_count > f->cur_menubar_el_allocated)
    {
      deallocate (f->cur_menubar_el);
      f->cur_menubar_el = allocate (el_count * sizeof (pm_menu));
      f->cur_menubar_el_allocated = el_count;
    }
  memmove (f->cur_menubar_el, els, el_count * sizeof (pm_menu));
  if (str_bytes > f->cur_menubar_str_allocated)
    {
      deallocate (f->cur_menubar_str);
      f->cur_menubar_str = allocate (str_bytes);
      f->cur_menubar_str_allocated = str_bytes;
    }
  memmove (f->cur_menubar_str, str, str_bytes);
}


/* Remove all items of the system menu of HWND, except for items ID1
   and ID2. */

static void sysmenu_remove_all_but (HWND hwnd, SHORT id1, SHORT id2)
{
  HWND hwndSysMenu;
  MENUITEM mi;
  SHORT i, id;
  MRESULT mr;

  hwndSysMenu = WinWindowFromID (hwnd, FID_SYSMENU);
  if (hwndSysMenu == NULLHANDLE)
    return;
  mr = WinSendMsg (hwndSysMenu, MM_QUERYITEM,
                   MPFROM2SHORT (SC_SYSMENU, FALSE), (MPARAM)&mi);
  if (!SHORT1FROMMR (mr) || mi.hwndSubMenu == NULLHANDLE)
    return;
  hwndSysMenu = mi.hwndSubMenu;
  mr = WinSendMsg (hwndSysMenu, MM_QUERYITEMCOUNT,
                   MPFROMSHORT (0), MPFROMSHORT (0));
  for (i = SHORT1FROMMR (mr); i >= 0; --i)
    {
      mr = WinSendMsg (hwndSysMenu, MM_ITEMIDFROMPOSITION,
                       MPFROMSHORT (i), MPFROMSHORT (0));
      id = SHORT1FROMMR (mr);
      if (id != MID_ERROR && id != id1 && id != id2)
        WinSendMsg (hwndSysMenu, MM_DELETEITEM,
                    MPFROM2SHORT (id, FALSE), 0L);
    }
}


/* Dialog procedure for dialog boxes created by create_dialog(). */

static MRESULT DlgProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  int i;

  switch (msg)
    {
    case WM_INITDLG:

      /* SAA/CUA 89 requires all non-available menu choices being
         removed (from the system menu).  For dialog boxes, only the
         Move and Close choices remain.  As we don't want to have to
         handle the Close choice, remove all choices but the Move
         choice. */

      sysmenu_remove_all_but (hwnd, (SHORT)SC_MOVE, (SHORT)SC_MOVE);

      /* Setting WS_DISABLED in the dialog template makes the controls
         invisible, therefore we disable disabled buttons now. */

      for (i = 0; i < sizeof (disabled_buttons); ++i)
        if (disabled_buttons[i])
          WinEnableWindow (WinWindowFromID (hwnd, i), FALSE);

      /* The focus hasn't changed. */

      return ((MRESULT)FALSE);

    case WM_COMMAND:

      /* A button was pressed.  Dissmiss the dialog, letting
         WinProcessDlg return the button number ID. */

      WinDismissDlg (hwnd, COMMANDMSG (&msg)->cmd);
      break;
    }
  return (WinDefDlgProc (hwnd, msg, mp1, mp2));
}


/* Compute the width of a string, in dialog box units. */

static int dialog_string_width (const char *s)
{
  size_t len;
  POINTL *pptl;

  /* Allocate an ARRAY of POINTL structures.  There's one POINTL
     structure for each character, and one for the position after the
     last character. */

  len = strlen (s);
  if (len == 0)
    return (0);
  pptl = alloca ((len + 1) * sizeof (POINTL));

  /* Compute the positions of the characters. */

  if (!GpiQueryCharStringPos (hpsScreen, 0, len, s, NULL, pptl))
    return (0);

  /* Compute the distance between the X position after drawing the
     last character and initial X position.  Convert the distance to
     dialog box units. */

  return (PIX_TO_DLGUNIT_X (pptl[len].x - pptl[0].x));
}


/* Create a dialog and return its window handle. */

static HWND create_dialog (frame_data *f, int buttons, pm_menu *p, char *str)
{
  HWND hwnd;
  PDLGTEMPLATE pdlgt;
  DLGTITEM *item, *dialog_item, *msg_item = NULL;
  USHORT cb;
  LONG dialog_width, dialog_height; /* Pixels */
  ULONG ctldata;
  int i, items, x, cx;
  char *tpl, *s;
  char *title = "Emacs";
  char *msg = NULL;

  /* If there is a PMMENU_TITLE entry, it's the first entry. */

  if (p->type == PMMENU_TITLE)
    {
      msg = str + p->str_offset;
      ++p;
    }

  /* Compute the number of items in the DLGTEMPLATE. */

  items = buttons + 1;          /* DIALOG, plus one PUSHBUTTON per button */
  if (msg != NULL)
    items += 2;                 /* ICON, LTEXT */

  /* Compute the size of the fixed part of the template. */

  cb = sizeof (DLGTEMPLATE) + (items - 1) * sizeof (DLGTITEM);

  /* Allocate a buffer for the template, providing plenty space for
     the variable part (strings etc). */

  tpl = alloca (cb + 8192);

  /* Fill-in the values for DLGTEMPLATE. */

  pdlgt = (PDLGTEMPLATE)tpl;
  pdlgt->type = 0;              /* Only format 0 is defined */
  pdlgt->codepage = DIALOG_CODE_PAGE;
  pdlgt->offadlgti = offsetof (DLGTEMPLATE, adlgti);
  pdlgt->fsTemplateStatus = 0;  /* Reserved, must be 0 */
  pdlgt->iItemFocus = (USHORT)(-1);
  pdlgt->coffPresParams = 0;

  item = pdlgt->adlgti;

  /* Add a DIALOG `control'.  Only this control has children.  The x,
     y, and cx members are set further down, when the width of the
     dialog is known. */

  dialog_item = item++;
  dialog_item->fsItemStatus = 0; /* Reserved, must be 0 */
  dialog_item->cChildren = items - 1; /* All but DIALOG */
  dialog_item->cchClassName = 0;
  dialog_item->offClassName = (USHORT)(ULONG)WC_FRAME & 0xffff;
  dialog_item->cchText = strlen (title);
  dialog_item->offText = cb;
  strcpy (tpl + cb, title); cb += strlen (title) + 1;
  dialog_item->flStyle = WS_CLIPSIBLINGS | WS_SAVEBITS | FS_DLGBORDER;
  dialog_item->cy = 44;  /* TODO */
  dialog_item->id = 0;
  dialog_item->offPresParams = (USHORT)(-1);
  dialog_item->offCtlData = cb;

  ctldata = FCF_TITLEBAR | FCF_SYSMENU;
  memcpy (tpl + cb, &ctldata, sizeof (ctldata)); cb += sizeof (ctldata);

  /* Now come the controls to be placed in the DIALOG.  Children
     follow the parent in the array of DLGTITEMs. */

  /* If a message is provided, add controls for the icon (SYSICON) and
     the message (LTEXT). */

  if (msg != NULL)
    {
      char *icon = "#12";       /* SPTR_ICONQUESTION */

      x = 10;

      item->fsItemStatus = 0;
      item->cChildren = 0;
      item->cchClassName = 0;
      item->offClassName = (USHORT)(ULONG)WC_STATIC & 0xffff;
      item->cchText = strlen (icon);
      item->offText = cb;
      strcpy (tpl + cb, icon); cb += strlen (icon) + 1;
      item->flStyle = WS_VISIBLE | SS_SYSICON;
      item->x = x;
      item->y = 28;
      item->cx = 0;
      item->cy = 0;
      item->id = 0;
      item->offPresParams = (USHORT)(-1);
      item->offCtlData = (USHORT)(-1);
      ++item;
      x += PIX_TO_DLGUNIT_X (cxIcon) + 8;

      cx = dialog_string_width (msg);
      msg_item = item++;
      msg_item->fsItemStatus = 0;
      msg_item->cChildren = 0;
      msg_item->cchClassName = 0;
      msg_item->offClassName = (USHORT)(ULONG)WC_STATIC & 0xffff;
      msg_item->cchText = strlen (msg);
      msg_item->offText = cb;
      strcpy (tpl + cb, msg); cb += strlen (msg) + 1;
      msg_item->flStyle = WS_VISIBLE | WS_GROUP | SS_TEXT;
      msg_item->x = x;
      msg_item->y = 28;
      msg_item->cx = cx + 4;
      msg_item->cy = 8;
      msg_item->id = 0;
      msg_item->offPresParams = (USHORT)(-1);
      msg_item->offCtlData = (USHORT)(-1);
    }

  /* Add the BUTTON controls. */

  i = 0; x = 10;
  memset (disabled_buttons, 0, sizeof (disabled_buttons));
  while (p->type != PMMENU_END)
    {
      switch (p->type)
        {
        case PMMENU_ITEM:
          s = str + p->str_offset;
          cx = dialog_string_width (s) + 4;
          if (cx < 32) cx = 32;
          item->fsItemStatus = 0;
          item->cChildren = 0;
          item->cchClassName = 0;
          item->offClassName = (USHORT)(ULONG)WC_BUTTON & 0xffff;
          item->cchText = strlen (s);
          item->offText = cb;
          strcpy (tpl + cb, s); cb += strlen (s) + 1;
          item->flStyle = (WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_AUTOSIZE);

          item->x = x;
          item->y = 8;
          item->cx = cx;
          item->cy = 12;
          item->id = p->item;
          item->offPresParams = (USHORT)(-1);
          item->offCtlData = (USHORT)(-1);

          if (!p->enable && p->item < sizeof (disabled_buttons))
            disabled_buttons[p->item] = 1;

          x += cx + 10;
          ++item; ++i;
          break;

        default:
          abort ();
        }
      ++p;
    }
  if (i != buttons)
    return (NULLHANDLE);

  /* Compute the width of the dialog. */

  cx = x;
  if (cx < 120)
    cx = 120;
  if (msg_item != NULL && cx < (msg_item->x + msg_item->cx + 4))
    cx = msg_item->x + msg_item->cx + 4;

  dialog_item->cx = cx;

  /* Center the dialog in the frame window. */

  dialog_width = DLGUNIT_TO_PIX_X (dialog_item->cx);
  dialog_height = DLGUNIT_TO_PIX_Y (dialog_item->cy);
  dialog_item->x = PIX_TO_DLGUNIT_X ((f->cxClient - dialog_width) / 2);
  dialog_item->y = PIX_TO_DLGUNIT_Y ((f->cyClient - dialog_height) / 2);

  /* Set the size of the structure. */

  pdlgt->cbTemplate = cb;

  /* Create the dialog window from the template. */

  hwnd = WinCreateDlg (HWND_DESKTOP, f->hwndClient, DlgProc, pdlgt, NULL);
  return (hwnd);
}


MRESULT FileDlgProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
    {
    case FDM_ERROR:
      /* Let the file dialog display the error message.  */
      return (MRESULT)0;

    case FDM_FILTER:
      /* Don't omit any files.  */
      return (MRESULT)TRUE;

    case FDM_VALIDATE:
      /* Check validity.  */
      if (((PFILEDLG)WinQueryWindowULong (hwnd, QWL_USER))->ulUser)
        {
          DosError (FERR_DISABLEHARDERR | FERR_ENABLEEXCEPTION);
          if (access ((char *)PVOIDFROMMP (mp1), 0) != 0)
            {
              pm_message (hwnd, "File %s does not exist",
                          (char *)PVOIDFROMMP (mp1));
              return (MRESULT)FALSE;
            }
        }
      return (MRESULT)TRUE;
    }
  return WinDefFileDlgProc (hwnd, msg, mp1, mp2);
}


/* Display and process a file dialog.  */

static void file_dialog (frame_data *f, pm_filedialog *pdata)
{
  FILEDLG fdg;
  pm_filedialog data;
  char *p;

  data = *pdata;
  DosPostEventSem (hevDone);

  /* Select the default directory as current working directory.  */

  DosError (FERR_DISABLEHARDERR | FERR_ENABLEEXCEPTION);
  _chdir2 (data.dir);

  /* Initialize the FILEDLG structure.  */

  fdg.cbSize = sizeof (FILEDLG);
  fdg.pszTitle = data.title;
  fdg.pszOKButton = data.ok_button;
  fdg.ulUser = data.must_match;
  fdg.fl = (data.save_as ? (FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB)
            : FDS_OPEN_DIALOG);
  fdg.fl |= FDS_CENTER;
  fdg.pfnDlgProc = FileDlgProc;
  fdg.lReturn = 0;
  fdg.lSRC = 0;
  fdg.hMod = 0;
  fdg.usDlgId = 0;
  fdg.x = 0;
  fdg.y = 0;
  strcpy (fdg.szFullFile, data.defalt);
  fdg.pszIType       = NULL;
  fdg.papszITypeList = NULL;
  fdg.pszIDrive      = NULL;
  fdg.papszIDriveList= NULL;
  fdg.sEAType        = 0;
  fdg.papszFQFilename= NULL;
  fdg.ulFQFCount     = 0;

  /* Process the dialog box.  */

  if (WinFileDlg (HWND_DESKTOP, f->hwndClient, &fdg)
      && fdg.lReturn == DID_OK)
    {
      for (p = fdg.szFullFile; *p != 0; ++p)
        if (*p == '\\')
          *p = '/';
      send_answer (data.serial, fdg.szFullFile, strlen (fdg.szFullFile), 0);
    }
  else
    send_answer (data.serial, "", 0, 0);
}


/* Capture the mouse. */

static void capture (frame_data *f, int yes)
{
  if (yes && !capture_flag)
    {
      WinSetCapture (HWND_DESKTOP, f->hwndClient);
      capture_flag = TRUE;
    }
  if (!yes && capture_flag)
    {
      WinSetCapture (HWND_DESKTOP, NULLHANDLE);
      capture_flag = FALSE;
    }
}


/* Mouse button pressed or released.  Compute the coordinates of the
   character cell, convert the modifier bits, and send to Emacs.  This
   function returns FALSE if the message is to be passed on to the
   default window procedure (which in turn passes it on to the frame
   window) because this window doesn't have the focus.  That way, the
   window is made active and the button message is not sent to Emacs.
   Moreover, WM_BUTTONxUP messages are ignored after making the window
   active until an appropriate WM_BUTTONxDOWN message is received. */

static int mouse_button (HWND hwnd, MPARAM mp1, MPARAM mp2, int button,
                         int modifier)
{
  frame_data *f;

  f = WinQueryWindowPtr (hwnd, 0);
  if (hwnd == WinQueryFocus (HWND_DESKTOP))
    {
      if (modifier == down_modifier)
        f->ignore_button_up &= ~(1 << button);
      if (f->buttons[button] != 0
          && (modifier == down_modifier
              || !(f->ignore_button_up & (1 << button))))
        {
          capture (f, modifier == down_modifier);
          modifier |= convert_modifiers (f, (USHORT)SHORT2FROMMP (mp2));
          send_button (f, (USHORT)SHORT1FROMMP (mp1),
                       (USHORT)SHORT2FROMMP (mp1),
                       f->buttons[button] - 1, modifier,
                       WinQueryMsgTime (hab));
          return (TRUE);
        }
    }
  else
    f->ignore_button_up = 7;

  /* Continue processing -- make window active */
  return (FALSE);
}


/* Send a BUTTON_DROP event to Emacs. */

static void send_drop (frame_data *f, LONG x, LONG y, int button,
                       const char *str, unsigned len)
{
  DosRequestMutexSem (drop_mutex, SEM_INDEFINITE_WAIT);
  drop_list[drop_in].cookie = drop_cookie;
  drop_list[drop_in].len = len;
  memcpy (drop_list[drop_in].str, str, len);
  if (++drop_in == DROP_MAX) drop_in = 0;
  ++drop_count;
  DosReleaseMutexSem (drop_mutex);
  send_button (f, x, y, button, 0, drop_cookie);
  ++drop_cookie;
}


/* An object is being dragged over the client window.  Check whether
   it can be dropped or not. */

MRESULT drag_over (PDRAGINFO pDraginfo)
{
  PDRAGITEM pditem;
  USHORT indicator, operation;

/* Redefine this macro for debugging. */
#define DRAG_FAIL(x) ((void)0)

  indicator = DOR_NODROPOP; operation = DO_COPY;
  if (!DrgAccessDraginfo (pDraginfo))
    DRAG_FAIL ("DrgAccessDraginfo failed");
  else if (!(pDraginfo->usOperation == DO_DEFAULT
             || pDraginfo->usOperation == DO_COPY))
    DRAG_FAIL ("Invalid operation");
  else if (DrgQueryDragitemCount (pDraginfo) < 1)
    DRAG_FAIL ("Invalid count");
  else if (DrgQueryDragitemCount (pDraginfo) > DROP_MAX - drop_count)
    DRAG_FAIL ("Circular buffer full");
  else
    {
      pditem = DrgQueryDragitemPtr (pDraginfo, 0);
      if (!(pditem->fsSupportedOps & DO_COPYABLE))
        DRAG_FAIL ("Not copyable");
      else if (!DrgVerifyRMF (pditem, "DRM_OS2FILE", NULL))
        DRAG_FAIL ("DrgVerifyRMF failed");
      else
        {
          /* The object can be dropped (copied). */
          indicator = DOR_DROP; operation = DO_COPY;
        }
    }
  DrgFreeDraginfo (pDraginfo);
  return (MRFROM2SHORT (indicator, operation));
}


/* An object is dropped on the client window. */

MRESULT drag_drop (HWND hwnd, PDRAGINFO pDraginfo)
{
  PDRAGITEM pditem;
  POINTL ptl;
  char name[CCHMAXPATH];
  char path[CCHMAXPATH];
  char *p;
  int count, idx, len;
  frame_data *f;

  f = WinQueryWindowPtr (hwnd, 0);
  DrgAccessDraginfo (pDraginfo);
  ptl.x = pDraginfo->xDrop; ptl.y = pDraginfo->yDrop;
  WinMapWindowPoints (HWND_DESKTOP, hwnd, &ptl, 1);
  count = DrgQueryDragitemCount (pDraginfo);
  for (idx = 0; idx < count && drop_count < DROP_MAX; ++idx)
    {
      pditem = DrgQueryDragitemPtr (pDraginfo, idx);
      DrgQueryStrName (pditem->hstrContainerName, sizeof (path), path);
      DrgQueryStrName (pditem->hstrSourceName, sizeof (name), name);
      DrgSendTransferMsg (pditem->hwndItem, DM_ENDCONVERSATION,
                          MPFROMLONG (pditem->ulItemID),
                          MPFROMSHORT (DMFL_TARGETSUCCESSFUL));
      len = strlen (path);
      if (len >= 1 && strchr ("\\/:", path[len-1]) == NULL)
        path[len++] = '/';
      if (len + strlen (name) + 1 <= sizeof (path))
        {
          strcpy (path + len, name);
          for (p = path; *p != 0; ++p)
            if (*p == '\\')
              *p = '/';
          send_drop (f, ptl.x, ptl.y, BUTTON_DROP_FILE, path, strlen (path));
        }
    }
  DrgDeleteDraginfoStrHandles (pDraginfo);
  DrgFreeDraginfo (pDraginfo);
  return (0);
}


/* Send to Emacs the name of the dropped object associated with
   COOKIE.  Discard all objects dropped before the one associated with
   COOKIE. */

static void get_drop (int cookie, int serial)
{
  struct drop d;

  d.str[0] = 0;
  DosRequestMutexSem (drop_mutex, SEM_INDEFINITE_WAIT);
  while (drop_count > 0)
    {
      if (drop_list[drop_out].cookie == cookie)
        {
          d = drop_list[drop_out];
          if (++drop_out == DROP_MAX) drop_out = 0;
          --drop_count;
          break;
        }
      if (drop_list[drop_out].cookie > cookie)
        break;
      if (++drop_out == DROP_MAX) drop_out = 0;
      --drop_count;
    }
  DosReleaseMutexSem (drop_mutex);
  send_answer (serial, d.str, d.len, 0);
}


/* A color has been dropped on a frame (or the background color has
   been changed with WinSetPresParam). */

static void drop_color (HWND hwnd)
{
  frame_data *f;
  RGB rgb;
  char buf[3];

  f = WinQueryWindowPtr (hwnd, 0);
  if (WinQueryPresParam (hwnd, PP_BACKGROUNDCOLOR, 0, NULL,
                         sizeof (rgb), &rgb,
                         QPF_NOINHERIT | QPF_PURERGBCOLOR) == sizeof (rgb))
    {
      buf[0] = rgb.bRed;
      buf[1] = rgb.bGreen;
      buf[2] = rgb.bBlue;
      send_drop (f, 0, 0, BUTTON_DROP_COLOR, buf, 3);
    }

}


/* Handle PMR_UPDATE_SCROLLBAR.  Derived from code written by Patrick
   Nadeau.  */

static void update_scrollbar (unsigned id, const slider *s)
{
  HWND hwndScrollBar;

  if (id >= MAX_SCROLLBARS)
    return;

  /* Don't update the slider while dragging. */

  if (dragging_scrollbar_slider)
    {
      scrollbar_table[id].async = *s;
      return;
    }

  hwndScrollBar = scrollbar_table[id].hwnd;

  WinSendMsg (hwndScrollBar, SBM_SETSCROLLBAR,
              MPFROMSHORT ((short)s->position),
              MPFROM2SHORT (0, (short)s->whole));
  WinSendMsg (hwndScrollBar, SBM_SETTHUMBSIZE,
              MPFROM2SHORT ((short)s->portion, (short)s->whole),
              0);

  scrollbar_table[id].range = (short)s->whole;
}


/* Handle PMR_MOVE_SCROLLBAR.  Derived from code written by Patrick
   Nadeau.  */

static void move_scrollbar (frame_data *f, unsigned id, int top, int left,
                            int width, int height)
{
  HWND hwndScrollBar;
  LONG x, y, cx, cy;

  /* Don't update the slider while dragging. */

  if (dragging_scrollbar_slider || id >= MAX_SCROLLBARS)
    return;

  hwndScrollBar = scrollbar_table[id].hwnd;

  x = make_x (f, left);
  y = make_y (f, top + height - 1) - f->cyDesc;

  cx = make_x (f, width) - make_x (f, 0);
  cy = make_y (f, 0) - make_y (f, height);

  WinSetWindowPos (hwndScrollBar, HWND_TOP, x, y, cx, cy,
                   SWP_SIZE | SWP_MOVE | SWP_SHOW);
}


/* Handle UWM_CREATE_SCROLLBAR.  Derived from code written by Patrick
   Nadeau.  */

static void create_scrollbar (frame_data *f, pm_create_scrollbar *psb)
{
  HWND hwndScrollBar = NULLHANDLE;
  pmd_create_scrollbar result;
  unsigned id;

  for (id = 0; id < MAX_SCROLLBARS; ++id)
    if (scrollbar_table[id].hwnd == NULLHANDLE)
      break;
  if (id >= MAX_SCROLLBARS)
    id = SB_INVALID_ID;
  else
    {
      hwndScrollBar = WinCreateWindow (f->hwndClient, WC_SCROLLBAR, "",
                                       SBS_VERT | WS_DISABLED,
                                       0, 0, 0, 0,
                                       f->hwndClient, HWND_TOP,
                                       FID_SCROLLBAR + id, NULL, NULL);
      if (hwndScrollBar == NULLHANDLE)
        id = SB_INVALID_ID;
      else
        {
          scrollbar_table[id].hwnd = hwndScrollBar;
          scrollbar_table[id].async.whole = -1;
          update_scrollbar (id, &psb->s);
          move_scrollbar (f, id, psb->top, psb->left, psb->width, psb->height);
        }
    }

  result.id = id;
  send_answer (psb->serial, &result, sizeof (result), 0);
}


/* Handle UWM_DESTROY_SCROLLBAR.  Derived from code written by Patrick
   Nadeau.  */

static void destroy_scrollbar (unsigned id)
{
  if (id >= MAX_SCROLLBARS)
    return;
  if (scrollbar_table[id].hwnd != NULLHANDLE)
    {
      WinDestroyWindow (scrollbar_table[id].hwnd);
      scrollbar_table[id].hwnd = NULLHANDLE;
    }
}


/* Handle WM_VSCROLL.  Originally written by Patrick Nadeau, modified
   by Eberhard Mattes. */

static void handle_vscroll (HWND hwnd, MPARAM mp1, MPARAM mp2)
{
  frame_data *f;
  int part;
  USHORT cmd;
  unsigned id;
  int slider_pos, slider_range;
  pm_event pme;

  id = SHORT1FROMMP (mp1) - FID_SCROLLBAR;
  if (id >= MAX_SCROLLBARS || scrollbar_table[id].hwnd == NULLHANDLE)
    return;

  slider_pos = SHORT1FROMMP (mp2);
  cmd = SHORT2FROMMP (mp2);

  /* PM always gives a slider pos of 0 for events SB_PAGEDOWN,
     SB_PAGEUP, SB_LINEDOWN and SB_LINEUP.

     When this happens we set both the range and the slider pos to 0
     to indicate that the pair is not valid for computing buffer
     positions.

     The only message that does give a valid slider pos is
     SB_SLIDERPOSITION (or SB_SLIDERTRACK) in which case we do set the
     pair to the slider pos and range.

     Hence we set the default slider_range to 0 here.  */
	  
  slider_range = 0;

  /* Avoid updating the slider position and size while dragging.  */

  if (cmd == SB_SLIDERTRACK && !dragging_scrollbar_slider)
    {
      dragging_scrollbar_slider = TRUE;
      scrollbar_table[id].async.whole = -1;
    }
  else if (cmd != SB_SLIDERTRACK && dragging_scrollbar_slider)
    {
      dragging_scrollbar_slider = FALSE;
      if (scrollbar_table[id].async.whole != -1)
        update_scrollbar (id, &scrollbar_table[id].async);
    }

  /* We only generate events for the cases below.  While we are at it
     we figure out what the scroll bar part should be. */

  switch (cmd)
    {
    case SB_PAGEDOWN:
      part = scroll_bar_below_handle;
      break;
    case SB_PAGEUP:
      part = scroll_bar_above_handle;
      break;
    case SB_LINEDOWN:
      part = scroll_bar_down_arrow;
      break;
    case SB_LINEUP:
      part = scroll_bar_up_arrow;
      break;
    case SB_SLIDERPOSITION:
    case SB_SLIDERTRACK:
      part = scroll_bar_handle;
      slider_range = scrollbar_table[id].range;
      break;
    default:
      return;
    }

  f = WinQueryWindowPtr (hwnd, 0);

  pme.scrollbar.header.type = PME_SCROLLBAR;
  pme.scrollbar.header.frame = f->id;

  pme.button.modifiers = up_modifier;
  pme.scrollbar.button = f->buttons[0]-1; /* BUTTON1 in PM */
  pme.scrollbar.part = part;
  pme.scrollbar.x = slider_pos;
  pme.scrollbar.y = slider_range;
  pme.scrollbar.id = id;
  pme.scrollbar.timestamp = WinQueryMsgTime (hab);
  send_event (&pme);
}


/* Client window procedure for the Emacs frames.  This function is
   called in the PM thread. */

static MRESULT ClientWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HPS hps;
  HDC hdc;
  RECTL rcl, rclBound;
  POINTL ptl;
  SIZEL sizel;
  SWP swp;
  ULONG options, cmd;
  HWND hwndDlg;
  PQMSG pqmsg;
  unsigned fsflags, usch, usvk, usrep, ussc, bit;
  pm_event pme;
  frame_data *f;
  menu_data *md;
  menubar_data *mbd;
  int i, dlg_answer;

  switch (msg)
    {
    case WM_CREATE:

      /* Creating the client window.  The pointer to the frame_data
         structure is passed in a global variable as the window is
         create by WinCreateStdWindow.  Save the pointer in the window
         data area. */

      f = new_frame;
      WinSetWindowPtr (hwnd, 0, f);

      /* Setup the fields of the frame_data structure. */

      f->hwndClient = hwnd;
      f->hwndPopupMenu = NULLHANDLE;
      f->hwndMenubar = NULLHANDLE;
      hdc = WinOpenWindowDC (hwnd);
      sizel.cx = sizel.cy = 0;
      f->hpsClient = GpiCreatePS (hab, hdc, &sizel,
                                  PU_PELS | GPIT_MICRO | GPIA_ASSOC);

      /* Switch color table to RGB mode.  This will be overriden by
         make_palette() if the palette manager is used. */

      GpiCreateLogColorTable (f->hpsClient, LCOL_PURECOLOR, LCOLF_RGB,
                              0, 0, NULL);

#ifdef USE_PALETTE_MANAGER
      f->hpal = 0;
      make_palette (f);
#endif

      /* Select the default font and get the font metrics. */

      set_default_font (f);

      /* Put current attributes into cache. */
      f->cur_color = GpiQueryColor (f->hpsClient);
      f->cur_backcolor = GpiQueryBackColor (f->hpsClient);
      f->cur_backmix = GpiQueryBackMix (f->hpsClient);
      f->cur_charset = GpiQueryCharSet (f->hpsClient);
      f->cur_cbox_size = 0;
      return (0);

    case WM_DESTROY:
#ifdef USE_PALETTE_MANAGER
      if (has_palette_manager)
        {
          GpiSelectPalette (f->hpsClient, NULLHANDLE);
          GpiDeletePalette (f->hpal);
        }
#endif
      break;

    case WM_PAINT:

      /* The client window needs repainting.  Let Emacs redraw the
         frame.  Here, we only erase the border of partial character
         boxes which may surround the Emacs frame.  When maximizing a
         window or when calling WinSetWindowPos() in another program,
         the window may be slightly larger than required, leaving a
         border at the right and bottom edges.  */

      f = WinQueryWindowPtr (hwnd, 0);
      hps = WinBeginPaint (hwnd, NULLHANDLE, &rclBound);
      if (f->stage < STAGE_READY)
        {
          WinEndPaint (hps);
          return (0);
        }
      GpiCreateLogColorTable (hps, LCOL_PURECOLOR, LCOLF_RGB, 0, 0, NULL);
      rcl.xLeft = 0; rcl.xRight = f->width * f->cxChar - 1;
      rcl.yTop = f->cyClient; rcl.yBottom = rcl.yTop - f->height * f->cyChar;
      GpiExcludeClipRectangle (hps, &rcl);
      WinQueryWindowRect (hwnd, &rcl);
      WinFillRect (hps, &rcl, GET_COLOR (hps, f->background_color));
      WinEndPaint (hps);

      /* Send a redraw event to Emacs.  The minimal bounding rectangle
         for the update region is sent along with the event.  */

      pme.paint.header.type = PME_PAINT;
      pme.paint.header.frame = f->id;
      pme.paint.x0 = rclBound.xLeft / f->cxChar;
      pme.paint.x1 = rclBound.xRight / f->cxChar;
      pme.paint.y0 = (f->cyClient - rclBound.yTop) / f->cyChar;
      pme.paint.y1 = (f->cyClient - rclBound.yBottom) / f->cyChar;
      send_event (&pme);
      return (0);

#ifdef USE_PALETTE_MANAGER
    case WM_REALIZEPALETTE:
      if (has_palette_manager)
        {
          ULONG size;

          f = WinQueryWindowPtr (hwnd, 0);
          if (WinRealizePalette (hwnd, f->hpsClient, &size) && size != 0)
            WinInvalidateRect (hwnd, NULL, FALSE);
        }
      return ((MRESULT)FALSE);
#endif

    case WM_CLOSE:

      /* The window is to be closed on the user's request.  Send a
         delete-frame (or close-frame) event to Emacs and do not pass
         the message to the default window procedure (otherwise,
         WM_QUIT would be posted to the message queue).  */

      f = WinQueryWindowPtr (hwnd, 0);
#if 0
      pme.header.type = PME_WINDOWDELETE;
      pme.header.frame = f->id;
      send_event (&pme);
#else
      send_key (f, PMK_VIRTUAL, VK_CLOSE_FRAME, 0, 1);
#endif
      return (0);

    case WM_CHAR:

      /* Keyboard message.  Processing keys is quite messy as you will
         learn soon.  First, get the frame data pointer and extract
         the WM_CHAR message flags. */

      f = WinQueryWindowPtr (hwnd, 0);
      fsflags = (USHORT)SHORT1FROMMP (mp1);

      /* Ignore key-up messages.  Processing key-up messages might be
         useful for VK_NEWLINE and VK_ENTER (see below), but isn't
         implemented. */

      if (fsflags & KC_KEYUP)
        return (0);

      /* Extract the repeat count, the character code and the virtual
         key code. */

      usrep = CHAR3FROMMP (mp1);
      ussc = CHAR4FROMMP (mp1);
      usch = (USHORT)SHORT1FROMMP (mp2);
      usvk = (USHORT)SHORT2FROMMP (mp2);

      /* If it's a deadkey, remember the character code (it should
         have one) as it might be required if the user depresses a key
         which cannot be turned into a composite character.  Return
         after storing the character code. */

      if (fsflags & KC_DEADKEY)
        {
          f->deadkey = usch;
          return (0);
        }

      /* Handle invalid compositions: after depressing a deadkey, the
         user has depressed a key which cannot be turned into a
         composite character.  Insert the deadkey (accent) and
         continue processing with the second character.  We have to
         decrement the repeat count (that's not documented). */

      if (fsflags & KC_INVALIDCOMP)
        {
          if (f->deadkey == 0)
            return (0);
          send_key (f, PMK_ASCII, f->deadkey, 0, 1);
          if (usrep > 1)
            --usrep;
        }
      f->deadkey = 0;

      /* Ignore `ALT + keypad digit' keys -- they're used for entering
         the decimal code of a character and should not be seen by
         Emacs.  After releasing the ALT key, a WM_CHAR message for
         the character is sent where all the flag bits except KC_CHAR
         are zero.  Note that KC_VIRTUALKEY is not set for ALT-5,
         therefore we have to check the scan code to distinguish the
         keypad digit keys from the main keyboard digit keys.  This is
         dirty. */

      if ((fsflags & (KC_ALT|KC_SCANCODE|KC_CHAR)) == (KC_ALT|KC_SCANCODE))
        switch (ussc)
          {
          case 0x47: case 0x48: case 0x49:   /* 7 8 9 */
          case 0x4b: case 0x4c: case 0x4d:   /* 4 5 6 */
          case 0x4f: case 0x50: case 0x51:   /* 1 2 3 */
          case 0x52:                         /* 0     */
            return (0);
          }

      /* Handle some special virtual keys first. */

      if (fsflags & KC_VIRTUALKEY)
        switch (usvk)
          {
          case VK_ESC:

            /* The KC_CHAR bit is not set for the VK_ESC key.
               Therefore we have to handle that key here. */

            send_key (f, PMK_ASCII, 0x1b, fsflags, usrep);
            return (0);

          case VK_TAB:

            /* The KC_CHAR bit is not set for the VK_TAB key if code
               page 1004 is in effect.  Therefore we have to handle
               that key here. */

            if (fsflags & (KC_SHIFT|KC_CTRL|KC_ALT|KC_ALTGR))
              send_key (f, PMK_VIRTUAL, VK_TAB, fsflags, usrep);
            else
              send_key (f, PMK_ASCII, 0x09, fsflags, usrep);
            return (0);

          case VK_BACKSPACE:

            /* The KC_CHAR bit is set for the VK_BACKSPACE key.  As we
               want a `backspace' event instead of character 0x08, we
               handle VK_BACKSPACE here. */

            send_key (f, PMK_VIRTUAL, VK_BACKSPACE, fsflags, usrep);
            return (0);

          case VK_SPACE:

            /* Use character code 0x20 for space (with modifiers).  Do
               not do this if KC_CHAR is set, as space following a
               dead key should yield the accent. */

            if (!(fsflags & KC_CHAR))
              {
                send_key (f, PMK_ASCII, 0x20, fsflags, usrep);
                return (0);
              }
            break;

          case VK_NEWLINE:

            /* The NEWLINE and ENTER keys don't generate a code when
               depressed while ALT or SHIFT are depressed.  A code is
               generated when NEWLINE or ENTER are released!  This is
               probably a hack for people who keep their fingers too
               long on the shift keys, like me.  CTRL, however, works.
               For convenience (and compatibility), we translate C-RET
               to LFD here. */

            send_key (f, PMK_ASCII, (fsflags & KC_CTRL ? 0x0a : 0x0d),
                      0, usrep);
            return (0);

          case VK_ENTER:

            /* See above. */

            if (fsflags & KC_CTRL)
              send_key (f, PMK_ASCII, 0x0a, 0, usrep);
            else
              send_key (f, PMK_VIRTUAL, VK_ENTER, 0, usrep);
            return (0);
          }

      /* Treat keypad keys specially.  Check both the character code
         and the scan code to avoid problems with faked WM_CHAR
         events. */

      if ((fsflags & (KC_CHAR|KC_SCANCODE)) && ussc >= 0x37 && ussc <= 0x5c)
        for (i = 0; i < sizeof (keypad) / sizeof (keypad[0]); ++i)
          if (keypad[i].ch == usch && keypad[i].sc == ussc)
            {
              send_key (f, PMK_VIRTUAL, keypad[i].vk,
                        fsflags & ~keypad[i].clr, usrep);
              return (0);
            }

      /* If a character code is present, use that and throw away all
         the modifiers.  Letters with KC_ALT or KC_CTRL set don't have
         KC_CHAR set.  With a non-US keyboard, however, the
         Presentation Manager seems to ignore the CTRL key when the
         right ALT key (ALTGR) is depressed.  That is, KC_CHAR and
         KC_CTRL are set and the character code is not changed by the
         CTRL key.

         If the (left) ALT key is depressed together with the right
         ALT key (ALTGR), KC_CHAR is not set, that is, a user with a
         German keyboard cannot enter M-{ by typing ALT+ALTGR+7.  He
         or she has to type ESC { instead. */

      if (fsflags & KC_CHAR)
        {
          if ((fsflags & KC_CTRL) && (usch >= 0x40 && usch <= 0x5f))
            send_key (f, PMK_ASCII, usch & 0x1f, 0, usrep);
          else
            send_key (f, PMK_ASCII, usch, fsflags & KC_CTRL, usrep);
          return (0);
        }

      /* Now process keys depressed while the CTRL key is down.
         KC_CHAR and KC_VIRTUALKEY are not set for ASCII keys when the
         CTRL key is down.  If both the ALT and CTRL keys are down,
         process the key here.  Remove the KC_CTRL modifier and
         translate the character code if the character is a regular
         control character.  Otherwise, keep the character code and
         set the CTRL modifier.  Ignore keys for which the character
         code is zero.  This happens for the / key on the numeric
         keypad.  If the character is Ctrl-G, we send a signal instead
         of an event. */

      if ((fsflags & (KC_CTRL|KC_VIRTUALKEY)) == KC_CTRL)
        {
          if (usch >= 0x40 && usch <= 0x7f)
            {
              send_key (f, PMK_ASCII, usch & 0x1f, fsflags & KC_ALT, usrep);
              return (0);
            }
          else if ((usch & 0xff) != 0)
            {
              send_key (f, PMK_ASCII, usch, fsflags & (KC_CTRL|KC_ALT), usrep);
              return (0);
            }
        }

      /* Now process keys depressed while the ALT key is down (and the
         CTRL key is up).  Simply keep the character code and the ALT
         modifier.  Ignore keys for which the character code is zero.
         This happens for Alt+Altgr+@, for instance.  The reason is
         unknown. */

      if ((fsflags & (KC_ALT|KC_VIRTUALKEY)) == KC_ALT && (usch & 0xff) != 0)
        {
          send_key (f, PMK_ASCII, usch, KC_ALT, usrep);
          return (0);
        }

      /* If the key is a virtual key and hasn't been processed above,
         generate a system-defined keyboard event and keep all the
         modifiers (except SHIFT, in some cases).  Messages generated
         for shift keys etc. are ignored. */

      if (fsflags & KC_VIRTUALKEY)
        switch (usvk)
          {
          case VK_SHIFT:
          case VK_CTRL:
          case VK_ALT:
          case VK_ALTGRAF:
          case VK_NUMLOCK:
          case VK_CAPSLOCK:

            /* Ignore these keys. */

            return (0);

          case VK_BACKTAB:

            /* Remove the SHIFT modifier for the backtab key. */

            send_key (f, PMK_VIRTUAL, usvk, fsflags & ~KC_SHIFT, usrep);
            return (0);

          default:

            /* Remove the SHIFT modifier for the keys of the numeric
               keypad that share a number key (including . or ,) and a
               virtual key.  Otherwise, the HOME key, for instance,
               would yield `S-home' instead of `home' in numlock
               mode.  We look at the scan codes, which is dirty. */

            switch (ussc)
              {
              case 0x47: case 0x48: case 0x49:   /* 7 8 9 */
              case 0x4b: case 0x4c: case 0x4d:   /* 4 5 6 */
              case 0x4f: case 0x50: case 0x51:   /* 1 2 3 */
              case 0x52: case 0x53:              /* 0 .   */
                fsflags &= ~KC_SHIFT;
                break;
              }

            send_key (f, PMK_VIRTUAL, usvk, fsflags, usrep);
            return (0);
          }

      /* Generate keyboard events for keys that are neither character
         keys not virtual keys.  Both the KC_VIRTUALKEY and KC_CHAR
         bits are zero. We have to look at the scan code, which is
         dirty. */

      if (fsflags & KC_SCANCODE)
        switch (ussc)
          {
          case 0x4c:            /* `CENTER' key, `5' */
            send_key (f, PMK_VIRTUAL, VK_KP_CENTER, fsflags & ~KC_SHIFT, usrep);
            return (0);

          case 0x5c:            /* `DIVIDE' key on numeric keypad */

            /* This key doesn't generate a character code if the
               German keyboard layout is active.  Looks like a bug. */

            send_key (f, PMK_ASCII, '/', fsflags, usrep);
            return (0);

          default:

            /* When hitting a letter or digit key while the right Alt
               key (AltGr on most non-US keyboards) is down, neither
               KC_ALT nor KC_CHAR nor KC_VIRTUALKEY is set.  (On US
               keyboards, the right Alt key is equivalent to the left
               Alt key.)  Fortunately, the character code seems to be
               valid.  Use altgr_modifier and the character code if
               the character code looks valid.  Note that AltGr is
               used on most non-US keyboards to enter special
               characters; on the German keyboard, for instance, you
               have to type AltGr+q to get @.  These key combinations
               have been handled above (KC_CHAR is set). */

            if (usch > 0)
              send_key (f, PMK_ASCII, usch, fsflags | KC_ALTGR, usrep);
            break;
          }
      return (0);

    case WM_TRANSLATEACCEL:

      /* Examine accelerator table.  We ignore the accelerator table
         for keys which are not listed in the shortcuts frame
         parameter.  By ignoring the accelerator table, we receive
         WM_CHAR messages for F1, F10, A-f4, A-space etc. */

      f = WinQueryWindowPtr (hwnd, 0);

      /* If all shortcuts are disabled, don't check the character.  */

      if (f->shortcuts == 0)
        return (FALSE);

      /* Get the parameters of the WM_CHAR message.  */

      pqmsg = PVOIDFROMMP (mp1);
      fsflags = (USHORT)SHORT1FROMMP (pqmsg->mp1);
      usch = (USHORT)SHORT1FROMMP (pqmsg->mp2);
      usvk = (USHORT)SHORT2FROMMP (pqmsg->mp2);

      /* Compute the SHORTCUT_* bit. */

      bit = 0;
      if (fsflags & KC_VIRTUALKEY)
        switch (usvk)
          {
          case VK_ALT:
            bit = SHORTCUT_ALT;
            break;
          case VK_ALTGRAF:
            bit = SHORTCUT_ALTGR;
            break;
          case VK_F1:
            bit = SHORTCUT_F1;
            break;
          case VK_F4:
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_F4;
            break;
          case VK_F5:
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_F5;
            break;
          case VK_F6:
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_F6;
            break;
          case VK_F7:
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_F7;
            break;
          case VK_F8:
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_F8;
            break;
          case VK_F9:
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_F9;
            break;
          case VK_F10:
            bit = (fsflags & KC_ALT) ? SHORTCUT_ALT_F10 : SHORTCUT_F1;
            break;
          case VK_F11:
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_F11;
            break;
          }
      else if (fsflags & KC_CHAR)
        switch (usch)
          {
          case ' ':
            if (fsflags & KC_ALT) bit = SHORTCUT_ALT_SPACE;
            break;
          }

      /* If the key is known and not enabled in f->shortcuts, ignore
         the accelerator table. */

      if (bit != 0 && !(f->shortcuts & bit))
        return (FALSE);
      break;

    case WM_COMMAND:

      /* Menu or button or accelerator. */

      f = WinQueryWindowPtr (hwnd, 0);
      cmd = COMMANDMSG (&msg)->cmd;
      if (COMMANDMSG (&msg)->source == CMDSRC_MENU
          && cmd >= IDM_MENU && cmd <= IDM_MENU_LAST)
        {
          send_answer (menu_serial, NULL, 0, cmd - IDM_MENU);
          f->popup_active = FALSE;
          return (0);
        }
      else if (COMMANDMSG (&msg)->source == CMDSRC_MENU
               && cmd >= IDM_MENUBAR && cmd <= IDM_MENUBAR_LAST)
        {
          /* An item of a submenu of the menubar. */

          pme.menubar.header.type = PME_MENUBAR;
          pme.menubar.header.frame = f->id;
          pme.menubar.number = f->menubar_id_map[cmd - IDM_MENUBAR];
          send_event (&pme);
          return (0);
        }
      break;

    case WM_MENUEND:

      /* Menu terminates.  If menu_selected is FALSE, the user hasn't
         selected a menu item of a popup menu.  That is, the menu was
         dismissed by pressing ESC or clicking outside the menu.  We
         send a 0 to Emacs by faking a menu selection. */

      f = WinQueryWindowPtr (hwnd, 0);
      if ((HWND)LONGFROMMP (mp2) == f->hwndPopupMenu)
        {
          if (!menu_selected)
            {
              send_answer (menu_serial, NULL, 0, 0);
              f->popup_active = FALSE;
            }
        }
      return (0);

    case WM_MENUSELECT:

      /* A menu item has been selected.  Set menu_selected if
         appropriate. */

      if (SHORT2FROMMP (mp1))
        {
          f = WinQueryWindowPtr (hwnd, 0);
          cmd = SHORT1FROMMP (mp1);
          if (cmd >= IDM_MENU && cmd <= IDM_MENU_LAST && f->popup_active)
            menu_selected = TRUE;
        }
      break;

    case WM_BUTTON1DOWN:
      if (mouse_button (hwnd, mp1, mp2, 0, down_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON1UP:
      if (mouse_button (hwnd, mp1, mp2, 0, up_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON1DBLCLK:
      if (mouse_button (hwnd, mp1, mp2, 0, down_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON2DOWN:
      if (mouse_button (hwnd, mp1, mp2, 1, down_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON2UP:
      if (mouse_button (hwnd, mp1, mp2, 1, up_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON2DBLCLK:
      if (mouse_button (hwnd, mp1, mp2, 1, down_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON3DOWN:
      if (mouse_button (hwnd, mp1, mp2, 2, down_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON3UP:
      if (mouse_button (hwnd, mp1, mp2, 2, up_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_BUTTON3DBLCLK:
      if (mouse_button (hwnd, mp1, mp2, 2, down_modifier))
        return ((MRESULT)TRUE);
      break;

    case WM_SIZE:

      /* The size of the window has changed.  Store the new size and
         recreate the cursor.  The cursor must be recreated to install
         the new clipping rectangle.  While minimizing or restoring
         the window, this message is ignored. */

      f = WinQueryWindowPtr (hwnd, 0);
      if (f->ignore_wm_size || f->stage < STAGE_READY)
        break;
      if (f->minimizing)
        {
          f->minimizing = FALSE;
          break;
        }
      if (f->restoring)
        {
          f->restoring = FALSE;
          set_size (f);
          break;
        }
      f->cxClient = SHORT1FROMMP (mp2);
      f->cyClient = SHORT2FROMMP (mp2);
      set_cursor (f);
      /* Notify Emacs of the new window size. */
      report_size (f);
      break;

    case WM_MOVE:

      /* The position of the window has been changed. */

      f = WinQueryWindowPtr (hwnd, 0);
      WinQueryWindowPos (f->hwndClient, &swp);
      pme.framemove.header.type = PME_FRAMEMOVE;
      pme.framemove.header.frame = f->id;
      ptl.x = swp.x; ptl.y = swp.y;
      WinMapWindowPoints (f->hwndFrame, HWND_DESKTOP, &ptl, 1);
      pme.framemove.top = swpScreen.cy - 1 - (ptl.y + swp.cy);
      pme.framemove.left = ptl.x;
      send_event (&pme);
      break;

    case WM_MOUSEMOVE:
      if (track_mouse)
        {
          f = WinQueryWindowPtr (hwnd, 0);
          pme.mouse.x = charbox_x (f, (USHORT)SHORT1FROMMP (mp1));
          pme.mouse.y = charbox_y (f, (USHORT)SHORT2FROMMP (mp1));
          if (pme.mouse.x >= 0 && pme.mouse.y >= 0
              && (pme.mouse.x != track_x || pme.mouse.y != track_y))
            {
              pme.mouse.header.type = PME_MOUSEMOVE;
              pme.mouse.header.frame = f->id;
              send_event (&pme);
              track_x = pme.mouse.x; track_y = pme.mouse.y;
            }
        }
      break;

    case WM_SETFOCUS:

      /* Create the cursor when receiving the input focus, destroy the
         cursor when loosing the input focus. */

      f = WinQueryWindowPtr (hwnd, 0);
      if (SHORT1FROMMP (mp2))
        create_cursor (f);
      else
        WinDestroyCursor (hwnd);
      return (0);

    case WM_SHOW:

      /* Call initial_show_frame() if a frame is initially made
         visible via the Window List.  Also notify Emacs. */

      if (SHORT1FROMMP (mp1))
        {
          f = WinQueryWindowPtr (hwnd, 0);
          if (f->stage == STAGE_INVISIBLE)
            {
              /* Don't call initial_show_frame() from within WM_SHOW
                 processing. */
              WinPostMsg (f->hwndClient, UWM_INITIAL_SHOW, MPFROMP (f), 0);
              /* Notify Emacs. */
              pme.header.type = PME_RESTORE;
              pme.header.frame = f->id;
              send_event (&pme);
            }
        }
      return (0);

    case WM_VSCROLL:
      handle_vscroll (hwnd, mp1, mp2);
      return (0);

    case WM_PRESPARAMCHANGED:
      if (LONGFROMMP (mp1) == PP_BACKGROUNDCOLOR)
        drop_color (hwnd);
      return 0;

    case DM_DRAGOVER:

      /* Determine whether the object can be dropped. */

      return (drag_over ((PDRAGINFO)mp1));

    case DM_DROP:

      /* Drop an object. */

      return (drag_drop (hwnd, (PDRAGINFO)mp1));

    case UWM_POPUPMENU:
      f = (frame_data *)mp1;
      md = (menu_data *)mp2;
      if (f->hwndPopupMenu != NULLHANDLE)
        WinDestroyWindow (f->hwndPopupMenu);
      f->hwndPopupMenu = create_popup_menu (f, md->data, md->str);
      options = (PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_MOUSEBUTTON3
                 | PU_KEYBOARD | PU_HCONSTRAIN | PU_VCONSTRAIN);
      if (md->align_top)
        options |= PU_POSITIONONITEM;
      if (md->button >= 1 && md->button <= 3)
        for (i = 0; i < 3; ++i)
          if (f->buttons[i] == md->button)
            switch (i)
              {
              case 0:
                if (WinGetKeyState (HWND_DESKTOP, VK_BUTTON1) & 0x8000)
                  options |= PU_MOUSEBUTTON1DOWN;
                break;
              case 1:
                if (WinGetKeyState (HWND_DESKTOP, VK_BUTTON2) & 0x8000)
                  options |= PU_MOUSEBUTTON2DOWN;
                break;
              case 2:
                if (WinGetKeyState (HWND_DESKTOP, VK_BUTTON3) & 0x8000)
                  options |= PU_MOUSEBUTTON3DOWN;
                break;
              }
      menu_selected = FALSE;
      capture (f, FALSE);
      if (!WinPopupMenu (f->hwndFrame, hwnd, f->hwndPopupMenu,
                         md->x, md->y, first_id, options))
        send_answer (menu_serial, NULL, 0, 0);
      return (0);

    case UWM_DIALOG:
      f = (frame_data *)mp1;
      md = (menu_data *)mp2;
      /* WinCreateDlg must not be called while the mouse is captured. */
      capture (f, FALSE);
      hwndDlg = create_dialog (f, md->button, md->data, md->str);
      DosPostEventSem (hevDone);
      dlg_answer = 0;
      if (hwndDlg != NULLHANDLE)
        dlg_answer = WinProcessDlg (hwndDlg);
      send_answer (dialog_serial, NULL, 0, dlg_answer);
      return (0);

    case UWM_FILEDIALOG:
      /* WinCreateDlg must not be called while the mouse is captured.
         Perhaps that's also true for WinFileDlg.  */

      f = WinQueryWindowPtr (hwnd, 0);
      capture (f, FALSE);

      file_dialog ((frame_data *)mp1, (pm_filedialog *)mp2);
      return (0);

    case UWM_MENUBAR:
      f = (frame_data *)mp1;
      mbd = (menubar_data *)mp2;
      create_menubar (f, mbd->data, mbd->str, mbd->el_count, mbd->str_bytes);
      if (mbd->data != NULL)
        DosPostEventSem (hevDone);
      return (0);

    case UWM_CREATE_SCROLLBAR:
      f = (frame_data *)mp1;
      create_scrollbar (f, (pm_create_scrollbar *)mp2);
      DosPostEventSem (hevDone);
      return (0);

    case UWM_DESTROY_SCROLLBAR:
      destroy_scrollbar (SHORT1FROMMP (mp2));
      return (0);

    case UWM_INITIAL_SHOW:
      f = (frame_data *)mp1;
      initial_show_frame (f);
      return (0);
    }
  return (WinDefWindowProc (hwnd, msg, mp1, mp2));
}


/* This window procedure is used for subclassing frame windows.  We
   subclass frame windows to set the grid for resizing the window to
   the character box.  */

static MRESULT FrameWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HWND hwndClient;
  frame_data *f;
  MRESULT mr;
  PTRACKINFO pti;
  PSWP pswp;
  pm_event pme;

  switch (msg)
    {
    case WM_QUERYTRACKINFO:

      /* Fill-in TRACKINFO structure for resizing or moving a window.
         First call the normal frame window procedure to set up the
         structure. */

      mr = old_frame_proc (hwnd, msg, mp1, mp2);
      pti = (PTRACKINFO)mp2;

      /* If we're resizing the window, set the grid. */

      if (((pti->fs & TF_MOVE) != TF_MOVE)
          && ((pti->fs & TF_MOVE) || (pti->fs & TF_SETPOINTERPOS)))
        {
          hwndClient = WinWindowFromID (hwnd, FID_CLIENT);
          f = WinQueryWindowPtr (hwndClient, 0);
          if (f->cxChar != 0 && f->cyChar != 0)
            {
              pti->fs |= TF_GRID;
              pti->cxGrid = pti->cxKeyboard = f->cxChar;
              pti->cyGrid = pti->cyKeyboard = f->cyChar;
            }
        }
      return (mr);

    case WM_MINMAXFRAME:

      /* The frame window is being minimized or maximized.  We set a
         flag to suppress sending a resize event to Emacs when
         minimizing the window. */

      hwndClient = WinWindowFromID (hwnd, FID_CLIENT);
      f = WinQueryWindowPtr (hwndClient, 0);
      pswp = (PSWP)mp1;
      if (!f->minimized && (pswp->fl & SWP_MINIMIZE))
        {
          f->minimizing = f->minimized = TRUE;
          pme.header.type = PME_MINIMIZE;
          pme.header.frame = f->id;
          send_event (&pme);
        }
      else if (f->minimized && (pswp->fl & (SWP_RESTORE | SWP_MAXIMIZE)))
        {
          f->minimized = FALSE;
          if (pswp->fl & SWP_RESTORE)
            f->restoring = TRUE;
          pme.header.type = PME_RESTORE;
          pme.header.frame = f->id;
          send_event (&pme);
        }
      break;
    }
  return (old_frame_proc (hwnd, msg, mp1, mp2));
}


/* This is the window procedure for the object window of the PM
   thread.  It is used for creating and destroying windows as windows
   belong to the thread by which they have been created.  If there
   were no object window, we had no window of the PM thread to which
   the messages could be sent. */

static MRESULT ObjectWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  ULONG flFrameFlags;
  frame_data *f;
  PFNWP fun;

  switch (msg)
    {
    case UWM_CREATE:
      f = (frame_data *)mp1;

      /* Dirty trick: WinCreateStdWindow() doesn't accept a pCtlData
         parameter.  As using WinCreateWindow() would be too complicated
         for a frame window, the pointer is passed in a global variable.
         As this code cannot be reentered, this is safe. */

      new_frame = f;
      flFrameFlags = (FCF_TITLEBAR      | FCF_SYSMENU | FCF_ICON |
                      FCF_SIZEBORDER    | FCF_MINMAX  |
                      FCF_SHELLPOSITION | FCF_TASKLIST);
      f->hwndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
                                         &flFrameFlags, szClientClass,
                                         NULL, 0L, 0, 1, NULL);
      fun = WinSubclassWindow (f->hwndFrame, FrameWndProc);
      if (old_frame_proc != NULL && old_frame_proc != fun)
        abort ();
      old_frame_proc = fun;
      return (0);

    case UWM_DESTROY:
      f = (frame_data *)mp1;
      WinDestroyWindow (f->hwndFrame);
      return (0);
    }
  return (0);
}


/* Receive SIZE bytes from Emacs proper.  The data is stored to
   DST. */

static void receive (void *dst, ULONG size)
{
  char *d;
  ULONG n;

  d = dst;
  while (size != 0)
    {
      if (DosRead (inbound_pipe, d, size, &n) != 0 || n == 0)
        pm_error ("Cannot read from pipe");
      size -= n;
      d += n;
    }
}


/* Return a pointer to the frame data for the frame with identity FID.
   If no such frame exists, NULL is returned, causing a protection
   violation later (this must not happen). */

static frame_data *find_frame (ULONG fid)
{
  frame_data *f;

  for (f = hash_table[fid % HASH_SIZE]; f != NULL; f = f->next)
    if (f->id == fid)
      return (f);
  return (NULL);
}


/* Create a new frame.  This function is called in the thread which
   reads the pipe.  Therefore, we must send a message to the PM thread
   to actually create the PM window. */

static void create_frame (pmr_create *pc)
{
  frame_data *f;
  ULONG fid, hash;

  fid = pc->header.frame;
  hash = fid % HASH_SIZE;
  f = allocate (sizeof (*f));
  f->next = hash_table[hash];
  hash_table[hash] = f;
  f->id = fid;
  f->width = pc->width;
  f->height = pc->height;
  f->sb_width = 0;
  f->background_color = RGB_WHITE;
  f->cursor_x = 0; f->cursor_y = 0;
  f->cursor_on = FALSE;
  f->cursor_type = CURSORTYPE_BOX;
  f->cursor_width = 0;
  f->cursor_blink = TRUE;
  f->ignore_button_up = 0;
  f->stage = STAGE_CREATING;
  f->initial_visibility = 0;
  f->initial_left_base = 0;
  f->initial_top_base = 0;
  f->minimized = FALSE;
  f->minimizing = FALSE;
  f->restoring = FALSE;
  f->ignore_wm_size = FALSE;
  f->deadkey = 0;
  f->shortcuts = 0;
  f->popup_active = FALSE;
  f->menubar_id_map = NULL;
  f->menubar_id_allocated = 0;
  f->menubar_id_used = 0;
  f->cur_menubar_el = NULL;
  f->cur_menubar_el_allocated = 0;
  f->cur_menubar_str = NULL;
  f->cur_menubar_str_allocated = 0;
  f->initial_menubar_el_count = 0;
  f->initial_menubar_str_bytes = 0;
  f->buttons[0] = 1;
  f->buttons[1] = 3;
  f->buttons[2] = 2;
  f->menu_font[0] = 0;          /* Use the default font */
  memset (f->font_defined, 0, sizeof (f->font_defined));

  /* We must not create the cursor until cxChar and cyChar are known.
     Therefore we set both to zero before creating the window to be
     able to detect that case. */

  f->cxChar = 0; f->cyChar = 0;

  f->var_width_fonts = FALSE;
  parse_font_spec (&f->font, DEFAULT_FONT);
  f->alt_modifier = meta_modifier;
  f->altgr_modifier = alt_modifier;
  WinSendMsg (hwndObject, UWM_CREATE, MPFROMP (f), 0);
}


/* Destroy a frame.  A window can be destroyed only by the thread
   which created it.  Therefore we send a message to the PM thread to
   actually destroy the window. */

static void destroy_frame (ULONG fid)
{
  frame_data *f, **fp;

  for (fp = &hash_table[fid % HASH_SIZE]; *fp != NULL; fp = &(*fp)->next)
    {
      f = *fp;
      if (f->id == fid)
        {
          *fp = f->next;
          WinSendMsg (hwndObject, UWM_DESTROY, MPFROMP (f), 0);
          deallocate (f->menubar_id_map);
          deallocate (f->cur_menubar_el);
          deallocate (f->cur_menubar_str);
          deallocate (f);
          return;
        }
    }
}


static void set_pos (frame_data *f, int left, int left_base,
                     int top, int top_base)
{
  SWP swp;
  RECTL rcl;
  POINTL ptl;

  if (f->stage < STAGE_READY)
    {
      if (left_base != 0)
        {
          f->initial_left = left; f->initial_left_base = left_base;
        }
      if (top_base != 0)
        {
          f->initial_top = top; f->initial_top_base = top_base;
        }
      return;
    }
  WinQueryWindowPos (f->hwndClient, &swp);
  ptl.x = swp.x;                /* Lower left-hand corner */
  ptl.y = swp.y;
  WinMapWindowPoints (f->hwndFrame, HWND_DESKTOP, &ptl, 1);
  swp.x = ptl.x;
  swp.y = ptl.y;
  if (left_base > 0)
    swp.x = left;
  else if (left_base < 0)
    swp.x = swpScreen.cx - swp.cx + left;
  if (top_base > 0)
    swp.y = swpScreen.cy - 1 - (top + swp.cy);
  else if (top_base < 0)
    swp.y = -top;
  rcl.xLeft = swp.x; rcl.xRight = swp.x + swp.cx;
  rcl.yBottom = swp.y; rcl.yTop = swp.y + swp.cy;
  WinCalcFrameRect (f->hwndFrame, &rcl, FALSE);
  swp.x = rcl.xLeft;
  swp.y = rcl.yBottom;
  WinSetWindowPos (f->hwndFrame, HWND_TOP, swp.x, swp.y, 0, 0, SWP_MOVE);
}


#define CONV_BUTTON(c) ((c) >= '1' && (c) <= '3' ? (c) - '0' : 0)

/* Modify parameters of an existing frame. */

static void modify_frame (frame_data *f)
{
  pm_modify more;
  int repaint;

  receive (&more, sizeof (more));
  repaint = FALSE;
  if (more.var_width_fonts != 0)
    f->var_width_fonts = (more.var_width_fonts == PMR_TRUE);
  if (more.background_color != COLOR_NONE)
    {
      f->background_color = more.background_color;
      repaint = TRUE;
    }
  if (more.default_font[0] != 0)
    {
      parse_font_spec (&f->font, more.default_font);
      set_default_font (f);
      set_cursor (f);
      if (more.width == 0 && more.height == 0)
        set_size (f);
    }
  if (more.menu_font_set)
    {
      strcpy (f->menu_font, more.menu_font);
      update_menu_font (f);
    }
  if (more.width != 0 || more.height != 0 || more.sb_width != -1)
    {
      if (more.width != 0) f->width = more.width;
      if (more.height != 0) f->height = more.height;
      if (more.sb_width != -1) f->sb_width = more.sb_width;
      set_size (f);
    }
  if (more.left_base != 0 || more.top_base != 0)
    set_pos (f, more.left, more.left_base, more.top, more.top_base);
  if (more.cursor_type != 0)
    {
      f->cursor_type = more.cursor_type;
      f->cursor_width = more.cursor_width;
    }
  if (more.cursor_blink != 0)
    f->cursor_blink = (more.cursor_blink == PMR_TRUE);
  if (more.cursor_type != 0 || more.cursor_blink != 0)
    set_cursor (f);
  if (more.alt_modifier != 0)
    f->alt_modifier = more.alt_modifier;
  if (more.altgr_modifier != 0)
    f->altgr_modifier = more.altgr_modifier;
  if (more.shortcuts != 0)
    f->shortcuts = more.shortcuts & ~SHORTCUT_SET;
  if (more.buttons[0] != 0)
    {
      /* Note that the middle button is button 3 (not 2) under OS/2. */
      f->buttons[0] = CONV_BUTTON (more.buttons[0]);
      f->buttons[1] = CONV_BUTTON (more.buttons[2]);
      f->buttons[2] = CONV_BUTTON (more.buttons[1]);
    }
  if (repaint)
    WinInvalidateRect (f->hwndClient, NULL, FALSE);
}


/* Get the current mouse position and send an appropriate answer to
   Emacs. */

static void get_mousepos (int serial)
{
  HWND hwnd;
  POINTL ptl;
  frame_data *f;
  int i;
  pmd_mousepos result;

  result.frame = 0; result.x = 0; result.y = 0;
  WinQueryPointerPos (HWND_DESKTOP, &ptl);
  hwnd = WinWindowFromPoint (HWND_DESKTOP, &ptl, TRUE);
  if (hwnd != NULLHANDLE)
    {
      for (i = 0; i < HASH_SIZE; ++i)
        for (f = hash_table[i]; f != NULL; f = f->next)
          if (hwnd == f->hwndClient)
            {
              WinMapWindowPoints (HWND_DESKTOP, f->hwndClient, &ptl, 1);
              result.frame = f->id;
              result.x = charbox_x (f, ptl.x);
              result.y = charbox_y (f, ptl.y);
              if (result.x < 0) result.x = 0;
              if (result.y < 0) result.y = 0;
              break;
            }
    }
  send_answer (serial, &result, sizeof (result), 0);
}


/* Get the position of the frame window and send an appropriate answer
   to Emacs. */

static void get_framepos (frame_data *f, int serial)
{
  SWP swp;
  POINTL ptl;
  pmd_framepos result;

  WinQueryWindowPos (f->hwndClient, &swp);
  ptl.x = swp.x; ptl.y = swp.y;
  WinMapWindowPoints (f->hwndFrame, HWND_DESKTOP, &ptl, 1);
  result.left = ptl.x;
  result.top = swpScreen.cy - 1 - (ptl.y + swp.cy);
  result.pix_width = f->width;  /* TODO */
  result.pix_height = f->height; /* TODO */
  send_answer (serial, &result, sizeof (result), 0);
}


/* Get the width or height of the screen in pixels and in
   millimeters. */

static void get_dimen (HDC hdc, ULONG caps_dimen, ULONG caps_res,
                       int *ppix, int *pmm)
{
  LONG dimen, res;

  /* Retrieve the width or height in pixels. */

  if (!DevQueryCaps (hdc, caps_dimen, 1, &dimen))
    {
      *ppix = 1; *pmm = 1;
    }
  else
    {
      *ppix = dimen;

      /* Retrieve and the resolution in pixels per meter. */

      if (!DevQueryCaps (hdc, caps_res, 1, &res)
          || res == 0)
        *pmm = 1;
      else
        *pmm = (1000 * dimen) / res;
    }
}


/* Get the number of planes etc. and send an appropriate answer to
   Emacs. */

static void get_config (int serial)
{
  HPS hps;
  HDC hdc;
  LONG colors;
  pmd_config result;

  hps = WinGetScreenPS (HWND_DESKTOP);
  hdc = GpiQueryDevice (hps);
  DevQueryCaps (hdc, CAPS_PHYS_COLORS, 1, &colors);
  result.planes = ffs (colors) - 1;
  DevQueryCaps (hdc, CAPS_COLORS, 1, &colors);
  result.color_cells = (int)colors;
  get_dimen (hdc, CAPS_WIDTH, CAPS_HORIZONTAL_RESOLUTION,
             &result.width, &result.width_mm);
  get_dimen (hdc, CAPS_HEIGHT, CAPS_VERTICAL_RESOLUTION,
             &result.height, &result.height_mm);
  send_answer (serial, &result, sizeof (result), 0);
}


/* Retrieve a text from the clipboard and send it to Emacs.  The text
   is in CR/LF format, without trailing null character.  The byte
   count as 32-bit number precedes the text.  If GET_TEXT is zero, the
   text isn't sent. */

static void get_clipboard (int serial, int get_text)
{
  ULONG size, osize, flag, ulData;
  PCH str, end;
  int opened;

  str = NULL; size = 0;
  opened = WinOpenClipbrd (hab);
  if (opened)
    {
      ulData = WinQueryClipbrdData (hab, CF_TEXT);
      /* Use DosQueryMem to cope with a missing null character. */
      osize = 0x7fffffff; flag = 0;
      if (ulData != 0 && DosQueryMem ((PVOID)ulData, &osize, &flag) == 0
          && (end = memchr ((PCH)ulData, 0, osize)) != NULL)
        {
          str = (PCH)ulData;
          size = end - str;
        }
    }
  if (get_text)
    {
      if (str != NULL)
        send_answer (serial, str, size, 0);
      else
        send_answer (serial, "", 0, 0);
    }
  else
    send_answer (serial, NULL, 0, size);
  if (opened)
    WinCloseClipbrd (hab);
}


/* Receive a buffer from Emacs and put the text into the clipboard.
   SIZE is the size of the text, in bytes.  The buffer is in CR/LF
   format, without trailing null character. */

static void put_clipboard (unsigned long size)
{
  char *buf;

  DosAllocSharedMem ((PVOID)&buf, NULL, size + 1,
                     PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GIVEABLE);
  receive (buf, size);
  buf[size] = 0;
  if (WinOpenClipbrd (hab))
    {
      WinEmptyClipbrd (hab);
      WinSetClipbrdData (hab, (ULONG)buf, CF_TEXT, CFI_POINTER);
      WinCloseClipbrd (hab);
    }
}


/* Define a face. */

static void define_face (frame_data *f, pmr_face *pf, const char *name)
{
  face_data *face;
  font_spec spec;
  int i, gc;

  /* Use the default font of the frame if the font name cannot be
     parsed (or if it is empty). */

  if (!parse_font_spec (&spec, name))
      spec = f->font;

  /* Add the underline attribute to the font selection if the face is
     underlined. */

  if (pf->underline)
    spec.sel |= FATTR_SEL_UNDERSCORE;

  /* Search the face table for the face.  Set GC to the face number if
     found.  Set GC to 0 otherwise. */

  gc = 0;
  for (i = 1; i <= nfaces; ++i)
    if (face_vector[i].foreground == pf->foreground
        && face_vector[i].background == pf->background
        && equal_font_spec (&face_vector[i].spec, &spec))
      {
        gc = i;
        break;
      }

  /* Add the face if it is not in the face table. */

  if (gc == 0)
    {
      if (nfaces + 1 >= nfaces_allocated)
        {
          face_data *np;

          nfaces_allocated += 16;
          np = allocate (nfaces_allocated * sizeof (face_data));
          if (nfaces != 0)
            memcpy (np + 1, face_vector + 1, nfaces * sizeof (face_data));
          deallocate (face_vector);
          face_vector = np;
        }
      gc = ++nfaces;
      face = &face_vector[nfaces];
      face->spec = spec;
      face->foreground = pf->foreground;
      face->background = pf->background;
      face->font_id = 0;
#ifdef USE_PALETTE_MANAGER
      add_color (face->foreground);
      add_color (face->background);
      make_palette (f);
#endif
    }

  /* Send the face number to Emacs (which will use it as X graphics
     context). */

  send_answer (pf->serial, NULL, 0, gc);
}


/* Hide all windows and post a WM_QUIT message to the message queue of
   the PM thread.  We hide all windows with one call to avoid
   excessive repainting. */

static void terminate (void)
{
  frame_data *f;
  PSWP pswp;
  int i, j, n;

  terminating = TRUE;
  n = 0;
  for (i = 0; i < HASH_SIZE; ++i)
    for (f = hash_table[i]; f != NULL; f = f->next)
      ++n;
  if (n != 0)
    {
      pswp = allocate (n * sizeof (SWP));
      j = 0;
      for (i = 0; i < HASH_SIZE; ++i)
        for (f = hash_table[i]; f != NULL; f = f->next)
          if (j < n)
            {
              pswp[j].hwnd = f->hwndFrame;
              pswp[j].fl = SWP_HIDE;
              ++j;
            }
      WinSetMultWindowPos (hab, pswp, j);
      deallocate (pswp);
    }
  WinPostMsg (hwndObject, WM_QUIT, 0, 0);

  /* Sit for about 49 days to avoid reading from the pipe, which
     probably no longer exists now.  That is no longer true, as Emacs
     now waits until pmemacs.exe is dead, before closing the pipe.  */

  DosSleep ((ULONG)-1);
}


/* Build a font name in "Host Portable Character Representation".
   Here's an X11R4 example:
   -misc-fixed-bold-r-normal--13-100-100-100-c-80-iso8859-1 */

static int make_x_font_spec (unsigned char *dst, PFONTMETRICS pfm,
                             int bold, int italic, int size)
{
  return sprintf (dst, "-os2-%s-%s-%s-normal--%d-%d-%d-%d-m-1-cp850",
                  pfm->szFacename,
                  bold ? "bold" : "medium",
                  italic ? "i" : "r",
                  (int)pfm->lMaxCharInc,
                  size,
                  pfm->sXDeviceRes,
                  pfm->sYDeviceRes);
}


/* Build a font name in OS/2 format (size.name).  We extend the syntax
   to size[.bold][.italic]name. */

static int make_pm_font_spec (unsigned char *dst, PFONTMETRICS pfm,
                              int bold, int italic, int size)
{
  int i;

  i = sprintf (dst, "%d", (size + 5) / 10);
  if (bold)
    i += sprintf (dst+i, ".bold");
  if (italic)
    i += sprintf (dst+i, ".italic");
  i += sprintf (dst+i, ".%s", pfm->szFacename);
  return (i);
}


/* Pattern matching for listfont().  This should ignore letter case
   but doesn't now. */

static int font_match (const UCHAR *pattern, const UCHAR *name)
{
  for (;;)
    switch (*pattern)
      {
      case 0:
        return (*name == 0);
      case '?':
        ++pattern;
        if (*name == 0)
          return 0;
        ++name;
        break;
      case '*':
        while (*pattern == '*')
          ++pattern;
        if (*pattern == 0)
          return 1;
        while (*name != 0)
          {
            if (font_match (pattern, name))
              return 1;
            ++name;
          }
        return 0;
      default:
        if (*pattern != *name)
          return 0;
        ++pattern; ++name;
        break;
      }
}


/* Buffer for pm-list-fonts. */

static UCHAR *fontlist_buffer = NULL;
static ULONG fontlist_size = 0;
static ULONG fontlist_used = 0;

#define LISTFONT_PM     0x01    /* List PM variant only */
#define LISTFONT_X      0x02    /* List X variant only */
#define LISTFONT_NAME   0x04    /* Name is known */
#define LISTFONT_SIZE   0x08    /* Size is known */


static void fontlist_grow (ULONG incr)
{
  UCHAR *p;

  while (fontlist_used + incr > fontlist_size)
    {
      fontlist_size += 0x10000;
      p = allocate (fontlist_size);
      memcpy (p, fontlist_buffer, fontlist_used);
      deallocate (fontlist_buffer);
      fontlist_buffer = p;
    }
}


/* List one font for fontlist().  Reallocate the buffer if it's too
   small. */

static void list_one_font (pmd_fontlist *answer, PFONTMETRICS pfm,
                           int bold, int italic, int size,
                           const UCHAR *pattern, int flags)
{
  ULONG len;
  UCHAR *p;

  fontlist_grow (256);
  p = fontlist_buffer + fontlist_used;
  if (flags & LISTFONT_X)
    {
      len = make_x_font_spec (p + 1, pfm, bold, italic, size);
      if (font_match (pattern, p + 1))
        {
          *p = (UCHAR)len;
          fontlist_used += 1 + len;
          ++answer->count;
          p += 1 + len;
        }
    }
  if (flags & LISTFONT_PM)
    {
      len = make_pm_font_spec (p + 1, pfm, bold, italic, size);
      if (font_match (pattern, p + 1))
        {
          *p = (UCHAR)len;
          fontlist_used += 1 + len;
          ++answer->count;
        }
    }
}


/* Send a list of fonts to Emacs, for pm_list_fonts.  Try to optimize
   for speed by getting the name and size from the pattern.  If the
   size is not known, restrict the range of sizes for outline fonts to
   1 through 48pt, intersected with the range of sizes the font is
   designed for.  If the size is known, sizes greater than 48pt can be
   used. */

static void fontlist (frame_data *f, int serial, const UCHAR *pattern)
{
  pmd_fontlist answer;
  ULONG fonts, temp, i;
  PFONTMETRICS pfm;
  int bold, italic, size, flags;
  font_spec spec;

  temp = 0;
  fonts = GpiQueryFonts (f->hpsClient, QF_PUBLIC | QF_PRIVATE, NULL,
                         &temp, sizeof (FONTMETRICS), NULL);
  pfm = allocate (fonts * sizeof (FONTMETRICS));
  GpiQueryFonts (f->hpsClient, QF_PUBLIC | QF_PRIVATE, NULL,
                 &fonts, sizeof (FONTMETRICS), pfm);
  answer.count = 0;
  fontlist_used = 0;
  fontlist_grow (sizeof (answer));
  fontlist_used += sizeof (answer);
  flags = LISTFONT_PM | LISTFONT_X;
  if (pattern[0] == '-')
    flags &= ~LISTFONT_PM;
  if (pattern[0] >= '0' && pattern[0] <= '9')
    flags &= ~LISTFONT_X;
  if (parse_font_spec (&spec, pattern))
    {
      flags |= LISTFONT_SIZE;
      if (strpbrk (spec.name, "?*") == NULL)
        flags |= LISTFONT_NAME;
    }
  for (i = 0; i < fonts; ++i)
    if (f->var_width_fonts || (pfm[i].fsType & FM_TYPE_FIXED))
      if (!(flags & LISTFONT_NAME)
          || strcmp (pfm[i].szFacename, spec.name) == 0)
        for (bold = 0; bold <= 1; ++bold)
          for (italic = 0; italic <= 1; ++italic)
            if (pfm[i].fsDefn & FM_DEFN_OUTLINE)
              {
                if (flags & LISTFONT_SIZE)
                  {
                    if (spec.size >= pfm[i].sMinimumPointSize
                        && spec.size <= pfm[i].sMaximumPointSize)
                      list_one_font (&answer, pfm + i, bold, italic,
                                     spec.size, pattern, flags);
                  }
                else
                  for (size = 10 * ((pfm[i].sMinimumPointSize + 9) / 10);
                       size <= 480
                       && size <= pfm[i].sMaximumPointSize; size += 10)
                    list_one_font (&answer, pfm + i, bold, italic,
                                   size, pattern, flags);
              }
            else
              list_one_font (&answer, pfm + i, bold, italic,
                             pfm[i].sNominalPointSize, pattern, flags);
  memcpy (fontlist_buffer, &answer, sizeof (answer));
  send_answer (serial, fontlist_buffer, fontlist_used, 0);
  deallocate (pfm);
}


#define CURSOR_OFF(f) (void)((f)->cursor_on \
                             && WinShowCursor ((f)->hwndClient, FALSE))

#define CURSOR_BACK(f) (void)((f)->cursor_on \
                              && WinShowCursor ((f)->hwndClient, TRUE))



#define GROW_BUF(SIZE) do \
  if ((SIZE) > buf_size) \
    { \
      deallocate (buf); \
      buf_size = ((SIZE) + 0xfff) & ~0xfff; \
      buf = allocate (buf_size); \
    } \
  while (0)

/* Read the pipe.  This function is run in a secondary thread.  Here,
   we receive requests from Emacs. */

static void pipe_thread (ULONG arg)
{
  HMQ hmq;
  POINTL ptl, aptl[3];
  RECTL rcl, rcl2;
  SIZEF cbox;
  COLOR foreground, background;
  pm_request pmr;
  menu_data md;
  menubar_data mbd;
  frame_data *f;
  char *buf;
  ULONG buf_size, post_count;
  LONG cx;
  int font_id;

  hmq = WinCreateMsgQueue (hab, 50);
  buf_size = 512;
  buf = allocate (buf_size);
  for (;;)
    {
      receive (&pmr, sizeof (pmr));
#if 0
      sprintf (buf, "PMR %d", pmr.header.type);
      WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, buf, "debug", 0,
                     MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
#endif
      switch (pmr.header.type)
        {
        case PMR_CREATE:
          create_frame (&pmr.create);
          break;

        case PMR_CREATEDONE:
          f = find_frame (pmr.header.frame);
          if (f->initial_visibility > 0)
            initial_show_frame (f);
          else
            f->stage = STAGE_INVISIBLE;
          break;

        case PMR_DESTROY:
          destroy_frame (pmr.header.frame);
          break;

        case PMR_MODIFY:
          f = find_frame (pmr.header.frame);
          modify_frame (f);
          break;

        case PMR_GLYPHS:
          receive (buf, pmr.glyphs.count);
          f = find_frame (pmr.glyphs.header.frame);
          CURSOR_OFF (f);
          ptl.x = make_x (f, pmr.glyphs.x);
          ptl.y = make_y (f, pmr.glyphs.y);

          /* Note: pmr.glyphs.count must be <= 512 */

          /* Get the font ID for the face.  If no font has been
             created for that face, the font ID is zero.
             font_defined[0] is always zero, therefore make_font()
             will be called. */
          font_id = face_vector[pmr.glyphs.face].font_id;
          if (!f->font_defined[font_id])
            font_id = make_font (f, &face_vector[pmr.glyphs.face]);
          if (font_id != f->cur_charset)
            {
              GpiSetCharSet (f->hpsClient, font_id);
              f->cur_charset = font_id;
            }
          foreground = face_vector[pmr.glyphs.face].foreground;
          if (foreground != f->cur_color)
            {
              GpiSetColor (f->hpsClient, GET_COLOR (f->hpsClient, foreground));
              f->cur_color = foreground;
            }
          background = face_vector[pmr.glyphs.face].background;
          if (font_vector[font_id].use_incr)
            {
              if (f->cur_backmix != BM_LEAVEALONE)
                {
                  GpiSetBackMix (f->hpsClient, BM_LEAVEALONE);
                  f->cur_backmix = BM_LEAVEALONE;
                }
              if (font_vector[font_id].outline
                  && font_vector[font_id].cbox_size != f->cur_cbox_size)
                {
                  cbox.cx = cbox.cy = font_vector[font_id].cbox_size;
                  GpiSetCharBox (f->hpsClient, &cbox);
                  f->cur_cbox_size = font_vector[font_id].cbox_size;
                }
              rcl.xLeft = ptl.x;
              rcl.yBottom = ptl.y - f->cyDesc;
              rcl.xRight = rcl.xLeft + pmr.glyphs.count * f->cxChar;
              rcl.yTop = rcl.yBottom + f->cyChar;
              WinFillRect (f->hpsClient, &rcl,
                           GET_COLOR (f->hpsClient, background));
              --rcl.xRight; --rcl.yTop;
              GpiCharStringPosAt (f->hpsClient, &ptl, &rcl,
                                  CHS_VECTOR | CHS_CLIP,
                                  pmr.glyphs.count, buf, f->increments);
            }
          else
            {
              if (f->cur_backmix != BM_OVERPAINT)
                {
                  GpiSetBackMix (f->hpsClient, BM_OVERPAINT);
                  f->cur_backmix = BM_OVERPAINT;
                }
              if (background != f->cur_backcolor)
                {
                  GpiSetBackColor (f->hpsClient,
                                   GET_COLOR (f->hpsClient, background));
                  f->cur_backcolor = background;
                }
              GpiCharStringAt (f->hpsClient, &ptl, pmr.glyphs.count, buf);
            }
          CURSOR_BACK (f);
          break;

        case PMR_CLEAR:
          f = find_frame (pmr.glyphs.header.frame);
          CURSOR_OFF (f);
          rcl.xLeft = 0; rcl.xRight = f->cxClient;
          rcl.yBottom = 0; rcl.yTop = f->cyClient;
          WinFillRect (f->hpsClient, &rcl,
                       GET_COLOR (f->hpsClient, f->background_color));
          CURSOR_BACK (f);
          break;

        case PMR_CLREOL:
          f = find_frame (pmr.glyphs.header.frame);
          CURSOR_OFF (f);
          rcl.xLeft = make_x (f, pmr.clreol.x0);
          rcl.xRight = make_x (f, pmr.clreol.x1);
          rcl.yBottom = make_y (f, pmr.clreol.y) - f->cyDesc;
          rcl.yTop = rcl.yBottom + f->cyChar;
          WinFillRect (f->hpsClient, &rcl,
                       GET_COLOR (f->hpsClient, f->background_color));
          CURSOR_BACK (f);
          break;

        case PMR_LINES:
          f = find_frame (pmr.glyphs.header.frame);
          CURSOR_OFF (f);
          cx = f->cxClient - f->sb_width * f->cxChar;
          aptl[0].x = 0;
          aptl[1].x = cx;
          aptl[2].x = 0;
          rcl.xLeft = 0;
          rcl.xRight = cx;
          if (pmr.lines.count > 0)
            {
              aptl[0].y = make_y (f, pmr.lines.max_y);
              aptl[1].y = make_y (f, pmr.lines.y + pmr.lines.count);
              aptl[2].y = make_y (f, pmr.lines.max_y - pmr.lines.count);
              rcl.yBottom = make_y (f, pmr.lines.y + pmr.lines.count);
              rcl.yTop = make_y (f, pmr.lines.y);
            }
          else
            {
              aptl[0].y = make_y (f, pmr.lines.max_y + pmr.lines.count);
              aptl[1].y = make_y (f, pmr.lines.y);
              aptl[2].y = make_y (f, pmr.lines.max_y);
              rcl.yBottom = make_y (f, pmr.lines.max_y);
              rcl.yTop = make_y (f, pmr.lines.max_y + pmr.lines.count);
            }
          aptl[0].y += f->cyChar - f->cyDesc;
          aptl[1].y += f->cyChar - f->cyDesc;
          aptl[2].y += f->cyChar - f->cyDesc;
          rcl.yBottom += f->cyChar - f->cyDesc;
          rcl.yTop += f->cyChar - f->cyDesc;

          /* Use GpiBitblt if the source rectangle is completely
             visible.  Otherwise, repaint the target rectangle. */

          rcl2.xLeft = 0;
          rcl2.xRight = cx - 1;
          rcl2.yBottom = aptl[2].y;
          rcl2.yTop = aptl[2].y + aptl[1].y - aptl[0].y - 1;
          if (GpiRectVisible (f->hpsClient, &rcl2) == RVIS_VISIBLE)
            GpiBitBlt (f->hpsClient, f->hpsClient, 3, aptl, ROP_SRCCOPY,
                       BBO_IGNORE);
          else
            {
              rcl2.xLeft = 0; rcl2.xRight = cx;
              rcl2.yBottom = aptl[0].y; rcl2.yTop = aptl[1].y;
              WinInvalidateRect (f->hwndClient, &rcl2, FALSE);
            }

          /* Clear the lines uncovered. */

          WinFillRect (f->hpsClient, &rcl,
                       GET_COLOR (f->hpsClient, f->background_color));
          CURSOR_BACK (f);
          break;

        case PMR_CURSOR:

          /* Turn on or off cursor; change cursor position. */

          f = find_frame (pmr.cursor.header.frame);
          f->cursor_x = make_x (f, pmr.cursor.x);
          f->cursor_y = make_y (f, pmr.cursor.y) - f->cyDesc;
          f->cursor_on = pmr.cursor.on;

          if (has_focus (f))
            {
              if (f->cursor_on)
                create_cursor (f);
              else
                WinDestroyCursor (f->hwndClient);
            }
          break;

        case PMR_BELL:

          /* Sound the bell.  We don't want to wait until DosBeep()
             completes, therefore we delegate work to another thread.
             A visible bell is handled directly to avoid problems with
             multiple threads painting at the same time. */

          f = find_frame (pmr.bell.header.frame);
          if (pmr.bell.visible)
            {
              rcl.xLeft = f->cxClient / 4;
              rcl.xRight = rcl.xLeft + f->cxClient / 2;
              rcl.yBottom = f->cyClient / 4;
              rcl.yTop = rcl.yBottom + f->cyClient / 2;
              WinInvertRect (f->hpsClient, &rcl);
              DosSleep (150);
              WinInvertRect (f->hpsClient, &rcl);
            }
          else
            DosPostEventSem (hevBeep);
          break;

        case PMR_NAME:
          f = find_frame (pmr.name.header.frame);
          receive (buf, pmr.name.count);
          buf[pmr.name.count] = 0;
          WinSetWindowText (f->hwndFrame, buf);
          break;

        case PMR_VISIBLE:
          f = find_frame (pmr.visible.header.frame);
          if (f->stage == STAGE_CREATING)
            f->initial_visibility = pmr.visible.visible ? 1 : 0;
          else if (f->stage == STAGE_INVISIBLE)
            {
              f->initial_visibility = pmr.visible.visible ? 1 : 0;
              if (f->initial_visibility > 0)
                initial_show_frame (f);
            }
          else if (pmr.visible.visible)
            WinSetWindowPos (f->hwndFrame, NULLHANDLE, 0, 0, 0, 0,
                             SWP_RESTORE | SWP_SHOW);
          else
            WinShowWindow (f->hwndFrame, FALSE);
          break;

        case PMR_FOCUS:
          f = find_frame (pmr.header.frame);
          WinSetActiveWindow (HWND_DESKTOP, f->hwndFrame);
#if 0
          WinSetWindowPos (f->hwndFrame, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
#endif
          break;

        case PMR_ICONIFY:
          f = find_frame (pmr.header.frame);
          if (f->stage < STAGE_READY)
            f->initial_visibility = -1;
          else
            WinSetWindowPos (f->hwndFrame, NULLHANDLE, 0, 0, 0, 0,
                             SWP_MINIMIZE);
          break;

        case PMR_SIZE:
          f = find_frame (pmr.size.header.frame);
          f->width = pmr.size.width;
          f->height = pmr.size.height;
          set_size (f);
          break;

        case PMR_POPUPMENU:
          f = find_frame (pmr.popupmenu.header.frame);
          GROW_BUF (pmr.popupmenu.size);
          receive (buf, pmr.popupmenu.size);
          menu_serial = pmr.popupmenu.serial;
          md.button = pmr.popupmenu.button;
          md.align_top = pmr.popupmenu.align_top;
          md.x = make_x (f, pmr.popupmenu.x);
          md.y = make_y (f, pmr.popupmenu.y);
          md.data = (pm_menu *)buf;
          md.str = buf + pmr.popupmenu.count * sizeof (pm_menu);
          WinSendMsg (f->hwndClient, UWM_POPUPMENU,
                      MPFROMP (f), MPFROMP (&md));
          break;

        case PMR_MENUBAR:
          f = find_frame (pmr.menubar.header.frame);
          GROW_BUF (pmr.menubar.size);
          receive (buf, pmr.menubar.size);
          mbd.data = (pm_menu *)buf;
          mbd.str = buf + pmr.menubar.entries * sizeof (pm_menu);
          mbd.el_count = pmr.menubar.entries;
          mbd.str_bytes = (pmr.menubar.size
                           - pmr.menubar.entries * sizeof (pm_menu));
          DosResetEventSem (hevDone, &post_count);
          WinSendMsg (f->hwndClient, UWM_MENUBAR, MPFROMP (f), MPFROMP (&mbd));

          /* Wait until we are done updating the menubar to avoid
             overwriting `buf' while the other thread is still
             accessing it. */

          DosWaitEventSem (hevDone, SEM_INDEFINITE_WAIT);
          break;

        case PMR_MOUSEPOS:
          get_mousepos (pmr.mousepos.serial);
          break;

        case PMR_FRAMEPOS:
          f = find_frame (pmr.framepos.header.frame);
          get_framepos (f, pmr.framepos.serial);
          break;

        case PMR_PASTE:
          get_clipboard (pmr.paste.serial, pmr.paste.get_text);
          break;

        case PMR_CUT:
          put_clipboard (pmr.cut.size);
          break;

        case PMR_QUITCHAR:
          quit_char = pmr.quitchar.quitchar;
          break;

        case PMR_CLOSE:
          terminate ();
          break;

        case PMR_RAISE:
          f = find_frame (pmr.header.frame);
          WinSetWindowPos (f->hwndFrame, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
          break;

        case PMR_LOWER:
          f = find_frame (pmr.header.frame);
          WinSetWindowPos (f->hwndFrame, HWND_BOTTOM, 0, 0, 0, 0, SWP_ZORDER);
          break;

        case PMR_FACE:
          f = find_frame (pmr.face.header.frame);
          receive (buf, pmr.face.name_length);
          buf[pmr.face.name_length] = 0;
          define_face (f, &pmr.face, buf);
          break;

        case PMR_FONTLIST:
          f = find_frame (pmr.fontlist.header.frame);
          receive (buf, pmr.fontlist.pattern_length);
          buf[pmr.fontlist.pattern_length] = 0;
          fontlist (f, pmr.fontlist.serial, buf);
          break;

        case PMR_TRACKMOUSE:
          track_mouse = pmr.track.flag;
          track_x = -1; track_y = -1;
          break;

        case PMR_SETPOS:
          f = find_frame (pmr.setpos.header.frame);
          set_pos (f, pmr.setpos.left, pmr.setpos.left_base,
                   pmr.setpos.top, pmr.setpos.top_base);
          break;

        case PMR_CONFIG:
          get_config (pmr.config.serial);
          break;

        case PMR_DIALOG:
          f = find_frame (pmr.dialog.header.frame);
          GROW_BUF (pmr.dialog.size);
          receive (buf, pmr.dialog.size);
          dialog_serial = pmr.dialog.serial;
          md.button = pmr.dialog.buttons;
          md.x = 0;
          md.y = 0;
          md.data = (pm_menu *)buf;
          md.str = buf + pmr.dialog.count * sizeof (pm_menu);
          DosResetEventSem (hevDone, &post_count);
          WinPostMsg (f->hwndClient, UWM_DIALOG,
                      MPFROMP (f), MPFROMP (&md));

          /* Wait until the dialog has been created to avoid
             overwriting `buf' while create_dialog() is still
             accessing it. */

          DosWaitEventSem (hevDone, SEM_INDEFINITE_WAIT);
          break;

        case PMR_DROP:
          get_drop (pmr.drop.cookie, pmr.drop.serial);
          break;

        case PMR_INITIALIZE:
          set_code_page (pmr.initialize.codepage, 0, FALSE);
          break;

        case PMR_CPLIST:
          cplist (pmr.cplist.serial);
          break;

        case PMR_CODEPAGE:
          set_code_page (pmr.codepage.codepage, pmr.codepage.serial, TRUE);
          break;

        case PMR_FILEDIALOG:
          f = find_frame (pmr.header.frame);
          GROW_BUF (sizeof (pm_filedialog));
          receive (buf, sizeof (pm_filedialog));

          DosResetEventSem (hevDone, &post_count);
          WinPostMsg (f->hwndClient, UWM_FILEDIALOG,
                      MPFROMP (f), MPFROMP (buf));

          /* Wait until the data has been copied to avoid overwriting
             `buf' while file_dialog() is still accessing it. */

          DosWaitEventSem (hevDone, SEM_INDEFINITE_WAIT);
          break;

	case PMR_CREATE_SCROLLBAR:
          f = find_frame (pmr.header.frame);
          GROW_BUF (sizeof (pm_create_scrollbar));
          receive (buf, sizeof (pm_create_scrollbar));

          DosResetEventSem (hevDone, &post_count);
          WinPostMsg (f->hwndClient, UWM_CREATE_SCROLLBAR,
                      MPFROMP (f), MPFROMP (buf));

          /* Wait until we are done creating the scroll bar to avoid
             overwriting `buf' while the other thread is still
             accessing it. */

          DosWaitEventSem (hevDone, SEM_INDEFINITE_WAIT);
          break;

        case PMR_UPDATE_SCROLLBAR:
          update_scrollbar (pmr.update_scrollbar.id,
                            &pmr.update_scrollbar.s);
          break;

        case PMR_MOVE_SCROLLBAR:
          f = find_frame (pmr.header.frame);
          move_scrollbar (f, pmr.move_scrollbar.id,
                          pmr.move_scrollbar.top,
                          pmr.move_scrollbar.left,
                          pmr.move_scrollbar.width,
                          pmr.move_scrollbar.height);
          break;

        case PMR_DESTROY_SCROLLBAR:
          f = find_frame (pmr.header.frame);
          WinPostMsg (f->hwndClient, UWM_DESTROY_SCROLLBAR,
                      0, MPFROMSHORT (pmr.destroy_scrollbar.id));
          break;

        default:
          pm_error ("Unknown message type");
        }
    }
}


/* Generate sound.  This function is run in a thread of its own.  As
   DosBeep() blocks until the previous DosBeep() ends, we donate a
   separate thread to calling it to avoid blocking the pipe. */

static void beep_thread (ULONG arg)
{
  ULONG count;

  for (;;)
    {
      DosWaitEventSem (hevBeep, SEM_INDEFINITE_WAIT);
      if (DosResetEventSem (hevBeep, &count) == 0)
        while (count > 0)
          {
            DosBeep (880, 50);  /* Same as WA_WARNING */
            --count;
          }
    }
}


/* Initializations. */

static void initialize (void)
{
  FONTMETRICS fm;
  int i;

#ifdef TRACE
  {
    ULONG rc, action;

    rc = DosOpen (TRACE_FNAME, &trace_handle, &action, 0, 0,
                  OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,
                  (OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYNONE
                   | OPEN_FLAGS_NOINHERIT),
                  NULL);
    if (rc != 0)
      trace_handle = TRACE_OFF;
  }
#endif

  /* Obtain the width of an icon. */

  cxIcon = WinQuerySysValue (HWND_DESKTOP, SV_CXICON);

  /* Obtain the width and height of the default font, for converting
     to and from dialog box units. */

  hpsScreen = WinGetScreenPS (HWND_DESKTOP);
  GpiQueryFontMetrics (hpsScreen, (LONG)sizeof (fm), &fm);
  default_font_width = fm.lAveCharWidth;
  default_font_height = fm.lMaxBaselineExt;

  /* Create the event semaphore for telling pipe_thread() that the
     message sent to the main thread has been processed and the memory
     buffer is no longer required.  */

  DosCreateEventSem (NULL, &hevDone, 0, FALSE);

  /* Create a Mutex semaphore for protecting drop_list.  Allocate
     drop_list.  */

  DosCreateMutexSem (NULL, &drop_mutex, 0, FALSE);
  drop_list = allocate (DROP_MAX * sizeof (*drop_list));

#ifdef USE_PALETTE_MANAGER
  {
    HDC hdc;
    LONG caps;

    /* Check for palette manager. */
    hdc = GpiQueryDevice (hpsScreen);
    if (DevQueryCaps (hdc, CAPS_ADDITIONAL_GRAPHICS, 1, &caps)
        && (caps & CAPS_PALETTE_MANAGER))
      has_palette_manager = TRUE;
    color_table[colors++] = RGB_BLACK;
    color_table[colors++] = RGB_WHITE;
  }
#endif

  /* Initialize the table of scrollbars. */

  for (i = 0; i < MAX_SCROLLBARS; ++i)
    scrollbar_table[i].hwnd = NULLHANDLE;
}


/* Terminate the program. */

static void terminate_process (void)
{
#ifdef TRACE
  if (trace_handle != TRACE_OFF)
    {
      DosClose (trace_handle);
      trace_handle = TRACE_OFF;
    }
#endif
  WinDestroyMsgQueue (hmq);
  WinTerminate (hab);
  exit (0);
}


/* Entrypoint.  On the command line, three numbers are passed.  The
   first one is the process ID of Emacs, the other numbers are the
   file handles for the input and output pipes, respectively. */

int main (int argc, char *argv[])
{
  QMSG qmsg;
  TID tid;
  int i;

  /* Initialize the Presentation Manager and create a message queue
     for this thread, the PM thread. */

  hab = WinInitialize (0);
  hmq = WinCreateMsgQueue (hab, 50);

  /* Parse the command line. */

  if (argc - 1 != 3)
    pm_error ("Do not run pmemacs.exe manually; it is started by emacs.exe.");
  emacs_pid = atoi (argv[1+0]);
  inbound_pipe = atoi (argv[1+1]);
  outbound_pipe = atoi (argv[1+2]);

  /* Initialize the (hash) table of frames. */

  for (i = 0; i < HASH_SIZE; ++i)
    hash_table[i] = NULL;

  /* Obtain the size of the screen. */

  WinQueryWindowPos (HWND_DESKTOP, &swpScreen);

  /* Register window classes. */

  WinRegisterClass (hab, szClientClass, ClientWndProc,
                    CS_SIZEREDRAW | CS_MOVENOTIFY | CS_CLIPCHILDREN, 4L);
  WinRegisterClass (hab, szFrameClass, FrameWndProc, 0, 0L);
  WinRegisterClass (hab, szObjectClass, ObjectWndProc, 0, 0L);

  /* Create the object window.  See ObjectWndProc() for details. */

  hwndObject = WinCreateWindow (HWND_OBJECT, szObjectClass, "", 0,
                                0, 0, 0, 0, NULLHANDLE, HWND_BOTTOM,
                                0, NULL, NULL);

  initialize ();

  /* Create the event semaphore for triggering the sound thread. */

  DosCreateEventSem (NULL, &hevBeep, 0, FALSE);

  /* Start the threads used for reading the pipe and for beeping. */

  DosCreateThread (&tid, beep_thread, 0, 2, 0x8000);
  DosCreateThread (&tid, pipe_thread, 0, 2, 0x8000);

  /* This is the message loop of the PM thread.  This loop ends when
     receiving a WM_QUIT message. */

  while (WinGetMsg (hab, &qmsg, 0L, 0, 0))
    WinDispatchMsg (hab, &qmsg);
  terminate_process ();
  return (0);
}
