head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	2000.10.21.20.36.18;	author JeremyB;	state Exp;
branches;
next	;


desc
@Emacs 20.6
@


1.1
log
@Initial revision
@
text
@/* emxdep.c, emx-specific bits of GNU Emacs.
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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/process.h>
#include <sys/ea.h>
#include <sys/ead.h>
#include <sys/nls.h>

#define INCL_DEVICES
#define INCL_DOSPROCESS
#define INCL_DOSSESMGR
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#define INCL_WINSWITCHLIST
#define INCL_KBD
#define INCL_VIO
#define UCHAR OS2UCHAR
#include <os2.h>
#undef UCHAR

#include "config.h"
#include "lisp.h"
#include "commands.h"

/* Priority class and level.  */
struct priority
{
  ULONG pclass;
  LONG plevel;
};

extern Lisp_Object Qicon;

Lisp_Object Qascii;
Lisp_Object Qbinary;
Lisp_Object Qbinary_process_input;
Lisp_Object Qbitmap;
Lisp_Object Qmetafile;
Lisp_Object Qea;
Lisp_Object Qmvmt;
Lisp_Object Qmvst;
Lisp_Object Qasn1;
Lisp_Object Qemx_case_map;
Lisp_Object Qpreserve;

/* Alist of elements (REGEXP . HANDLER) for program file names for
   which HANDLER is called by call-process and start-process.  */
static Lisp_Object Vprogram_name_handler_alist;

/* This variable describes handlers that have "already" had a chance
   to handle the current operation.  */
static Lisp_Object Vinhibit_program_name_handlers;

/* List of regular expressions identifying file names for which binary
   mode is to be used.  */
Lisp_Object Vemx_binary_mode_list;

/* The system emx is running on: `ms-dos' or `os2'.  */
Lisp_Object Vemx_system_type;

/* The priority to be used for child processes.  */
Lisp_Object Vprocess_priority;

/* Use binary mode for input pipe to processes and output pipe from
   processes, respectively.  */
int binary_process_input;
int binary_process_output;

/* Use binary mode for input pipe to asynchronous processes.  */
int binary_async_process_input;

/* Prevent `expand-file-name' from prepending a drive letter.  */
static int remote_expand_file_name;

/* The current code page.  */
int cur_code_page;

/* Translate file names to lower case if this variable is non-nil.
   Let `expand-file-name' preserve letter case in file names by using
   the `emx-case-map' text property if this variable is `preserve'. */
static Lisp_Object Vcase_fold_file_names;

/* Start child processes as sessions (vs. processes). */
static int emx_start_session;

/* Send signals to process tree. */
static int emx_kill_tree;

/* Process priority class and level to be used for child process, set
   by check_process_priority.  */
static struct priority child_priority;

/* Process priority class and level of the main thread of Emacs.  */
static struct priority emacs_priority;

/* The following variables are used for accessing text-mode files as
   if they were binary files.  fb_fd is the handle of the current
   `faked binary file' (there can be at most one such file at a time).
   If fb_bd is -1, there is no `faked binary file'.  fb_buf holds the
   complete contents of the file, with cr/lf translated to newline.
   fb_size is the number of translated bytes.  fb_pos is the current
   value of the file position indicator.  */
static int fb_fd;
static char *fb_buf;
static int fb_size;
static int fb_pos;

extern Lisp_Object Vprocess_environment;
#ifdef HAVE_PM
extern int pm_pid;
extern int pm_session_started;
extern Lisp_Object pm_list_code_pages ();
#endif /* HAVE_PM */


int sigblock (int mask)
{
  return 0;
}

int sigsetmask (int mask)
{
  return 0;
}

int nice (int incr)
{
  return 0;
}

int setuid (int id)
{
  return 0;
}

#define EMX_UID 1

int getuid (void)
{
  return EMX_UID;
}

int geteuid (void)
{
  return EMX_UID;
}

/* The pw_uid member is not used anywhere in Emacs 19.28.  Avoid,
   however, surprises in future versions. */

struct passwd *emxdep_getpwuid (uid_t uid)
{
  struct passwd *p;

  p = (getpwuid)(uid);
  if (p)
    p->pw_uid = EMX_UID;
  return p;
}

struct passwd *emxdep_getpwnam (const char *name)
{
  struct passwd *p;

  p = (getpwnam)(name);
  if (p)
    p->pw_uid = EMX_UID;
  return p;
}

int link (char *name1, char *name2)
{
  return (rename (name1, name2));
}

int setpgrp (int pid, int pgrp)
{
  return 0;
}

int gethostname (char *name, int namelen)
{
  char *sp = getenv ("SYSTEMNAME");
  if (!sp)
    sp = "standalone";
  _strncpy (name, sp, namelen);
  return 0;
}

int vfork (void)
{
  return 0;                     /* We're the child process! */
}


int emx_killpg (gid, signo)
{
  return kill (emx_kill_tree ? gid : -gid, signo);
}


/* The following functions are used to be able to read a text-mode
   file as if it were a binary file, that is, lseek() works and the
   st_size field of struct stat is exact.  This is done by reading the
   entire file into memory, translating cr/lf to the newline
   character.  Only one file at a time can be a `faked binary file'.  */

/* Prepare for reading from FD as if it were the handle of a binary
   file.  SIZE is a pointer to the st_size field of struct stat for
   the file.  This function assumes that FD is opened in text mode.
   Return 0 on success, -1 on error.  */

int fb_start (int fd, off_t *size)
{
  if (fb_fd != -1)
    abort ();
  fb_fd = fd;
  fb_pos = 0;
  fb_buf = (char *) xmalloc (*size);
  fb_size = read (fd, fb_buf, *size);
  if (fb_size < 0)
    {
      xfree (fb_buf);
      fb_fd = -1;
      return -1;
    }
  *size = fb_size;
  return 0;
}


/* Replace read() for `faked binary files'.  If FD is not the handle
   passed to fb_start(), the original read() function is called.  */

int fb_read (int fd, void *buf, int nbyte)
{
  if (fd != fb_fd)
    return read (fd, buf, nbyte);
  if (nbyte > fb_size - fb_pos)
    nbyte = fb_size - fb_pos;
  bcopy (fb_buf + fb_pos, buf, nbyte);
  fb_pos += nbyte;
  return nbyte;
}


/* Replace lseek() for `faked binary files'.  If FD is not the handle
   passed to fb_start(), the original lseek() function is called.  */

long fb_lseek (int fd, long offset, int origin)
{
  if (fd != fb_fd)
    return lseek (fd, offset, origin);
  if (origin != SEEK_SET)
    abort ();
  if (offset < 0 || offset > fb_size)
    return (-1);
  fb_pos = offset;
  return fb_pos;
}


/* Replace close() for `faked binary files'.  If FD is not the handle
   passed to fb_start(), the original close() function is called.
   This function releases the memory allocated by fb_start() and
   removes the special meaning of FD for the functions above.  */

int fb_close (int fd)
{
  if (fd == fb_fd)
    {
      xfree (fb_buf);
      fb_fd = -1;
    }
  return close (fd);
}


static Lisp_Object emx_build_case_fold_string (const unsigned char *p)
{
  unsigned char *down;
  size_t i, j, len;
  Lisp_Object obj, start, end;

  len = strlen (p);

  if (NILP (Vcase_fold_file_names))
      /* make_string does not take _const_ unsigned char *   JMB */
      return make_string ((unsigned char *)p, len);

  down = alloca (len + 1);
  memcpy (down, p, len + 1);
  _nls_strlwr (down);
  obj = make_string (down, len);
  if (!EQ (Vcase_fold_file_names, Qpreserve))
    return obj;
  i = 0;
  if (_fngetdrive (p) != 0)
    i += 2;
  while (i < len)
    if (p[i] == down[i])
      ++i;
    else
      {
        j = i++;
        while (i < len && p[i] != down[i])
          ++i;
        XSETFASTINT (start, j);
        XSETFASTINT (end, i);
        Fput_text_property (start, end, Qemx_case_map, Qt, obj);
      }
  return obj;
}


