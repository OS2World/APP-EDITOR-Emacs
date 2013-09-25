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
@/* Unexec for the Alliant FX/2800.  */

#include <stdio.h>

unexec (new_name, a_name, data_start, bss_start, entry_address)
     char *new_name, *a_name;
     unsigned data_start, bss_start, entry_address;
{
  int stat;
    
  stat = elf_write_modified_data (a_name, new_name);
  if (stat < 0)
    perror ("emacs: elf_write_modified_data");
  else if (stat > 0)
    fprintf (stderr, "Unspecified error from elf_write_modified_data.\n");
}
@
