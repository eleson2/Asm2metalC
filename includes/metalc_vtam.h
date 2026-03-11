/*********************************************************************
 * METALC_VTAM.H - VTAM/SNA Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for VTAM (Virtual Telecommunications Access
 * Method) exit development.
 *
 * Supported exits:
 *   Logon/Session exits:
 *     ISTEXCAA - Application exit
 *     ISTEXCCS - Session control exit
 *     ISTINCDT - Interpret table exit
 *     ISTEXCUV - USS verification exit
 *
 *   Network exits:
 *     ISTEXCVR - Virtual route exit
 *     ISTEXCSD - Session disconnect exit
 *     ISTEXCPM - Performance monitor exit
 *
 *   Authorization exits:
 *     ISTEXCLY - Logon verify exit
 *     ISTEXCPA - Password exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_vtam.h"
 *
 * IMPORTANT: VTAM exits run in cross-memory mode.
 *            Control block layouts vary by z/OS release.
 *********************************************************************/

#ifndef METALC_VTAM_H
#define METALC_VTAM_H

#include "metalc_base.h"

/*===================================================================
 * VTAM Exit Return Codes
 *===================================================================*/

/* General exit return codes */
#define VTAM_RC_CONTINUE       RC_OK     /* Continue processing           */
#define VTAM_RC_REJECT         RC_WARNING  /* Reject request                */
#define VTAM_RC_MODIFIED       RC_ERROR    /* Parameters modified           */
#define VTAM_RC_ERROR          RC_SEVERE   /* Error occurred                */

/* Logon exit (ISTEXCLY) return codes */
#define VTAM_LY_ACCEPT         RC_OK     /* Accept logon                  */
#define VTAM_LY_REJECT         RC_WARNING  /* Reject logon                  */
#define VTAM_LY_DEFER          RC_ERROR    /* Defer to RACF                 */
#define VTAM_LY_REDIRECT       RC_SEVERE   /* Redirect to another APPL      */

/* USS exit return codes */
#define VTAM_USS_CONTINUE      RC_OK     /* Continue with USS             */
#define VTAM_USS_REJECT        RC_WARNING  /* Reject USS command            */
#define VTAM_USS_MODIFIED      RC_ERROR    /* USS modified                  */

/* Session control return codes */
#define VTAM_SC_CONTINUE       RC_OK     /* Continue session              */
#define VTAM_SC_TERMINATE      RC_WARNING  /* Terminate session             */
#define VTAM_SC_HOLD           RC_ERROR    /* Hold session                  */

/*===================================================================
 * VTAM Sense Codes
 *===================================================================*/

#define VTAM_SENSE_OK          0x00000000  /* No error               */
#define VTAM_SENSE_PATH_ERR    0x08010000  /* Path error             */
#define VTAM_SENSE_PROT_ERR    0x08020000  /* Protocol error         */
#define VTAM_SENSE_LINK_ERR    0x08030000  /* Link failure           */
#define VTAM_SENSE_SESSION     0x08050000  /* Session limit          */
#define VTAM_SENSE_RESOURCE    0x08060000  /* Resource not available */
#define VTAM_SENSE_LU_INACT    0x08090000  /* LU not active          */
#define VTAM_SENSE_TIMEOUT     0x080A0000  /* Timeout                */
#define VTAM_SENSE_SECURITY    0x080F0000  /* Security violation     */
#define VTAM_SENSE_APPL_REJ    0x08140000  /* Application reject     */

/*===================================================================
 * VTAM Request Types
 *===================================================================*/

#define VTAM_REQ_LOGON         0x01  /* Logon request                 */
#define VTAM_REQ_LOGOFF        0x02  /* Logoff request                */
#define VTAM_REQ_BIND          0x03  /* BIND request                  */
#define VTAM_REQ_UNBIND        0x04  /* UNBIND request                */
#define VTAM_REQ_SDT           0x05  /* Start Data Traffic            */
#define VTAM_REQ_CLEAR         0x06  /* Clear request                 */
#define VTAM_REQ_STSN          0x07  /* Set and Test Sequence Numbers */
#define VTAM_REQ_RQR           0x08  /* Request Recovery              */
#define VTAM_REQ_SHUTD         0x09  /* Shutdown                      */
#define VTAM_REQ_BIS           0x0A  /* Bracket Initiation Stopped    */

/*===================================================================
 * VTAM Session Types
 *===================================================================*/