Lisp_Object emx_get_original_case_string (Lisp_Object obj)
{
  Lisp_Object start, end, next;
  size_t i, e, len;
  unsigned char *str;

  if (!EQ (Vcase_fold_file_names, Qpreserve))
    return obj;
  len = XSTRING (obj)->size;
  XSETFASTINT (start, 0);
  XSETFASTINT (end, len);
  start = Ftext_property_not_all (start, end, Qemx_case_map, Qnil, obj);
  if (NILP (start))
    return obj;
  str = alloca (len + 1);
  memcpy (str, XSTRING (obj)->data, len + 1);
  do
    {
      next = Fnext_single_property_change (start, Qemx_case_map, obj, Qnil);
      if (NILP (next))
        XSETFASTINT (next, len);
      e = XINT (next);
      for (i = XINT (start); i < e; ++i)
        str[i] = _nls_toupper (str[i]);
      start = Fnext_single_property_change (next, Qemx_case_map, obj, Qnil);
    } while (!NILP (start));
  return make_string (str, len);
}


DEFUN ("emx-original-file-name", Femx_original_file_name, Semx_original_file_name,
  1, 1, 0,
  "Return the original file name for FILENAME, with case mapping undone.\n\
Just return FILENAME if `preserve-file-name-case' is nil.\n\
This function uses the text properties set by `expand-file-name' if\n\
`preserve-file-name-case' is non-nil.")
  (filename)
     Lisp_Object filename;
{
  CHECK_STRING (filename, 0);
  return emx_get_original_case_string (filename);
}


/* Match "^/[^/:]*[^/:.]:" to check for ange-ftp path. */

static int emx_ange_ftp_p (const unsigned char *p)
{
  if (p[0] != '/' || p[1] == ':')
    return 0;
  do
    {
      ++p;
    } while (*p != 0 && *p != '/' && *p != ':');
  return *p == ':' && p[-1] != '.';
}


#define UNC_P(s) (IS_DIRECTORY_SEP ((s)[0]) && IS_DIRECTORY_SEP ((s)[1]) && !IS_DIRECTORY_SEP ((s)[2]))

Lisp_Object emx_expand_file_name (Lisp_Object name, Lisp_Object defalt)
{
  const unsigned char *nm, *p;
  unsigned char *q, *result;
  int trailing_slash, drive, size;

  name = emx_get_original_case_string (name);
  nm = XSTRING (name)->data;

  /* Use the Unix version of expand-file-name for ange-ftp. */

  if (remote_expand_file_name || emx_ange_ftp_p (nm))
    return Qnil;

  /* Check for a trailing slash (which will be preserved).  */

  if (*nm == 0)
    trailing_slash = 0;
  else
    {
      p = strchr (nm, 0);
      trailing_slash = IS_DIRECTORY_SEP (p[-1]);
    }
  p = nm;

  /* Expand "~" and "~user".  */

  if (p[0] == '~')
    {
      unsigned char *tem;

      ++p;
      while (*p != 0 && !IS_DIRECTORY_SEP (*p))
	++p;			/* skip user name ("all users are equal") */
      if (*p != 0)
	++p;			/* skip slash or backslash */
      q = (unsigned char *) egetenv ("HOME");
      if (q == 0 || *q == 0)
        {
          /* HOME not set, use the root directory of the current
             drive.  */

          tem = alloca (strlen (p) + 2);
          tem[0] = '/';
          strcpy (tem + 1, p);
        }
      else
        {
          tem = alloca (strlen (q) + strlen (p) + 2);
          strcpy (tem, q);
          q = strchr (tem, 0);
          while (q != tem && IS_DIRECTORY_SEP (q[-1]))
            --q;
          *q++ = '/';
          strcpy (q, p);
        }
      p = tem;
    }

  if (!_fngetdrive (p) && !UNC_P (p))
    {
      /* There is no drive letter.  Get the drive letter (or the
         complete directory) from the default directory.  */

      unsigned char *tem;

      CHECK_STRING (defalt, 1);
      q = XSTRING (defalt)->data;
      if (IS_DIRECTORY_SEP (*p))
        {
          /* It's an absolute pathname lacking a drive letter.
             Prepend the drive letter of the default directory.  */

          drive = _fngetdrive (q);
          if (drive)
            {
              while (IS_DIRECTORY_SEP (*p))
                ++p;
              tem = alloca (strlen (p) + 4);
              tem[0] = (unsigned char)drive;
              tem[1] = ':';
              tem[2] = '/';
              strcpy (tem + 3, p);
              p = tem;
            }
        }
      else
        {
          /* It's a relative pathname.  Prepend the default directory.  */

          tem = alloca (strlen (q) + strlen (p) + 2);
          strcpy (tem, q);
          q = strchr (tem, 0);
          if (q != tem && !IS_DIRECTORY_SEP (q[-1]))
            *q++ = '/';
          strcpy (q, p);
          p = tem;
        }
    }

  /* Prepend the drive letter (and a slash if there's no slash after
     the drive letter -- a drive letter always denotes the root
     directory).  */

  drive = _fngetdrive (p);
  if (drive)
    p += 2;
  else if (UNC_P (p))
    {
      /* Universal Naming Convention (\\server\path) -- don't prepend
         a drive letter.  */
      drive = -1;
    }
  else
    drive = _getdrive ();

  if (drive != -1)
    {
      unsigned char *tem;

      while (IS_DIRECTORY_SEP (*p))
        ++p;
      tem = alloca (strlen (p) + 4);
      tem[0] = (unsigned char)drive;
      tem[1] = ':';
      tem[2] = '/';
      strcpy (tem + 3, p);
      p = tem;
    }

  /* Now remove "/./" and "/../".  As we've created an absolute
     pathname, the output string of _abspath() is not longer than the
     input string.  _abspath() translates backslashes into forward
     slashes.  */

  size = strlen (p) + 1;
  result = alloca (size + 1);   /* One extra byte for the trailing slash */
  if (_abspath (result, p, size) != 0)
    error ("Internal error in expand-file-name");

  /* Add or remove the trailing slash.  */

  q = strchr (result, 0);
  if (q != result && IS_DIRECTORY_SEP (q[-1]))
    {
      if (!trailing_slash && !(_fngetdrive (result) && q == result + 3))
        q[-1] = 0;              /* Remove the trailing slash */
    }
  else if (trailing_slash)
    {
      q[0] = '/'; q[1] = 0;     /* Add a trailing slash */
    }

  /* Translate to lower case and build a Lisp string.  */

  return emx_build_case_fold_string (result);
}


Lisp_Object find_program_name_handler (Lisp_Object filename)
{
  /* This function must not munge the match data.  */
  Lisp_Object chain;
  char *tem, *p;

  CHECK_STRING (filename, 0);

  /* Use a shortcut.  */

  if (NILP (Vprogram_name_handler_alist))
    return Qnil;

  /* Convert the string to lower case and replace backslashes with
     forward slashes.  This simplifies the regexps considerably.  */

  tem = (char *)alloca (XSTRING (filename)->size + 1);
  strcpy (tem, XSTRING (filename)->data);
  _nls_strlwr (tem);
  for (p = tem; *p != 0; ++p)
    if (*p == '\\')
      *p = '/';
  filename = build_string (tem);

  for (chain = Vprogram_name_handler_alist; CONSP (chain);
       chain = XCONS (chain)->cdr)
    {
      Lisp_Object elt;
      elt = XCONS (chain)->car;
      if (CONSP (elt))
	{
	  Lisp_Object string;
	  string = XCONS (elt)->car;
	  if (STRINGP (string)
	      && fast_string_match (string, filename) >= 0
              && NILP (Fmemq (XCONS (elt)->cdr,
                              Vinhibit_program_name_handlers)))
            return XCONS (elt)->cdr;
	}
      QUIT;
    }
  return Qnil;
}


