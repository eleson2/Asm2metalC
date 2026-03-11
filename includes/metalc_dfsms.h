/*********************************************************************
 * METALC_DFSMS.H - DFSMS Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for DFSMS (Data Facility Storage Management
 * Subsystem) exit development.
 *
 * Supported exits:
 *   ACS (Automatic Class Selection):
 *     IGDACSDC - Data class ACS routine
 *     IGDACSMC - Management class ACS routine
 *     IGDACSSC - Storage class ACS routine
 *     IGDACSSG - Storage group ACS routine
 *
 *   Catalog exits:
 *     IGGPRE00 - Catalog pre-processing
 *     IGGPOST0 - Catalog post-processing
 *     IGG0CLFA - Catalog locate exit
 *
 *   Allocation exits:
 *     IEFDB401 - Dynamic allocation exit
 *     IGGDAI00 - DADSM allocation exit
 *
 *   OAM exits:
 *     CBRUXSAE - Storage admin exit
 *     CBRUXENT - Entry processing exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_dfsms.h"
 *
 * IMPORTANT: ACS routines have specific restrictions.
 *            Verify syntax against ISMF ACS validation.
 *********************************************************************/

#ifndef METALC_DFSMS_H
#define METALC_DFSMS_H

#include "metalc_base.h"

/*===================================================================
 * DFSMS Exit Return Codes
 *===================================================================*/

/* ACS routine return codes */
#define ACS_RC_CONTINUE        RC_OK     /* Continue processing           */
#define ACS_RC_SELECTED        RC_WARNING  /* Class/group selected          */
#define ACS_RC_FAIL            RC_ERROR    /* ACS failure                   */

/* Catalog exit return codes */
#define CAT_RC_CONTINUE        RC_OK     /* Continue processing           */
#define CAT_RC_REJECT          RC_WARNING  /* Reject request                */
#define CAT_RC_MODIFIED        RC_ERROR    /* Parameters modified           */
#define CAT_RC_BYPASS          RC_SEVERE   /* Bypass catalog processing     */

/* Allocation exit return codes */
#define ALLOC_RC_CONTINUE      RC_OK     /* Continue allocation           */
#define ALLOC_RC_MODIFIED      RC_WARNING  /* Allocation modified           */
#define ALLOC_RC_REJECT        RC_ERROR    /* Reject allocation             */
#define ALLOC_RC_RETRY         RC_SEVERE   /* Retry with modifications      */

/* OAM exit return codes */
#define OAM_RC_CONTINUE        RC_OK     /* Continue processing           */
#define OAM_RC_REJECT          RC_WARNING  /* Reject request                */
#define OAM_RC_MODIFIED        RC_ERROR    /* Parameters modified           */

/*===================================================================
 * SMS Classes and Groups
 *===================================================================*/

/* Data class attributes */
#define DC_RECFM_F             0x80  /* Fixed                         */
#define DC_RECFM_V             0x40  /* Variable                      */
#define DC_RECFM_U             0x20  /* Undefined                     */
#define DC_RECFM_B             0x10  /* Blocked                       */
#define DC_RECFM_S             0x08  /* Spanned                       */
#define DC_RECFM_A             0x04  /* ASA control                   */
#define DC_RECFM_M             0x02  /* Machine control               */

/* DSORG - Data set organization */
#define DSORG_PS               0x40  /* Physical sequential           */
#define DSORG_DA               0x20  /* Direct access                 */
#define DSORG_PO               0x02  /* Partitioned                   */
#define DSORG_PDSE             0x82  /* PDSE (library)                */
#define DSORG_VSAM             0x08  /* VSAM                          */

/* Storage group types */
#define SG_TYPE_POOL           0x01  /* Pool storage group            */
#define SG_TYPE_VIO            0x02  /* VIO storage group             */
#define SG_TYPE_DUMMY          0x03  /* Dummy storage group           */
#define SG_TYPE_TAPE           0x04  /* Tape storage group            */
#define SG_TYPE_OBJECT         0x05  /* Object storage group          */

/*===================================================================
 * Catalog Request Types
 *===================================================================*/

