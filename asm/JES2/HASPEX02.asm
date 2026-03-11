*********************************************************************
*                                                                   *
*                        HASP EXIT 2                                *
*                                                                   *
* SOURCE              =  CBTTape File 346 (Bob Break)               *
*                                                                   *
*     This exit gets control when each JOB statement or JOB         *
*     JOB continuation statement is processed and performs the      *
*     following functions:                                          *
*                                                                   *
*     >  Obtain storage for and initialize the SWBTJCT control      *
*        block, as an extension to the IBM JCT (JCTX).              *
*     >  Scan a jcl job card for a symbolic jobclass on the         *
*        CLASS= keyword and convert the symbolic designation to     *
*        a standard jobclass (A-Z, or 0-9).                         *
*     >  Scan a jcl job card for a symbolic msgclass on the         *
*        MSGCLASS= keyword and convert the symbolic designation     *
*        to a standard msgclass (A-Z, or 0-9).                      *
*                                                                   *
*     Entry point: EXIT02                                           *
*                                                                   *
*     Input registers:                                              *
*        R0     Code indicating type of job statement being scanned *
*               0  - initial JOB statement image                    *
*               4  - subsequent JOB statement continuation image    *
*        R1     Address of 3-word parameter list                    *
*        R10    JCT address                                         *
*        R11    HCT address                                         *
*        R13    PCE address                                         *
*        R14    Return address                                      *
*        R15    Entry address                                       *
*                                                                   *
*     Output registers:                                             *
*        R0-14  Unchanged                                           *
*        R15    Return code                                         *
*                                                                   *
*     Author: Bob Break                                             *
*                                                                   *
*********************************************************************

         COPY  $HASPGBL

HASPEX02 $MODULE ENVIRON=JES2,                                         X
               RMODE=ANY,                                              X
               SPLEVEL=NOCHECK,                                        X
               CVT,                                                    X
               $BUFFER,                                                X
               $HASPEQU,                                               X
               $HCT,                                                   X
               $JCT,                                                   X
               $JQE,                                                   X
               $PCE

EXIT02  $ENTRY BASE=R12            EXIT02 routine entry point

*********************************************************************
*        Save caller's registers and establish addressabilities.    *
*********************************************************************

EXIT000 $SAVE  NAME=EXIT02         Save caller's registers
         LR    R12,R15             Set base register
         ST    R0,PCEUSER0         Save entry R0
         L     R2,0(,R1)           Get job statement buffer pointer
         USING JCT,R10

*********************************************************************
*        Obtain a module workarea.                                  *
*********************************************************************

         LA    R1,EXIT02WL         Get workarea length
         LA    R1,3(,R1)           Round up to word boundary
         SRL   R1,2                Convert to number of words
         LA    R1,1(,R1)           Add one for identifier
        $GETWORK WORDS=(R1),USE=EX02
         LA    R1,4(,R1)           Point past identifier
         LR    R9,R1               Set workarea address
         LR    R14,R1              Set move to address
         LA    R15,EXIT02WL        Set move to length
         XR    R1,R1               Set move from length and pad
         MVCL  R14,R0              Clear workarea
         USING EXIT02W,R9
         ST    R2,JOBBUFP          Save job statement image buffer ptr
         L     R0,PCEUSER0         Restore entry register 0
         LTR   R0,R0               Continuation job statement?
         BNZ   EXIT020             Yes - branch, continue

*********************************************************************
*        Search the job statement image for the CLASS= keyword.     *
*********************************************************************

EXIT020  LA    R3,64               Set maximum search length
         L     R4,JOBBUFP          Get job statement buffer pointer
EXIT150  CLC   0(7,R4),=C',CLASS='  CLASS= keyword?
         BE    EXIT200             Yes - branch, continue
         CLC   0(7,R4),=C' CLASS='  CLASS= Keyword?
         BE    EXIT200             Yes - branch, continue
         LA    R4,1(,R4)           Bump to next buffer position
         BCT   R3,EXIT150          Loop back to check next position
         B     EXIT399             Branch - no CLASS= keyword found

*********************************************************************
*        The CLASS= keyword has been found.                         *
*********************************************************************

EXIT200  LA    R4,7(,R4)           Point to actual jobclass
         LA    R5,1(,R4)           Point past possible length 1 class
         CLI   0(R5),C' '          Length 1 class followed by blank?
         BE    EXIT399             Yes - branch, continue
         CLI   0(R5),C','          Length 1 class followed by comma?
         BE    EXIT399             Yes - branch, continue

*********************************************************************
*        Isolate the symbolic jobclass and length.                  *
*********************************************************************

         LA    R5,1(,R5)           Point to next position
         LA    R3,6                Set loop control
EXIT210  CLI   0(R5),C' '          Blank after symbolic jobclass?
         BE    EXIT220             Yes - branch, get symbolic length
         CLI   0(R5),C','          Comma after symbolic jobclass?
         BE    EXIT220             Yes - branch, get symbolic length
         LA    R5,1(,R5)           Point to next position
         BCT   R3,EXIT210          Branch - check next position
EXIT220  SR    R5,R4               Get symbolic jobclass length
         STH   R5,JCLASSL          Set symbolic jobclass length
         BCTR  R5,0                Minus 1 for execute
         MVC   JOBCLASS(0),0(R4)   Executed instruction
         EX    R5,*-6              Set symbolic jobclass

*********************************************************************
*        Issue WTO with the symbolic jobclass information.          *
*********************************************************************

EXIT221  MVC   MSGAREA(MSGLEN),MSGSKEL  Set message skeleton
         MVC   MSGJOBID,JCTJOBID   Set jobid in message area
         MVC   MSGJNAM,JCTJNAME    Set jobname in message area
         MVC   MSGSYMJ,JOBCLASS    Set jobclass in message area
         LA    R1,MSGAREA          Point to WTO message area
         LA    R0,MSGLEN           Set WTO message length
        $WTO   (R1),(R0),JOB=NO    Issue the WTO

EXIT399  DS    0H

*********************************************************************
*        Set return code, restore caller's registers and return.    *
*********************************************************************

RETURN   L     R2,RETCODE          Get routine return code
         S     R9,$F4              Get EXIT02 workarea address
        $RETWORK (R9)              Return workarea
         LR    R15,R2              Set routine return code
        $RETURN RC=(R15)           Return to caller

         DROP  R9,R10              EXIT02W, JCT

         LTORG

MSGSKEL  EQU   *                   WTO message skeleton
         DC    X'900F'             Message id
         DC    CL8' '              Jobid from JCTJOBID
         DC    C' '
         DC    CL8' '              Jobname from JCTJNAME
         DC    C' '
         DC    CL8' '              Symbolic jobclass from jobcard
MSGLEN   EQU   *-MSGSKEL           Total length of WTO message

        $MODEND

EXIT02W  DSECT                     EXIT02 workarea
RETCODE  DS    F                   EXIT02 return code
JOBBUFP  DS    A                   Job statement image buffer pointer
JCLASS   DS    CL1                 Actual jobclass
JOBCLASS DS    CL8                 Symbolic jobclass
JCLASSL  DS    H                   Symbolic jobclass length
MSGAREA  DS    0C                  WTO message area
MSGIDENT DS    XL2                 Message identifier
MSGJOBID DS    CL8                 Jobid from JCTJOBID
         DS    C
MSGJNAM  DS    CL8                 Jobname from JCTJNAME
         DS    C
MSGSYMJ  DS    CL8                 Symbolic jobclass from jobcard
EXIT02WL EQU   *-EXIT02W           EXIT02 workarea length

         END
