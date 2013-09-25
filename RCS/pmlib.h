head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	2000.10.21.21.08.32;	author JeremyB;	state Exp;
branches;
next	;


desc
@OS/2 Emacs
@


1.1
log
@Initial revision
@
text
@/* pmlib.h -- Minimal X11/Xlib.h for the OS/2 Presentation Manager
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


#if !defined (_PMLIB_H)
#define _PMLIB_H

typedef int GC;
typedef int Pixmap;
typedef unsigned long Pixel;
typedef int Display;
typedef int Colormap;

typedef struct pm_font
{
  struct pm_font *next;
  struct
    {
      int width;
    } max_bounds;
  int ascent;
  int descent;
  char name[100];
} XFontStruct;

typedef struct
{
  int width, height, x, y;
} XRectangle;

typedef struct
{
  int pixel;
} XColor;

typedef int Widget;
typedef int LWLIB_ID;
typedef int XtPointer;

#define DefaultScreenOfDisplay(d)	0
#define DefaultColormapOfScreen(s)	0


/* void XFreeGC (Display *dpy, GC gc); */
#define XFreeGC(dpy,gc)			((void)0)

XFontStruct *XLoadQueryFont (Display *dpy, char *name);
int XParseColor (Display *dpy, Colormap cmap, char *name, XColor *color);

/* void XFreeFont (Display *dpy, XFontStruct *font); */
#define XFreeFont(dpy,font)		((void)0)

/* int XAllocColor (Display *dpy, Colormap cmap, XColor *color); */
#define XAllocColor(dpy,cmap,color)	1

#endif
@
