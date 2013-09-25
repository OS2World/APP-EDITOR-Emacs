head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	2000.10.21.20.43.53;	author JeremyB;	state Exp;
branches;
next	;


desc
@OS/2 Emacs 20.6
@


1.1
log
@Initial revision
@
text
@/* pmfns.c -- xfns.c for the OS/2 Presentation Manager
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

#include <stdio.h>
#include "config.h"
#include "lisp.h"
#include "pmlib.h"
#include "pmterm.h"
#include "pmemacs.h"
#include "frame.h"
#include "window.h"
#include "dispextern.h"
#include "keyboard.h"
#include "blockinput.h"
#include <epaths.h>
#include "termhooks.h"

extern pmd_config pm_config;
extern int pm_session_started;

extern Lisp_Object Qheight, Qicon, Qmenu_bar_lines, Qminibuffer, Qname;
extern Lisp_Object Qonly, Qunsplittable, Qunderline, Qwidth;


/* The background and shape of the mouse pointer, and shape when not
   over text or in the modeline.  */
Lisp_Object Vx_pointer_shape;

/* Search path for bitmap files.  */
Lisp_Object Vx_bitmap_file_path;


Lisp_Object Qalt;
Lisp_Object Qalt_f4;
Lisp_Object Qalt_f5;
Lisp_Object Qalt_f6;
Lisp_Object Qalt_f7;
Lisp_Object Qalt_f8;
Lisp_Object Qalt_f9;
Lisp_Object Qalt_f10;
Lisp_Object Qalt_f11;
Lisp_Object Qalt_modifier;
Lisp_Object Qalt_space;
Lisp_Object Qaltgr;
Lisp_Object Qaltgr_modifier;
Lisp_Object Qbackground_color;
Lisp_Object Qbar;
Lisp_Object Qborder_color;
Lisp_Object Qborder_width;
Lisp_Object Qbox;
Lisp_Object Qcursor_blink;
Lisp_Object Qcursor_type;
Lisp_Object Qdisplay;
Lisp_Object Qdown;
Lisp_Object Qf1;
Lisp_Object Qf10;
Lisp_Object Qforeground_color;
Lisp_Object Qframe;
Lisp_Object Qhalftone;
Lisp_Object Qhyper;
Lisp_Object Qleft;
Lisp_Object Qright;
Lisp_Object Qmenu_font;
Lisp_Object Qmeta;
Lisp_Object Qmouse_1;
Lisp_Object Qmouse_2;
Lisp_Object Qmouse_3;
Lisp_Object Qmouse_buttons;
Lisp_Object Qnone;
Lisp_Object Qparent_id;
Lisp_Object Qscroll_bar_width;
Lisp_Object Qshortcuts;
Lisp_Object Qsuper;
Lisp_Object Qtop;
Lisp_Object Qvisibility;
Lisp_Object Qvar_width_fonts;
Lisp_Object Qvertical_scroll_bars;
Lisp_Object Qvisibility;
Lisp_Object Qwindow_id;
Lisp_Object Qx_frame_parameter;
Lisp_Object Qx_resource_name;
Lisp_Object Quser_position;
Lisp_Object Quser_size;
Lisp_Object Qdisplay;

Lisp_Object Vpm_color_alist;


void x_set_frame_parameters (struct frame *f, Lisp_Object alist);
void x_set_name (struct frame *f, Lisp_Object name, int explicit);

/* Nonzero if we can use mouse menus.
   You should not call this unless HAVE_MENUS is defined.  */

int
have_menus_p ()
{
  return pm_session_started;
}


void
check_x ()
{
  if (!pm_session_started)
    error ("PM Emacs not in use or not initialized");
}

/* Let the user specify an X display with a frame.
   nil stands for the selected frame--or, if that is not an X frame,
   the first X display on the list.  */

static struct x_display_info *
check_pm_display_info (frame)
     Lisp_Object frame;
{
  if (NILP (frame))
    {
      if (FRAME_X_P (selected_frame))
	return FRAME_X_DISPLAY_INFO (selected_frame);
      else if (pm_session_started)
	return pm_display;
      else
	error ("PM windows are not in use or not initialized");
    }
  else if (STRINGP (frame))
    return pm_display;
  else
    {
      FRAME_PTR f;

      CHECK_LIVE_FRAME (frame, 0);
      f = XFRAME (frame);
      if (! FRAME_X_P (f))
	error ("non-PM frame used");
      return FRAME_X_DISPLAY_INFO (f);
    }
}


void
free_frame_menubar (FRAME_PTR f)
{
  pm_request pmr;
  pm_menu pmm;

  pmr.menubar.header.type = PMR_MENUBAR;
  pmr.menubar.header.frame = (unsigned long)f;
  pmr.menubar.entries = 1;
  pmr.menubar.size = sizeof (pmm);
  pmm.type = PMMENU_END;
  pm_send (&pmr, sizeof (pmr));
  pm_send (&pmm, sizeof (pmm));
}


int
defined_color (f, color, color_def, alloc)
     FRAME_PTR f;
     char *color;
     XColor *color_def;
     int alloc;
{
  Lisp_Object tem;
  int r, g, b;
  char *name, *p;

  if (color[0] == '#')
    {
      int n;
      unsigned rgb = 0;

      ++color;
      for (n = 0; n < 6; ++n)
        {
          rgb <<= 4;
          if (*color >= '0' && *color <= '9')
            rgb |= *color - '0';
          else if (*color >= 'a' && *color <= 'f')
            rgb |= *color - 'a' + 10;
          else if (*color >= 'A' && *color <= 'F')
            rgb |= *color - 'A' + 10;
          else
            return 0;
          ++color;
        }
      if (*color != 0)
        return 0;
      color_def->pixel = rgb;
      return 1;
    }
  if (strnicmp (color, "rgbi:", 5) == 0)
    {
      double fr, fg, fb;
      int n;

      if (sscanf (color + 5, "%lf/%lf/%lf%n", &fr, &fg, &fb, &n) != 3
          || n != strlen (color + 5)
          || fr < 0.0 || fg < 0.0 || fb < 0.0
          || fr > 1.0 || fg > 1.0 || fb > 1.0)
        return 0;
      r = (int)(fr * 255.0);
      g = (int)(fg * 255.0);
      b = (int)(fb * 255.0);
      color_def->pixel = (r << 16) | (g << 8) | b;
      return 1;
    }
  name = alloca (strlen (color) + 1);
  for (p = name; *color != 0; ++color)
    if (*color != ' ')
      *p++ = *color;
  *p = 0;
  tem = Fassoc (Fdowncase (build_string (name)), Vpm_color_alist);
  if (CONSP (tem))
    {
      tem = Fcdr (tem);
      if (VECTORP (tem) && XVECTOR (tem)->size == 3
          && INTEGERP (XVECTOR (tem)->contents[0])
          && INTEGERP (XVECTOR (tem)->contents[1])
          && INTEGERP (XVECTOR (tem)->contents[2]))
        {
          r = XINT (XVECTOR (tem)->contents[0]);
          g = XINT (XVECTOR (tem)->contents[1]);
          b = XINT (XVECTOR (tem)->contents[2]);
          if (!((r & ~0xff) || (g & ~0xff) || (b & ~0xff)))
            {
              color_def->pixel = (r << 16) | (g << 8) | b;
              return 1;
            }
        }
    }
  return 0;
}


