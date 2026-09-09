/*********************************************************************
 * verify_structs.c - Compile-time struct layout verification
 *
 * GENERATED FILE - do not edit by hand.
 *   Regenerate:  python tools/gen_struct_asserts.py --write
 *   Check:       python tools/gen_struct_asserts.py --check
 *
 * Assertions are derived from the +N offset comments and the
 * Total-bytes markers in includes/metalc_*.h.  A compile
 * error here means a struct does not lay out the way its own header
 * documents - usually a wrong field type, a missing #pragma pack(1),
 * or an offset comment that was not updated with the field.
 *
 * This file has no executable body - a clean compile is the pass.
 *
 * On z/OS:
 *   xlc -qmetal -S -qlist -I../includes verify_structs.c
 * Off-platform (structure only, no assembler):
 *   tools/lint_host.sh
 *
 * NOTE: this proves the C matches the header. It does NOT prove the
 * header matches the real z/OS DSECT - only assembling the product
 * macro on the target system does that.
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_acf2.h"
#include "metalc_cics.h"
#include "metalc_db2.h"
#include "metalc_dfsms.h"
#include "metalc_ims.h"
#include "metalc_jes2.h"
#include "metalc_mq.h"
#include "metalc_netview.h"
#include "metalc_opc.h"
#include "metalc_racf.h"
#include "metalc_sa.h"
#include "metalc_saf.h"
#include "metalc_smf.h"
#include "metalc_svc.h"
#include "metalc_tcpip.h"
#include "metalc_vtam.h"
#include "metalc_verify.h"

/*===================================================================
 * metalc_acf2.h
 *===================================================================*/

/* struct acvald */
VERIFY_SIZE(acvald, 64);
VERIFY_OFFSET(acvald, acvalflg, 0);
VERIFY_OFFSET(acvald, acvalrc, 1);
VERIFY_OFFSET(acvald, acvalrsn, 2);
VERIFY_OFFSET(acvald, acvallid, 4);
VERIFY_OFFSET(acvald, acvalnam, 12);
VERIFY_OFFSET(acvald, _reserved1, 32);
VERIFY_OFFSET(acvald, acvalpwd, 34);
VERIFY_OFFSET(acvald, acvalnpw, 42);
VERIFY_OFFSET(acvald, acvalgrp, 50);
VERIFY_OFFSET(acvald, acvalfl2, 58);
VERIFY_OFFSET(acvald, _reserved2, 59);

/* struct acucb */
VERIFY_SIZE(acucb, 132);
VERIFY_OFFSET(acucb, acucblid, 0);
VERIFY_OFFSET(acucb, acucbnam, 8);
VERIFY_OFFSET(acucb, acucbflg, 28);
VERIFY_OFFSET(acucb, acucbprv, 32);
VERIFY_OFFSET(acucb, acucbpgm, 64);
VERIFY_OFFSET(acucb, acucbgrp, 72);
VERIFY_OFFSET(acucb, acucbpwdt, 80);
VERIFY_OFFSET(acucb, acucbpwit, 84);
VERIFY_OFFSET(acucb, acucbpwvc, 86);
VERIFY_OFFSET(acucb, acucbsrc, 87);
VERIFY_OFFSET(acucb, acucbdpt, 90);
VERIFY_OFFSET(acucb, acucbphn, 98);
VERIFY_OFFSET(acucb, acucbcrdt, 114);
VERIFY_OFFSET(acucb, acucbladt, 118);
VERIFY_OFFSET(acucb, acucblatm, 122);
VERIFY_OFFSET(acucb, _reserved, 126);

/* struct acresblk */
VERIFY_SIZE(acresblk, 80);
VERIFY_OFFSET(acresblk, acresflg, 0);
VERIFY_OFFSET(acresblk, acresrc, 1);
VERIFY_OFFSET(acresblk, acresrsn, 2);
VERIFY_OFFSET(acresblk, acresrnm, 4);
VERIFY_OFFSET(acresblk, acrestyp, 48);
VERIFY_OFFSET(acresblk, acreslid, 56);
VERIFY_OFFSET(acresblk, acresacc, 64);
VERIFY_OFFSET(acresblk, acresfl2, 65);
VERIFY_OFFSET(acresblk, _reserved, 66);
VERIFY_OFFSET(acresblk, acresucb, 68);
VERIFY_OFFSET(acresblk, acresvol, 72);
VERIFY_OFFSET(acresblk, _reserved2, 78);

/* struct acsrcblk */
VERIFY_SIZE(acsrcblk, 32);
VERIFY_OFFSET(acsrcblk, acsrctyp, 0);
VERIFY_OFFSET(acsrcblk, acsrcid, 8);
VERIFY_OFFSET(acsrcblk, acsrcflg, 16);
VERIFY_OFFSET(acsrcblk, _reserved, 17);
VERIFY_OFFSET(acsrcblk, acsrctrm, 20);
VERIFY_OFFSET(acsrcblk, acsrcadr, 28);

/* struct acfasvt */
VERIFY_OFFSET(acfasvt, acfaid, 0);
VERIFY_OFFSET(acfasvt, acfaver, 4);
VERIFY_OFFSET(acfasvt, acfarel, 6);
VERIFY_OFFSET(acfasvt, acfaucb, 8);
VERIFY_OFFSET(acfasvt, acfarule, 12);
VERIFY_OFFSET(acfasvt, acfaflg, 16);

/*===================================================================
 * metalc_base.h
 *===================================================================*/

/* struct cvt */
VERIFY_OFFSET(cvt, cvtfix, 0);
VERIFY_OFFSET(cvt, cvttcbp, 128);

/* struct ascb */
/* KNOWN ISSUE - assertions disabled for struct ascb.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_OFFSET(ascb, ascbid, 0); */
/* VERIFY_OFFSET(ascb, ascbfwdp, 4); */
/* VERIFY_OFFSET(ascb, ascbbwdp, 8); */
/* VERIFY_OFFSET(ascb, ascbasid, 36); */
/* VERIFY_OFFSET(ascb, ascbjbni, 172); */
/* VERIFY_OFFSET(ascb, ascbjbns, 180); */

/* struct tcb */
VERIFY_OFFSET(tcb, tcbrbp, 0);
VERIFY_OFFSET(tcb, tcbpie, 4);
VERIFY_OFFSET(tcb, tcbdeb, 8);
VERIFY_OFFSET(tcb, tcbtio, 12);
VERIFY_OFFSET(tcb, tcbcmp, 16);

/* struct tiot */
VERIFY_OFFSET(tiot, tiocnjob, 0);
VERIFY_OFFSET(tiot, tiocstep, 8);

/*===================================================================
 * metalc_cics.h
 *===================================================================*/

/* struct dfheiblk */
VERIFY_SIZE(dfheiblk, 86);
VERIFY_OFFSET(dfheiblk, eibtime, 0);
VERIFY_OFFSET(dfheiblk, eibdate, 4);
VERIFY_OFFSET(dfheiblk, eibtrnid, 8);
VERIFY_OFFSET(dfheiblk, eibtaskn, 12);
VERIFY_OFFSET(dfheiblk, eibtrmid, 16);
VERIFY_OFFSET(dfheiblk, eibcposn, 20);
VERIFY_OFFSET(dfheiblk, eibcalen, 22);
VERIFY_OFFSET(dfheiblk, eibaid, 24);
VERIFY_OFFSET(dfheiblk, eibfn, 25);
VERIFY_OFFSET(dfheiblk, eibrcode, 27);
VERIFY_OFFSET(dfheiblk, eibds, 33);
VERIFY_OFFSET(dfheiblk, eibreqid, 41);
VERIFY_OFFSET(dfheiblk, eibrsrce, 49);
VERIFY_OFFSET(dfheiblk, eibsync, 57);
VERIFY_OFFSET(dfheiblk, eibfree, 58);
VERIFY_OFFSET(dfheiblk, eibrecv, 59);
VERIFY_OFFSET(dfheiblk, eibsend, 60);
VERIFY_OFFSET(dfheiblk, eibatt, 61);
VERIFY_OFFSET(dfheiblk, eibeoc, 62);
VERIFY_OFFSET(dfheiblk, eibfmh, 63);
VERIFY_OFFSET(dfheiblk, eibcompl, 64);
VERIFY_OFFSET(dfheiblk, eibsig, 65);
VERIFY_OFFSET(dfheiblk, eibconf, 66);
VERIFY_OFFSET(dfheiblk, eiberr, 67);
VERIFY_OFFSET(dfheiblk, eiberrcd, 68);
VERIFY_OFFSET(dfheiblk, eibsynrb, 72);
VERIFY_OFFSET(dfheiblk, eibnodat, 73);
VERIFY_OFFSET(dfheiblk, eibresp, 74);
VERIFY_OFFSET(dfheiblk, eibresp2, 78);
VERIFY_OFFSET(dfheiblk, eibrldbk, 82);
VERIFY_OFFSET(dfheiblk, _reserved, 83);

