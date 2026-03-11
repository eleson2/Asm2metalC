/*********************************************************************
 * METALC_MQ.H - IBM MQ Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for IBM MQ for z/OS exit development.
 *
 * Supported exits:
 *   Channel exits:
 *     MQXR_SEC    - Security exit
 *     MQXR_SEND   - Send exit
 *     MQXR_RECEIVE - Receive exit
 *     MQXR_MSG    - Message exit
 *
 *   API exits:
 *     MQ_CONN_EXIT - Connection exit
 *     MQ_DISC_EXIT - Disconnection exit
 *     MQ_OPEN_EXIT - Open exit
 *     MQ_CLOSE_EXIT - Close exit
 *     MQ_GET_EXIT  - Get exit
 *     MQ_PUT_EXIT  - Put exit
 *
 *   Other exits:
 *     Data conversion exit
 *     Cluster workload exit
 *     Publish exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_mq.h"
 *
 * IMPORTANT: MQ exits must be reentrant and thread-safe.
 *********************************************************************/

#ifndef METALC_MQ_H
#define METALC_MQ_H

#include "metalc_base.h"

/*===================================================================
 * MQ Exit Return Codes
 *===================================================================*/

/* General exit return codes */
#define MQXCC_OK               0     /* Continue processing           */
#define MQXCC_SUPPRESS_FUNCTION 1    /* Suppress MQ function          */
#define MQXCC_SKIP_FUNCTION    2     /* Skip this function            */
#define MQXCC_SUPPRESS_EXIT    3     /* Suppress further exit calls   */
#define MQXCC_CLOSE_CHANNEL    4     /* Close the channel             */
#define MQXCC_FAILED           -1    /* Exit failed                   */

/* Channel exit reason codes */
#define MQXR_INIT              1     /* Initialize                    */
#define MQXR_TERM              2     /* Terminate                     */
#define MQXR_MSG               3     /* Message processing            */
#define MQXR_XMIT_Q_MSG        4     /* Transmission queue message    */
#define MQXR_SEC_MSG           5     /* Security message              */
#define MQXR_INIT_SEC          6     /* Initialize security           */
#define MQXR_RETRY             7     /* Retry                         */
#define MQXR_SEC_PARMS         8     /* Security parameters           */

/* API exit function codes */
#define MQXF_CONN              1     /* MQCONN                        */
#define MQXF_CONNX             2     /* MQCONNX                       */
#define MQXF_DISC              3     /* MQDISC                        */
#define MQXF_OPEN              4     /* MQOPEN                        */
#define MQXF_CLOSE             5     /* MQCLOSE                       */
#define MQXF_PUT               6     /* MQPUT                         */
#define MQXF_PUT1              7     /* MQPUT1                        */
#define MQXF_GET               8     /* MQGET                         */
#define MQXF_INQ               9     /* MQINQ                         */
#define MQXF_SET               10    /* MQSET                         */
#define MQXF_BEGIN             11    /* MQBEGIN                       */
#define MQXF_CMIT              12    /* MQCMIT                        */
#define MQXF_BACK              13    /* MQBACK                        */
#define MQXF_SUB               14    /* MQSUB                         */
#define MQXF_SUBRQ             15    /* MQSUBRQ                       */

/*===================================================================
 * MQ Return and Reason Codes
 *===================================================================*/

/* Completion codes */
#define MQCC_OK                0     /* Successful completion         */
#define MQCC_WARNING           1     /* Warning                       */
#define MQCC_FAILED            2     /* Failed                        */

/* Common reason codes */
#define MQRC_NONE              0     /* No reason                     */
#define MQRC_APPL_FIRST        900   /* First application-defined     */
#define MQRC_APPL_LAST         999   /* Last application-defined      */
#define MQRC_NOT_AUTHORIZED    2035  /* Not authorized                */
#define MQRC_Q_MGR_STOPPING    2162  /* Queue manager stopping        */
#define MQRC_CONNECTION_BROKEN 2009  /* Connection broken             */
#define MQRC_NO_MSG_AVAILABLE  2033  /* No message available          */
#define MQRC_UNKNOWN_OBJECT_NAME 2085 /* Unknown object name          */
#define MQRC_HOST_NOT_AVAILABLE 2538 /* Host not available           */

