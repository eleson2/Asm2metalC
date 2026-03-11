*********************************************************************
*                                                                   *
*                        HASP EXIT 20                               *
*                                                                   *
* SOURCE              =  CBTTape File 346 (Bob Break)               *
*                                                                   *
*     This exit gets control at end of job input and performs       *
*     the following functions:                                      *
*                                                                   *
*     > Sets the JES2 reader time and date in the JQE and           *
*       checkpoints the JQE.                                        *
*     > Forces batch jobs to output class "E".                      *
*                                                                   *
*     Entry point: EXIT20                                           *
*                                                                   *
*     Input registers:                                              *
*        R0     Code indicating                                     *
*         - 0 = Normal end of input                                 *
*         - 4 = Job has JECL error                                  *
*        R1-9   N/A                                                 *
*        R10    JCT address                                         *
*        R11    HCT address                                         *
*        R12    N/A                                                 *
*        R13    PCE address                                         *
*        R14    Return address                                      *
*        R15    Entry address                                       *
*                                                                   *
*     Output registers:                                             *
*        R0-14  Unchanged                                           *
*        R15:   Return code                                         *
*                                                                   *
*     Register usage(internal):                                     *
*        R0-8   Work registers                                      *
*        R9     EXIT20 workarea address                             *
*        R10    JCT address                                         *
*        R11    HCT address                                         *
*        R12    Base register                                       *
*        R13    PCE address                                         *
*        R14-15 Work and linkage registers                          *
*                                                                   *
*     Author: Bob Break                                             *
*                                                                   *
*********************************************************************

         COPY  $HASPGBL

HASPEX20 $MODULE ENVIRON=JES2,                                         X
               RMODE=ANY,                                              X
               SPLEVEL=NOCHECK,                                        X
               $BUFFER,                                                X
               $HASPEQU,                                               X
               $HCT,                                                   X
               $JCT,                                                   X
               $JQE,                                                   X
               $PCE,                                                   X
               $RDRWORK,                                               X
               $SQD

EXIT20  $ENTRY BASE=R12

*********************************************************************
*                                                                   *
*        Save caller's registers and establish addressabilities.    *
*                                                                   *
*********************************************************************

         USING JCT,R10
EXIT000 $SAVE  NAME=EXIT20
         LR    R12,R15
         LA    R1,EXIT20WL
         LA    R1,3(,R1)
         SRL   R1,2
         LA    R1,1(,R1)
        $GETWORK WORDS=(R1),USE=EX20
         LA    R1,4(,R1)
         LR    R9,R1
         LA    R15,EXIT20WL
         XR    R1,R1
         MVCL  R14,R0
         USING EXIT20W,R9

*********************************************************************
*                                                                   *
*        Obtain the current time of day and date, and then set      *
*        the values into the JQE.                                   *
*                                                                   *
*********************************************************************

EXIT100 $DOGJQE ACTION=(FETCH,READ),JQE=PCEJQE
         LR    R1,R0
         USING JQE,R1
         CLC   JQERDRON,$ZEROS
         BNE   EXIT199
         DROP  R1
         L     R2,RDWSQD
         USING SQD,R2
         XC    SQDLEVEL+1(SQDEND-(SQDLEVEL+1)),SQDLEVEL+1
         DROP  R2
         LR    R0,R9
        $SUBIT RDRTIME,SQDADDR=(R2),PARM0=(R0)
         LTR   R15,R15
         BNZ   EXIT199

*********************************************************************
*                                                                   *
*        Obtain access to the ckpt queues and checkpoint the JQE.   *
*                                                                   *
*********************************************************************

        $QSUSE
        $DOGJQE ACTION=(FETCH,UPDATE),JQE=PCEJQE
         LR    R1,R0
         USING JQE,R1
         MVC   JQERDRON,EX20TIME
         MVC   JQERDTON,EX20DATE
        $DOGJQE ACTION=RETURN,CBADDR=JQE
         DROP  R1
EXIT199  DS    0H

*********************************************************************
*                                                                   *
*        Force batch jobs to msgclass "E".                          *
*                                                                   *
*********************************************************************

EXIT200  CLI   JCTJOBID,C'J'
         BNE   EXIT299
         MVI   JCTMCLAS,C'E'
EXIT299  DS    0H

*********************************************************************
*                                                                   *
*        Return to caller.                                          *
*                                                                   *
*********************************************************************

RETURN   S     R9,$F4
        $RETWORK (R9)
         XR     R15,R15
        $RETURN RC=(R15)

         DROP  R9,R10

         LTORG


***************************************************************
*                                                             *
*        RDRTIME - Obtain time of day and date.               *
*                                                             *
*   Function: This routine is called by $SUBIT. It issues     *
*             the MVS TIME macro to obtain current time       *
*             of day and date.                                *
*                                                             *
*   Linkage: Via $CALL from the general subtask facility      *
*                                                             *
*   Environment: Subtask                                      *
*                                                             *
***************************************************************

         USING RDRTIME,R12
RDRTIME  BAKR  R14,0
         LR    R12,R15
         LR    R3,R0
        $GETHP TYPE=GET,VERSIZE=RDRTIMWL
         LR    R9,R1
         USING RDRTIMW,R9
         LR    R14,R1
         LA    R15,RDRTIMWL
         XR    R1,R1
         MVCL  R14,R0
         ST    R13,RDRTR13
         LA    R13,RDRTSAVE
         TIME  DEC,RDRTIMWT,DATETYPE=MMDDYYYY,LINKAGE=SYSTEM,          X
               MF=(E,TIMELIST)
         L     R13,RDRTR13
         USING EXIT20W,R3
         MVC   EX20TIME,RDRTIMWT
         MVC   EX20DATE,RDRTIMWT+8
         DROP  R3
        $GETHP TYPE=FREE,CELL=(R9)
         PR
         DROP  R9,R12

         LTORG

        $MODEND

RDRTIMW  DSECT
RDRTR13  DS    A
RDRTSAVE DS    18F
RDRTIMWT DS    CL16
TIMELIST TIME  LINKAGE=SYSTEM,MF=L
RDRTIMWL EQU   *-RDRTIMW

EXIT20W  DSECT
EX20TIME DS    CL8
EX20DATE DS    CL8
EXIT20WL EQU   *-EXIT20W

         END
