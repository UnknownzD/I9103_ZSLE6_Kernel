/*
 * Copyright (C) 2011 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#ifndef __SH532U_H__
#define __SH532U_H__

#include <linux/ioctl.h> /* For IOCTL macros */

#define SH532U_IOCTL_GET_CONFIG	_IOR('o', 1, struct sh532u_config)
#define SH532U_IOCTL_SET_POSITION	_IOW('o', 2, u32)

struct sh532u_config {
	__u32 settle_time;
	__u32 focal_length;
	__u32 fnumber;
	s16 pos_low;
	s16 pos_high;
	s16 limit_low;
	s16 limit_high;
};

/* Register Definition  : Sany Driver IC */
/* EEPROM addresses */
#define addrHallOffset		0x10
#define addrHallBias		0x11
#define addrInf1		0x12
#define addrMac1		0x13
#define addrLoopGainH		0x14
#define addrLoopGainL		0x15
#define addrInf2		0x16
#define addrMac2		0x17

#define addrInf1_H		0x20 /* bottom mechanical limit of HVCA */
#define addrInf1_L		0x21
#define addrMac1_H		0x22 /* top mechanical limit of HVCA */
#define addrMac1_L		0x23
#define addrInf2_H		0x24 /* lens position when object is ?120cm */
#define addrInf2_L		0x25
#define addrMac2_H		0x26 /* lens position when object is ?10cm */
#define addrMac2_L		0x27
#define addrDacDeltaUp_H	0x28 /* difference between face up and down */
#define addrDacDeltaUp_L	0x29
#define addrAFoffset_H		0x2A /* best focus position subtract value */
#define addrAFoffset_L		0x2B

/* Convergence Judgement */
#define INI_MSSET_211		0x00
#define CHTGOKN_TIME		0x80
#define CHTGOKN_WAIT		1
#define CHTGOKN_TIMEOUT		50
#define CHTGSTOKN_TOMEOUT	15

/* StepMove */
#define STMV_SIZE		0x0180

#define STMCHTG_ON		0x08
#define STMSV_ON		0x04
#define STMLFF_ON		0x02
#define STMVEN_ON		0x01
#define STMCHTG_OFF		0x00
#define STMSV_OFF		0x00
#define STMLFF_OFF		0x00
#define STMVEN_OFF		0x00

#define STMCHTG_SET		STMCHTG_ON
#define STMSV_SET		STMSV_ON
#define STMLFF_SET		STMLFF_OFF

#define CHTGST_ON		0x01
#define DEFAULT_DADAT		0x8040

/* Delay RAM 00h ~ 3Fh */
#define ADHXI_211H		0x00
#define ADHXI_211L		0x01
#define PIDZO_211H		0x02
#define PIDZO_211L		0x03
#define RZ_211H		0x04
#define RZ_211L		0x05
#define DZ1_211H		0x06
#define DZ1_211L		0x07
#define DZ2_211H		0x08
#define DZ2_211L		0x09
#define UZ1_211H		0x0A
#define UZ1_211L		0x0B
#define UZ2_211H		0x0C
#define UZ2_211L		0x0D
#define IZ1_211H		0x0E
#define IZ1_211L		0x0F
#define IZ2_211H		0x10
#define IZ2_211L		0x11
#define MS1Z01_211H		0x12
#define MS1Z01_211L		0x13
#define MS1Z11_211H		0x14
#define MS1Z11_211L		0x15
#define MS1Z12_211H		0x16
#define MS1Z12_211L		0x17
#define MS1Z22_211H		0x18
#define MS1Z22_211L		0x19
#define MS2Z01_211H		0x1A
#define MS2Z01_211L		0x1B
#define MS2Z11_211H		0x1C
#define MS2Z11_211L		0x1D
#define MS2Z12_211H		0x1E
#define MS2Z12_211L		0x1F
#define MS2Z22_211H		0x20
#define MS2Z22_211L		0x21
#define MS2Z23_211H		0x22
#define MS2Z23_211L		0x23
#define OZ1_211H		0x24
#define OZ1_211L		0x25
#define OZ2_211H		0x26
#define OZ2_211L		0x27
#define DAHLXO_211H		0x28
#define DAHLXO_211L		0x29
#define OZ3_211H		0x2A
#define OZ3_211L		0x2B
#define OZ4_211H		0x2C
#define OZ4_211L		0x2D
#define OZ5_211H		0x2E
#define OZ5_211L		0x2F
#define oe_211H		0x30
#define oe_211L		0x31
#define MSR1CMAX_211H		0x32
#define MSR1CMAX_211L		0x33
#define MSR1CMIN_211H		0x34
#define MSR1CMIN_211L		0x35
#define MSR2CMAX_211H		0x36
#define MSR2CMAX_211L		0x37
#define MSR2CMIN_211H		0x38
#define MSR2CMIN_211L		0x39
#define OFFSET_211H		0x3A
#define OFFSET_211L		0x3B
#define ADOFFSET_211H		0x3C
#define ADOFFSET_211L		0x3D
#define EZ_211H		0x3E
#define EZ_211L		0x3F

