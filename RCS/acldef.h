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
@#define	ACL$K_LENGTH	12
#define	ACL$C_LENGTH	12
#define	ACL$C_FILE	1
#define	ACL$C_DEVICE	2
#define	ACL$C_JOBCTL_QUEUE	3
#define	ACL$C_COMMON_EF_CLUSTER	4
#define	ACL$C_LOGICAL_NAME_TABLE	5
#define	ACL$C_PROCESS	6
#define	ACL$C_GROUP_GLOBAL_SECTION	7
#define	ACL$C_SYSTEM_GLOBAL_SECTION	8
#define	ACL$C_ADDACLENT	1
#define	ACL$C_DELACLENT	2
#define	ACL$C_MODACLENT	3
#define	ACL$C_FNDACLENT	4
#define	ACL$C_FNDACETYP	5
#define	ACL$C_DELETEACL	6
#define	ACL$C_READACL	7
#define	ACL$C_ACLLENGTH	8
#define	ACL$C_READACE	9
#define	ACL$C_RLOCK_ACL	10
#define	ACL$C_WLOCK_ACL	11
#define	ACL$C_UNLOCK_ACL	12
#define	ACL$S_ADDACLENT	255
#define	ACL$S_DELACLENT	255
#define	ACL$S_MODACLENT	255
#define	ACL$S_FNDACLENT	255
#define	ACL$S_FNDACETYP	255
#define	ACL$S_DELETEACL	255
#define	ACL$S_READACL	512
#define	ACL$S_ACLLENGTH	4
#define	ACL$S_READACE	255
#define	ACL$S_RLOCK_ACL	4
#define	ACL$S_WLOCK_ACL	4
#define	ACL$S_UNLOCK_ACL	4
#define	ACL$S_ACLDEF	16
#define	ACL$L_FLINK	0
#define	ACL$L_BLINK	4
#define	ACL$W_SIZE	8
#define	ACL$B_TYPE	10
#define	ACL$L_LIST	12
@