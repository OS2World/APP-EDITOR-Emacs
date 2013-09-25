/* pmterm.c -- xterm.c for the OS/2 Presentation Manager
   Copyright (C) 1993-1996 Eberhard Mattes.
   Copyright (C) 1995 Patrick Nadeau (scroll bar code).
   Copyright (C) 1989-1995 Free Software Foundation, Inc. (code from xterm.c).

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


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/process.h>
#include <sys/ioctl.h>

#include "config.h"
#include "lisp.h"
#include "blockinput.h"
#include "keyboard.h"
#include "window.h"
#include "frame.h"
#include "disptab.h"
#include "termhooks.h"
#include "termopts.h"
#include "termchar.h"
#include "dispextern.h"
#include "pmterm.h"
#include "pmemacs.h"
#include "charset.h"


pmd_config pm_config;

struct pm_queue
{
  struct pm_queue *next;
  pm_event event;
};

static struct pm_queue *pm_queue_first;
static struct pm_queue **pm_queue_add = &pm_queue_first;

int pm_pid = -1;

/* The current serial number for requests which return an answer. */
int pm_serial;

extern struct face *intern_face (struct frame *f, struct face *face);

void x_make_frame_visible (struct frame *f);
static void dump_rectangle (FRAME_PTR f, int left, int top, int right,
    int bottom);
static Lisp_Object find_scroll_bar_window (FRAME_PTR f, unsigned id);


/* The current code page, defined in emxdep.c. */
extern int cur_code_page;

/* This is the quit character, which is defined in keyboard.c.  It is
   used by the PM_set_terminal_modes function, which is assigned to
   set_terminal_modes_hook. */
extern int quit_char;

/* Dummy variable for simplifying changes to xfaces.c. */
Display *x_current_display;

/* cf. x_display_list */
struct x_display_info *pm_display;

int pm_session_started = 0;

int curs_x;
int curs_y;

extern struct frame *updating_frame;

static int outbound_pipe;

static char pm_send_buffer[4096];
static int pm_send_buffered = 0;

/* During an update, nonzero if chars output now should be highlighted.  */
static int highlight;

/* During an update, maximum vpos for ins/del line operations to affect.  */
static int flexlines;

/* The mouse is on this frame.  */
static FRAME_PTR mouse_frame;

/* Shut down Emacs in an orderly fashion, because of a SIGPIPE on the
   pmemacs pipe. */

static SIGTYPE
pm_sigpipe (int sig)
{
  shut_down_emacs (sig, 1, Qnil);
  fatal ("pmemacs died.");
}


/* Send a message to pmemacs. */

void pm_send (const void *src, unsigned size)
{
    const char *s;
    int n;

    if (pm_send_buffered > 0)     /* Important! */
        pm_send_flush ();
    s = src;
    while (size != 0)
    {
        n = write (outbound_pipe, s, size);
        if (n == -1 || n == 0)
        {
            shut_down_emacs (0, 1, Qnil);
            fatal ("Cannot send message to PM Emacs.");
        }
        size -= n;
        s += n;
    }
}


/* When sending many small requests, it's better to use this function.
   If we want to receive data from pmemacs we have to call
   pm_send_flush() before waiting for the answer. */

void pm_send_collect (const void *src, unsigned size)
{
  if (pm_send_buffered + size > sizeof (pm_send_buffer))
    {
      pm_send_flush ();
      if (size > sizeof (pm_send_buffer))
        {
          pm_send (src, size);
          return;
        }
    }
  memcpy (pm_send_buffer + pm_send_buffered, src, size);
  pm_send_buffered += size;
}


/* Flush buffer filled by pm_send_collect. */

void pm_send_flush (void)
{
  int size;

  if (pm_send_buffered > 0)
    {
      size = pm_send_buffered;
      pm_send_buffered = 0;
      pm_send (pm_send_buffer, size);
    }
}


/* Receive data from pmemacs. */

static int receive (void *dst, size_t size, int return_error)
{
  char *d;
  int n;

  d = dst;
  while (size != 0)
    {
      n = read (0, d, size);
      if (n == -1 && errno == EINTR)
        continue;
      if (n == -1 || n == 0)
        {
          if (return_error)
            return 0;
          else
            {
              shut_down_emacs (0, 1, Qnil);
              fatal ("Failed to receive data from PM Emacs.");
            }
        }
      size -= n;
      d += n;
    }
  return 1;
}


/* Put an event into the queue, for processing it later. */

static void pm_put_queue (pm_event *event)
{
  struct pm_queue *pmq;

  pmq = (struct pm_queue *)xmalloc (sizeof (struct pm_queue));
  pmq->next = NULL;
  pmq->event = *event;
  *pm_queue_add = pmq;
  pm_queue_add = &pmq->next;
  interrupt_input_pending = 1;
}


/* Get an event from the queue. */

static void pm_get_queue (pm_event *event)
{
  struct pm_queue *next;

  *event = pm_queue_first->event;
  next = pm_queue_first->next;
  xfree (pm_queue_first);
  pm_queue_first = next;
  if (pm_queue_first == NULL)
    pm_queue_add = &pm_queue_first;
  else
    interrupt_input_pending = 1;
}


/* Discard the data following an PME_ANSWER event. */

static void discard_answer (pm_event *p)
{
  char *tem;

  if (p->answer.size > 0)
    {
      tem = (char *)xmalloc (p->answer.size);
      receive (tem, p->answer.size, 0);
      xfree (tem);
    }
}


/* Receive an answer from pmemacs.  SERIAL is the serial number of the
   request.  Store the received data to DST.  If DST is NULL, use
   malloc() to allocate a suitable buffer (this is not done when
   receiving a single int).  Store the number of bytes received to
   *SIZE, unless SIZE is NULL.  Process PME_PAINT events if MAY_PAINT
   is true.  Return a pointer to the buffer (DST or allocated).
   Return NULL on failure.

   The pm_send() call and the associated pm_receive() call must be
   enclosed in BLOCK_INPUT and UNBLOCK_INPUT. */