#define CAT_REQ_DEFINE         0x01  /* Define entry                  */
#define CAT_REQ_DELETE         0x02  /* Delete entry                  */
#define CAT_REQ_ALTER          0x03  /* Alter entry                   */
#define CAT_REQ_LOCATE         0x04  /* Locate entry                  */
#define CAT_REQ_LISTCAT        0x05  /* List catalog                  */

/* Catalog entry types */
#define CAT_ENT_NONVSAM        0x01  /* Non-VSAM dataset              */
#define CAT_ENT_CLUSTER        0x02  /* VSAM cluster                  */
#define CAT_ENT_DATA           0x03  /* VSAM data component           */
#define CAT_ENT_INDEX          0x04  /* VSAM index component          */
#define CAT_ENT_ALIAS          0x05  /* Alias                         */
#define CAT_ENT_GDG            0x06  /* Generation data group         */
#define CAT_ENT_UCAT           0x07  /* User catalog                  */
#define CAT_ENT_PATH           0x08  /* Path                          */
#define CAT_ENT_AIX            0x09  /* Alternate index               */
#define CAT_ENT_PAGEDS         0x0A  /* Page space                    */

/*===================================================================
 * ACS Read-Only Variables
 *===================================================================*/

#pragma pack(1)

struct acs_read_vars {
    char           dsname[44];       /* +0   Dataset name             */
    char           dstype[8];        /* +44  Dataset type             */
    char           recfm[4];         /* +52  Record format            */
    uint32_t       lrecl;            /* +56  Logical record length    */
    uint32_t       blksize;          /* +60  Block size               */
    uint32_t       primary;          /* +64  Primary allocation       */
    uint32_t       secondary;        /* +68  Secondary allocation     */
    char           unit[8];          /* +72  Unit name                */
    char           volume[6];        /* +80  Volume serial            */
    char           _reserved1[2];    /* +86  Reserved                 */
    char           jobname[8];       /* +88  Job name                 */
    char           stepname[8];      /* +96  Step name                */
    char           ddname[8];        /* +104 DD name                  */
    char           userid[8];        /* +112 User ID                  */
    char           acctinfo[32];     /* +120 Account info             */
    char           pgmname[8];       /* +152 Program name             */
    uint8_t        dsorg;            /* +160 Dataset organization     */
    uint8_t        status;           /* +161 Dataset status           */
    uint8_t        ndisp;            /* +162 Normal disposition       */
    uint8_t        adisp;            /* +163 Abnormal disposition     */
    char           expdt[8];         /* +164 Expiration date          */
    char           dataclas[8];      /* +172 Requested data class     */
    char           storclas[8];      /* +180 Requested storage class  */
    char           mgmtclas[8];      /* +188 Requested mgmt class     */
    char           storgrp[8];       /* +196 Requested storage group  */
};                                   /* Total: 204 bytes              */

/* STATUS values */
#define ACS_STAT_OLD           0x01  /* OLD                           */
#define ACS_STAT_MOD           0x02  /* MOD                           */
#define ACS_STAT_NEW           0x03  /* NEW                           */
#define ACS_STAT_SHR           0x04  /* SHR                           */

/* NDISP/ADISP values */
#define ACS_DISP_DELETE        0x01  /* DELETE                        */
#define ACS_DISP_KEEP          0x02  /* KEEP                          */
#define ACS_DISP_CATLG         0x03  /* CATLG                         */
#define ACS_DISP_UNCATLG       0x04  /* UNCATLG                       */
#define ACS_DISP_PASS          0x05  /* PASS                          */

#pragma pack()

/*===================================================================
 * ACS Write Variables (Output)
 *===================================================================*/

#pragma pack(1)

struct acs_write_vars {
    char           dataclas[8];      /* +0   Selected data class      */
    char           storclas[8];      /* +8   Selected storage class   */
    char           mgmtclas[8];      /* +16  Selected mgmt class      */
    char           storgrp[8];       /* +24  Selected storage group   */
    uint32_t       flags;            /* +32  Processing flags         */
};                                   /* Total: 36 bytes               */

/* FLAGS bits */
#define ACS_OUT_DCSET      0x80000000  /* Data class set              */
#define ACS_OUT_SCSET      0x40000000  /* Storage class set           */
#define ACS_OUT_MCSET      0x20000000  /* Management class set        */
#define ACS_OUT_SGSET      0x10000000  /* Storage group set           */

