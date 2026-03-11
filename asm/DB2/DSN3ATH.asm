 TITLE 'DSN3ATH - DB2 Authorization Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  DSN3ATH                                      *
*                                                                     *
* DESCRIPTIVE NAME    =  DB2 Authorization Exit Routine               *
*                                                                     *
* SOURCE              =  Based on IBM DB2 sample                      *
*                                                                     *
* FUNCTION =                                                          *
*    Called by DB2 to perform external authorization checking.         *
*    This exit supplements (or replaces) DB2's internal auth           *
*    and RACF checking for object access.                              *
*                                                                     *
*    This example:                                                     *
*    1) Logs all access attempts to tables with HLQ 'PAYROLL'         *
*    2) Denies access to PAYROLL tables from batch connections         *
*       unless the auth ID has SYSADM privilege                        *
*    3) Defers all other authorization to normal DB2 processing        *
*                                                                     *
*    Return codes:                                                     *
*      0  = Allow access                                               *
*      4  = Deny access                                                *
*      8  = Continue with normal DB2 authorization                     *
*      12 = Error occurred                                             *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list                                   *
*    R2  = Pointer to work area                                        *
*    R3  = Work register                                               *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    +0(4)  Work area pointer                                          *
*    +4(1)  Function code (1=object, 2=system, 3=table, 4=plan)       *
*    +5(1)  Flags                                                      *
*    +6(2)  Privilege requested                                        *
*    +8(8)  Authorization ID                                           *
*    +16(18) Object name                                               *
*    +36(8) Object owner                                               *
*    +44(8) Object type                                                *
*    +52(8) Schema name                                                *
*    +60(4) Reason code (output)                                       *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
DSN3ATH  CSECT ,
DSN3ATH  AMODE 31
DSN3ATH  RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING DSN3ATH,R12     *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Check function code - only handle table access (func=3)              *
***********************************************************************
         CLI   4(R10),3            Is this a table access check?
         BNE   DEFER               No - defer to DB2
*
***********************************************************************
* Check if object name starts with 'PAYROLL'                           *
***********************************************************************
         CLC   16(7,R10),=C'PAYROLL'  Is it a PAYROLL table?
         BNE   DEFER               No - defer to DB2
*
***********************************************************************
* PAYROLL table access - log the attempt                               *
***********************************************************************
         MVC   WTOAUTH+4+21(8),8(R10)  Move auth ID to WTO
         MVC   WTOAUTH+4+34(18),16(R10) Move object name
         WTO   MF=(E,WTOAUTH)     Issue audit WTO
*
***********************************************************************
* Check for SYSADM - SYSADM can always access                         *
* (In real exit, would check ACEE or privilege table)                  *
***********************************************************************
         CLC   8(8,R10),=CL8'SYSADM' Is auth ID SYSADM?
         BE    ALLOW               Yes - allow access
*
***********************************************************************
* Non-SYSADM accessing PAYROLL - deny with reason code                 *
***********************************************************************
DENY     DS    0H
         MVC   60(4,R10),=F'200'   Set reason code
         LA    R15,4               RC=4 - deny access
         B     RETURN              Return
*
***********************************************************************
* Allow access                                                         *
***********************************************************************
ALLOW    DS    0H
         LA    R15,0               RC=0 - allow access
         B     RETURN              Return
*
***********************************************************************
* Defer to normal DB2 authorization                                    *
***********************************************************************
DEFER    DS    0H
         LA    R15,8               RC=8 - continue DB2 auth
*
***********************************************************************
* Return to DB2                                                        *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to DB2
*
***********************************************************************
* WTO Parameter Area                                                   *
***********************************************************************
WTOAUTH  WTO   'DSN3ATH PAYROLL AUTH=XXXXXXXX OBJ=XXXXXXXXXXXXXXXXXX',X
               ROUTCDE=(9,11),MF=L
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
         END   DSN3ATH
