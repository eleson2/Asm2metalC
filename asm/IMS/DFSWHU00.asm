***********************************************************************
* MODULE NAME: DFSWHU00                                               *
* FUNCTION: IMS GREETING/SIGN-ON EXIT CALLING SAF                     *
***********************************************************************
DFSWHU00 CSECT
DFSWHU00 AMODE 31
DFSWHU00 RMODE ANY
         BAKR  R14,0               * Save context on linkage stack
         LR    R12,R15             * Establish base
         USING DFSWHU00,R12
*
         L     R2,0(,R1)           * R1 points to a list of pointers
         L     R3,0(,R2)           * R3 = Address of User ID (8 bytes)
         L     R4,4(,R2)           * R4 = Address of Group Name (8 bytes)
*
***********************************************************************
* GET STORAGE FOR SAF WORK AREA (REQUIRED FOR REENTRANCY)             *
***********************************************************************
         STORAGE OBTAIN,LENGTH=512,ADDR=(R11),LOC=ANY
         LR    R10,R11             * R10 points to our work area
*
***********************************************************************
* ISSUE RACROUTE REQUEST=AUTH                                         *
***********************************************************************
* This is the "Complex" part for your regression test.                *
***********************************************************************
         RACROUTE REQUEST=AUTH,                                        +
               CLASS='APPL',                                           +
               ENTITY=('IMSPROD'),                                     +
               USERID=(R3),                                            +
               WORKA=(R10),                                            +
               ATTR=READ,                                              +
               MF=(E,RACLIST)
*
         ST    R15,SAF_RC          * Save return code
*
***********************************************************************
* EVALUATE SAF RETURN CODE                                            *
***********************************************************************
         LTR   R15,R15             * Was access granted (RC=0)?
         BZ    ALLOW_USER
*
         LA    R15,8               * RC=8: Reject the sign-on
         B     CLEANUP
*
ALLOW_USER EQU *
         XR    R15,R15             * RC=0: Allow the sign-on
*
CLEANUP  EQU   *
         LR    R5,R15              * Save our RC across STORAGE RELEASE
         STORAGE RELEASE,LENGTH=512,ADDR=(R11)
         LR    R15,R5              * Restore our RC
         PR                        * Return to IMS
*
***********************************************************************
* DATA AREAS AND PARAMETER LISTS                                      *
***********************************************************************
SAF_RC   DS    F
RACLIST  RACROUTE REQUEST=AUTH,MF=L  * Generate the static parm list
*
         END   DFSWHU00

