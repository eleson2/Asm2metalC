***********************************************************************
* MODULE NAME: SAFAUTH                                                *
* FUNCTION:    RACROUTE REQUEST=AUTH stub callable from Metal C       *
*                                                                     *
* Metal C cannot expand the RACROUTE macro, so this stub owns the     *
* macro expansion and presents a plain OS-linkage entry point.        *
* Declared to C in includes/metalc_saf.h as saf_auth_call().          *
*                                                                     *
* This replaces the inline "XR 15,15" RACROUTE placeholder that       *
* previously sat in converted/IMS/DFSWHU00.c.  See                    *
* docs/racroute-metalc-patterns.md section 3 (Strategy B).            *
*                                                                     *
* LINKAGE:  Standard OS.  R1 -> A(SAFPARM)                            *
*           R13 -> caller savearea, R14 return, R15 entry             *
*                                                                     *
* SAFPARM (mapped by struct saf_auth_parm in metalc_saf.h):           *
*   +0   A(entity)   ptr to entity name, blank padded to the class    *
*                    maximum length (8 for APPL, 44 for DATASET)      *
*   +4   A(class)    ptr to 8-byte class name, blank padded           *
*   +8   A(userid)   ptr to 8-byte userid, or 0 for the task ACEE     *
*   +12  A(acee)     ptr to ACEE, or 0 for the task ACEE              *
*   +16  F  attr     0=READ 4=UPDATE 8=CONTROL 16=ALTER               *
*   +20  F  saf_rc   OUT - R15 from RACROUTE (SAF return code)        *
*   +24  F  racf_rc  OUT - R0  from RACROUTE (RACF return code)       *
*   +28  F  racf_rsn OUT - R1  from RACROUTE (RACF reason code)       *
*                                                                     *
* RETURN:   R15 = SAF return code (0 granted, 4 no decision, 8 denied)*
*           A code the stub cannot obtain is reported as 8 so the    *
*           caller's default-deny path is taken.                      *
*                                                                     *
* REENTRANT: yes.  The MF=L parameter list is a read-only model that  *
*           is copied into GETMAINed storage before MF=(E,...) fills  *
*           in the runtime values.  Nothing is written into the       *
*           module's own storage.                                     *
*                                                                     *
* ATTRIBUTES: REENTRANT, AMODE 31, RMODE ANY, key 8 problem state     *
*                                                                     *
* -------------------------------------------------------------------*
* REVIEW REQUIRED BEFORE PRODUCTION ASSEMBLY                          *
* This stub has not been assembled -- there is no z/OS system in this *
* repository.  A systems programmer must confirm against the RACF     *
* Macro Reference for the installed RACF level:                       *
*   1. the RELEASE= level coded below is valid for that RACF release; *
*   2. WORKLEN is at least the WORKA size that RELEASE= requires;     *
*   3. the register forms ENTITY=((R2)), CLASS=(R3), USERID=(R5)      *
*      match the operand descriptions for that release, in particular *
*      whether CLASS= expects a length-prefixed field;                *
* Until that review is signed off, DFSWHU00 must stay on the          *
* Production Deployment Blocks list.                                  *
***********************************************************************
         YREGS
*
SAFAUTH  CSECT
SAFAUTH  AMODE 31
SAFAUTH  RMODE ANY
         SAVE  (14,12)
         LR    R12,R15
         USING SAFAUTH,R12
         LR    R11,R1              R11 -> A(SAFPARM)
         L     R11,0(,R11)         R11 -> SAFPARM
         USING SAFPARM,R11
*
*---------------------------------------------------------------------
* Obtain dynamic storage: savearea + RACROUTE work area + parm list.  *
* COND=YES so a storage shortage returns rather than abends; the      *
* caller then sees SAF RC=8 (denied) and rejects.                     *
*---------------------------------------------------------------------
         STORAGE OBTAIN,LENGTH=WORKLEN,ADDR=(R10),SP=0,LOC=ANY,       X
               COND=YES
         LTR   R15,R15
         BZ    SAFGOTST
         LA    R15,8               No storage - report denied
         ST    R15,SAFSAFRC
         B     SAFEXIT
*
SAFGOTST DS    0H
         USING WORKD,R10
         ST    R13,WSAVE+4         Chain savearea forward
         ST    R10,8(,R13)         Chain savearea backward
         LA    R13,WSAVE
*
*---------------------------------------------------------------------
* Copy the read-only MF=L model into the dynamic parameter list.      *
*---------------------------------------------------------------------
         MVC   WLIST(LIST_LEN),SAFMODL
