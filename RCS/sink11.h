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
@#define sink_width 48
#define sink_height 48
static char sink_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
   0xff, 0xff, 0xff, 0xff, 0x80, 0x9f, 
   0xff, 0xff, 0xff, 0xff, 0x9f, 0x9f, 
   0xff, 0xff, 0xff, 0xff, 0x00, 0x80, 
   0xff, 0xff, 0xff, 0x7f, 0xfe, 0xbf, 
   0xff, 0xff, 0xff, 0x7f, 0x03, 0xa0, 
   0xff, 0xff, 0xff, 0x7f, 0xfd, 0xaf, 
   0xff, 0xff, 0xff, 0x3f, 0xf9, 0xaf, 
   0xff, 0xff, 0xff, 0xff, 0xff, 0xaf, 
   0xff, 0xff, 0xff, 0xff, 0xfc, 0xaf, 
   0xff, 0xff, 0xff, 0x7f, 0xf8, 0xaf, 
   0xff, 0xff, 0xff, 0xff, 0xfc, 0xaf, 
   0xff, 0xff, 0xff, 0xff, 0xff, 0xaf, 
   0xff, 0xff, 0xff, 0xbf, 0xf7, 0xaf, 
   0xff, 0xff, 0xff, 0x3f, 0xf3, 0xaf, 
   0xff, 0xff, 0xff, 0xff, 0xfc, 0xaf, 
   0x3f, 0x00, 0x00, 0x00, 0x00, 0x20, 
   0x7f, 0x00, 0x00, 0x00, 0x00, 0xe0, 
   0xdf, 0xf8, 0xff, 0xff, 0xff, 0x07, 
   0xcf, 0xf9, 0x0f, 0xff, 0xff, 0xe7, 
   0xcf, 0xf9, 0xf7, 0xff, 0xff, 0xe7, 
   0xff, 0xf9, 0xf7, 0x63, 0xfb, 0xe7, 
   0xff, 0xf9, 0x37, 0x5a, 0xfb, 0xe7, 
   0xcf, 0xf9, 0xf7, 0x5a, 0xfb, 0xe7, 
   0xcf, 0xf9, 0xf7, 0x5a, 0xf9, 0xe7, 
   0xef, 0xf9, 0x0f, 0xdb, 0xfa, 0xe7, 
   0xff, 0xf9, 0xff, 0xff, 0xff, 0xe7, 
   0xdf, 0xf9, 0xff, 0xff, 0xff, 0xe7, 
   0xcf, 0x19, 0xfc, 0xff, 0xff, 0xe7, 
   0xcf, 0xd9, 0xff, 0xff, 0xff, 0xe7, 
   0xff, 0xd9, 0x47, 0xce, 0x73, 0xe6, 
   0xff, 0x19, 0xb6, 0xb5, 0xad, 0xe7, 
   0xcf, 0xd9, 0xb7, 0xb5, 0x7d, 0xe6, 
   0xc7, 0xd9, 0xb7, 0xb5, 0xed, 0xe5, 
   0xef, 0x19, 0xb4, 0x4d, 0x73, 0xe6, 
   0xff, 0xf1, 0xff, 0xff, 0xff, 0xe3, 
   0xff, 0x03, 0x80, 0x03, 0x00, 0xf0, 
   0xef, 0x07, 0x00, 0x01, 0x00, 0xf8, 
   0xc7, 0xff, 0x3f, 0xf9, 0xff, 0xff, 
   0xe7, 0xff, 0x7f, 0xfd, 0xe0, 0xff, 
   0xff, 0xff, 0x7f, 0x7d, 0xdf, 0xff, 
   0xff, 0xff, 0x7f, 0xbd, 0xb1, 0xff, 
   0xff, 0xff, 0x7f, 0xbb, 0xae, 0xff, 
   0xef, 0xff, 0xff, 0xda, 0xae, 0xff, 
   0xc7, 0xff, 0xff, 0x66, 0xaf, 0xff, 
   0xe7, 0xff, 0xff, 0xbd, 0xaf, 0xff, 
   0xff, 0xff, 0xff, 0xc3, 0xaf, 0xff, 
   0xff, 0xff, 0xff, 0xff, 0xaf, 0xff};
@