/* Set the priority of process or thread ID (depending on SCOPE) to
   the values found in PRTY.  */

static void set_priority (const struct priority *prty, ULONG scope, ULONG id)
{
  if (_osmode == OS2_MODE)
    {
      DosSetPriority (scope, prty->pclass, -31, id);
      if (prty->plevel != 0)
        DosSetPriority (scope, prty->pclass, prty->plevel, id);
    }
}


/* Decode the priority class and level given by PCLASS and PLEVEL,
   respectively.  Store the result to *PRTY.  Signal an error if input
   is invalid.  */

static void decode_priority (struct priority *prty, Lisp_Object pclass,
                             Lisp_Object plevel)
{
  if (NILP (pclass))
    prty->pclass = PRTYC_NOCHANGE;
  else if (EQ (pclass, intern ("idle-time")))
    prty->pclass = PRTYC_IDLETIME;
  else if (EQ (pclass, intern ("regular")))
    prty->pclass = PRTYC_REGULAR;
  else if (EQ (pclass, intern ("foreground-server")))
    prty->pclass = PRTYC_FOREGROUNDSERVER;
  else
    error ("Invalid priority class");
  if (NILP (plevel))
    prty->plevel = 0;
  else
    {
      CHECK_NUMBER (plevel, 1);
      prty->plevel = XINT (plevel);
      if (prty->plevel < 0 || prty->plevel > 31)
        error ("Invalid priority level");
    }
}


/* Check the value of process-priority and store the priority in
   child_priority.  This is done before starting the process as
   signalling an error after successfully starting the child process
   isn't a good idea.  */

void check_process_priority (void)
{
  if (CONSP (Vprocess_priority))
    decode_priority (&child_priority, Fcar (Vprocess_priority),
                     Fcdr (Vprocess_priority));
  else
    decode_priority (&child_priority, Vprocess_priority, Qnil);
}


/* Stolen from child_setup of callproc.c and hacked severly.  */

int emx_child_setup (in, out, err, new_argv, set_pgrp, current_dir)
     int in, out, err;
     register char **new_argv;
     int set_pgrp;
     Lisp_Object current_dir;
{
  int saved_in, saved_out, saved_err;
  int saved_errno;
  char *org_cwd = 0;
  char org_cwd_buf[512];
  char **env, *p;
  int pid, mode;

  {
    register unsigned char *temp;
    register int i;

    i = XSTRING (current_dir)->size;
    temp = (unsigned char *) alloca (i + 2);
    bcopy (XSTRING (current_dir)->data, temp, i);
    if (i > 1 && IS_DIRECTORY_SEP (temp[i-1]) && temp[i-2] != ':')
      --i;
    temp[i] = 0;
    org_cwd = _getcwd2 (org_cwd_buf, sizeof (org_cwd_buf));
    _chdir2 (temp);
  }

  /* Set `env' to a vector of the strings in Vprocess_environment.  */
  {
    register Lisp_Object tem;
    register char **new_env;
    register int new_length;

    new_length = 0;
    for (tem = Vprocess_environment;
         CONSP (tem) && STRINGP (XCONS (tem)->car);
         tem = XCONS (tem)->cdr)
      new_length++;

    /* new_length + 1 to include terminating 0 */
    env = new_env = (char **) alloca ((new_length + 1) * sizeof (char *));

    /* Copy the Vprocess_alist strings into new_env.  */
    for (tem = Vprocess_environment;
         CONSP  (tem) && STRINGP (XCONS (tem)->car);
	 tem = XCONS (tem)->cdr)
      *new_env++ = (char *) XSTRING (XCONS (tem)->car)->data;
    *new_env = 0;
  }

  saved_in = dup (0);
  saved_out = dup (1);
  saved_err = dup (2);

  if (saved_in == -1 || saved_out == -1 || saved_err == -1)
    {
      if (saved_in != -1) close (saved_in);
      if (saved_out != -1) close (saved_out);
      if (saved_err != -1) close (saved_err);
      return -1;
    }

  fcntl (saved_in, F_SETFD, 1);
  fcntl (saved_out, F_SETFD, 1);
  fcntl (saved_err, F_SETFD, 1);

  close (0);
  close (1);
  close (2);

  dup2 (in, 0);
  dup2 (out, 1);
  dup2 (err, 2);

  /* Close Emacs's descriptors that this process should not have.  */
  close_process_descs ();

  /* Set the priority.  Note that check_process_priority must have
     been called.  Changing the priority after starting the process
     yields ERROR_NOT_DESCENDANT, therefore we let the child process
     inherit the priority.  */
  if (!NILP (Vprocess_priority))
    set_priority (&child_priority, PRTYS_THREAD, 0);

  if (_osmode == OS2_MODE && emx_start_session)
    mode = P_SESSION | P_MINIMIZE | P_BACKGROUND;
  else
    mode = P_NOWAIT;
  pid = spawnvpe (mode, new_argv[0], new_argv, env);
  saved_errno = errno;

  /* Restore our priority.  */
  if (!NILP (Vprocess_priority))
    set_priority (&emacs_priority, PRTYS_THREAD, 0);

  dup2 (saved_in, 0); close (saved_in);
  dup2 (saved_out, 1); close (saved_out);
  dup2 (saved_err, 2); close (saved_err);
  if (err != out)
    close (err);
  if (in == out)                /* pty */
    close (out);
  if (org_cwd != 0)
    _chdir2 (org_cwd);
  errno = saved_errno;
  return pid;
}


void emx_proc_input_pipe (int fd)
{
  if (fd >= 0 && binary_process_input)
    setmode (fd, O_BINARY);
}


void emx_proc_output_pipe (int fd)
{
  if (fd >= 0 && binary_process_output)
    setmode (fd, O_BINARY);
}


void emx_setup_start_process (void)
{
  if (!binary_process_input && binary_async_process_input)
    specbind (Qbinary_process_input, Qt);
}


/* emx 0.9b (revision index 40) does not support tcsetattr() for PTYs.
   Turn off ISIG and OPOST by using DosDevIOCtl. */

#define IOCTL_XF86SUP           0x76
#define XF86SUP_TIOCSETA        0x48
#define XF86SUP_TIOCGETA        0x65

struct pt_termios
{
  unsigned short c_iflag;
  unsigned short c_oflag;
  unsigned short c_cflag;
  unsigned short c_lflag;
  unsigned char	c_cc[NCCS];
  long reserved[4];
};

void emx_setup_slave_pty (int fd)
{
  ULONG rc, parm_length, data_length;
  struct pt_termios tio;

  data_length = sizeof (tio);
  rc = DosDevIOCtl (fd, IOCTL_XF86SUP, XF86SUP_TIOCGETA,
                    NULL, 0, NULL,
                    &tio, data_length, &data_length);
  if (rc == 0)
    {
      parm_length = sizeof (tio);
      tio.c_lflag &= ~(ISIG|ECHO);
      tio.c_oflag &= ~OPOST;
      rc = DosDevIOCtl (fd, IOCTL_XF86SUP, XF86SUP_TIOCSETA,
                        &tio, parm_length, &parm_length,
                        NULL, 0, NULL);
    }
}


/* Delete temporary file when a process ends.  Note that
   emx_async_delete_now is called in the SIGCLD handler, and may
   interrupt emx_async_delete_add.  Therefore we don't deallocate or
   modify the list in emx_async_delete_now.  We cannot use the PID as
   delete_temp_file of callproc.c does not know the PID. */

struct async_delete
{
  struct async_delete *next;
  int active, length;
  char *name;
};

static struct async_delete *async_delete_head;