void *pm_receive (int serial, void *dst, int *size, int may_paint)
{
  pm_event pme;
  FRAME_PTR f;

  /* TODO: Check queue for PME_PAINT events if MAY_PAINT is true? */
  for (;;)
    {
      receive (&pme, sizeof (pme), 0);
      switch (pme.header.type)
        {
        case PME_ANSWER:
          if (pme.answer.serial != serial)
            {
              printf ("pm_receive: serial number mismatch: %d vs. %d\r\n",
                      pme.answer.serial, serial);
              discard_answer (&pme);
              if (size != NULL)
                *size = 0;
              return (NULL);
            }
          if (pme.answer.size == -1)
            {
              /* DST must be non-NULL. */
              *(int *)dst = pme.answer.one_word;
              if (size != NULL)
                *size = sizeof (int);
            }
          else
            {
              if (dst == NULL)
                dst = xmalloc (pme.answer.size);
              receive (dst, pme.answer.size, 0);
              if (size != NULL)
                *size = pme.answer.size;
            }
          return (dst);

        case PME_PAINT:
          if (may_paint)
            {
              f = (FRAME_PTR)pme.header.frame;
              if (pm_active_frame (f))
                dump_rectangle (f, pme.paint.x0, pme.paint.y0,
                                pme.paint.x1, pme.paint.y1);
            }
          else
            pm_put_queue (&pme);
          break;

        default:

          /* Warning: pm_put_queue() calls xmalloc(), which may call
             PM_read_socket(), which does not know how to handle
             PME_ANSWER.  Therefore the caller of pm_receive() must
             use BLOCK_INPUT! */

          pm_put_queue (&pme);
          break;
        }
    }
}

/* Keeping a table of active frames.  This is used to avoid acting on
   events for frames already deleted.  After deleting a frame, there
   may still be events for that frame in the queue (pipe 0). */

#define HASH_SIZE 53
#define FRAME_HASH(f) ((unsigned long)f % HASH_SIZE)

struct hash_frame
{
  struct hash_frame *next;
  FRAME_PTR frame;
};

static struct hash_frame *frame_table[HASH_SIZE];

/* Before sending PMR_CREATE, this function should be called to enable
   receiving events from the new frame F. */

void pm_add_frame (FRAME_PTR f)
{
  struct hash_frame *p;
  int hash;

  hash = FRAME_HASH (f);
  for (p = frame_table[hash]; p != 0; p = p->next)
    if (p->frame == f)
      abort ();
  p = (struct hash_frame *) xmalloc (sizeof (*p));
  p->frame = f;
  p->next = frame_table[hash];
  frame_table[hash] = p;
}


/* When deleting a frame, this function should be called to disable
   receiving events from the frame F. */

void pm_del_frame (FRAME_PTR f)
{
  struct hash_frame **p, *q;

  for (p = &frame_table[FRAME_HASH (f)]; *p != 0; p = &(*p)->next)
    if ((*p)->frame == f)
      {
        q = (*p)->next;
        xfree (*p);
        *p = q;
        return;
      }
  abort ();
}


/* Check whether we are allowed to receive events for frame F. */

int pm_active_frame (FRAME_PTR f)
{
  struct hash_frame *p;

  for (p = frame_table[FRAME_HASH (f)]; p != 0; p = p->next)
    if (p->frame == f)
      return 1;
  return 0;
}



/* Turn mouse tracking on or off.  Tell pmemacs to start or stop
   sending PME_MOUSEMOVE events. */

void pm_mouse_tracking (int flag)
{
#if 0
  /* As Emacs 19.23 uses mouse movement events for highlighting
   mouse-sensitive areas, mouse movement events are always enabled and
   pm_mouse_tracking does nothing.  Perhaps later versions of Emacs
   want to turn off mouse movement events, therefore we keep this
   code. */

  pm_request pmr;

  pmr.track.header.type = PMR_TRACKMOUSE;
  pmr.track.header.frame = 0;
  pmr.track.flag = flag;
  pm_send (&pmr, sizeof (pmr));
#endif
}


/* Turn the cursor on or off. */

static void pm_display_cursor (struct frame *f, int on)
{
  pm_request pmr;

  if (!FRAME_VISIBLE_P (f))
    return;
  if (!on && f->phys_cursor_x < 0)
    return;
  if (f != updating_frame)
    {
      curs_x = FRAME_CURSOR_X (f);
      curs_y = FRAME_CURSOR_Y (f);
    }
  if (on && (f->phys_cursor_x != curs_x || f->phys_cursor_y != curs_y))
    {
      pmr.cursor.header.type = PMR_CURSOR;
      pmr.cursor.header.frame = (unsigned long)f;
      pmr.cursor.x = curs_x;
      pmr.cursor.y = curs_y;
      pmr.cursor.on = 1;
      pm_send (&pmr, sizeof (pmr));
      f->phys_cursor_x = curs_x;
      f->phys_cursor_y = curs_y;
    }
  if (!on)
    {
      pmr.cursor.header.type = PMR_CURSOR;
      pmr.cursor.header.frame = (unsigned long)f;
      pmr.cursor.x = 0;
      pmr.cursor.y = 0;
      pmr.cursor.on = 0;
      pm_send (&pmr, sizeof (pmr));
      f->phys_cursor_x = -1;
    }
}


/* TODO: avoid PMR_CLREOL */

void dump_glyphs (struct frame *f, GLYPH *gp, int enable, int used,
                  int x, int y, int cols, int hl)
{
    pm_request pmr, *pmrp;
    int n, clear;
    char *buf;
    int tlen = GLYPH_TABLE_LENGTH;
    Lisp_Object *tbase = GLYPH_TABLE_BASE;

    buf = alloca (sizeof (pmr) + cols);
    pmrp = (pm_request *)buf;
    buf += sizeof (pmr);
    if (enable)
    {
        n = used - x;
        if (n < 0) n = 0;
    }
    else
        n = 0;
    if (n > cols)
        n = cols;
    clear = cols - n;

    while (n > 0)
    {
        /* Get the face-code of the next GLYPH.  */
        int cf, len;
        int g = *gp;
        char *cp;

        GLYPH_FOLLOW_ALIASES (tbase, tlen, g);
        cf = FAST_GLYPH_FACE (g);
        /* Find the run of consecutive glyphs with the same face-code.
           Extract their character codes into BUF.  */
        cp = buf;
        while (n > 0)
        {
            g = *gp;
            GLYPH_FOLLOW_ALIASES (tbase, tlen, g);
            if (FAST_GLYPH_FACE (g) != cf)
                break;

            *cp++ = FAST_GLYPH_CHAR (g);
            --n;
            ++gp;
        }

        /* LEN gets the length of the run.  */
        len = cp - buf;

        /* Now output this run of chars, with the font and pixel values
           determined by the face code CF.  */
        {
            struct face *face = FRAME_DEFAULT_FACE (f);

            /* HL = 3 means use a mouse face previously chosen.  */
            if (hl == 3)
                cf = FRAME_X_DISPLAY_INFO (f)->mouse_face_face_id;

            if (cf != 0)
            {
                /* It's possible for the display table to specify
                   a face code that is out of range.  Use 0 in that case.  */
                if (cf < 0 || cf >= FRAME_N_COMPUTED_FACES (f))
                    cf = 0;

                if (cf == 1)
                    face = FRAME_MODE_LINE_FACE (f);
                else
                    face = intern_face (f, FRAME_COMPUTED_FACES (f) [cf]);
            }
            else if (hl == 1)
            {
                face = FRAME_MODE_LINE_FACE (f);
            }
            pmrp->glyphs.header.type = PMR_GLYPHS;
            pmrp->glyphs.header.frame = (unsigned long)f;
            pmrp->glyphs.count = len;
            pmrp->glyphs.x = x;
            pmrp->glyphs.y = y;
            pmrp->glyphs.face = face->gc;
            pm_send_collect (pmrp, sizeof (pmr) + len);
        }

        x += len;
    }

    if (clear > 0)
    {
        pmr.clreol.header.type = PMR_CLREOL;
        pmr.clreol.header.frame = (unsigned long)f;
        pmr.clreol.y = y;
        pmr.clreol.x0 = x;
        pmr.clreol.x1 = x + clear; /* -1 ? */
        pm_send_collect (&pmr, sizeof (pmr));
    }
    pm_send_flush ();
}


