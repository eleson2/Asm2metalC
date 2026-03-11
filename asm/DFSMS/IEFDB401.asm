 TITLE 'IEFDB401 - DFSMS Dynamic Allocation Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  IEFDB401                                     *
*                                                                     *
* DESCRIPTIVE NAME    =  Dynamic Allocation Installation Exit         *
*                                                                     *
* SOURCE              =  Based on CBTTape File 067 patterns           *
*                                                                     *
* FUNCTION =                                                          *
*    Called by the allocation routines before and after allocation.    *
*    This exit can modify allocation parameters such as unit,          *
*    volume, space, and SMS classes.                                    *
*                                                                     *
*    This example:                                                     *
*    1) Redirects large allocations (>1000 cyl) to LARGE storage cls  *
*    2) Enforces PROD storage class for production HLQ datasets       *
*    3) Blocks allocation of datasets to specific volumes              *
*    4) Logs all allocations exceeding threshold                       *
*                                                                     *
*    Return codes:                                                     *
*      0  = Continue with allocation as-is                             *
*      4  = Allocation modified by exit                                *
*      8  = Reject the allocation                                      *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Pointer to parameter list                                   *
*    R2  = Work register                                               *
*    R3  = Work register                                               *
*    R12 = Base register                                               *
*                                                                     *
* PARAMETER LIST (IEFDB401 exit parameter block):                     *
*    +0(4)  Work area pointer                                          *
*    +4(1)  Function code (1=alloc, 2=unalloc, 3=concat)              *
*    +5(1)  Flags                                                      *
*    +8(44) Dataset name                                               *
*    +52(8) DD name                                                    *
*    +60(6) Volume serial                                              *
*    +68(8) Unit name                                                  *
*    +76(4) Primary allocation (quantity)                              *
*    +80(4) Secondary allocation (quantity)                            *
*    +84(1) Space type (1=TRK, 2=CYL, 3=BLK)                        *
*    +96(8) User ID                                                    *
*    +104(8) Job name                                                  *
*    +128(8) Data class (I/O)                                          *
*    +136(8) Storage class (I/O)                                       *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, AMODE 31, RMODE ANY               *
*                                                                     *
***********************************************************************
IEFDB401 CSECT ,
IEFDB401 AMODE 31
IEFDB401 RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING IEFDB401,R12    *** Establish addressability
         LR    R10,R1              Save parameter list address
*
***********************************************************************
* Check function code - only process allocations                       *
***********************************************************************
         CLI   4(R10),1            Is this an allocation request?
         BNE   CONTINUE            No - continue
*
***********************************************************************
* Check for large allocation - primary > 1000 cylinders                *
***********************************************************************
         CLI   84(R10),2           Is space type cylinders?
         BNE   CHKPROD             No - check next rule
         L     R3,76(,R10)         Load primary allocation
         C     R3,=F'1000'         Is it > 1000 cylinders?
         BNH   CHKPROD             No - check next rule
*
* Large allocation - override storage class to LARGE                   *
         MVC   136(8,R10),=CL8'LARGE'  Set storage class
*
* Log the large allocation                                             *
         MVC   WTOLRG+4+22(8),104(R10)  Move job name
         MVC   WTOLRG+4+35(44),8(R10)   Move dataset name
         WTO   MF=(E,WTOLRG)      Issue WTO
         LA    R15,4               RC=4 - allocation modified
         B     RETURN              Return
*
***********************************************************************
* Check for PROD HLQ - enforce FAST storage class                     *
***********************************************************************
CHKPROD  DS    0H
         CLC   8(5,R10),=C'PROD.' Dataset HLQ = PROD?
         BNE   CHKVOL              No - check next rule
*
* PROD datasets - ensure FAST storage class                            *
         CLC   136(8,R10),=CL8'FAST'  Already FAST?
         BE    CONTINUE            Yes - no change needed
         MVC   136(8,R10),=CL8'FAST'  Set storage class
         LA    R15,4               RC=4 - allocation modified
         B     RETURN              Return
*
***********************************************************************
* Check for restricted volumes                                         *
***********************************************************************
CHKVOL   DS    0H
         CLC   60(6,R10),=CL6'RSVD01'  Restricted volume?
         BE    REJECT              Yes - reject
         CLC   60(6,R10),=CL6'RSVD02'  Restricted volume?
         BE    REJECT              Yes - reject
*
***********************************************************************
* Continue with allocation unchanged                                   *
***********************************************************************
CONTINUE DS    0H
         LA    R15,0               RC=0 - continue unchanged
         B     RETURN              Return
*
***********************************************************************
* Reject the allocation                                                *
***********************************************************************
REJECT   DS    0H
         MVC   WTOREJ+4+24(8),104(R10)  Move job name
         MVC   WTOREJ+4+37(6),60(R10)   Move volume serial
         WTO   MF=(E,WTOREJ)      Issue WTO
         LA    R15,8               RC=8 - reject
*
***********************************************************************
* Return to allocation routines                                        *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return to caller
*
***********************************************************************
* WTO Areas                                                            *
***********************************************************************
WTOLRG   WTO   'IEFDB401 LARGE ALLOC JOB=XXXXXXXX DSN=XXXXXXXXXXXXXXXXXXXX
               XXXXXXXXXXXXXXXXXXXXXXXXXX',ROUTCDE=(11),MF=L
WTOREJ   WTO   'IEFDB401 ALLOC REJECTED JOB=XXXXXXXX VOL=XXXXXX',    X
               ROUTCDE=(2,11),MF=L
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
         END   IEFDB401