void emx_async_delete_add (char *name)
{
  struct async_delete *p;
  int length;

  length = strlen (name);
  for (p = async_delete_head; p; p = p->next)
    if (!p->active && p->length <= length)
      break;
  if (!p)
    for (p = async_delete_head; p; p = p->next)
      if (!p->active)
        break;
  if (p)
    {
      if (p->length < length)
        {
          free (p->name);
          p->name = (char *)xmalloc (length + 1);
          p->length = length;
        }
      strcpy (p->name, name);
      p->active = 1;
    }
  else
    {
      p = (struct async_delete *)xmalloc (sizeof (struct async_delete));
      p->name = (char *)xmalloc (length + 1);
      p->length = length;
      strcpy (p->name, name);
      p->active = 1;
      p->next = async_delete_head;
      async_delete_head = p;
    }

  /* SIGCLD may occur between the first attempt to delete the file
     (before calling this function) and this point. */

  if (unlink (name) == 0)
    p->active = 0;
}


/* Try to delete the temporary files. */

void emx_async_delete_now (void)
{
  struct async_delete *p;
  int again = 1;

  while (again)
    {
      again = 0;
      for (p = async_delete_head; p; p = p->next)
        if (p->active && !(unlink (p->name) != 0 && errno == EACCES))
          {
            p->active = 0;
            again = 1;
          }
    }
}


void emx_exec_name (char **dst)
{
  PTIB ptib;
  PPIB ppib;
  char name[280], *p;

  if (_osmode == OS2_MODE)
    {
      DosGetInfoBlocks (&ptib, &ppib);
      DosQueryModuleName (ppib->pib_hmte, sizeof (name), name);
      _nls_strlwr (name);
      *dst = strdup (name);
    }
  for (p = *dst; *p != 0; ++p)
    if (*p == '\\')
      *p = '/';
}


DEFUN ("emx-binary-mode-p", Femx_binary_mode_p, Semx_binary_mode_p, 1, 1, 0,
  "Return t if binary mode should be used for FILENAME.\n\
Otherwise, return nil.\n\
Binary mode is used if one of the regular expressions in\n\
`emx-binary-mode-list' match FILENAME.")
  (filename)
    Lisp_Object filename;
{
  /* This function must not munge the match data.  */
  Lisp_Object chain;

  CHECK_STRING (filename, 0);

  for (chain = Vemx_binary_mode_list; CONSP (chain);
       chain = XCONS (chain)->cdr)
    {
      Lisp_Object elt;
      elt = XCONS (chain)->car;
      if (STRINGP (elt) && fast_string_match (elt, filename) >= 0)
        return Qt;
      QUIT;
    }
  return Qnil;
}


#ifdef HAVE_PM

DEFUN ("pm-session-bond", Fpm_session_bond, Spm_session_bond, 1, 1, 0,
  "Establish or break bond between emacs.exe and pmemacs.exe sessions.\n\
Non-nil BOND establishes the bond, nil BOND breaks the bond.")
  (bond)
     Lisp_Object bond;
{
  HSWITCH hSwitch;
  SWCNTRL data;
  STATUSDATA status;

  if (!pm_session_started)
    error ("PM Emacs connection not established");

  hSwitch = WinQuerySwitchHandle (NULLHANDLE, pm_pid);
  status.Length = sizeof (status);
  status.SelectInd = SET_SESSION_UNCHANGED;
  status.BondInd = (NILP (bond) ? SET_SESSION_NO_BOND : SET_SESSION_BOND);
  if (hSwitch != NULLHANDLE && WinQuerySwitchEntry (hSwitch, &data) == 0)
    DosSetSession (data.idSession, &status);
  return Qnil;
}

#endif /* HAVE_PM */

DEFUN ("remove-from-window-list", Fremove_from_window_list,
  Sremove_from_window_list, 0, 0, 0,
  "Remove Emacs from the Window List.")
  ()
{
  HSWITCH hSwitch;

  hSwitch = WinQuerySwitchHandle (NULLHANDLE, getpid ());
  WinRemoveSwitchEntry (hSwitch);
  return Qnil;
}


DEFUN ("filesystem-type", Ffilesystem_type, Sfilesystem_type,
  1, 1, 0,
  "Return a string identifying the filesystem type of PATH.\n\
Filesystem types include FAT, HPFS, LAN, CDFS, NFS and NETWARE.")
  (string)
     Lisp_Object string;
{
  char drive[3], type[16];
  int d;

  CHECK_STRING (string, 0);
  d = _fngetdrive (XSTRING (string)->data);
  if (d == 0)
    d = _getdrive ();
  drive[0] = (char)d;
  drive[1] = ':';
  drive[2] = 0;
  if (_filesys (drive, type, sizeof (type)) != 0)
    error ("_filesys() failed");
  return build_string (type);
}


DEFUN ("file-name-valid-p", Ffile_name_valid_p, Sfile_name_valid_p,
  1, 1, 0,
  "Return t if STRING is a valid file name.\n\
Whether a file name is valid or not depends on the file system.\n\
This is a special feature of GNU Emacs for emx.")
  (string)
     Lisp_Object string;
{
  int i;
  unsigned char *name;

  CHECK_STRING (string, 0);
  name = XSTRING (string)->data;
  if (_osmode == OS2_MODE)
    {
      i = open (name, O_RDONLY);
      if (i >= 0)
        {
          close (i);
          return Qt;
        }
      i = _syserrno ();
      return (i != 15 && i != 123 && i != 206) ? Qt : Qnil;
    }
  else
    {
      if (_fngetdrive (name) != 0)
        name += 2;
      if (*name == 0)
        return Qnil;
      if (strpbrk (name, " \"'*+,:;<=>?[]|") != NULL)
        return Qnil;
      for (i = 0; name[i] != 0; ++i)
        if (name[i] < 0x20)
          return Qnil;
      for (;;)
        {
          i = 0;
          while (*name != 0 && !IS_DIRECTORY_SEP (*name) && *name != '.')
            ++i, ++name;
          if (i > 8)
            return Qnil;
          if (*name == '.')
            {
              ++name;
              if (i == 0)
                {
                  if (*name == '.')
                    ++name;
                  if (*name != 0 && !IS_DIRECTORY_SEP (*name))
                    return Qnil;
                }
              i = 0;
              while (*name != 0 && !IS_DIRECTORY_SEP (*name) && *name != '.')
                ++i, ++name;
              if (i > 3)
                return Qnil;
            }
          if (*name == 0)
            return Qt;
          if (!IS_DIRECTORY_SEP (*name))
            return Qnil;
          ++name;
        }
    }
}


DEFUN ("keyboard-type", Fkeyboard_type, Skeyboard_type,
  0, 0, 0,
  "Return information about the keyboard.\n\
The value is a list of the form (COUNTRY SUBCOUNTRY CODEPAGE), where\n\
  COUNTRY is the country code of the keyboard layout (a string),\n\
    for instance \"US\".\n\
  SUBCOUNTRY is the subcountry code (a string), for instance \"103 \".\n\
  CODEPAGE is the codepage (a number), on which the current keyboard\n\
    translation table is based, for instance 437.\n\
This function is currently implemented under OS/2 only.\n\
If the keyboard information cannot be retrieved (because Emacs is\n\
running under MS-DOS, for instance), nil is returned.")
  ()
{
  ULONG plen, dlen, action;
  HFILE handle;
  struct
    {
      USHORT length;
      USHORT codepage;
      UCHAR strings[8];
    } kd;
  Lisp_Object value;

  value = Qnil;
  if (_osmode == OS2_MODE
      && DosOpen ("KBD$", &handle, &action, 0, 0,
                  OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE,
                  NULL) == 0)
    {
      kd.length = sizeof (kd);
      dlen = sizeof (kd); plen = 0;
      if (DosDevIOCtl (handle, 4, 0x7b, NULL, plen, &plen,
                       &kd, dlen, &dlen) == 0)
        value = Fcons (build_string (kd.strings),
                       Fcons (build_string (strchr (kd.strings, 0) + 1),
                              Fcons (make_number (kd.codepage), Qnil)));
      DosClose (handle);
    }
  return value;
}


