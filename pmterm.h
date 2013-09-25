/* pmterm.h
   Copyright (C) 1993-1996 Eberhard Mattes.
   Copyright (C) 1995 Patrick Nadeau (scroll bar code)
   Copyright (C) 1989-1995 Free Software Foundation, Inc. (code from xterm.h)

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


#include "pmlib.h"

struct frame;

void pm_send (const void *src, unsigned size);
void pm_send_collect (const void *src, unsigned size);
void pm_send_flush (void);
void *pm_receive (int serial, void *dst, int *size, int may_paint);
void pm_menubar_selection (struct frame *f, int sel);

#define XDISPLAY
#define ROOT_WINDOW		((Window *)0)
#define FRAME_X_WINDOW(f)	((unsigned long)f)

struct x_output
{
  XFontStruct *font;
  int internal_border_width;
  int pixel_height;
  int pixel_width;
  int line_height;

  /* If a fontset is specified for this frame instead of font, this
     value contains an ID of the fontset, else -1.  */
  int fontset;

  unsigned background_color;
  unsigned foreground_color;

  /* Table of parameter faces for this frame.  */
  struct face **param_faces;
  int n_param_faces;

  /* Table of computed faces for this frame. */
  struct face **computed_faces;
  int n_computed_faces;		/* How many are valid */
  int size_computed_faces;	/* How many are allocated */

  /* This is the Emacs structure for the X display this frame is on.  */
  struct x_display_info *display_info;

  int left_pos;
  int top_pos;

  /* Mouse position.  Valid only if mouse_moved is non-zero.  */
  int mouse_x;
  int mouse_y;
};

/* Borrowed from xterm.h.  */

struct x_display_info
{
  /* Number of frames that are on this display.  */
  int reference_count;

  /* Mask of things that cause the mouse to be grabbed.  */
  int grabbed;

  /* These variables describe the range of text currently shown
     in its mouse-face, together with the window they apply to.
     As long as the mouse stays within this range, we need not
     redraw anything on its account.  */
  int mouse_face_beg_row, mouse_face_beg_col;
  int mouse_face_end_row, mouse_face_end_col;
  int mouse_face_past_end;
  Lisp_Object mouse_face_window;
  int mouse_face_face_id;

  /* 1 if a mouse motion event came and we didn't handle it right away because
     gc was in progress.  */
  int mouse_face_deferred_gc;

  /* FRAME and X, Y position of mouse when last checked for
     highlighting.  X and Y can be negative or out of range for the frame.  */
  struct frame *mouse_face_mouse_frame;
  int mouse_face_mouse_x, mouse_face_mouse_y;

  /* Nonzero means defer mouse-motion highlighting.  */
  int mouse_face_defer;
};


#define x_destroy_bitmap(f,id)

extern Display *x_current_display;
extern int pm_serial;

extern struct x_display_info *pm_display;

#define PIXEL_WIDTH(f)  ((f)->output_data.x->pixel_width)
#define PIXEL_HEIGHT(f) ((f)->output_data.x->pixel_height)

#define FONT_WIDTH(f)   ((f)->max_bounds.width)
#define FONT_HEIGHT(f)  ((f)->ascent + (f)->descent)
#define FONT_BASE(f)    ((f)->ascent)

#define FRAME_PARAM_FACES(f) ((f)->output_data.x->param_faces)
#define FRAME_N_PARAM_FACES(f) ((f)->output_data.x->n_param_faces)
#define FRAME_DEFAULT_PARAM_FACE(f) (FRAME_PARAM_FACES (f)[0])
#define FRAME_MODE_LINE_PARAM_FACE(f) (FRAME_PARAM_FACES (f)[1])

#define FRAME_COMPUTED_FACES(f) ((f)->output_data.x->computed_faces)
#define FRAME_N_COMPUTED_FACES(f) ((f)->output_data.x->n_computed_faces)
#define FRAME_SIZE_COMPUTED_FACES(f) ((f)->output_data.x->size_computed_faces)
#define FRAME_DEFAULT_FACE(f) ((f)->output_data.x->computed_faces[0])
#define FRAME_MODE_LINE_FACE(f) ((f)->output_data.x->computed_faces[1])

#define FRAME_X_DISPLAY(f) 0

/* This gives the x_display_info structure for the display F is on.  */
#define FRAME_X_DISPLAY_INFO(f) ((f)->output_data.x->display_info)

#define FRAME_FOREGROUND_PIXEL(f) ((f)->output_data.x->foreground_color)
#define FRAME_BACKGROUND_PIXEL(f) ((f)->output_data.x->background_color)
#define FRAME_FONT(f) ((f)->output_data.x->font)
#define FRAME_INTERNAL_BORDER_WIDTH(f) ((f)->output_data.x->internal_border_width)
#define FRAME_LINE_HEIGHT(f) ((f)->output_data.x->line_height)

/* We represent scroll bars as lisp vectors.  This allows us to place
   references to them in windows without worrying about whether we'll
   end up with windows referring to dead scroll bars; the garbage
   collector will free it when its time comes.

   We use struct scroll_bar as a template for accessing fields of the
   vector.  */

struct scroll_bar {

  /* These fields are shared by all vectors.  */
  EMACS_INT size_from_Lisp_Vector_struct;
  struct Lisp_Vector *next_from_Lisp_Vector_struct;

  /* The window we're a scroll bar for.  */
  Lisp_Object window;

  /* The next and previous in the chain of scroll bars in this frame.  */
  Lisp_Object next, prev;

  /* The ID of this scroll bar.  */
  Lisp_Object id;

  /* Speed hack: the following fields cache the status of the
     scrollbar.  */
  Lisp_Object top, left, width, height;
  Lisp_Object whole, portion, position;
  Lisp_Object scaled_whole, scaled_portion, scaled_position;
};

/* The number of elements a vector holding a struct scroll_bar needs.  */
#define SCROLL_BAR_VEC_SIZE					\
  ((sizeof (struct scroll_bar)					\
    - sizeof (EMACS_INT) - sizeof (struct Lisp_Vector *))	\
   / sizeof (Lisp_Object))

/* Turning a lisp vector value into a pointer to a struct scroll_bar.  */
#define XSCROLL_BAR(vec) ((struct scroll_bar *) XVECTOR (vec))
