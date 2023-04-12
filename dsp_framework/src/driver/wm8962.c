/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * wm8962.c - wm8962 driver
 */

#include "mydefs.h"
#include "io.h"
#include "i2c.h"
#include "wm8962.h"
#include "debug.h"

struct reg_default {
	unsigned int reg;
	unsigned int def;
};

static struct reg_default wm8962_reg_cache[] = {
	{ 0, 0x009F },   /* R0     - Left Input volume */
	{ 1, 0x069F },   /* R1     - Right Input volume */
	{ 2, 0x0000 },   /* R2     - HPOUTL volume */
	{ 3, 0x0000 },   /* R3     - HPOUTR volume */

	{ 4, 0x0020 },   /* R4     - Clocking1 */

	{ 5, 0x0018 },   /* R5     - ADC & DAC Control 1 */
	{ 6, 0x2008 },   /* R6     - ADC & DAC Control 2 */
	{ 7, 0x000A },   /* R7     - Audio Interface 0 */
	{ 8, 0x01E4 },   /* R8     - Clocking2 */
	{ 9, 0x0300 },   /* R9     - Audio Interface 1 */
	{ 10, 0x00C0 },  /* R10    - Left DAC volume */
	{ 11, 0x00C0 },  /* R11    - Right DAC volume */

	{ 14, 0x0040 },   /* R14    - Audio Interface 2 */
	{ 15, 0x6243 },   /* R15    - Software Reset */

	{ 17, 0x007B },   /* R17    - ALC1 */
	{ 18, 0x0000 },   /* R18    - ALC2 */
	{ 19, 0x1C32 },   /* R19    - ALC3 */
	{ 20, 0x3200 },   /* R20    - Noise Gate */
	{ 21, 0x00C0 },   /* R21    - Left ADC volume */
	{ 22, 0x00C0 },   /* R22    - Right ADC volume */
	{ 23, 0x0100 },   /* R23    - Additional control(1) */
	{ 24, 0x0000 },   /* R24    - Additional control(2) */
	{ 25, 0x0000 },   /* R25    - Pwr Mgmt (1) */
	{ 26, 0x0000 },   /* R26    - Pwr Mgmt (2) */
	{ 27, 0x0010 },   /* R27    - Additional Control (3) */
	{ 28, 0x0000 },   /* R28    - Anti-pop */

	{ 30, 0x005E },   /* R30    - Clocking 3 */
	{ 31, 0x0000 },   /* R31    - Input mixer control (1) */
	{ 32, 0x0145 },   /* R32    - Left input mixer volume */
	{ 33, 0x0145 },   /* R33    - Right input mixer volume */
	{ 34, 0x0009 },   /* R34    - Input mixer control (2) */
	{ 35, 0x0004 },   /* R35    - Input bias control */
	{ 37, 0x0008 },   /* R37    - Left input PGA control */
	{ 38, 0x0008 },   /* R38    - Right input PGA control */

	{ 40, 0x0000 },   /* R40    - SPKOUTL volume */
	{ 41, 0x0000 },   /* R41    - SPKOUTR volume */

	{ 47, 0x0000 },   /* R47    - Thermal Shutdown Status */
	{ 48, 0x8027 },   /* R48    - Additional Control4 */

	{ 49, 0x0010 },   /* R49    - Class D Control 1 */
	{ 51, 0x0003 },   /* R51    - Class D Control 2 */

	{ 56, 0x0506 },   /* R56    - Clocking 4 */
	{ 57, 0x0000 },   /* R57    - DAC DSP Mixing (1) */
	{ 58, 0x0000 },   /* R58    - DAC DSP Mixing (2) */

	{ 60, 0x0000 },   /* R60    - DC Servo 0 */
	{ 61, 0x0000 },   /* R61    - DC Servo 1 */

	{ 64, 0x0810 },   /* R64    - DC Servo 4 */
	{ 66, 0x0000 },   /* R66    - DC Servo 6 */

	{ 68, 0x001B },   /* R68    - Analogue PGA Bias */
	{ 69, 0x0000 },   /* R69    - Analogue HP 0 */

	{ 71, 0x01FB },   /* R71    - Analogue HP 2 */
	{ 72, 0x0000 },   /* R72    - Charge Pump 1 */

	{ 82, 0x0004 },   /* R82    - Charge Pump B */

	{ 87, 0x0000 },   /* R87    - Write Sequencer Control 1 */

	{ 90, 0x0000 },   /* R90    - Write Sequencer Control 2 */

	{ 93, 0x0000 },   /* R93    - Write Sequencer Control 3 */
	{ 94, 0x0000 },   /* R94    - Control Interface */

	{ 99, 0x0000 },   /* R99    - Mixer Enables */
	{ 100, 0x0000 },   /* R100   - Headphone Mixer (1) */
	{ 101, 0x0000 },   /* R101   - Headphone Mixer (2) */
	{ 102, 0x013F },   /* R102   - Headphone Mixer (3) */
	{ 103, 0x013F },   /* R103   - Headphone Mixer (4) */

	{ 105, 0x0000 },   /* R105   - Speaker Mixer (1) */
	{ 106, 0x0000 },   /* R106   - Speaker Mixer (2) */
	{ 107, 0x013F },   /* R107   - Speaker Mixer (3) */
	{ 108, 0x013F },   /* R108   - Speaker Mixer (4) */
	{ 109, 0x0003 },   /* R109   - Speaker Mixer (5) */
	{ 110, 0x0002 },   /* R110   - Beep Generator (1) */

	{ 115, 0x0006 },   /* R115   - Oscillator Trim (3) */
	{ 116, 0x0026 },   /* R116   - Oscillator Trim (4) */

	{ 119, 0x0000 },   /* R119   - Oscillator Trim (7) */

	{ 124, 0x0011 },   /* R124   - Analogue Clocking1 */
	{ 125, 0x004B },   /* R125   - Analogue Clocking2 */
	{ 126, 0x001F },   /* R126   - Analogue Clocking3 */
	{ 127, 0x0000 },   /* R127   - PLL Software Reset */

	{ 129, 0x0001 },   /* R129   - PLL 2 */
	{ 131, 0x0010 },   /* R131   - PLL 4 */

	{ 136, 0x0067 },   /* R136   - PLL 9 */
	{ 137, 0x001C },   /* R137   - PLL 10 */
	{ 138, 0x0071 },   /* R138   - PLL 11 */
	{ 139, 0x00C7 },   /* R139   - PLL 12 */
	{ 140, 0x0067 },   /* R140   - PLL 13 */
	{ 141, 0x0048 },   /* R141   - PLL 14 */
	{ 142, 0x0022 },   /* R142   - PLL 15 */
	{ 143, 0x0097 },   /* R143   - PLL 16 */
	{ 150, 0x0003 },   /* R150   - PLL DLL */

	{ 155, 0x000C },   /* R155   - FLL Control (1) */
	{ 156, 0x0039 },   /* R156   - FLL Control (2) */
	{ 157, 0x0180 },   /* R157   - FLL Control (3) */

	{ 159, 0x0032 },   /* R159   - FLL Control (5) */
	{ 160, 0x0018 },   /* R160   - FLL Control (6) */
	{ 161, 0x007D },   /* R161   - FLL Control (7) */
	{ 162, 0x0008 },   /* R162   - FLL Control (8) */

	{ 252, 0x0005 },   /* R252   - General test 1 */

	{ 256, 0x0000 },   /* R256   - DF1 */
	{ 257, 0x0000 },   /* R257   - DF2 */
	{ 258, 0x0000 },   /* R258   - DF3 */
	{ 259, 0x0000 },   /* R259   - DF4 */
	{ 260, 0x0000 },   /* R260   - DF5 */
	{ 261, 0x0000 },   /* R261   - DF6 */
	{ 262, 0x0000 },   /* R262   - DF7 */

	{ 264, 0x0000 },   /* R264   - LHPF1 */
	{ 265, 0x0000 },   /* R265   - LHPF2 */

	{ 268, 0x0000 },   /* R268   - THREED1 */
	{ 269, 0x0000 },   /* R269   - THREED2 */
	{ 270, 0x0000 },   /* R270   - THREED3 */
	{ 271, 0x0000 },   /* R271   - THREED4 */

	{ 276, 0x000C },   /* R276   - DRC 1 */
	{ 277, 0x0925 },   /* R277   - DRC 2 */
	{ 278, 0x0000 },   /* R278   - DRC 3 */
	{ 279, 0x0000 },   /* R279   - DRC 4 */
	{ 280, 0x0000 },   /* R280   - DRC 5 */

	{ 285, 0x0000 },   /* R285   - Tloopback */

	{ 335, 0x0004 },   /* R335   - EQ1 */
	{ 336, 0x6318 },   /* R336   - EQ2 */
	{ 337, 0x6300 },   /* R337   - EQ3 */
	{ 338, 0x0FCA },   /* R338   - EQ4 */
	{ 339, 0x0400 },   /* R339   - EQ5 */
	{ 340, 0x00D8 },   /* R340   - EQ6 */
	{ 341, 0x1EB5 },   /* R341   - EQ7 */
	{ 342, 0xF145 },   /* R342   - EQ8 */
	{ 343, 0x0B75 },   /* R343   - EQ9 */
	{ 344, 0x01C5 },   /* R344   - EQ10 */
	{ 345, 0x1C58 },   /* R345   - EQ11 */
	{ 346, 0xF373 },   /* R346   - EQ12 */
	{ 347, 0x0A54 },   /* R347   - EQ13 */
	{ 348, 0x0558 },   /* R348   - EQ14 */
	{ 349, 0x168E },   /* R349   - EQ15 */
	{ 350, 0xF829 },   /* R350   - EQ16 */
	{ 351, 0x07AD },   /* R351   - EQ17 */
	{ 352, 0x1103 },   /* R352   - EQ18 */
	{ 353, 0x0564 },   /* R353   - EQ19 */
	{ 354, 0x0559 },   /* R354   - EQ20 */
	{ 355, 0x4000 },   /* R355   - EQ21 */
	{ 356, 0x6318 },   /* R356   - EQ22 */
	{ 357, 0x6300 },   /* R357   - EQ23 */
	{ 358, 0x0FCA },   /* R358   - EQ24 */
	{ 359, 0x0400 },   /* R359   - EQ25 */
	{ 360, 0x00D8 },   /* R360   - EQ26 */
	{ 361, 0x1EB5 },   /* R361   - EQ27 */
	{ 362, 0xF145 },   /* R362   - EQ28 */
	{ 363, 0x0B75 },   /* R363   - EQ29 */
	{ 364, 0x01C5 },   /* R364   - EQ30 */
	{ 365, 0x1C58 },   /* R365   - EQ31 */
	{ 366, 0xF373 },   /* R366   - EQ32 */
	{ 367, 0x0A54 },   /* R367   - EQ33 */
	{ 368, 0x0558 },   /* R368   - EQ34 */
	{ 369, 0x168E },   /* R369   - EQ35 */
	{ 370, 0xF829 },   /* R370   - EQ36 */
	{ 371, 0x07AD },   /* R371   - EQ37 */
	{ 372, 0x1103 },   /* R372   - EQ38 */
	{ 373, 0x0564 },   /* R373   - EQ39 */
	{ 374, 0x0559 },   /* R374   - EQ40 */
	{ 375, 0x4000 },   /* R375   - EQ41 */

	{ 513, 0x0000 },   /* R513   - GPIO 2 */
	{ 514, 0x0000 },   /* R514   - GPIO 3 */

	{ 516, 0x8100 },   /* R516   - GPIO 5 */
	{ 517, 0x8100 },   /* R517   - GPIO 6 */

	{ 560, 0x0000 },   /* R560   - Interrupt Status 1 */
	{ 561, 0x0000 },   /* R561   - Interrupt Status 2 */

	{ 568, 0x0030 },   /* R568   - Interrupt Status 1 Mask */
	{ 569, 0xFFFF },   /* R569   - Interrupt Status 2 Mask */

	{ 576, 0x0000 },   /* R576   - Interrupt Control */

	{ 584, 0x003F },   /* R584   - IRQ Debounce */

	{ 586, 0x0000 },   /* R586   -  MICINT Source Pol */

	{ 768, 0x1C00 },   /* R768   - DSP2 Power Management */
	{ 1037, 0x0000 },   /* R1037  - DSP2_ExecControl */