DEFUN ("emacs-priority", Femacs_priority, Semacs_priority,
  1, 2, 0,
  "Set the priority of the Emacs process.\n\
PCLASS selects the priority class.  Possible values are\n\
  nil (no change),\n\
  idle-time (idle-time priority class -- you don't want to use this),\n\
  regular (regular priority class -- this is the OS/2 default,\n\
    a priority boost is applied if the process is in the foreground), and\n\
  foreground-server (fixed-high priority class -- use with care).\n\
PLEVEL is nil (same as 0) or a number between 0 and 31 which indicates\n\
  the priority level within the priority class.  Level 31 has the highest\n\
  priority in each class, the default value assigned by OS/2 is 0.\n\
Child processes inherit the priority unless process-priority is non-nil.\n\
This function is implemented under OS/2 only.")
  (pclass, plevel)
     Lisp_Object pclass, plevel;
{
  struct priority prty;

  decode_priority (&prty, pclass, plevel);
  set_priority (&prty, PRTYS_THREAD, 0);
  emacs_priority = prty;
  return Qnil;
}


/* This function is called on program startup.  */

void emx_setup (void)
{
  int min_rev = 40;

  /* Check the emx.dll revision index. */

  if (_osmode == OS2_MODE && _emx_rev < min_rev)
    {
      ULONG rc;
      HMODULE hmod;
      char name[CCHMAXPATH];
      char fail[9];

      fprintf (stderr, "Your emx.dll has revision index %d.\n"
               "Emacs requires revision index %d (version 0.9b, fix00) "
               "or later.\n",
               _emx_rev, min_rev);
      rc = DosLoadModule (fail, sizeof (fail), "emx", &hmod);
      if (rc == 0)
        {
          rc = DosQueryModuleName (hmod, sizeof (name), name);
          if (rc == 0)
            fprintf (stderr, "Please delete or update `%s'.\n", name);
          DosFreeModule (hmod);
        }
      exit (1);
    }

  /* No `faked binary file' is currently in used.  */

  fb_fd = -1;

  /* Initialize the priority to be used after starting a child
     process.  At the moment, I'm too lazy for looking in TIB2...  */

  emacs_priority.pclass = PRTYC_REGULAR;
  emacs_priority.plevel = 0;

  /* Initialize the current code page.  */

  cur_code_page = 0;
  if (_osmode == OS2_MODE)
    {
      ULONG rc, cp, len;

      len = 0;
      rc = DosQueryCp (sizeof (cp), &cp, &len);
      if ((rc == 0 || rc == ERROR_CPLIST_TOO_SMALL) && len == sizeof (cp))
        cur_code_page = cp;
    }
}


DEFUN ("current-code-page", Fcurrent_code_page, Scurrent_code_page, 0, 0, 0,
  "Return the current code page identifier.\n\
The value is nil if the code page identifier cannot be retrieved.\n\
Code pages are not yet supported under MS-DOS.")
  ()
{
  if (_osmode == DOS_MODE || cur_code_page == 0)
    return Qnil;
  return make_number (cur_code_page);
}


DEFUN ("list-code-pages", Flist_code_pages, Slist_code_pages, 0, 0, 0,
  "Return a list of available code pages.\n\
Code pages are not yet supported under MS-DOS.")
  ()
{
  ULONG rc, acp[8], len;
  Lisp_Object list;
  int i;

  if (_osmode == DOS_MODE)
    return Qnil;
#ifdef HAVE_PM
  if (pm_session_started)
    return pm_list_code_pages ();
#else /* not HAVE_PM
  return Qnil;                  /* TODO */
#endif /* not HAVE_PM */
  len = 0;
  rc = DosQueryCp (sizeof (acp), acp, &len);
  if ((rc != 0 && rc != ERROR_CPLIST_TOO_SMALL) || len < sizeof (acp[0]))
    return Qnil;
  list = Qnil;
  for (i = len / sizeof (acp[0]) - 1; i >= 1; --i)
    list = Fcons (make_number (acp[i]), list);
  return list;
}


DEFUN ("set-code-page", Fset_code_page, Sset_code_page, 1, 1, "NCode page: ",
  "Set the code page to CODE-PAGE.\n\
Code pages are not yet supported under MS-DOS.")
  (code_page)
     Lisp_Object code_page;
{
  CHECK_NUMBER (code_page, 0);
  if (_osmode == DOS_MODE)
    error ("Cannot set code page under MS-DOS");
#ifndef HAVE_PM
  error ("Cannot set code page under X");
#else /* HAVE_PM */
  if (XINT (code_page) >= 1 && XINT (code_page) <= 32767)
    {
      USHORT rc, cp;

      cp = XINT (code_page);
      if (cp == cur_code_page)
        return Qnil;
      if (pm_session_started)
        {
          if (pm_set_code_page (cp))
            {
              cur_code_page = cp;
              return Qnil;
            }
        }
      else
        {
          rc = VioSetCp (0, cp, 0);
          if (rc == 0)
            {
              rc = KbdSetCp (0, cp, 0);
              if (rc != 0)
                VioSetCp (0, (USHORT)cur_code_page, 0);
            }
          if (rc == 0)
            {
              cur_code_page = cp;
              return Qnil;
            }
        }
    }
  error ("Invalid code page");
#endif /* HAVE_PM */
}


DEFUN ("set-cursor-size", Fset_cursor_size, Sset_cursor_size, 2, 2,
  "NFirst row: \nLast row: ",
  "Set the text-mode cursor size.\n\
The cursor will occupy rows START through END.  Negative arguments\n\
are percentages.")
  (start, end)
     Lisp_Object start, end;
{
  VIOCURSORINFO vci1, vci2, *pvci;

  CHECK_NUMBER (start, 0);
  CHECK_NUMBER (end, 0);
  if (_osmode == DOS_MODE)
    error ("Cannot set cursor size under MS-DOS");
  if (XINT (start) < -100 || XINT (start) > 31)
    error ("Invalid cursor start row");
  if (XINT (end) < -100 || XINT (end) > 31)
    error ("Invalid cursor end row");
  pvci = _THUNK_PTR_STRUCT_OK (&vci1) ? &vci1 : &vci2;

  if (VioGetCurType (pvci, 0) != 0)
    error ("VioGetCurType failed");
  pvci->yStart = (USHORT)XINT (start);
  pvci->cEnd = (USHORT)XINT (end);
  if (VioSetCurType (pvci, 0) != 0)
    error ("VioSetCurType failed");
  return Qnil;
}


DEFUN ("get-cursor-size", Fget_cursor_size, Sget_cursor_size, 0, 0, 0,
  "Return the text-mode cursor size: (START . END).\n\
START is the start row, END is the end row of the cursor.\n\
Return nil if the cursor size cannot be retrieved.")
  ()
{
  VIOCURSORINFO vci1, vci2, *pvci;

  if (_osmode == DOS_MODE)
    return Qnil;
  pvci = _THUNK_PTR_STRUCT_OK (&vci1) ? &vci1 : &vci2;
  if (VioGetCurType (pvci, 0) != 0)
    return Qnil;
  return Fcons (make_number (pvci->yStart), make_number (pvci->cEnd));
}


static int emx_longname_mismatch (const char *fname, const char *longname)
{
  return _fncmp (_getname (fname), longname) != 0;
}