#pragma pack()

/*===================================================================
 * ACS Routine Parameter List
 *===================================================================*/

#pragma pack(1)

struct acs_parm {
    void          *acswork;          /* +0   Work area pointer        */
    uint8_t        acsfunc;          /* +4   Function code            */
    uint8_t        acstype;          /* +5   ACS routine type         */
    uint16_t       acsflags;         /* +6   Flags                    */
    struct acs_read_vars *acsread;   /* +8   Read variables           */
    struct acs_write_vars *acswrite; /* +12  Write variables          */
    uint32_t       acsreasn;         /* +16  Reason code (output)     */
    char          *acsmsg;           /* +20  Message area             */
    uint16_t       acsmsgln;         /* +24  Message length           */
    uint16_t       _reserved;        /* +26  Reserved                 */
};                                   /* Total: 28 bytes               */

/* ACSFUNC function codes */
#define ACS_FUNC_INIT      EXIT_FUNC_INIT      /* Initialization                */
#define ACS_FUNC_SELECT    0x02      /* Class/group selection         */
#define ACS_FUNC_TERM      EXIT_FUNC_TERM      /* Termination                   */

/* ACSTYPE - ACS routine type */
#define ACS_TYPE_DC        0x01      /* Data class                    */
#define ACS_TYPE_SC        0x02      /* Storage class                 */
#define ACS_TYPE_MC        0x03      /* Management class              */
#define ACS_TYPE_SG        0x04      /* Storage group                 */

/* ACSFLAGS bits */
#define ACS_FLG_BYPASS     0x8000    /* Bypass ACS processing         */
#define ACS_FLG_DEFAULT    0x4000    /* Use default class             */
#define ACS_FLG_ALLOC      0x2000    /* Allocation request            */
#define ACS_FLG_RECALL     0x1000    /* Recall request                */

#pragma pack()

/*===================================================================
 * Catalog Exit Parameter List (IGGPRE00/IGGPOST0)
 *===================================================================*/

#pragma pack(1)

struct cat_exit_parm {
    void          *catwork;          /* +0   Work area pointer        */
    uint8_t        catfunc;          /* +4   Function code            */
    uint8_t        catreq;           /* +5   Request type             */
    uint8_t        catentry;         /* +6   Entry type               */
    uint8_t        catflags;         /* +7   Flags                    */
    char           catdsn[44];       /* +8   Dataset name             */
    char           catname[44];      /* +52  Catalog name             */
    char           catvol[6];        /* +96  Volume serial            */
    char           _reserved1[2];    /* +102 Reserved                 */
    char           catuser[8];       /* +104 User ID                  */
    char           catjob[8];        /* +112 Job name                 */
    char           catpgm[8];        /* +120 Program name             */
    uint32_t       catreasn;         /* +128 Reason code (output)     */
    void          *catparms;         /* +132 Catalog parms pointer    */
    uint32_t       catprmln;         /* +136 Catalog parms length     */
};                                   /* Total: 140 bytes              */

/* CATFUNC function codes */
#define CAT_FUNC_PRE       0x01      /* Pre-processing exit           */
#define CAT_FUNC_POST      0x02      /* Post-processing exit          */

/* CATFLAGS bits */
#define CAT_FLG_SMS        0x80      /* SMS-managed dataset           */
#define CAT_FLG_VSAM       0x40      /* VSAM dataset                  */
#define CAT_FLG_GDG        0x20      /* GDG dataset                   */
#define CAT_FLG_TAPE       0x10      /* Tape dataset                  */

#pragma pack()

/*===================================================================
 * Allocation Exit Parameter List (IEFDB401)
 *===================================================================*/

#pragma pack(1)