static void dump_rectangle (FRAME_PTR f, int left, int top,
                            int right, int bottom)
{
  struct frame_glyphs *active_frame = FRAME_CURRENT_GLYPHS (f);
  int cols, rows, y, cursor_flag;

  if (FRAME_GARBAGED_P (f))
    return;

  /* Clip the rectangle to what can be visible.  */
  if (left < 0)
    left = 0;
  if (top < 0)
    top = 0;
  if (right >= f->width)
    right = f->width - 1;
  if (bottom >= f->height)
    bottom = f->height - 1;

  /* Get size in chars of the rectangle.  */
  cols = 1 + right - left;
  rows = 1 + bottom - top;

  /* If rectangle has zero area, return.  */
  if (rows <= 0) return;
  if (cols <= 0) return;

  cursor_flag = (f->phys_cursor_x >= 0);
  pm_display_cursor (f, 0);

  /* Display the text in the rectangle, one text line at a time.  */

  for (y = top; y <= bottom; y++)
    dump_glyphs (f, &active_frame->glyphs[y][left],
                 active_frame->enable[y], active_frame->used[y],
                 left, y, cols, active_frame->highlight[y]);
  if (cursor_flag)
    pm_display_cursor (f, 1);
}


char *x_get_keysym_name (int keysym)
{
  return NULL;
}


/* Given a pixel position (PIX_X, PIX_Y) on the frame F, return
   glyph co-ordinates in (*X, *Y).  Set *BOUNDS to the rectangle
   that the glyph at X, Y occupies, if BOUNDS != 0.
   If NOCLIP is nonzero, do not force the value into range.  */

void
pixel_to_glyph_coords (f, pix_x, pix_y, x, y, bounds, noclip)
     FRAME_PTR f;
     register int pix_x, pix_y;
     register int *x, *y;
     XRectangle *bounds;
     int noclip;
{
  /*TODO*/
  *x = pix_x;
  *y = pix_y;
  if (bounds)
    {
      bounds->x = pix_x;
      bounds->y = pix_y;
      bounds->width = 1;
      bounds->height = 1;
    }
}

void
glyph_to_pixel_coords (f, x, y, pix_x, pix_y)
     FRAME_PTR f;
     register int x, y;
     register int *pix_x, *pix_y;
{
  /*TODO*/
  *pix_x = x;
  *pix_y = y;
}


/* This function can be called at quite inconvenient times; therefore
   it must not do anything which may result in a call to `message2' or
   `update_frame'.  */

int PM_read_socket (int sd, struct input_event *bufp, int numchars,
                    /* int waitp, */ int expected)
{
  int count = 0;
  pm_event pme;
  int nread;
  FRAME_PTR f;

  if (interrupt_input_blocked)
    {
      interrupt_input_pending = 1;
      return -1;
    }
  interrupt_input_pending = 0;
  if (numchars <= 0)
    abort ();

  if (pm_queue_first != NULL)
    pm_get_queue (&pme);
  else
    {
      if (ioctl (0, FIONREAD, &nread) < 0 || nread < sizeof (pme))
        return 0;
      receive (&pme, sizeof (pme), 0);
    }
  f = (FRAME_PTR)pme.header.frame;
  if (!pm_active_frame (f) && pme.header.type != PME_ANSWER)
    return 0;
  switch (pme.header.type)
    {
    case PME_PAINT:

      /* A frame needs repainting. */

      dump_rectangle (f, pme.paint.x0, pme.paint.y0,
                      pme.paint.x1, pme.paint.y1);
      break;

    case PME_KEY:

      /* A key has been pressed. */

      switch (pme.key.type)
        {
        case PMK_ASCII:
          bufp->kind = ascii_keystroke;
          bufp->code = pme.key.code;
          break;
        case PMK_VIRTUAL:
          bufp->kind = non_ascii_keystroke;
          bufp->code = pme.key.code | (1 << 28);
          break;
        default:
          abort ();
        }
      bufp->modifiers = pme.key.modifiers;
      bufp->timestamp = 0;
      XSETFRAME (bufp->frame_or_window, f);
      ++count; ++bufp;
      break;

    case PME_BUTTON:

      /* A mouse button has been pressed or released. */

      bufp->kind = mouse_click;
      bufp->code = pme.button.button;
      XSETFRAME (bufp->frame_or_window, f);
      XSETFASTINT (bufp->x, pme.button.x);
      XSETFASTINT (bufp->y, pme.button.y);
      bufp->modifiers = pme.button.modifiers;
      bufp->timestamp = pme.button.timestamp;
      ++count; ++bufp;
      break;

    case PME_SCROLLBAR:

      /* Some event has occured on the scroll bar. */

      bufp->kind = scroll_bar_click;
      bufp->code = pme.scrollbar.button;
      bufp->part = pme.scrollbar.part;
      bufp->frame_or_window = find_scroll_bar_window (f, pme.scrollbar.id);
      XSETFASTINT (bufp->x, pme.scrollbar.x);
      XSETFASTINT (bufp->y, pme.scrollbar.y);
      bufp->modifiers = pme.button.modifiers;
      bufp->timestamp = pme.button.timestamp;
      ++count; ++bufp;
      break;

    case PME_SIZE:

      /* The size of the frame has been changed by the user.  Adjust
         frame and set new size. */

      if (pme.size.width != FRAME_WIDTH (f)
          || pme.size.height != FRAME_HEIGHT (f))
        {
          change_frame_size (f, pme.size.height, pme.size.width, 0, 1);
          f->output_data.x->pixel_height = pme.size.pix_height;
          f->output_data.x->pixel_width = pme.size.pix_width;
          SET_FRAME_GARBAGED (f);
        }
      break;

    case PME_RESTORE:

      /* A frame window is being restored. */

      if (f->async_visible == 0)
        {
          f->async_visible = 1;
          f->async_iconified = 0;
          SET_FRAME_GARBAGED (f);
        }
      bufp->kind = deiconify_event;
      XSETFRAME (bufp->frame_or_window, f);
      bufp++; count++; numchars--;
      break;

    case PME_MINIMIZE:

      /* A frame window is being minimized. */

      f->async_visible = 0;
      f->async_iconified = 1;
      bufp->kind = iconify_event;
      XSETFRAME (bufp->frame_or_window, f);
      bufp++; count++; numchars--;
      break;

    case PME_WINDOWDELETE:

      /* Close a window. */

      bufp->kind = delete_window_event;
      XSETFRAME (bufp->frame_or_window, f);
      bufp++; count++; numchars--;
      break;

    case PME_MENUBAR:

      /* An item of a submenu of the menu bar has been selected. */
      pm_menubar_selection ((FRAME_PTR)pme.menubar.header.frame,
                            pme.menubar.number);
      break;

    case PME_MOUSEMOVE:

      /* The mouse has been moved. */

      mouse_frame = f;
      f->output_data.x->mouse_x = pme.mouse.x;
      f->output_data.x->mouse_y = pme.mouse.y;
      f->mouse_moved = 1;
      note_mouse_highlight (f, pme.mouse.x, pme.mouse.y);
      break;

    case PME_FRAMEMOVE:

      /* A frame has been moved. */

      f->output_data.x->left_pos = pme.framemove.left;
      f->output_data.x->top_pos = pme.framemove.top;
      break;

    case PME_ANSWER:

      /* A synchronous answer.  Ignore it. */

      discard_answer (&pme);
      break;

    default:
      fatal ("Bad event type %d from PM Emacs.", pme.header.type);
    }
  return count;
}