void emx_fix_longname (const char *fname)
{
  struct _ea ea;
  char *longname;

  if (_ea_get (&ea, fname, -1, ".LONGNAME") == 0 && ea.value != NULL)
    {
      if (ea.size >= 4 && *(unsigned short *)ea.value == EAT_ASCII)
        {
          longname = alloca (ea.size - 4 + 1);
          memcpy (longname, (const char *)ea.value + 4, ea.size - 4);
          longname[ea.size-4] = 0;
          if (emx_longname_mismatch (fname, longname))
            _ea_remove (fname, -1, ".LONGNAME");
        }
      else
        _ea_remove (fname, -1, ".LONGNAME");
    }
}


void emx_fix_ead_longname (_ead ead, const char *fname)
{
  int idx, size;
  const void *val;
  char *longname;

  idx = _ead_find (ead, ".LONGNAME");
  if (idx != -1)
    {
      val = _ead_get_value (ead, idx);
      size = _ead_value_size (ead, idx);
      if (val != NULL && size >= 4 && *(unsigned short *)val == EAT_ASCII)
        {
          longname = alloca (size - 4 + 1);
          memcpy (longname, (const char *)val + 4, size - 4);
          longname[size-4] = 0;
          if (emx_longname_mismatch (fname, longname))
            _ead_delete (ead, idx);
        }
      else
        _ead_delete (ead, idx);
    }
}


static Lisp_Object ea_to_lisp (struct _ea *pea)
{
  Lisp_Object elts[4];
  USHORT type, size;

  if (!pea->value)
    return Qnil;

  elts[0] = make_number (pea->flags);
  if (pea->size < 4)
    elts[1] = elts[2] = Qnil;
  else
    {
      type = ((USHORT *)pea->value)[0];
      size = ((USHORT *)pea->value)[1];
      switch (type)
        {
        case EAT_ASCII:
          elts[1] = Qascii;
          break;
        case EAT_BINARY:
          elts[1] = Qbinary;
          break;
        case EAT_BITMAP:
          elts[1] = Qbitmap;
          break;
        case EAT_METAFILE:
          elts[1] = Qmetafile;
          break;
        case EAT_ICON:
          elts[1] = Qicon;
          break;
        case EAT_EA:
          elts[1] = Qea;
          break;
        case EAT_MVMT:
          elts[1] = Qmvmt;
          break;
        case EAT_MVST:
          elts[1] = Qmvst;
          break;
        case EAT_ASN1:
          elts[1] = Qasn1;
          break;
        default:
          elts[1] = make_number (type);
          break;
        }
      elts[2] = make_number (size);
    }
  elts[3] = make_string (pea->value, pea->size);
  return Fvector (4, elts);
}


static void ea_from_lisp (struct _ea *pea, Lisp_Object vec)
{
  Lisp_Object l_flags, l_type, l_size, l_value;
  int value_size;
  void *value;

  if (XVECTOR (vec)->size != 4)
    error ("EA vector does not have 4 elements");

  l_flags = XVECTOR (vec)->contents[0];
  l_type  = XVECTOR (vec)->contents[1];
  l_size  = XVECTOR (vec)->contents[2];
  l_value = XVECTOR (vec)->contents[3];

  if (!STRINGP (l_value))
    error ("Invalid EA vector");
  value_size = XSTRING (l_value)->size;
  value = alloca (value_size);
  bcopy (XSTRING (l_value)->data, value, value_size);

  if (!INTEGERP (l_flags) || XINT (l_flags) < 0 || XINT (l_flags > 255))
    error ("Invalid EA vector");
  pea->flags = XINT (l_flags);

  if (!NILP (l_type))
    {
      USHORT type;

      if (INTEGERP (l_type))
        {
          if (XINT (l_type) < 0 || XINT (l_type) > 65535)
            error ("Invalid EA vector");
          type = XINT (l_type);
        }
      else if (EQ (l_type, Qascii))
        type = EAT_ASCII;
      else if (EQ (l_type, Qbinary))
        type = EAT_BINARY;
      else if (EQ (l_type, Qbitmap))
        type = EAT_BITMAP;
      else if (EQ (l_type, Qmetafile))
        type = EAT_METAFILE;
      else if (EQ (l_type, Qicon))
        type = EAT_ICON;
      else if (EQ (l_type, Qea))
        type = EAT_EA;
      else if (EQ (l_type, Qmvmt))
        type = EAT_MVMT;
      else if (EQ (l_type, Qmvst))
        type = EAT_MVST;
      else if (EQ (l_type, Qasn1))
        type = EAT_ASN1;
      else
        error ("Invalid EA vector");
      if (value_size >= 4)
        ((USHORT *)value)[0] = type;
    }

  if (!NILP (l_size))
    {
      if (!INTEGERP (l_size) || XINT (l_size) < 0 || XINT (l_size > 65535 - 4))
        error ("Invalid EA vector");
      if (value_size >= 4)
        ((USHORT *)value)[1] = (USHORT)XINT (l_size);
    }
  pea->size = value_size;
  pea->value = xmalloc (value_size);
  bcopy (value, pea->value, value_size);
}


static Lisp_Object
ead_unwind (obj)
     Lisp_Object obj;
{
  _ead ead;
  char *s;

  s = XSTRING (obj)->data;
  sscanf (s, "%p", &ead);
  _ead_destroy (ead);
  return Qnil;
}


static void ead_record_unwind_protect (_ead ead)
{
  char buf[64];

  /* _ead is a pointer type.  Make a string from its value to turn it
     into a Lisp object.  I don't dare to use XPNTR et al.  */
  sprintf (buf, "%p", ead);
  record_unwind_protect (ead_unwind, build_string (buf));
}


DEFUN ("get-ea", Fget_ea, Sget_ea, 2, 2, 0,
  "Get from file or directory PATH the extended attribute NAME.\n\
Return nil if there is no such extended attribute.  Otherwise,\n\
return a vector [FLAGS TYPE SIZE VALUE], where FLAGS is the flags byte\n\
of the extended attribute, TYPE identifies the type of the extended\n\
attribute (`ascii', `binary', `bitmap', `metafile', `icon', `ea',\n\
`mvmt', `mvst', `asn1', or a number), SIZE is the contents of the\n\
size field of the value and VALUE is the (binary) value\n\
of the extended attribute, including the type and size fields.\n\
TYPE and SIZE are nil if the value is too short.")
  (path, name)
     Lisp_Object path, name;
{
  struct _ea ea;
  Lisp_Object result;

  CHECK_STRING (path, 0);
  CHECK_STRING (name, 1);
  path = Fexpand_file_name (path, Qnil);

  if (_ea_get (&ea, XSTRING (path)->data, 0, XSTRING (name)->data) != 0)
    report_file_error ("Getting extended attribute", Fcons (path, Qnil));
  result = ea_to_lisp (&ea);
  _ea_free (&ea);
  return result;
}


DEFUN ("get-ea-string", Fget_ea_string, Sget_ea_string, 2, 2, 0,
  "Get from file or directory PATH the extended attribute NAME.\n\
Return nil if there is no such extended attribute, or if the value\n\
of the extended attribute is not a string.")
  (path, name)
     Lisp_Object path, name;
{
  struct _ea ea;
  Lisp_Object result;

  CHECK_STRING (path, 0);
  CHECK_STRING (name, 1);
  path = Fexpand_file_name (path, Qnil);

  if (_ea_get (&ea, XSTRING (path)->data, 0, XSTRING (name)->data) != 0)
    report_file_error ("Getting extended attribute", Fcons (path, Qnil));
  if (ea.value && ea.size >= 4 && *(USHORT *)ea.value == EAT_ASCII)
    result = make_string (ea.value + 4, ea.size - 4);
  else
    result = Qnil;
  _ea_free (&ea);
  return result;
}


