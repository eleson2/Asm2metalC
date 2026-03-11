 TITLE 'ACF2PWX - ACF2 Password Validation Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  ACF2PWX                                      *
*                                                                     *
* DESCRIPTIVE NAME    =  ACF2 Password Quality Exit (LGNPW)          *
*                                                                     *
* FUNCTION =                                                          *
*    Called by CA ACF2 during password change requests. This exit      *
*    enforces additional password quality rules beyond the standard    *
*    ACF2 password policy.                                              *
*                                                                     *
*    Password rules enforced:                                          *
*    1) Password must not equal the logonid                            *
*    2) Password must contain at least one numeric character           *
*    3) Password must not contain 3 or more repeated characters        *
*    4) Password must not be a common word from table                  *
*                                                                     *
*    Return codes:                                                     *
*      0  = Accept password                                            *
*      4  = Reject password                                            *
*      8  = Revoke logonid (severe violation)                          *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Address of parameter list (array of pointers)              *
*    R2  = Pointer to ACVALD block                                     *
*    R3  = Work register                                               *
*    R4  = Work register                                               *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST:                                                     *
*    R1 -> array of fullword pointers:                                 *
*      +0(4)  Pointer to ACVALD (validation parameter block)          *
*                                                                     *
* ACVALD BLOCK:                                                       *
*    +0(1)  Flags (x'80'=new pwd, x'40'=verify, x'20'=change)       *
*    +1(1)  Return code (output)                                       *
*    +2(2)  Reason code (output)                                       *
*    +4(8)  Logonid                                                    *
*    +12(20) User name                                                 *
*    +34(8) Password                                                   *
*    +42(8) New password                                               *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
ACF2PWX  CSECT ,
ACF2PWX  AMODE 31
ACF2PWX  RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING ACF2PWX,R12     *** Establish addressability
*
***********************************************************************
* Get ACVALD pointer from parameter list                               *
***********************************************************************
         L     R2,0(,R1)           Load ACVALD pointer
*
***********************************************************************
* Check if new password is provided                                    *
***********************************************************************
         TM    0(R2),X'80'         New password flag set?
         BNO   ACCEPT              No new password - accept
*
***********************************************************************
* Rule 1: Password must not equal logonid                              *
* Compare new password to logonid (constant-time for security)         *
***********************************************************************
         XR    R3,R3               Clear result register
         LA    R4,8                Loop count
         LA    R5,42(R2)           Point to new password
         LA    R6,4(R2)            Point to logonid
CHKUID   DS    0H
         XR    R7,R7               Clear work register
         IC    R7,0(R5)            Load password byte
         XR    R8,R8               Clear work register
         IC    R8,0(R6)            Load logonid byte
         XR    R7,R8               XOR the bytes
         OR    R3,R7               Accumulate differences
         LA    R5,1(,R5)           Advance password pointer
         LA    R6,1(,R6)           Advance logonid pointer
         BCT   R4,CHKUID           Continue comparison
*
         LTR   R3,R3               Any difference found?
         BZ    REJ1005             No - password = logonid, reject
*
***********************************************************************
* Rule 2: At least one numeric character                               *
***********************************************************************
         LA    R4,8                Loop count
         LA    R5,42(R2)           Point to new password
         XR    R3,R3               Clear found flag
CHKNUM   DS    0H
         CLI   0(R5),C'0'          Is this >= '0'?
         BL    NOTNUM              No - not numeric
         CLI   0(R5),C'9'          Is this <= '9'?
         BH    NOTNUM              No - not numeric
         LA    R3,1                Set found flag
         B     NUMOK               Found a digit
NOTNUM   DS    0H
         LA    R5,1(,R5)           Advance pointer
         BCT   R4,CHKNUM           Continue checking
*
         LTR   R3,R3               Was a digit found?
         BZ    REJ1008             No - reject
NUMOK    DS    0H
*
***********************************************************************
* Rule 3: No 3 consecutive identical characters                        *
***********************************************************************
         LA    R4,6                Check 6 positions (for 8-char pw)
         LA    R5,42(R2)           Point to new password
CHKREP   DS    0H
         CLC   0(1,R5),1(R5)      First two match?
         BNE   REPNEXT             No - advance
         CLC   0(1,R5),2(R5)      Three match?
         BE    REJ1004             Yes - reject
REPNEXT  DS    0H
         LA    R5,1(,R5)           Advance pointer
         BCT   R4,CHKREP           Continue checking
*
***********************************************************************
* Rule 4: Not a common/dictionary word                                 *
***********************************************************************
         LA    R5,BADLIST           Point to bad password list
         LA    R6,BADNOE            Number of entries
CHKBAD   DS    0H
         CLC   42(8,R2),0(R5)     Match bad password?
         BE    REJ1003             Yes - reject
         LA    R5,8(,R5)           Next entry
         BCT   R6,CHKBAD           Continue checking
*
***********************************************************************
* All rules passed - accept the password                               *
***********************************************************************
ACCEPT   DS    0H
         LA    R15,0               RC=0 - accept password
         B     RETURN              Return
*
***********************************************************************
* Reject - password equals logonid                                     *
***********************************************************************
REJ1005  DS    0H
         MVC   2(2,R2),=H'1005'   Reason: contains userid
         WTO   'ACF2PWX Password rejected - equals logonid',          X
               ROUTCDE=(9,11)
         LA    R15,4               RC=4 - reject
         B     RETURN              Return
*
***********************************************************************
* Reject - no numeric character                                        *
***********************************************************************
REJ1008  DS    0H
         MVC   2(2,R2),=H'1008'   Reason: no numeric
         WTO   'ACF2PWX Password rejected - no numeric character',    X
               ROUTCDE=(11)
         LA    R15,4               RC=4 - reject
         B     RETURN              Return
*
***********************************************************************
* Reject - sequential/repeated characters                              *
***********************************************************************
REJ1004  DS    0H
         MVC   2(2,R2),=H'1004'   Reason: sequential chars
         WTO   'ACF2PWX Password rejected - repeated characters',     X
               ROUTCDE=(11)
         LA    R15,4               RC=4 - reject
         B     RETURN              Return
*
***********************************************************************
* Reject - dictionary/common word                                      *
***********************************************************************
REJ1003  DS    0H
         MVC   2(2,R2),=H'1003'   Reason: dictionary word
         WTO   'ACF2PWX Password rejected - common word',             X
               ROUTCDE=(11)
         LA    R15,4               RC=4 - reject
*
***********************************************************************
* Return to ACF2                                                       *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to ACF2
*
***********************************************************************
* Bad password list (8-byte entries, uppercase, blank padded)          *
***********************************************************************
BADLIST  DS    0D
         DC    CL8'PASSWORD'
         DC    CL8'PASSW0RD'
         DC    CL8'12345678'
         DC    CL8'QWERTY  '
         DC    CL8'ABCD1234'
         DC    CL8'LETMEIN '
         DC    CL8'WELCOME '
         DC    CL8'CHANGE  '
         DC    CL8'SECURITY'
         DC    CL8'TEST1234'
BADNOE   EQU   (*-BADLIST)/8      Number of entries
*
***********************************************************************
* Register Equates                                                     *
***********************************************************************
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R10      EQU   10
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
         END   ACF2PWX