/* struct dfhuepar */
VERIFY_SIZE(dfhuepar, 56);
VERIFY_OFFSET(dfhuepar, uepexn, 0);
VERIFY_OFFSET(dfhuepar, uepgaa, 4);
VERIFY_OFFSET(dfhuepar, ueptaa, 8);
VERIFY_OFFSET(dfhuepar, uephmsa, 12);
VERIFY_OFFSET(dfhuepar, uepactv, 16);
VERIFY_OFFSET(dfhuepar, ueprecur, 17);
VERIFY_OFFSET(dfhuepar, uepterm, 18);
VERIFY_OFFSET(dfhuepar, uepflags, 19);
VERIFY_OFFSET(dfhuepar, uepcrsa, 20);
VERIFY_OFFSET(dfhuepar, ueptca, 24);
VERIFY_OFFSET(dfhuepar, uepeib, 28);
VERIFY_OFFSET(dfhuepar, ueptran, 32);
VERIFY_OFFSET(dfhuepar, uepterm4, 36);
VERIFY_OFFSET(dfhuepar, uepuser, 40);
VERIFY_OFFSET(dfhuepar, uepwork, 48);
VERIFY_OFFSET(dfhuepar, uepwklen, 52);

/* struct dfhfcpar */
VERIFY_SIZE(dfhfcpar, 84);
VERIFY_OFFSET(dfhfcpar, base, 0);
VERIFY_OFFSET(dfhfcpar, fcpdsn, 56);
VERIFY_OFFSET(dfhfcpar, fcpreq, 64);
VERIFY_OFFSET(dfhfcpar, fcpopt, 65);
VERIFY_OFFSET(dfhfcpar, fcpresp, 66);
VERIFY_OFFSET(dfhfcpar, fcpresp2, 67);
VERIFY_OFFSET(dfhfcpar, fcpkey, 68);
VERIFY_OFFSET(dfhfcpar, fcpklen, 72);
VERIFY_OFFSET(dfhfcpar, fcprlen, 74);
VERIFY_OFFSET(dfhfcpar, fcprec, 76);
VERIFY_OFFSET(dfhfcpar, fcprrn, 80);

/* struct dfhpcpar */
VERIFY_SIZE(dfhpcpar, 88);
VERIFY_OFFSET(dfhpcpar, base, 0);
VERIFY_OFFSET(dfhpcpar, pcpprog, 56);
VERIFY_OFFSET(dfhpcpar, pcpreq, 64);
VERIFY_OFFSET(dfhpcpar, pcpresp, 65);
VERIFY_OFFSET(dfhpcpar, _reserved1, 66);
VERIFY_OFFSET(dfhpcpar, pcpcomm, 68);
VERIFY_OFFSET(dfhpcpar, pcpclen, 72);
VERIFY_OFFSET(dfhpcpar, pcptran, 76);
VERIFY_OFFSET(dfhpcpar, pcpuser, 80);

/* struct dfhtsepar */
VERIFY_SIZE(dfhtsepar, 80);
VERIFY_OFFSET(dfhtsepar, base, 0);
VERIFY_OFFSET(dfhtsepar, tsepque, 56);
VERIFY_OFFSET(dfhtsepar, tsepreq, 64);
VERIFY_OFFSET(dfhtsepar, tsepmain, 65);
VERIFY_OFFSET(dfhtsepar, tsepitem, 66);
VERIFY_OFFSET(dfhtsepar, tsepdata, 68);
VERIFY_OFFSET(dfhtsepar, tseplen, 72);
VERIFY_OFFSET(dfhtsepar, tsepresp, 76);
VERIFY_OFFSET(dfhtsepar, _reserved, 77);

/* struct dfhpeppar */
VERIFY_SIZE(dfhpeppar, 40);
VERIFY_OFFSET(dfhpeppar, pepeib, 0);
VERIFY_OFFSET(dfhpeppar, pepcomm, 4);
VERIFY_OFFSET(dfhpeppar, peptran, 8);
VERIFY_OFFSET(dfhpeppar, pepprog, 12);
VERIFY_OFFSET(dfhpeppar, pepabnd, 20);
VERIFY_OFFSET(dfhpeppar, peppsc, 24);
VERIFY_OFFSET(dfhpeppar, peppsw, 28);
VERIFY_OFFSET(dfhpeppar, pepregs, 32);
VERIFY_OFFSET(dfhpeppar, peptype, 36);
VERIFY_OFFSET(dfhpeppar, pepflags, 37);
VERIFY_OFFSET(dfhpeppar, _reserved, 38);

/* struct dfhseppar */
VERIFY_SIZE(dfhseppar, 52);
VERIFY_OFFSET(dfhseppar, sepeib, 0);
VERIFY_OFFSET(dfhseppar, sepuser, 4);
VERIFY_OFFSET(dfhseppar, seppass, 12);
VERIFY_OFFSET(dfhseppar, sepnpass, 20);
VERIFY_OFFSET(dfhseppar, sepgrp, 28);
VERIFY_OFFSET(dfhseppar, septerm, 36);
VERIFY_OFFSET(dfhseppar, sepflags, 40);
VERIFY_OFFSET(dfhseppar, sepresp, 41);
VERIFY_OFFSET(dfhseppar, _reserved, 42);
VERIFY_OFFSET(dfhseppar, sepmsg, 44);
VERIFY_OFFSET(dfhseppar, sepmsgln, 48);
VERIFY_OFFSET(dfhseppar, _reserved2, 50);

/*===================================================================
 * metalc_db2.h
 *===================================================================*/

/* struct db2_xac_parm */
/* KNOWN ISSUE - assertions disabled for struct db2_xac_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(db2_xac_parm, 116); */

/* struct db2_ath_parm */
/* KNOWN ISSUE - assertions disabled for struct db2_ath_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(db2_ath_parm, 72); */

/* struct db2_sgn_parm */
/* KNOWN ISSUE - assertions disabled for struct db2_sgn_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(db2_sgn_parm, 64); */

/* struct db2_edit_parm */
/* KNOWN ISSUE - assertions disabled for struct db2_edit_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(db2_edit_parm, 60); */

/* struct db2_field_parm */
/* KNOWN ISSUE - assertions disabled for struct db2_field_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(db2_field_parm, 68); */

/*===================================================================
 * metalc_dfsms.h
 *===================================================================*/

/* struct acs_read_vars */
VERIFY_SIZE(acs_read_vars, 204);
VERIFY_OFFSET(acs_read_vars, dsname, 0);
VERIFY_OFFSET(acs_read_vars, dstype, 44);
VERIFY_OFFSET(acs_read_vars, recfm, 52);
VERIFY_OFFSET(acs_read_vars, lrecl, 56);
VERIFY_OFFSET(acs_read_vars, blksize, 60);
VERIFY_OFFSET(acs_read_vars, primary, 64);
VERIFY_OFFSET(acs_read_vars, secondary, 68);
VERIFY_OFFSET(acs_read_vars, unit, 72);
VERIFY_OFFSET(acs_read_vars, volume, 80);
VERIFY_OFFSET(acs_read_vars, _reserved1, 86);
VERIFY_OFFSET(acs_read_vars, jobname, 88);
VERIFY_OFFSET(acs_read_vars, stepname, 96);
VERIFY_OFFSET(acs_read_vars, ddname, 104);
VERIFY_OFFSET(acs_read_vars, userid, 112);
VERIFY_OFFSET(acs_read_vars, acctinfo, 120);
VERIFY_OFFSET(acs_read_vars, pgmname, 152);
VERIFY_OFFSET(acs_read_vars, dsorg, 160);
VERIFY_OFFSET(acs_read_vars, status, 161);
VERIFY_OFFSET(acs_read_vars, ndisp, 162);
VERIFY_OFFSET(acs_read_vars, adisp, 163);
VERIFY_OFFSET(acs_read_vars, expdt, 164);
VERIFY_OFFSET(acs_read_vars, dataclas, 172);
VERIFY_OFFSET(acs_read_vars, storclas, 180);
VERIFY_OFFSET(acs_read_vars, mgmtclas, 188);
VERIFY_OFFSET(acs_read_vars, storgrp, 196);

/* struct acs_write_vars */
VERIFY_SIZE(acs_write_vars, 36);
VERIFY_OFFSET(acs_write_vars, dataclas, 0);
VERIFY_OFFSET(acs_write_vars, storclas, 8);
VERIFY_OFFSET(acs_write_vars, mgmtclas, 16);
VERIFY_OFFSET(acs_write_vars, storgrp, 24);
VERIFY_OFFSET(acs_write_vars, flags, 32);