DEFUN ("get-ea-list", Fget_ea_list, Sget_ea_list, 1, 1, 0,
  "Get from file or directory PATH all extended attributes.\n\
Return a list of elements (NAME . VECTOR), one element for each\n\
extended attribute.  NAME is the name of the extended attribute,\n\
VECTOR is a vector [FLAGS TYPE SIZE VALUE], where FLAGS is the flags\n\
byte of the extended attribute, TYPE identifies the type of the extended\n\
attribute (`ascii', `binary', `bitmap', `metafile', `icon', `ea',\n\
`mvmt', `mvst', `asn1', or a number), SIZE is the contents of the\n\
size field of the value and VALUE is the (binary) value\n\
of the extended attribute, including the type and size fields.\n\
TYPE and SIZE are nil if the value is too short.")
  (path)
     Lisp_Object path;
{
  _ead ead;
  struct _ea ea;
  Lisp_Object result, vec, name;
  int i, n;
  int count = specpdl_ptr - specpdl;

  CHECK_STRING (path, 0);
  path = Fexpand_file_name (path, Qnil);

  ead = _ead_create ();
  if (!ead)
    report_file_error ("Getting extended attribute", Fcons (path, Qnil));

  ead_record_unwind_protect (ead);

  if (_ead_read (ead, XSTRING (path)->data, 0, 0) != 0)
    report_file_error ("Getting extended attribute", Fcons (path, Qnil));

  result = Qnil;
  n = _ead_count (ead);
  for (i = n; i >= 1; --i)
    {
      ea.flags = _ead_get_flags (ead, i);
      ea.size = _ead_value_size (ead, i);
      ea.value = (void *)_ead_get_value (ead, i);
      vec = ea_to_lisp (&ea);
      /* build_string does not take _const_ char * JMB */
      name = build_string ((char *)_ead_get_name (ead, i));
      result = Fcons (Fcons (name, vec), result);
    }

  return unbind_to (count, result);
}


DEFUN ("put-ea", Fput_ea, Sput_ea, 3, 3, 0,
  "Attach an extended attribute to the file or directory PATH.\n\
NAME is the name of the extended attribute.  DATA is a vector\n\
[FLAGS TYPE SIZE VALUE] containing the value, where FLAGS is the\n\
flags byte of the extended attribute, TYPE identifies the type of\n\
the extended attribute (`ascii', `binary', `bitmap', `metafile',\n\
`icon', `ea', `mvmt', `mvst', `asn1', or a number), SIZE is the\n\
size the value and VALUE is the (binary) value of the extended\n\
attribute, including the type and size fields.  If TYPE is non-nil,\n\
it will override the type field of VALUE.  If SIZE is non-nil, it\n\
will override the size field of VALUE.")
  (path, name, value)
     Lisp_Object path, name, value;
{
  struct _ea ea;

  CHECK_STRING (path, 0);
  CHECK_STRING (name, 1);
  CHECK_VECTOR (value, 2);
  path = Fexpand_file_name (path, Qnil);

  ea_from_lisp (&ea, value);
  if (_ea_put (&ea, XSTRING (path)->data, 0, XSTRING (name)->data) != 0)
    {
      xfree (ea.value);
      report_file_error ("Attaching extended attribute", Fcons (path, Qnil));
    }
  xfree (ea.value);
  return Qnil;
}


DEFUN ("put-ea-list", Fput_ea_list, Sput_ea_list, 2, 4, 0,
  "Attach a list of extended attributes to the file or directory PATH.\n\
EAS is a list of elements (NAME . VECTOR), one element for each\n\
extended attribute.  NAME is the name of the extended attribute,\n\
VECTOR is a vector [FLAGS TYPE SIZE VALUE], where FLAGS is the flags\n\
byte of the extended attribute, TYPE identifies the type of the extended\n\
attribute (`ascii', `binary', `bitmap', `metafile', `icon', `ea', `mvmt',\n\
`mvst', `asn1', or a number), SIZE is the size the value and VALUE is\n\
the (binary) value of the extended attribute, including the type and size\n\
fields.  If TYPE is non-nil, it will override the type field of VALUE.\n\
If SIZE is non-nil, it will override the size field of VALUE.\n\
If the optional third argument REPLACE is non-nil, the existing extended\n\
attributes of the file or directory are removed before attaching the\n\
new extended attributes.\n\
If the optional fourth argument FIXLONGNAME is non-nil, the .LONGNAME\n\
extended attribute will be removed if it doesn't match the file name.")
  (path, eas, replace, fixlongname)
     Lisp_Object path, eas, replace, fixlongname;
{
  _ead ead;
  struct _ea ea;
  Lisp_Object elt, name, vec;
  int rc;
  int count = specpdl_ptr - specpdl;

  CHECK_STRING (path, 0);
  path = Fexpand_file_name (path, Qnil);

  ead = _ead_create ();
  if (!ead)
    report_file_error ("Attaching extended attributes", Fcons (path, Qnil));

  ead_record_unwind_protect (ead);

  while (CONSP (eas))
    {
      elt = XCONS (eas)->car;
      CHECK_CONS (elt, 1);
      name = XCONS (elt)->car;
      vec = XCONS (elt)->cdr;
      CHECK_STRING (name, 1);
      CHECK_VECTOR (vec, 1);
      ea_from_lisp (&ea, vec);
      /* When using emx 0.8h, this requires the C library of emxfix07.  */
      rc = _ead_add (ead, XSTRING (name)->data, ea.flags, ea.value, ea.size);
      xfree (ea.value);
      if (rc < 0)
        report_file_error ("Attaching extended attributes",
                           Fcons (path, Qnil));
      eas = XCONS (eas)->cdr;
    }
  if (!NILP (eas))
    CHECK_CONS (eas, 1);

  if (!NILP (fixlongname))
    emx_fix_ead_longname (ead, XSTRING (path)->data);

  if (_ead_write (ead, XSTRING (path)->data, 0,
                  NILP (replace) ? _EAD_MERGE : 0) != 0)
    report_file_error ("Attaching extended attributes", Fcons (path, Qnil));

  if (NILP (replace) && !NILP (fixlongname))
    emx_fix_longname (XSTRING (path)->data);

  return unbind_to (count, Qnil);
}


DEFUN ("put-ea-string", Fput_ea_string, Sput_ea_string, 3, 3, 0,
  "Attach an extended attribute (string) to the file or directory PATH.\n\
NAME is the name of the extended attribute.  VALUE is a string\n\
containing the value.  The flags byte of the extended attribute is\n\
set to 0, the type is set to `ascii', and the size is computed from\n\
the length of the string.")
  (path, name, value)
     Lisp_Object path, name, value;
{
  struct _ea ea;
  char *str;
  int size;

  CHECK_STRING (path, 0);
  CHECK_STRING (name, 1);
  CHECK_STRING (value, 2);
  path = Fexpand_file_name (path, Qnil);

  size = XSTRING (value)->size;
  if (size > 65535 - 4)
    error ("Value of extended attribute too long");
  str = alloca (4 + size);
  bcopy (XSTRING (value)->data, str + 4, size);
  ((USHORT *)str)[0] = EAT_ASCII;
  ((USHORT *)str)[1] = (USHORT)size;
  ea.flags = 0;
  ea.size = 4 + size;
  ea.value = str;
  if (_ea_put (&ea, XSTRING (path)->data, 0, XSTRING (name)->data) != 0)
    report_file_error ("Attaching extended attribute", Fcons (path, Qnil));
  return Qnil;
}


DEFUN ("remove-ea", Fremove_ea, Sremove_ea, 2, 2, 0,
  "Remove extended attribute NAME from the file or directory PATH.")
  (path, name)
     Lisp_Object path, name;
{
  CHECK_STRING (path, 0);
  CHECK_STRING (name, 1);
  path = Fexpand_file_name (path, Qnil);

  if (_ea_remove (XSTRING (path)->data, 0, XSTRING (name)->data) != 0)
    report_file_error ("Removing extended attribute", Fcons (path, Qnil));
  return Qnil;
}


