 TITLE 'AOFEXC02 - System Automation Resource State Change Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  AOFEXC02                                     *
*                                                                     *
* DESCRIPTIVE NAME    =  SA Resource State Change Exit                *
*                                                                     *
* FUNCTION =                                                          *
*    Called by System Automation when a monitored resource changes     *
*    state. This exit can:                                              *
*    1) Continue with normal processing (RC=0)                         *
*    2) Suppress the automation action (RC=4)                          *
*    3) Override automation with custom action (RC=8)                  *
*    4) Defer the action (RC=12)                                       *
*                                                                     *
*    This example:                                                     *
*    - Issues WTO for critical resource state changes                  *
*    - Suppresses auto-recovery during maintenance windows             *
*    - Logs all state transitions to HARDDOWN                          *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R3  = Pointer to resource block                                   *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    +0(4)  Work area pointer                                          *
*    +4(1)  Function code (1=init, 2=monitor, 3=state change)         *
*    +5(1)  Event type                                                 *
*    +6(2)  Flags (x'8000'=manual, x'4000'=recovery)                 *
*    +8(4)  Pointer to resource block                                  *
*    +12(1) Old observed state                                         *
*    +13(1) New observed state                                         *
*    +14(1) Old desired state                                          *
*    +15(1) New desired state                                          *
*                                                                     *
* RESOURCE BLOCK:                                                     *
*    +0(32)  Resource name                                             *
*    +32(1)  Resource type                                             *
*    +33(1)  Observed state                                            *
*    +34(1)  Desired state                                             *
*    +35(1)  Automation flag (1=yes, 2=no, 3=assist)                  *
*    +84(8)  Job name                                                  *
*    +152(1) Health indicator                                          *
*    +153(1) Priority                                                  *
*    +155(1) Flags (x'80'=critical)                                   *
*                                                                     *
* OBSERVED STATES:                                                     *
*    x'01' = HARDDOWN                                                  *
*    x'02' = SOFTDOWN                                                  *
*    x'03' = STARTING                                                  *
*    x'04' = AVAILABLE                                                 *
*    x'05' = DEGRADED                                                  *
*    x'06' = STOPPING                                                  *
*    x'07' = PROBLEM                                                   *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
AOFEXC02 CSECT ,
AOFEXC02 AMODE 31
AOFEXC02 RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING AOFEXC02,R12    *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Check function code - only process state changes                     *
***********************************************************************
         CLI   4(R10),3            Is this a state change?
         BNE   CONTINUE            No - continue
*
***********************************************************************
* Get resource block pointer                                           *
***********************************************************************
         L     R3,8(,R10)          Load resource block pointer
         LTR   R3,R3               Is pointer valid?
         BZ    CONTINUE            No - continue
*
***********************************************************************
* Check if transitioning to HARDDOWN (x'01')                           *
***********************************************************************
         CLI   13(R10),X'01'       New state = HARDDOWN?
         BNE   CHKCRIT             No - check critical
*
* Resource going HARDDOWN - log it                                     *
         MVC   WTODOWN+4+20(32),0(R3)   Move resource name
         MVC   WTODOWN+4+57(8),84(R3)   Move job name
         WTO   MF=(E,WTODOWN)     Issue WTO
*
***********************************************************************
* Check if this is a critical resource                                 *
***********************************************************************
CHKCRIT  DS    0H
         TM    155(R3),X'80'       Is critical flag set?
         BNO   CHKMAINT            Not critical - check maintenance
*
* Critical resource state change                                       *
         CLI   13(R10),X'07'       New state = PROBLEM?
         BE    ALERTCRIT           Yes - alert
         CLI   13(R10),X'01'       New state = HARDDOWN?
         BE    ALERTCRIT           Yes - alert
         CLI   13(R10),X'05'       New state = DEGRADED?
         BE    ALERTCRIT           Yes - alert
         B     CHKMAINT            Other state - check maintenance
*
***********************************************************************
* Alert for critical resource failure                                  *
***********************************************************************
ALERTCRIT DS   0H
         MVC   WTOCRIT+4+24(32),0(R3)  Move resource name
         WTO   MF=(E,WTOCRIT)     Issue critical WTO
         B     CONTINUE            Continue with automation
*
***********************************************************************
* Check maintenance window - suppress auto-recovery if active          *
* (In production, would check a time-based flag or ECF token)          *
***********************************************************************
CHKMAINT DS    0H
         TM    6(R10),X'40'        Is recovery in progress?
         BNO   CONTINUE            No - continue normally
*
* Recovery in progress - check automation flag                         *
         CLI   35(R3),3            Automation = ASSIST?
         BNE   CONTINUE            No - continue normally
*
* In assist mode during recovery - suppress automatic action            *
         LA    R15,4               RC=4 - suppress automation
         B     RETURN              Return
*
***********************************************************************
* Continue with normal SA processing                                   *
***********************************************************************
CONTINUE DS    0H
         LA    R15,0               RC=0 - continue
*
***********************************************************************
* Return to System Automation                                          *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to SA
*
***********************************************************************
* WTO Areas                                                            *
***********************************************************************
WTODOWN  WTO   'AOFEXC02 HARDDOWN RES=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX JOX
               B=XXXXXXXX',ROUTCDE=(2,11),MF=L
WTOCRIT  WTO   'AOFEXC02 CRITICAL ALERT RES=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
               XX',ROUTCDE=(1,2,11),MF=L
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
         END   AOFEXC02
