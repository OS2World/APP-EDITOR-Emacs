head	1.1;
access;
symbols;
locks
	JeremyB:1.1; strict;
comment	@ * @;


1.1
date	2000.10.21.21.12.34;	author JeremyB;	state Exp;
branches;
next	;


desc
@OS/2 Emacs path file.
@


1.1
log
@Initial revision
@
text
@/* Hey Emacs, this is -*- C -*- code!  */

/* The default search path for Lisp function "load".
   This sets load-path.  */
#define PATH_LOADSEARCH "/emacs/20.6/lisp"

/* Like PATH_LOADSEARCH, but used only when Emacs is dumping.  This
   path is usually identical to PATH_LOADSEARCH except that the entry
   for the directory containing the installed lisp files has been
   replaced with ../lisp.  */
#define PATH_DUMPLOADSEARCH "../lisp"

/* The extra search path for programs to invoke.  This is appended to
   whatever the PATH environment variable says to set the Lisp
   variable exec-path and the first file name in it sets the Lisp
   variable exec-directory.  exec-directory is used for finding
   executables and other architecture-dependent files.  */
#define PATH_EXEC "/emacs/20.6/bin"

/* Where Emacs should look for its architecture-independent data
   files, like the NEWS file.  The lisp variable data-directory
   is set to this value.  */
#define PATH_DATA "/emacs/20.6/data"

/* Where Emacs should look for X bitmap files.
   The lisp variable x-bitmap-file-path is set based on this value.  */
#define PATH_BITMAPS "/usr/include/X11/bitmaps"

/* Where Emacs should look for its docstring file.  The lisp variable
   doc-directory is set to this value.  */
#define PATH_DOC "/emacs/20.6/etc"

/* The name of the directory that contains lock files with which we
   record what files are being modified in Emacs.  This directory
   should be writable by everyone.  THE STRING MUST END WITH A
   SLASH!!!  */
#define PATH_LOCK "/emacs/20.6/lock/"

/* Where the configuration process believes the info tree lives.  The
   lisp variable configure-info-directory gets its value from this
   macro, and is then used to set the Info-default-directory-list.  */
#define PATH_INFO "/usr/local/info"
@