	{ 4096, 0x001C },   /* R4096  - Write Sequencer 0 */
	{ 4097, 0x0003 },   /* R4097  - Write Sequencer 1 */
	{ 4098, 0x0103 },   /* R4098  - Write Sequencer 2 */
	{ 4099, 0x0000 },   /* R4099  - Write Sequencer 3 */
	{ 4100, 0x0019 },   /* R4101  - Write Sequencer 4 */
	{ 4101, 0x0007 },   /* R4102  - Write Sequencer 5 */
	{ 4102, 0x0206 },   /* R4103  - Write Sequencer 6 */
	{ 4103, 0x0000 },   /* R4104  - Write Sequencer 7 */

	// 4603 ~4607 */

	{ 8192, 0x0000 },   /* R8192  - DSP2 Instruction RAM 0 */

	{ 9216, 0x0030 },   /* R9216  - DSP2 Address RAM 2 */
	{ 9217, 0x0000 },   /* R9217  - DSP2 Address RAM 1 */
	{ 9218, 0x0000 },   /* R9218  - DSP2 Address RAM 0 */

	{ 12288, 0x0000 },   /* R12288 - DSP2 Data1 RAM 1 */
	{ 12289, 0x0000 },   /* R12289 - DSP2 Data1 RAM 0 */

	{ 13312, 0x0000 },   /* R13312 - DSP2 Data2 RAM 1 */
	{ 13313, 0x0000 },   /* R13313 - DSP2 Data2 RAM 0 */

	{ 14336, 0x0000 },   /* R14336 - DSP2 Data3 RAM 1 */
	{ 14337, 0x0000 },   /* R14337 - DSP2 Data3 RAM 0 */

	{ 15360, 0x000A },   /* R15360 - DSP2 Coeff RAM 0 */

	{ 16384, 0x0000 },   /* R16384 - RETUNEADC_SHARED_COEFF_1 */
	{ 16385, 0x0000 },   /* R16385 - RETUNEADC_SHARED_COEFF_0 */
	{ 16386, 0x0000 },   /* R16386 - RETUNEDAC_SHARED_COEFF_1 */
	{ 16387, 0x0000 },   /* R16387 - RETUNEDAC_SHARED_COEFF_0 */
	{ 16388, 0x0000 },   /* R16388 - SOUNDSTAGE_ENABLES_1 */
	{ 16389, 0x0000 },   /* R16389 - SOUNDSTAGE_ENABLES_0 */

	{ 16896, 0x0002 },   /* R16896 - HDBASS_AI_1 */
	{ 16897, 0xBD12 },   /* R16897 - HDBASS_AI_0 */
	{ 16898, 0x007C },   /* R16898 - HDBASS_AR_1 */
	{ 16899, 0x586C },   /* R16899 - HDBASS_AR_0 */
	{ 16900, 0x0053 },   /* R16900 - HDBASS_B_1 */
	{ 16901, 0x8121 },   /* R16901 - HDBASS_B_0 */
	{ 16902, 0x003F },   /* R16902 - HDBASS_K_1 */
	{ 16903, 0x8BD8 },   /* R16903 - HDBASS_K_0 */
	{ 16904, 0x0032 },   /* R16904 - HDBASS_N1_1 */
	{ 16905, 0xF52D },   /* R16905 - HDBASS_N1_0 */
	{ 16906, 0x0065 },   /* R16906 - HDBASS_N2_1 */
	{ 16907, 0xAC8C },   /* R16907 - HDBASS_N2_0 */
	{ 16908, 0x006B },   /* R16908 - HDBASS_N3_1 */
	{ 16909, 0xE087 },   /* R16909 - HDBASS_N3_0 */
	{ 16910, 0x0072 },   /* R16910 - HDBASS_N4_1 */
	{ 16911, 0x1483 },   /* R16911 - HDBASS_N4_0 */
	{ 16912, 0x0072 },   /* R16912 - HDBASS_N5_1 */
	{ 16913, 0x1483 },   /* R16913 - HDBASS_N5_0 */
	{ 16914, 0x0043 },   /* R16914 - HDBASS_X1_1 */
	{ 16915, 0x3525 },   /* R16915 - HDBASS_X1_0 */
	{ 16916, 0x0006 },   /* R16916 - HDBASS_X2_1 */
	{ 16917, 0x6A4A },   /* R16917 - HDBASS_X2_0 */
	{ 16918, 0x0043 },   /* R16918 - HDBASS_X3_1 */
	{ 16919, 0x6079 },   /* R16919 - HDBASS_X3_0 */
	{ 16920, 0x0008 },   /* R16920 - HDBASS_ATK_1 */
	{ 16921, 0x0000 },   /* R16921 - HDBASS_ATK_0 */
	{ 16922, 0x0001 },   /* R16922 - HDBASS_DCY_1 */
	{ 16923, 0x0000 },   /* R16923 - HDBASS_DCY_0 */
	{ 16924, 0x0059 },   /* R16924 - HDBASS_PG_1 */
	{ 16925, 0x999A },   /* R16925 - HDBASS_PG_0 */

	{ 17408, 0x0083 },   /* R17408 - HPF_C_1 */
	{ 17409, 0x98AD },   /* R17409 - HPF_C_0 */

	{ 17920, 0x007F },   /* R17920 - ADCL_RETUNE_C1_1 */
	{ 17921, 0xFFFF },   /* R17921 - ADCL_RETUNE_C1_0 */
	{ 17922, 0x0000 },   /* R17922 - ADCL_RETUNE_C2_1 */
	{ 17923, 0x0000 },   /* R17923 - ADCL_RETUNE_C2_0 */
	{ 17924, 0x0000 },   /* R17924 - ADCL_RETUNE_C3_1 */
	{ 17925, 0x0000 },   /* R17925 - ADCL_RETUNE_C3_0 */
	{ 17926, 0x0000 },   /* R17926 - ADCL_RETUNE_C4_1 */
	{ 17927, 0x0000 },   /* R17927 - ADCL_RETUNE_C4_0 */
	{ 17928, 0x0000 },   /* R17928 - ADCL_RETUNE_C5_1 */
	{ 17929, 0x0000 },   /* R17929 - ADCL_RETUNE_C5_0 */
	{ 17930, 0x0000 },   /* R17930 - ADCL_RETUNE_C6_1 */
	{ 17931, 0x0000 },   /* R17931 - ADCL_RETUNE_C6_0 */
	{ 17932, 0x0000 },   /* R17932 - ADCL_RETUNE_C7_1 */
	{ 17933, 0x0000 },   /* R17933 - ADCL_RETUNE_C7_0 */
	{ 17934, 0x0000 },   /* R17934 - ADCL_RETUNE_C8_1 */
	{ 17935, 0x0000 },   /* R17935 - ADCL_RETUNE_C8_0 */
	{ 17936, 0x0000 },   /* R17936 - ADCL_RETUNE_C9_1 */
	{ 17937, 0x0000 },   /* R17937 - ADCL_RETUNE_C9_0 */
	{ 17938, 0x0000 },   /* R17938 - ADCL_RETUNE_C10_1 */
	{ 17939, 0x0000 },   /* R17939 - ADCL_RETUNE_C10_0 */
	{ 17940, 0x0000 },   /* R17940 - ADCL_RETUNE_C11_1 */
	{ 17941, 0x0000 },   /* R17941 - ADCL_RETUNE_C11_0 */
	{ 17942, 0x0000 },   /* R17942 - ADCL_RETUNE_C12_1 */
	{ 17943, 0x0000 },   /* R17943 - ADCL_RETUNE_C12_0 */
	{ 17944, 0x0000 },   /* R17944 - ADCL_RETUNE_C13_1 */
	{ 17945, 0x0000 },   /* R17945 - ADCL_RETUNE_C13_0 */
	{ 17946, 0x0000 },   /* R17946 - ADCL_RETUNE_C14_1 */
	{ 17947, 0x0000 },   /* R17947 - ADCL_RETUNE_C14_0 */
	{ 17948, 0x0000 },   /* R17948 - ADCL_RETUNE_C15_1 */
	{ 17949, 0x0000 },   /* R17949 - ADCL_RETUNE_C15_0 */
	{ 17950, 0x0000 },   /* R17950 - ADCL_RETUNE_C16_1 */
	{ 17951, 0x0000 },   /* R17951 - ADCL_RETUNE_C16_0 */
	{ 17952, 0x0000 },   /* R17952 - ADCL_RETUNE_C17_1 */
	{ 17953, 0x0000 },   /* R17953 - ADCL_RETUNE_C17_0 */
	{ 17954, 0x0000 },   /* R17954 - ADCL_RETUNE_C18_1 */
	{ 17955, 0x0000 },   /* R17955 - ADCL_RETUNE_C18_0 */
	{ 17956, 0x0000 },   /* R17956 - ADCL_RETUNE_C19_1 */
	{ 17957, 0x0000 },   /* R17957 - ADCL_RETUNE_C19_0 */
	{ 17958, 0x0000 },   /* R17958 - ADCL_RETUNE_C20_1 */
	{ 17959, 0x0000 },   /* R17959 - ADCL_RETUNE_C20_0 */
	{ 17960, 0x0000 },   /* R17960 - ADCL_RETUNE_C21_1 */
	{ 17961, 0x0000 },   /* R17961 - ADCL_RETUNE_C21_0 */
	{ 17962, 0x0000 },   /* R17962 - ADCL_RETUNE_C22_1 */
	{ 17963, 0x0000 },   /* R17963 - ADCL_RETUNE_C22_0 */
	{ 17964, 0x0000 },   /* R17964 - ADCL_RETUNE_C23_1 */
	{ 17965, 0x0000 },   /* R17965 - ADCL_RETUNE_C23_0 */
	{ 17966, 0x0000 },   /* R17966 - ADCL_RETUNE_C24_1 */
	{ 17967, 0x0000 },   /* R17967 - ADCL_RETUNE_C24_0 */
	{ 17968, 0x0000 },   /* R17968 - ADCL_RETUNE_C25_1 */
	{ 17969, 0x0000 },   /* R17969 - ADCL_RETUNE_C25_0 */
	{ 17970, 0x0000 },   /* R17970 - ADCL_RETUNE_C26_1 */
	{ 17971, 0x0000 },   /* R17971 - ADCL_RETUNE_C26_0 */
	{ 17972, 0x0000 },   /* R17972 - ADCL_RETUNE_C27_1 */
	{ 17973, 0x0000 },   /* R17973 - ADCL_RETUNE_C27_0 */
	{ 17974, 0x0000 },   /* R17974 - ADCL_RETUNE_C28_1 */
	{ 17975, 0x0000 },   /* R17975 - ADCL_RETUNE_C28_0 */
	{ 17976, 0x0000 },   /* R17976 - ADCL_RETUNE_C29_1 */
	{ 17977, 0x0000 },   /* R17977 - ADCL_RETUNE_C29_0 */
	{ 17978, 0x0000 },   /* R17978 - ADCL_RETUNE_C30_1 */
	{ 17979, 0x0000 },   /* R17979 - ADCL_RETUNE_C30_0 */
	{ 17980, 0x0000 },   /* R17980 - ADCL_RETUNE_C31_1 */
	{ 17981, 0x0000 },   /* R17981 - ADCL_RETUNE_C31_0 */
	{ 17982, 0x0000 },   /* R17982 - ADCL_RETUNE_C32_1 */
	{ 17983, 0x0000 },   /* R17983 - ADCL_RETUNE_C32_0 */

	{ 18432, 0x0020 },   /* R18432 - RETUNEADC_PG2_1 */
	{ 18433, 0x0000 },   /* R18433 - RETUNEADC_PG2_0 */
	{ 18434, 0x0040 },   /* R18434 - RETUNEADC_PG_1 */
	{ 18435, 0x0000 },   /* R18435 - RETUNEADC_PG_0 */

