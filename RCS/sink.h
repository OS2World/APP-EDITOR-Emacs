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
#ifdef HAVE_X11
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
#else
short sink_bits[] = {
   0xffff, 0xffff, 0xffff, 0xffff,
   0xffff, 0x9f80, 0xffff, 0xffff,
   0x9f9f, 0xffff, 0xffff, 0x8000,
   0xffff, 0x7fff, 0xbffe, 0xffff,
   0x7fff, 0xa003, 0xffff, 0x7fff,
   0xaffd, 0xffff, 0x3fff, 0xaff9,
   0xffff, 0xffff, 0xafff, 0xffff,
   0xffff, 0xaffc, 0xffff, 0x7fff,
   0xaff8, 0xffff, 0xffff, 0xaffc,
   0xffff, 0xffff, 0xafff, 0xffff,
   0xbfff, 0xaff7, 0xffff, 0x3fff,
   0xaff3, 0xffff, 0xffff, 0xaffc,
   0x003f, 0x0000, 0x2000, 0x007f,
   0x0000, 0xe000, 0xf8df, 0xffff,
   0x07ff, 0xf9cf, 0xff0f, 0xe7ff,
   0xf9cf, 0xfff7, 0xe7ff, 0xf9ff,
   0x63f7, 0xe7fb, 0xf9ff, 0x5a37,
   0xe7fb, 0xf9cf, 0x5af7, 0xe7fb,
   0xf9cf, 0x5af7, 0xe7f9, 0xf9ef,
   0xdb0f, 0xe7fa, 0xf9ff, 0xffff,
   0xe7ff, 0xf9df, 0xffff, 0xe7ff,
   0x19cf, 0xfffc, 0xe7ff, 0xd9cf,
   0xffff, 0xe7ff, 0xd9ff, 0xce47,
   0xe673, 0x19ff, 0xb5b6, 0xe7ad,
   0xd9cf, 0xb5b7, 0xe67d, 0xd9c7,
   0xb5b7, 0xe5ed, 0x19ef, 0x4db4,
   0xe673, 0xf1ff, 0xffff, 0xe3ff,
   0x03ff, 0x0380, 0xf000, 0x07ef,
   0x0100, 0xf800, 0xffc7, 0xf93f,
   0xffff, 0xffe7, 0xfd7f, 0xffe0,
   0xffff, 0x7d7f, 0xffdf, 0xffff,
   0xbd7f, 0xffb1, 0xffff, 0xbb7f,
   0xffae, 0xffef, 0xdaff, 0xffae,
   0xffc7, 0x66ff, 0xffaf, 0xffe7,
   0xbdff, 0xffaf, 0xffff, 0xc3ff,
   0xffaf, 0xffff, 0xffff, 0xffaf};
#endif /* HAVE_X11 */
@