/*===================================================================
 * MQ Object Types
 *===================================================================*/

#define MQOT_Q                 1     /* Queue                         */
#define MQOT_NAMELIST          2     /* Namelist                      */
#define MQOT_PROCESS           3     /* Process                       */
#define MQOT_STORAGE_CLASS     4     /* Storage class                 */
#define MQOT_Q_MGR             5     /* Queue manager                 */
#define MQOT_CHANNEL           6     /* Channel                       */
#define MQOT_AUTH_INFO         7     /* Authentication info           */
#define MQOT_TOPIC             8     /* Topic                         */
#define MQOT_CF_STRUC          9     /* CF structure                  */

/*===================================================================
 * MQ Queue Types
 *===================================================================*/

#define MQQT_LOCAL             1     /* Local queue                   */
#define MQQT_MODEL             2     /* Model queue                   */
#define MQQT_ALIAS             3     /* Alias queue                   */
#define MQQT_REMOTE            6     /* Remote queue                  */
#define MQQT_CLUSTER           7     /* Cluster queue                 */

/*===================================================================
 * MQ Channel Types
 *===================================================================*/

#define MQCHT_SENDER           1     /* Sender                        */
#define MQCHT_SERVER           2     /* Server                        */
#define MQCHT_RECEIVER         3     /* Receiver                      */
#define MQCHT_REQUESTER        4     /* Requester                     */
#define MQCHT_CLNTCONN         6     /* Client connection             */
#define MQCHT_SVRCONN          7     /* Server connection             */
#define MQCHT_CLUSRCVR         8     /* Cluster receiver              */
#define MQCHT_CLUSSDR          9     /* Cluster sender                */

/*===================================================================
 * MQMD - Message Descriptor
 *===================================================================*/

#pragma pack(1)

struct mqmd {
    char           strucId[4];       /* +0   Structure ID 'MD  '      */
    uint32_t       version;          /* +4   Structure version        */
    uint32_t       report;           /* +8   Report options           */
    uint32_t       msgType;          /* +12  Message type             */
    uint32_t       expiry;           /* +16  Expiry time              */
    uint32_t       feedback;         /* +20  Feedback code            */
    uint32_t       encoding;         /* +24  Encoding                 */
    uint32_t       codedCharSetId;   /* +28  Coded character set ID   */
    char           format[8];        /* +32  Message format           */
    uint32_t       priority;         /* +40  Message priority         */
    uint32_t       persistence;      /* +44  Persistence              */
    char           msgId[24];        /* +48  Message ID               */
    char           correlId[24];     /* +72  Correlation ID           */
    uint32_t       backoutCount;     /* +96  Backout count            */
    char           replyToQ[48];     /* +100 Reply-to queue           */
    char           replyToQMgr[48];  /* +148 Reply-to queue manager   */
    char           userId[12];       /* +196 User ID                  */
    char           accountingToken[32]; /* +208 Accounting token      */
    char           applIdentityData[32]; /* +240 Application data     */
    uint32_t       putApplType;      /* +272 Put application type     */
    char           putApplName[28];  /* +276 Put application name     */
    char           putDate[8];       /* +304 Put date                 */
    char           putTime[8];       /* +312 Put time                 */
    char           applOriginData[4]; /* +320 Application origin data */
    char           groupId[24];      /* +324 Group ID                 */
    uint32_t       msgSeqNumber;     /* +348 Message sequence number  */
    uint32_t       offset;           /* +352 Offset                   */
    uint32_t       msgFlags;         /* +356 Message flags            */
    uint32_t       originalLength;   /* +360 Original length          */
};                                   /* Total: 364 bytes              */

/* Message types */
#define MQMT_REQUEST           1     /* Request                       */
#define MQMT_REPLY             2     /* Reply                         */
#define MQMT_DATAGRAM          8     /* Datagram                      */
#define MQMT_REPORT            4     /* Report                        */

/* Persistence values */
#define MQPER_NOT_PERSISTENT   0     /* Not persistent                */
#define MQPER_PERSISTENT       1     /* Persistent                    */
#define MQPER_PERSISTENCE_AS_Q_DEF 2 /* As queue default            */

