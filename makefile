# Makefile for GNU Emacs.
# Copyright (C) 1985, 87, 88, 93, 94, 95 Free Software Foundation, Inc.

# This file is part of GNU Emacs.

# GNU Emacs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# GNU Emacs is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with GNU Emacs; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

# Modified for emx by Jeremy Bowen, Jun 1999

# Here are the things that we expect ../configure to edit.
# We use $(srcdir) explicitly in dependencies so as not to depend on VPATH.
srcdir=.
VPATH=.
CC=gcc
CPP=cpp
CFLAGS=-g -O2
LN_S=gcc
# Substitute an assignment for the MAKE variable, because
# BSD doesn't have it as a default.


# On Xenix and the IBM RS6000, double-dot gets screwed up.
dot = .
dotdot = ${dot}${dot}
lispsource = ${srcdir}/$(dot)$(dot)/lisp/
libsrc = $(dot)$(dot)/lib-src/
etc = $(dot)$(dot)/etc/
shortnamesdir = $(dot)$(dot)/shortnames/
cppdir = $(dot)$(dot)/cpp/
oldXMenudir = $(dot)$(dot)/oldXMenu/
lwlibdir = $(dot)$(dot)/lwlib/

# Configuration files for .o files to depend on.
M_FILE = ${srcdir}/m/intel386.h
S_FILE = ${srcdir}/s/emx.h
config_h = config.h $(M_FILE) $(S_FILE)

# ========================== start of cpp stuff =======================
CPPFLAGS=
LDFLAGS=-Zcrtdll
C_SWITCH_SYSTEM=
STARTFILES =  
TOOLKIT_DEFINES = -DUSE_PM
ALL_CFLAGS=-Demacs -DHAVE_CONFIG_H $(TOOLKIT_DEFINES) $(MYCPPFLAG) -I. -I${srcdir} ${CFLAGS}
.c.o:
	$(CC) -c $(CPPFLAGS) $(ALL_CFLAGS) $<
XOBJ= pmterm.o pmfns.o pmselect.o pmfaces.o xfaces.o fontset.o
OLDXMENU=
LIBXMENU=
LIBXT= $(LIBW) -lXmu  -lXt $(LIBXTR6) -lXext
X11_LDFLAGS =    
LIBX=
LD = $(CC)
ALL_LDFLAGS =	$(LDFLAGS)
obj=    dispnew.o frame.o scroll.o xdisp.o xmenu.o window.o	charset.o coding.o category.o ccl.o	cm.o term.o $(XOBJ)	emacs.o keyboard.o macros.o keymap.o sysdep.o	buffer.o filelock.o insdel.o marker.o intervals.o textprop.o	minibuf.o fileio.o dired.o filemode.o	cmds.o casetab.o casefiddle.o indent.o search.o regex.o undo.o	alloc.o data.o doc.o editfns.o callint.o	eval.o floatfns.o fns.o print.o lread.o	abbrev.o syntax.o unexecemx.o  mocklisp.o bytecode.o	process.o callproc.o	region-cache.o	doprnt.o strftime.o   getloadavg.o    emxdep.o 
SOME_MACHINE_OBJECTS = emxdep.o intervals.o textprop.o   pmfaces.o pmfns.o pmselect.o pmterm.o xfaces.o xmenu.o
termcapobj = termcap.o tparam.o
mallocobj = gmalloc.o ralloc.o vm-limit.o
allocaobj =
widgetobj=
otherobj= $(termcapobj) lastfile.o $(mallocobj) $(allocaobj) $(widgetobj)
lisp=	${lispsource}abbrev.elc	\
	${lispsource}buff-menu.elc	\
	${lispsource}byte-run.elc	\
	${lispsource}cus-start.el	\
	${lispsource}custom.elc	\
	${lispsource}emacs-lisp/lisp-mode.elc	\
	${lispsource}emacs-lisp/lisp.elc	\
	${lispsource}faces.elc	\
	${lispsource}files.elc	\
	${lispsource}format.elc	\
	${lispsource}facemenu.elc	\
	${lispsource}mouse.elc \
	${lispsource}select.elc \
	${lispsource}scroll-bar.elc	\
	${lispsource}float-sup.elc	\
	${lispsource}frame.elc		\
	${lispsource}help.elc	\
	${lispsource}indent.elc	\
	${lispsource}isearch.elc	\
	${lispsource}loadup.el	\
	${lispsource}loaddefs.el	\
	${lispsource}bindings.el	\
	${lispsource}map-ynp.elc	\
	${lispsource}menu-bar.elc	\
	${lispsource}international/mule.elc	\
	${lispsource}international/mule-conf.el	\
	${lispsource}international/mule-cmds.elc	\
	${lispsource}international/characters.elc	\
	${lispsource}case-table.elc	\
	${lispsource}language/chinese.elc	\
	${lispsource}language/cyrillic.elc	\
	${lispsource}language/english.elc	\
	${lispsource}language/ethiopic.elc	\
	${lispsource}language/european.elc	\
	${lispsource}language/czech.elc	\
	${lispsource}language/slovak.elc	\
	${lispsource}language/romanian.elc	\
	${lispsource}language/greek.elc	\
	${lispsource}language/hebrew.elc	\
	${lispsource}language/japanese.elc	\
	${lispsource}language/korean.elc	\
	${lispsource}language/thai.elc	\
	${lispsource}language/misc-lang.elc	\
	${lispsource}paths.el	\
	${lispsource}register.elc	\
	${lispsource}replace.elc	\
	${lispsource}simple.elc	\
	${lispsource}startup.elc	\
	${lispsource}subr.elc	\
	${lispsource}textmodes/fill.elc	\
	${lispsource}textmodes/page.elc	\
	${lispsource}textmodes/paragraphs.elc	\
	${lispsource}textmodes/text-mode.elc	\
	${lispsource}vc-hooks.elc	\
	${lispsource}ediff-hook.elc		\
	${lispsource}emx-patch.elc \
	${lispsource}emx-funcs.elc \
	${lispsource}emx-keys.el			\
	${lispsource}widget.elc	\
	${lispsource}window.elc	\
	${lispsource}version.el \
	${lispsource}language/indian.elc	\
	${lispsource}language/devanagari.elc	\
	${lispsource}language/lao.elc	\
	${lispsource}language/tibetan.elc	\
	${lispsource}language/vietnamese.elc	\

