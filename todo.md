# Asm2C Conversion Tasks

## 1. Products identified from ./includes/ headers (DONE)

| # | Header | Product | Representative Exit |
|---|--------|---------|---------------------|
| 1 | metalc_smf.h | SMF | IEFU83 |
| 2 | metalc_jes2.h | JES2 | *(existing: HASPEX02, HASPEX20)* |
| 3 | metalc_racf.h | RACF | *(existing: ICHPWX01)* |
| 4 | metalc_opc.h | OPC/TWS | EQQUX007 |
| 5 | metalc_cics.h | CICS | DFHPEP |
| 6 | metalc_ims.h | IMS | *(existing: DFSMSCE0, DFSWHU00)* |
| 7 | metalc_db2.h | DB2 | DSN3ATH |
| 8 | metalc_vtam.h | VTAM | ISTEXCLY |
| 9 | metalc_tcpip.h | TCP/IP | FTCHKCMD |
| 10 | metalc_dfsms.h | DFSMS | IEFDB401 |
| 11 | metalc_mq.h | MQ | CSQXLIB (Channel Security) |
| 12 | metalc_sa.h | SA | AOFEXC02 |
| 13 | metalc_base.h | z/OS Base | *(utility header, no exit)* |
| 14 | metalc_netview.h | NetView | DSIEX01 |
| 15 | metalc_acf2.h | ACF2 | ACF2PWX (Password Exit) |

## 2. Gap list - products needing examples (DONE)

Already had: IMS (2), RACF (1), JES2 (2)

## 3. Create examples - progress

- [x] SMF - `asm/SMF/IEFU83.asm` (SMF record filtering exit)
- [x] OPC/TWS - `asm/OPC/EQQUX007.asm` (Operation status change exit)
- [x] CICS - `asm/CICS/DFHPEP.asm` (Program error program)
- [x] DB2 - `asm/DB2/DSN3ATH.asm` (Authorization exit)
- [x] VTAM - `asm/VTAM/ISTEXCLY.asm` (Logon verify exit)
- [x] TCP/IP - `asm/TCPIP/FTCHKCMD.asm` (FTP command validation exit)
- [x] DFSMS - `asm/DFSMS/IEFDB401.asm` (Dynamic allocation exit)
- [x] MQ - `asm/MQ/CSQXLIB.asm` (Channel security exit)
- [x] SA - `asm/SA/AOFEXC02.asm` (Resource state change exit)
- [x] NetView - `asm/NETVIEW/DSIEX01.asm` (Command preprocessing exit)
- [x] ACF2 - `asm/ACF2/ACF2PWX.asm` (Password validation exit)

All 11 missing products now have assembler exit examples.
Total: 16 assembler exits across 14 products (IMS has 2, JES2 has 2).

## 4. Metal C conversions (DONE)

- [x] SMF - `converted/SMF/IEFU83.c`
- [x] OPC/TWS - `converted/OPC/EQQUX007.c`
- [x] CICS - `converted/CICS/DFHPEP.c`
- [x] DB2 - `converted/DB2/DSN3ATH.c`
- [x] VTAM - `converted/VTAM/ISTEXCLY.c`
- [x] TCP/IP - `converted/TCPIP/FTCHKCMD.c`
- [x] DFSMS - `converted/DFSMS/IEFDB401.c`
- [x] MQ - `converted/MQ/CSQXLIB.c`
- [x] SA - `converted/SA/AOFEXC02.c`
- [x] NetView - `converted/NETVIEW/DSIEX01.c`
- [x] ACF2 - `converted/ACF2/ACF2PWX.c`

All 11 assembler exits converted to Metal C (IBM xlc -qmetal, no LE dependencies).
