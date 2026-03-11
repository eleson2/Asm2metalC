 TITLE 'ICHPWX01 - RACF Password Quality Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  ICHPWX01                                     *
*                                                                     *
* DESCRIPTIVE NAME    =  RACF Password Quality Exit                   *
*                                                                     *
* SOURCE              =  CBTTape File 728 (Dave Jousma)               *
*                                                                     *
* FUNCTION =                                                          *
*    The password quality rules are:                                  *
*                                                                     *
*    1) Password must be 8 characters in length. This rule is         *
*       enforced externally via RACF specifications.                  *
*    2) Password must contain at least one non-alphabetic character.  *
*       This rule is enforced by this exit.                           *
*    3) The first and last characters of the password must be non-    *
*       numeric. This rule is enforced by this exit.                  *
*    4) Password must not be the same as your 10 previous passwords.  *
*       This rule is enforced externally via RACF specifications.     *
*    5) Password must not contain 4 consecutive characters of your    *
*       previous password. This rule is enforced in this exit.        *
*    6) Password must not contain 3 identical adjacent characters.    *
*       This rule is enforced by this exit.                           *
*    7) Password must not contain your User ID. This rule is enforced *
*       by this exit.                                                 *
*    8) Password must not contain certain keywords.  This rule is     *
*       enforced by this exit.                                        *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, REUSABLE, AMODE 31, RMODE ANY     *
*                                                                     *
***********************************************************************
ICHPWX01 CSECT ,
ICHPWX01 AMODE 31                  31-bit addressing mode
ICHPWX01 RMODE ANY                 31-bit residence
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING ICHPWX01,R12    *** Synchronize base register
         LR    R10,R1              Load PWXPL address
         USING PWXPL,R10       *** Synchronize PWXPL DSECT
         GETMAIN R,LV=X01WKLEN     Acquire working storage
         LR    R2,R1               Save address of working storage
         LR    R0,R1               Zero out area using MVCL
         LA    R1,X01WKLEN         (same)
         XR    R15,R15             (same)
         MVCL  R0,R14              (same)
         ST    R2,8(,R13)          Chain save areas together
         ST    R13,4(,R2)          (same)
         LR    R13,R2              Load R13 with work area address
         USING X01WK,R13       *** Synchronize X01WK DSECT

**********************************************************************
* We must first determine who is calling us. We can be called from   *
* RACINIT processing, the PASSWORD command, or the ALTUSER command.  *
* We only want to control passwords for RACINIT processing and       *
* the PASSWORD command.                                              *
**********************************************************************
X01A010  DS    0H
         L     R3,PWXCALLR               Load caller flag
         CLI   0(R3),PWXRINIT            Called by RACINIT?
         BE    X01A100                   yes, continue
         CLI   0(R3),PWXPWORD            Called by PASSWORD?
         BE    X01A100                   yes, continue
         B     X01A900                   anything else, ok, and exit

**********************************************************************
* Check if New Password Specified                                    *
**********************************************************************
X01A100  DS    0H
         ICM   R0,B'1111',PWXNEWPW Point to new password field
         BZ    X01A900             Branch if no new password

**********************************************************************
* Enforce Rule #2 - At Least One Non-Alpha Character                 *
**********************************************************************
X01A200  DS    0H
         L     R3,PWXNEWPW         Point to new password field
         XR    R1,R1               Get length of new password
         IC    R1,0(,R3)           (same)
         BCTR  R1,0                Make relative to zero
         TRT   1(*-*,R3),X01NALPH  Test for non-alphabetic
         EX    R1,*-6              (same)
         BZ    X01BPW02            Password violates rule #2