/* Coefficient RAM 40h ~ 7Fh */
#define ag_211H		0x40
#define ag_211L		0x41
#define da_211H		0x42
#define da_211L		0x43
#define db_211H		0x44
#define db_211L		0x45
#define dc_211H		0x46
#define dc_211L		0x47
#define dg_211H		0x48
#define dg_211L		0x49
#define pg_211H		0x4A
#define pg_211L		0x4B
#define gain1_211H		0x4C
#define gain1_211L		0x4D
#define gain2_211H		0x4E
#define gain2_211L		0x4F
#define ua_211H		0x50
#define ua_211L		0x51
#define uc_211H		0x52
#define uc_211L		0x53
#define ia_211H		0x54
#define ia_211L		0x55
#define ib_211H		0x56
#define ib_211L		0x57
#define i_c_211H		0x58
#define i_c_211L		0x59
#define ms11a_211H		0x5A
#define ms11a_211L		0x5B
#define ms11c_211H		0x5C
#define ms11c_211L		0x5D
#define ms12a_211H		0x5E
#define ms12a_211L		0x5F
#define ms12c_211H		0x60
#define ms12c_211L		0x61
#define ms21a_211H		0x62
#define ms21a_211L		0x63
#define ms21b_211H		0x64
#define ms21b_211L		0x65
#define ms21c_211H		0x66
#define ms21c_211L		0x67
#define ms22a_211H		0x68
#define ms22a_211L		0x69
#define ms22c_211H		0x6A
#define ms22c_211L		0x6B
#define ms22d_211H		0x6C
#define ms22d_211L		0x6D
#define ms22e_211H		0x6E
#define ms22e_211L		0x6F
#define ms23p_211H		0x70
#define ms23p_211L		0x71
#define oa_211H		0x72
#define oa_211L		0x73
#define oc_211H		0x74
#define oc_211L		0x75
#define PX12_211H		0x76
#define PX12_211L		0x77
#define PX3_211H		0x78
#define PX3_211L		0x79
#define MS2X_211H		0x7A
#define MS2X_211L		0x7B
#define CHTGX_211H		0x7C
#define CHTGX_211L		0x7D
#define CHTGN_211H		0x7E
#define CHTGN_211L		0x7F

/* Register 80h ~  9F */
#define CLKSEL_211		0x80
#define ADSET_211		0x81
#define PWMSEL_211		0x82
#define SWTCH_211		0x83
#define STBY_211		0x84
#define CLR_211		0x85
#define DSSEL_211		0x86
#define ENBL_211		0x87
#define ANA1_211		0x88
#define STMVEN_211		0x8A
#define STPT_211		0x8B
#define SWFC_211		0x8C
#define SWEN_211		0x8D
#define MSNUM_211		0x8E
#define MSSET_211		0x8F
#define DLYMON_211		0x90
#define MONA_211		0x91
#define PWMLIMIT_211		0x92
#define PINSEL_211		0x93
#define PWMSEL2_211		0x94
#define SFTRST_211		0x95
#define TEST_211		0x96
#define PWMZONE2_211		0x97
#define PWMZONE1_211		0x98
#define PWMZONE0_211		0x99
#define ZONE3_211		0x9A
#define ZONE2_211		0x9B
#define ZONE1_211		0x9C
#define ZONE0_211		0x9D
#define GCTIM_211		0x9E
#define GCTIM_211NU		0x9F
#define STMINT_211		0xA0
#define STMVENDH_211		0xA1
#define STMVENDL_211		0xA2
#define MSNUMR_211		0xA3
#define  ANA2_211		0xA4

/* Device ID of HVCA Drive IC */
#define HVCA_DEVICE_ID		0xE4

/* Device ID of E2P ROM */
#define EEP_DEVICE_ID		0xA0
#define EEP_PAGE0		0x00
#define EEP_PAGE1		0x02
#define EEP_PAGE2		0x04
#define EEP_PAGE3		0x06
/* E2P ROM has 1023 bytes. So there are 4 pages memory */
/* E2PROM Device ID = 1 0 1 0  0 P0 P1 0 */
/*
P0  P1
0   0   : Page 0
0   1   : Page 1
1   0   : Page 2
1   1   : Page 3
*/
/* Page 0: address 0x000~0x0FF, E2PROM Device ID = E2P_DEVICE_ID|E2P_PAGE0 */
/* Page 1: address 0x100~0x1FF, E2PROM Device ID = E2P_DEVICE_ID|E2P_PAGE1 */
/* Page 2: address 0x200~0x2FF, E2PROM Device ID = E2P_DEVICE_ID|E2P_PAGE2 */
/* Page 3: address 0x300~0x3FF, E2PROM Device ID = E2P_DEVICE_ID|E2P_PAGE3 */
/*
*/

/* E2P data type define of HVCA Initial Value Section */
#define DIRECT_MODE		0x00
#define INDIRECT_EEPROM	0x10
#define INDIRECT_HVCA		0x20
#define MASK_AND		0x70
#define MASK_OR		0x80

#define DATA_1BYTE		0x01
#define DATA_2BYTE		0x02

#define START_ADDR		0x0030
#define END_ADDR		0x01BF

/*Macro define*/
#define abs(a)		(((a) > 0) ? (a) : -(a))

#endif
/* __SH532U_H__ */