static void PM_set_terminal_modes (void)
{
  pm_request pmr;

  pmr.quitchar.header.type = PMR_QUITCHAR;
  pmr.quitchar.header.frame = 0;
  pmr.quitchar.quitchar = quit_char;
  pm_send (&pmr, sizeof (pmr));
}


static void PM_cursor_to (int row, int col)
{
  curs_x = col;
  curs_y = row;
  if (updating_frame == 0)
    pm_display_cursor (selected_frame, 1);
}


static void PM_write_glyphs (GLYPH *start, int len)
{
  pm_request pmr;
  struct frame *f;

  f = updating_frame;
  if (f == 0)
    {
      f = selected_frame;
      curs_x = f->cursor_x;
      curs_y = f->cursor_y;
    }
  dump_glyphs (f, start, 1, curs_x + len, curs_x, curs_y, len, highlight);
  if (updating_frame == 0)
    {
      f->cursor_x += len;
      pm_display_cursor (f, 1);
      f->cursor_x -= len;
    }
  else
    curs_x += len;
}


static void PM_set_terminal_window (int n)
{
  if (updating_frame == 0)
    abort ();
  if (n <= 0 || n > updating_frame->height)
    flexlines = updating_frame->height;
  else
    flexlines = n;
}


static void PM_ins_del_lines (int vpos, int n)
{
  pm_request pmr;

  if (updating_frame == 0)
    abort ();
  if (vpos >= flexlines)
    return;
  pmr.lines.header.type = PMR_LINES;
  pmr.lines.header.frame = (unsigned long)updating_frame;
  pmr.lines.y = vpos;
  pmr.lines.max_y = flexlines;
  pmr.lines.count = n;
  pm_send (&pmr, sizeof (pmr));
}


static void PM_update_begin (FRAME_PTR f)
{
  if (f == 0)
    abort ();
  flexlines = f->height;
  highlight = 0;
  pm_display_cursor (f, 0);

  /* Borrowed from xterm.c. */

  if (f == FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_frame)
    {
      /* Don't do highlighting for mouse motion during the update.  */
      FRAME_X_DISPLAY_INFO (f)->mouse_face_defer = 1;

      /* If the frame needs to be redrawn,
	 simply forget about any prior mouse highlighting.  */
      if (FRAME_GARBAGED_P (f))
	FRAME_X_DISPLAY_INFO (f)->mouse_face_window = Qnil;

      if (!NILP (FRAME_X_DISPLAY_INFO (f)->mouse_face_window))
	{
	  int firstline, lastline, i;
	  struct window *w = XWINDOW (FRAME_X_DISPLAY_INFO (f)->mouse_face_window);

	  /* Find the first, and the last+1, lines affected by redisplay.  */
	  for (firstline = 0; firstline < f->height; firstline++)
	    if (FRAME_DESIRED_GLYPHS (f)->enable[firstline])
	      break;

	  lastline = f->height;
	  for (i = f->height - 1; i >= 0; i--)
	    {
	      if (FRAME_DESIRED_GLYPHS (f)->enable[i])
		break;
	      else
		lastline = i;
	    }

	  /* Can we tell that this update does not affect the window
	     where the mouse highlight is?  If so, no need to turn off.
	     Likewise, don't do anything if the frame is garbaged;
	     in that case, the FRAME_CURRENT_GLYPHS that we would use
	     are all wrong, and we will redisplay that line anyway.  */
	  if (! (firstline > (XFASTINT (w->top) + window_internal_height (w))
		 || lastline < XFASTINT (w->top)))
	    clear_mouse_face (FRAME_X_DISPLAY_INFO (f));
	}
    }
}


static void PM_update_end (struct frame *f)
{
  pm_display_cursor (f, 1);
  if (f == FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_frame)
    FRAME_X_DISPLAY_INFO (f)->mouse_face_defer = 0;
  updating_frame = 0;
}


static void PM_clear_frame ()
{
  pm_request pmr;
  struct frame *f;

  f = updating_frame;
  if (f == 0)
    f = selected_frame;
  f->phys_cursor_x = -1;
  curs_x = 0;
  curs_y = 0;
  pmr.header.type = PMR_CLEAR;
  pmr.header.frame = (unsigned long)f;
  pm_send (&pmr, sizeof (pmr));
}

static void PM_clear_end_of_line (int first_unused)
{
  pm_request pmr;
  struct frame *f = updating_frame;

  if (f == 0)
    abort ();

  if (curs_y < 0 || curs_y >= f->height)
    return;
  if (first_unused <= 0)
    return;
  if (first_unused >= f->width)
    first_unused = f->width;

  pmr.clreol.header.type = PMR_CLREOL;
  pmr.clreol.header.frame = (unsigned long)f;
  pmr.clreol.y = curs_y;
  pmr.clreol.x0 = curs_x;
  pmr.clreol.x1 = first_unused;
  pm_send (&pmr, sizeof (pmr));
}


