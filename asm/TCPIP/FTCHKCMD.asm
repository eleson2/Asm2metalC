 TITLE 'FTCHKCMD - FTP Command Validation Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  FTCHKCMD                                     *
*                                                                     *
* DESCRIPTIVE NAME    =  FTP Server Command Validation Exit           *
*                                                                     *
* FUNCTION =                                                          *
*    Called by z/OS FTP server before executing a client command.      *
*    This exit can:                                                    *
*    1) Allow the command (RC=0)                                       *
*    2) Reject the command (RC=4)                                      *
*    3) Modify command parameters (RC=8)                               *
*    4) Terminate the session (RC=12)                                  *
*                                                                     *
*    This example:                                                     *
*    - Blocks DELETE (DELE) commands for SYS1.* datasets               *
*    - Blocks SITE commands from non-internal IP addresses             *
*    - Logs all STOR (upload) commands                                  *
*    - Allows all other commands                                       *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R3  = Work register                                               *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    +0(4)  Work area pointer                                          *
*    +4(1)  Function code (1=init, 2=command, 3=term)                 *
*    +5(1)  Flags (x'80'=SSL, x'40'=anonymous, x'20'=MVS)            *
*    +6(2)  Command code (14=RETR, 15=STOR, 23=DELE, 29=SITE)        *
*    +8(8)  User ID                                                    *
*    +16(4) Client IP address (4 bytes, network order)                 *
*    +20(2) Client port                                                *
*    +24(4) Command text pointer                                       *
*    +28(4) Command text length                                        *
*    +32(4) Argument pointer                                           *
*    +36(4) Argument length                                            *
*    +40(44) Dataset name                                              *
*    +340(4) Reason code (output)                                      *
*    +344(2) FTP reply code (output)                                   *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
FTCHKCMD CSECT ,
FTCHKCMD AMODE 31
FTCHKCMD RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING FTCHKCMD,R12    *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Check function code - only process command calls                     *
***********************************************************************
         CLI   4(R10),2            Is this a command call?
         BNE   ALLOW               No - allow (init/term)
*
***********************************************************************
* Check for DELETE (DELE) command - code 23                            *
***********************************************************************
         CLC   6(2,R10),=H'23'    Is this DELE command?
         BNE   CHKSTOR             No - check next
*
* DELETE command - check dataset name for SYS1.                        *
         CLC   40(5,R10),=C'SYS1.' Dataset starts with SYS1.?
         BNE   ALLOW               No - allow delete
*
* Reject deletion of SYS1.* datasets                                   *
         MVC   344(2,R10),=H'550'  FTP reply 550 (not accessible)
         MVC   340(4,R10),=F'100'  Reason code
         LA    R15,4               RC=4 - reject command
         B     RETURN              Return
*
***********************************************************************
* Check for STOR (upload) command - code 15                            *
***********************************************************************
CHKSTOR  DS    0H
         CLC   6(2,R10),=H'15'    Is this STOR command?
         BNE   CHKSITE             No - check next
*
* Log the upload                                                       *
         MVC   WTOSTOR+4+19(8),8(R10)  Move user ID
         WTO   MF=(E,WTOSTOR)     Issue audit WTO
         B     ALLOW               Allow the upload
*
***********************************************************************
* Check for SITE command - code 29                                     *
***********************************************************************
CHKSITE  DS    0H
         CLC   6(2,R10),=H'29'    Is this SITE command?
         BNE   ALLOW               No - allow
*
* SITE command - check if from internal network (10.x.x.x)            *
         CLI   16(R10),10          First octet = 10?
         BE    ALLOW               Yes (10.0.0.0/8) - allow
*
* External IP trying SITE command - reject                             *
         MVC   344(2,R10),=H'500'  FTP reply 500 (syntax error)
         MVC   340(4,R10),=F'200'  Reason code
         LA    R15,4               RC=4 - reject
         B     RETURN              Return
*
***********************************************************************
* Allow the command                                                    *
***********************************************************************
ALLOW    DS    0H
         LA    R15,0               RC=0 - allow
*
***********************************************************************
* Return to FTP server                                                 *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to FTP
*
***********************************************************************
* WTO Areas                                                            *
***********************************************************************
WTOSTOR  WTO   'FTCHKCMD FTP STOR USER=XXXXXXXX',                     X
               ROUTCDE=(11),MF=L
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
         END   FTCHKCMD