	{ 18944, 0x007F },   /* R18944 - ADCR_RETUNE_C1_1 */
	{ 18945, 0xFFFF },   /* R18945 - ADCR_RETUNE_C1_0 */
	{ 18946, 0x0000 },   /* R18946 - ADCR_RETUNE_C2_1 */
	{ 18947, 0x0000 },   /* R18947 - ADCR_RETUNE_C2_0 */
	{ 18948, 0x0000 },   /* R18948 - ADCR_RETUNE_C3_1 */
	{ 18949, 0x0000 },   /* R18949 - ADCR_RETUNE_C3_0 */
	{ 18950, 0x0000 },   /* R18950 - ADCR_RETUNE_C4_1 */
	{ 18951, 0x0000 },   /* R18951 - ADCR_RETUNE_C4_0 */
	{ 18952, 0x0000 },   /* R18952 - ADCR_RETUNE_C5_1 */
	{ 18953, 0x0000 },   /* R18953 - ADCR_RETUNE_C5_0 */
	{ 18954, 0x0000 },   /* R18954 - ADCR_RETUNE_C6_1 */
	{ 18955, 0x0000 },   /* R18955 - ADCR_RETUNE_C6_0 */
	{ 18956, 0x0000 },   /* R18956 - ADCR_RETUNE_C7_1 */
	{ 18957, 0x0000 },   /* R18957 - ADCR_RETUNE_C7_0 */
	{ 18958, 0x0000 },   /* R18958 - ADCR_RETUNE_C8_1 */
	{ 18959, 0x0000 },   /* R18959 - ADCR_RETUNE_C8_0 */
	{ 18960, 0x0000 },   /* R18960 - ADCR_RETUNE_C9_1 */
	{ 18961, 0x0000 },   /* R18961 - ADCR_RETUNE_C9_0 */
	{ 18962, 0x0000 },   /* R18962 - ADCR_RETUNE_C10_1 */
	{ 18963, 0x0000 },   /* R18963 - ADCR_RETUNE_C10_0 */
	{ 18964, 0x0000 },   /* R18964 - ADCR_RETUNE_C11_1 */
	{ 18965, 0x0000 },   /* R18965 - ADCR_RETUNE_C11_0 */
	{ 18966, 0x0000 },   /* R18966 - ADCR_RETUNE_C12_1 */
	{ 18967, 0x0000 },   /* R18967 - ADCR_RETUNE_C12_0 */
	{ 18968, 0x0000 },   /* R18968 - ADCR_RETUNE_C13_1 */
	{ 18969, 0x0000 },   /* R18969 - ADCR_RETUNE_C13_0 */
	{ 18970, 0x0000 },   /* R18970 - ADCR_RETUNE_C14_1 */
	{ 18971, 0x0000 },   /* R18971 - ADCR_RETUNE_C14_0 */
	{ 18972, 0x0000 },   /* R18972 - ADCR_RETUNE_C15_1 */
	{ 18973, 0x0000 },   /* R18973 - ADCR_RETUNE_C15_0 */
	{ 18974, 0x0000 },   /* R18974 - ADCR_RETUNE_C16_1 */
	{ 18975, 0x0000 },   /* R18975 - ADCR_RETUNE_C16_0 */
	{ 18976, 0x0000 },   /* R18976 - ADCR_RETUNE_C17_1 */
	{ 18977, 0x0000 },   /* R18977 - ADCR_RETUNE_C17_0 */
	{ 18978, 0x0000 },   /* R18978 - ADCR_RETUNE_C18_1 */
	{ 18979, 0x0000 },   /* R18979 - ADCR_RETUNE_C18_0 */
	{ 18980, 0x0000 },   /* R18980 - ADCR_RETUNE_C19_1 */
	{ 18981, 0x0000 },   /* R18981 - ADCR_RETUNE_C19_0 */
	{ 18982, 0x0000 },   /* R18982 - ADCR_RETUNE_C20_1 */
	{ 18983, 0x0000 },   /* R18983 - ADCR_RETUNE_C20_0 */
	{ 18984, 0x0000 },   /* R18984 - ADCR_RETUNE_C21_1 */
	{ 18985, 0x0000 },   /* R18985 - ADCR_RETUNE_C21_0 */
	{ 18986, 0x0000 },   /* R18986 - ADCR_RETUNE_C22_1 */
	{ 18987, 0x0000 },   /* R18987 - ADCR_RETUNE_C22_0 */
	{ 18988, 0x0000 },   /* R18988 - ADCR_RETUNE_C23_1 */
	{ 18989, 0x0000 },   /* R18989 - ADCR_RETUNE_C23_0 */
	{ 18990, 0x0000 },   /* R18990 - ADCR_RETUNE_C24_1 */
	{ 18991, 0x0000 },   /* R18991 - ADCR_RETUNE_C24_0 */
	{ 18992, 0x0000 },   /* R18992 - ADCR_RETUNE_C25_1 */
	{ 18993, 0x0000 },   /* R18993 - ADCR_RETUNE_C25_0 */
	{ 18994, 0x0000 },   /* R18994 - ADCR_RETUNE_C26_1 */
	{ 18995, 0x0000 },   /* R18995 - ADCR_RETUNE_C26_0 */
	{ 18996, 0x0000 },   /* R18996 - ADCR_RETUNE_C27_1 */
	{ 18997, 0x0000 },   /* R18997 - ADCR_RETUNE_C27_0 */
	{ 18998, 0x0000 },   /* R18998 - ADCR_RETUNE_C28_1 */
	{ 18999, 0x0000 },   /* R18999 - ADCR_RETUNE_C28_0 */
	{ 19000, 0x0000 },   /* R19000 - ADCR_RETUNE_C29_1 */
	{ 19001, 0x0000 },   /* R19001 - ADCR_RETUNE_C29_0 */
	{ 19002, 0x0000 },   /* R19002 - ADCR_RETUNE_C30_1 */
	{ 19003, 0x0000 },   /* R19003 - ADCR_RETUNE_C30_0 */
	{ 19004, 0x0000 },   /* R19004 - ADCR_RETUNE_C31_1 */
	{ 19005, 0x0000 },   /* R19005 - ADCR_RETUNE_C31_0 */
	{ 19006, 0x0000 },   /* R19006 - ADCR_RETUNE_C32_1 */
	{ 19007, 0x0000 },   /* R19007 - ADCR_RETUNE_C32_0 */

	{ 19456, 0x007F },   /* R19456 - DACL_RETUNE_C1_1 */
	{ 19457, 0xFFFF },   /* R19457 - DACL_RETUNE_C1_0 */
	{ 19458, 0x0000 },   /* R19458 - DACL_RETUNE_C2_1 */
	{ 19459, 0x0000 },   /* R19459 - DACL_RETUNE_C2_0 */
	{ 19460, 0x0000 },   /* R19460 - DACL_RETUNE_C3_1 */
	{ 19461, 0x0000 },   /* R19461 - DACL_RETUNE_C3_0 */
	{ 19462, 0x0000 },   /* R19462 - DACL_RETUNE_C4_1 */
	{ 19463, 0x0000 },   /* R19463 - DACL_RETUNE_C4_0 */
	{ 19464, 0x0000 },   /* R19464 - DACL_RETUNE_C5_1 */
	{ 19465, 0x0000 },   /* R19465 - DACL_RETUNE_C5_0 */
	{ 19466, 0x0000 },   /* R19466 - DACL_RETUNE_C6_1 */
	{ 19467, 0x0000 },   /* R19467 - DACL_RETUNE_C6_0 */
	{ 19468, 0x0000 },   /* R19468 - DACL_RETUNE_C7_1 */
	{ 19469, 0x0000 },   /* R19469 - DACL_RETUNE_C7_0 */
	{ 19470, 0x0000 },   /* R19470 - DACL_RETUNE_C8_1 */
	{ 19471, 0x0000 },   /* R19471 - DACL_RETUNE_C8_0 */
	{ 19472, 0x0000 },   /* R19472 - DACL_RETUNE_C9_1 */
	{ 19473, 0x0000 },   /* R19473 - DACL_RETUNE_C9_0 */
	{ 19474, 0x0000 },   /* R19474 - DACL_RETUNE_C10_1 */
	{ 19475, 0x0000 },   /* R19475 - DACL_RETUNE_C10_0 */
	{ 19476, 0x0000 },   /* R19476 - DACL_RETUNE_C11_1 */
	{ 19477, 0x0000 },   /* R19477 - DACL_RETUNE_C11_0 */
	{ 19478, 0x0000 },   /* R19478 - DACL_RETUNE_C12_1 */
	{ 19479, 0x0000 },   /* R19479 - DACL_RETUNE_C12_0 */
	{ 19480, 0x0000 },   /* R19480 - DACL_RETUNE_C13_1 */
	{ 19481, 0x0000 },   /* R19481 - DACL_RETUNE_C13_0 */
	{ 19482, 0x0000 },   /* R19482 - DACL_RETUNE_C14_1 */
	{ 19483, 0x0000 },   /* R19483 - DACL_RETUNE_C14_0 */
	{ 19484, 0x0000 },   /* R19484 - DACL_RETUNE_C15_1 */
	{ 19485, 0x0000 },   /* R19485 - DACL_RETUNE_C15_0 */
	{ 19486, 0x0000 },   /* R19486 - DACL_RETUNE_C16_1 */
	{ 19487, 0x0000 },   /* R19487 - DACL_RETUNE_C16_0 */
	{ 19488, 0x0000 },   /* R19488 - DACL_RETUNE_C17_1 */
	{ 19489, 0x0000 },   /* R19489 - DACL_RETUNE_C17_0 */
	{ 19490, 0x0000 },   /* R19490 - DACL_RETUNE_C18_1 */
	{ 19491, 0x0000 },   /* R19491 - DACL_RETUNE_C18_0 */
	{ 19492, 0x0000 },   /* R19492 - DACL_RETUNE_C19_1 */
	{ 19493, 0x0000 },   /* R19493 - DACL_RETUNE_C19_0 */
	{ 19494, 0x0000 },   /* R19494 - DACL_RETUNE_C20_1 */
	{ 19495, 0x0000 },   /* R19495 - DACL_RETUNE_C20_0 */
	{ 19496, 0x0000 },   /* R19496 - DACL_RETUNE_C21_1 */
	{ 19497, 0x0000 },   /* R19497 - DACL_RETUNE_C21_0 */
	{ 19498, 0x0000 },   /* R19498 - DACL_RETUNE_C22_1 */
	{ 19499, 0x0000 },   /* R19499 - DACL_RETUNE_C22_0 */
	{ 19500, 0x0000 },   /* R19500 - DACL_RETUNE_C23_1 */
	{ 19501, 0x0000 },   /* R19501 - DACL_RETUNE_C23_0 */
	{ 19502, 0x0000 },   /* R19502 - DACL_RETUNE_C24_1 */
	{ 19503, 0x0000 },   /* R19503 - DACL_RETUNE_C24_0 */
	{ 19504, 0x0000 },   /* R19504 - DACL_RETUNE_C25_1 */
	{ 19505, 0x0000 },   /* R19505 - DACL_RETUNE_C25_0 */
	{ 19506, 0x0000 },   /* R19506 - DACL_RETUNE_C26_1 */
	{ 19507, 0x0000 },   /* R19507 - DACL_RETUNE_C26_0 */
	{ 19508, 0x0000 },   /* R19508 - DACL_RETUNE_C27_1 */
	{ 19509, 0x0000 },   /* R19509 - DACL_RETUNE_C27_0 */
	{ 19510, 0x0000 },   /* R19510 - DACL_RETUNE_C28_1 */
	{ 19511, 0x0000 },   /* R19511 - DACL_RETUNE_C28_0 */
	{ 19512, 0x0000 },   /* R19512 - DACL_RETUNE_C29_1 */
	{ 19513, 0x0000 },   /* R19513 - DACL_RETUNE_C29_0 */
	{ 19514, 0x0000 },   /* R19514 - DACL_RETUNE_C30_1 */
	{ 19515, 0x0000 },   /* R19515 - DACL_RETUNE_C30_0 */
	{ 19516, 0x0000 },   /* R19516 - DACL_RETUNE_C31_1 */
	{ 19517, 0x0000 },   /* R19517 - DACL_RETUNE_C31_0 */
	{ 19518, 0x0000 },   /* R19518 - DACL_RETUNE_C32_1 */
	{ 19519, 0x0000 },   /* R19519 - DACL_RETUNE_C32_0 */

	{ 19968, 0x0020 },   /* R19968 - RETUNEDAC_PG2_1 */
	{ 19969, 0x0000 },   /* R19969 - RETUNEDAC_PG2_0 */
	{ 19970, 0x0040 },   /* R19970 - RETUNEDAC_PG_1 */
	{ 19971, 0x0000 },   /* R19971 - RETUNEDAC_PG_0 */