/* Priority values */
#define MQPRI_PRIORITY_AS_Q_DEF -1   /* As queue default              */

#pragma pack()

/*===================================================================
 * MQOD - Object Descriptor
 *===================================================================*/

#pragma pack(1)

struct mqod {
    char           strucId[4];       /* +0   Structure ID 'OD  '      */
    uint32_t       version;          /* +4   Structure version        */
    uint32_t       objectType;       /* +8   Object type              */
    char           objectName[48];   /* +12  Object name              */
    char           objectQMgrName[48]; /* +60 Queue manager name      */
    char           dynamicQName[48]; /* +108 Dynamic queue name       */
    char           alternateUserId[12]; /* +156 Alternate user ID     */
    uint32_t       recsPresent;      /* +168 Number of records        */
    uint32_t       knownDestCount;   /* +172 Known destination count  */
    uint32_t       unknownDestCount; /* +176 Unknown dest count       */
    uint32_t       invalidDestCount; /* +180 Invalid dest count       */
    uint32_t       objectRecOffset;  /* +184 Object record offset     */
    uint32_t       responseRecOffset; /* +188 Response record offset  */
    void          *objectRecPtr;     /* +192 Object record pointer    */
    void          *responseRecPtr;   /* +196 Response record pointer  */
    char           alternateSecurityId[40]; /* +200 Alt security ID   */
    char           resolvedQName[48]; /* +240 Resolved queue name     */
    char           resolvedQMgrName[48]; /* +288 Resolved QMgr name   */
};                                   /* Total: 336 bytes (v4)         */

#pragma pack()

/*===================================================================
 * Channel Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct mqcxp {
    char           strucId[4];       /* +0   Structure ID 'CXP '      */
    uint32_t       version;          /* +4   Structure version        */
    uint32_t       exitId;           /* +8   Exit ID                  */
    uint32_t       exitReason;       /* +12  Exit reason              */
    uint32_t       exitResponse;     /* +16  Exit response (output)   */
    uint32_t       exitResponse2;    /* +20  Secondary response       */
    uint32_t       feedback;         /* +24  Feedback code            */
    uint32_t       maxSegmentLength; /* +28  Max segment length       */
    char          *exitUserArea[16]; /* +32  User area                */
    char          *exitData;         /* +48  Exit data pointer        */
    uint32_t       exitDataLength;   /* +52  Exit data length         */
    char          *msgRetryUserData; /* +56  Retry user data          */
    uint32_t       msgRetryCount;    /* +60  Message retry count      */
    uint32_t       msgRetryInterval; /* +64  Retry interval           */
    uint32_t       msgRetryReason;   /* +68  Retry reason             */
    void          *headerLength;     /* +72  Header length pointer    */
    char           partnerName[48];  /* +76  Partner name             */
    uint32_t       fapLevel;         /* +124 FAP level                */
    uint32_t       capabilityFlags;  /* +128 Capability flags         */
    uint32_t       exitNumber;       /* +132 Exit number              */
    char          *exitSpace;        /* +136 Exit space pointer       */
};                                   /* Total: 140 bytes              */

#pragma pack()

/*===================================================================
 * Channel Definition Structure
 *===================================================================*/

#pragma pack(1)