DEFUN ("remove-all-eas", Fremove_all_eas, Sremove_all_eas, 1, 1, 0,
  "Remove all extended attribute from the file or directory PATH.")
  (path)
     Lisp_Object path;
{
  _ead ead;
  int rc;

  CHECK_STRING (path, 0);
  path = Fexpand_file_name (path, Qnil);

  ead = _ead_create ();
  if (!ead)
    report_file_error ("Removing all extended attributes", Fcons (path, Qnil));
  rc = _ead_write (ead, XSTRING (path)->data, 0, 0);
  _ead_destroy (ead);
  if (rc != 0)
    report_file_error ("Removing all extended attributes", Fcons (path, Qnil));
  return Qnil;
}


void emx_resize_vio (int *pheight, int *pwidth)
{
  int w, h;
  VIOMODEINFO vmi;
  ULONG rc;

  if (_osmode == DOS_MODE) return;
  h = *pheight; w = *pwidth;
  if (w <= 40)
    w = 40;
  else if (w <= 80)
    w = 80;
  else
    w = 132;
  if (h <= 2)
    h = 2;

  vmi.cb = sizeof (vmi);
  rc = VioGetMode (&vmi, 0);
  if (rc != 0) return;
  vmi.col = w;
  vmi.row = h;
  rc = VioSetMode (&vmi, 0);
  if (rc != 0) return;
  rc = VioGetMode (&vmi, 0);
  if (rc != 0) return;
  *pwidth = vmi.col;
  *pheight = vmi.row;
}


void
syms_of_emxdep ()
{
  Qascii = intern ("ascii");
  staticpro (&Qascii);
  Qbinary = intern ("binary");
  staticpro (&Qbinary);
  Qbinary_process_input = intern ("binary-process-input");
  staticpro (&Qbinary_process_input);
  Qbitmap = intern ("bitmap");
  staticpro (&Qbitmap);
  Qmetafile = intern ("metafile");
  staticpro (&Qmetafile);
  Qea = intern ("ea");
  staticpro (&Qea);
  Qmvmt = intern ("mvmt");
  staticpro (&Qmvmt);
  Qmvst = intern ("mvst");
  staticpro (&Qmvst);
  Qasn1 = intern ("asn1");
  staticpro (&Qasn1);
  Qemx_case_map = intern ("emx-case-map");
  staticpro (&Qemx_case_map);
  Qpreserve = intern ("preserve");
  staticpro (&Qpreserve);

  DEFVAR_LISP ("emx-binary-mode-list", &Vemx_binary_mode_list,
    "*List of regular expressions.  Binary mode is used for files\n\
matching one of the regular expressions.");
  Vemx_binary_mode_list = Qnil;

  DEFVAR_BOOL ("binary-process-input", &binary_process_input,
    "*Non-nil means write process input in binary mode.\n\
nil means use file type of the buffer for writing process input.\n\
binary-process-input is examined by call-process and start-process.");
  binary_process_input = 0;

  DEFVAR_BOOL ("binary-process-output", &binary_process_output,
    "*Non-nil means read process output in binary mode.\n\
nil means use file type of the buffer for reading process output.\n\
binary-process-output is examined by call-process and start-process.");
  binary_process_output = 0;

  DEFVAR_BOOL ("binary-async-process-input", &binary_async_process_input,
    "*Default value for `binary-process-input' if `binary-process-input'\n\
is nil; used for `start-process' only.");
  binary_async_process_input = 1;

  DEFVAR_LISP ("emx-system-type", &Vemx_system_type,
    "Underlying operating system (or program loader).\n\
Possible values are `os2' and `ms-dos'.");
  Vemx_system_type = intern (_osmode == OS2_MODE ? "os2" : "ms-dos");

  DEFVAR_LISP ("process-priority", &Vprocess_priority,
    "Priority to be assigned to child processes.\n\
Possible values are nil (child processes inherit the priority of Emacs),\n\
a priority class, or (PCLASS . PLEVEL), where PCLASS is a priority\n\
class and PLEVEL is a priority level.  Available priority classes are:\n\
  idle-time (idle-time priority class),\n\
  regular (regular priority class -- this is the OS/2 default,\n\
    a priority boost is applied if the process is in the foreground), and\n\
  foreground-server (fixed-high priority class).\n\
If PLEVEL is omitted or is nil, a priority level of 0 is used.\n\
Otherwise, PLEVEL shall be a number between 0 and 31 which indicates\n\
  the priority level within the priority class.  Level 31 has the highest\n\
  priority in each class, the default value assigned by OS/2 is 0.");
  Vprocess_priority = Qnil;

  DEFVAR_LISP ("case-fold-file-names", &Vcase_fold_file_names,
    "*`t' if all file names should be translated to lower case.\n\
`nil' if file names should not be translated to lower case.\n\
`preserve' if `expand-file-name' should use text attributes to preserve\n\
case in file names translated to lower case.  This feature is experimental.");
  Vcase_fold_file_names = Qt;

  DEFVAR_LISP ("program-name-handler-alist", &Vprogram_name_handler_alist,
    "*Alist of elements (REGEXP . HANDLER) for programs handled specially.\n\
If a file name matches REGEXP, then HANDLER is called to take over the work\n\
of call-process and start-process.\n\
\n\
The first argument given to HANDLER is a list containing the name of the\n\
primitive to be handled and the arguments of the primitive.\n\
For call-process, the list is (call-process INFILE BUFFER DISPLAY);\n\
for start-process, the list is (start-process NAME BUFFER).  The second\n\
argument passed to HANDLER is the file name of the program; the third\n\
argument is a list of the program arguments.  For example, if you do\n\
    (call-process PROGRAM \"infile\" t nil ARG1 ARG2)\n\
and PROGRAM is handled by HANDLER, then HANDLER is called like this:\n\
    (funcall HANDLER '(call-process \"infile\" t nil) PROGRAM '(ARG1 ARG2))\n\
HANDLER may modify the structure of its first and third arguments.");
  Vprogram_name_handler_alist = Qnil;

  DEFVAR_LISP ("inhibit-program-name-handlers", &Vinhibit_program_name_handlers,
    "A list of program name handlers that temporarily should not be used.");
  Vinhibit_program_name_handlers = Qnil;

  DEFVAR_BOOL ("emx-start-session", &emx_start_session,
    "*Non-nil to let `start-process' create separate sessions.\n\
Nil to let `start-process' run processes in the same session as Emacs.");
  emx_start_session = 1;

  DEFVAR_BOOL ("remote-expand-file-name", &remote_expand_file_name,
    "Non-nil to prevent `expand-file-name' from prepending a drive letter.");
  remote_expand_file_name = 0;

  DEFVAR_BOOL ("emx-kill-tree", &emx_kill_tree,
    "Non-nil if signals for process groups should be sent to process tree.\n\
Nil to send signals to the target process only.");
  emx_kill_tree = 1;

  defsubr (&Scurrent_code_page);
  defsubr (&Slist_code_pages);
  defsubr (&Sset_code_page);
  defsubr (&Sset_cursor_size);
  defsubr (&Sget_cursor_size);
  defsubr (&Semacs_priority);
  defsubr (&Sfilesystem_type);
  defsubr (&Skeyboard_type);
  defsubr (&Sfile_name_valid_p);
  defsubr (&Sremove_from_window_list);
#ifdef HAVE_PM
  defsubr (&Spm_session_bond);
#endif /* HAVE_PM */
  defsubr (&Semx_binary_mode_p);
  defsubr (&Sget_ea);
  defsubr (&Sget_ea_string);
  defsubr (&Sget_ea_list);
  defsubr (&Sput_ea);
  defsubr (&Sput_ea_string);
  defsubr (&Sput_ea_list);
  defsubr (&Sremove_ea);
  defsubr (&Sremove_all_eas);
  defsubr (&Semx_original_file_name);
}
@