struct alloc_exit_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           alcdsn[44];       /* +8   Dataset name             */
    char           alcdd[8];         /* +52  DD name                  */
    char           alcvol[6];        /* +60  Volume serial            */
    char           _reserved2[2];    /* +66  Reserved                 */
    char           alcunit[8];       /* +68  Unit name                */
    uint32_t       alcpri;           /* +76  Primary allocation       */
    uint32_t       alcsec;           /* +80  Secondary allocation     */
    uint8_t        alcspace;         /* +84  Space type               */
    uint8_t        alcdsorg;         /* +85  Dataset organization     */
    uint8_t        alcrecfm;         /* +86  Record format            */
    uint8_t        _reserved3;       /* +87  Reserved                 */
    uint32_t       alclrecl;         /* +88  Logical record length    */
    uint32_t       alcblksz;         /* +92  Block size               */
    char           alcuser[8];       /* +96  User ID                  */
    char           alcjob[8];        /* +104 Job name                 */
    char           alcstep[8];       /* +112 Step name                */
    char           alcpgm[8];        /* +120 Program name             */
    char           alcdclas[8];      /* +128 Data class (I/O)         */
    char           alcsclas[8];      /* +136 Storage class (I/O)      */
    char           alcmclas[8];      /* +144 Management class (I/O)   */
    char           alcsgrp[8];       /* +152 Storage group (I/O)      */
    uint32_t       alcreasn;         /* +160 Reason code (output)     */
};                                   /* Total: 164 bytes              */

/* ALCFUNC function codes */
#define ALLOC_FUNC_ALLOC   0x01      /* Allocation request            */
#define ALLOC_FUNC_UNALLOC 0x02      /* Unallocation                  */
#define ALLOC_FUNC_CONCAT  0x03      /* Concatenation                 */

/* ALCSPACE - Space type */
#define ALLOC_SPACE_TRK    0x01      /* Tracks                        */
#define ALLOC_SPACE_CYL    0x02      /* Cylinders                     */
#define ALLOC_SPACE_BLK    0x03      /* Blocks                        */
#define ALLOC_SPACE_KB     0x04      /* Kilobytes                     */
#define ALLOC_SPACE_MB     0x05      /* Megabytes                     */
#define ALLOC_SPACE_REC    0x06      /* Records                       */

#pragma pack()

/*===================================================================
 * OAM Exit Parameter List (CBRUXSAE)
 *===================================================================*/

#pragma pack(1)

struct oam_exit_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           oamcoll[44];      /* +8   Collection name          */
    char           oamobj[44];       /* +52  Object name              */
    char           oamuser[8];       /* +96  User ID                  */
    char           oamjob[8];        /* +104 Job name                 */
    char           oamsclas[8];      /* +112 Storage class            */
    char           oammclas[8];      /* +120 Management class         */
    uint32_t       oamobjsz;         /* +128 Object size              */
    uint32_t       oamreasn;         /* +132 Reason code (output)     */
};                                   /* Total: 136 bytes              */

/* OAMFUNC function codes */
#define OAM_FUNC_STORE     0x01      /* Store object                  */
#define OAM_FUNC_RETRIEVE  0x02      /* Retrieve object               */
#define OAM_FUNC_DELETE    0x03      /* Delete object                 */
#define OAM_FUNC_QUERY     0x04      /* Query object                  */

#pragma pack()

/*===================================================================
 * DFSMS Utility Functions
 *===================================================================*/

/**
 * sms_match_dsn_prefix - Match dataset name prefix
 * @dsn: Dataset name (44 chars)
 * @prefix: Prefix to match
 * @len: Prefix length
 * Returns: 1 if matches, 0 otherwise
 */
static inline int sms_match_dsn_prefix(const char dsn[44],
                                        const char *prefix,
                                        size_t len) {
    return match_prefix(dsn, prefix, len);
}

/**
 * sms_match_dsn_hlq - Match high-level qualifier
 * @dsn: Dataset name (44 chars)
 * @hlq: HLQ to match
 * Returns: 1 if matches, 0 otherwise
 */
static inline int sms_match_dsn_hlq(const char dsn[44], const char *hlq) {
    size_t i = 0;
    while (hlq[i] && hlq[i] != ' ' && i < 8) {
        if (dsn[i] != hlq[i]) return 0;
        i++;
    }
    /* Must be followed by '.' or end of HLQ */
    return (dsn[i] == '.' || dsn[i] == ' ');
}