struct mqcd {
    char           channelName[20];  /* +0   Channel name             */
    uint32_t       version;          /* +20  Structure version        */
    uint32_t       channelType;      /* +24  Channel type             */
    uint32_t       transportType;    /* +28  Transport type           */
    char           desc[64];         /* +32  Description              */
    char           qMgrName[48];     /* +96  Queue manager name       */
    char           xmitQName[48];    /* +144 Transmission queue       */
    char           shortConnectionName[20]; /* +192 Short conn name   */
    char           mCAName[20];      /* +212 MCA name                 */
    char           modeName[8];      /* +232 Mode name                */
    char           tpName[64];       /* +240 TP name                  */
    uint32_t       batchSize;        /* +304 Batch size               */
    uint32_t       discInterval;     /* +308 Disconnect interval      */
    uint32_t       shortRetryCount;  /* +312 Short retry count        */
    uint32_t       shortRetryInterval; /* +316 Short retry interval   */
    uint32_t       longRetryCount;   /* +320 Long retry count         */
    uint32_t       longRetryInterval; /* +324 Long retry interval     */
    char           securityExit[128]; /* +328 Security exit name      */
    char           msgExit[128];     /* +456 Message exit name        */
    char           sendExit[128];    /* +584 Send exit name           */
    char           receiveExit[128]; /* +712 Receive exit name        */
    uint32_t       seqNumberWrap;    /* +840 Sequence number wrap     */
    uint32_t       maxMsgLength;     /* +844 Maximum message length   */
    uint32_t       putAuthority;     /* +848 Put authority            */
    uint32_t       dataConversion;   /* +852 Data conversion          */
    char           securityUserData[32]; /* +856 Security user data   */
    char           msgUserData[32];  /* +888 Message user data        */
    char           sendUserData[32]; /* +920 Send user data           */
    char           receiveUserData[32]; /* +952 Receive user data     */
    char           userIdentifier[12]; /* +984 User identifier        */
    char           password[12];     /* +996 Password                 */
    char           mCAUserIdentifier[12]; /* +1008 MCA user ID        */
    uint32_t       mCAType;          /* +1020 MCA type                */
    char           connectionName[264]; /* +1024 Connection name      */
    char           remoteUserIdentifier[12]; /* +1288 Remote user ID  */
    char           remotePassword[12]; /* +1300 Remote password       */
    /* Additional fields vary by version */
};

#pragma pack()

/*===================================================================
 * API Exit Context
 *===================================================================*/

#pragma pack(1)

struct mqaxc {
    char           strucId[4];       /* +0   Structure ID 'AXC '      */
    uint32_t       version;          /* +4   Structure version        */
    uint32_t       environment;      /* +8   Environment              */
    char           userId[12];       /* +12  User ID                  */
    char           securityId[40];   /* +24  Security ID              */
    char           connectionName[48]; /* +64 Connection name         */
    uint32_t       langId;           /* +112 Language ID              */
    char           channelName[20];  /* +116 Channel name             */
    void          *qMgrHandle;       /* +136 Queue manager handle     */
    void          *exitUserArea[16]; /* +140 Exit user area           */
    uint32_t       function;         /* +156 Function code            */
    uint32_t       exitResponse;     /* +160 Exit response (output)   */
};                                   /* Total: 164 bytes              */

/* Environment values */
#define MQXE_OTHER             0     /* Other                         */
#define MQXE_MCA               1     /* Message channel agent         */
#define MQXE_MCA_SVRCONN       2     /* Server connection MCA         */
#define MQXE_COMMAND_SERVER    3     /* Command server                */
#define MQXE_MQSC              4     /* MQSC                          */

#pragma pack()

/*===================================================================
 * API Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct mqaxp {
    char           strucId[4];       /* +0   Structure ID 'AXP '      */
    uint32_t       version;          /* +4   Structure version        */
    uint32_t       exitId;           /* +8   Exit ID                  */
    uint32_t       exitReason;       /* +12  Exit reason              */
    uint32_t       exitResponse;     /* +16  Exit response (output)   */
    uint32_t       exitResponse2;    /* +20  Secondary response       */
    uint32_t       feedback;         /* +24  Feedback code            */
    uint32_t       apiCallersCompCode; /* +28 Caller's comp code      */
    uint32_t       apiCallersReason; /* +32  Caller's reason code     */
    uint32_t       exitCompCode;     /* +36  Exit comp code (output)  */
    uint32_t       exitReason2;      /* +40  Exit reason code (out)   */
    void          *exitUserArea[16]; /* +44  Exit user area           */
    struct mqmd   *mqmdPtr;          /* +60  MQMD pointer             */
    struct mqod   *mqodPtr;          /* +64  MQOD pointer             */
    void          *bufferPtr;        /* +68  Buffer pointer           */
    uint32_t       bufferLength;     /* +72  Buffer length            */
};                                   /* Total: 76 bytes               */