static void pm_get_framepos (FRAME_PTR f)
{
  pm_request pmr;
  pmd_framepos answer;

  BLOCK_INPUT;
  pmr.framepos.header.type = PMR_FRAMEPOS;
  pmr.framepos.header.frame = (unsigned long)f;
  pmr.framepos.serial = pm_serial++;
  pm_send (&pmr, sizeof (pmr));
  if (pm_receive (pmr.framepos.serial, &answer, NULL, 0) != NULL)
    {
      f->output_data.x->left_pos = answer.left;
      f->output_data.x->top_pos = answer.top;
      f->output_data.x->pixel_height = answer.pix_height;
      f->output_data.x->pixel_width = answer.pix_width;
    }
  UNBLOCK_INPUT;
}


int x_set_menu_bar_lines (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  int nlines;
  int olines = FRAME_MENU_BAR_LINES (f);

  if (FRAME_MINIBUF_ONLY_P (f))
    return;

  if (INTEGERP (arg))
    nlines = XINT (arg);
  else
    nlines = 0;

#ifdef USE_X_TOOLKIT
  FRAME_MENU_BAR_LINES (f) = 0;
  if (nlines)
    FRAME_EXTERNAL_MENU_BAR (f) = 1;
  else
    {
      if (FRAME_EXTERNAL_MENU_BAR (f) == 1)
	free_frame_menubar (f);
      FRAME_EXTERNAL_MENU_BAR (f) = 0;
    }
#else /* not USE_X_TOOLKIT */
  FRAME_MENU_BAR_LINES (f) = nlines;
  x_set_menu_bar_lines_1 (f->root_window, nlines - olines);
  x_set_window_size (f, 0, FRAME_WIDTH (f),
                     FRAME_HEIGHT (f) + nlines - olines);
#endif /* not USE_X_TOOLKIT */
  return (1);
}


static Lisp_Object pm_get_arg (Lisp_Object alist, Lisp_Object param)
{
  Lisp_Object tem;

  tem = Fassq (param, alist);
  if (EQ (tem, Qnil))
    tem = Fassq (param, Vdefault_frame_alist);
  if (EQ (tem, Qnil))
    return Qunbound;
  return Fcdr (tem);
}


/* Record in frame F the specified or default value according to ALIST
   of the parameter named PARAM (a Lisp symbol).  */

static Lisp_Object
pm_default_parameter (f, alist, prop, deflt)
     struct frame *f;
     Lisp_Object alist;
     Lisp_Object prop;
     Lisp_Object deflt;
{
  Lisp_Object tem;

  tem = pm_get_arg (alist, prop);
  if (EQ (tem, Qunbound))
    tem = deflt;
  x_set_frame_parameters (f, Fcons (Fcons (prop, tem), Qnil));
  return tem;
}


static int pm_set_name (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  char *p1, *p2;
  long n;

  x_set_name (f, arg, 1);
  return (1);
}


static int pm_set_font (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  Lisp_Object bar;

  CHECK_STRING (arg, 1);

  if (XSTRING (arg)->size > 0
      && XSTRING (arg)->size < sizeof (dst->default_font))
    {
      f->output_data.x->font = XLoadQueryFont (FRAME_X_DISPLAY (f),
                                               XSTRING (arg)->data);
      if (f->output_data.x->font == NULL)
        error ("Font `%s' is not defined", XSTRING (arg)->data);
      strcpy (dst->default_font, XSTRING (arg)->data);

      /* Invalidate the position cache of all scrollbars of this
         frame.  This will reposition the scrollbars on the next
         redraw. */

      for (bar = FRAME_SCROLL_BARS (f); !NILP (bar);
           bar = XSCROLL_BAR (bar)->next)
        XSETINT (XSCROLL_BAR (bar)->width, -1);
      for (bar = FRAME_CONDEMNED_SCROLL_BARS (f); !NILP (bar);
           bar = XSCROLL_BAR (bar)->next)
        XSETINT (XSCROLL_BAR (bar)->width, -1);

      return 1;
    }
  error ("Font `%s' is not defined", XSTRING (arg)->data);
  return 0;
}


static int pm_set_menu_font (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  if (NILP (arg))
    {
      dst->menu_font[0] = 0;
      dst->menu_font_set = 1;
      return 1;
    }
  else if (STRINGP (arg) && XSTRING (arg)->size > 0
      && XSTRING (arg)->size < sizeof (dst->menu_font))
    {
      strcpy (dst->menu_font, XSTRING (arg)->data);
      dst->menu_font_set = 1;
      return 1;
    }
  return 0;
}


static int pm_set_visibility (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  Lisp_Object frame;

  XSETFRAME (frame, f);
  if (NILP (arg))
    Fmake_frame_invisible (frame, Qt);
  else if (EQ (arg, Qicon))
    Ficonify_frame (frame);
  else
    Fmake_frame_visible (frame);
}


static int pm_set_cursor_type (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  if (EQ (arg, Qbox))
    dst->cursor_type = CURSORTYPE_BOX;
  else if (EQ (arg, Qbar))
    {
      dst->cursor_type = CURSORTYPE_BAR;
      dst->cursor_width = 0;
    }
  else if (EQ (arg, Qframe))
    dst->cursor_type = CURSORTYPE_FRAME;
  else if (EQ (arg, Qunderline))
    dst->cursor_type = CURSORTYPE_UNDERLINE;
  else if (EQ (arg, Qhalftone))
    dst->cursor_type = CURSORTYPE_HALFTONE;
  else if (CONSP (arg) && EQ (XCONS (arg)->car, Qbar)
           && INTEGERP (XCONS (arg)->cdr))
    {
      dst->cursor_type = CURSORTYPE_BAR;
      dst->cursor_width = XINT (XCONS (arg)->cdr);
    }
  else
    return (0);
  return (1);
}


static int pm_set_cursor_blink (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  dst->cursor_blink = (NILP (arg) ? PMR_FALSE : PMR_TRUE);
  return (1);
}


static int pm_set_color (FRAME_PTR f, int *dst, Lisp_Object arg)
{
  XColor color;

  if (STRINGP (arg) && defined_color (f, XSTRING (arg)->data, &color, 0))
    {
      *dst = color.pixel;
      recompute_basic_faces (f);
      if (FRAME_VISIBLE_P (f))
        redraw_frame (f);
      return (1);
    }
  return (0);
}


static int pm_set_foreground_color (FRAME_PTR f, pm_modify *dst,
                                    Lisp_Object arg)
{
  return (pm_set_color (f, &f->output_data.x->foreground_color, arg));
}