	{ 20480, 0x007F },   /* R20480 - DACR_RETUNE_C1_1 */
	{ 20481, 0xFFFF },   /* R20481 - DACR_RETUNE_C1_0 */
	{ 20482, 0x0000 },   /* R20482 - DACR_RETUNE_C2_1 */
	{ 20483, 0x0000 },   /* R20483 - DACR_RETUNE_C2_0 */
	{ 20484, 0x0000 },   /* R20484 - DACR_RETUNE_C3_1 */
	{ 20485, 0x0000 },   /* R20485 - DACR_RETUNE_C3_0 */
	{ 20486, 0x0000 },   /* R20486 - DACR_RETUNE_C4_1 */
	{ 20487, 0x0000 },   /* R20487 - DACR_RETUNE_C4_0 */
	{ 20488, 0x0000 },   /* R20488 - DACR_RETUNE_C5_1 */
	{ 20489, 0x0000 },   /* R20489 - DACR_RETUNE_C5_0 */
	{ 20490, 0x0000 },   /* R20490 - DACR_RETUNE_C6_1 */
	{ 20491, 0x0000 },   /* R20491 - DACR_RETUNE_C6_0 */
	{ 20492, 0x0000 },   /* R20492 - DACR_RETUNE_C7_1 */
	{ 20493, 0x0000 },   /* R20493 - DACR_RETUNE_C7_0 */
	{ 20494, 0x0000 },   /* R20494 - DACR_RETUNE_C8_1 */
	{ 20495, 0x0000 },   /* R20495 - DACR_RETUNE_C8_0 */
	{ 20496, 0x0000 },   /* R20496 - DACR_RETUNE_C9_1 */
	{ 20497, 0x0000 },   /* R20497 - DACR_RETUNE_C9_0 */
	{ 20498, 0x0000 },   /* R20498 - DACR_RETUNE_C10_1 */
	{ 20499, 0x0000 },   /* R20499 - DACR_RETUNE_C10_0 */
	{ 20500, 0x0000 },   /* R20500 - DACR_RETUNE_C11_1 */
	{ 20501, 0x0000 },   /* R20501 - DACR_RETUNE_C11_0 */
	{ 20502, 0x0000 },   /* R20502 - DACR_RETUNE_C12_1 */
	{ 20503, 0x0000 },   /* R20503 - DACR_RETUNE_C12_0 */
	{ 20504, 0x0000 },   /* R20504 - DACR_RETUNE_C13_1 */
	{ 20505, 0x0000 },   /* R20505 - DACR_RETUNE_C13_0 */
	{ 20506, 0x0000 },   /* R20506 - DACR_RETUNE_C14_1 */
	{ 20507, 0x0000 },   /* R20507 - DACR_RETUNE_C14_0 */
	{ 20508, 0x0000 },   /* R20508 - DACR_RETUNE_C15_1 */
	{ 20509, 0x0000 },   /* R20509 - DACR_RETUNE_C15_0 */
	{ 20510, 0x0000 },   /* R20510 - DACR_RETUNE_C16_1 */
	{ 20511, 0x0000 },   /* R20511 - DACR_RETUNE_C16_0 */
	{ 20512, 0x0000 },   /* R20512 - DACR_RETUNE_C17_1 */
	{ 20513, 0x0000 },   /* R20513 - DACR_RETUNE_C17_0 */
	{ 20514, 0x0000 },   /* R20514 - DACR_RETUNE_C18_1 */
	{ 20515, 0x0000 },   /* R20515 - DACR_RETUNE_C18_0 */
	{ 20516, 0x0000 },   /* R20516 - DACR_RETUNE_C19_1 */
	{ 20517, 0x0000 },   /* R20517 - DACR_RETUNE_C19_0 */
	{ 20518, 0x0000 },   /* R20518 - DACR_RETUNE_C20_1 */
	{ 20519, 0x0000 },   /* R20519 - DACR_RETUNE_C20_0 */
	{ 20520, 0x0000 },   /* R20520 - DACR_RETUNE_C21_1 */
	{ 20521, 0x0000 },   /* R20521 - DACR_RETUNE_C21_0 */
	{ 20522, 0x0000 },   /* R20522 - DACR_RETUNE_C22_1 */
	{ 20523, 0x0000 },   /* R20523 - DACR_RETUNE_C22_0 */
	{ 20524, 0x0000 },   /* R20524 - DACR_RETUNE_C23_1 */
	{ 20525, 0x0000 },   /* R20525 - DACR_RETUNE_C23_0 */
	{ 20526, 0x0000 },   /* R20526 - DACR_RETUNE_C24_1 */
	{ 20527, 0x0000 },   /* R20527 - DACR_RETUNE_C24_0 */
	{ 20528, 0x0000 },   /* R20528 - DACR_RETUNE_C25_1 */
	{ 20529, 0x0000 },   /* R20529 - DACR_RETUNE_C25_0 */
	{ 20530, 0x0000 },   /* R20530 - DACR_RETUNE_C26_1 */
	{ 20531, 0x0000 },   /* R20531 - DACR_RETUNE_C26_0 */
	{ 20532, 0x0000 },   /* R20532 - DACR_RETUNE_C27_1 */
	{ 20533, 0x0000 },   /* R20533 - DACR_RETUNE_C27_0 */
	{ 20534, 0x0000 },   /* R20534 - DACR_RETUNE_C28_1 */
	{ 20535, 0x0000 },   /* R20535 - DACR_RETUNE_C28_0 */
	{ 20536, 0x0000 },   /* R20536 - DACR_RETUNE_C29_1 */
	{ 20537, 0x0000 },   /* R20537 - DACR_RETUNE_C29_0 */
	{ 20538, 0x0000 },   /* R20538 - DACR_RETUNE_C30_1 */
	{ 20539, 0x0000 },   /* R20539 - DACR_RETUNE_C30_0 */
	{ 20540, 0x0000 },   /* R20540 - DACR_RETUNE_C31_1 */
	{ 20541, 0x0000 },   /* R20541 - DACR_RETUNE_C31_0 */
	{ 20542, 0x0000 },   /* R20542 - DACR_RETUNE_C32_1 */
	{ 20543, 0x0000 },   /* R20543 - DACR_RETUNE_C32_0 */

	{ 20992, 0x008C },   /* R20992 - VSS_XHD2_1 */
	{ 20993, 0x0200 },   /* R20993 - VSS_XHD2_0 */
	{ 20994, 0x0035 },   /* R20994 - VSS_XHD3_1 */
	{ 20995, 0x0700 },   /* R20995 - VSS_XHD3_0 */
	{ 20996, 0x003A },   /* R20996 - VSS_XHN1_1 */
	{ 20997, 0x4100 },   /* R20997 - VSS_XHN1_0 */
	{ 20998, 0x008B },   /* R20998 - VSS_XHN2_1 */
	{ 20999, 0x7D00 },   /* R20999 - VSS_XHN2_0 */
	{ 21000, 0x003A },   /* R21000 - VSS_XHN3_1 */
	{ 21001, 0x4100 },   /* R21001 - VSS_XHN3_0 */
	{ 21002, 0x008C },   /* R21002 - VSS_XLA_1 */
	{ 21003, 0xFEE8 },   /* R21003 - VSS_XLA_0 */
	{ 21004, 0x0078 },   /* R21004 - VSS_XLB_1 */
	{ 21005, 0x0000 },   /* R21005 - VSS_XLB_0 */
	{ 21006, 0x003F },   /* R21006 - VSS_XLG_1 */
	{ 21007, 0xB260 },   /* R21007 - VSS_XLG_0 */
	{ 21008, 0x002D },   /* R21008 - VSS_PG2_1 */
	{ 21009, 0x1818 },   /* R21009 - VSS_PG2_0 */
	{ 21010, 0x0020 },   /* R21010 - VSS_PG_1 */
	{ 21011, 0x0000 },   /* R21011 - VSS_PG_0 */
	{ 21012, 0x00F1 },   /* R21012 - VSS_XTD1_1 */
	{ 21013, 0x8340 },   /* R21013 - VSS_XTD1_0 */
	{ 21014, 0x00FB },   /* R21014 - VSS_XTD2_1 */
	{ 21015, 0x8300 },   /* R21015 - VSS_XTD2_0 */
	{ 21016, 0x00EE },   /* R21016 - VSS_XTD3_1 */
	{ 21017, 0xAEC0 },   /* R21017 - VSS_XTD3_0 */
	{ 21018, 0x00FB },   /* R21018 - VSS_XTD4_1 */
	{ 21019, 0xAC40 },   /* R21019 - VSS_XTD4_0 */
	{ 21020, 0x00F1 },   /* R21020 - VSS_XTD5_1 */
	{ 21021, 0x7F80 },   /* R21021 - VSS_XTD5_0 */
	{ 21022, 0x00F4 },   /* R21022 - VSS_XTD6_1 */
	{ 21023, 0x3B40 },   /* R21023 - VSS_XTD6_0 */
	{ 21024, 0x00F5 },   /* R21024 - VSS_XTD7_1 */
	{ 21025, 0xFB00 },   /* R21025 - VSS_XTD7_0 */
	{ 21026, 0x00EA },   /* R21026 - VSS_XTD8_1 */
	{ 21027, 0x10C0 },   /* R21027 - VSS_XTD8_0 */
	{ 21028, 0x00FC },   /* R21028 - VSS_XTD9_1 */
	{ 21029, 0xC580 },   /* R21029 - VSS_XTD9_0 */
	{ 21030, 0x00E2 },   /* R21030 - VSS_XTD10_1 */
	{ 21031, 0x75C0 },   /* R21031 - VSS_XTD10_0 */
	{ 21032, 0x0004 },   /* R21032 - VSS_XTD11_1 */
	{ 21033, 0xB480 },   /* R21033 - VSS_XTD11_0 */
	{ 21034, 0x00D4 },   /* R21034 - VSS_XTD12_1 */
	{ 21035, 0xF980 },   /* R21035 - VSS_XTD12_0 */
	{ 21036, 0x0004 },   /* R21036 - VSS_XTD13_1 */
	{ 21037, 0x9140 },   /* R21037 - VSS_XTD13_0 */
	{ 21038, 0x00D8 },   /* R21038 - VSS_XTD14_1 */
	{ 21039, 0xA480 },   /* R21039 - VSS_XTD14_0 */
	{ 21040, 0x0002 },   /* R21040 - VSS_XTD15_1 */
	{ 21041, 0x3DC0 },   /* R21041 - VSS_XTD15_0 */
	{ 21042, 0x00CF },   /* R21042 - VSS_XTD16_1 */
	{ 21043, 0x7A80 },   /* R21043 - VSS_XTD16_0 */
	{ 21044, 0x00DC },   /* R21044 - VSS_XTD17_1 */
	{ 21045, 0x0600 },   /* R21045 - VSS_XTD17_0 */
	{ 21046, 0x00F2 },   /* R21046 - VSS_XTD18_1 */
	{ 21047, 0xDAC0 },   /* R21047 - VSS_XTD18_0 */
	{ 21048, 0x00BA },   /* R21048 - VSS_XTD19_1 */
	{ 21049, 0xF340 },   /* R21049 - VSS_XTD19_0 */
	{ 21050, 0x000A },   /* R21050 - VSS_XTD20_1 */
	{ 21051, 0x7940 },   /* R21051 - VSS_XTD20_0 */
	{ 21052, 0x001C },   /* R21052 - VSS_XTD21_1 */
	{ 21053, 0x0680 },   /* R21053 - VSS_XTD21_0 */
	{ 21054, 0x00FD },   /* R21054 - VSS_XTD22_1 */
	{ 21055, 0x2D00 },   /* R21055 - VSS_XTD22_0 */
	{ 21056, 0x001C },   /* R21056 - VSS_XTD23_1 */
	{ 21057, 0xE840 },   /* R21057 - VSS_XTD23_0 */
	{ 21058, 0x000D },   /* R21058 - VSS_XTD24_1 */
	{ 21059, 0xDC40 },   /* R21059 - VSS_XTD24_0 */
	{ 21060, 0x00FC },   /* R21060 - VSS_XTD25_1 */
	{ 21061, 0x9D00 },   /* R21061 - VSS_XTD25_0 */
	{ 21062, 0x0009 },   /* R21062 - VSS_XTD26_1 */
	{ 21063, 0x5580 },   /* R21063 - VSS_XTD26_0 */
	{ 21064, 0x00FE },   /* R21064 - VSS_XTD27_1 */
	{ 21065, 0x7E80 },   /* R21065 - VSS_XTD27_0 */
	{ 21066, 0x000E },   /* R21066 - VSS_XTD28_1 */
	{ 21067, 0xAB40 },   /* R21067 - VSS_XTD28_0 */
	{ 21068, 0x00F9 },   /* R21068 - VSS_XTD29_1 */
	{ 21069, 0x9880 },   /* R21069 - VSS_XTD29_0 */
	{ 21070, 0x0009 },   /* R21070 - VSS_XTD30_1 */
	{ 21071, 0x87C0 },   /* R21071 - VSS_XTD30_0 */
	{ 21072, 0x00FD },   /* R21072 - VSS_XTD31_1 */
	{ 21073, 0x2C40 },   /* R21073 - VSS_XTD31_0 */
	{ 21074, 0x0009 },   /* R21074 - VSS_XTD32_1 */
	{ 21075, 0x4800 },   /* R21075 - VSS_XTD32_0 */
	{ 21076, 0x0003 },   /* R21076 - VSS_XTS1_1 */
	{ 21077, 0x5F40 },   /* R21077 - VSS_XTS1_0 */
	{ 21078, 0x0000 },   /* R21078 - VSS_XTS2_1 */
	{ 21079, 0x8700 },   /* R21079 - VSS_XTS2_0 */
	{ 21080, 0x00FA },   /* R21080 - VSS_XTS3_1 */
	{ 21081, 0xE4C0 },   /* R21081 - VSS_XTS3_0 */
	{ 21082, 0x0000 },   /* R21082 - VSS_XTS4_1 */
	{ 21083, 0x0B40 },   /* R21083 - VSS_XTS4_0 */
	{ 21084, 0x0004 },   /* R21084 - VSS_XTS5_1 */
	{ 21085, 0xE180 },   /* R21085 - VSS_XTS5_0 */
	{ 21086, 0x0001 },   /* R21086 - VSS_XTS6_1 */
	{ 21087, 0x1F40 },   /* R21087 - VSS_XTS6_0 */
	{ 21088, 0x00F8 },   /* R21088 - VSS_XTS7_1 */
	{ 21089, 0xB000 },   /* R21089 - VSS_XTS7_0 */
	{ 21090, 0x00FB },   /* R21090 - VSS_XTS8_1 */
	{ 21091, 0xCBC0 },   /* R21091 - VSS_XTS8_0 */
	{ 21092, 0x0004 },   /* R21092 - VSS_XTS9_1 */
	{ 21093, 0xF380 },   /* R21093 - VSS_XTS9_0 */
	{ 21094, 0x0007 },   /* R21094 - VSS_XTS10_1 */
	{ 21095, 0xDF40 },   /* R21095 - VSS_XTS10_0 */
	{ 21096, 0x00FF },   /* R21096 - VSS_XTS11_1 */
	{ 21097, 0x0700 },   /* R21097 - VSS_XTS11_0 */
	{ 21098, 0x00EF },   /* R21098 - VSS_XTS12_1 */
	{ 21099, 0xD700 },   /* R21099 - VSS_XTS12_0 */
	{ 21100, 0x00FB },   /* R21100 - VSS_XTS13_1 */
	{ 21101, 0xAF40 },   /* R21101 - VSS_XTS13_0 */
	{ 21102, 0x0010 },   /* R21102 - VSS_XTS14_1 */
	{ 21103, 0x8A80 },   /* R21103 - VSS_XTS14_0 */
	{ 21104, 0x0011 },   /* R21104 - VSS_XTS15_1 */
	{ 21105, 0x07C0 },   /* R21105 - VSS_XTS15_0 */
	{ 21106, 0x00E0 },   /* R21106 - VSS_XTS16_1 */
	{ 21107, 0x0800 },   /* R21107 - VSS_XTS16_0 */
	{ 21108, 0x00D2 },   /* R21108 - VSS_XTS17_1 */
	{ 21109, 0x7600 },   /* R21109 - VSS_XTS17_0 */
	{ 21110, 0x0020 },   /* R21110 - VSS_XTS18_1 */
	{ 21111, 0xCF40 },   /* R21111 - VSS_XTS18_0 */
	{ 21112, 0x0030 },   /* R21112 - VSS_XTS19_1 */
	{ 21113, 0x2340 },   /* R21113 - VSS_XTS19_0 */
	{ 21114, 0x00FD },   /* R21114 - VSS_XTS20_1 */
	{ 21115, 0x69C0 },   /* R21115 - VSS_XTS20_0 */
	{ 21116, 0x0028 },   /* R21116 - VSS_XTS21_1 */
	{ 21117, 0x3500 },   /* R21117 - VSS_XTS21_0 */
	{ 21118, 0x0006 },   /* R21118 - VSS_XTS22_1 */
	{ 21119, 0x3300 },   /* R21119 - VSS_XTS22_0 */
	{ 21120, 0x00D9 },   /* R21120 - VSS_XTS23_1 */
	{ 21121, 0xF6C0 },   /* R21121 - VSS_XTS23_0 */
	{ 21122, 0x00F3 },   /* R21122 - VSS_XTS24_1 */
	{ 21123, 0x3340 },   /* R21123 - VSS_XTS24_0 */
	{ 21124, 0x000F },   /* R21124 - VSS_XTS25_1 */
	{ 21125, 0x4200 },   /* R21125 - VSS_XTS25_0 */
	{ 21126, 0x0004 },   /* R21126 - VSS_XTS26_1 */
	{ 21127, 0x0C80 },   /* R21127 - VSS_XTS26_0 */
	{ 21128, 0x00FB },   /* R21128 - VSS_XTS27_1 */
	{ 21129, 0x3F80 },   /* R21129 - VSS_XTS27_0 */
	{ 21130, 0x00F7 },   /* R21130 - VSS_XTS28_1 */
	{ 21131, 0x57C0 },   /* R21131 - VSS_XTS28_0 */
	{ 21132, 0x0003 },   /* R21132 - VSS_XTS29_1 */
	{ 21133, 0x5400 },   /* R21133 - VSS_XTS29_0 */
	{ 21134, 0x0000 },   /* R21134 - VSS_XTS30_1 */
	{ 21135, 0xC6C0 },   /* R21135 - VSS_XTS30_0 */
	{ 21136, 0x0003 },   /* R21136 - VSS_XTS31_1 */
	{ 21137, 0x12C0 },   /* R21137 - VSS_XTS31_0 */
	{ 21138, 0x00FD },   /* R21138 - VSS_XTS32_1 */
	{ 21139, 0x8580 },   /* R21139 - VSS_XTS32_0 */
};

