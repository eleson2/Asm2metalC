 TITLE 'IEFU83 - SMF Record Filtering Exit'
***********************************************************************
*                                                                     *
* MODULE NAME         =  IEFU83                                       *
*                                                                     *
* DESCRIPTIVE NAME    =  SMF Record Write Exit                        *
*                                                                     *
* SOURCE              =  Based on SYS1.SAMPLIB IEFU83 pattern         *
*                                                                     *
* FUNCTION =                                                          *
*    This exit is called by SMF before writing a record to the        *
*    SMF data set. It can:                                             *
*    1) Allow the record to be written (RC=0)                         *
*    2) Suppress the record (RC=4)                                    *
*    3) Halt SMF recording (RC=8)                                     *
*                                                                     *
*    This example suppresses SMF type 40 (storage class memory)       *
*    and type 42 (DFSMS statistics) records to reduce volume,         *
*    and records from job names starting with 'SYS' to reduce         *
*    system noise.                                                     *
*                                                                     *
* REGISTER USAGE:                                                     *
*    R1  = Pointer to parameter list on entry                         *
*    R2  = Pointer to SMF record                                      *
*    R3  = Work register                                              *
*    R12 = Base register                                              *
*    R15 = Return code on exit                                        *
*                                                                     *
* PARAMETER LIST (pointed to by R1):                                  *
*    +0  SMF Record Address (pointer)                                 *
*                                                                     *
* SMF RECORD HEADER:                                                  *
*    +0(2)  Record length                                             *
*    +2(2)  Segment descriptor                                        *
*    +4(1)  System indicator flags                                    *
*    +5(1)  Record type (0-255)                                       *
*    +6(4)  Time (0.01 second units since midnight)                   *
*    +10(4) Date (0cyydddF packed)                                    *
*    +14(4) System ID                                                 *
*                                                                     *
* ATTRIBUTES          =  REENTRANT, REUSABLE, AMODE 31, RMODE ANY    *
*                                                                     *
***********************************************************************
IEFU83   CSECT ,
IEFU83   AMODE 31                  31-bit addressing mode
IEFU83   RMODE ANY                 Residence anywhere
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING IEFU83,R12      *** Establish addressability
*
***********************************************************************
* Get pointer to SMF record from parameter list                       *
***********************************************************************
         L     R2,0(,R1)           Load pointer to SMF record
*
***********************************************************************
* Check record type - suppress type 40 and 42 records                 *
***********************************************************************
         CLI   5(R2),40            Is this a type 40 record?
         BE    SUPPRESS            Yes - suppress it
         CLI   5(R2),42            Is this a type 42 record?
         BE    SUPPRESS            Yes - suppress it
*
***********************************************************************
* Check for subtype header (flag bit x'80' at offset +4)              *
* If subtype header exists, job info may be at different offsets.      *
* For type 30 records (common addr space), check the job name         *
* in the identification section.                                       *
***********************************************************************
         CLI   5(R2),30            Is this a type 30 record?
         BNE   CHKBASIC            No - check basic header
*
* Type 30 - subtype header present. Check if system job.              *
* The identification section offset is at +24 in type 30 header.      *
         LH    R3,24(R2)           Load offset to ID section
         LTR   R3,R3               Is there an ID section?
         BZ    ALLOW               No - allow it
         AR    R3,R2               Point to ID section
         CLC   0(3,R3),=C'SYS'    Job name starts with SYS?
         BE    SUPPRESS            Yes - suppress system job records
         B     ALLOW               No - allow it
*
***********************************************************************
* Basic record header - check record type for ones that have          *
* job name at offset +18                                               *
***********************************************************************
CHKBASIC DS    0H
         CLI   5(R2),4             Type 4 (step termination)?
         BE    CHKJOB              Yes - check job name
         CLI   5(R2),5             Type 5 (job termination)?
         BE    CHKJOB              Yes - check job name
         CLI   5(R2),14            Type 14 (dataset input)?
         BE    CHKJOB              Yes - check job name
         CLI   5(R2),15            Type 15 (dataset output)?
         BE    CHKJOB              Yes - check job name
         B     ALLOW               Other types - allow
*
***********************************************************************
* Check if job name starts with 'SYS' - suppress system jobs          *
***********************************************************************
CHKJOB   DS    0H
         CLC   18(3,R2),=C'SYS'   Job name starts with SYS?
         BE    SUPPRESS            Yes - suppress it
*
***********************************************************************
* Allow the record to be written                                       *
***********************************************************************
ALLOW    DS    0H
         LA    R15,0               RC=0 - write the record
         B     RETURN              Return to caller
*
***********************************************************************
* Suppress the record (do not write)                                   *
***********************************************************************
SUPPRESS DS    0H
         LA    R15,4               RC=4 - suppress the record
*
***********************************************************************
* Return to SMF                                                        *
***********************************************************************
RETURN   DS    0H
         RETURN (14,12),RC=(15)    Return with RC in R15
*
***********************************************************************
* Register Equates                                                     *
***********************************************************************
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
         END   IEFU83