/* This is called after a redisplay on frame F.  */

static void 
PM_frame_up_to_date (FRAME_PTR f)
{
  if (FRAME_X_DISPLAY_INFO (f)->mouse_face_deferred_gc
      || f == FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_frame)
    {
      note_mouse_highlight (FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_frame,
			    FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_x,
			    FRAME_X_DISPLAY_INFO (f)->mouse_face_mouse_y);
      FRAME_X_DISPLAY_INFO (f)->mouse_face_deferred_gc = 0;
    }
}


static void PM_reassert_line_highlight (int new, int vpos)
{
  highlight = new;
}


static void PM_change_line_highlight (int new_highlight, int vpos,
                                 int first_unused_hpos)
{
  highlight = new_highlight;
  PM_cursor_to (vpos, 0);
  PM_clear_end_of_line (updating_frame->width);
}


/* Let pmemacs ring the bell. */

void PM_ring_bell ()
{
  pm_request pmr;

  pmr.bell.header.type = PMR_BELL;
  pmr.bell.header.frame = (unsigned long)selected_frame;
  pmr.bell.visible = visible_bell;
  pm_send (&pmr, sizeof (pmr));
}


/* Return the current mouse position. */

static void PM_mouse_position (FRAME_PTR *f, int insist,
                               Lisp_Object *bar_window,
                               enum scroll_bar_part *part,
                               Lisp_Object *x, Lisp_Object *y,
                               unsigned long *time)
{
  Lisp_Object tail, frame;

  if (mouse_frame && mouse_frame->mouse_moved)
    {
      *f = mouse_frame;
      XSETINT (*x, mouse_frame->output_data.x->mouse_x);
      XSETINT (*y, mouse_frame->output_data.x->mouse_y);
      *bar_window = Qnil;
      *time = 0;
    }
  else
    {
      /* Probably we don't need this -- is PM_mouse_position ever
         called when mouse_moved is zero? */
      pmd_mousepos result;
      pm_request pmr;
      void *buf;

      BLOCK_INPUT;
      pmr.mousepos.header.type = PMR_MOUSEPOS;
      pmr.mousepos.header.frame = -1;
      pmr.mousepos.serial = pm_serial++;
      pm_send (&pmr, sizeof (pmr));
      buf = pm_receive (pmr.mousepos.serial, &result, NULL, 0);
      UNBLOCK_INPUT;
      if (buf != NULL && result.frame != 0)
        {
          *f = (FRAME_PTR)result.frame;
          XSETINT (*x, result.x);
          XSETINT (*y, result.y);
          *bar_window = Qnil;
          *time = 0;
        }
    }
  FOR_EACH_FRAME (tail, frame)
    XFRAME (frame)->mouse_moved = 0;
  mouse_frame = 0;
}


static Lisp_Object find_scroll_bar_window (FRAME_PTR f, unsigned id)
{
  Lisp_Object bar;

  for (bar = FRAME_SCROLL_BARS (f); !NILP (bar); bar = XSCROLL_BAR (bar)->next)
    if (XSCROLL_BAR (bar)->id == id)
      return XSCROLL_BAR (bar)->window;
  for (bar = FRAME_CONDEMNED_SCROLL_BARS (f); !NILP (bar);
       bar = XSCROLL_BAR (bar)->next)
    if (XSCROLL_BAR (bar)->id == id)
      return XSCROLL_BAR (bar)->window;
  return Qnil;
}


/* Scale scrollbar values to a range accepted by the PM (0 through
   32767).  Return 1 if changed.  */

static int pm_scroll_bar_scale (slider *s, struct scroll_bar *bar,
                                int portion, int whole, int position)
{
  int scaled_portion, scaled_whole, scaled_position;
  double scale;

  if (XINT (bar->portion) == portion && XINT (bar->whole) == whole
      && XINT (bar->position) == position)
    {
      s->portion = s->whole = s->position = -1;
      return 0;
    }

  XSETINT (bar->portion, portion);
  XSETINT (bar->whole, whole);
  XSETINT (bar->position, position);

  if (whole == 0)
    scaled_portion = scaled_whole = scaled_position = 0;
  else
    {
      scale = 32767.0 / whole;
      scaled_portion = (long)(scale * portion + 0.001);
      scaled_whole = 32767;
      scaled_position = (long)(scale * position + 0.001);

      /* Attempt to work around a bug in the PM: If `scaled_portion'
         is close to `scaled_portion', the slider becomes one pixel
         high.  The constant (35 below) depends on `scaled_whole'. */

      if (scaled_portion > scaled_whole - 35 && scaled_portion < scaled_whole)
        scaled_portion = scaled_whole - 35;
    }

  if (XINT (bar->scaled_portion) == scaled_portion
      && XINT (bar->scaled_whole) == scaled_whole
      && XINT (bar->scaled_position) == scaled_position)
    {
      s->portion = s->whole = s->position = -1;
      return 0;
    }

  XSETINT (bar->scaled_portion, scaled_portion);
  XSETINT (bar->scaled_whole, scaled_whole);
  XSETINT (bar->scaled_position, scaled_position);

  s->portion = scaled_portion;
  s->whole = scaled_whole;
  s->position = scaled_position;
  return 1;
}


/* Ask pmemacs to create a new scroll bar and return the scroll bar
   vector for it.  Originally written by Patrick Nadeau, modified by
   Eberhard Mattes.  */

static struct scroll_bar *
pm_scroll_bar_create (struct window *w, int portion, int whole, int position,
                      int top, int left, int width, int height)
{
  FRAME_PTR f;
  struct scroll_bar *bar;
  pm_request pmr;
  pm_create_scrollbar more;
  pmd_create_scrollbar answer;
  unsigned sb_id;

  if (!w) abort();
  f = XFRAME (WINDOW_FRAME (w));
  if (!f) abort();

  bar = XSCROLL_BAR (Fmake_vector (make_number (SCROLL_BAR_VEC_SIZE), Qnil));

  /* Initialize cached values.  */

  XSETINT (bar->top, top);
  XSETINT (bar->left, left);
  XSETINT (bar->width, width);
  XSETINT (bar->height, height);
  XSETINT (bar->whole, -1);
  XSETINT (bar->portion, -1);
  XSETINT (bar->position, -1);
  XSETINT (bar->scaled_whole, -1);
  XSETINT (bar->scaled_portion, -1);
  XSETINT (bar->scaled_position, -1);

  pm_scroll_bar_scale (&more.s, bar, portion, whole, position);

  more.top = top;
  more.left = left;
  more.width = width;
  more.height = height;
  more.serial = pm_serial++;

  BLOCK_INPUT;
  pmr.header.type = PMR_CREATE_SCROLLBAR;
  pmr.header.frame = (unsigned)f;
  pm_send_collect (&pmr, sizeof (pmr));
  pm_send_collect (&more, sizeof (more));
  pm_send_flush ();
  if (pm_receive (more.serial, &answer, NULL, 0) != NULL)
    sb_id = answer.id;
  else
    sb_id = SB_INVALID_ID;
  UNBLOCK_INPUT;

  if (sb_id != SB_INVALID_ID)
    {
      XSETINT (bar->id, sb_id);
      XSETWINDOW (bar->window, w);

      /* Add scroll bar to its frame's list of scroll bars.  */

      bar->next = FRAME_SCROLL_BARS (f);
      bar->prev = Qnil;

      XSETVECTOR (FRAME_SCROLL_BARS (f), bar);
      if (!NILP (bar->next))
	XSETVECTOR (XSCROLL_BAR (bar->next)->prev, bar);
    }
  else
    {
      fprintf (stderr, "Could not create the scroll bar\n");
      abort();
    }
  return bar;
}


