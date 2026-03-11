 TITLE 'ISTEXCLY - VTAM Logon Verify Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  ISTEXCLY                                     *
*                                                                     *
* DESCRIPTIVE NAME    =  VTAM Logon Verification Exit                 *
*                                                                     *
* FUNCTION =                                                          *
*    Called by VTAM when a logon request is received for an            *
*    application. This exit can:                                       *
*    1) Accept the logon (RC=0)                                        *
*    2) Reject the logon (RC=4)                                        *
*    3) Defer to RACF for authentication (RC=8)                        *
*    4) Redirect to another application (RC=12)                        *
*                                                                     *
*    This example:                                                     *
*    - Logs all logon attempts via WTO                                 *
*    - Rejects logons to ADMIN application from non-admin LUs          *
*    - Defers all other logons to RACF                                 *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R3  = Work register                                               *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    +0(4)  Work area pointer                                          *
*    +4(1)  Function code (1=logon, 2=logoff, 3=verify, 4=init)       *
*    +5(1)  Flags (x'80'=RACF avail, x'40'=encrypted)                *
*    +8(8)  LU name                                                    *
*    +16(8) Application name                                           *
*    +24(8) Mode name                                                  *
*    +32(4) Logon data pointer                                         *
*    +36(4) Logon data length                                          *
*    +40(8) User ID                                                    *
*    +48(8) Password (masked)                                          *
*    +56(8) New application (for redirect, output)                     *
*    +64(4) Reason code (output)                                       *
*    +68(4) Sense code (output)                                        *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
ISTEXCLY CSECT ,
ISTEXCLY AMODE 31
ISTEXCLY RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING ISTEXCLY,R12    *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Check function code - only process logon requests                    *
***********************************************************************
         CLI   4(R10),1            Is this a logon request?
         BNE   DEFER               No - defer to RACF
*
***********************************************************************
* Log the logon attempt                                                *
***********************************************************************
         MVC   WTOLOGN+4+17(8),8(R10)   Move LU name
         MVC   WTOLOGN+4+31(8),16(R10)  Move application name
         MVC   WTOLOGN+4+44(8),40(R10)  Move user ID
         WTO   MF=(E,WTOLOGN)     Issue WTO
*
***********************************************************************
* Check if target application is ADMIN                                 *
***********************************************************************
         CLC   16(5,R10),=C'ADMIN'  Is target application ADMIN?
         BNE   DEFER               No - defer to RACF
*
***********************************************************************
* ADMIN application - check LU name prefix                             *
* Only allow LUs starting with 'ADM' to access ADMIN app              *
***********************************************************************
         CLC   8(3,R10),=C'ADM'   LU name starts with ADM?
         BE    DEFER               Yes - defer to RACF for auth
*
***********************************************************************
* Reject - non-admin LU trying to access ADMIN application             *
***********************************************************************
         MVC   68(4,R10),SENSESEC  Set security sense code
         MVC   64(4,R10),=F'100'   Set reason code
         LA    R15,4               RC=4 - reject logon
         B     RETURN              Return
*
***********************************************************************
* Defer to RACF for standard authentication                            *
***********************************************************************
DEFER    DS    0H
         LA    R15,8               RC=8 - defer to RACF
*
***********************************************************************
* Return to VTAM                                                       *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to VTAM
*
***********************************************************************
* Constants                                                            *
***********************************************************************
SENSESEC DC    X'080F0000'         Security violation sense code
*
WTOLOGN  WTO   'ISTEXCLY LOGON LU=XXXXXXXX APPL=XXXXXXXX USER=XXXXXXXX
               XX',ROUTCDE=(9,11),MF=L
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
         END   ISTEXCLY