/* struct acs_parm */
VERIFY_SIZE(acs_parm, 28);
VERIFY_OFFSET(acs_parm, acswork, 0);
VERIFY_OFFSET(acs_parm, acsfunc, 4);
VERIFY_OFFSET(acs_parm, acstype, 5);
VERIFY_OFFSET(acs_parm, acsflags, 6);
VERIFY_OFFSET(acs_parm, acsread, 8);
VERIFY_OFFSET(acs_parm, acswrite, 12);
VERIFY_OFFSET(acs_parm, acsreasn, 16);
VERIFY_OFFSET(acs_parm, acsmsg, 20);
VERIFY_OFFSET(acs_parm, acsmsgln, 24);
VERIFY_OFFSET(acs_parm, _reserved, 26);

/* struct cat_exit_parm */
VERIFY_SIZE(cat_exit_parm, 140);
VERIFY_OFFSET(cat_exit_parm, catwork, 0);
VERIFY_OFFSET(cat_exit_parm, catfunc, 4);
VERIFY_OFFSET(cat_exit_parm, catreq, 5);
VERIFY_OFFSET(cat_exit_parm, catentry, 6);
VERIFY_OFFSET(cat_exit_parm, catflags, 7);
VERIFY_OFFSET(cat_exit_parm, catdsn, 8);
VERIFY_OFFSET(cat_exit_parm, catname, 52);
VERIFY_OFFSET(cat_exit_parm, catvol, 96);
VERIFY_OFFSET(cat_exit_parm, _reserved1, 102);
VERIFY_OFFSET(cat_exit_parm, catuser, 104);
VERIFY_OFFSET(cat_exit_parm, catjob, 112);
VERIFY_OFFSET(cat_exit_parm, catpgm, 120);
VERIFY_OFFSET(cat_exit_parm, catreasn, 128);
VERIFY_OFFSET(cat_exit_parm, catparms, 132);
VERIFY_OFFSET(cat_exit_parm, catprmln, 136);

/* struct alloc_exit_parm */
VERIFY_SIZE(alloc_exit_parm, 164);

/* struct oam_exit_parm */
VERIFY_SIZE(oam_exit_parm, 136);

/*===================================================================
 * metalc_ims.h
 *===================================================================*/

/* struct iopcb */
VERIFY_SIZE(iopcb, 60);
VERIFY_OFFSET(iopcb, lterm, 0);
VERIFY_OFFSET(iopcb, _reserved1, 8);
VERIFY_OFFSET(iopcb, status, 10);
VERIFY_OFFSET(iopcb, msgdate, 12);
VERIFY_OFFSET(iopcb, msgtime, 16);
VERIFY_OFFSET(iopcb, msginlen, 20);
VERIFY_OFFSET(iopcb, msgseqno, 22);
VERIFY_OFFSET(iopcb, modname, 24);
VERIFY_OFFSET(iopcb, userid, 32);
VERIFY_OFFSET(iopcb, groupid, 40);
VERIFY_OFFSET(iopcb, _reserved2, 48);

/* struct dbpcb */
VERIFY_SIZE(dbpcb, 292);
VERIFY_OFFSET(dbpcb, dbdname, 0);
VERIFY_OFFSET(dbpcb, seglevel, 8);
VERIFY_OFFSET(dbpcb, status, 10);
VERIFY_OFFSET(dbpcb, procopt, 12);
VERIFY_OFFSET(dbpcb, reserved, 16);
VERIFY_OFFSET(dbpcb, segname, 20);
VERIFY_OFFSET(dbpcb, keyfdbk, 28);
VERIFY_OFFSET(dbpcb, sensegcnt, 32);
VERIFY_OFFSET(dbpcb, keyarea, 36);

/* struct imsscd */
VERIFY_SIZE(imsscd, 64);
VERIFY_OFFSET(imsscd, scdtran, 0);
VERIFY_OFFSET(imsscd, scdpsb, 8);
VERIFY_OFFSET(imsscd, scdlterm, 16);
VERIFY_OFFSET(imsscd, scduser, 24);
VERIFY_OFFSET(imsscd, scdgroup, 32);
VERIFY_OFFSET(imsscd, scdflags, 40);
VERIFY_OFFSET(imsscd, scdtype, 41);
VERIFY_OFFSET(imsscd, scdprio, 42);
VERIFY_OFFSET(imsscd, scddate, 44);
VERIFY_OFFSET(imsscd, scdtime, 48);
VERIFY_OFFSET(imsscd, scdcpuid, 52);
VERIFY_OFFSET(imsscd, scdiopcb, 56);
VERIFY_OFFSET(imsscd, scdpsba, 60);

/* struct ims_sgnx_parm */
VERIFY_SIZE(ims_sgnx_parm, 68);

/* struct ims_txit_parm */
VERIFY_SIZE(ims_txit_parm, 60);

/* struct ims_flgx_parm */
/* KNOWN ISSUE - assertions disabled for struct ims_flgx_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(ims_flgx_parm, 44); */

/* struct ims_me_parm */
VERIFY_SIZE(ims_me_parm, 40);

/* struct ims_bsex_parm */
VERIFY_SIZE(ims_bsex_parm, 40);

/* struct mscd */
VERIFY_OFFSET(mscd, mscdname, 0);
VERIFY_OFFSET(mscd, mscdflg1, 8);

/* struct mscp */
VERIFY_OFFSET(mscp, mscpfunc, 0);
VERIFY_OFFSET(mscp, mscpflg1, 4);
VERIFY_OFFSET(mscp, _reserved, 5);
VERIFY_OFFSET(mscp, mscpdest, 8);
VERIFY_OFFSET(mscp, mscpmsga, 12);

/*===================================================================
 * metalc_jes2.h
 *===================================================================*/

/* struct jct */
VERIFY_OFFSET(jct, jctid, 0);
VERIFY_OFFSET(jct, jctjobid, 4);
VERIFY_OFFSET(jct, jctjname, 6);
VERIFY_OFFSET(jct, jctjclas, 14);
VERIFY_OFFSET(jct, jctprio, 15);
VERIFY_OFFSET(jct, jctmclas, 16);
VERIFY_OFFSET(jct, jctroute, 17);
VERIFY_OFFSET(jct, _filler1, 25);
VERIFY_OFFSET(jct, jctflg1, 27);
VERIFY_OFFSET(jct, jctflg2, 28);
VERIFY_OFFSET(jct, jctflg3, 29);
VERIFY_OFFSET(jct, jctflg4, 30);
VERIFY_OFFSET(jct, _filler2, 31);
VERIFY_OFFSET(jct, jctpname, 32);
VERIFY_OFFSET(jct, jctacct, 52);
VERIFY_OFFSET(jct, jcttsuid, 84);
VERIFY_OFFSET(jct, jctgroup, 92);
VERIFY_OFFSET(jct, jctnnode, 100);
VERIFY_OFFSET(jct, jctnuser, 108);
VERIFY_OFFSET(jct, jctsubsm, 116);
VERIFY_OFFSET(jct, jctsubdt, 120);
VERIFY_OFFSET(jct, jctstrte, 124);
VERIFY_OFFSET(jct, jctstrtd, 128);
VERIFY_OFFSET(jct, jctendtm, 132);
VERIFY_OFFSET(jct, jctenddt, 136);
VERIFY_OFFSET(jct, jctnstep, 140);
VERIFY_OFFSET(jct, jctestep, 142);
VERIFY_OFFSET(jct, jctmxrc, 144);
VERIFY_OFFSET(jct, jctabcod, 148);
VERIFY_OFFSET(jct, jctlines, 152);
VERIFY_OFFSET(jct, jctpages, 156);
VERIFY_OFFSET(jct, jctcards, 160);

/* struct jqe */
VERIFY_SIZE(jqe, 44);
VERIFY_OFFSET(jqe, jqeid, 0);
VERIFY_OFFSET(jqe, jqejobid, 4);
VERIFY_OFFSET(jqe, jqetype, 6);
VERIFY_OFFSET(jqe, jqeprio, 7);
VERIFY_OFFSET(jqe, jqeflg1, 8);
VERIFY_OFFSET(jqe, jqeflg2, 9);
VERIFY_OFFSET(jqe, jqeflg3, 10);
VERIFY_OFFSET(jqe, jqeflg4, 11);
VERIFY_OFFSET(jqe, jqejct, 12);
VERIFY_OFFSET(jqe, jqenext, 16);
VERIFY_OFFSET(jqe, jqeprev, 20);
VERIFY_OFFSET(jqe, jqeqtime, 24);
VERIFY_OFFSET(jqe, jqeqdate, 28);
VERIFY_OFFSET(jqe, jqejname, 32);
VERIFY_OFFSET(jqe, jqejclas, 40);
VERIFY_OFFSET(jqe, _reserved, 41);