shortlisp=	\
	../lisp/abbrev.elc	\
	../lisp/buff-menu.elc	\
	../lisp/byte-run.elc	\
	../lisp/cus-start.el	\
	../lisp/custom.elc	\
	../lisp/emacs-lisp/lisp-mode.elc	\
	../lisp/emacs-lisp/lisp.elc	\
	../lisp/faces.elc	\
	../lisp/files.elc	\
	../lisp/format.elc	\
	../lisp/help.elc	\
	../lisp/indent.elc	\
	../lisp/isearch.elc	\
	../lisp/loadup.el	\
	../lisp/loaddefs.el	\
	../lisp/bindings.el	\
	../lisp/map-ynp.elc	\
	../lisp/international/mule.elc	\
	../lisp/international/mule-conf.el	\
	../lisp/international/mule-cmds.elc	\
	../lisp/international/characters.elc	\
	../lisp/case-table.elc	\
	../lisp/language/chinese.elc	\
	../lisp/language/cyrillic.elc	\
	../lisp/language/english.elc	\
	../lisp/language/ethiopic.elc	\
	../lisp/language/european.elc	\
	../lisp/language/czech.elc	\
	../lisp/language/slovak.elc	\
	../lisp/language/romanian.elc	\
	../lisp/language/greek.elc	\
	../lisp/language/hebrew.elc	\
	../lisp/language/japanese.elc	\
	../lisp/language/korean.elc	\
	../lisp/language/thai.elc	\
	../lisp/language/misc-lang.elc	\
	../lisp/paths.el	\
	../lisp/register.elc	\
	../lisp/replace.elc	\
	../lisp/simple.elc	\
	../lisp/startup.elc	\
	../lisp/subr.elc	\
	../lisp/textmodes/fill.elc	\
	../lisp/textmodes/page.elc	\
	../lisp/textmodes/paragraphs.elc	\
	../lisp/textmodes/text-mode.elc	\
	../lisp/vc-hooks.elc	\
	../lisp/ediff-hook.elc	\
	../lisp/widget.elc	\
	../lisp/window.elc	\
	../lisp/version.el \
	../lisp/language/indian.elc	\
	../lisp/language/devanagari.elc	\
	../lisp/language/lao.elc	\
	../lisp/language/tibetan.elc	\
	../lisp/language/vietnamese.elc	\


