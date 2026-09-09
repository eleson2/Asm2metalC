# AI Translation Rules: z/OS TCP/IP Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with z/OS Communications Server (TCP/IP) exit-specific guidance.
It covers FTP server exits, Telnet exits, and general TCP/IP installation exits.

---

## 1. z/OS TCP/IP Exit Overview

z/OS Communications Server TCP/IP provides installation exits for:

| Exit Type | Module(s) | Purpose |
|-----------|-----------|---------|
| FTP Command Check | `FTCHKCMD` | Validate/reject FTP commands before execution |
| FTP SMF | `FTSMFEXT` | Supplement FTP SMF records |
| FTP Translate | `FTTRNMVS` | Translate dataset names |
| Telnet Exit | `TTELSEXT` | Telnet connection validation |
| DNS Exit | `EZADNSXT` | DNS query processing |
| IP Filter | `EZAFILT` | IP packet filtering |

Exit module names are configured in the `FTPxx`, `TELNETxx`, or `PROFILE.TCPIP`
configuration datasets.

---

## 2. Exit Architecture

### FTP Command Check Exit (FTCHKCMD)

The FTP command check exit is called for every client command.  It receives
a standard parameter block via R1.  The `func` field distinguishes
initialization, command, and termination calls.

```c
#pragma prolog(FTCHKCMD, "SAVE(14,12),LR(12,15)")
#pragma epilog(FTCHKCMD, "RETURN(14,12)")

int FTCHKCMD(struct ftp_chkcmd_parm *parm) {
    if (parm->func != FTP_FUNC_CMD) {
        return FTP_RC_CONTINUE;
    }
    ...
}
```

The parameter block uses `EXIT_PARM_HEADER` for the first 8 bytes.

---

## 3. TCP/IP Control Block Structures

### FTP_CHKCMD_PARM — FTP Command Check Parameter Block

```c
#pragma pack(1)
struct ftp_chkcmd_parm {
    EXIT_PARM_HEADER;             /* +0   work(+0), func(+4), flags(+5), reserved(+6) */
    /* parm->reserved (at +6) maps to ftpcmd (command code, 1-byte) */
    char      ftpuser[8];         /* +8   FTP user ID                    */
    char      ftpdsn[44];         /* +16  Dataset name operand           */
    uint16_t  ftpreply;           /* +60  FTP reply code (output)        */
    int32_t   ftpreasn;           /* +62  Reason code (output)           */
    struct {
        uint8_t addr[4];          /* +66  Client IPv4 address            */
    }         ftpclient;
    uint16_t  ftpport;            /* +70  Client port                    */
    uint8_t   ftppadding[2];      /* +72  Alignment                      */
};
#pragma pack()

/* func values (in EXIT_PARM_HEADER work area) */
#define FTP_FUNC_INIT    1    /* Exit initialization                     */
#define FTP_FUNC_CMD     2    /* Command check                           */
#define FTP_FUNC_TERM    3    /* Exit termination                        */

/* FTP command codes (parm->reserved / ftpcmd) */
#define FTP_CMD_USER     1    /* USER command                            */
#define FTP_CMD_PASS     2    /* PASS command                            */
#define FTP_CMD_RETR    11    /* RETR (download) command                 */
#define FTP_CMD_STOR    15    /* STOR (upload) command                   */
#define FTP_CMD_APPE    16    /* APPE (append) command                   */
#define FTP_CMD_DELE    23    /* DELE (delete) command                   */
#define FTP_CMD_SITE    29    /* SITE command                            */
#define FTP_CMD_MKD     32    /* MKD (make directory) command            */
#define FTP_CMD_RMD     33    /* RMD (remove directory) command          */

/* FTP flags (parm->flags) */
#define FTP_FLAG_SSL     0x80  /* SSL/TLS connection                     */
#define FTP_FLAG_ANON    0x40  /* Anonymous user                         */
#define FTP_FLAG_PASV    0x20  /* Passive mode                           */

/* FTP reply codes for ftpreply (output field) */
#define FTP_REPLY_OK         200  /* Command OK                          */
#define FTP_REPLY_NOACCESS   550  /* No such file or access denied       */
#define FTP_REPLY_CMDERR     500  /* Unrecognized/rejected command       */
#define FTP_REPLY_AUTHERR    530  /* Not logged in / authorization error */
```

---

## 4. Exit Return Codes

### FTP Command Check Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `FTP_RC_CONTINUE` | Allow the command to proceed |
| 4 | `FTP_RC_REJECT` | Reject the command; send `ftpreply` to client |

When returning `FTP_RC_REJECT`, always set `parm->ftpreply` to a valid FTP
reply code (e.g., 550 for access denied).  If `ftpreply` is zero, z/OS FTP
sends a generic "command failed" response.

---

## 5. Common TCP/IP Exit Patterns

### Pattern 1: FTP Command Blocking and Logging (FTCHKCMD)