#define CODEC_CACHEREGNUM (sizeof(wm8962_reg_cache)/sizeof(wm8962_reg_cache[0]))

static bool wm8962_readable_register(unsigned int reg)
{
	switch (reg) {
	case WM8962_REG_LEFT_INPUT_VOLUME:
	case WM8962_REG_RIGHT_INPUT_VOLUME:
	case WM8962_REG_HPOUTL_VOLUME:
	case WM8962_REG_HPOUTR_VOLUME:
	case WM8962_REG_CLOCKING1:
	case WM8962_REG_ADC_DAC_CONTROL_1:
	case WM8962_REG_ADC_DAC_CONTROL_2:
	case WM8962_REG_AUDIO_INTERFACE_0:
	case WM8962_REG_CLOCKING2:
	case WM8962_REG_AUDIO_INTERFACE_1:
	case WM8962_REG_LEFT_DAC_VOLUME:
	case WM8962_REG_RIGHT_DAC_VOLUME:
	case WM8962_REG_AUDIO_INTERFACE_2:
	case WM8962_REG_SOFTWARE_RESET:
	case WM8962_REG_ALC1:
	case WM8962_REG_ALC2:
	case WM8962_REG_ALC3:
	case WM8962_REG_NOISE_GATE:
	case WM8962_REG_LEFT_ADC_VOLUME:
	case WM8962_REG_RIGHT_ADC_VOLUME:
	case WM8962_REG_ADDITIONAL_CONTROL_1:
	case WM8962_REG_ADDITIONAL_CONTROL_2:
	case WM8962_REG_PWR_MGMT_1:
	case WM8962_REG_PWR_MGMT_2:
	case WM8962_REG_ADDITIONAL_CONTROL_3:
	case WM8962_REG_ANTI_POP:
	case WM8962_REG_CLOCKING_3:
	case WM8962_REG_INPUT_MIXER_CONTROL_1:
	case WM8962_REG_LEFT_INPUT_MIXER_VOLUME:
	case WM8962_REG_RIGHT_INPUT_MIXER_VOLUME:
	case WM8962_REG_INPUT_MIXER_CONTROL_2:
	case WM8962_REG_INPUT_BIAS_CONTROL:
	case WM8962_REG_LEFT_INPUT_PGA_CONTROL:
	case WM8962_REG_RIGHT_INPUT_PGA_CONTROL:
	case WM8962_REG_SPKOUTL_VOLUME:
	case WM8962_REG_SPKOUTR_VOLUME:
	case WM8962_REG_THERMAL_SHUTDOWN_STATUS:
	case WM8962_REG_ADDITIONAL_CONTROL_4:
	case WM8962_REG_CLASS_D_CONTROL_1:
	case WM8962_REG_CLASS_D_CONTROL_2:
	case WM8962_REG_CLOCKING_4:
	case WM8962_REG_DAC_DSP_MIXING_1:
	case WM8962_REG_DAC_DSP_MIXING_2:
	case WM8962_REG_DC_SERVO_0:
	case WM8962_REG_DC_SERVO_1:
	case WM8962_REG_DC_SERVO_4:
	case WM8962_REG_DC_SERVO_6:
	case WM8962_REG_ANALOGUE_PGA_BIAS:
	case WM8962_REG_ANALOGUE_HP_0:
	case WM8962_REG_ANALOGUE_HP_2:
	case WM8962_REG_CHARGE_PUMP_1:
	case WM8962_REG_CHARGE_PUMP_B:
	case WM8962_REG_WRITE_SEQUENCER_CONTROL_1:
	case WM8962_REG_WRITE_SEQUENCER_CONTROL_2:
	case WM8962_REG_WRITE_SEQUENCER_CONTROL_3:
	case WM8962_REG_CONTROL_INTERFACE:
	case WM8962_REG_MIXER_ENABLES:
	case WM8962_REG_HEADPHONE_MIXER_1:
	case WM8962_REG_HEADPHONE_MIXER_2:
	case WM8962_REG_HEADPHONE_MIXER_3:
	case WM8962_REG_HEADPHONE_MIXER_4:
	case WM8962_REG_SPEAKER_MIXER_1:
	case WM8962_REG_SPEAKER_MIXER_2:
	case WM8962_REG_SPEAKER_MIXER_3:
	case WM8962_REG_SPEAKER_MIXER_4:
	case WM8962_REG_SPEAKER_MIXER_5:
	case WM8962_REG_BEEP_GENERATOR_1:
	case WM8962_REG_OSCILLATOR_TRIM_3:
	case WM8962_REG_OSCILLATOR_TRIM_4:
	case WM8962_REG_OSCILLATOR_TRIM_7:
	case WM8962_REG_ANALOGUE_CLOCKING1:
	case WM8962_REG_ANALOGUE_CLOCKING2:
	case WM8962_REG_ANALOGUE_CLOCKING3:
	case WM8962_REG_PLL_SOFTWARE_RESET:
	case WM8962_REG_PLL2:
	case WM8962_REG_PLL_4:
	case WM8962_REG_PLL_9:
	case WM8962_REG_PLL_10:
	case WM8962_REG_PLL_11:
	case WM8962_REG_PLL_12:
	case WM8962_REG_PLL_13:
	case WM8962_REG_PLL_14:
	case WM8962_REG_PLL_15:
	case WM8962_REG_PLL_16:
	case WM8962_REG_FLL_CONTROL_1:
	case WM8962_REG_FLL_CONTROL_2:
	case WM8962_REG_FLL_CONTROL_3:
	case WM8962_REG_FLL_CONTROL_5:
	case WM8962_REG_FLL_CONTROL_6:
	case WM8962_REG_FLL_CONTROL_7:
	case WM8962_REG_FLL_CONTROL_8:
	case WM8962_REG_GENERAL_TEST_1:
	case WM8962_REG_DF1:
	case WM8962_REG_DF2:
	case WM8962_REG_DF3:
	case WM8962_REG_DF4:
	case WM8962_REG_DF5:
	case WM8962_REG_DF6:
	case WM8962_REG_DF7:
	case WM8962_REG_LHPF1:
	case WM8962_REG_LHPF2:
	case WM8962_REG_THREED1:
	case WM8962_REG_THREED2:
	case WM8962_REG_THREED3:
	case WM8962_REG_THREED4:
	case WM8962_REG_DRC_1:
	case WM8962_REG_DRC_2:
	case WM8962_REG_DRC_3:
	case WM8962_REG_DRC_4:
	case WM8962_REG_DRC_5:
	case WM8962_REG_TLOOPBACK:
	case WM8962_REG_EQ1:
	case WM8962_REG_EQ2:
	case WM8962_REG_EQ3:
	case WM8962_REG_EQ4:
	case WM8962_REG_EQ5:
	case WM8962_REG_EQ6:
	case WM8962_REG_EQ7:
	case WM8962_REG_EQ8:
	case WM8962_REG_EQ9:
	case WM8962_REG_EQ10:
	case WM8962_REG_EQ11:
	case WM8962_REG_EQ12:
	case WM8962_REG_EQ13:
	case WM8962_REG_EQ14:
	case WM8962_REG_EQ15:
	case WM8962_REG_EQ16:
	case WM8962_REG_EQ17:
	case WM8962_REG_EQ18:
	case WM8962_REG_EQ19:
	case WM8962_REG_EQ20:
	case WM8962_REG_EQ21:
	case WM8962_REG_EQ22:
	case WM8962_REG_EQ23:
	case WM8962_REG_EQ24:
	case WM8962_REG_EQ25:
	case WM8962_REG_EQ26:
	case WM8962_REG_EQ27:
	case WM8962_REG_EQ28:
	case WM8962_REG_EQ29:
	case WM8962_REG_EQ30:
	case WM8962_REG_EQ31:
	case WM8962_REG_EQ32:
	case WM8962_REG_EQ33:
	case WM8962_REG_EQ34:
	case WM8962_REG_EQ35:
	case WM8962_REG_EQ36:
	case WM8962_REG_EQ37:
	case WM8962_REG_EQ38:
	case WM8962_REG_EQ39:
	case WM8962_REG_EQ40:
	case WM8962_REG_EQ41:
	case WM8962_REG_GPIO_2:
	case WM8962_REG_GPIO_3:
	case WM8962_REG_GPIO_5:
	case WM8962_REG_GPIO_6:
	case WM8962_REG_INTERRUPT_STATUS_1:
	case WM8962_REG_INTERRUPT_STATUS_2:
	case WM8962_REG_INTERRUPT_STATUS_1_MASK:
	case WM8962_REG_INTERRUPT_STATUS_2_MASK:
	case WM8962_REG_INTERRUPT_CONTROL:
	case WM8962_REG_IRQ_DEBOUNCE:
	case WM8962_REG_MICINT_SOURCE_POL:
	case WM8962_REG_DSP2_POWER_MANAGEMENT:
	case WM8962_REG_DSP2_EXECCONTROL:
	case WM8962_REG_DSP2_INSTRUCTION_RAM_0:
	case WM8962_REG_DSP2_ADDRESS_RAM_2:
	case WM8962_REG_DSP2_ADDRESS_RAM_1:
	case WM8962_REG_DSP2_ADDRESS_RAM_0:
	case WM8962_REG_DSP2_DATA1_RAM_1:
	case WM8962_REG_DSP2_DATA1_RAM_0:
	case WM8962_REG_DSP2_DATA2_RAM_1:
	case WM8962_REG_DSP2_DATA2_RAM_0:
	case WM8962_REG_DSP2_DATA3_RAM_1:
	case WM8962_REG_DSP2_DATA3_RAM_0:
	case WM8962_REG_DSP2_COEFF_RAM_0:
	case WM8962_REG_RETUNEADC_SHARED_COEFF_1:
	case WM8962_REG_RETUNEADC_SHARED_COEFF_0:
	case WM8962_REG_RETUNEDAC_SHARED_COEFF_1:
	case WM8962_REG_RETUNEDAC_SHARED_COEFF_0:
	case WM8962_REG_SOUNDSTAGE_ENABLES_1:
	case WM8962_REG_SOUNDSTAGE_ENABLES_0:
	case WM8962_REG_HDBASS_AI_1:
	case WM8962_REG_HDBASS_AI_0:
	case WM8962_REG_HDBASS_AR_1:
	case WM8962_REG_HDBASS_AR_0:
	case WM8962_REG_HDBASS_B_1:
	case WM8962_REG_HDBASS_B_0:
	case WM8962_REG_HDBASS_K_1:
	case WM8962_REG_HDBASS_K_0:
	case WM8962_REG_HDBASS_N1_1:
	case WM8962_REG_HDBASS_N1_0:
	case WM8962_REG_HDBASS_N2_1:
	case WM8962_REG_HDBASS_N2_0:
	case WM8962_REG_HDBASS_N3_1:
	case WM8962_REG_HDBASS_N3_0:
	case WM8962_REG_HDBASS_N4_1:
	case WM8962_REG_HDBASS_N4_0:
	case WM8962_REG_HDBASS_N5_1:
	case WM8962_REG_HDBASS_N5_0:
	case WM8962_REG_HDBASS_X1_1:
	case WM8962_REG_HDBASS_X1_0:
	case WM8962_REG_HDBASS_X2_1:
	case WM8962_REG_HDBASS_X2_0:
	case WM8962_REG_HDBASS_X3_1:
	case WM8962_REG_HDBASS_X3_0:
	case WM8962_REG_HDBASS_ATK_1:
	case WM8962_REG_HDBASS_ATK_0:
	case WM8962_REG_HDBASS_DCY_1:
	case WM8962_REG_HDBASS_DCY_0:
	case WM8962_REG_HDBASS_PG_1:
	case WM8962_REG_HDBASS_PG_0:
	case WM8962_REG_HPF_C_1:
	case WM8962_REG_HPF_C_0:
	case WM8962_REG_ADCL_RETUNE_C1_1:
	case WM8962_REG_ADCL_RETUNE_C1_0:
	case WM8962_REG_ADCL_RETUNE_C2_1:
	case WM8962_REG_ADCL_RETUNE_C2_0:
	case WM8962_REG_ADCL_RETUNE_C3_1:
	case WM8962_REG_ADCL_RETUNE_C3_0:
	case WM8962_REG_ADCL_RETUNE_C4_1:
	case WM8962_REG_ADCL_RETUNE_C4_0:
	case WM8962_REG_ADCL_RETUNE_C5_1:
	case WM8962_REG_ADCL_RETUNE_C5_0:
	case WM8962_REG_ADCL_RETUNE_C6_1:
	case WM8962_REG_ADCL_RETUNE_C6_0:
	case WM8962_REG_ADCL_RETUNE_C7_1:
	case WM8962_REG_ADCL_RETUNE_C7_0:
	case WM8962_REG_ADCL_RETUNE_C8_1:
	case WM8962_REG_ADCL_RETUNE_C8_0:
	case WM8962_REG_ADCL_RETUNE_C9_1:
	case WM8962_REG_ADCL_RETUNE_C9_0:
	case WM8962_REG_ADCL_RETUNE_C10_1:
	case WM8962_REG_ADCL_RETUNE_C10_0:
	case WM8962_REG_ADCL_RETUNE_C11_1:
	case WM8962_REG_ADCL_RETUNE_C11_0:
	case WM8962_REG_ADCL_RETUNE_C12_1:
	case WM8962_REG_ADCL_RETUNE_C12_0:
	case WM8962_REG_ADCL_RETUNE_C13_1:
	case WM8962_REG_ADCL_RETUNE_C13_0:
	case WM8962_REG_ADCL_RETUNE_C14_1:
	case WM8962_REG_ADCL_RETUNE_C14_0:
	case WM8962_REG_ADCL_RETUNE_C15_1:
	case WM8962_REG_ADCL_RETUNE_C15_0:
	case WM8962_REG_ADCL_RETUNE_C16_1:
	case WM8962_REG_ADCL_RETUNE_C16_0:
	case WM8962_REG_ADCL_RETUNE_C17_1:
	case WM8962_REG_ADCL_RETUNE_C17_0:
	case WM8962_REG_ADCL_RETUNE_C18_1:
	case WM8962_REG_ADCL_RETUNE_C18_0:
	case WM8962_REG_ADCL_RETUNE_C19_1:
	case WM8962_REG_ADCL_RETUNE_C19_0:
	case WM8962_REG_ADCL_RETUNE_C20_1:
	case WM8962_REG_ADCL_RETUNE_C20_0:
	case WM8962_REG_ADCL_RETUNE_C21_1:
	case WM8962_REG_ADCL_RETUNE_C21_0:
	case WM8962_REG_ADCL_RETUNE_C22_1:
	case WM8962_REG_ADCL_RETUNE_C22_0:
	case WM8962_REG_ADCL_RETUNE_C23_1:
	case WM8962_REG_ADCL_RETUNE_C23_0:
	case WM8962_REG_ADCL_RETUNE_C24_1:
	case WM8962_REG_ADCL_RETUNE_C24_0:
	case WM8962_REG_ADCL_RETUNE_C25_1:
	case WM8962_REG_ADCL_RETUNE_C25_0:
	case WM8962_REG_ADCL_RETUNE_C26_1:
	case WM8962_REG_ADCL_RETUNE_C26_0:
	case WM8962_REG_ADCL_RETUNE_C27_1:
	case WM8962_REG_ADCL_RETUNE_C27_0:
	case WM8962_REG_ADCL_RETUNE_C28_1:
	case WM8962_REG_ADCL_RETUNE_C28_0:
	case WM8962_REG_ADCL_RETUNE_C29_1:
	case WM8962_REG_ADCL_RETUNE_C29_0:
	case WM8962_REG_ADCL_RETUNE_C30_1:
	case WM8962_REG_ADCL_RETUNE_C30_0:
	case WM8962_REG_ADCL_RETUNE_C31_1:
	case WM8962_REG_ADCL_RETUNE_C31_0:
	case WM8962_REG_ADCL_RETUNE_C32_1:
	case WM8962_REG_ADCL_RETUNE_C32_0:
	case WM8962_REG_RETUNEADC_PG2_1:
	case WM8962_REG_RETUNEADC_PG2_0:
	case WM8962_REG_RETUNEADC_PG_1:
	case WM8962_REG_RETUNEADC_PG_0:
	case WM8962_REG_ADCR_RETUNE_C1_1:
	case WM8962_REG_ADCR_RETUNE_C1_0:
	case WM8962_REG_ADCR_RETUNE_C2_1:
	case WM8962_REG_ADCR_RETUNE_C2_0:
	case WM8962_REG_ADCR_RETUNE_C3_1:
	case WM8962_REG_ADCR_RETUNE_C3_0:
	case WM8962_REG_ADCR_RETUNE_C4_1:
	case WM8962_REG_ADCR_RETUNE_C4_0:
	case WM8962_REG_ADCR_RETUNE_C5_1:
	case WM8962_REG_ADCR_RETUNE_C5_0:
	case WM8962_REG_ADCR_RETUNE_C6_1:
	case WM8962_REG_ADCR_RETUNE_C6_0:
	case WM8962_REG_ADCR_RETUNE_C7_1:
	case WM8962_REG_ADCR_RETUNE_C7_0:
	case WM8962_REG_ADCR_RETUNE_C8_1:
	case WM8962_REG_ADCR_RETUNE_C8_0:
	case WM8962_REG_ADCR_RETUNE_C9_1:
	case WM8962_REG_ADCR_RETUNE_C9_0:
	case WM8962_REG_ADCR_RETUNE_C10_1:
	case WM8962_REG_ADCR_RETUNE_C10_0:
	case WM8962_REG_ADCR_RETUNE_C11_1:
	case WM8962_REG_ADCR_RETUNE_C11_0:
	case WM8962_REG_ADCR_RETUNE_C12_1:
	case WM8962_REG_ADCR_RETUNE_C12_0:
	case WM8962_REG_ADCR_RETUNE_C13_1:
	case WM8962_REG_ADCR_RETUNE_C13_0:
	case WM8962_REG_ADCR_RETUNE_C14_1:
	case WM8962_REG_ADCR_RETUNE_C14_0:
	case WM8962_REG_ADCR_RETUNE_C15_1:
	case WM8962_REG_ADCR_RETUNE_C15_0:
	case WM8962_REG_ADCR_RETUNE_C16_1:
	case WM8962_REG_ADCR_RETUNE_C16_0:
	case WM8962_REG_ADCR_RETUNE_C17_1:
	case WM8962_REG_ADCR_RETUNE_C17_0:
	case WM8962_REG_ADCR_RETUNE_C18_1:
	case WM8962_REG_ADCR_RETUNE_C18_0:
	case WM8962_REG_ADCR_RETUNE_C19_1:
	case WM8962_REG_ADCR_RETUNE_C19_0:
	case WM8962_REG_ADCR_RETUNE_C20_1:
	case WM8962_REG_ADCR_RETUNE_C20_0:
	case WM8962_REG_ADCR_RETUNE_C21_1:
	case WM8962_REG_ADCR_RETUNE_C21_0:
	case WM8962_REG_ADCR_RETUNE_C22_1:
	case WM8962_REG_ADCR_RETUNE_C22_0:
	case WM8962_REG_ADCR_RETUNE_C23_1:
	case WM8962_REG_ADCR_RETUNE_C23_0:
	case WM8962_REG_ADCR_RETUNE_C24_1:
	case WM8962_REG_ADCR_RETUNE_C24_0:
	case WM8962_REG_ADCR_RETUNE_C25_1:
	case WM8962_REG_ADCR_RETUNE_C25_0:
	case WM8962_REG_ADCR_RETUNE_C26_1:
	case WM8962_REG_ADCR_RETUNE_C26_0:
	case WM8962_REG_ADCR_RETUNE_C27_1:
	case WM8962_REG_ADCR_RETUNE_C27_0:
	case WM8962_REG_ADCR_RETUNE_C28_1:
	case WM8962_REG_ADCR_RETUNE_C28_0:
	case WM8962_REG_ADCR_RETUNE_C29_1:
	case WM8962_REG_ADCR_RETUNE_C29_0:
	case WM8962_REG_ADCR_RETUNE_C30_1:
	case WM8962_REG_ADCR_RETUNE_C30_0:
	case WM8962_REG_ADCR_RETUNE_C31_1:
	case WM8962_REG_ADCR_RETUNE_C31_0:
	case WM8962_REG_ADCR_RETUNE_C32_1:
	case WM8962_REG_ADCR_RETUNE_C32_0:
	case WM8962_REG_DACL_RETUNE_C1_1:
	case WM8962_REG_DACL_RETUNE_C1_0:
	case WM8962_REG_DACL_RETUNE_C2_1:
	case WM8962_REG_DACL_RETUNE_C2_0:
	case WM8962_REG_DACL_RETUNE_C3_1:
	case WM8962_REG_DACL_RETUNE_C3_0:
	case WM8962_REG_DACL_RETUNE_C4_1:
	case WM8962_REG_DACL_RETUNE_C4_0:
	case WM8962_REG_DACL_RETUNE_C5_1:
	case WM8962_REG_DACL_RETUNE_C5_0:
	case WM8962_REG_DACL_RETUNE_C6_1:
	case WM8962_REG_DACL_RETUNE_C6_0:
	case WM8962_REG_DACL_RETUNE_C7_1:
	case WM8962_REG_DACL_RETUNE_C7_0:
	case WM8962_REG_DACL_RETUNE_C8_1:
	case WM8962_REG_DACL_RETUNE_C8_0:
	case WM8962_REG_DACL_RETUNE_C9_1:
	case WM8962_REG_DACL_RETUNE_C9_0:
	case WM8962_REG_DACL_RETUNE_C10_1:
	case WM8962_REG_DACL_RETUNE_C10_0:
	case WM8962_REG_DACL_RETUNE_C11_1:
	case WM8962_REG_DACL_RETUNE_C11_0:
	case WM8962_REG_DACL_RETUNE_C12_1:
	case WM8962_REG_DACL_RETUNE_C12_0:
	case WM8962_REG_DACL_RETUNE_C13_1:
	case WM8962_REG_DACL_RETUNE_C13_0:
	case WM8962_REG_DACL_RETUNE_C14_1:
	case WM8962_REG_DACL_RETUNE_C14_0:
	case WM8962_REG_DACL_RETUNE_C15_1:
	case WM8962_REG_DACL_RETUNE_C15_0:
	case WM8962_REG_DACL_RETUNE_C16_1:
	case WM8962_REG_DACL_RETUNE_C16_0:
	case WM8962_REG_DACL_RETUNE_C17_1:
	case WM8962_REG_DACL_RETUNE_C17_0:
	case WM8962_REG_DACL_RETUNE_C18_1:
	case WM8962_REG_DACL_RETUNE_C18_0:
	case WM8962_REG_DACL_RETUNE_C19_1:
	case WM8962_REG_DACL_RETUNE_C19_0:
	case WM8962_REG_DACL_RETUNE_C20_1:
	case WM8962_REG_DACL_RETUNE_C20_0:
	case WM8962_REG_DACL_RETUNE_C21_1:
	case WM8962_REG_DACL_RETUNE_C21_0:
	case WM8962_REG_DACL_RETUNE_C22_1:
	case WM8962_REG_DACL_RETUNE_C22_0:
	case WM8962_REG_DACL_RETUNE_C23_1:
	case WM8962_REG_DACL_RETUNE_C23_0:
	case WM8962_REG_DACL_RETUNE_C24_1:
	case WM8962_REG_DACL_RETUNE_C24_0:
	case WM8962_REG_DACL_RETUNE_C25_1:
	case WM8962_REG_DACL_RETUNE_C25_0:
	case WM8962_REG_DACL_RETUNE_C26_1:
	case WM8962_REG_DACL_RETUNE_C26_0:
	case WM8962_REG_DACL_RETUNE_C27_1:
	case WM8962_REG_DACL_RETUNE_C27_0:
	case WM8962_REG_DACL_RETUNE_C28_1:
	case WM8962_REG_DACL_RETUNE_C28_0:
	case WM8962_REG_DACL_RETUNE_C29_1:
	case WM8962_REG_DACL_RETUNE_C29_0:
	case WM8962_REG_DACL_RETUNE_C30_1:
	case WM8962_REG_DACL_RETUNE_C30_0:
	case WM8962_REG_DACL_RETUNE_C31_1:
	case WM8962_REG_DACL_RETUNE_C31_0:
	case WM8962_REG_DACL_RETUNE_C32_1:
	case WM8962_REG_DACL_RETUNE_C32_0:
	case WM8962_REG_RETUNEDAC_PG2_1:
	case WM8962_REG_RETUNEDAC_PG2_0:
	case WM8962_REG_RETUNEDAC_PG_1:
	case WM8962_REG_RETUNEDAC_PG_0:
	case WM8962_REG_DACR_RETUNE_C1_1:
	case WM8962_REG_DACR_RETUNE_C1_0:
	case WM8962_REG_DACR_RETUNE_C2_1:
	case WM8962_REG_DACR_RETUNE_C2_0:
	case WM8962_REG_DACR_RETUNE_C3_1:
	case WM8962_REG_DACR_RETUNE_C3_0:
	case WM8962_REG_DACR_RETUNE_C4_1:
	case WM8962_REG_DACR_RETUNE_C4_0:
	case WM8962_REG_DACR_RETUNE_C5_1:
	case WM8962_REG_DACR_RETUNE_C5_0:
	case WM8962_REG_DACR_RETUNE_C6_1:
	case WM8962_REG_DACR_RETUNE_C6_0:
	case WM8962_REG_DACR_RETUNE_C7_1:
	case WM8962_REG_DACR_RETUNE_C7_0:
	case WM8962_REG_DACR_RETUNE_C8_1:
	case WM8962_REG_DACR_RETUNE_C8_0:
	case WM8962_REG_DACR_RETUNE_C9_1:
	case WM8962_REG_DACR_RETUNE_C9_0:
	case WM8962_REG_DACR_RETUNE_C10_1:
	case WM8962_REG_DACR_RETUNE_C10_0:
	case WM8962_REG_DACR_RETUNE_C11_1:
	case WM8962_REG_DACR_RETUNE_C11_0:
	case WM8962_REG_DACR_RETUNE_C12_1:
	case WM8962_REG_DACR_RETUNE_C12_0:
	case WM8962_REG_DACR_RETUNE_C13_1:
	case WM8962_REG_DACR_RETUNE_C13_0:
	case WM8962_REG_DACR_RETUNE_C14_1:
	case WM8962_REG_DACR_RETUNE_C14_0:
	case WM8962_REG_DACR_RETUNE_C15_1:
	case WM8962_REG_DACR_RETUNE_C15_0:
	case WM8962_REG_DACR_RETUNE_C16_1:
	case WM8962_REG_DACR_RETUNE_C16_0:
	case WM8962_REG_DACR_RETUNE_C17_1:
	case WM8962_REG_DACR_RETUNE_C17_0:
	case WM8962_REG_DACR_RETUNE_C18_1:
	case WM8962_REG_DACR_RETUNE_C18_0:
	case WM8962_REG_DACR_RETUNE_C19_1:
	case WM8962_REG_DACR_RETUNE_C19_0:
	case WM8962_REG_DACR_RETUNE_C20_1:
	case WM8962_REG_DACR_RETUNE_C20_0:
	case WM8962_REG_DACR_RETUNE_C21_1:
	case WM8962_REG_DACR_RETUNE_C21_0:
	case WM8962_REG_DACR_RETUNE_C22_1:
	case WM8962_REG_DACR_RETUNE_C22_0:
	case WM8962_REG_DACR_RETUNE_C23_1:
	case WM8962_REG_DACR_RETUNE_C23_0:
	case WM8962_REG_DACR_RETUNE_C24_1:
	case WM8962_REG_DACR_RETUNE_C24_0:
	case WM8962_REG_DACR_RETUNE_C25_1:
	case WM8962_REG_DACR_RETUNE_C25_0:
	case WM8962_REG_DACR_RETUNE_C26_1:
	case WM8962_REG_DACR_RETUNE_C26_0:
	case WM8962_REG_DACR_RETUNE_C27_1:
	case WM8962_REG_DACR_RETUNE_C27_0:
	case WM8962_REG_DACR_RETUNE_C28_1:
	case WM8962_REG_DACR_RETUNE_C28_0:
	case WM8962_REG_DACR_RETUNE_C29_1:
	case WM8962_REG_DACR_RETUNE_C29_0:
	case WM8962_REG_DACR_RETUNE_C30_1:
	case WM8962_REG_DACR_RETUNE_C30_0:
	case WM8962_REG_DACR_RETUNE_C31_1:
	case WM8962_REG_DACR_RETUNE_C31_0:
	case WM8962_REG_DACR_RETUNE_C32_1:
	case WM8962_REG_DACR_RETUNE_C32_0:
	case WM8962_REG_VSS_XHD2_1:
	case WM8962_REG_VSS_XHD2_0:
	case WM8962_REG_VSS_XHD3_1:
	case WM8962_REG_VSS_XHD3_0:
	case WM8962_REG_VSS_XHN1_1:
	case WM8962_REG_VSS_XHN1_0:
	case WM8962_REG_VSS_XHN2_1:
	case WM8962_REG_VSS_XHN2_0:
	case WM8962_REG_VSS_XHN3_1:
	case WM8962_REG_VSS_XHN3_0:
	case WM8962_REG_VSS_XLA_1:
	case WM8962_REG_VSS_XLA_0:
	case WM8962_REG_VSS_XLB_1:
	case WM8962_REG_VSS_XLB_0:
	case WM8962_REG_VSS_XLG_1:
	case WM8962_REG_VSS_XLG_0:
	case WM8962_REG_VSS_PG2_1:
	case WM8962_REG_VSS_PG2_0:
	case WM8962_REG_VSS_PG_1:
	case WM8962_REG_VSS_PG_0:
	case WM8962_REG_VSS_XTD1_1:
	case WM8962_REG_VSS_XTD1_0:
	case WM8962_REG_VSS_XTD2_1:
	case WM8962_REG_VSS_XTD2_0:
	case WM8962_REG_VSS_XTD3_1:
	case WM8962_REG_VSS_XTD3_0:
	case WM8962_REG_VSS_XTD4_1:
	case WM8962_REG_VSS_XTD4_0:
	case WM8962_REG_VSS_XTD5_1:
	case WM8962_REG_VSS_XTD5_0:
	case WM8962_REG_VSS_XTD6_1:
	case WM8962_REG_VSS_XTD6_0:
	case WM8962_REG_VSS_XTD7_1:
	case WM8962_REG_VSS_XTD7_0:
	case WM8962_REG_VSS_XTD8_1:
	case WM8962_REG_VSS_XTD8_0:
	case WM8962_REG_VSS_XTD9_1:
	case WM8962_REG_VSS_XTD9_0:
	case WM8962_REG_VSS_XTD10_1:
	case WM8962_REG_VSS_XTD10_0:
	case WM8962_REG_VSS_XTD11_1:
	case WM8962_REG_VSS_XTD11_0:
	case WM8962_REG_VSS_XTD12_1:
	case WM8962_REG_VSS_XTD12_0:
	case WM8962_REG_VSS_XTD13_1:
	case WM8962_REG_VSS_XTD13_0:
	case WM8962_REG_VSS_XTD14_1:
	case WM8962_REG_VSS_XTD14_0:
	case WM8962_REG_VSS_XTD15_1:
	case WM8962_REG_VSS_XTD15_0:
	case WM8962_REG_VSS_XTD16_1:
	case WM8962_REG_VSS_XTD16_0:
	case WM8962_REG_VSS_XTD17_1:
	case WM8962_REG_VSS_XTD17_0:
	case WM8962_REG_VSS_XTD18_1:
	case WM8962_REG_VSS_XTD18_0:
	case WM8962_REG_VSS_XTD19_1:
	case WM8962_REG_VSS_XTD19_0:
	case WM8962_REG_VSS_XTD20_1:
	case WM8962_REG_VSS_XTD20_0:
	case WM8962_REG_VSS_XTD21_1:
	case WM8962_REG_VSS_XTD21_0:
	case WM8962_REG_VSS_XTD22_1:
	case WM8962_REG_VSS_XTD22_0:
	case WM8962_REG_VSS_XTD23_1:
	case WM8962_REG_VSS_XTD23_0:
	case WM8962_REG_VSS_XTD24_1:
	case WM8962_REG_VSS_XTD24_0:
	case WM8962_REG_VSS_XTD25_1:
	case WM8962_REG_VSS_XTD25_0:
	case WM8962_REG_VSS_XTD26_1:
	case WM8962_REG_VSS_XTD26_0:
	case WM8962_REG_VSS_XTD27_1:
	case WM8962_REG_VSS_XTD27_0:
	case WM8962_REG_VSS_XTD28_1:
	case WM8962_REG_VSS_XTD28_0:
	case WM8962_REG_VSS_XTD29_1:
	case WM8962_REG_VSS_XTD29_0:
	case WM8962_REG_VSS_XTD30_1:
	case WM8962_REG_VSS_XTD30_0:
	case WM8962_REG_VSS_XTD31_1:
	case WM8962_REG_VSS_XTD31_0:
	case WM8962_REG_VSS_XTD32_1:
	case WM8962_REG_VSS_XTD32_0:
	case WM8962_REG_VSS_XTS1_1:
	case WM8962_REG_VSS_XTS1_0:
	case WM8962_REG_VSS_XTS2_1:
	case WM8962_REG_VSS_XTS2_0:
	case WM8962_REG_VSS_XTS3_1:
	case WM8962_REG_VSS_XTS3_0:
	case WM8962_REG_VSS_XTS4_1:
	case WM8962_REG_VSS_XTS4_0:
	case WM8962_REG_VSS_XTS5_1:
	case WM8962_REG_VSS_XTS5_0:
	case WM8962_REG_VSS_XTS6_1:
	case WM8962_REG_VSS_XTS6_0:
	case WM8962_REG_VSS_XTS7_1:
	case WM8962_REG_VSS_XTS7_0:
	case WM8962_REG_VSS_XTS8_1:
	case WM8962_REG_VSS_XTS8_0:
	case WM8962_REG_VSS_XTS9_1:
	case WM8962_REG_VSS_XTS9_0:
	case WM8962_REG_VSS_XTS10_1:
	case WM8962_REG_VSS_XTS10_0:
	case WM8962_REG_VSS_XTS11_1:
	case WM8962_REG_VSS_XTS11_0:
	case WM8962_REG_VSS_XTS12_1:
	case WM8962_REG_VSS_XTS12_0:
	case WM8962_REG_VSS_XTS13_1:
	case WM8962_REG_VSS_XTS13_0:
	case WM8962_REG_VSS_XTS14_1:
	case WM8962_REG_VSS_XTS14_0:
	case WM8962_REG_VSS_XTS15_1:
	case WM8962_REG_VSS_XTS15_0:
	case WM8962_REG_VSS_XTS16_1:
	case WM8962_REG_VSS_XTS16_0:
	case WM8962_REG_VSS_XTS17_1:
	case WM8962_REG_VSS_XTS17_0:
	case WM8962_REG_VSS_XTS18_1:
	case WM8962_REG_VSS_XTS18_0:
	case WM8962_REG_VSS_XTS19_1:
	case WM8962_REG_VSS_XTS19_0:
	case WM8962_REG_VSS_XTS20_1:
	case WM8962_REG_VSS_XTS20_0:
	case WM8962_REG_VSS_XTS21_1:
	case WM8962_REG_VSS_XTS21_0:
	case WM8962_REG_VSS_XTS22_1:
	case WM8962_REG_VSS_XTS22_0:
	case WM8962_REG_VSS_XTS23_1:
	case WM8962_REG_VSS_XTS23_0:
	case WM8962_REG_VSS_XTS24_1:
	case WM8962_REG_VSS_XTS24_0:
	case WM8962_REG_VSS_XTS25_1:
	case WM8962_REG_VSS_XTS25_0:
	case WM8962_REG_VSS_XTS26_1:
	case WM8962_REG_VSS_XTS26_0:
	case WM8962_REG_VSS_XTS27_1:
	case WM8962_REG_VSS_XTS27_0:
	case WM8962_REG_VSS_XTS28_1:
	case WM8962_REG_VSS_XTS28_0:
	case WM8962_REG_VSS_XTS29_1:
	case WM8962_REG_VSS_XTS29_0:
	case WM8962_REG_VSS_XTS30_1:
	case WM8962_REG_VSS_XTS30_0:
	case WM8962_REG_VSS_XTS31_1:
	case WM8962_REG_VSS_XTS31_0:
	case WM8962_REG_VSS_XTS32_1:
	case WM8962_REG_VSS_XTS32_0:
		return true;
	default:
		return false;
	}
}

