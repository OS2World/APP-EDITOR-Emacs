head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	2000.10.21.20.44.47;	author JeremyB;	state Exp;
branches;
next	;


desc
@OS/2 Emacs.
@


1.1
log
@Initial revision
@
text
@/* pmfaces.c -- mouse face code from xterm.c for the OS/2 Presentation Manager
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.

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


#include <stdlib.h>
#include "config.h"
#include "lisp.h"
#include "buffer.h"
#include "window.h"
#include "frame.h"
#include "dispextern.h"
#include "pmterm.h"

extern int curs_x;
extern int curs_y;

/* Borrowed from xterm.c. */

extern Lisp_Object Qmouse_face;

/* This is used for debugging, to turn off note_mouse_highlight.  */
static int disable_mouse_highlight;


/* Borrowed from xterm.c. */

#define min(a,b) ((a)<(b) ? (a) : (b))

/* TODO */
#define CHAR_TO_PIXEL_ROW(f, row) (row)
#define CHAR_TO_PIXEL_COL(f, col) (col)

/* Find the row and column of position POS in window WINDOW.
   Store them in *COLUMNP and *ROWP.
   This assumes display in WINDOW is up to date.
   If POS is above start of WINDOW, return coords
   of start of first screen line.
   If POS is after end of WINDOW, return coords of end of last screen line.

   Value is 1 if POS is in range, 0 if it was off screen.  */

static int
fast_find_position (window, pos, columnp, rowp)
     Lisp_Object window;
     int pos;
     int *columnp, *rowp;
{
  struct window *w = XWINDOW (window);
  FRAME_PTR f = XFRAME (WINDOW_FRAME (w));
  int i;
  int row = 0;
  int left = w->left;
  int top = w->top;
  int height = XFASTINT (w->height) - ! MINI_WINDOW_P (w);
  int width = window_internal_width (w);
  int *charstarts;
  int lastcol;
  int maybe_next_line = 0;

  /* Find the right row.  */
  for (i = 0;
       i < height;
       i++)
    {
      int linestart = FRAME_CURRENT_GLYPHS (f)->charstarts[top + i][left];
      if (linestart > pos)
	break;
      /* If the position sought is the end of the buffer,
	 don't include the blank lines at the bottom of the window.  */
      if (linestart == pos && pos == BUF_ZV (XBUFFER (w->buffer)))
	{
	  maybe_next_line = 1;
	  break;
	}
      if (linestart > 0)
	row = i;
    }

  /* Find the right column with in it.  */
  charstarts = FRAME_CURRENT_GLYPHS (f)->charstarts[top + row];
  lastcol = left;
  for (i = 0; i < width; i++)
    {
      if (charstarts[left + i] == pos)
	{
	  *rowp = row + top;
	  *columnp = i + left;
	  return 1;
	}
      else if (charstarts[left + i] > pos)
	break;
      else if (charstarts[left + i] > 0)
	lastcol = left + i;
    }

  /* If we're looking for the end of the buffer,
     and we didn't find it in the line we scanned,
     use the start of the following line.  */
  if (maybe_next_line)
    {
      row++;
      i = 0;
    }

  *rowp = row + top;
  *columnp = lastcol;
  return 0;
}

/* Display the active region described by mouse_face_*
   in its mouse-face if HL > 0, in its normal face if HL = 0.  */