**Assembler:**
```asm
FTCHKCMD CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)          Parameter block
         USING FTPCHKDS,R10
         CLI   FTPFUNC,2            Command check call?
         BNE   RETURN               No - continue
*        Check command code
         CLI   FTPCMD,23            DELE (delete)?
         BNE   CHKSTOR
*        Check dataset name for SYS1.
         CLC   FTPDSN(5),=CL5'SYS1.'
         BNE   RETURN
*        Block delete of SYS1.*
         MVC   FTPREPLY(2),=H'550'
         LA    R15,4
         BR    R14
CHKSTOR  CLI   FTPCMD,15            STOR (upload)?
         BNE   RETURN
*        Log upload
         ...
RETURN   SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_tcpip.h"

#pragma prolog(FTCHKCMD, "SAVE(14,12),LR(12,15)")
#pragma epilog(FTCHKCMD, "RETURN(14,12)")

int FTCHKCMD(struct ftp_chkcmd_parm *parm) {

    /* Only process command calls */
    if (parm->func != FTP_FUNC_CMD) {
        return FTP_RC_CONTINUE;
    }

    /* DELE (23) - block deletes of SYS1.* */
    if (parm->reserved == FTP_CMD_DELE) {
        if (match_prefix(parm->ftpdsn, "SYS1.", 5)) {
            parm->ftpreply = FTP_REPLY_NOACCESS;   /* 550 */
            parm->ftpreasn = 100;
            return FTP_RC_REJECT;
        }
    }

    /* STOR (15) - log uploads */
    else if (parm->reserved == FTP_CMD_STOR) {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "FTCHKCMD STOR USER=");
        msg_append_field(msg, &pos, parm->ftpuser, 8);
        msg_append_str(msg, &pos, " DSN=");
        for (int i = 0; i < 44 && parm->ftpdsn[i] != ' '; i++) {
            msg[pos++] = parm->ftpdsn[i];
        }
        wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /* SITE (29) - block from external networks */
    else if (parm->reserved == FTP_CMD_SITE) {
        if (parm->ftpclient.addr[0] != 10) {
            parm->ftpreply = FTP_REPLY_CMDERR;   /* 500 */
            parm->ftpreasn = 200;
            return FTP_RC_REJECT;
        }
    }

    return FTP_RC_CONTINUE;
}
```

### Pattern 2: Anonymous FTP Restriction

**Metal C:**
```c
int FTCHKCMD(struct ftp_chkcmd_parm *parm) {

    if (parm->func != FTP_FUNC_CMD) {
        return FTP_RC_CONTINUE;
    }

    /* Anonymous users may only RETR (download), not STOR or DELE */
    if (TM_ALL(parm->flags, FTP_FLAG_ANON)) {
        if (parm->reserved == FTP_CMD_STOR ||
            parm->reserved == FTP_CMD_APPE ||
            parm->reserved == FTP_CMD_DELE ||
            parm->reserved == FTP_CMD_MKD  ||
            parm->reserved == FTP_CMD_RMD) {
            parm->ftpreply = FTP_REPLY_AUTHERR;   /* 530 */
            parm->ftpreasn = 300;
            return FTP_RC_REJECT;
        }
    }

    return FTP_RC_CONTINUE;
}
```

---

## 6. TCP/IP-Specific Considerations

### 6.1 EXIT_PARM_HEADER Overlap

The FTP parameter block uses `EXIT_PARM_HEADER` for the first 8 bytes:
- `parm->work` (+0): not used in FTP
- `parm->func` (+4): function code (init/cmd/term) — **1 byte at +4**
- `parm->flags` (+5): FTP flags (SSL, anonymous, etc.) — **1 byte at +5**
- `parm->reserved` (+6): FTP command code — **1 byte at +6** (mapped to `ftpcmd` in ASM DSECT)

This overlap is non-obvious: the `reserved` field of the standard header is
**reused** as the command code.  Always treat `parm->reserved` as `ftpcmd`.

### 6.2 Dataset Name Format

`ftpdsn` is a 44-byte field containing the **MVS dataset name** (not the
FTP path).  It is space-padded and **not null-terminated**.  Use
`match_prefix` for HLQ checks; iterate to find the actual length.

### 6.3 IPv4 Address Format

`ftpclient.addr[4]` contains the client IPv4 address in **network byte order**
(big-endian).  `addr[0]` is the most-significant octet.  For internal network
checks (10.x.x.x), check `addr[0] == 10`.

### 6.4 FTPxx Configuration

The FTP command check exit is configured in the FTPxx dataset:

```
EXITCHECK FTCHKCMD
```

A restart of the FTP server (or `STOP`/`START FTP`) is required after changes.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(FTCHKCMD),DISP=SHR
//SYSLIN   DD DSN=your.obj(FTCHKCMD),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLMOD  DD DSN=your.tcpip.exits(FTCHKCMD),DISP=SHR
//SYSLIN   DD DSN=your.obj(FTCHKCMD),DISP=SHR
```

The exit library must be in the FTP server's STEPLIB.

---

## 8. Testing

1. **Test FTP server**: Use a dedicated FTP server instance.
2. **FTP trace**: `MODIFY FTP,TRACE,OPTION=EXIT` to trace exit calls.
3. **FTP debug log**: Examine the FTP server job log for exit return codes.
4. **Test each command code**: Test RETR, STOR, DELE, SITE explicitly.

---

## 9. Verification Checklist (TCP/IP-Specific)

- [ ] `parm->reserved` correctly interpreted as `ftpcmd` (command code)
- [ ] FTP reply code set before `FTP_RC_REJECT` return
- [ ] IPv4 address bytes in correct network byte order
- [ ] Dataset name comparisons use `match_prefix` (not null-terminated strings)
- [ ] Anonymous flag `FTP_FLAG_ANON` checked where appropriate
- [ ] Exit registered in FTPxx dataset with `EXITCHECK` statement
- [ ] FTP server restarted after exit update
- [ ] Exit is reentrant — no static writable data
- [ ] Tested for each command code the exit affects
