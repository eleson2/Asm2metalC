 TITLE 'CSQXLIB - MQ Channel Security Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  CSQXLIB                                      *
*                                                                     *
* DESCRIPTIVE NAME    =  IBM MQ Channel Security Exit                 *
*                                                                     *
* FUNCTION =                                                          *
*    Called by MQ during channel startup for security validation.      *
*    This exit authenticates the partner queue manager and can         *
*    reject unauthorized channel connections.                           *
*                                                                     *
*    This example:                                                     *
*    1) Validates partner name against an allow list                   *
*    2) Logs all channel connection attempts                           *
*    3) Rejects connections from unknown partners                      *
*                                                                     *
*    Return codes (in MQCXP exitResponse field):                       *
*      0 (MQXCC_OK)             = Continue processing                  *
*      4 (MQXCC_CLOSE_CHANNEL)  = Close the channel                    *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R2  = Pointer to MQCXP                                            *
*    R3  = Pointer to MQCD                                             *
*    R12 = Base register                                               *
*                                                                     *
* ENTRY PARAMETERS (standard MQ channel exit):                        *
*    R1 -> Parameter list:                                             *
*          +0(4) Pointer to MQCXP (Channel Exit Parameter)            *
*          +4(4) Pointer to MQCD (Channel Definition)                 *
*          +8(4) Pointer to data buffer                               *
*          +12(4) Data buffer length                                   *
*                                                                     *
* MQCXP key fields:                                                   *
*    +8(4)  Exit ID                                                    *
*    +12(4) Exit reason (1=init, 2=term, 5=sec_msg, 6=init_sec)      *
*    +16(4) Exit response (output)                                     *
*    +76(48) Partner name                                              *
*                                                                     *
* MQCD key fields:                                                     *
*    +0(20) Channel name                                               *
*    +24(4) Channel type                                               *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
CSQXLIB  CSECT ,
CSQXLIB  AMODE 31
CSQXLIB  RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING CSQXLIB,R12     *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Load MQCXP and MQCD pointers                                        *
***********************************************************************
         L     R2,0(,R10)          Load MQCXP pointer
         L     R3,4(,R10)          Load MQCD pointer
*
***********************************************************************
* Check exit reason - handle INIT_SEC (reason=6)                       *
***********************************************************************
         CLC   12(4,R2),=F'6'     Is this INIT_SEC?
         BE    CHKPART             Yes - check partner
*
* For other reasons (INIT, TERM, etc.) just continue                   *
         MVC   16(4,R2),=F'0'     Set exitResponse = MQXCC_OK
         B     EXIT0               Return OK
*
***********************************************************************
* Check partner name against allow list                                *
***********************************************************************
CHKPART  DS    0H
* Log the connection attempt                                           *
         MVC   WTOCHN+4+17(20),0(R3)   Move channel name
         MVC   WTOCHN+4+46(20),76(R2)  Move partner name
         WTO   MF=(E,WTOCHN)      Issue WTO
*
* Check against allowed partners                                       *
         LA    R5,PARTLIST         Point to partner list
         LA    R6,PARTNOE          Number of entries
CHKLOOP  DS    0H
         CLC   76(20,R2),0(R5)    Does partner match?
         BE    ACCEPT              Yes - accept
         LA    R5,20(,R5)          Next entry
         BCT   R6,CHKLOOP         Continue checking
*
***********************************************************************
* Partner not in allow list - reject                                   *
***********************************************************************
         MVC   WTOREJ+4+20(20),76(R2)  Move partner name
         WTO   MF=(E,WTOREJ)      Issue WTO
         MVC   16(4,R2),=F'4'     exitResponse = MQXCC_CLOSE_CHANNEL
         LA    R15,0               Return code
         B     RETURN              Return
*
***********************************************************************
* Partner accepted                                                     *
***********************************************************************
ACCEPT   DS    0H
         MVC   16(4,R2),=F'0'     exitResponse = MQXCC_OK
*
***********************************************************************
* Return                                                               *
***********************************************************************
EXIT0    DS    0H
         LA    R15,0               RC=0
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to MQ
*
***********************************************************************
* Allowed partner list (20-byte entries, blank padded)                 *
***********************************************************************
PARTLIST DS    0D
         DC    CL20'PROD.QMGR1          '
         DC    CL20'PROD.QMGR2          '
         DC    CL20'DR.QMGR1            '
         DC    CL20'TEST.QMGR1          '
PARTNOE  EQU   (*-PARTLIST)/20    Number of entries
*
***********************************************************************
* WTO Areas                                                            *
***********************************************************************
WTOCHN   WTO   'CSQXLIB MQ CHAN=XXXXXXXXXXXXXXXXXXXX PARTNER=XXXXXXXXXXXX
               XXXXXXXX',ROUTCDE=(11),MF=L
WTOREJ   WTO   'CSQXLIB MQ REJECTED=XXXXXXXXXXXXXXXXXXXX',            X
               ROUTCDE=(2,11),MF=L
*
***********************************************************************
* Register Equates                                                     *
***********************************************************************
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R5       EQU   5
R6       EQU   6
R10      EQU   10
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
         END   CSQXLIB
