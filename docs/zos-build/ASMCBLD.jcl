//ASMCBLD  JOB (ACCT),'METAL C EXIT BUILD',CLASS=A,MSGCLASS=X,
//             NOTIFY=&SYSUID
//*********************************************************************
//* BUILD ONE CONVERTED METAL C EXIT                                  *
//*                                                                   *
//* Compile -> assemble -> bind for a converted exit, optionally with *
//* an assembler stub from asm/stubs/.                                *
//*                                                                   *
//* NOT TESTED.  There is no z/OS system in this repository, so this  *
//* JCL has never been submitted.  Treat it as a starting point a     *
//* systems programmer adapts, not as a working procedure.            *
//*                                                                   *
//* SITE VALUES TO SUPPLY BEFORE FIRST USE:                           *
//*   ACCT         job accounting information                         *
//*   &HLQ         high level qualifier for your datasets             *
//*   &MEMBER      exit name, e.g. DFSWHU00                           *
//*   &STUB        stub name, or leave the ASMSTUB step commented out *
//*   &TLIB        target load library (must be APF-authorized for    *
//*                security exits; LPALIB for RACF exits)             *
//*   CBC.SCCNCMP  your XL C compiler library                         *
//*********************************************************************
//         SET HLQ=USERID.ASM2C
//         SET MEMBER=DFSWHU00
//         SET STUB=SAFAUTH
//         SET TLIB=USERID.LOADLIB
//*
//*------------------------------------------------------------------*
//* STEP 1: COMPILE THE METAL C SOURCE TO HLASM                       *
//*                                                                   *
//* -qmetal    no Language Environment                                *
//* -S         emit assembler source                                  *
//* -qlist     listing, for verifying the generated prolog/epilog     *
//* Add -q64 for an AMODE 64 exit (see docs/amode64-exits.md).        *
//*------------------------------------------------------------------*
//COMPILE  EXEC PGM=CCNDRVR,REGION=0M,
//         PARM='-qmetal -S -qlist -I//''&HLQ..INCLUDE'''
//STEPLIB  DD DISP=SHR,DSN=CBC.SCCNCMP
//SYSIN    DD DISP=SHR,DSN=&HLQ..CONVERTD(&MEMBER)
//SYSLIN   DD DISP=(NEW,PASS),DSN=&&ASMSRC,
//            SPACE=(TRK,(20,10)),UNIT=SYSDA,
//            DCB=(RECFM=FB,LRECL=80,BLKSIZE=8000)
//SYSPRINT DD SYSOUT=*
//SYSOUT   DD SYSOUT=*
//*
//*------------------------------------------------------------------*
//* STEP 2: ASSEMBLE THE GENERATED HLASM                              *
//*                                                                   *
//* SYS1.MACLIB is REQUIRED: the service wrappers in metalc_svc.h     *
//* expand WTO, GETMAIN/FREEMAIN and STORAGE macros here, not at      *
//* compile time.  Without it every wrapper fails to assemble.        *
//*------------------------------------------------------------------*
//ASM      EXEC PGM=ASMA90,COND=(0,LT,COMPILE),
//         PARM='OBJECT,NODECK,LIST,RENT'
//SYSLIB   DD DISP=SHR,DSN=SYS1.MACLIB
//         DD DISP=SHR,DSN=SYS1.MODGEN
//SYSUT1   DD UNIT=SYSDA,SPACE=(CYL,(5,5))
//SYSIN    DD DISP=(OLD,DELETE),DSN=&&ASMSRC
//SYSLIN   DD DISP=(NEW,PASS),DSN=&&OBJ,
//            SPACE=(TRK,(20,10)),UNIT=SYSDA,
//            DCB=(RECFM=FB,LRECL=80,BLKSIZE=8000)
//SYSPRINT DD SYSOUT=*
//*
//*------------------------------------------------------------------*
//* STEP 3: ASSEMBLE THE STUB (only for exits that call one)          *
//*                                                                   *
//* Delete this step for an exit with no asm/stubs/ dependency.  An   *
//* exit that calls saf_auth*() needs SAFAUTH or the bind fails with  *
//* an unresolved external -- which is the intended behaviour: a      *
//* missing service must not link clean.                              *
//*------------------------------------------------------------------*
//ASMSTUB  EXEC PGM=ASMA90,COND=(0,LT,ASM),
//         PARM='OBJECT,NODECK,LIST,RENT'
//SYSLIB   DD DISP=SHR,DSN=SYS1.MACLIB
//         DD DISP=SHR,DSN=SYS1.MODGEN
//SYSUT1   DD UNIT=SYSDA,SPACE=(CYL,(5,5))
//SYSIN    DD DISP=SHR,DSN=&HLQ..ASMSTUBS(&STUB)
//SYSLIN   DD DISP=(NEW,PASS),DSN=&&STUBOBJ,
//            SPACE=(TRK,(20,10)),UNIT=SYSDA,
//            DCB=(RECFM=FB,LRECL=80,BLKSIZE=8000)
//SYSPRINT DD SYSOUT=*
//*
//*------------------------------------------------------------------*
//* STEP 4: BIND                                                      *
//*                                                                   *
//* RENT,REUS,REFR: every exit in this framework must be reentrant.   *
//* AMODE 31 / RMODE ANY unless the module is AMODE 64.               *
//*------------------------------------------------------------------*
//BIND     EXEC PGM=IEWL,COND=(0,LT,ASMSTUB),
//         PARM='RENT,REUS,REFR,AMODE=31,RMODE=ANY,LIST,MAP,XREF'
//SYSLIN   DD DISP=(OLD,DELETE),DSN=&&OBJ
//         DD DISP=(OLD,DELETE),DSN=&&STUBOBJ
//         DD *
  NAME &MEMBER(R)
/*
//SYSLMOD  DD DISP=SHR,DSN=&TLIB
//SYSUT1   DD UNIT=SYSDA,SPACE=(CYL,(5,5))
//SYSPRINT DD SYSOUT=*
//*
//*********************************************************************
//* AFTER A SUCCESSFUL BIND, CHECK THE LISTINGS - THE BIND SUCCEEDING *
//* IS NOT THE SAME AS THE EXIT BEING CORRECT:                        *
//*                                                                   *
//*  1. COMPILE listing: the prolog/epilog the #pragma produced match  *
//*     the ASM source's linkage (SAVE/RETURN vs BAKR/PR).            *
//*  2. ASM listing: every service macro expanded. An unexpanded      *
//*     macro means SYS1.MACLIB was missing from SYSLIB.              *
//*  3. BIND map: no unresolved external references. An unresolved    *
//*     saf_auth_call means the stub was not assembled in.            *
//*  4. Module attributes are RENT and REUS.                          *
//*********************************************************************