/* struct pce */
VERIFY_OFFSET(pce, pceid, 0);
VERIFY_OFFSET(pce, pcetype, 4);
VERIFY_OFFSET(pce, pceflags, 5);
VERIFY_OFFSET(pce, _reserved, 6);
VERIFY_OFFSET(pce, pcejct, 8);
VERIFY_OFFSET(pce, pcejqe, 12);
VERIFY_OFFSET(pce, pceanchr, 16);
VERIFY_OFFSET(pce, pcework, 20);

/* struct jes2_xpl */
VERIFY_OFFSET(jes2_xpl, xplxrt, 0);
VERIFY_OFFSET(jes2_xpl, xplexitp, 4);
VERIFY_OFFSET(jes2_xpl, xplsubpt, 8);
VERIFY_OFFSET(jes2_xpl, xplwork, 12);
VERIFY_OFFSET(jes2_xpl, xplwsize, 16);
VERIFY_OFFSET(jes2_xpl, xpljct, 20);
VERIFY_OFFSET(jes2_xpl, xpljqe, 24);
VERIFY_OFFSET(jes2_xpl, xplpce, 28);
VERIFY_OFFSET(jes2_xpl, xplflags, 32);
VERIFY_OFFSET(jes2_xpl, xplrc, 36);

/* struct jes2_exit1_parm */
VERIFY_OFFSET(jes2_exit1_parm, e1jct, 40);
VERIFY_OFFSET(jes2_exit1_parm, e1jqe, 44);
VERIFY_OFFSET(jes2_exit1_parm, e1flags, 48);
VERIFY_OFFSET(jes2_exit1_parm, _reserved, 49);

/* struct jes2_exit4_parm */
VERIFY_OFFSET(jes2_exit4_parm, e4jct, 40);
VERIFY_OFFSET(jes2_exit4_parm, e4jqe, 44);
VERIFY_OFFSET(jes2_exit4_parm, e4jds, 48);
VERIFY_OFFSET(jes2_exit4_parm, e4dest, 52);
VERIFY_OFFSET(jes2_exit4_parm, e4destl, 56);
VERIFY_OFFSET(jes2_exit4_parm, e4flags, 58);
VERIFY_OFFSET(jes2_exit4_parm, _reserved, 59);

/* struct jes2_exit8_parm */
VERIFY_OFFSET(jes2_exit8_parm, e8jct, 40);
VERIFY_OFFSET(jes2_exit8_parm, e8stmt, 44);
VERIFY_OFFSET(jes2_exit8_parm, e8stmlen, 48);
VERIFY_OFFSET(jes2_exit8_parm, e8flags, 50);
VERIFY_OFFSET(jes2_exit8_parm, e8card, 51);
VERIFY_OFFSET(jes2_exit8_parm, e8errmsg, 52);
VERIFY_OFFSET(jes2_exit8_parm, e8errlen, 56);
VERIFY_OFFSET(jes2_exit8_parm, _reserved, 58);

/* struct jes2_exit20_parm */
VERIFY_OFFSET(jes2_exit20_parm, e20jct, 40);
VERIFY_OFFSET(jes2_exit20_parm, e20cpu, 44);
VERIFY_OFFSET(jes2_exit20_parm, e20lines, 48);
VERIFY_OFFSET(jes2_exit20_parm, e20pages, 52);
VERIFY_OFFSET(jes2_exit20_parm, e20cards, 56);
VERIFY_OFFSET(jes2_exit20_parm, e20maxrc, 60);

/*===================================================================
 * metalc_mq.h
 *===================================================================*/

/* struct mqmd */
VERIFY_SIZE(mqmd, 364);
VERIFY_OFFSET(mqmd, strucId, 0);
VERIFY_OFFSET(mqmd, version, 4);
VERIFY_OFFSET(mqmd, report, 8);
VERIFY_OFFSET(mqmd, msgType, 12);
VERIFY_OFFSET(mqmd, expiry, 16);
VERIFY_OFFSET(mqmd, feedback, 20);
VERIFY_OFFSET(mqmd, encoding, 24);
VERIFY_OFFSET(mqmd, codedCharSetId, 28);
VERIFY_OFFSET(mqmd, format, 32);
VERIFY_OFFSET(mqmd, priority, 40);
VERIFY_OFFSET(mqmd, persistence, 44);
VERIFY_OFFSET(mqmd, msgId, 48);
VERIFY_OFFSET(mqmd, correlId, 72);
VERIFY_OFFSET(mqmd, backoutCount, 96);
VERIFY_OFFSET(mqmd, replyToQ, 100);
VERIFY_OFFSET(mqmd, replyToQMgr, 148);
VERIFY_OFFSET(mqmd, userId, 196);
VERIFY_OFFSET(mqmd, accountingToken, 208);
VERIFY_OFFSET(mqmd, applIdentityData, 240);
VERIFY_OFFSET(mqmd, putApplType, 272);
VERIFY_OFFSET(mqmd, putApplName, 276);
VERIFY_OFFSET(mqmd, putDate, 304);
VERIFY_OFFSET(mqmd, putTime, 312);
VERIFY_OFFSET(mqmd, applOriginData, 320);
VERIFY_OFFSET(mqmd, groupId, 324);
VERIFY_OFFSET(mqmd, msgSeqNumber, 348);
VERIFY_OFFSET(mqmd, offset, 352);
VERIFY_OFFSET(mqmd, msgFlags, 356);
VERIFY_OFFSET(mqmd, originalLength, 360);

/* struct mqod */
VERIFY_SIZE(mqod, 336);
VERIFY_OFFSET(mqod, strucId, 0);
VERIFY_OFFSET(mqod, version, 4);
VERIFY_OFFSET(mqod, objectType, 8);
VERIFY_OFFSET(mqod, objectName, 12);
VERIFY_OFFSET(mqod, objectQMgrName, 60);
VERIFY_OFFSET(mqod, dynamicQName, 108);
VERIFY_OFFSET(mqod, alternateUserId, 156);
VERIFY_OFFSET(mqod, recsPresent, 168);
VERIFY_OFFSET(mqod, knownDestCount, 172);
VERIFY_OFFSET(mqod, unknownDestCount, 176);
VERIFY_OFFSET(mqod, invalidDestCount, 180);
VERIFY_OFFSET(mqod, objectRecOffset, 184);
VERIFY_OFFSET(mqod, responseRecOffset, 188);
VERIFY_OFFSET(mqod, objectRecPtr, 192);
VERIFY_OFFSET(mqod, responseRecPtr, 196);
VERIFY_OFFSET(mqod, alternateSecurityId, 200);
VERIFY_OFFSET(mqod, resolvedQName, 240);
VERIFY_OFFSET(mqod, resolvedQMgrName, 288);

/* struct mqcxp */
VERIFY_SIZE(mqcxp, 140);
VERIFY_OFFSET(mqcxp, strucId, 0);
VERIFY_OFFSET(mqcxp, version, 4);
VERIFY_OFFSET(mqcxp, exitId, 8);
VERIFY_OFFSET(mqcxp, exitReason, 12);
VERIFY_OFFSET(mqcxp, exitResponse, 16);
VERIFY_OFFSET(mqcxp, exitResponse2, 20);
VERIFY_OFFSET(mqcxp, feedback, 24);
VERIFY_OFFSET(mqcxp, maxSegmentLength, 28);
VERIFY_OFFSET(mqcxp, exitUserArea, 32);
VERIFY_OFFSET(mqcxp, exitData, 48);
VERIFY_OFFSET(mqcxp, exitDataLength, 52);
VERIFY_OFFSET(mqcxp, msgRetryUserData, 56);
VERIFY_OFFSET(mqcxp, msgRetryCount, 60);
VERIFY_OFFSET(mqcxp, msgRetryInterval, 64);
VERIFY_OFFSET(mqcxp, msgRetryReason, 68);
VERIFY_OFFSET(mqcxp, headerLength, 72);
VERIFY_OFFSET(mqcxp, partnerName, 76);
VERIFY_OFFSET(mqcxp, fapLevel, 124);
VERIFY_OFFSET(mqcxp, capabilityFlags, 128);
VERIFY_OFFSET(mqcxp, exitNumber, 132);
VERIFY_OFFSET(mqcxp, exitSpace, 136);

