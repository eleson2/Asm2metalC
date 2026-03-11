 TITLE 'DSIEX01 - NetView Command Preprocessing Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  DSIEX01                                      *
*                                                                     *
* DESCRIPTIVE NAME    =  NetView Command Preprocessing Exit           *
*                                                                     *
* FUNCTION =                                                          *
*    Called by NetView before a command is executed. This exit:        *
*    1) Can allow the command (RC=0)                                   *
*    2) Can suppress the command (RC=4)                                *
*    3) Can modify the command (RC=8)                                  *
*    4) Can route to different operator (RC=12)                        *
*                                                                     *
*    This example:                                                     *
*    - Blocks CANCEL and FORCE MVS commands for non-auth operators    *
*    - Logs all VTAM commands for audit                                *
*    - Allows all other commands                                       *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R3  = Work register                                               *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    +0(4)  Work area pointer                                          *
*    +4(1)  Function code (1=pre, 2=post, 4=init, 5=term)            *
*    +5(1)  Command type (1=NV, 2=MVS, 4=VTAM, 8=JES)               *
*    +6(2)  Flags (x'8000'=authorized, x'4000'=console)              *
*    +8(8)  Operator ID                                                *
*    +16(8) Domain ID                                                  *
*    +24(4) Command text pointer                                       *
*    +28(4) Command text length                                        *
*    +48(4) Reason code (output)                                       *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
DSIEX01  CSECT ,
DSIEX01  AMODE 31
DSIEX01  RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING DSIEX01,R12     *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Check function code - only process pre-processing (func=1)           *
***********************************************************************
         CLI   4(R10),1            Is this pre-processing?
         BNE   ALLOW               No - allow
*
***********************************************************************
* Check command type - is it an MVS command?                            *
***********************************************************************
         CLI   5(R10),2            Is this an MVS command?
         BE    CHKMVS              Yes - check MVS command
         CLI   5(R10),4            Is this a VTAM command?
         BE    LOGVTAM             Yes - log it
         B     ALLOW               Other - allow
*
***********************************************************************
* MVS command - check for dangerous commands                           *
***********************************************************************
CHKMVS   DS    0H
* Check if operator is authorized                                     *
         TM    6(R10),X'80'        Is authorized flag set?
         BO    ALLOW               Yes - allow everything
*
* Non-authorized operator - check for CANCEL and FORCE                 *
         L     R3,24(,R10)         Load command text pointer
         L     R4,28(,R10)         Load command text length
         LTR   R4,R4               Is length valid?
         BNP   ALLOW               No - allow
*
         C     R4,=F'6'            At least 6 chars for CANCEL?
         BL    CHKFORCE            No - check FORCE
         CLC   0(6,R3),=C'CANCEL' Is it CANCEL?
         BE    REJECT              Yes - reject
*
CHKFORCE DS    0H
         C     R4,=F'5'            At least 5 chars for FORCE?
         BL    ALLOW               No - allow
         CLC   0(5,R3),=C'FORCE'  Is it FORCE?
         BE    REJECT              Yes - reject
         B     ALLOW               Allow other commands
*
***********************************************************************
* Log VTAM commands for audit                                          *
***********************************************************************
LOGVTAM  DS    0H
         MVC   WTOVTAM+4+19(8),8(R10)  Move operator ID
         WTO   MF=(E,WTOVTAM)     Issue audit WTO
         B     ALLOW               Allow the VTAM command
*
***********************************************************************
* Reject the command                                                   *
***********************************************************************
REJECT   DS    0H
         MVC   WTOREJ+4+25(8),8(R10)   Move operator ID
         WTO   MF=(E,WTOREJ)      Issue WTO
         MVC   48(4,R10),=F'100'   Set reason code
         LA    R15,4               RC=4 - suppress command
         B     RETURN              Return
*
***********************************************************************
* Allow the command                                                    *
***********************************************************************
ALLOW    DS    0H
         LA    R15,0               RC=0 - allow
*
***********************************************************************
* Return to NetView                                                    *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to NetView
*
***********************************************************************
* WTO Areas                                                            *
***********************************************************************
WTOVTAM  WTO   'DSIEX01 VTAM CMD OPER=XXXXXXXX',                     X
               ROUTCDE=(11),MF=L
WTOREJ   WTO   'DSIEX01 CMD BLOCKED OPER=XXXXXXXX NOT AUTHORIZED',   X
               ROUTCDE=(2,11),MF=L
*
***********************************************************************
* Register Equates                                                     *
***********************************************************************
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R10      EQU   10
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
         END   DSIEX01