*
*---------------------------------------------------------------------
* Load the caller's values into registers for the MF=(E,...) forms.   *
*---------------------------------------------------------------------
         L     R2,SAFENT           R2 -> entity name
         L     R3,SAFCLS           R3 -> class name
         L     R5,SAFUSER          R5 -> userid (may be 0)
         L     R6,SAFACEE          R6 -> ACEE   (may be 0)
         L     R4,SAFATTR          R4 =  access intent
*
*---------------------------------------------------------------------
* ATTR= takes a keyword, not a register, so branch to the matching    *
* expansion.  Anything unrecognised is treated as ALTER, the most     *
* restrictive intent -- never as READ.                                *
*---------------------------------------------------------------------
         C     R4,=F'0'
         BE    SAFREAD
         C     R4,=F'4'
         BE    SAFUPD
         C     R4,=F'8'
         BE    SAFCTL
         B     SAFALT
*
SAFREAD  DS    0H
         RACROUTE REQUEST=AUTH,MF=(E,WLIST),WORKA=WWORK,              X
               CLASS=(R3),ENTITY=((R2)),USERID=(R5),ACEE=(R6),        X
               ATTR=READ,RELEASE=2.6
         B     SAFSAVE
*
SAFUPD   DS    0H
         RACROUTE REQUEST=AUTH,MF=(E,WLIST),WORKA=WWORK,              X
               CLASS=(R3),ENTITY=((R2)),USERID=(R5),ACEE=(R6),        X
               ATTR=UPDATE,RELEASE=2.6
         B     SAFSAVE
*
SAFCTL   DS    0H
         RACROUTE REQUEST=AUTH,MF=(E,WLIST),WORKA=WWORK,              X
               CLASS=(R3),ENTITY=((R2)),USERID=(R5),ACEE=(R6),        X
               ATTR=CONTROL,RELEASE=2.6
         B     SAFSAVE
*
SAFALT   DS    0H
         RACROUTE REQUEST=AUTH,MF=(E,WLIST),WORKA=WWORK,              X
               CLASS=(R3),ENTITY=((R2)),USERID=(R5),ACEE=(R6),        X
               ATTR=ALTER,RELEASE=2.6
*
*---------------------------------------------------------------------
* Save all three return codes for the caller.  R15 is the decision;   *
* R0 and R1 are diagnostic only.                                      *
*---------------------------------------------------------------------
SAFSAVE  DS    0H
         ST    R15,SAFSAFRC        SAF return code  (the decision)
         ST    R0,SAFRACRC         RACF return code (diagnostic)
         ST    R1,SAFRACRS         RACF reason code (diagnostic)
*
*---------------------------------------------------------------------
* Release dynamic storage and return.  R7 carries the RC across the   *
* STORAGE RELEASE, which clobbers R15.                                *
*---------------------------------------------------------------------
         LR    R7,R15
         L     R13,WSAVE+4
         STORAGE RELEASE,LENGTH=WORKLEN,ADDR=(R10),SP=0
         LR    R15,R7
*
SAFEXIT  DS    0H
         RETURN (14,12),RC=(15)    R15 already holds the SAF RC
*
***********************************************************************
* CONSTANTS - read-only, safe in a reentrant module                   *
***********************************************************************
         DS    0F
SAFMODL  RACROUTE REQUEST=AUTH,MF=L
LIST_LEN EQU   *-SAFMODL
*
         LTORG
*
***********************************************************************
* DYNAMIC WORK AREA                                                   *
***********************************************************************
WORKD    DSECT
WSAVE    DS    18F                 Standard savearea
         DS    0D
WWORK    DS    CL512               RACROUTE WORKA
         DS    0F
WLIST    DS    CL(LIST_LEN)        Copy of the MF=L parameter list
WORKLEN  EQU   *-WORKD
*
***********************************************************************
* CALLER PARAMETER BLOCK - struct saf_auth_parm                       *
***********************************************************************
SAFPARM  DSECT
SAFENT   DS    A                   +0   ptr to entity name
SAFCLS   DS    A                   +4   ptr to class name
SAFUSER  DS    A                   +8   ptr to userid, or 0
SAFACEE  DS    A                   +12  ptr to ACEE, or 0
SAFATTR  DS    F                   +16  access intent
SAFSAFRC DS    F                   +20  OUT SAF return code
SAFRACRC DS    F                   +24  OUT RACF return code
SAFRACRS DS    F                   +28  OUT RACF reason code
*
         END   SAFAUTH