static void
show_mouse_face (dpyinfo, hl)
     struct x_display_info *dpyinfo;
     int hl;
{
  struct window *w = XWINDOW (dpyinfo->mouse_face_window);
  int width = window_internal_width (w);
  FRAME_PTR f = XFRAME (WINDOW_FRAME (w));
  int i;
  int cursor_off = 0;
  int old_curs_x = curs_x;
  int old_curs_y = curs_y;

  /* Set these variables temporarily
     so that if we have to turn the cursor off and on again
     we will put it back at the same place.  */
  curs_x = f->phys_cursor_x;
  curs_y = f->phys_cursor_y;

  for (i = FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_row;
       i <= FRAME_X_DISPLAY_INFO (f)->mouse_face_end_row; i++)
    {
      int column = (i == FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_row
		    ? FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_col
		    : w->left);
      int endcolumn = (i == FRAME_X_DISPLAY_INFO (f)->mouse_face_end_row
		       ? FRAME_X_DISPLAY_INFO (f)->mouse_face_end_col
		       : w->left + width);
      endcolumn = min (endcolumn, FRAME_CURRENT_GLYPHS (f)->used[i]);

      /*TODO*/
#ifndef EMX
      /* If the cursor's in the text we are about to rewrite,
	 turn the cursor off.  */
      if (i == curs_y
	  && curs_x >= FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_col - 1
	  && curs_x <= FRAME_X_DISPLAY_INFO (f)->mouse_face_end_col)
	{
	  x_display_cursor (f, 0);
	  cursor_off = 1;
	}

      dumpglyphs (f,
		  CHAR_TO_PIXEL_COL (f, column),
		  CHAR_TO_PIXEL_ROW (f, i),
		  FRAME_CURRENT_GLYPHS (f)->glyphs[i] + column,
		  endcolumn - column,
		  /* Highlight with mouse face if hl > 0.  */
		  hl > 0 ? 3 : 0, 0);
#else /* EMX */
      dump_glyphs (f,
		   FRAME_CURRENT_GLYPHS (f)->glyphs[i] + column,
                   FRAME_CURRENT_GLYPHS (f)->enable[i],
                   FRAME_CURRENT_GLYPHS (f)->used[i],
		   CHAR_TO_PIXEL_COL (f, column),
		   CHAR_TO_PIXEL_ROW (f, i),
		   endcolumn - column,
		   /* Highlight with mouse face if hl > 0.  */
		   hl > 0 ? 3 : 0);
#endif /* EMX */
    }

  /*TODO*/
#ifdef EMX
  curs_x = old_curs_x;
  curs_y = old_curs_y;
#else /* not EMX */
  /* If we turned the cursor off, turn it back on.  */
  if (cursor_off)
    x_display_cursor (f, 1);

  curs_x = old_curs_x;
  curs_y = old_curs_y;

  /* Change the mouse cursor according to the value of HL.  */
  if (hl > 0)
    XDefineCursor (FRAME_X_DISPLAY (f), FRAME_X_WINDOW (f),
		   f->display.x->cross_cursor);
  else
    XDefineCursor (FRAME_X_DISPLAY (f), FRAME_X_WINDOW (f),
		   f->display.x->text_cursor);
#endif /* not EMX */
}

/* Clear out the mouse-highlighted active region.
   Redraw it unhighlighted first.  */

void
clear_mouse_face (dpyinfo)
     struct x_display_info *dpyinfo;
{
  if (! NILP (dpyinfo->mouse_face_window))
    show_mouse_face (dpyinfo, 0);

  dpyinfo->mouse_face_beg_row = dpyinfo->mouse_face_beg_col = -1;
  dpyinfo->mouse_face_end_row = dpyinfo->mouse_face_end_col = -1;
  dpyinfo->mouse_face_window = Qnil;
}

/* Take proper action when the mouse has moved to position X, Y on frame F
   as regards highlighting characters that have mouse-face properties.
   Also dehighlighting chars where the mouse was before.
   X and Y can be negative or out of range.  */