/* Destroy the PM window for BAR, and set its Emacs window's scroll
   bar to nil.  Originally written by Patrick Nadeau, modified by
   Eberhard Mattes.  */

static void pm_scroll_bar_close (struct scroll_bar *bar)
{
  pm_request pmr;
  FRAME_PTR f;

  f = XFRAME (WINDOW_FRAME (XWINDOW (bar->window)));
  pmr.destroy_scrollbar.header.type = PMR_DESTROY_SCROLLBAR;
  pmr.destroy_scrollbar.header.frame = (unsigned)f;
  pmr.destroy_scrollbar.id = XINT (bar->id);
  pm_send (&pmr, sizeof (pmr));

  /* Disassociate this scroll bar from its window.  */

  XWINDOW (bar->window)->vertical_scroll_bar = Qnil;
}


/* Refresh scrollbar.  Originally written by Patrick Nadeau, modified
   by Eberhard Mattes.  */

pm_scroll_bar_move (struct scroll_bar *bar, int portion, int whole,
                    int position, int top, int left, int width, int height)
{
  pm_request pmr;
  FRAME_PTR f;

  f = XFRAME (WINDOW_FRAME (XWINDOW (bar->window)));

  if (XINT (bar->top) != top || XINT (bar->left) != left
      || XINT (bar->width) != width || XINT (bar->height) != height)
    {
      pmr.move_scrollbar.header.type = PMR_MOVE_SCROLLBAR;
      pmr.move_scrollbar.header.frame = (unsigned)f;
      pmr.move_scrollbar.id = XINT (bar->id);
      pmr.move_scrollbar.top = top;
      pmr.move_scrollbar.left = left;
      pmr.move_scrollbar.width = width;
      pmr.move_scrollbar.height = height;
      pm_send_collect (&pmr, sizeof (pmr));
      XSETINT (bar->top, top);
      XSETINT (bar->left, left);
      XSETINT (bar->width, width);
      XSETINT (bar->height, height);
    }
  if (pm_scroll_bar_scale (&pmr.update_scrollbar.s, bar,
                           portion, whole, position))
    {
      pmr.update_scrollbar.header.type = PMR_UPDATE_SCROLLBAR;
      pmr.update_scrollbar.header.frame = (unsigned)f;
      pmr.update_scrollbar.id = XINT (bar->id);
      pm_send_collect (&pmr, sizeof (pmr));
    }
  pm_send_flush ();
}


/* Set the vertical scroll bar for WINDOW to have its upper left
   corner at (TOP, LEFT), and be LENGTH rows high.  Set its handle to
   indicate that we are displaying PORTION characters out of a total
   of WHOLE characters, starting at POSITION.  If WINDOW doesn't yet
   have a scroll bar, create one for it.  Originally written by
   Patrick Nadeau, modified by Eberhard Mattes.  */

static void PM_set_vertical_scroll_bar (struct window *w, int portion,
                                        int whole, int position)
{
  FRAME_PTR f = XFRAME (WINDOW_FRAME (w));
  int top = XINT (w->top);
  int left = WINDOW_VERTICAL_SCROLL_BAR_COLUMN (w);
  int height = WINDOW_VERTICAL_SCROLL_BAR_HEIGHT (w);
  int width = FRAME_SCROLL_BAR_COLS (f);
  struct scroll_bar *bar;

  /* Does the scroll bar exist yet?  */

  if (NILP (w->vertical_scroll_bar))
    {
      bar = pm_scroll_bar_create (w, portion, whole, position,
                                  top, left, width, height);
      XSETVECTOR (w->vertical_scroll_bar, bar);
    }
  else
    {
      /* It may just need to be moved and resized.  */

      bar = XSCROLL_BAR (w->vertical_scroll_bar);
      pm_scroll_bar_move (bar, portion, whole, position,
                          top, left, width, height);
    }
}


/* [Code and comments borrowed from xterm.c] */

/* The following three hooks are used when we're doing a thorough
   redisplay of the frame.  We don't explicitly know which scroll bars
   are going to be deleted, because keeping track of when windows go
   away is a real pain - "Can you say set-window-configuration, boys
   and girls?"  Instead, we just assert at the beginning of redisplay
   that *all* scroll bars are to be removed, and then save a scroll bar
   from the fiery pit when we actually redisplay its window.  */

/* Arrange for all scroll bars on FRAME to be removed at the next call
   to `*judge_scroll_bars_hook'.  A scroll bar may be spared if
   `*redeem_scroll_bar_hook' is applied to its window before the judgement.  */

static void PM_condemn_scroll_bars (FRAME_PTR frame)
{
  /* The condemned list should be empty at this point; if it's not,
     then the rest of Emacs isn't using the condemn/redeem/judge
     protocol correctly.  */

  if (!NILP (FRAME_CONDEMNED_SCROLL_BARS (frame)))
    abort ();

  /* Move them all to the "condemned" list.  */
  FRAME_CONDEMNED_SCROLL_BARS (frame) = FRAME_SCROLL_BARS (frame);
  FRAME_SCROLL_BARS (frame) = Qnil;
}


/* Unmark WINDOW's scroll bar for deletion in this judgement cycle.
   Note that WINDOW isn't necessarily condemned at all.  */