#define VTAM_SESS_LU0          0x00  /* LU type 0                     */
#define VTAM_SESS_LU1          0x01  /* LU type 1                     */
#define VTAM_SESS_LU2          0x02  /* LU type 2 (3270)              */
#define VTAM_SESS_LU3          0x03  /* LU type 3                     */
#define VTAM_SESS_LU4          0x04  /* LU type 4                     */
#define VTAM_SESS_LU62         0x06  /* LU type 6.2 (APPC)            */
#define VTAM_SESS_LU7          0x07  /* LU type 7                     */

/*===================================================================
 * ACB - Access Method Control Block
 *===================================================================*/

#pragma pack(1)

struct vtam_acb {
    char           acbid[4];         /* +0   'ACB ' identifier        */
    uint8_t        acblen;           /* +4   Length                   */
    uint8_t        acbtype;          /* +5   Type                     */
    uint16_t       acbflags;         /* +6   Flags                    */
    void          *acbappl;          /* +8   Application name ptr     */
    void          *acblogon;         /* +12  Logon exit address       */
    void          *acblosep;         /* +16  LOSTERM exit address     */
    void          *acbrelrq;         /* +20  RELREQ exit address      */
    void          *acbuserfd;        /* +24  User field               */
    uint32_t       acberror;         /* +28  Error code               */
    uint32_t       acbsession;       /* +32  Max sessions             */
    uint32_t       acbactive;        /* +36  Active sessions          */
};                                   /* Total: 40 bytes               */

/* ACBFLAGS bits */
#define ACB_FLG_OPEN     0x8000      /* ACB is open                   */
#define ACB_FLG_AUTH     0x4000      /* Authorized application        */
#define ACB_FLG_PASS     0x2000      /* Password required             */
#define ACB_FLG_COLD     0x1000      /* Cold start                    */

#pragma pack()

/*===================================================================
 * NIB - Node Initialization Block
 *===================================================================*/

#pragma pack(1)

struct vtam_nib {
    char           nibid[4];         /* +0   'NIB ' identifier        */
    uint8_t        niblen;           /* +4   Length                   */
    uint8_t        nibtype;          /* +5   Type                     */
    uint16_t       nibflags;         /* +6   Flags                    */
    char           nibsym[8];        /* +8   Symbolic name            */
    void          *nibmode;          /* +16  Mode name pointer        */
    void          *nibuser;          /* +20  User data pointer        */
    uint32_t       nibusrln;         /* +24  User data length         */
    void          *niblogon;         /* +28  Logon data pointer       */
    uint32_t       niblglen;         /* +32  Logon data length        */
};                                   /* Total: 36 bytes               */

/* NIBFLAGS bits */
#define NIB_FLG_PROC     0x8000      /* PROC option                   */
#define NIB_FLG_CONANY   0x4000      /* CONANY option                 */
#define NIB_FLG_BNDAREA  0x2000      /* BIND area provided            */

#pragma pack()

/*===================================================================
 * RPL - Request Parameter List
 *===================================================================*/

#pragma pack(1)

struct vtam_rpl {
    char           rplid[4];         /* +0   'RPL ' identifier        */
    uint8_t        rpllen;           /* +4   Length                   */
    uint8_t        rpltype;          /* +5   Type                     */
    uint16_t       rplflags;         /* +6   Flags                    */
    uint32_t       rplrtncd;         /* +8   Return code              */
    uint32_t       rplfdb2;          /* +12  Feedback code            */
    uint32_t       rplsense;         /* +16  Sense code               */
    void          *rplarea;          /* +20  Data area pointer        */
    uint32_t       rplareap;         /* +24  Area length              */
    uint32_t       rplrlen;          /* +28  Received length          */
    void          *rplacb;           /* +32  ACB pointer              */
    void          *rplarg;           /* +36  Argument pointer         */
    void          *rplnib;           /* +40  NIB pointer              */
    void          *rplusfld;         /* +44  User field               */
    uint8_t        rplreq;           /* +48  Request code             */
    uint8_t        rplcntrl;         /* +49  Control byte             */
    uint16_t       _reserved;        /* +50  Reserved                 */
};                                   /* Total: 52 bytes               */

/* RPLFLAGS bits */
#define RPL_FLG_SYNC     0x8000      /* Synchronous request           */
#define RPL_FLG_ASYNC    0x4000      /* Asynchronous request          */
#define RPL_FLG_NOTIFY   0x2000      /* Notify on completion          */
#define RPL_FLG_POST     0x1000      /* Post ECB on completion        */

/* RPL return codes */
#define RPL_RTNCD_OK     0x00        /* Successful                    */
#define RPL_RTNCD_ENV    0x04        /* Environmental error           */
#define RPL_RTNCD_LOG    0x08        /* Logical error                 */
#define RPL_RTNCD_EXCEP  0x0C        /* Exception response            */
#define RPL_RTNCD_LOST   0x10        /* Lost data                     */
#define RPL_RTNCD_SIG    0x14        /* Signal received               */

