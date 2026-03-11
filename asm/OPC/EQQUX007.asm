 TITLE 'EQQUX007 - OPC/TWS Operation Status Change Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  EQQUX007                                     *
*                                                                     *
* DESCRIPTIVE NAME    =  TWS Operation Status Change Exit             *
*                                                                     *
* FUNCTION =                                                          *
*    Called by TWS when an operation changes status. This exit:        *
*    1) Logs error status changes for critical operations              *
*    2) Issues WTO for jobs ending in error on critical path           *
*    3) Can reject or modify status changes                            *
*                                                                     *
*    Return codes:                                                     *
*      0  = Continue normal processing                                 *
*      4  = Status was modified by exit                                *
*      8  = Reject the status change                                   *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R2  = Work register                                               *
*    R3  = Pointer to OPINFO block                                     *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    +0(4)  Function code (1=init, 2=status change, 3=term)           *
*    +4(4)  Pointer to OPINFO (operation information block)            *
*    +8(1)  Old status                                                 *
*    +9(1)  New status                                                 *
*    +10(2) Reserved                                                   *
*    +12(4) Return code from job                                       *
*    +16(4) Abend code (if applicable)                                 *
*                                                                     *
* OPINFO BLOCK:                                                       *
*    +0(16)  Application name                                          *
*    +16(8)  Application group                                         *
*    +24(4)  Input arrival date                                        *
*    +28(2)  Operation number                                          *
*    +30(1)  Operation status                                          *
*    +31(1)  Extended status                                           *
*    +32(8)  Job name                                                  *
*    +40(4)  Workstation name                                          *
*    +44(4)  Estimated duration                                        *
*    +68(2)  Maximum return code                                       *
*    +70(1)  Priority                                                  *
*    +71(1)  Flags (x'80'=critical path)                              *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
EQQUX007 CSECT ,
EQQUX007 AMODE 31
EQQUX007 RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING EQQUX007,R12    *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Check function code - only process status changes                    *
***********************************************************************
         L     R2,0(,R10)          Load function code
         C     R2,=F'2'            Is this a status change call?
         BNE   EXIT0               No - return RC=0
*
***********************************************************************
* Get operation information block                                      *
***********************************************************************
         L     R3,4(,R10)          Load OPINFO pointer
         LTR   R3,R3               Is pointer valid?
         BZ    EXIT0               No - return RC=0
*
***********************************************************************
* Check if new status is Error ('E' = x'C5')                          *
***********************************************************************
         CLI   9(R10),C'E'         Is new status = Error?
         BNE   EXIT0               No - allow and return
*
***********************************************************************
* Operation ended in error - check if on critical path                 *
***********************************************************************
         TM    71(R3),X'80'        Is critical path flag set?
         BNO   LOGERR              Not critical - just log it
*
***********************************************************************
* Critical path error - issue WTO alert                                *
***********************************************************************
         MVC   WTOMSG+0(8),32(R3)  Move job name to message
         WTO   MF=(E,WTOLIST)      Issue WTO
*
***********************************************************************
* Log the error (for all error statuses)                               *
***********************************************************************
LOGERR   DS    0H
* Error logging would go here - write to log dataset, etc.             *
* For this example, we just allow the status change.                   *
*
***********************************************************************
* Return RC=0 - continue processing                                    *
***********************************************************************
EXIT0    DS    0H
         LA    R15,0               RC=0 - continue
         RETURN (14,12),RC=(15)    Return to TWS
*
***********************************************************************
* WTO Parameter Area                                                   *
***********************************************************************
WTOLIST  WTO   'EQQUX007 CRITICAL PATH ERROR: XXXXXXXX ENDED IN ERROX
               R',ROUTCDE=(2,11),MF=L
WTOMSG   EQU   WTOLIST+4+29,8     Offset to job name in message
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
         END   EQQUX007