static void PM_redeem_scroll_bar (struct window *window)
{
  struct scroll_bar *bar;

  /* We can't redeem this window's scroll bar if it doesn't have one.  */
  if (NILP (window->vertical_scroll_bar))
    abort ();

  bar = XSCROLL_BAR (window->vertical_scroll_bar);

  /* Unlink it from the condemned list.  */
  {
    FRAME_PTR f = XFRAME (WINDOW_FRAME (window));

    if (NILP (bar->prev))
      {
	/* If the prev pointer is nil, it must be the first in one of
           the lists.  */
	if (EQ (FRAME_SCROLL_BARS (f), window->vertical_scroll_bar))
	  /* It's not condemned.  Everything's fine.  */
	  return;
	else if (EQ (FRAME_CONDEMNED_SCROLL_BARS (f),
		     window->vertical_scroll_bar))
	  FRAME_CONDEMNED_SCROLL_BARS (f) = bar->next;
	else
	  /* If its prev pointer is nil, it must be at the front of
             one or the other!  */
	  abort ();
      }
    else
      XSCROLL_BAR (bar->prev)->next = bar->next;

    if (! NILP (bar->next))
      XSCROLL_BAR (bar->next)->prev = bar->prev;

    bar->next = FRAME_SCROLL_BARS (f);
    bar->prev = Qnil;
    XSETVECTOR (FRAME_SCROLL_BARS (f), bar);
    if (! NILP (bar->next))
      XSETVECTOR (XSCROLL_BAR (bar->next)->prev, bar);
  }
}

/* Remove all scroll bars on FRAME that haven't been saved since the
   last call to `*condemn_scroll_bars_hook'.  */

static void PM_judge_scroll_bars (FRAME_PTR f)
{
  Lisp_Object bar, next;

  bar = FRAME_CONDEMNED_SCROLL_BARS (f);

  /* Clear out the condemned list now so we won't try to process any
     more events on the hapless scroll bars.  */
  FRAME_CONDEMNED_SCROLL_BARS (f) = Qnil;

  for (; ! NILP (bar); bar = next)
    {
      struct scroll_bar *b = XSCROLL_BAR (bar);

      pm_scroll_bar_close (b);

      next = b->next;
      b->next = b->prev = Qnil;
    }

  /* Now there should be no references to the condemned scroll bars,
     and they should get garbage-collected.  */
}


void x_destroy_window (struct frame *f)
{
  struct x_display_info *dpyinfo = FRAME_X_DISPLAY_INFO (f);
  pm_request pmr;

  dpyinfo->reference_count--;

  if (f == dpyinfo->mouse_face_mouse_frame)
    {
      dpyinfo->mouse_face_beg_row
	= dpyinfo->mouse_face_beg_col = -1;
      dpyinfo->mouse_face_end_row
	= dpyinfo->mouse_face_end_col = -1;
      dpyinfo->mouse_face_window = Qnil;
    }

#if 0
  /* This is not required as no resources were allocated for the
     menubar in Emacs proper. */
  free_frame_menubar (f);
#endif
  pm_del_frame (f);
  pmr.header.type = PMR_DESTROY;
  pmr.header.frame = (unsigned long)f;
  pm_send (&pmr, sizeof (pmr));
  free_frame_faces (f);
}


void x_set_mouse_position (struct frame *f, int x, int y)
{
  /*TODO*/
}


/* Move the mouse to position pixel PIX_X, PIX_Y relative to frame F.  */

void x_set_mouse_pixel_position (struct frame *f, int pix_x, int pix_y)
{
  /*TODO*/
}


static void PM_frame_raise_lower (struct frame *f, int raise_flag)
{
  pm_request pmr;

  pmr.header.type = (raise_flag ? PMR_RAISE : PMR_LOWER);
  pmr.header.frame = (unsigned long)f;
  pm_send (&pmr, sizeof (pmr));
}


void x_make_frame_visible (struct frame *f)
{
  pm_request pmr;

  if (!FRAME_VISIBLE_P (f))
    {
      pmr.visible.header.type = PMR_VISIBLE;
      pmr.visible.header.frame = (unsigned long)f;
      pmr.visible.visible = 1;
      pm_send (&pmr, sizeof (pmr));
      f->async_visible = 1;
      f->async_iconified = 0;
    }
}


void x_make_frame_invisible (struct frame *f)
{
  pm_request pmr;

  if (!f->async_visible)
    return;
  f->async_visible = 0;
  pmr.visible.header.type = PMR_VISIBLE;
  pmr.visible.header.frame = (unsigned long)f;
  pmr.visible.visible = 0;
  pm_send (&pmr, sizeof (pmr));
}


void x_iconify_frame (struct frame *f)
{
  pm_request pmr;

  f->async_visible = 0;
  f->async_iconified = 1;
  pmr.header.type = PMR_ICONIFY;
  pmr.header.frame = (unsigned long)f;
  pm_send (&pmr, sizeof (pmr));
}


void x_set_window_size (struct frame *f, int change_gravity,
                        int cols, int rows)
{
  pm_request pmr;

  check_frame_size (f, &rows, &cols);
  change_frame_size (f, rows, cols, 0, 0);
  pmr.size.header.type = PMR_SIZE;
  pmr.size.header.frame = (unsigned long)f;
  pmr.size.width = cols;
  pmr.size.height = rows;
  pm_send (&pmr, sizeof (pmr));

  /* If cursor was outside the new size, mark it as off.  */
  if (f->phys_cursor_y >= rows
      || f->phys_cursor_x >= cols)
    {
      f->phys_cursor_x = -1;
      f->phys_cursor_y = -1;
    }
  SET_FRAME_GARBAGED (f);
}


void x_set_offset (struct frame *f, int xoff, int yoff, int change_gravity)
{
  pm_request pmr;

  pmr.setpos.header.type = PMR_SETPOS;
  pmr.setpos.header.frame = (unsigned long)f;
  if (xoff >= 0)
    {
      pmr.setpos.left = xoff;
      pmr.setpos.left_base = 1;
    }
  else
    {
      pmr.setpos.left = -xoff;
      pmr.setpos.left_base = -1;
    }
  if (yoff >= 0)
    {
      pmr.setpos.top = yoff;
      pmr.setpos.top_base = 1;
    }
  else
    {
      pmr.setpos.top = -yoff;
      pmr.setpos.top_base = -1;
    }
  pm_send (&pmr, sizeof (pmr));
}


void x_focus_on_frame (struct frame *f)
{
  pm_request pmr;

  pmr.header.type = PMR_FOCUS;
  pmr.header.frame = (unsigned long)f;
  pm_send (&pmr, sizeof (pmr));
}


