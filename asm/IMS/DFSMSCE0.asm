***********************************************************************
* MODULE NAME: DFSMSCE0                                               *
* FUNCTION: TM AND MSC MESSAGE ROUTING AND CONTROL EXIT               *
***********************************************************************
DFSMSCE0 CSECT
DFSMSCE0 AMODE 31
DFSMSCE0 RMODE ANY
         BAKR  R14,0               * Save registers on linkage stack
         LR    R12,R15             * Set up base register
         USING DFSMSCE0,R12
*
         L     R2,0(,R1)           * R2 = Address of the MSCP (Parmlist)
         USING MSCP,R2             * Map the MSC Parameter List
*
***********************************************************************
* ROUTE BASED ON FUNCTION CODE                                        *
***********************************************************************
         L     R3,MSCPFUNC         * Load function code
         C     R3,=F'1'            * Is it Function 1 (Entry/Init)?
         BE    INIT_RTN
         C     R3,=F'2'            * Is it Function 2 (Msg Routing)?
         BE    ROUTE_RTN
         B     EXIT_OK             * Ignore other functions
*
***********************************************************************
* ROUTE_RTN: Inspects message and overrides destination if needed     *
***********************************************************************
ROUTE_RTN EQU  *
         L     R4,MSCPdest         * R4 = Address of Destination block
         USING MSCD,R4             * Map the Destination DSECT
*
         TM    MSCDFLG1,MSCDLOG    * Is this a logical terminal?
         BNO   EXIT_OK             * If not, we don't touch it
*
* COMPLEX LOGIC: If Destination is 'TERM1', reroute to 'TERM2'
         CLC   MSCDNAME(8),=CL8'TERM1'
         BNE   EXIT_OK
*
         MVC   MSCDNAME(8),=CL8'TERM2' * Override destination
         OI    MSCPFLG1,MSCPREQD   * Tell IMS we modified the dest
*
EXIT_OK  EQU   *
         XR    R15,R15             * Return Code 0
         PR                        * Return
*
INIT_RTN EQU   *
         * [Initialization Logic Here]
         B     EXIT_OK
*
***********************************************************************
* DSECT MAPPINGS                                                      *
***********************************************************************
MSCP     DSECT                     * MSC Parameter List
MSCPFUNC DS    F                   * Function Code
MSCPFLG1 DS    X                   * Status Flags
MSCPREQD EQU   X'80'               * Flag: Reroute requested
         DS    XL3                 * Reserved
MSCPdest DS    A                   * Pointer to Destination Block
MSCPmsga DS    A                   * Pointer to Message Prefix
*
MSCD     DSECT                     * Destination Block Mapping
MSCDNAME DS    CL8                 * Destination Name
MSCDFLG1 DS    B                   * Type Flags
MSCDLOG  EQU   X'01'               * Bit: Destination is an LTERM
         END   DFSMSCE0
