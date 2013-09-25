/* System description file for emx running on top of OS/2 2.x */

#ifndef EMX
#define EMX
#endif
#define USG
#define USG5
#define SYSTEM_TYPE "emx"
#undef MULTI_KBOARD
#define NOMULTIPLEJOBS
/* #define INTERRUPT_INPUT */
#define FIRST_PTY_LETTER 'p'
/* #define HAVE_TERMIOS */
#define HAVE_TERMIO
#define HAVE_PTYS
#define HAVE_SOCKETS
/* #define NONSYSTEM_DIR_LIBRARY */
#define BSTRING
#define subprocesses
/* #define COFF */
#define MAIL_USE_FLOCK
/* #define CLASH_DETECTION */
/* #define SIGTYPE int */
/* #define static */
/* #define SYSTEM_MALLOC */
#define HAVE_VFORK
#define	PENDING_OUTPUT_COUNT(FILE) ((FILE)->_ptr - (FILE)->_buffer)
#define START_FILES
#define UNEXEC unexecemx.o
#define UNEXEC_SRC unexecemx.c
#define ORDINARY_LINK
#define LIB_STANDARD -lsocket
#define LD_SWITCH_SYSTEM
#define _setjmp setjmp
#define _longjmp longjmp
#define DATA_START 0
#define SYMS_SYSTEM syms_of_emxdep()
#define NULL_DEVICE "nul"
#define EXEC_SUFFIXES ".exe:.com:"
#define ORDINARY_LINK
#define OS2
#define POSIX_SIGNALS
#define SEPCHAR ';'
#define SYSTEM_PURESIZE_EXTRA 40000
#ifdef HAVE_PM
#define OTHER_FILES pmemacs.exe
#endif
#define HAVE_CBRT
#define HAVE_RINT
#define FLOAT_CHECK_ERRNO
#define TERMCAP_FILE "/emx/etc/termcap.dat"
#define GAP_USE_BCOPY
#define BCOPY_UPWARD_SAFE 1
#define BCOPY_DOWNWARD_SAFE 1

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0
#endif

#define DEVICE_SEP ':'
#define IS_DIRECTORY_SEP(c) ((c) == '/' || (c) == '\\')
#define IS_ANY_SEP(c) (IS_DIRECTORY_SEP (c) || IS_DEVICE_SEP (c))

#define MODE_LINE_BINARY_TEXT(buf) (NILP ((buf)->emx_binary_mode) ? "T" : "B")

#define DECLARE_GETPWUID_WITH_UID_T
#define getpwnam(X) emxdep_getpwnam(X)
#define getpwuid(X) emxdep_getpwuid(X)

#define SETUP_SLAVE_PTY emx_setup_slave_pty (xforkin)
