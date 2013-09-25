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
@/*
    Structure for storing VMS specific information for an EMACS process

    We use the event flags 1-23 for processes, keyboard input and timer
*/

/*
    Same as MAXDESC in process.c
*/
#define	MAX_EVENT_FLAGS		23

typedef  struct {
    char	inputBuffer[1024];
    short	inputChan;
    short	outputChan;
    short	busy;
    int		pid;
    int		eventFlag;
    int		exitStatus;
    short       iosb[4];
} VMS_PROC_STUFF;
@