#pragma pack()

/*===================================================================
 * Logon Exit Parameter List (ISTEXCLY)
 *===================================================================*/

#pragma pack(1)

struct vtam_ly_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           lyluname[8];      /* +8   LU name                  */
    char           lyappl[8];        /* +16  Application name         */
    char           lymode[8];        /* +24  Mode name                */
    void          *lylogon;          /* +32  Logon data pointer       */
    uint32_t       lylglen;          /* +36  Logon data length        */
    char           lyuserid[8];      /* +40  User ID                  */
    char           lypass[8];        /* +48  Password (masked)        */
    char           lynewapp[8];      /* +56  New application (redir)  */
    uint32_t       lyreasn;          /* +64  Reason code (output)     */
    uint32_t       lysense;          /* +68  Sense code (output)      */
};                                   /* Total: 72 bytes               */

/* LYFUNC function codes */
#define LY_FUNC_LOGON    0x01        /* Logon request                 */
#define LY_FUNC_LOGOFF   0x02        /* Logoff notification           */
#define LY_FUNC_VERIFY   0x03        /* Verification only             */
#define LY_FUNC_INIT     EXIT_FUNC_INIT /* Initialization             */
#define LY_FUNC_TERM     EXIT_FUNC_TERM /* Termination                */

/* LYFLAGS bits */
#define LY_FLG_RACF      0x80        /* RACF available                */
#define LY_FLG_ENCRYPT   0x40        /* Password encrypted            */
#define LY_FLG_RELOGON   0x20        /* Re-logon attempt              */
#define LY_FLG_SIMLOGON  0x10        /* Simulated logon               */

#pragma pack()

/*===================================================================
 * Session Control Exit Parameter List (ISTEXCCS)
 *===================================================================*/

#pragma pack(1)

struct vtam_cs_parm {
    void          *cswork;           /* +0   Work area pointer        */
    uint8_t        csfunc;           /* +4   Function code            */
    uint8_t        csflags;          /* +5   Flags                    */
    uint8_t        cslutype;         /* +6   LU type                  */
    uint8_t        _reserved1;       /* +7   Reserved                 */
    char           csluname[8];      /* +8   LU name                  */
    char           csappl[8];        /* +16  Application name         */
    void          *cssessp;          /* +24  Session parameters ptr   */
    uint32_t       cssessln;         /* +28  Session parms length     */
    void          *csbindp;          /* +32  BIND image pointer       */
    uint32_t       csbindln;         /* +36  BIND image length        */
    uint32_t       csreasn;          /* +40  Reason code (output)     */
    uint32_t       cssense;          /* +44  Sense code (output)      */
};                                   /* Total: 48 bytes               */

/* CSFUNC function codes */
#define CS_FUNC_BIND     0x01        /* BIND processing               */
#define CS_FUNC_UNBIND   0x02        /* UNBIND processing             */
#define CS_FUNC_SDT      0x03        /* Start Data Traffic            */
#define CS_FUNC_CLEAR    0x04        /* Clear processing              */
#define CS_FUNC_SHUTD    0x05        /* Shutdown                      */

#pragma pack()

/*===================================================================
 * USS Exit Parameter List (ISTEXCUV)
 *===================================================================*/

#pragma pack(1)

struct vtam_uv_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           uvluname[8];      /* +8   LU name                  */
    char          *uvcmd;            /* +16  USS command pointer      */
    uint32_t       uvcmdlen;         /* +20  USS command length       */
    char          *uvmsg;            /* +24  USS message pointer      */
    uint32_t       uvmsglen;         /* +28  USS message length       */
    char          *uvnewcmd;         /* +32  New command (output)     */
    uint32_t       uvnewlen;         /* +36  New command length       */
    uint32_t       uvreasn;          /* +40  Reason code              */
};                                   /* Total: 44 bytes               */

/* UVFUNC function codes */
#define UV_FUNC_LOGON    0x01        /* USS LOGON command             */
#define UV_FUNC_LOGOFF   0x02        /* USS LOGOFF command            */
#define UV_FUNC_MSG      0x03        /* USS MSG command               */
#define UV_FUNC_IBMTEST  0x04        /* USS IBMTEST command           */
#define UV_FUNC_OTHER    0x05        /* Other USS command             */

#pragma pack()

/*===================================================================
 * Virtual Route Exit Parameter List (ISTEXCVR)
 *===================================================================*/

#pragma pack(1)