/* struct mqcd */
VERIFY_OFFSET(mqcd, channelName, 0);
VERIFY_OFFSET(mqcd, version, 20);
VERIFY_OFFSET(mqcd, channelType, 24);
VERIFY_OFFSET(mqcd, transportType, 28);
VERIFY_OFFSET(mqcd, desc, 32);
VERIFY_OFFSET(mqcd, qMgrName, 96);
VERIFY_OFFSET(mqcd, xmitQName, 144);
VERIFY_OFFSET(mqcd, shortConnectionName, 192);
VERIFY_OFFSET(mqcd, mCAName, 212);
VERIFY_OFFSET(mqcd, modeName, 232);
VERIFY_OFFSET(mqcd, tpName, 240);
VERIFY_OFFSET(mqcd, batchSize, 304);
VERIFY_OFFSET(mqcd, discInterval, 308);
VERIFY_OFFSET(mqcd, shortRetryCount, 312);
VERIFY_OFFSET(mqcd, shortRetryInterval, 316);
VERIFY_OFFSET(mqcd, longRetryCount, 320);
VERIFY_OFFSET(mqcd, longRetryInterval, 324);
VERIFY_OFFSET(mqcd, securityExit, 328);
VERIFY_OFFSET(mqcd, msgExit, 456);
VERIFY_OFFSET(mqcd, sendExit, 584);
VERIFY_OFFSET(mqcd, receiveExit, 712);
VERIFY_OFFSET(mqcd, seqNumberWrap, 840);
VERIFY_OFFSET(mqcd, maxMsgLength, 844);
VERIFY_OFFSET(mqcd, putAuthority, 848);
VERIFY_OFFSET(mqcd, dataConversion, 852);
VERIFY_OFFSET(mqcd, securityUserData, 856);
VERIFY_OFFSET(mqcd, msgUserData, 888);
VERIFY_OFFSET(mqcd, sendUserData, 920);
VERIFY_OFFSET(mqcd, receiveUserData, 952);
VERIFY_OFFSET(mqcd, userIdentifier, 984);
VERIFY_OFFSET(mqcd, password, 996);
VERIFY_OFFSET(mqcd, mCAUserIdentifier, 1008);
VERIFY_OFFSET(mqcd, mCAType, 1020);
VERIFY_OFFSET(mqcd, connectionName, 1024);
VERIFY_OFFSET(mqcd, remoteUserIdentifier, 1288);
VERIFY_OFFSET(mqcd, remotePassword, 1300);

/* struct mqaxc */
VERIFY_SIZE(mqaxc, 164);
VERIFY_OFFSET(mqaxc, strucId, 0);
VERIFY_OFFSET(mqaxc, version, 4);
VERIFY_OFFSET(mqaxc, environment, 8);
VERIFY_OFFSET(mqaxc, userId, 12);
VERIFY_OFFSET(mqaxc, securityId, 24);
VERIFY_OFFSET(mqaxc, connectionName, 64);
VERIFY_OFFSET(mqaxc, langId, 112);
VERIFY_OFFSET(mqaxc, channelName, 116);
VERIFY_OFFSET(mqaxc, qMgrHandle, 136);
VERIFY_OFFSET(mqaxc, exitUserArea, 140);
VERIFY_OFFSET(mqaxc, function, 156);
VERIFY_OFFSET(mqaxc, exitResponse, 160);

/* struct mqaxp */
VERIFY_SIZE(mqaxp, 76);
VERIFY_OFFSET(mqaxp, strucId, 0);
VERIFY_OFFSET(mqaxp, version, 4);
VERIFY_OFFSET(mqaxp, exitId, 8);
VERIFY_OFFSET(mqaxp, exitReason, 12);
VERIFY_OFFSET(mqaxp, exitResponse, 16);
VERIFY_OFFSET(mqaxp, exitResponse2, 20);
VERIFY_OFFSET(mqaxp, feedback, 24);
VERIFY_OFFSET(mqaxp, apiCallersCompCode, 28);
VERIFY_OFFSET(mqaxp, apiCallersReason, 32);
VERIFY_OFFSET(mqaxp, exitCompCode, 36);
VERIFY_OFFSET(mqaxp, exitReason2, 40);
VERIFY_OFFSET(mqaxp, exitUserArea, 44);
VERIFY_OFFSET(mqaxp, mqmdPtr, 60);
VERIFY_OFFSET(mqaxp, mqodPtr, 64);
VERIFY_OFFSET(mqaxp, bufferPtr, 68);
VERIFY_OFFSET(mqaxp, bufferLength, 72);

/*===================================================================
 * metalc_netview.h
 *===================================================================*/

/* struct nv_cmd_parm */
VERIFY_SIZE(nv_cmd_parm, 60);

/* struct nv_msg_parm */
VERIFY_SIZE(nv_msg_parm, 84);

/* struct nv_auto_parm */
VERIFY_SIZE(nv_auto_parm, 64);

/* struct nv_logon_parm */
VERIFY_SIZE(nv_logon_parm, 84);

/* struct nv_alert_parm */
VERIFY_SIZE(nv_alert_parm, 104);

/* struct nv_session_parm */
VERIFY_SIZE(nv_session_parm, 60);

/*===================================================================
 * metalc_opc.h
 *===================================================================*/

/* struct opc_adid */
VERIFY_SIZE(opc_adid, 32);
VERIFY_OFFSET(opc_adid, ad_name, 0);
VERIFY_OFFSET(opc_adid, ad_group, 16);
VERIFY_OFFSET(opc_adid, ad_ia, 24);
VERIFY_OFFSET(opc_adid, ad_iatime, 28);
VERIFY_OFFSET(opc_adid, ad_oper, 30);

/* struct opc_oper */
VERIFY_SIZE(opc_oper, 156);
VERIFY_OFFSET(opc_oper, op_adname, 0);
VERIFY_OFFSET(opc_oper, op_group, 16);
VERIFY_OFFSET(opc_oper, op_ia, 24);
VERIFY_OFFSET(opc_oper, op_opnum, 28);
VERIFY_OFFSET(opc_oper, op_status, 30);
VERIFY_OFFSET(opc_oper, op_extstatus, 31);
VERIFY_OFFSET(opc_oper, op_jobname, 32);
VERIFY_OFFSET(opc_oper, op_wsname, 40);
VERIFY_OFFSET(opc_oper, op_estdur, 44);
VERIFY_OFFSET(opc_oper, op_actdur, 48);
VERIFY_OFFSET(opc_oper, op_planstart, 52);
VERIFY_OFFSET(opc_oper, op_actstart, 56);
VERIFY_OFFSET(opc_oper, op_planend, 60);
VERIFY_OFFSET(opc_oper, op_actend, 64);
VERIFY_OFFSET(opc_oper, op_maxrc, 68);
VERIFY_OFFSET(opc_oper, op_priority, 70);
VERIFY_OFFSET(opc_oper, op_flags, 71);
VERIFY_OFFSET(opc_oper, op_desc, 72);
VERIFY_OFFSET(opc_oper, op_owner, 96);
VERIFY_OFFSET(opc_oper, op_jclmem, 104);
VERIFY_OFFSET(opc_oper, op_jcllib, 112);

/* struct opc_ux001_parm */
VERIFY_SIZE(opc_ux001_parm, 40);
VERIFY_OFFSET(opc_ux001_parm, ux001work, 0);
VERIFY_OFFSET(opc_ux001_parm, ux001func, 4);
VERIFY_OFFSET(opc_ux001_parm, ux001evty, 5);
VERIFY_OFFSET(opc_ux001_parm, _reserved1, 6);
VERIFY_OFFSET(opc_ux001_parm, ux001data, 8);
VERIFY_OFFSET(opc_ux001_parm, ux001dlen, 12);
VERIFY_OFFSET(opc_ux001_parm, ux001jobn, 16);
VERIFY_OFFSET(opc_ux001_parm, ux001jnum, 24);
VERIFY_OFFSET(opc_ux001_parm, ux001retc, 32);
VERIFY_OFFSET(opc_ux001_parm, ux001reas, 36);

/* struct opc_ux002_parm */
VERIFY_SIZE(opc_ux002_parm, 56);
VERIFY_OFFSET(opc_ux002_parm, ux002work, 0);
VERIFY_OFFSET(opc_ux002_parm, ux002func, 4);
VERIFY_OFFSET(opc_ux002_parm, ux002evty, 5);
VERIFY_OFFSET(opc_ux002_parm, _reserved1, 6);
VERIFY_OFFSET(opc_ux002_parm, ux002jobn, 8);
VERIFY_OFFSET(opc_ux002_parm, ux002jnum, 16);
VERIFY_OFFSET(opc_ux002_parm, ux002rc, 24);
VERIFY_OFFSET(opc_ux002_parm, ux002abnd, 28);
VERIFY_OFFSET(opc_ux002_parm, ux002step, 32);
VERIFY_OFFSET(opc_ux002_parm, ux002pgm, 40);
VERIFY_OFFSET(opc_ux002_parm, ux002time, 48);
VERIFY_OFFSET(opc_ux002_parm, ux002date, 52);