void WM8962_dump()
{
	int i;
	uint32_t val = 0;
	int32_t data_width = 16;
	for (i = 0; i < CODEC_CACHEREGNUM; i++)
		if (wm8962_readable_register(i))
			i2c_read_data(BUS_CODEC_ADDR, i, &val, data_width);
}

struct reg_default * find_reg_cache_by_reg(int reg)
{
	uint32_t flag = 0;
	int i;
	for (i = 0; i < CODEC_CACHEREGNUM; i++) {
		if (wm8962_reg_cache[i].reg == reg) {
			flag = 1;
			break;
		}
	}

	if (flag)
		return (struct reg_default *)&wm8962_reg_cache[i];
	else
		return NULL;
}

void WM8962_Init()
{
	WM8962_WriteReg(WM8962_REG_SOFTWARE_RESET, 0x6243);
	WM8962_WriteReg(WM8962_REG_PLL_SOFTWARE_RESET, 0x0000);

	WM8962_WriteReg(WM8962_REG_CLOCKING2, 0x09c4);

	WM8962_WriteReg(WM8962_REG_LEFT_INPUT_VOLUME, 0x0128);
	WM8962_WriteReg(WM8962_REG_RIGHT_INPUT_VOLUME, 0x0528);
	WM8962_WriteReg(WM8962_REG_HPOUTL_VOLUME, 0x015d);
	WM8962_WriteReg(WM8962_REG_HPOUTR_VOLUME, 0x015d);

	WM8962_WriteReg(WM8962_REG_CLOCKING2, 0x09c4);

	WM8962_WriteReg(WM8962_REG_LEFT_DAC_VOLUME, 0x01c0);
	WM8962_WriteReg(WM8962_REG_RIGHT_DAC_VOLUME, 0x01c0);

	WM8962_WriteReg(WM8962_REG_LEFT_ADC_VOLUME, 0x01d8);
	WM8962_WriteReg(WM8962_REG_RIGHT_ADC_VOLUME, 0x01d8);

	WM8962_WriteReg(WM8962_REG_RIGHT_INPUT_MIXER_VOLUME, 0x0147);
	WM8962_WriteReg(WM8962_REG_INPUT_MIXER_CONTROL_2, 0x000b);

	WM8962_WriteReg(WM8962_REG_RIGHT_INPUT_PGA_CONTROL, 0x0002);

	WM8962_WriteReg(WM8962_REG_SPKOUTL_VOLUME, 0x0172);
	WM8962_WriteReg(WM8962_REG_SPKOUTR_VOLUME, 0x0172);

	WM8962_WriteReg(WM8962_REG_CHARGE_PUMP_B, 0x0005);
	WM8962_WriteReg(WM8962_REG_PLL2, 0x0041);
	WM8962_WriteReg(WM8962_REG_THREED1, 0x0040);

	WM8962_WriteReg(WM8962_REG_EQ1, 0x0000);
	WM8962_WriteReg(WM8962_REG_IRQ_DEBOUNCE, 0x0000);

	WM8962_WriteReg(WM8962_REG_ANTI_POP, 0x0018);
	WM8962_WriteReg(WM8962_REG_PWR_MGMT_1, 0x01c0);

	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_1, 0x0008);
	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_2, 0x0038);
	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_3, 0x0180);
	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_5, 0x0032);
	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_6, 0x0000);
	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_7, 0x0001);
	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_8, 0x0008);

	WM8962_WriteReg(WM8962_REG_FLL_CONTROL_1, 0x0009);

	WM8962_WriteReg(WM8962_REG_CLOCKING2, 0x0bc4);
	WM8962_WriteReg(WM8962_REG_AUDIO_INTERFACE_0, 0x0002);

	WM8962_WriteReg(WM8962_REG_PWR_MGMT_1, 0x0140);
	WM8962_WriteReg(WM8962_REG_PWR_MGMT_1, 0x00c0);

	WM8962_WriteReg(WM8962_REG_CLOCKING2, 0x0be4);
	WM8962_WriteReg(WM8962_REG_CLOCKING2, 0x0bc4);
	WM8962_WriteReg(WM8962_REG_CLOCKING2, 0x0bc7);
	WM8962_WriteReg(WM8962_REG_AUDIO_INTERFACE_2, 0x0020);
	WM8962_WriteReg(WM8962_REG_CLOCKING2, 0x0be7);

	WM8962_WriteReg(WM8962_REG_ADDITIONAL_CONTROL_1, 0x0161);
	WM8962_WriteReg(WM8962_REG_CHARGE_PUMP_1, 0x0001);

	WM8962_WriteReg(WM8962_REG_DSP2_POWER_MANAGEMENT, 0x1c01);

	WM8962_WriteReg(WM8962_REG_PWR_MGMT_2, 0x0060);
	WM8962_WriteReg(WM8962_REG_HPOUTL_VOLUME, 0x015d);
	WM8962_WriteReg(WM8962_REG_HPOUTR_VOLUME, 0x015d);

	WM8962_WriteReg(WM8962_REG_PWR_MGMT_2, 0x01e0);

	WM8962_WriteReg(WM8962_REG_ANALOGUE_HP_0, 0x0011);
	WM8962_WriteReg(WM8962_REG_ANALOGUE_HP_0, 0x0033);
	WM8962_WriteReg(WM8962_REG_DC_SERVO_1, 0x03cc);
	WM8962_WriteReg(WM8962_REG_ANALOGUE_HP_0, 0x0077);
	WM8962_WriteReg(WM8962_REG_ANALOGUE_HP_0, 0x00ff);
	WM8962_WriteReg(WM8962_REG_CLASS_D_CONTROL_1, 0x0000);
	WM8962_WriteReg(WM8962_REG_ADC_DAC_CONTROL_1, 0x0010);

	return;
}