**********************************************************************
* Enforce Rule #5 - Not Four Consecutive Chars of Previous           *
**********************************************************************
X01A400  DS    0H
         L     R3,PWXNEWPW         Point to new password field
         XR    R4,R4               Get length of new password
         IC    R4,0(,R3)           (same)
         SH    R4,=H'4'            Compute length - 4
         BNP   X01A500             Branch if not > 4 chars
         ICM   R5,B'1111',PWXCURPW Point to old password field
         BZ    X01A500             Branch if no old password
         XR    R6,R6               Get length of old password
         IC    R6,0(,R5)           (same)
         SH    R6,=H'4'            Compute length - 4
         BNP   X01A500             Branch if not > 4 chars
X01A410  DS    0H
         LR    R1,R5               Get old pw address
         LR    R0,R6               Get old pw length - 4
X01A420  DS    0H
         CLC   1(4,R3),1(R1)       Four characters match ?
         BE    X01BPW05            Branch if yes (violates #5)
         LA    R1,1(,R1)           Advance old pw pointer
         BCT   R0,X01A420          Continue comparisons
         LA    R3,1(,R3)           Advance new pw pointer
         BCT   R4,X01A410          Continue comparisons

**********************************************************************
* Enforce Rule #6 - Not Three Consecutive Equal Chars                *
**********************************************************************
X01A500  DS    0H
         L     R3,PWXNEWPW         Point to new password field
         XR    R0,R0               Get length of new password
         IC    R0,0(,R3)           (same)
         SH    R0,=H'3'            Compute length - 3
         BNP   X01A600             Branch if not > 3 chars
X01A510  DS    0H
         CLC   1(1,R3),2(R3)       Two characters match ?
         BNE   X01A520             Branch if not
         CLC   1(1,R3),3(R3)       Three characters match ?
         BE    X01BPW06            Branch if yes (violates #6)
X01A520  DS    0H
         LA    R3,1(,R3)           Advance pointer
         BCT   R0,X01A510          Continue comparisons

**********************************************************************
* Enforce Rule #7 - Password Does not Contain User ID                *
**********************************************************************
X01A600  DS    0H
         L     R3,PWXNEWPW         Point to new password field
         XR    R4,R4               Get length of new password
         IC    R4,0(,R3)           (same)
         ICM   R5,B'1111',PWXUSRID Point to user ID field
         BZ    X01A700             Branch if user ID unknown
         XR    R6,R6               Get length of user ID
         IC    R6,0(,R5)           (same)
         SR    R4,R6               Get pw length - user ID length
         BM    X01A700             Branch if password is shorter
         BCTR  R6,0                User ID length rel. to zero
X01A610  DS    0H
         CLC   1(*-*,R5),1(R3)     User ID found in password ?
         EX    R6,*-6              (same)
         BE    X01BPW07            Branch if yes (violates #7)
         LA    R3,1(,R3)           Advance pointer
         SH    R4,=H'1'            Decrement length remaining
         BP    X01A610             Branch if more to check

**********************************************************************
*  Enforce Rule #8 - Password Does not Contain Reserved words        *
**********************************************************************
X01A700  DS    0H
         LA    R6,PASSTAB          Point to start of table
         SLR   R7,R7               Clear register
         LH    R7,=AL2(PASSNOE)    Get number of entries
X01A710  BAS   R14,X01A740         Find length of entry
         L     R3,PWXNEWPW         Point to new password field
         XR    R4,R4               Get length of new password
         IC    R4,0(,R3)           (same)
         CR    R1,R4               Compare lengths
         BH    X01A730             If entry > new password
         SR    R4,R1               Subtract for find limit
         LA    R5,1(,R3)           Point to new password
         BCTR  R1,0                Prepare for execute
         LA    R4,1(,R4)           Adjust For Rel.
X01A720  EX    R1,CHECKPW          String found in password ?
         BE    X01BPW08            Yes, exit with RC=4
X01A730  LA    R6,8(R6)            Bump up table entry
         BCT   R7,X01A710          Branch back round
         B     X01A900             Passed all entries, exit
X01A740  DS    0H
         SLR   R1,R1               Clear register
         LR    R15,R6              Point to start of entry
         LA    R2,8                Set maximum count
X01A750  CLI   0(R15),C' '         Do we have space ?
         BE    X01A760             Yes, get out
         LA    R15,1(,R15)         Bump up byte
         LA    R1,1(,R1)           Increment counter
         BCT   R2,X01A750          Loop back round
X01A760  BR    R14                 Go back to caller

**********************************************************************
* Executed Instructions                                              *
**********************************************************************
CHECKPW  CLC   0(*-*,R5),0(R6)     Compare password to string

**********************************************************************
* Accept New Password                                                *
**********************************************************************
X01A900  DS    0H
         L     R15,4(,R13)         Point to caller's save area
         MVC   16(4,R15),=F'0'     Set zero return code
         B     X01Z900             Return

**********************************************************************
* Reject New Password                                                *
**********************************************************************
X01BPW02 DS    0H
         WTO   'ICHPWX02 Password must contain at least one non-alphabe+
               tic character',ROUTCDE=(11)
         B     X01Z100             Continue
X01BPW05 DS    0H
         WTO   'ICHPWX05 Password must not contain 4 consecutive charac+
               ters of previous password',ROUTCDE=(11)
         B     X01Z100             Continue
X01BPW06 DS    0H
         WTO   'ICHPWX06 Password must not contain 3 identical adjacent+
                characters',ROUTCDE=(11)
         B     X01Z100             Continue
X01BPW07 DS    0H
         WTO   'ICHPWX07 Password must not contain your User ID',      +
               ROUTCDE=(11)
         B     X01Z100             Continue
X01BPW08 DS    0H
         WTO   'ICHPWX08 Password has invalid character combination',  +
               ROUTCDE=(11)
         B     X01Z100             Continue

X01Z100  DS    0H
         L     R15,4(,R13)         Point to caller's save area
         MVC   16(4,R15),=F'4'     Set return code = '4'
         B     X01Z900             Return

**********************************************************************
* Free Work Area and Return                                          *
**********************************************************************
X01Z900  DS    0H
         L     R2,4(,R13)          Load caller's save area address
         FREEMAIN R,LV=X01WKLEN,   Free work area                      +
               A=(13)              (same)
         LR    R13,R2              Place save area address in R13
         RETURN (14,12)            Return to caller
         EJECT ,
**********************************************************************
* Constants                                                          *
**********************************************************************
X01NALPH DC    256AL1(*-X01NALPH)  Non-alphabetic table
         ORG   X01NALPH+C'A'       Uppercase 'A'-'I' set to x'00'
         DC    9X'00'              (same)
         ORG   X01NALPH+C'J'       Uppercase 'J'-'R' set to x'00'
         DC    9X'00'              (same)
         ORG   X01NALPH+C'S'       Uppercase 'S'-'Z' set to x'00'
         DC    8X'00'              (same)
         ORG   ,
**********************************************************************
* Restricted Password table - contains poor quality patterns         *
**********************************************************************
PASSTAB  DS    0D
         DC    CL8'JAN     '
         DC    CL8'FEB     '
         DC    CL8'MAR     '
         DC    CL8'APR     '
         DC    CL8'MAY     '
         DC    CL8'JUN     '
         DC    CL8'JUL     '
         DC    CL8'AUG     '
         DC    CL8'SEP     '
         DC    CL8'OCT     '
         DC    CL8'NOV     '
         DC    CL8'DEC     '
         DC    CL8'PASS    '
         DC    CL8'TEST    '
         DC    CL8'USER    '
         DC    CL8'1234    '
         DC    CL8'QWERTY  '
PASSNOE  EQU   ((*-PASSTAB)/8)
**********************************************************************
* Dummy Control Sections and EQUates                                 *
**********************************************************************
X01WK    DSECT ,                   Module Work Area
X01WKSAV DS    18F                 Standard save area
         DS    0D                  Align to doubleword
X01WKLEN EQU   *-X01WK             Length of work area
         ICHPWXP ,                 PWXPL
         YREGS ,                   Registers
         END   ICHPWX01