/* struct opc_ux003_parm */
VERIFY_SIZE(opc_ux003_parm, 48);

/* struct opc_ux004_parm */
VERIFY_SIZE(opc_ux004_parm, 76);

/* struct opc_ux007_parm */
VERIFY_SIZE(opc_ux007_parm, 28);
VERIFY_OFFSET(opc_ux007_parm, ux007func, 0);
VERIFY_OFFSET(opc_ux007_parm, ux007oper, 4);
VERIFY_OFFSET(opc_ux007_parm, ux007oldst, 8);
VERIFY_OFFSET(opc_ux007_parm, ux007newst, 9);
VERIFY_OFFSET(opc_ux007_parm, _reserved1, 10);
VERIFY_OFFSET(opc_ux007_parm, ux007rc, 12);
VERIFY_OFFSET(opc_ux007_parm, ux007abnd, 16);
VERIFY_OFFSET(opc_ux007_parm, ux007msg, 20);
VERIFY_OFFSET(opc_ux007_parm, ux007msgl, 24);
VERIFY_OFFSET(opc_ux007_parm, _reserved2, 26);

/* struct opc_ux009_parm */
VERIFY_SIZE(opc_ux009_parm, 76);

/*===================================================================
 * metalc_racf.h
 *===================================================================*/

/* struct acee */
VERIFY_OFFSET(acee, aceeacee, 0);
VERIFY_OFFSET(acee, aceesp, 4);
VERIFY_OFFSET(acee, aceelen, 5);
VERIFY_OFFSET(acee, aceevrsn, 6);
VERIFY_OFFSET(acee, aceeflg1, 7);
VERIFY_OFFSET(acee, aceeiep, 8);
VERIFY_OFFSET(acee, aceeuser, 12);
VERIFY_OFFSET(acee, aceeusri, 20);
VERIFY_OFFSET(acee, aceepass, 28);
VERIFY_OFFSET(acee, aceegrpn, 36);
VERIFY_OFFSET(acee, aceedefg, 40);
VERIFY_OFFSET(acee, aceeflg2, 48);
VERIFY_OFFSET(acee, aceeflg3, 49);
VERIFY_OFFSET(acee, aceeflg4, 50);
VERIFY_OFFSET(acee, aceeflg5, 51);
VERIFY_OFFSET(acee, aceeunam, 52);
VERIFY_OFFSET(acee, aceetrlv, 56);
VERIFY_OFFSET(acee, aceeinst, 60);
VERIFY_OFFSET(acee, aceecre8, 64);
VERIFY_OFFSET(acee, aceetokn, 68);
VERIFY_OFFSET(acee, aceeproc, 72);
VERIFY_OFFSET(acee, aceetrmp, 76);
VERIFY_OFFSET(acee, aceeapts, 80);
VERIFY_OFFSET(acee, aceefcgp, 84);

/* struct racf_rix_parm */
VERIFY_SIZE(racf_rix_parm, 64);
VERIFY_OFFSET(racf_rix_parm, rixacee, 0);
VERIFY_OFFSET(racf_rix_parm, rixwork, 4);
VERIFY_OFFSET(racf_rix_parm, rixuser, 8);
VERIFY_OFFSET(racf_rix_parm, rixuserl, 12);
VERIFY_OFFSET(racf_rix_parm, rixflg1, 13);
VERIFY_OFFSET(racf_rix_parm, rixflg2, 14);
VERIFY_OFFSET(racf_rix_parm, rixflg3, 15);
VERIFY_OFFSET(racf_rix_parm, rixpass, 16);
VERIFY_OFFSET(racf_rix_parm, rixpassl, 20);
VERIFY_OFFSET(racf_rix_parm, _reserved1, 21);
VERIFY_OFFSET(racf_rix_parm, rixnpass, 24);
VERIFY_OFFSET(racf_rix_parm, rixnpasl, 28);
VERIFY_OFFSET(racf_rix_parm, _reserved2, 29);
VERIFY_OFFSET(racf_rix_parm, rixgroup, 32);
VERIFY_OFFSET(racf_rix_parm, rixgrupl, 36);
VERIFY_OFFSET(racf_rix_parm, _reserved3, 37);
VERIFY_OFFSET(racf_rix_parm, rixappl, 40);
VERIFY_OFFSET(racf_rix_parm, rixappll, 44);
VERIFY_OFFSET(racf_rix_parm, _reserved4, 45);
VERIFY_OFFSET(racf_rix_parm, rixterm, 48);
VERIFY_OFFSET(racf_rix_parm, rixterml, 52);
VERIFY_OFFSET(racf_rix_parm, _reserved5, 53);
VERIFY_OFFSET(racf_rix_parm, rixreasn, 56);
VERIFY_OFFSET(racf_rix_parm, rixinstd, 60);

/* struct racf_rcx_parm */
VERIFY_SIZE(racf_rcx_parm, 56);
VERIFY_OFFSET(racf_rcx_parm, rcxacee, 0);
VERIFY_OFFSET(racf_rcx_parm, rcxwork, 4);
VERIFY_OFFSET(racf_rcx_parm, rcxclass, 8);
VERIFY_OFFSET(racf_rcx_parm, rcxclsln, 12);
VERIFY_OFFSET(racf_rcx_parm, rcxflg1, 13);
VERIFY_OFFSET(racf_rcx_parm, rcxflg2, 14);
VERIFY_OFFSET(racf_rcx_parm, rcxflg3, 15);
VERIFY_OFFSET(racf_rcx_parm, rcxentty, 16);
VERIFY_OFFSET(racf_rcx_parm, rcxentyl, 20);
VERIFY_OFFSET(racf_rcx_parm, rcxattr, 22);
VERIFY_OFFSET(racf_rcx_parm, rcxcsect, 23);
VERIFY_OFFSET(racf_rcx_parm, rcxinstd, 24);
VERIFY_OFFSET(racf_rcx_parm, rcxprof, 28);
VERIFY_OFFSET(racf_rcx_parm, rcxowner, 32);
VERIFY_OFFSET(racf_rcx_parm, rcxvol, 36);
VERIFY_OFFSET(racf_rcx_parm, rcxvolln, 40);
VERIFY_OFFSET(racf_rcx_parm, _reserved1, 41);
VERIFY_OFFSET(racf_rcx_parm, rcxreasn, 44);
VERIFY_OFFSET(racf_rcx_parm, rcxlogst, 48);
VERIFY_OFFSET(racf_rcx_parm, rcxlgsln, 52);
VERIFY_OFFSET(racf_rcx_parm, _reserved2, 54);

/* struct racf_pwx_parm */
VERIFY_SIZE(racf_pwx_parm, 44);
VERIFY_OFFSET(racf_pwx_parm, pwxwork, 0);
VERIFY_OFFSET(racf_pwx_parm, pwxuser, 4);
VERIFY_OFFSET(racf_pwx_parm, pwxuserl, 8);
VERIFY_OFFSET(racf_pwx_parm, pwxflg1, 9);
VERIFY_OFFSET(racf_pwx_parm, pwxflg2, 10);
VERIFY_OFFSET(racf_pwx_parm, _reserved1, 11);
VERIFY_OFFSET(racf_pwx_parm, pwxpass, 12);
VERIFY_OFFSET(racf_pwx_parm, pwxpassl, 16);
VERIFY_OFFSET(racf_pwx_parm, _reserved2, 17);
VERIFY_OFFSET(racf_pwx_parm, pwxnpass, 20);
VERIFY_OFFSET(racf_pwx_parm, pwxnpasl, 24);
VERIFY_OFFSET(racf_pwx_parm, _reserved3, 25);
VERIFY_OFFSET(racf_pwx_parm, pwxcallr, 28);
VERIFY_OFFSET(racf_pwx_parm, pwxinstd, 32);
VERIFY_OFFSET(racf_pwx_parm, pwxmsgp, 36);
VERIFY_OFFSET(racf_pwx_parm, pwxmsgln, 40);
VERIFY_OFFSET(racf_pwx_parm, _reserved4, 42);