void
note_mouse_highlight (f, x, y)
     FRAME_PTR f;
     int x, y;
{
  int row, column, portion;
  XRectangle new_glyph;
  Lisp_Object window;
  struct window *w;

  if (disable_mouse_highlight)
    return;

  FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_x = x;
  FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_y = y;
  FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_frame = f;

  if (FRAME_X_DISPLAY_INFO (f)->mouse_face_defer)
    return;

  if (gc_in_progress)
    {
      FRAME_X_DISPLAY_INFO (f)->mouse_face_deferred_gc = 1;
      return;
    }

  /* Find out which glyph the mouse is on.  */
  pixel_to_glyph_coords (f, x, y, &column, &row,
			 &new_glyph, FRAME_X_DISPLAY_INFO (f)->grabbed);

  /* Which window is that in?  */
  window = window_from_coordinates (f, column, row, &portion);
  w = XWINDOW (window);

  /* If we were displaying active text in another window, clear that.  */
  if (! EQ (window, FRAME_X_DISPLAY_INFO (f)->mouse_face_window))
    clear_mouse_face (FRAME_X_DISPLAY_INFO (f));

  /* Are we in a window whose display is up to date?
     And verify the buffer's text has not changed.  */
  if (WINDOWP (window) && portion == 0 && row >= 0 && column >= 0
      && row < FRAME_HEIGHT (f) && column < FRAME_WIDTH (f)
      && EQ (w->window_end_valid, w->buffer)
      && w->last_modified == BUF_MODIFF (XBUFFER (w->buffer)))
    {
      int *ptr = FRAME_CURRENT_GLYPHS (f)->charstarts[row];
      int i, pos;

      /* Find which buffer position the mouse corresponds to.  */
      for (i = column; i >= 0; i--)
	if (ptr[i] > 0)
	  break;
      pos = ptr[i];
      /* Is it outside the displayed active region (if any)?  */
      if (pos <= 0)
	clear_mouse_face (FRAME_X_DISPLAY_INFO (f));
      else if (! (EQ (window, FRAME_X_DISPLAY_INFO (f)->mouse_face_window)
		  && row >= FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_row
		  && row <= FRAME_X_DISPLAY_INFO (f)->mouse_face_end_row
		  && (row > FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_row
		      || column >= FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_col)
		  && (row < FRAME_X_DISPLAY_INFO (f)->mouse_face_end_row
		      || column < FRAME_X_DISPLAY_INFO (f)->mouse_face_end_col
		      || FRAME_X_DISPLAY_INFO (f)->mouse_face_past_end)))
	{
	  Lisp_Object mouse_face, overlay, position;
	  Lisp_Object *overlay_vec;
	  int len, noverlays, ignor1;
	  struct buffer *obuf;
	  int obegv, ozv;

	  /* If we get an out-of-range value, return now; avoid an error.  */
	  if (pos > BUF_Z (XBUFFER (w->buffer)))
	    return;

	  /* Make the window's buffer temporarily current for
	     overlays_at and compute_char_face.  */
	  obuf = current_buffer;
	  current_buffer = XBUFFER (w->buffer);
	  obegv = BEGV;
	  ozv = ZV;
	  BEGV = BEG;
	  ZV = Z;

	  /* Yes.  Clear the display of the old active region, if any.  */
	  clear_mouse_face (FRAME_X_DISPLAY_INFO (f));

	  /* Is this char mouse-active?  */
	  XSETINT (position, pos);

	  len = 10;
	  overlay_vec = (Lisp_Object *) xmalloc (len * sizeof (Lisp_Object));

	  /* Put all the overlays we want in a vector in overlay_vec.
	     Store the length in len.  */
	  noverlays = overlays_at (XINT (pos), 1, &overlay_vec, &len,
				   NULL, NULL);
	  noverlays = sort_overlays (overlay_vec, noverlays, w);

	  /* Find the highest priority overlay that has a mouse-face prop.  */
	  overlay = Qnil;
	  for (i = 0; i < noverlays; i++)
	    {
	      mouse_face = Foverlay_get (overlay_vec[i], Qmouse_face);
	      if (!NILP (mouse_face))
		{
		  overlay = overlay_vec[i];
		  break;
		}
	    }
	  free (overlay_vec);
	  /* If no overlay applies, get a text property.  */
	  if (NILP (overlay))
	    mouse_face = Fget_text_property (position, Qmouse_face, w->buffer);

	  /* Handle the overlay case.  */
	  if (! NILP (overlay))
	    {
	      /* Find the range of text around this char that
		 should be active.  */
	      Lisp_Object before, after;
	      int ignore;

	      before = Foverlay_start (overlay);
	      after = Foverlay_end (overlay);
	      /* Record this as the current active region.  */
	      fast_find_position (window, before,
				  &FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_col,
				  &FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_row);
	      FRAME_X_DISPLAY_INFO (f)->mouse_face_past_end
		= !fast_find_position (window, after,
				       &FRAME_X_DISPLAY_INFO (f)->mouse_face_end_col,
				       &FRAME_X_DISPLAY_INFO (f)->mouse_face_end_row);
	      FRAME_X_DISPLAY_INFO (f)->mouse_face_window = window;
	      FRAME_X_DISPLAY_INFO (f)->mouse_face_face_id
		= compute_char_face (f, w, pos, 0, 0,
				     &ignore, pos + 1, 1);

	      /* Display it as active.  */
	      show_mouse_face (FRAME_X_DISPLAY_INFO (f), 1);
	    }
	  /* Handle the text property case.  */
	  else if (! NILP (mouse_face))
	    {
	      /* Find the range of text around this char that
		 should be active.  */
	      Lisp_Object before, after, beginning, end;
	      int ignore;

	      beginning = Fmarker_position (w->start);
	      XSETINT (end, (BUF_Z (XBUFFER (w->buffer))
			     - XFASTINT (w->window_end_pos)));
	      before
		= Fprevious_single_property_change (make_number (pos + 1),
						    Qmouse_face,
						    w->buffer, beginning);
	      after
		= Fnext_single_property_change (position, Qmouse_face,
						w->buffer, end);
	      /* Record this as the current active region.  */
	      fast_find_position (window, before,
				  &FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_col,
				  &FRAME_X_DISPLAY_INFO (f)->mouse_face_beg_row);
	      FRAME_X_DISPLAY_INFO (f)->mouse_face_past_end
		= !fast_find_position (window, after,
				       &FRAME_X_DISPLAY_INFO (f)->mouse_face_end_col,
				       &FRAME_X_DISPLAY_INFO (f)->mouse_face_end_row);
	      FRAME_X_DISPLAY_INFO (f)->mouse_face_window = window;
	      FRAME_X_DISPLAY_INFO (f)->mouse_face_face_id
		= compute_char_face (f, w, pos, 0, 0,
				     &ignore, pos + 1, 1);

	      /* Display it as active.  */
	      show_mouse_face (FRAME_X_DISPLAY_INFO (f), 1);
	    }
	  BEGV = obegv;
	  ZV = ozv;
	  current_buffer = obuf;
	}
    }
}
@