SOME_MACHINE_LISP = ${dotdot}/lisp/facemenu.elc   ${dotdot}/lisp/float-sup.elc ${dotdot}/lisp/frame.elc   ${dotdot}/lisp/menu-bar.elc ${dotdot}/lisp/mouse.elc   ${dotdot}/lisp/select.elc ${dotdot}/lisp/scroll-bar.elc ${dotdot}/lisp/ls-lisp.elc    ${dotdot}/lisp/w32-fns.elc ${dotdot}/lisp/dos-w32.elc
LIBES = $(LOADLIBES) $(LDLIBS) $(LIBX) -lg  $(GNULIB_VAR) -lm  -lsocket  $(GNULIB_VAR)
all: emacs.exe pmemacs.exe 
emacs.exe: temacs.exe ${etc}DOC ${lisp}
	temacs -batch -l loadup dump
	-./emacs -q -batch -f list-load-path-shadows
${etc}DOC: ${obj} ${lisp}
	-rm -f ${etc}DOC
	..\lib-src\make-docfile -d ${srcdir} ${SOME_MACHINE_OBJECTS} ${obj} > ${etc}DOC
	..\lib-src\make-docfile -a ${etc}DOC -d ${srcdir} ${SOME_MACHINE_LISP} ${shortlisp}
${libsrc}make-docfile:
	cd ${libsrc}; ${MAKE} ${MFLAGS} make-docfile
temacs:   $(LOCALCPP) $(STARTFILES) stamp-oldxmenu ${obj} ${otherobj}     
	$(LD)  ${STARTFLAGS} ${ALL_LDFLAGS} -o temacs ${STARTFILES} ${obj} ${otherobj}          ${LIBES}
prefix-args: prefix-args.c $(config_h)
	$(CC) $(ALL_CFLAGS) $(LDFLAGS) ${srcdir}/prefix-args.c -o prefix-args
CPP = $(CC) -E
stamp-oldxmenu: ${OLDXMENU} ../src/$(OLDXMENU) 
	touch stamp-oldxmenu
../src/$(OLDXMENU): ${OLDXMENU}
oldxmenu.dummy: really-lwlib
C_SWITCH_MACHINE_1 =  
C_SWITCH_SYSTEM_1 =  
C_SWITCH_SITE_1 =  
C_SWITCH_X_SITE_1 =  
C_SWITCH_X_MACHINE_1 =  
C_SWITCH_X_SYSTEM_1 =  
really-lwlib:
	cd ${lwlibdir}; ${MAKE} ${MFLAGS}       CC='${CC}' CFLAGS='${CFLAGS}' MAKE='${MAKE}'     "C_SWITCH_X_SITE=$(C_SWITCH_X_SITE_1)"     "C_SWITCH_X_MACHINE=$(C_SWITCH_X_MACHINE_1)"     "C_SWITCH_X_SYSTEM=$(C_SWITCH_X_SYSTEM_1)"     "C_SWITCH_SITE=$(C_SWITCH_SITE_1)"     "C_SWITCH_MACHINE=$(C_SWITCH_MACHINE_1)"     "C_SWITCH_SYSTEM=$(C_SWITCH_SYSTEM_1)"
	@true   
.PHONY: really-lwlib
../config.status:: paths.in
	@echo "The file epaths.h needs to be set up from paths.in."
	@echo "Please run the `configure' script again."
	exit 1

../config.status:: config.in
	@echo "The file config.h needs to be set up from config.in."
	@echo "Please run the `configure' script again."
	exit 1
abbrev.o: abbrev.c buffer.h window.h commands.h $(config_h)
buffer.o: buffer.c buffer.h region-cache.h commands.h window.h    intervals.h  blockinput.h charset.h $(config_h)
callint.o: callint.c window.h commands.h buffer.h mocklisp.h    keyboard.h $(config_h)
callproc.o: callproc.c epaths.h buffer.h commands.h $(config_h)	process.h systty.h syssignal.h charset.h coding.h 
casefiddle.o: casefiddle.c syntax.h commands.h buffer.h $(config_h)
casetab.o: casetab.c buffer.h $(config_h)
category.o: category.c category.h buffer.h charset.h $(config_h)
ccl.o: ccl.c ccl.h charset.h coding.h $(config_h)
charset.o: charset.c charset.h buffer.h coding.h disptab.h $(config_h)
coding.o: coding.c coding.h ccl.h buffer.h charset.h $(config_h)
cm.o: cm.c cm.h termhooks.h $(config_h)
cmds.o: cmds.c syntax.h buffer.h charset.h commands.h window.h $(config_h)	
pre-crt0.o: pre-crt0.c
ecrt0.o: ecrt0.c $(config_h)
	CRT0_COMPILE ${srcdir}/ecrt0.c