/* struct racf_ncv_parm */
VERIFY_SIZE(racf_ncv_parm, 40);
VERIFY_OFFSET(racf_ncv_parm, ncvwork, 0);
VERIFY_OFFSET(racf_ncv_parm, ncvuser, 4);
VERIFY_OFFSET(racf_ncv_parm, ncvuserl, 8);
VERIFY_OFFSET(racf_ncv_parm, ncvflg1, 9);
VERIFY_OFFSET(racf_ncv_parm, _reserved1, 10);
VERIFY_OFFSET(racf_ncv_parm, ncvnpass, 12);
VERIFY_OFFSET(racf_ncv_parm, ncvnpasl, 16);
VERIFY_OFFSET(racf_ncv_parm, _reserved2, 17);
VERIFY_OFFSET(racf_ncv_parm, ncvopass, 20);
VERIFY_OFFSET(racf_ncv_parm, ncvopasl, 24);
VERIFY_OFFSET(racf_ncv_parm, _reserved3, 25);
VERIFY_OFFSET(racf_ncv_parm, ncvmsgp, 28);
VERIFY_OFFSET(racf_ncv_parm, ncvmsgln, 32);
VERIFY_OFFSET(racf_ncv_parm, ncvmsgmx, 34);
VERIFY_OFFSET(racf_ncv_parm, ncvinstd, 36);

/* struct racf_cnx_parm */
VERIFY_SIZE(racf_cnx_parm, 40);
VERIFY_OFFSET(racf_cnx_parm, cnxwork, 0);
VERIFY_OFFSET(racf_cnx_parm, cnxacee, 4);
VERIFY_OFFSET(racf_cnx_parm, cnxcmd, 8);
VERIFY_OFFSET(racf_cnx_parm, cnxcmdln, 12);
VERIFY_OFFSET(racf_cnx_parm, cnxflg1, 14);
VERIFY_OFFSET(racf_cnx_parm, cnxflg2, 15);
VERIFY_OFFSET(racf_cnx_parm, cnxverb, 16);
VERIFY_OFFSET(racf_cnx_parm, cnxvrbln, 20);
VERIFY_OFFSET(racf_cnx_parm, _reserved1, 21);
VERIFY_OFFSET(racf_cnx_parm, cnxprof, 24);
VERIFY_OFFSET(racf_cnx_parm, cnxprofn, 28);
VERIFY_OFFSET(racf_cnx_parm, _reserved2, 30);
VERIFY_OFFSET(racf_cnx_parm, cnxclass, 32);
VERIFY_OFFSET(racf_cnx_parm, cnxclsln, 36);
VERIFY_OFFSET(racf_cnx_parm, _reserved3, 37);

/*===================================================================
 * metalc_sa.h
 *===================================================================*/

/* struct sa_resource */
VERIFY_SIZE(sa_resource, 156);
VERIFY_OFFSET(sa_resource, resname, 0);
VERIFY_OFFSET(sa_resource, restype, 32);
VERIFY_OFFSET(sa_resource, obsstate, 33);
VERIFY_OFFSET(sa_resource, desstate, 34);
VERIFY_OFFSET(sa_resource, automation, 35);
VERIFY_OFFSET(sa_resource, resgroup, 36);
VERIFY_OFFSET(sa_resource, system, 68);
VERIFY_OFFSET(sa_resource, sysplex, 76);
VERIFY_OFFSET(sa_resource, jobname, 84);
VERIFY_OFFSET(sa_resource, asid, 92);
VERIFY_OFFSET(sa_resource, starttime, 96);
VERIFY_OFFSET(sa_resource, startdate, 100);
VERIFY_OFFSET(sa_resource, statetime, 104);
VERIFY_OFFSET(sa_resource, statedate, 108);
VERIFY_OFFSET(sa_resource, subsys, 112);
VERIFY_OFFSET(sa_resource, policy, 120);
VERIFY_OFFSET(sa_resource, health, 152);
VERIFY_OFFSET(sa_resource, priority, 153);
VERIFY_OFFSET(sa_resource, movegroup, 154);
VERIFY_OFFSET(sa_resource, flags, 155);

/* struct sa_res_parm */
VERIFY_SIZE(sa_res_parm, 44);

/* struct sa_rec_parm */
/* KNOWN ISSUE - assertions disabled for struct sa_rec_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(sa_rec_parm, 48); */

/* struct sa_msg_parm */
VERIFY_SIZE(sa_msg_parm, 60);

/* struct sa_notify_parm */
VERIFY_SIZE(sa_notify_parm, 192);

/* struct sa_timer_parm */
VERIFY_SIZE(sa_timer_parm, 72);

/*===================================================================
 * metalc_saf.h
 *===================================================================*/

/* struct saf_auth_parm */
VERIFY_OFFSET(saf_auth_parm, entity, 0);
VERIFY_OFFSET(saf_auth_parm, class_nm, 4);
VERIFY_OFFSET(saf_auth_parm, userid, 8);
VERIFY_OFFSET(saf_auth_parm, acee, 12);
VERIFY_OFFSET(saf_auth_parm, attr, 16);
VERIFY_OFFSET(saf_auth_parm, saf_rc, 20);
VERIFY_OFFSET(saf_auth_parm, racf_rc, 24);
VERIFY_OFFSET(saf_auth_parm, racf_rsn, 28);

/*===================================================================
 * metalc_smf.h
 *===================================================================*/

/* struct smf_header */
VERIFY_SIZE(smf_header, 18);
VERIFY_OFFSET(smf_header, smflen, 0);
VERIFY_OFFSET(smf_header, smfseg, 2);
VERIFY_OFFSET(smf_header, smfflg, 4);
VERIFY_OFFSET(smf_header, smfrty, 5);
VERIFY_OFFSET(smf_header, smftme, 6);
VERIFY_OFFSET(smf_header, smfdte, 10);
VERIFY_OFFSET(smf_header, smfsid, 14);

/* struct smf_subtype_header */
VERIFY_SIZE(smf_subtype_header, 24);
VERIFY_OFFSET(smf_subtype_header, header, 0);
VERIFY_OFFSET(smf_subtype_header, smfssi, 18);
VERIFY_OFFSET(smf_subtype_header, smfsubty, 22);

/* struct smf30_header */
VERIFY_OFFSET(smf30_header, hdr, 0);
VERIFY_OFFSET(smf30_header, smf30sof, 24);
VERIFY_OFFSET(smf30_header, smf30sol, 26);
VERIFY_OFFSET(smf30_header, smf30son, 28);

/* struct smf30_id_section */
VERIFY_OFFSET(smf30_id_section, smf30jbn, 0);
VERIFY_OFFSET(smf30_id_section, smf30pgm, 8);
VERIFY_OFFSET(smf30_id_section, smf30stm, 16);
VERIFY_OFFSET(smf30_id_section, smf30uif, 24);
VERIFY_OFFSET(smf30_id_section, smf30jnm, 32);
VERIFY_OFFSET(smf30_id_section, smf30stn, 40);
VERIFY_OFFSET(smf30_id_section, smf30cls, 42);
VERIFY_OFFSET(smf30_id_section, smf30pty, 43);

/* struct smf4_record */
VERIFY_OFFSET(smf4_record, header, 0);
VERIFY_OFFSET(smf4_record, smf4jbn, 18);
VERIFY_OFFSET(smf4_record, smf4rst, 26);
VERIFY_OFFSET(smf4_record, smf4rsd, 30);
VERIFY_OFFSET(smf4_record, smf4uif, 34);
VERIFY_OFFSET(smf4_record, smf4stm, 42);

/* struct smf5_record */
VERIFY_OFFSET(smf5_record, header, 0);
VERIFY_OFFSET(smf5_record, smf5jbn, 18);
VERIFY_OFFSET(smf5_record, smf5rst, 26);
VERIFY_OFFSET(smf5_record, smf5rsd, 30);
VERIFY_OFFSET(smf5_record, smf5uif, 34);
VERIFY_OFFSET(smf5_record, smf5nst, 42);

/* struct smf1415_record */
VERIFY_OFFSET(smf1415_record, header, 0);
VERIFY_OFFSET(smf1415_record, smf14jbn, 18);
VERIFY_OFFSET(smf1415_record, smf14rst, 26);
VERIFY_OFFSET(smf1415_record, smf14rsd, 30);
VERIFY_OFFSET(smf1415_record, smf14uif, 34);
VERIFY_OFFSET(smf1415_record, smf14dsn, 42);

/* struct smf80_record */
VERIFY_OFFSET(smf80_record, hdr, 0);
VERIFY_OFFSET(smf80_record, smf80uid, 24);
VERIFY_OFFSET(smf80_record, smf80evn, 32);
VERIFY_OFFSET(smf80_record, smf80evq, 33);

/* struct smf_exit_parm */
VERIFY_OFFSET(smf_exit_parm, smf_session, 0);
VERIFY_OFFSET(smf_exit_parm, subsys_id, 4);
VERIFY_OFFSET(smf_exit_parm, record_ptr, 8);
VERIFY_OFFSET(smf_exit_parm, record_len, 12);
VERIFY_OFFSET(smf_exit_parm, return_code, 16);
VERIFY_OFFSET(smf_exit_parm, flags, 20);
VERIFY_OFFSET(smf_exit_parm, reserved, 21);

