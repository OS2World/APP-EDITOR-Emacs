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
@/* This file is loaded before crt0.o on machines where we do not
   remap part of the data space into text space in unexec.
   On these machines, there is no problem with standard crt0.o's
   that make environ an initialized variable.  However, we do
   need to make sure the label data_start exists anyway.  */

/* Create a label to appear at the beginning of data space.  */

int data_start = 0;
@