struct vtam_vr_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           vrsubnet[8];      /* +8   Subarea network          */
    uint32_t       vrorigin;         /* +16  Origin subarea           */
    uint32_t       vrdest;           /* +20  Destination subarea      */
    uint8_t        vrtpf;            /* +24  Transmission priority    */
    uint8_t        vrcos;            /* +25  Class of service         */
    uint16_t       _reserved2;       /* +26  Reserved                 */
    void          *vrroutep;         /* +28  Route table pointer      */
    uint32_t       vrroutel;         /* +32  Route table length       */
    uint32_t       vrreasn;          /* +36  Reason code              */
};                                   /* Total: 40 bytes               */

/* VRFUNC function codes */
#define VR_FUNC_SELECT   0x01        /* Route selection               */
#define VR_FUNC_ACTIVATE 0x02        /* Route activation              */
#define VR_FUNC_DEACT    0x03        /* Route deactivation            */
#define VR_FUNC_CHANGE   0x04        /* Route change                  */

#pragma pack()

/*===================================================================
 * VTAM Utility Functions
 *===================================================================*/

/**
 * vtam_is_lu2 - Check if LU type 2 (3270)
 * @lutype: LU type code
 * Returns: 1 if LU2, 0 otherwise
 */
static inline int vtam_is_lu2(uint8_t lutype) {
    return lutype == VTAM_SESS_LU2;
}

/**
 * vtam_is_appc - Check if LU 6.2 (APPC)
 * @lutype: LU type code
 * Returns: 1 if APPC, 0 otherwise
 */
static inline int vtam_is_appc(uint8_t lutype) {
    return lutype == VTAM_SESS_LU62;
}

/**
 * vtam_sense_ok - Check if sense code indicates success
 * @sense: Sense code
 * Returns: 1 if OK, 0 otherwise
 */
static inline int vtam_sense_ok(uint32_t sense) {
    return sense == VTAM_SENSE_OK;
}

/**
 * vtam_is_security_error - Check for security-related sense code
 * @sense: Sense code
 * Returns: 1 if security error, 0 otherwise
 */
static inline int vtam_is_security_error(uint32_t sense) {
    return (sense & 0xFFFF0000) == VTAM_SENSE_SECURITY;
}

/**
 * vtam_match_luname - Compare LU names
 * @lu1: First LU name (8 chars)
 * @lu2: Second LU name (8 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int vtam_match_luname(const char lu1[8], const char lu2[8]) {
    return match_field(lu1, lu2, 8);
}

/**
 * vtam_match_applname - Compare application names
 * @app1: First application name (8 chars)
 * @app2: Second application name (8 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int vtam_match_applname(const char app1[8], const char app2[8]) {
    return match_field(app1, app2, 8);
}

/**
 * vtam_rpl_ok - Check if RPL return code is OK
 * @rpl: Pointer to RPL
 * Returns: 1 if OK, 0 otherwise
 */
static inline int vtam_rpl_ok(const struct vtam_rpl *rpl) {
    return rpl->rplrtncd == RPL_RTNCD_OK;
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Logon exit - custom logon validation
#pragma prolog(my_istexcly,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_istexcly,"RETURN(14,12)")

int my_istexcly(struct vtam_ly_parm *parm) {
    if (parm->lyfunc == LY_FUNC_LOGON) {
        // Check for restricted applications
        if (vtam_match_applname(parm->lyappl, "ADMIN   ")) {
            // Custom authorization check
            // if (!is_admin(parm->lyuserid)) {
            //     parm->lysense = VTAM_SENSE_SECURITY;
            //     return VTAM_LY_REJECT;
            // }
        }

        // Log all logon attempts
        // log_logon(parm->lyluname, parm->lyappl, parm->lyuserid);
    }

    // Defer to RACF for standard authentication
    return VTAM_LY_DEFER;
}

// Session control exit
#pragma prolog(my_istexccs,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_istexccs,"RETURN(14,12)")

int my_istexccs(struct vtam_cs_parm *parm) {
    if (parm->csfunc == CS_FUNC_BIND) {
        // Log session establishment
        // log_session(parm->csluname, parm->csappl, parm->cslutype);

        // Check LU type restrictions
        if (!vtam_is_lu2(parm->cslutype) && !vtam_is_appc(parm->cslutype)) {
            // Only allow LU2 and APPC sessions
            parm->cssense = VTAM_SENSE_APPL_REJ;
            return VTAM_SC_TERMINATE;
        }
    }

    return VTAM_SC_CONTINUE;
}

// USS exit - command validation
#pragma prolog(my_istexcuv,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_istexcuv,"RETURN(14,12)")

int my_istexcuv(struct vtam_uv_parm *parm) {
    if (parm->uvfunc == UV_FUNC_LOGON) {
        // Could modify USS LOGON command
        // or redirect to different application
    }

    return VTAM_USS_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_VTAM_H */