/* struct iefu29_parm */
VERIFY_OFFSET(iefu29_parm, dump_parm, 0);
VERIFY_OFFSET(iefu29_parm, dump_type, 4);
VERIFY_OFFSET(iefu29_parm, flags, 5);
VERIFY_OFFSET(iefu29_parm, reserved, 6);
VERIFY_OFFSET(iefu29_parm, jobname, 8);
VERIFY_OFFSET(iefu29_parm, stepname, 16);
VERIFY_OFFSET(iefu29_parm, abend_code, 24);
VERIFY_OFFSET(iefu29_parm, reason_code, 28);

/* struct iefuji_parm */
VERIFY_OFFSET(iefuji_parm, jmr, 0);
VERIFY_OFFSET(iefuji_parm, intrdr, 4);
VERIFY_OFFSET(iefuji_parm, jobname, 8);
VERIFY_OFFSET(iefuji_parm, jobclass, 16);
VERIFY_OFFSET(iefuji_parm, priority, 17);
VERIFY_OFFSET(iefuji_parm, reserved, 18);
VERIFY_OFFSET(iefuji_parm, programmer, 20);
VERIFY_OFFSET(iefuji_parm, account, 40);

/* struct iefusi_parm */
VERIFY_OFFSET(iefusi_parm, jmr, 0);
VERIFY_OFFSET(iefusi_parm, jobname, 4);
VERIFY_OFFSET(iefusi_parm, stepname, 12);
VERIFY_OFFSET(iefusi_parm, procstep, 20);
VERIFY_OFFSET(iefusi_parm, pgmname, 28);
VERIFY_OFFSET(iefusi_parm, region_req, 36);
VERIFY_OFFSET(iefusi_parm, region_lim, 40);
VERIFY_OFFSET(iefusi_parm, region_below, 44);
VERIFY_OFFSET(iefusi_parm, region_above, 48);

/*===================================================================
 * metalc_svc.h
 *===================================================================*/

/* struct wto_parm */
VERIFY_OFFSET(wto_parm, wto_len, 0);
VERIFY_OFFSET(wto_parm, wto_mcsflags, 2);
VERIFY_OFFSET(wto_parm, wto_text, 4);

/*===================================================================
 * metalc_tcpip.h
 *===================================================================*/

/* struct sockaddr_in */
VERIFY_SIZE(sockaddr_in, 16);
VERIFY_OFFSET(sockaddr_in, sin_len, 0);
VERIFY_OFFSET(sockaddr_in, sin_family, 1);
VERIFY_OFFSET(sockaddr_in, sin_port, 2);
VERIFY_OFFSET(sockaddr_in, sin_addr, 4);
VERIFY_OFFSET(sockaddr_in, sin_zero, 8);

/* struct sockaddr_in6 */
VERIFY_SIZE(sockaddr_in6, 28);
VERIFY_OFFSET(sockaddr_in6, sin6_len, 0);
VERIFY_OFFSET(sockaddr_in6, sin6_family, 1);
VERIFY_OFFSET(sockaddr_in6, sin6_port, 2);
VERIFY_OFFSET(sockaddr_in6, sin6_flowinfo, 4);
VERIFY_OFFSET(sockaddr_in6, sin6_addr, 8);
VERIFY_OFFSET(sockaddr_in6, sin6_scope_id, 24);

/* struct ftp_chkcmd_parm */
VERIFY_SIZE(ftp_chkcmd_parm, 356);

/* struct ftp_postpr_parm */
VERIFY_SIZE(ftp_postpr_parm, 340);

/* struct tn3270_conn_parm */
VERIFY_SIZE(tn3270_conn_parm, 68);

/* struct ipflt_parm */
/* KNOWN ISSUE - assertions disabled for struct ipflt_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(ipflt_parm, 52); */

/* struct tcpsec_parm */
/* KNOWN ISSUE - assertions disabled for struct tcpsec_parm.
   Tracked in tools/layout_known_issues.txt; see
   docs/layout-findings.md. Re-enable by regenerating
   once the mismatch is resolved. */
/* VERIFY_SIZE(tcpsec_parm, 48); */

/*===================================================================
 * metalc_vtam.h
 *===================================================================*/

/* struct vtam_acb */
VERIFY_SIZE(vtam_acb, 40);
VERIFY_OFFSET(vtam_acb, acbid, 0);
VERIFY_OFFSET(vtam_acb, acblen, 4);
VERIFY_OFFSET(vtam_acb, acbtype, 5);
VERIFY_OFFSET(vtam_acb, acbflags, 6);
VERIFY_OFFSET(vtam_acb, acbappl, 8);
VERIFY_OFFSET(vtam_acb, acblogon, 12);
VERIFY_OFFSET(vtam_acb, acblosep, 16);
VERIFY_OFFSET(vtam_acb, acbrelrq, 20);
VERIFY_OFFSET(vtam_acb, acbuserfd, 24);
VERIFY_OFFSET(vtam_acb, acberror, 28);
VERIFY_OFFSET(vtam_acb, acbsession, 32);
VERIFY_OFFSET(vtam_acb, acbactive, 36);

/* struct vtam_nib */
VERIFY_SIZE(vtam_nib, 36);
VERIFY_OFFSET(vtam_nib, nibid, 0);
VERIFY_OFFSET(vtam_nib, niblen, 4);
VERIFY_OFFSET(vtam_nib, nibtype, 5);
VERIFY_OFFSET(vtam_nib, nibflags, 6);
VERIFY_OFFSET(vtam_nib, nibsym, 8);
VERIFY_OFFSET(vtam_nib, nibmode, 16);
VERIFY_OFFSET(vtam_nib, nibuser, 20);
VERIFY_OFFSET(vtam_nib, nibusrln, 24);
VERIFY_OFFSET(vtam_nib, niblogon, 28);
VERIFY_OFFSET(vtam_nib, niblglen, 32);

/* struct vtam_rpl */
VERIFY_SIZE(vtam_rpl, 52);
VERIFY_OFFSET(vtam_rpl, rplid, 0);
VERIFY_OFFSET(vtam_rpl, rpllen, 4);
VERIFY_OFFSET(vtam_rpl, rpltype, 5);
VERIFY_OFFSET(vtam_rpl, rplflags, 6);
VERIFY_OFFSET(vtam_rpl, rplrtncd, 8);
VERIFY_OFFSET(vtam_rpl, rplfdb2, 12);
VERIFY_OFFSET(vtam_rpl, rplsense, 16);
VERIFY_OFFSET(vtam_rpl, rplarea, 20);
VERIFY_OFFSET(vtam_rpl, rplareap, 24);
VERIFY_OFFSET(vtam_rpl, rplrlen, 28);
VERIFY_OFFSET(vtam_rpl, rplacb, 32);
VERIFY_OFFSET(vtam_rpl, rplarg, 36);
VERIFY_OFFSET(vtam_rpl, rplnib, 40);
VERIFY_OFFSET(vtam_rpl, rplusfld, 44);
VERIFY_OFFSET(vtam_rpl, rplreq, 48);
VERIFY_OFFSET(vtam_rpl, rplcntrl, 49);
VERIFY_OFFSET(vtam_rpl, _reserved, 50);

/* struct vtam_ly_parm */
VERIFY_SIZE(vtam_ly_parm, 72);

/* struct vtam_cs_parm */
VERIFY_SIZE(vtam_cs_parm, 48);
VERIFY_OFFSET(vtam_cs_parm, cswork, 0);
VERIFY_OFFSET(vtam_cs_parm, csfunc, 4);
VERIFY_OFFSET(vtam_cs_parm, csflags, 5);
VERIFY_OFFSET(vtam_cs_parm, cslutype, 6);
VERIFY_OFFSET(vtam_cs_parm, _reserved1, 7);
VERIFY_OFFSET(vtam_cs_parm, csluname, 8);
VERIFY_OFFSET(vtam_cs_parm, csappl, 16);
VERIFY_OFFSET(vtam_cs_parm, cssessp, 24);
VERIFY_OFFSET(vtam_cs_parm, cssessln, 28);
VERIFY_OFFSET(vtam_cs_parm, csbindp, 32);
VERIFY_OFFSET(vtam_cs_parm, csbindln, 36);
VERIFY_OFFSET(vtam_cs_parm, csreasn, 40);
VERIFY_OFFSET(vtam_cs_parm, cssense, 44);

/* struct vtam_uv_parm */
VERIFY_SIZE(vtam_uv_parm, 44);

/* struct vtam_vr_parm */
VERIFY_SIZE(vtam_vr_parm, 40);

/* Generated: 68 size assertions, 821 offset assertions across 105 structs */