/**
 * sms_is_vsam - Check if dataset organization is VSAM
 * @dsorg: Dataset organization byte
 * Returns: 1 if VSAM, 0 otherwise
 */
static inline int sms_is_vsam(uint8_t dsorg) {
    return (dsorg & DSORG_VSAM) != 0;
}

/**
 * sms_is_pds - Check if dataset organization is PDS/PDSE
 * @dsorg: Dataset organization byte
 * Returns: 1 if PDS/PDSE, 0 otherwise
 */
static inline int sms_is_pds(uint8_t dsorg) {
    return (dsorg & DSORG_PO) != 0;
}

/**
 * sms_is_sequential - Check if dataset organization is sequential
 * @dsorg: Dataset organization byte
 * Returns: 1 if sequential, 0 otherwise
 */
static inline int sms_is_sequential(uint8_t dsorg) {
    return (dsorg & DSORG_PS) != 0;
}

/**
 * sms_set_class - Set class name in output area
 * @dest: Destination (8 bytes)
 * @class_name: Class name to set
 */
static inline void sms_set_class(char dest[8], const char *class_name) {
    set_fixed_string(dest, class_name, 8);
}

/*===================================================================
 * Example Usage
 *===================================================================*

// ACS routine - data class selection
#pragma prolog(my_igdacsdc,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_igdacsdc,"RETURN(14,12)")

int my_igdacsdc(struct acs_parm *parm) {
    if (parm->acsfunc != ACS_FUNC_SELECT) {
        return ACS_RC_CONTINUE;
    }

    struct acs_read_vars *read = parm->acsread;
    struct acs_write_vars *write = parm->acswrite;

    // Assign data class based on record length
    if (read->lrecl > 32760) {
        sms_set_class(write->dataclas, "LARGE");
    } else if (read->lrecl > 256) {
        sms_set_class(write->dataclas, "MEDIUM");
    } else {
        sms_set_class(write->dataclas, "SMALL");
    }

    write->flags |= ACS_OUT_DCSET;
    return ACS_RC_SELECTED;
}

// ACS routine - storage class selection
#pragma prolog(my_igdacssc,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_igdacssc,"RETURN(14,12)")

int my_igdacssc(struct acs_parm *parm) {
    if (parm->acsfunc != ACS_FUNC_SELECT) {
        return ACS_RC_CONTINUE;
    }

    struct acs_read_vars *read = parm->acsread;
    struct acs_write_vars *write = parm->acswrite;

    // Assign storage class based on HLQ
    if (sms_match_dsn_hlq(read->dsname, "PROD")) {
        sms_set_class(write->storclas, "FAST");
    } else if (sms_match_dsn_hlq(read->dsname, "TEST")) {
        sms_set_class(write->storclas, "STANDARD");
    } else {
        sms_set_class(write->storclas, "DEFAULT");
    }

    write->flags |= ACS_OUT_SCSET;
    return ACS_RC_SELECTED;
}

// Catalog pre-processing exit
#pragma prolog(my_iggpre00,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_iggpre00,"RETURN(14,12)")

int my_iggpre00(struct cat_exit_parm *parm) {
    // Log all define requests
    if (parm->catreq == CAT_REQ_DEFINE) {
        // log_catalog_define(parm->catdsn, parm->catuser);
    }

    // Block deletion of SYS1 datasets
    if (parm->catreq == CAT_REQ_DELETE) {
        if (sms_match_dsn_hlq(parm->catdsn, "SYS1")) {
            parm->catreasn = 100;
            return CAT_RC_REJECT;
        }
    }

    return CAT_RC_CONTINUE;
}

// Allocation exit - override SMS classes
#pragma prolog(my_iefdb401,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_iefdb401,"RETURN(14,12)")

int my_iefdb401(struct alloc_exit_parm *parm) {
    if (parm->alcfunc != ALLOC_FUNC_ALLOC) {
        return ALLOC_RC_CONTINUE;
    }

    // Override storage class for large allocations
    if (parm->alcpri > 1000 && parm->alcspace == ALLOC_SPACE_CYL) {
        sms_set_class(parm->alcsclas, "LARGE");
        return ALLOC_RC_MODIFIED;
    }

    return ALLOC_RC_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_DFSMS_H */
