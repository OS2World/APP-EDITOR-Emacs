head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	2000.06.08.09.15.24;	author JeremyB;	state Exp;
branches;
next	;


desc
@Baseline 20.6
@


1.1
log
@Initial revision
@
text
@/* Mark beginning of data space to dump as pure, for GNU Emacs.
   Copyright (C) 1997 Free Software Foundation, Inc.

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

#ifdef WINDOWSNT
/* See comments in lastfile.c.  */
char my_begdata[] = "Beginning of Emacs initialized data";
char my_begbss[1];  /* Do not initialize this variable.  */
static char _my_begbss[1];
char * my_begbss_static = _my_begbss;

/* Add a dummy reference to ensure emacs.obj is linked in.  */
extern int initialized;
static int * dummy = &initialized;
#endif

@