static int pm_set_background_color (FRAME_PTR f, pm_modify *dst,
                                    Lisp_Object arg)
{
  int ok;

  ok = pm_set_color (f, &f->output_data.x->background_color, arg);
  if (ok)
    dst->background_color = f->output_data.x->background_color;
  return ok;
}


static int pm_set_modifier (int *dst, Lisp_Object arg)
{
  if (EQ (arg, Qalt))
    *dst = alt_modifier;
  else if (EQ (arg, Qmeta))
    *dst = meta_modifier;
  else if (EQ (arg, Qsuper))
    *dst = super_modifier;
  else if (EQ (arg, Qhyper))
    *dst = hyper_modifier;
  else
    return (0);
  return (1);
}


static int pm_set_alt_modifier (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  return (pm_set_modifier (&dst->alt_modifier, arg));
}


static int pm_set_altgr_modifier (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  return (pm_set_modifier (&dst->altgr_modifier, arg));
}


static int pm_set_shortcuts (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{

  if (EQ (arg, Qt))
    dst->shortcuts = ~0;
  else if (NILP (arg) || CONSP (arg))
    {
      Lisp_Object elt;

      dst->shortcuts = SHORTCUT_SET;
      while (CONSP (arg))
        {
          elt = XCONS (arg)->car;
          if (EQ (elt, Qalt))
            dst->shortcuts |= SHORTCUT_ALT;
          else if (EQ (elt, Qaltgr))
            dst->shortcuts |= SHORTCUT_ALTGR;
          else if (EQ (elt, Qf1))
            dst->shortcuts |= SHORTCUT_F1;
          else if (EQ (elt, Qf10))
            dst->shortcuts |= SHORTCUT_F10;
          else if (EQ (elt, Qalt_f4))
            dst->shortcuts |= SHORTCUT_ALT_F4;
          else if (EQ (elt, Qalt_f5))
            dst->shortcuts |= SHORTCUT_ALT_F5;
          else if (EQ (elt, Qalt_f6))
            dst->shortcuts |= SHORTCUT_ALT_F6;
          else if (EQ (elt, Qalt_f7))
            dst->shortcuts |= SHORTCUT_ALT_F7;
          else if (EQ (elt, Qalt_f8))
            dst->shortcuts |= SHORTCUT_ALT_F8;
          else if (EQ (elt, Qalt_f9))
            dst->shortcuts |= SHORTCUT_ALT_F9;
          else if (EQ (elt, Qalt_f10))
            dst->shortcuts |= SHORTCUT_ALT_F10;
          else if (EQ (elt, Qalt_f11))
            dst->shortcuts |= SHORTCUT_ALT_F11;
          else if (EQ (elt, Qalt_space))
            dst->shortcuts |= SHORTCUT_ALT_SPACE;
          else
            {
              dst->shortcuts = 0;
              return 0;
            }
          arg = XCONS (arg)->cdr;
        }
    }
  else
    return 0;
  return 1;
}


static int pm_set_mouse_buttons (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  char *p;
  int i;

  if (STRINGP (arg) && XSTRING (arg)->size == 3)
    {
      p = XSTRING (arg)->data;
      for (i = 0; i < 3; ++i)
        if (!((p[i] >= '1' && p[i] <= '3') || p[i] == ' '))
          return (0);
      memcpy (dst->buttons, p, 3);
      return (1);
    }
  return (0);
}


static int pm_set_width (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  if (INTEGERP (arg) && XINT (arg) > 0)
    {
      dst->width = XINT (arg);
      return (1);
    }
  return (0);
}


static int pm_set_height (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  if (INTEGERP (arg) && XINT (arg) > 0)
    {
      dst->height = XINT (arg);
      return (1);
    }
  return (0);
}


static int pm_set_top_left (FRAME_PTR f, int *offset, int *base,
                            Lisp_Object arg)
{
  if (INTEGERP (arg))
    {
      *offset = XINT (arg);
      *base = XINT (arg) >= 0 ? 1 : -1;
      return 1;
    }
  else if (EQ (arg, Qminus))
    {
      *offset = 0;
      *base = -1;
      return 1;
    }
  else if (CONSP (arg) && EQ (XCONS (arg)->car, Qminus)
           && CONSP (XCONS (arg)->cdr)
           && INTEGERP (XCONS (XCONS (arg)->cdr)->car))
    {
      *offset = - XINT (XCONS (XCONS (arg)->cdr)->car);
      *base = -1;
      return 1;
    }
  else if (CONSP (arg) && EQ (XCONS (arg)->car, Qplus)
           && CONSP (XCONS (arg)->cdr)
           && INTEGERP (XCONS (XCONS (arg)->cdr)->car))
    {
      *offset = XINT (XCONS (XCONS (arg)->cdr)->car);
      *base = 1;
      return 1;
    }
  return 0;
}


static int pm_set_top (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  return pm_set_top_left (f, &dst->top, &dst->top_base, arg);
}


static int pm_set_left (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  return pm_set_top_left (f, &dst->left, &dst->left_base, arg);
}


static int pm_set_unsplittable (FRAME_PTR f, pm_modify *dst, Lisp_Object arg)
{
  f->no_split = !NILP (arg);
  return (1);
}


/* Handle `vertical-scroll-bar' frame parameter.  Originally written
   by Patrick Nadeau, modified by Eberhard Mattes. Jeremy Bowen 1999 */

static int pm_set_vertical_scroll_bars (FRAME_PTR f, pm_modify *dst,
                                        Lisp_Object arg)
{
    /* =================================================
       if (FRAME_CAN_HAVE_SCROLL_BARS (f))
       {
       FRAME_HAS_VERTICAL_SCROLL_BARS (f) = !NILP (arg);
       =======================================================*/
    if ((EQ (arg, Qleft) && FRAME_HAS_VERTICAL_SCROLL_BARS_ON_RIGHT (f))
        || (EQ (arg, Qright) && FRAME_HAS_VERTICAL_SCROLL_BARS_ON_LEFT (f))
        || (NILP (arg) && FRAME_HAS_VERTICAL_SCROLL_BARS (f))
        || (!NILP (arg) && ! FRAME_HAS_VERTICAL_SCROLL_BARS (f)))
    {
        FRAME_VERTICAL_SCROLL_BAR_TYPE (f) = (NILP (arg) ?
            vertical_scroll_bar_none :
            /* Put scroll bars on the right by default, as is conventional
               on OS/2.  */
            EQ (Qleft, arg)
            ? vertical_scroll_bar_left 
            : vertical_scroll_bar_right);


        /*===================================================*/
        if (FRAME_HAS_VERTICAL_SCROLL_BARS (f))
            dst->sb_width = FRAME_SCROLL_BAR_COLS (f);
        else
            dst->sb_width = 0;
    }
    return (1);
}


static int pm_set_scroll_bar_width (FRAME_PTR f, pm_modify *dst,
                                    Lisp_Object arg)
{
  if (NILP (arg))
    {
      FRAME_SCROLL_BAR_PIXEL_WIDTH (f) = 0;
      FRAME_SCROLL_BAR_COLS (f) = 2;
    }
  else
    return (0);
  if (FRAME_HAS_VERTICAL_SCROLL_BARS (f))
    dst->sb_width = FRAME_SCROLL_BAR_COLS (f);
  return (1);
}


static int pm_set_var_width_fonts (FRAME_PTR f, pm_modify *dst,
                                   Lisp_Object arg)
{
  dst->var_width_fonts = (NILP (arg) ? PMR_FALSE : PMR_TRUE);
  return (1);
}


struct pm_frame_parm_table
{
  char *name;
  int (*setter)(FRAME_PTR f, pm_modify *dst, Lisp_Object arg);
  int set;
  Lisp_Object obj;
};


static struct pm_frame_parm_table pm_frame_parms[] =
{
  {"width",                      pm_set_width, 0, 0},
  {"height",                     pm_set_height, 0, 0},
  {"top",                        pm_set_top, 0, 0},
  {"left",                       pm_set_left, 0, 0},
  {"cursor-blink",               pm_set_cursor_blink, 0, 0},
  {"cursor-type",                pm_set_cursor_type, 0, 0},
  {"font",                       pm_set_font, 0, 0},
  {"menu-font",                  pm_set_menu_font, 0, 0},
  {"foreground-color",           pm_set_foreground_color, 0, 0},
  {"background-color",           pm_set_background_color, 0, 0},
  {"name",                       pm_set_name, 0, 0},
  {"alt-modifier",               pm_set_alt_modifier, 0, 0},
  {"altgr-modifier",             pm_set_altgr_modifier, 0, 0},
  {"mouse-buttons",              pm_set_mouse_buttons, 0, 0},
  {"shortcuts",                  pm_set_shortcuts, 0, 0},
  {"vertical-scroll-bars",       pm_set_vertical_scroll_bars, 0, 0},
  {"visibility",                 pm_set_visibility, 0, 0},
  {"menu-bar-lines",             x_set_menu_bar_lines, 0, 0},
  {"scroll-bar-width",           pm_set_scroll_bar_width, 0, 0},
  {"unsplittable",               pm_set_unsplittable, 0, 0},
  {"var-width-fonts",            pm_set_var_width_fonts, 0, 0}
};


static void init_pm_parm_symbols (void)
{
  int i;

  for (i = 0; i < sizeof (pm_frame_parms) / sizeof (pm_frame_parms[0]); i++)
    pm_frame_parms[i].obj = intern (pm_frame_parms[i].name);
}


void x_report_frame_params (struct frame *f, Lisp_Object *alistptr)
{
  Lisp_Object tem;

  /* Represent negative positions (off the top or left screen edge)
     in a way that Fmodify_frame_parameters will understand correctly.  */
  XSETINT (tem, f->output_data.x->left_pos);
  if (f->output_data.x->left_pos >= 0)
    store_in_alist (alistptr, Qleft, tem);
  else
    store_in_alist (alistptr, Qleft, Fcons (Qplus, Fcons (tem, Qnil)));

  XSETINT (tem, f->output_data.x->top_pos);
  if (f->output_data.x->top_pos >= 0)
    store_in_alist (alistptr, Qtop, tem);
  else
    store_in_alist (alistptr, Qtop, Fcons (Qplus, Fcons (tem, Qnil)));

  FRAME_SAMPLE_VISIBILITY (f);
  store_in_alist (alistptr, Qvisibility,
		  (FRAME_VISIBLE_P (f) ? Qt
		   : FRAME_ICONIFIED_P (f) ? Qicon : Qnil));
  store_in_alist (alistptr, Qdisplay, build_string (":pm.0"));
}


void x_set_frame_parameters (struct frame *f, Lisp_Object alist)
{
  Lisp_Object tail;
  int i;
  pm_request pmr;
  pm_modify more;

  more.width = 0; more.height = 0;
  more.top = 0; more.top_base = 0; more.left = 0; more.left_base = 0;
  more.background_color = COLOR_NONE;
  more.default_font[0] = 0; more.menu_font[0] = 0; more.menu_font_set = 0;
  more.cursor_type = 0; more.cursor_width = 0; more.cursor_blink = 0;
  more.shortcuts = 0; more.alt_modifier = 0; more.altgr_modifier = 0;
  more.sb_width = -1; more.var_width_fonts = 0;
  memset (more.buttons, 0, sizeof (more.buttons));

  for (i = 0; i < sizeof (pm_frame_parms) / sizeof (pm_frame_parms[0]); i++)
    pm_frame_parms[i].set = 0;

  for (tail = alist; CONSP (tail); tail = Fcdr (tail))
    {
      Lisp_Object elt, prop, arg;

      elt = Fcar (tail);
      prop = Fcar (elt);
      arg = Fcdr (elt);

      for (i = 0; i < sizeof (pm_frame_parms)/sizeof (pm_frame_parms[0]); i++)
        if (EQ (prop, pm_frame_parms[i].obj))
          {
            if (!pm_frame_parms[i].set
                && pm_frame_parms[i].setter(f, &more, arg))
              {
                store_frame_param (f, prop, arg);
                pm_frame_parms[i].set = 1;
              }
            break;
          }
      if (i >= sizeof (pm_frame_parms)/sizeof (pm_frame_parms[0]))
        store_frame_param (f, prop, arg);
    }

  if (more.width != 0 || more.height != 0
      || more.top_base != 0 || more.left_base != 0
      || more.background_color != COLOR_NONE
      || more.default_font[0] != 0 || more.menu_font_set
      || more.cursor_type != 0 || more.cursor_blink != 0
      || more.alt_modifier != 0 || more.altgr_modifier != 0
      || more.shortcuts != 0 || more.buttons[0] != 0
      || more.sb_width != -1 || more.var_width_fonts != 0)
    {
      pmr.header.type = PMR_MODIFY;
      pmr.header.frame = (unsigned long)f;
      pm_send (&pmr, sizeof (pmr));
      pm_send (&more, sizeof (more));
      if (more.default_font[0] != 0)
        recompute_basic_faces (f);
    }
}


void x_set_name (struct frame *f, Lisp_Object name, int explicit)
{
  pm_request pmr;
  char *tmp;

  if (explicit)
    {
      if (f->explicit_name && NILP (name))
	update_mode_lines = 1;
      f->explicit_name = ! NILP (name);
    }
  else if (f->explicit_name)
    return;
  if (NILP (name))
    {
      /* Check for no change needed in this very common case
	 before we do any consing.  */
      if (strcmp (XSTRING (f->name)->data, "Emacs") == 0)
        return;
      name = build_string ("Emacs");
    }
  else
    CHECK_STRING (name, 0);
  if (!NILP (Fstring_equal (name, f->name)))
    return;
  if (strcmp (XSTRING (name)->data, "Emacs") == 0)
    tmp = XSTRING (name)->data;
  else
    {
      tmp = alloca (XSTRING (name)->size + 9);
      strcpy (tmp, "Emacs - ");
      strcpy (tmp + 8, XSTRING (name)->data);
    }
  pmr.name.header.type = PMR_NAME;
  pmr.name.header.frame = (unsigned long)f;
  pmr.name.count = strlen (tmp);
  pm_send (&pmr, sizeof (pmr));
  pm_send (tmp, pmr.name.count);
  f->name = name;
}


void x_implicitly_set_name (struct frame *f, Lisp_Object arg,
                            Lisp_Object oldval)
{
  x_set_name (f, arg, 0);
}


int
x_pixel_width (FRAME_PTR f)
{
  return PIXEL_WIDTH (f);
}

int
x_pixel_height (FRAME_PTR f)
{
  return PIXEL_HEIGHT (f);
}

int
x_char_width (FRAME_PTR f)
{
  return FONT_WIDTH (f->output_data.x->font);
}

int
x_char_height (FRAME_PTR f)
{
  return FONT_HEIGHT (f->output_data.x->font);
}


int
x_screen_planes (frame)
     Lisp_Object frame;
{
  return pm_config.planes;
}


/* Borrowed from xterm.c.  */

/* Extract a frame as a FRAME_PTR, defaulting to the selected frame
   and checking validity for X.  */

FRAME_PTR
check_x_frame (frame)
     Lisp_Object frame;
{
  FRAME_PTR f;

  if (NILP (frame))
    f = selected_frame;
  else
    {
      CHECK_LIVE_FRAME (frame, 0);
      f = XFRAME (frame);
    }
  if (! FRAME_X_P (f))
    error ("non-X frame used");
  return f;
}



DEFUN ("pm-list-fonts", Fpm_list_fonts, Spm_list_fonts, 1, 4, 0,
  "Return a list of the names of available fonts matching PATTERN.\n\
If optional arguments FACE and FRAME_ are specified, return only fonts\n\
the same size as FACE on FRAME.\n\
\n\
PATTERN is a string, perhaps with wildcard characters;\n\
  the * character matches any substring, and\n\
  the ? character matches any single character.\n\
  PATTERN is case-insensitive.\n\
FACE is a face name - a symbol.\n\
\n\
The return value is a list of strings, suitable as arguments to\n\
set-face-font.\n\
\n\
The list does not include fonts Emacs can't use (i.e.  proportional\n\
fonts), even if they match PATTERN and FACE.
\n\
The optional fourth argument MAXIMUM sets a limit on how many\n\
fonts to match.  The first MAXIMUM fonts are reported.")
  (pattern, face, frame, maximum)
    Lisp_Object pattern, face, frame, maximum;
{
  pm_request pmr;
  pmd_fontlist *answer;
  unsigned char *buf, *p;
  Lisp_Object *list;
  int i, len, n, count, maxnames;
  FRAME_PTR f;

  check_x ();
  CHECK_STRING (pattern, 0);
  if (!NILP (face))
    CHECK_SYMBOL (face, 1);
  if (!NILP (frame))
    CHECK_LIVE_FRAME (frame, 2);

  f = NILP (frame) ? selected_frame : XFRAME (frame);

  len = XSTRING (pattern)->size;
  if (len > 511) len = 511;

  BLOCK_INPUT;
  pmr.fontlist.header.type = PMR_FONTLIST;
  pmr.fontlist.header.frame = (unsigned long)f;
  pmr.fontlist.serial = pm_serial++;
  pmr.fontlist.pattern_length = len;
  pm_send (&pmr, sizeof (pmr));
  pm_send (XSTRING (pattern)->data, len);

  buf = pm_receive (pmr.fontlist.serial, NULL, NULL, 0);
  UNBLOCK_INPUT;
  if (buf == NULL)
    return Qnil;

  answer = (pmd_fontlist *)buf;

  /* Check to see if we have been given an upper limit */
  if (NILP(maximum))
    maxnames = answer->count;
  else
    {
      CHECK_NATNUM(maximum, 0);
      maxnames = (XINT (maximum) > answer->count) ?
          answer->count :
          XINT (maximum);
    }
      
  list = alloca (maxnames * sizeof (Lisp_Object));
  count = 0;
  p = buf + sizeof (pmd_fontlist);

  for (i = 0; i < maxnames; ++i)
    {
      len = *p++;
      list[count++] = make_string (p, len);
      p += len;
    }
  xfree (buf);
  return Flist (count, list);
}


DEFUN ("pm-color-defined-p", Fpm_color_defined_p, Spm_color_defined_p, 1, 2, 0,
  "Return non-nil if color COLOR is supported on frame FRAME.\n\
if FRAME is omitted or nil, use the selected frame.")
  (color, frame)
     Lisp_Object color, frame;
{
  XColor foo;
  FRAME_PTR f = check_x_frame (frame);

  CHECK_STRING (color, 1);

  if (defined_color (f, XSTRING (color)->data, &foo, 0))
    return Qt;
  else
    return Qnil;
}


DEFUN ("pm-color-values", Fpm_color_values, Spm_color_values, 1, 2, 0,
  "Return a description of the color named COLOR on frame FRAME.\n\
The value is a list of integer RGB values--(RED GREEN BLUE).\n\
These values appear to range from 0 to 65280 or 65535, depending\n\
on the system; white is (65280 65280 65280) or (65535 65535 65535).\n\
If FRAME is omitted or nil, use the selected frame.")
  (color, frame)
     Lisp_Object color, frame;
{
  XColor foo;
  FRAME_PTR f = check_x_frame (frame);

  CHECK_STRING (color, 1);

  if (defined_color (f, XSTRING (color)->data, &foo, 0))
    {
      Lisp_Object rgb[3];

      rgb[0] = make_number (((foo.pixel >> 16) & 0xff) * 256);
      rgb[1] = make_number (((foo.pixel >> 8) & 0xff) * 256);
      rgb[2] = make_number (((foo.pixel >> 0) & 0xff) * 256);
      return Flist (3, rgb);
    }
  else
    return Qnil;
}


DEFUN ("pm-display-color-p", Fpm_display_color_p, Spm_display_color_p, 0, 1, 0,
  "Return t if the display supports color.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return Qt;
}


DEFUN ("pm-display-grayscale-p", Fpm_display_grayscale_p,
  Spm_display_grayscale_p, 0, 1, 0,
  "Return t if the X display supports shades of gray.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  if (pm_config.planes <= 1)
    return Qnil;
  return Qt;
}


DEFUN ("x-display-pixel-width", Fx_display_pixel_width, Sx_display_pixel_width,
  0, 1, 0,
  "Returns the width in pixels of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return make_number (pm_config.width);
}

DEFUN ("x-display-pixel-height", Fx_display_pixel_height,
  Sx_display_pixel_height, 0, 1, 0,
  "Returns the height in pixels of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return make_number (pm_config.height);
}


DEFUN ("pm-display-planes", Fpm_display_planes, Spm_display_planes,
  0, 1, 0,
  "Returns the number of bitplanes of the display FRAME is on.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return make_number (pm_config.planes);
}


DEFUN ("pm-display-color-cells", Fpm_display_color_cells,
  Spm_display_color_cells, 0, 1, 0,
  "Returns the number of color cells of the display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return make_number (pm_config.color_cells);
}


DEFUN ("x-display-screens", Fx_display_screens, Sx_display_screens, 0, 1, 0,
  "Returns the number of screens on the X server of display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return make_number (1);
}


DEFUN ("x-display-mm-height", Fx_display_mm_height, Sx_display_mm_height, 0, 1, 0,
  "Returns the height in millimeters of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return make_number (pm_config.height_mm);
}

DEFUN ("x-display-mm-width", Fx_display_mm_width, Sx_display_mm_width, 0, 1, 0,
  "Returns the width in millimeters of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  return make_number (pm_config.width_mm);
}


DEFUN ("pm-open-connection", Fpm_open_connection, Spm_open_connection,
       0, 0, 0, "Open a connection to PM Emacs.")
  ()
{
  if (pm_session_started)
    error ("PM Emacs connection is already initialized");
  pm_init ();
  return Qnil;
}


/* This function is called by kill-emacs, see emacs.c. */

DEFUN ("x-close-connection", Fx_close_connection,
       Sx_close_connection, 1, 1, 0,
   "Close the connection to DISPLAY's X server.\n\
For DISPLAY, specify either a frame or a display name (a string).\n\
If DISPLAY is nil, that stands for the selected frame's display.")
  (display)
  Lisp_Object display;
{
  struct x_display_info *dpyinfo = check_pm_display_info (display);

  if (dpyinfo->reference_count > 0)
    error ("Display still has frames on it");
  pm_shutdown ();
  return Qnil;
}


DEFUN ("x-display-list", Fx_display_list, Sx_display_list, 0, 0, 0,
  "Return the list of display names that Emacs has connections to.")
  ()
{
  if (pm_session_started)
    return Fcons (build_string (":pm.0"), Qnil);
  else
    return Qnil;
}


Lisp_Object
x_get_focus_frame (frame)
     struct frame *frame;
{
  struct x_display_info *dpyinfo = FRAME_X_DISPLAY_INFO (frame);
  Lisp_Object tem;

  /*TODO*/
  XSETFRAME (tem, selected_frame);
  return tem;
}


DEFUN ("focus-frame", Ffocus_frame, Sfocus_frame, 1, 1, 0,
  "Set the focus on FRAME.")
  (frame)
     Lisp_Object frame;
{
  CHECK_LIVE_FRAME (frame, 0);

  if (FRAME_X_P (XFRAME (frame)))
    {
      x_focus_on_frame (XFRAME (frame));
      return frame;
    }

  return Qnil;
}


DEFUN ("unfocus-frame", Funfocus_frame, Sunfocus_frame, 0, 0, 0,
  "If a frame has been focused, release it.")
  ()
{
  return Qnil;
}


DEFUN ("pm-create-frame", Fpm_create_frame, Spm_create_frame,
       1, 1, 0,
  "Make a new PM window, which is called a \"frame\" in Emacs terms.\n\
Return an Emacs frame object.\n\
ALIST is an alist of frame parameters.\n\
If the parameters specify that the frame should not have a minibuffer,\n\
and do not specify a specific minibuffer window to use,\n\
then `default-minibuffer-frame' must be a frame whose minibuffer can\n\
be shared by the new frame.\n\
\n\
This function is an internal primitive--use `make-frame' instead.")
  (parms)
     Lisp_Object parms;
{
  struct frame *f;
  Lisp_Object frame, name, tem;
  int minibuffer_only = 0;
  int width, height;
  int count = specpdl_ptr - specpdl;
  struct gcpro gcpro1, gcpro2, gcpro3;
  pm_request pmr;
  Lisp_Object display;
  struct kboard *kb;

  check_x ();

  kb = &the_only_kboard;
  display = Qnil;

  name = pm_get_arg (parms, Qname);
  if (!STRINGP (name) && !EQ (name, Qunbound) && !NILP (name))
    error ("Invalid frame name--not a string or nil");

  /* make_frame_without_minibuffer can run Lisp code and garbage collect.  */
  frame = Qnil;
  GCPRO3 (parms, name, frame);

  minibuffer_only = 0;
  tem = pm_get_arg (parms, Qminibuffer);
  if (EQ (tem, Qnone) || NILP (tem))
    f = make_frame_without_minibuffer (Qnil, kb, display);
  else if (EQ (tem, Qonly))
    {
      f = make_minibuffer_frame ();
      minibuffer_only = 1;
    }
  else if (WINDOWP (tem))
    f = make_frame_without_minibuffer (tem, kb, display);
  else
    f = make_frame (1);

  XSETFRAME (frame, f);

  FRAME_CAN_HAVE_SCROLL_BARS (f) = 1;

  f->output_method = output_x_window;
  f->output_data.x = (struct x_output *) xmalloc (sizeof (struct x_output));
  bzero (f->output_data.x, sizeof (struct x_output));

  FRAME_X_DISPLAY_INFO (f) = pm_display;

  /* Note that the frame has no physical cursor right now.  */
  f->phys_cursor_x = -1;

  /* Set the name; the functions to which we pass f expect the name to
     be set.  */
  if (EQ (name, Qunbound) || NILP (name))
    {
      f->name = build_string ("Emacs");
      f->explicit_name = 0;
    }
  else
    {
      f->name = name;
      f->explicit_name = 1;
    }

  f->output_data.x->font = NULL;
  f->output_data.x->pixel_height = 0;
  f->output_data.x->pixel_width = 0;
  f->output_data.x->line_height = 1; /* TODO */
  tem = pm_get_arg (parms, Qheight);
  if (EQ (tem, Qunbound))
    tem = pm_get_arg (parms, Qwidth);
  if (EQ (tem, Qunbound))
    {
      width = 80; height = 25;
    }
  else
    {
      tem = pm_get_arg (parms, Qheight);
      if (EQ (tem, Qunbound))
        error ("Height not specified");
      CHECK_NUMBER (tem, 0);
      height = XINT (tem);

      tem = pm_get_arg (parms, Qwidth);
      if (EQ (tem, Qunbound))
        error ("Width not specified");
      CHECK_NUMBER (tem, 0);
      width = XINT (tem);
    }

  pm_add_frame (f);

  pmr.create.header.type = PMR_CREATE;
  pmr.create.header.frame = (unsigned long)f;
  pmr.create.height = height;
  pmr.create.width = width;
  pm_send (&pmr, sizeof (pmr));

  pm_default_parameter (f, parms, Qvar_width_fonts, Qnil);
  pm_default_parameter (f, parms, Qforeground_color, build_string ("black"));
  pm_default_parameter (f, parms, Qbackground_color, build_string ("white"));
  pm_default_parameter (f, parms, Qfont, build_string (DEFAULT_FONT));

  {
    Lisp_Object name;
    int explicit = f->explicit_name;

    f->explicit_name = 0;
    name = f->name;
    f->name = Qnil;
    x_set_name (f, name, explicit);
  }

  init_frame_faces (f);

  pm_default_parameter (f, parms, Qcursor_type, Qbox);
  pm_default_parameter (f, parms, Qcursor_blink, Qt);
  pm_default_parameter (f, parms, Qshortcuts, Qnil);
  pm_default_parameter (f, parms, Qalt_modifier, Qmeta);
  pm_default_parameter (f, parms, Qaltgr_modifier, Qalt);
  pm_default_parameter (f, parms, Qmouse_buttons, build_string ("132"));

  f->height = f->width = 0;
  change_frame_size (f, height, width, 1, 0);

  pm_default_parameter (f, parms, Qmenu_font, Qnil);
  pm_default_parameter (f, parms, Qmenu_bar_lines, make_number (1));
  pm_default_parameter (f, parms, Qtop, Qnil);
  pm_default_parameter (f, parms, Qleft, Qnil);
  pm_default_parameter (f, parms, Qscroll_bar_width, Qnil);
  pm_default_parameter (f, parms, Qvertical_scroll_bars, Qt);

  if (!minibuffer_only && FRAME_EXTERNAL_MENU_BAR (f))
    initialize_frame_menubar (f);

  tem = pm_get_arg (parms, Qunsplittable);
  f->no_split = minibuffer_only || EQ (tem, Qt);

  UNGCPRO;

  Vframe_list = Fcons (frame, Vframe_list);

  /* Now that the frame is official, it counts as a reference to
     its display.  */
  FRAME_X_DISPLAY_INFO (f)->reference_count++;

  /* Make the window appear on the frame and enable display,
     unless the caller says not to.  */
  {
    Lisp_Object visibility;

    visibility = pm_get_arg (parms, Qvisibility);
    if (EQ (visibility, Qunbound))
      visibility = Qt;

    if (EQ (visibility, Qicon))
      x_iconify_frame (f);
    else if (! NILP (visibility))
      x_make_frame_visible (f);
    else
      /* Must have been Qnil.  */
      ;
  }

  pmr.header.type = PMR_CREATEDONE;
  pmr.header.frame = (unsigned long)f;
  pm_send (&pmr, sizeof (pmr));

  pm_get_framepos (f);

  return unbind_to (count, frame);
}


/* Extract the event symbol sans modifiers from an event.  Used in
   xmenu.c */

int pm_event_button (Lisp_Object position)
{
  Lisp_Object head, els, ev;

  head = Fcar (position);           /* EVENT_HEAD (position) */
  els = Fget (head, Qevent_symbol_elements);
  if (Fmemq (Qdown, Fcdr (els)))
    {
      ev = Fcar (els);
      if (EQ (ev, Qmouse_1))
        return 1;
      else if (EQ (ev, Qmouse_2))
        return 2;
      else if (EQ (ev, Qmouse_3))
        return 3;
    }
  return 0;
}


DEFUN ("pm-get-drop", Fpm_get_drop, Spm_get_drop, 1, 1, 0,
  "Get name of dropped object.\n\
TIMESTAMP is the timestamp of the event.\n\
Return nil if there is no such object.")
  (timestamp)
     Lisp_Object timestamp;
{
  pm_request pmr;
  char name[260];               /* CCHMAXPATH */
  void *buf;
  int size;

  check_x ();

  CHECK_NUMBER (timestamp, 0);

  BLOCK_INPUT;
  pmr.drop.header.type = PMR_DROP;
  pmr.drop.header.frame = 0;
  pmr.drop.serial = pm_serial++;
  pmr.drop.cookie = XINT (timestamp);
  pm_send (&pmr, sizeof (pmr));
  buf = pm_receive (pmr.drop.serial, name, &size, 0);
  UNBLOCK_INPUT;
  if (buf == NULL || size == 0)
    return Qnil;
  return make_string (name, size);
}


/* Return a list of code pages supported by PM. */

Lisp_Object pm_list_code_pages (void)
{
  pm_request pmr;
  int *buf;
  int i, size;
  Lisp_Object list;

  BLOCK_INPUT;
  pmr.cplist.header.type = PMR_CPLIST;
  pmr.cplist.header.frame = 0;
  pmr.cplist.serial = pm_serial++;
  pm_send (&pmr, sizeof (pmr));
  buf = pm_receive (pmr.cplist.serial, NULL, &size, 0);
  UNBLOCK_INPUT;
  if (buf == NULL)
    return Qnil;
  list = Qnil;
  for (i = size / sizeof (int) - 1; i >= 1; --i)
    list = Fcons (make_number (buf[i]), list);
  xfree (buf);
  return list;
}


/* Send the new code page to pmemacs.exe, recompute all faces, and
   redraw all frames.  Return zero on error. */

int pm_set_code_page (int cp)
{
  pm_request pmr;
  void *buf;
  int ok;

  BLOCK_INPUT;
  pmr.codepage.header.type = PMR_CODEPAGE;
  pmr.codepage.header.frame = 0;
  pmr.codepage.codepage = cp;
  pmr.codepage.serial = pm_serial++;
  pm_send (&pmr, sizeof (pmr));
  buf = pm_receive (pmr.codepage.serial, &ok, NULL, 0);
  UNBLOCK_INPUT;
  if (buf == NULL || !ok)
    return 0;
  clear_face_cache ();          /* Recompute all faces */
  Fredraw_display ();
  return 1;
}


DEFUN ("pm-file-dialog", Fpm_file_dialog, Spm_file_dialog, 3, 7, 0,
  "Show and process a file dialog on frame FRAME with TITLE.\n\
If FRAME is nil, use the current frame.  The default directory is DIR,\n\
which is not expanded---you must call `expand-file-name' yourself.\n\
The initial value of the file-name entryfield is DEFAULT or empty if\n\
DEFAULT is nil.  Fifth arg MUSTMATCH non-nil means require existing\n\
file's name.  Sixth arg SAVEAS non-nil creates a Save As dialog instead\n\
of a Open dialog.  Seventh arg BUTTON specifies text to for the OK button,\n\
the default is \"OK\".\n\
Return the select file name as string.  Return nil, if no file name was\n\
selected.")
  (frame, title, dir, defalt, mustmatch, saveas, button)
     Lisp_Object frame, title, dir, defalt, mustmatch, saveas, button;
{
  pm_request pmr;
  pm_filedialog more;
  char name[260];               /* CCHMAXPATH */
  void *buf;
  int size;
  FRAME_PTR f;

  check_x ();

  if (NILP (frame))
    f = selected_frame;
  else
    {
      CHECK_LIVE_FRAME (frame, 0);
      f = XFRAME (frame);
    }

  CHECK_STRING (title, 1);
  CHECK_STRING (dir, 2);
  if (!NILP (defalt))
    CHECK_STRING (defalt, 3);
  if (!NILP (button))
    CHECK_STRING (button, 5);

  BLOCK_INPUT;
  pmr.header.type = PMR_FILEDIALOG;
  pmr.header.frame = (unsigned long)f;
  more.serial = pm_serial++;
  more.save_as = !NILP (saveas);
  more.must_match = !NILP (mustmatch);
  _strncpy (more.title, XSTRING (title)->data, sizeof (more.title));
  _strncpy (more.dir, XSTRING (dir)->data, sizeof (more.dir));
  if (NILP (defalt))
    more.defalt[0] = 0;
  else
    _strncpy (more.defalt, XSTRING (defalt)->data, sizeof (more.defalt));
  if (NILP (button))
    strcpy (more.ok_button, "OK");
  else
    _strncpy (more.ok_button, XSTRING (button)->data);
  pm_send (&pmr, sizeof (pmr));
  pm_send (&more, sizeof (more));

  buf = pm_receive (more.serial, name, &size, 1);
  UNBLOCK_INPUT;
  if (buf == NULL || size == 0)
    return Qnil;
  return make_string (name, size);
}


void x_sync (frame)
     Lisp_Object frame;
{
}


void syms_of_xfns ()
{
  Qalt = intern ("alt");
  staticpro (&Qalt);
  Qalt_f4 = intern ("alt-f4");
  staticpro (&Qalt_f4);
  Qalt_f5 = intern ("alt-f5");
  staticpro (&Qalt_f5);
  Qalt_f6 = intern ("alt-f6");
  staticpro (&Qalt_f6);
  Qalt_f7 = intern ("alt-f7");
  staticpro (&Qalt_f7);
  Qalt_f8 = intern ("alt-f8");
  staticpro (&Qalt_f8);
  Qalt_f9 = intern ("alt-f9");
  staticpro (&Qalt_f9);
  Qalt_f10 = intern ("alt-f10");
  staticpro (&Qalt_f10);
  Qalt_f11 = intern ("alt-f11");
  staticpro (&Qalt_f11);
  Qalt_modifier = intern ("alt-modifier");
  staticpro (&Qalt_modifier);
  Qalt_space = intern ("alt-space");
  staticpro (&Qalt_space);
  Qaltgr = intern ("altgr");
  staticpro (&Qaltgr);
  Qaltgr_modifier = intern ("altgr-modifier");
  staticpro (&Qaltgr_modifier);
  Qbackground_color = intern ("background-color");
  staticpro (&Qbackground_color);
  Qbar = intern ("bar");
  staticpro (&Qbar);
  Qbox = intern ("box");
  staticpro (&Qbox);
  Qcursor_blink = intern ("cursor-blink");
  staticpro (&Qcursor_blink);
  Qcursor_type = intern ("cursor-type");
  staticpro (&Qcursor_type);
  Qdisplay = intern ("display");
  staticpro (&Qdisplay);
  Qdown = intern ("down");
  staticpro (&Qdown);
  Qf1 = intern ("f1");
  staticpro (&Qf1);
  Qf10 = intern ("f10");
  staticpro (&Qf10);
  Qforeground_color = intern ("foreground-color");
  staticpro (&Qforeground_color);
  Qframe = intern ("frame");
  staticpro (&Qframe);
  Qhalftone = intern ("halftone");
  staticpro (&Qhalftone);
  Qhyper = intern ("hyper");
  staticpro (&Qhyper);
  Qleft = intern ("left");
  staticpro (&Qleft);
  Qright = intern ("right");
  staticpro (&Qright);
  Qmenu_font = intern ("menu-font");
  staticpro (&Qmenu_font);
  Qmeta = intern ("meta");
  staticpro (&Qmeta);
  Qmouse_1 = intern ("mouse-1");
  staticpro (&Qmouse_1);
  Qmouse_2 = intern ("mouse-2");
  staticpro (&Qmouse_2);
  Qmouse_3 = intern ("mouse-3");
  staticpro (&Qmouse_3);
  Qmouse_buttons = intern ("mouse-buttons");
  staticpro (&Qmouse_buttons);
  Qnone = intern ("none");
  staticpro (&Qnone);
  Qscroll_bar_width = intern ("scroll-bar-width");
  staticpro (&Qscroll_bar_width);
  Qshortcuts = intern ("shortcuts");
  staticpro (&Qshortcuts);
  Qsuper = intern ("super");
  staticpro (&Qsuper);
  Qtop = intern ("top");
  staticpro (&Qtop);
  Qvisibility = intern ("visibility");
  staticpro (&Qvisibility);
  Qvar_width_fonts = intern ("var-width-fonts");
  staticpro (&Qvar_width_fonts);
  Qvertical_scroll_bars = intern ("vertical-scroll-bars");
  staticpro (&Qvertical_scroll_bars);

  DEFVAR_LISP ("pm-color-alist", &Vpm_color_alist,
    "*List of elements (\"COLOR\" . [R G B]) for defining colors.\n\
\"COLOR\" is the name of the color.  Don't use upper-case letters.\n\
R, G and B are numbers in 0 through 255, indicating the intensity\n\
of the red, green and blue beams, respectively.");
  Vpm_color_alist = Qnil;

  DEFVAR_LISP ("x-bitmap-file-path", &Vx_bitmap_file_path,
    "List of directories to search for bitmap files for X.");
  Vx_bitmap_file_path = decode_env_path ((char *) 0, PATH_BITMAPS);

  DEFVAR_LISP ("x-pointer-shape", &Vx_pointer_shape,
    "The shape of the pointer when over text.\n\
Changing the value does not affect existing frames\n\
unless you set the mouse color.");
  Vx_pointer_shape = Qnil;


  Fprovide (intern ("x-toolkit"));

  defsubr (&Sfocus_frame);
  defsubr (&Sunfocus_frame);
  defsubr (&Spm_display_color_p);
  defsubr (&Spm_display_grayscale_p);
  defsubr (&Spm_display_planes);
  defsubr (&Spm_display_color_cells);
  defsubr (&Spm_list_fonts);
  defsubr (&Spm_color_defined_p);
  defsubr (&Spm_color_values);
  defsubr (&Spm_create_frame);
  defsubr (&Spm_open_connection);
  defsubr (&Spm_get_drop);
  defsubr (&Spm_file_dialog);
  defsubr (&Sx_close_connection);
  defsubr (&Sx_display_list);
  defsubr (&Sx_display_mm_height);
  defsubr (&Sx_display_mm_width);
  defsubr (&Sx_display_pixel_height);
  defsubr (&Sx_display_pixel_width);
  defsubr (&Sx_display_screens);

  init_pm_parm_symbols ();
}
@