/* Exit reason codes */
#define MQXR_BEFORE            1     /* Before API call               */
#define MQXR_AFTER             2     /* After API call                */
#define MQXR_CONNECTION        3     /* Connection                    */

#pragma pack()

/*===================================================================
 * MQ Utility Functions
 *===================================================================*/

/**
 * mq_match_queue_name - Compare queue names
 * @q1: First queue name (48 chars)
 * @q2: Second queue name (48 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int mq_match_queue_name(const char q1[48], const char q2[48]) {
    return match_field(q1, q2, 48);
}

/**
 * mq_match_channel_name - Compare channel names
 * @c1: First channel name (20 chars)
 * @c2: Second channel name (20 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int mq_match_channel_name(const char c1[20], const char c2[20]) {
    return match_field(c1, c2, 20);
}

/**
 * mq_match_userid - Compare user IDs
 * @u1: First user ID (12 chars)
 * @u2: Second user ID (12 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int mq_match_userid(const char u1[12], const char u2[12]) {
    return match_field(u1, u2, 12);
}

/**
 * mq_is_persistent - Check if message is persistent
 * @md: Message descriptor
 * Returns: 1 if persistent, 0 otherwise
 */
static inline int mq_is_persistent(const struct mqmd *md) {
    return md->persistence == MQPER_PERSISTENT;
}

/**
 * mq_is_request - Check if message is a request
 * @md: Message descriptor
 * Returns: 1 if request, 0 otherwise
 */
static inline int mq_is_request(const struct mqmd *md) {
    return md->msgType == MQMT_REQUEST;
}

/**
 * mq_set_string - Set MQ string field (blank padded)
 * @dest: Destination field
 * @len: Field length
 * @src: Source string (null-terminated)
 */
static inline void mq_set_string(char *dest, size_t len, const char *src) {
    set_fixed_string(dest, src, len);
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Channel security exit
#pragma prolog(my_mqchlsec,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_mqchlsec,"RETURN(14,12)")

int my_mqchlsec(struct mqcxp *parms, struct mqcd *cd,
                void *dataBuffer, uint32_t dataLength) {
    if (parms->exitReason == MQXR_INIT_SEC) {
        // Initialize security context
        return MQXCC_OK;
    }

    if (parms->exitReason == MQXR_SEC_MSG) {
        // Validate security flow
        // Custom authentication logic here
    }

    parms->exitResponse = MQXCC_OK;
    return MQXCC_OK;
}

// Channel message exit - audit messages
#pragma prolog(my_mqchlmsg,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_mqchlmsg,"RETURN(14,12)")

int my_mqchlmsg(struct mqcxp *parms, struct mqcd *cd,
                void *dataBuffer, uint32_t *dataLength) {
    if (parms->exitReason == MQXR_MSG) {
        // Log message information
        // Could modify message here
        // audit_message(cd->channelName, *dataLength);
    }

    parms->exitResponse = MQXCC_OK;
    return MQXCC_OK;
}

// API exit - authorize operations
#pragma prolog(my_mqapiexit,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_mqapiexit,"RETURN(14,12)")

int my_mqapiexit(struct mqaxp *parms, struct mqaxc *context) {
    // Check authorization before MQPUT
    if (parms->exitReason == MQXR_BEFORE &&
        context->function == MQXF_PUT) {

        // Block puts to certain queues
        if (parms->mqodPtr != NULL) {
            if (memcmp_inline(parms->mqodPtr->objectName, "SYSTEM.", 7) == 0) {
                // Don't allow puts to SYSTEM.* queues
                parms->exitCompCode = MQCC_FAILED;
                parms->exitReason2 = MQRC_NOT_AUTHORIZED;
                parms->exitResponse = MQXCC_SUPPRESS_FUNCTION;
                return MQXCC_SUPPRESS_FUNCTION;
            }
        }
    }

    // Log after successful GETs
    if (parms->exitReason == MQXR_AFTER &&
        context->function == MQXF_GET &&
        parms->apiCallersCompCode == MQCC_OK) {
        // audit_get(context->userId, parms->mqodPtr->objectName);
    }

    parms->exitResponse = MQXCC_OK;
    return MQXCC_OK;
}

 *===================================================================*/

#endif /* METALC_MQ_H */
