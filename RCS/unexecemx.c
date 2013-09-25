head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	2000.10.21.21.02.56;	author JeremyB;	state Exp;
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
@/* unexecemx.c
   Copyright (C) 1994-1996 Eberhard Mattes.

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


#include "config.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/process.h>

unexec (new_name, a_name, data_start, bss_start, entry_address)
     char *new_name, *a_name;
     unsigned data_start, bss_start, entry_address;
{
  int fd, rc, extract;
  char emxl_path[512];

  if (a_name == 0)
    fatal ("SYMFILE argument of dump-emacs is nil\n");
  fd = open ("emacs.cor", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
  if (fd < 0)
    fatal ("Cannot create core dump file\n");
  if (_core (fd) != 0)
    fatal ("Cannot write core dump file\n");
  close (fd);
  extract = 0;
  if (access (a_name, 0) != 0)
    {
      rc = spawnlp (P_WAIT, "emxbind.exe", "emxbind.exe",
                    "-xq", a_name, a_name, 0);
      if (rc == -1)
        fatal ("Cannot run emxbind\n");
      if (rc != 0)
        fatal ("emxbind failed to extract a.out image\n");
      extract = 1;
    }
  if (_path (emxl_path, "emxl.exe") != 0
      && _path (emxl_path, "emx.exe") != 0)
    strcpy (emxl_path, "/emx/bin/emxl.exe");
  rc = spawnlp (P_WAIT, "emxbind.exe", "emxbind.exe",
                "-bqs", "-h48", "-cemacs.cor",
                emxl_path, a_name, new_name,
                "-h40", 0);
  unlink ("emacs.cor");
  if (extract)
    unlink (a_name);
  if (rc == -1)
    fatal ("Cannot run emxbind\n");
  if (rc != 0)
    fatal ("emxbind failed to bind executable\n");
  return 0;
}
@