int WM8962_WriteReg(uint16_t reg, uint16_t val)
{
	int ret;
	uint32_t slave_addr = BUS_CODEC_ADDR;
	int32_t data_width = 16;
	/* copy data to cache */
	struct reg_default *reg_struct = find_reg_cache_by_reg(reg);
	if (reg_struct == NULL) {
		LOG1("REG[%x] not find!\n", reg);
		return 0;
	}

	reg_struct->def = val;

	ret = i2c_transfer_data(slave_addr, reg, val, data_width);
	return ret;
}

int WM8962_ReadReg(uint16_t reg, uint16_t *val)
{
	if (reg >= CODEC_CACHEREGNUM)
	{
		return -1;
	}

	struct reg_default *reg_struct = find_reg_cache_by_reg(reg);
	if (reg_struct == NULL) {
		LOG1("REG[%x] not find!\n", reg);
		return 0;
	}
	*val = reg_struct->def;

	return 0;
}

int WM8962_ModifyReg(uint16_t reg, uint16_t mask, uint16_t val)
{
	int ret   = 0;
	uint16_t reg_val = 0;

	WM8962_ReadReg(reg, &reg_val);

	reg_val &= (uint16_t)~mask;
	reg_val |= val;

	ret = WM8962_WriteReg(reg, reg_val);

	return ret;
}