void pm_init (void)
{
    int i, pipe_in[2], pipe_out[2];
    char arg1[12], arg2[12], arg3[12], *debugger;
    pm_request pmr;
    struct x_display_info *dpyinfo;
    Lisp_Object prog;

    mouse_frame = 0;

    for (i = 0; i < 64; i++)
        fcntl (i, F_SETFD, 1);

    sprintf (arg1, "%d", getpid ());

    if (pipe (pipe_out) != 0)
        fatal ("Cannot open pipe for PM Emacs.");
    fcntl (pipe_out[1], F_SETFD, 1); /* Don't pass write end to child */
    outbound_pipe = pipe_out[1];
    setmode (outbound_pipe, O_BINARY);
    sprintf (arg2, "%d", pipe_out[0]);

    if (pipe (pipe_in) != 0)
        fatal ("Cannot open pipe for PM Emacs.");
    dup2 (pipe_in[0], 0);         /* Replace stdin with pipe */
    close (pipe_in[0]);
    fcntl (0, F_SETFD, 1);        /* Don't pass read end to child */
    setmode (0, O_BINARY);
    sprintf (arg3, "%d", pipe_in[1]);

    prog = concat2 (Vinvocation_directory, build_string ("pmemacs.exe"));
    debugger = getenv ("PMEMACS_DEBUGGER");
    if (debugger != NULL)
        pm_pid = spawnl (P_PM, debugger, debugger,
                         XSTRING (prog)->data, arg1, arg2, arg3, 0);
    else
        pm_pid = spawnl (P_PM, XSTRING (prog)->data, "pmemacs.exe",
                         arg1, arg2, arg3, 0);
    if (pm_pid == -1)
        fatal ("Cannot start PM Emacs.");

    close (pipe_out[0]);
    close (pipe_in[1]);
    pm_session_started = 1;

    pmr.initialize.header.type = PMR_INITIALIZE;
    pmr.initialize.header.frame = 0;
    pmr.initialize.codepage = cur_code_page != 0 ? cur_code_page : 850;
    pm_send (&pmr, sizeof (pmr));

    clear_frame_hook = PM_clear_frame;
    clear_end_of_line_hook = PM_clear_end_of_line;
    ins_del_lines_hook = PM_ins_del_lines;
    set_terminal_window_hook = PM_set_terminal_window;
#if 0
    insert_glyphs_hook = PM_insert_glyphs;
    delete_glyphs_hook = PM_delete_glyphs;
#endif
    frame_raise_lower_hook = PM_frame_raise_lower;
    change_line_highlight_hook = PM_change_line_highlight;
    reassert_line_highlight_hook = PM_reassert_line_highlight;
    mouse_position_hook = PM_mouse_position;
    write_glyphs_hook = PM_write_glyphs;
    ring_bell_hook = PM_ring_bell;
    set_terminal_modes_hook = PM_set_terminal_modes;
    update_begin_hook = PM_update_begin;
    update_end_hook = PM_update_end;
    read_socket_hook = PM_read_socket;
    cursor_to_hook = PM_cursor_to;
    frame_up_to_date_hook = PM_frame_up_to_date;

    set_vertical_scroll_bar_hook  = PM_set_vertical_scroll_bar;
    judge_scroll_bars_hook = PM_judge_scroll_bars;
    redeem_scroll_bar_hook = PM_redeem_scroll_bar;
    condemn_scroll_bars_hook = PM_condemn_scroll_bars;

    scroll_region_ok = 1;		/* we'll scroll partial frames */
    char_ins_del_ok = 0;		/* just as fast to write the line */
    line_ins_del_ok = 1;		/* use GpiBitBlt */
    fast_clear_end_of_line = 1;	/* PM does this well */
    memory_below_frame = 0;	/* we don't remember what scrolls
                               off the bottom */
    baud_rate = 19200;

    /* Get the number of planes. */
    BLOCK_INPUT;
    pmr.config.header.type = PMR_CONFIG;
    pmr.config.header.frame = 0;
    pmr.config.serial = pm_serial++;
    pm_send (&pmr, sizeof (pmr));
    if (pm_receive (pmr.config.serial, &pm_config, NULL, 0) == NULL)
    {
        pm_config.planes = 1;
        pm_config.color_cells = 2;
        pm_config.width_mm = 1;
        pm_config.height_mm = 1;
    }
    UNBLOCK_INPUT;

    signal (SIGPIPE, pm_sigpipe);

    dpyinfo = (struct x_display_info *) xmalloc (sizeof (struct x_display_info));
    dpyinfo->reference_count = 0;
    dpyinfo->grabbed = 0;
    dpyinfo->mouse_face_mouse_frame = 0;
    dpyinfo->mouse_face_deferred_gc = 0;
    dpyinfo->mouse_face_beg_row = dpyinfo->mouse_face_beg_col = -1;
    dpyinfo->mouse_face_end_row = dpyinfo->mouse_face_end_col = -1;
    dpyinfo->mouse_face_face_id = 0;
    dpyinfo->mouse_face_window = Qnil;
    dpyinfo->mouse_face_mouse_x = dpyinfo->mouse_face_mouse_y = 0;
    dpyinfo->mouse_face_defer = 0;

    pm_display = dpyinfo;
}


void pm_exit (void)
{
  pm_request pmr;
  pm_event pme;

  pmr.header.type = PMR_CLOSE;
  pmr.header.frame = 0;
  pm_send (&pmr, sizeof (pmr));

  /* Wait until the pipe is closed, that is, until pmemacs.exe is
     dead.  Why use waitpid() if we can do without? :-) */

  while (receive (&pme, sizeof (pme), 1))
    if (pme.header.type == PME_ANSWER && pme.answer.size > 0)
      {
        char *tem = (char *)xmalloc (pme.answer.size);
        if (!receive (tem, pme.answer.size, 1))
          break;
        xfree (tem);
      }

  pm_session_started = 0;
  close (outbound_pipe);
  close (0);
}


void pm_shutdown (void)
{
  if (pm_session_started)
    pm_exit ();
}


void syms_of_xterm ()
{
}


void x_handle_selection_request (struct input_event *event)
{
}

void x_handle_selection_clear (struct input_event *event)
{
}


static XFontStruct *pm_fonts = 0;


XFontStruct *XLoadQueryFont (Display *dpy, char *name)
{
  XFontStruct *p;

  for (p = pm_fonts; p != 0; p = p->next)
    if (strcmp (p->name, name) == 0)
      return p;
  p = (XFontStruct *)xmalloc (sizeof (*p));
  bzero (p, sizeof (XFontStruct));
  p->max_bounds.width = 1;      /* TODO */
  p->ascent = 1;                /* TODO */
  p->descent = 0;               /* TODO */
  _strncpy (p->name, name, sizeof (p->name));
  p->next = pm_fonts;
  pm_fonts = p;
  return p;
}


int XParseColor (Display *dpy, Colormap cmap, char *name, XColor *color)
{
  return defined_color (NULL, name, color, 0);
}
