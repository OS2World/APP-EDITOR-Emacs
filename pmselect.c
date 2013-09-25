/* pmselect.c -- xselect.c for the OS/2 Presentation Manager
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


#include <stdio.h>
#include "config.h"
#include "lisp.h"
#include "frame.h"
#include "blockinput.h"
#include "pmterm.h"
#include "pmemacs.h"
#include "charset.h"
#include "coding.h"

Lisp_Object QCLIPBOARD;

/* Coding system for communicating with other Windows programs via the
   clipboard.  */
static Lisp_Object Vselection_coding_system;


void
x_clear_frame_selections (f)
     FRAME_PTR f;
{
}


DEFUN ("x-selection-owner-p", Fx_selection_owner_p, Sx_selection_owner_p,
  0, 1, 0,
  "Whether the current Emacs process owns the given X Selection.\n\
The arg should be the name of the selection in question, typically one of\n\
the symbols `PRIMARY', `SECONDARY', or `CLIPBOARD'.\n\
\(Those are literal upper-case symbol names, since that's what X expects.)\n\
For convenience, the symbol nil is the same as `PRIMARY',\n\
and t is the same as `SECONDARY'.)")
  (selection)
     Lisp_Object selection;
{
  return Qnil;
}


DEFUN ("x-selection-exists-p", Fx_selection_exists_p, Sx_selection_exists_p,
  0, 1, 0,
  "Whether there is an owner for the given X Selection.\n\
The arg should be the name of the selection in question, typically one of\n\
the symbols `PRIMARY', `SECONDARY', or `CLIPBOARD'.\n\
\(Those are literal upper-case symbol names, since that's what X expects.)\n\
For convenience, the symbol nil is the same as `PRIMARY',\n\
and t is the same as `SECONDARY'.)")
  (selection)
     Lisp_Object selection;
{
  return Qnil;
}


DEFUN ("pm-get-clipboard", Fpm_get_clipboard, Spm_get_clipboard,
  0, 0, 0,
  "Retrieve a text string from the clipboard.\n\
Return the empty string if there is no text in the clipboard.")
  ()
{
  pm_request pmr;
  int size;
  char *buf;
  Lisp_Object result;

  BLOCK_INPUT;
  pmr.paste.header.type = PMR_PASTE;
  pmr.paste.header.frame = 0;
  pmr.paste.serial = pm_serial++;
  pmr.paste.get_text = 1;
  pm_send (&pmr, sizeof (pmr));
  buf = pm_receive (pmr.paste.serial, 0, &size, 0);
  UNBLOCK_INPUT;
  if (!buf)
    result = make_string ("", 0);
  else
    {
      _crlf (buf, size, &size);
      result = make_string (buf, size);
      xfree (buf);
    }
  return result;
}


DEFUN ("pm-clipboard-ready-p", Fpm_clipboard_ready_p, Spm_clipboard_ready_p,
  0, 0, 0,
  "Return t if a text is in the clipboard.")
  ()
{
  pm_request pmr;
  void *buf;
  int size;

  BLOCK_INPUT;
  pmr.paste.header.type = PMR_PASTE;
  pmr.paste.header.frame = 0;
  pmr.paste.serial = pm_serial++;
  pmr.paste.get_text = 0;
  pm_send (&pmr, sizeof (pmr));
  buf = pm_receive (pmr.paste.serial, &size, 0, 0);
  UNBLOCK_INPUT;
  if (buf == NULL)
    return Qnil;
  return (size == 0 ? Qnil : Qt);
}


DEFUN ("pm-put-clipboard", Fpm_put_clipboard, Spm_put_clipboard,
  1, 1, 0,
  "Put a text string into the clipboard.")
  (string)
     Lisp_Object string;
{
  pm_request pmr;
  unsigned long size, tmp;
  char *p, *q, *buf;

  CHECK_STRING (string, 0);
  p = XSTRING (string)->data;
  size = XSTRING (string)->size;
  for (tmp = size; tmp != 0; --tmp)
    if (*p++ == '\n')
      ++size;
  buf = alloca (size);
  p = XSTRING (string)->data;
  q = buf;
  for (tmp = XSTRING (string)->size; tmp != 0; --tmp)
    {
      if (*p == '\n')
        *q++ = '\r';
      *q++ = *p++;
    }
  pmr.cut.header.type = PMR_CUT;
  pmr.cut.header.frame = 0;
  pmr.cut.size = size;
  pm_send (&pmr, sizeof (pmr));
  pm_send (buf, size);
  return Qnil;
}


void syms_of_xselect ()
{
  defsubr (&Sx_selection_owner_p);  
  defsubr (&Sx_selection_exists_p);
  defsubr (&Spm_get_clipboard);
  defsubr (&Spm_put_clipboard);
  defsubr (&Spm_clipboard_ready_p);

  DEFVAR_LISP ("selection-coding-system", &Vselection_coding_system,
    "Coding system for communicating with other X clients.\n\
When sending or receiving text via cut_buffer, selection, and clipboard,\n\
the text is encoded or decoded by this coding system.\n\
A default value is `compound-text'");
  Vselection_coding_system=intern ("iso-latin-1-dos");

  QCLIPBOARD = intern ("CLIPBOARD");	staticpro (&QCLIPBOARD);

  
}
