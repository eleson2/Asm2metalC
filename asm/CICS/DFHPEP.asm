 TITLE 'DFHPEP - CICS Program Error Program'
***********************************************************************
*                                                                     *
* MODULE NAME         =  DFHPEP                                       *
*                                                                     *
* DESCRIPTIVE NAME    =  CICS Program Error Program Exit              *
*                                                                     *
* FUNCTION =                                                          *
*    Called by CICS when a program check (ASRA), operating system      *
*    abend (ASRB), or runaway task (AICA) occurs. This exit:          *
*    1) Logs the abend information (transaction, program, abend code) *
*    2) Issues a WTO for ASRA abends in production transactions       *
*    3) Returns to let CICS perform normal abend processing            *
*                                                                     *
*    Return codes:                                                     *
*      0  = Continue with normal abend processing                      *
*      4  = Retry the operation                                        *
*      8  = Suppress the abend                                         *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R2  = Pointer to EIB                                              *
*    R3  = Work register                                               *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    +0(4)  EIB pointer                                                *
*    +4(4)  COMMAREA pointer                                           *
*    +8(4)  Transaction ID                                             *
*    +12(8) Program name that abended                                  *
*    +20(4) Abend code (ASRA/ASRB/AICA etc)                          *
*    +24(4) PSW at time of abend                                       *
*    +28(4) PSW address                                                *
*    +32(4) Register save area at abend                                *
*    +36(1) Error type (1=ASRA, 2=ASRB, 3=AICA, 4=user)             *
*    +37(1) Flags                                                      *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
DFHPEP   CSECT ,
DFHPEP   AMODE 31
DFHPEP   RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING DFHPEP,R12      *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Get abend code and transaction ID                                    *
***********************************************************************
         MVC   WABCODE(4),20(R10)  Get abend code
         MVC   WTRANID(4),8(R10)   Get transaction ID
         MVC   WPGMNAM(8),12(R10)  Get program name
*
***********************************************************************
* Check error type                                                     *
***********************************************************************
         CLI   36(R10),1           Is this ASRA (program check)?
         BE    LOGASRA             Yes - log and alert
         CLI   36(R10),3           Is this AICA (runaway)?
         BE    LOGAICA             Yes - log runaway
         B     LOGOTHER            Other - just log
*
***********************************************************************
* ASRA - Program check abend                                           *
* Issue WTO for production transactions (not CECI/CECS/CEBR)          *
***********************************************************************
LOGASRA  DS    0H
         CLC   WTRANID(4),=C'CECI' Is it CECI?
         BE    EXIT0                Yes - skip alert
         CLC   WTRANID(4),=C'CECS' Is it CECS?
         BE    EXIT0                Yes - skip alert
         CLC   WTRANID(4),=C'CEBR' Is it CEBR?
         BE    EXIT0                Yes - skip alert
*
* Issue WTO for production ASRA                                        *
         MVC   WTOASRA+4+19(4),WTRANID  Move tran ID
         MVC   WTOASRA+4+28(8),WPGMNAM  Move program name
         WTO   MF=(E,WTOASRA)     Issue WTO
         B     EXIT0               Continue with abend
*
***********************************************************************
* AICA - Runaway task                                                  *
***********************************************************************
LOGAICA  DS    0H
         MVC   WTOAICA+4+20(4),WTRANID  Move tran ID
         MVC   WTOAICA+4+29(8),WPGMNAM  Move program name
         WTO   MF=(E,WTOAICA)     Issue WTO
         B     EXIT0               Continue with abend
*
***********************************************************************
* Other abend - just continue                                          *
***********************************************************************
LOGOTHER DS    0H
*
***********************************************************************
* Return RC=0 - let CICS continue with normal abend processing        *
***********************************************************************
EXIT0    DS    0H
         LA    R15,0               RC=0 - continue normal processing
         RETURN (14,12),RC=(15)    Return to CICS
*
***********************************************************************
* Work fields                                                          *
***********************************************************************
WABCODE  DS    CL4                 Abend code
WTRANID  DS    CL4                 Transaction ID
WPGMNAM  DS    CL8                 Program name
*
WTOASRA  WTO   'DFHPEP ASRA TRAN=XXXX PGM=XXXXXXXX PROGRAM CHECK',   X
               ROUTCDE=(2,11),MF=L
WTOAICA  WTO   'DFHPEP AICA TRAN=XXXX PGM=XXXXXXXX RUNAWAY TASK',    X
               ROUTCDE=(2,11),MF=L
*
***********************************************************************
* Register Equates                                                     *
***********************************************************************
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R10      EQU   10
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
         END   DFHPEP