dired.o: dired.c commands.h buffer.h $(config_h) charset.h coding.h regex.h
dispnew.o: dispnew.c  commands.h frame.h window.h buffer.h dispextern.h lisp.h termchar.h termopts.h termhooks.h cm.h disptab.h indent.h intervals.h keyboard.h syssignal.h systty.h systime.h    pmterm.h pmlib.h blockinput.h charset.h  $(config_h)
doc.o: doc.c $(config_h) epaths.h buffer.h keyboard.h
doprnt.o: doprnt.c charset.h $(config_h)
dosfns.o: buffer.h termchar.h termhooks.h frame.h  dosfns.h $(config_h)
editfns.o: editfns.c window.h buffer.h systime.h intervals.h  charset.h    $(config_h)
emacs.o: emacs.c commands.h systty.h syssignal.h blockinput.h process.h    buffer.h intervals.h  $(config_h)
fileio.o: fileio.c window.h buffer.h systime.h intervals.h  charset.h    coding.h  $(config_h)
filelock.o: filelock.c buffer.h epaths.h $(config_h)
filemode.o: filemode.c  $(config_h)
frame.o: frame.c pmterm.h window.h frame.h termhooks.h commands.h keyboard.h    buffer.h charset.h fontset.h  $(config_h)
fontset.o: fontset.h fontset.c ccl.h charset.h frame.h $(config_h)
getloadavg.o: getloadavg.c $(config_h)
indent.o: indent.c frame.h window.h indent.h buffer.h $(config_h) termchar.h    termopts.h disptab.h region-cache.h charset.h
insdel.o: insdel.c window.h buffer.h intervals.h  blockinput.h charset.h   $(config_h)
keyboard.o: keyboard.c termchar.h termhooks.h termopts.h buffer.h charset.h    commands.h frame.h window.h macros.h disptab.h keyboard.h syssignal.h    systty.h systime.h dispextern.h syntax.h intervals.h blockinput.h    pmterm.h puresize.h  $(config_h)
keymap.o: keymap.c buffer.h commands.h keyboard.h termhooks.h blockinput.h    puresize.h charset.h $(config_h)
lastfile.o: lastfile.c  $(config_h)
macros.o: macros.c window.h buffer.h commands.h macros.h keyboard.h $(config_h)
malloc.o: malloc.c $(config_h)
gmalloc.o: gmalloc.c getpagesize.h $(config_h)
ralloc.o: ralloc.c $(config_h)
vm-limit.o: vm-limit.c mem-limits.h $(config_h)
marker.o: marker.c buffer.h charset.h $(config_h)
minibuf.o: minibuf.c syntax.h dispextern.h frame.h window.h    buffer.h commands.h charset.h  $(config_h)
mktime.o: mktime.c $(config_h)
mocklisp.o: mocklisp.c buffer.h $(config_h)
msdos.o: msdos.c  dosfns.h systime.h termhooks.h dispextern.h    termopts.h frame.h window.h $(config_h)
process.o: process.c process.h buffer.h window.h termhooks.h termopts.h    commands.h sysselect.h syssignal.h systime.h systty.h syswait.h frame.h    blockinput.h charset.h coding.h  $(config_h)
regex.o: regex.c syntax.h buffer.h $(config_h) regex.h category.h charset.h
region-cache.o: region-cache.c buffer.h region-cache.h
scroll.o: scroll.c termchar.h dispextern.h frame.h  $(config_h)
search.o: search.c regex.h commands.h buffer.h region-cache.h syntax.h    blockinput.h category.h charset.h $(config_h)
strftime.o: strftime.c $(config_h)
syntax.o: syntax.c syntax.h buffer.h commands.h category.h charset.h    $(config_h)
sysdep.o: sysdep.c $(config_h) dispextern.h termhooks.h termchar.h termopts.h    frame.h syssignal.h systty.h systime.h syswait.h blockinput.h window.h    
term.o: term.c termchar.h termhooks.h termopts.h $(config_h) cm.h frame.h    disptab.h keyboard.h charset.h coding.h 
termcap.o: termcap.c $(config_h)
terminfo.o: terminfo.c $(config_h)
tparam.o: tparam.c $(config_h)
undo.o: undo.c buffer.h commands.h $(config_h)
UNEXEC_ALIAS= unexecemx.o 
$(UNEXEC_ALIAS): unexecemx.c  $(config_h)
w16select.o: w16select.c lisp.h dispextern.h frame.h blockinput.h     $(config_h)
widget.o: widget.c pmterm.h frame.h dispextern.h widgetprv.h    $(srcdir)/../lwlib/lwlib.h $(config_h)
window.o: window.c indent.h commands.h frame.h window.h buffer.h termchar.h    termhooks.h disptab.h keyboard.h dispextern.h  $(config_h)
xdisp.o: xdisp.c macros.h commands.h indent.h buffer.h dispextern.h coding.h ccl.h   termchar.h frame.h window.h disptab.h termhooks.h charset.h region-cache.h $(config_h)
xfaces.o: xfaces.c dispextern.h frame.h pmterm.h buffer.h blockinput.h    window.h charset.h  $(config_h)
xfns.o: xfns.c buffer.h frame.h window.h keyboard.h pmterm.h   $(srcdir)/../lwlib/lwlib.h blockinput.h epaths.h charset.h $(config_h)
xmenu.o: xmenu.c pmterm.h termhooks.h window.h dispextern.h frame.h keyboard.h puresize.h pmemacs.h  $(srcdir)/../lwlib/lwlib.h blockinput.h  $(config_h)
xterm.o: xterm.c pmterm.h termhooks.h termopts.h termchar.h window.h   dispextern.h frame.h disptab.h blockinput.h systime.h syssignal.h   keyboard.h gnu.h sink.h sinkmask.h charset.h ccl.h fontset.h $(config_h)
xselect.o: xselect.c dispextern.h frame.h pmterm.h blockinput.h charset.h   coding.h buffer.h $(config_h)
xrdb.o: xrdb.c $(config_h) epaths.h
hftctl.o: hftctl.c $(config_h)
alloc.o: alloc.c frame.h window.h buffer.h  puresize.h syssignal.h keyboard.h  blockinput.h charset.h $(config_h) intervals.h 
bytecode.o: bytecode.c buffer.h syntax.h $(config_h)
data.o: data.c buffer.h puresize.h charset.h syssignal.h keyboard.h $(config_h)
eval.o: eval.c commands.h keyboard.h blockinput.h $(config_h)
floatfns.o: floatfns.c $(config_h)
fns.o: fns.c commands.h $(config_h) frame.h buffer.h keyboard.h  window.h intervals.h 
print.o: print.c process.h frame.h window.h buffer.h keyboard.h charset.h   $(config_h) dispextern.h 
lread.o: lread.c commands.h keyboard.h buffer.h epaths.h charset.h $(config_h)  termhooks.h 
textprop.o: textprop.c buffer.h window.h intervals.h $(config_h)
intervals.o: intervals.c buffer.h intervals.h keyboard.h puresize.h $(config_h)
sunfns.o: sunfns.c buffer.h window.h $(config_h)

${libsrc}emacstool: ${libsrc}emacstool.c
	cd ${libsrc}; ${MAKE} ${MFLAGS} emacstool
mostlyclean:
	rm -f temacs prefix-args core \#* *.o libXMenu11.a liblw.a
	rm -f ../etc/DOC
clean: mostlyclean
	rm -f emacs-* emacs
distclean: clean
	rm -f epaths.h config.h Makefile Makefile.c config.stamp stamp-oldxmenu ../etc/DOC-*
maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f TAGS
versionclean:
	-rm -f emacs emacs-* ../etc/DOC*
extraclean: distclean
	-rm -f *~ \#* m/?*~ s/?*~

SOURCES = *.[ch] [sm]/?* COPYING Makefile.in \
	config.in epaths.in README COPYING ChangeLog vms.pp-trans
unlock:
	chmod u+w $(SOURCES)

relock:
	chmod -w $(SOURCES)
	chmod +w epaths.h
ctagsfiles = [a-zA-Z]*.[hc]
TAGS: $(srcdir)/$(ctagsfiles)
	../lib-src/etags --include=TAGS-LISP	--regex='/[	]*DEFVAR_[A-Z_	(]+"\([^"]+\)"/'	$(srcdir)/$(ctagsfiles)
frc:
TAGS-LISP: frc
	$(MAKE) -f ${lispsource}Makefile TAGS-LISP ETAGS=../lib-src/etags	lispsource=${lispsource}
tags: TAGS TAGS-LISP
.PHONY: tags
pmemacs.o: pmemacs.c pmemacs.h termhooks.h lisp.h config.h
pmemacs.res: pmemacs.rc emacs.ico
	rc -r pmemacs
pmemacs.exe: pmemacs.o pmemacs.def pmemacs.res
	$(CC) $(ALL_CFLAGS) pmemacs.o -o pmemacs
	emxbind -bqs -dpmemacs.def -rpmemacs.res pmemacs

temacs.exe: temacs
	emxbind -bq -h48 temacs
