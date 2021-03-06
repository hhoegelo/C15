/******************************************************************************/
/** @file       pe_defines_labels.h
    @date       2018-04-20
    @version    1.0
    @author     Matthias Seeber
    @brief      c15 parameter labels (for better code readability)
                NOTE:
                The provided IDs represent the rendering items of the parameter
                engine, NOT the elements of the shared m_paramsignaldata[][] array!
                ..
    @todo
*******************************************************************************/

#pragma once

/* Envelope Parameter Labels - access parameter with (envIndex + ID) */
// NOTE: placeholders and Env Param Labels are needed until old envelopes disappear

enum class EnvelopeLabel
{
  E_ATT = 0,
  E_DEC1 = 1,
  E_BP = 2,
  E_DEC2 = 3,
  E_SUS = 4,
  E_REL = 5,
  E_LV = 7,
  E_AV = 8,
  E_D1V = 9,
  E_D2V = 10,
  E_RV = 11,
  E_LKT = 12,
  E_TKT = 13,
  E_AC = 14,
  E_RH = 15,
  E_SPL = 16
};

/* 'normal' Parameter Labels - access parameter with m_head[ID] - maybe, the rendering index would be much more efficient? */
enum class Parameters
{
  P_INVALID = -1,
  P_EA_ATT = 0,
  P_EA_DEC1 = 1,
  P_EA_BP = 2,
  P_EA_DEC2 = 3,
  P_EA_SUS = 4,
  P_EA_REL = 5,
  P_EA_GAIN = 6,
  P_EA_LV = 7,
  P_EA_AV = 8,
  P_EA_D1V = 9,
  P_EA_D2V = 10,
  P_EA_RV = 11,
  P_EA_LKT = 12,
  P_EA_TKT = 13,
  P_EA_AC = 14,
  P_EA_SPL = 16,

  P_EB_ATT = 17,
  P_EB_DEC1 = 18,
  P_EB_BP = 19,
  P_EB_DEC2 = 20,
  P_EB_SUS = 21,
  P_EB_REL = 22,
  P_EB_GAIN = 23,
  P_EB_LV = 24,
  P_EB_AV = 25,
  P_EB_D1V = 26,
  P_EB_D2V = 27,
  P_EB_RV = 28,
  P_EB_LKT = 29,
  P_EB_TKT = 30,
  P_EB_AC = 31,
  P_EB_SPL = 33,

  P_EC_ATT = 34,
  P_EC_DEC1 = 35,
  P_EC_BP = 36,
  P_EC_DEC2 = 37,
  P_EC_SUS = 38,
  P_EC_REL = 39,
  P_EC_LV = 41,
  P_EC_AV = 42,
  P_EC_RV = 45,
  P_EC_LKT = 46,
  P_EC_TKT = 47,
  P_EC_AC = 48,
  P_EC_RH = 49,

  P_OA_P = 51,
  P_OA_PKT = 52,
  P_OA_PEC = 53,
  P_OA_F = 54,
  P_OA_FEC = 55,
  P_OA_PMS = 56,
  P_OA_PMSEA = 57,
  P_OA_PMSSH = 58,
  P_OA_PMB = 59,
  P_OA_PMBEB = 60,
  P_OA_PMBSH = 61,
  P_OA_PMF = 62,
  P_OA_PMFEC = 63,
  P_OA_PHS = 64,
  P_OA_CHI = 65,

  P_SA_DRV = 66,
  P_SA_DEA = 67,
  P_SA_FLD = 68,
  P_SA_ASM = 69,
  P_SA_MIX = 70,
  P_SA_FBM = 71,
  P_SA_FBEC = 72,
  P_SA_RM = 73,

  P_OB_P = 74,
  P_OB_PKT = 75,
  P_OB_PEC = 76,
  P_OB_F = 77,
  P_OB_FEC = 78,
  P_OB_PMS = 79,
  P_OB_PMSEB = 80,
  P_OB_PMSSH = 81,
  P_OB_PMA = 82,
  P_OB_PMAEA = 83,
  P_OB_PMASH = 84,
  P_OB_PMF = 85,
  P_OB_PMFEC = 86,
  P_OB_PHS = 87,
  P_OB_CHI = 88,

  P_SB_DRV = 89,
  P_SB_DEB = 90,
  P_SB_FLD = 91,
  P_SB_ASM = 92,
  P_SB_MIX = 93,
  P_SB_FBM = 94,
  P_SB_FBEC = 95,
  P_SB_RM = 96,

  P_CMB_AB = 97,
  P_CMB_P = 98,
  P_CMB_PKT = 99,
  P_CMB_PEC = 100,
  P_CMB_D = 101,
  P_CMB_DKT = 102,
  P_CMB_DG = 103,
  P_CMB_APT = 104,
  P_CMB_APKT = 105,
  P_CMB_APEC = 106,
  P_CMB_APR = 107,
  P_CMB_LP = 108,
  P_CMB_LPKT = 109,
  P_CMB_LPEC = 110,
  P_CMB_PM = 111,
  P_CMB_PMAB = 112,

  P_SVF_AB = 113,
  P_SVF_CMB = 114,
  P_SVF_CUT = 115,
  P_SVF_CKT = 116,
  P_SVF_CEC = 117,
  P_SVF_SPR = 118,
  P_SVF_FM = 119,
  P_SVF_RES = 120,
  P_SVF_RKT = 121,
  P_SVF_REC = 122,
  P_SVF_LBH = 123,
  P_SVF_PAR = 124,
  P_SVF_FMAB = 125,

  P_FBM_CMB = 126,
  P_FBM_SVF = 127,
  P_FBM_FX = 128,
  P_FBM_REV = 129,
  P_FBM_DRV = 130,
  P_FBM_FLD = 131,
  P_FBM_ASM = 132,
  P_FBM_LKT = 133,
  P_FBM_LVL = 134,

  P_OM_AL = 135,
  P_OM_AP = 136,
  P_OM_BL = 137,
  P_OM_BP = 138,
  P_OM_CL = 139,
  P_OM_CP = 140,
  P_OM_SL = 141,
  P_OM_SP = 142,
  P_OM_DRV = 143,
  P_OM_FLD = 144,
  P_OM_ASM = 145,
  P_OM_LVL = 146,
  P_OM_KP = 147,

  P_CAB_DRV = 148,
  P_CAB_FLD = 149,
  P_CAB_ASM = 150,
  P_CAB_TILT = 151,
  P_CAB_LPF = 152,
  P_CAB_HPF = 153,
  P_CAB_LVL = 154,
  P_CAB_MIX = 155,

  P_GAP_CNT = 156,
  P_GAP_STE = 157,
  P_GAP_GAP = 158,
  P_GAP_RES = 159,
  P_GAP_BAL = 160,
  P_GAP_MIX = 161,

  P_FLA_TMOD = 162,
  P_FLA_PHS = 163,
  P_FLA_RTE = 164,
  P_FLA_TIME = 165,
  P_FLA_STE = 166,
  P_FLA_FB = 167,
  P_FLA_CFB = 168,
  P_FLA_LPF = 169,
  P_FLA_MIX = 170,
  P_FLA_ENV = 171,
  P_FLA_APM = 172,
  P_FLA_APT = 173,

  P_DLY_TIME = 174,
  P_DLY_STE = 175,
  P_DLY_FB = 176,
  P_DLY_CFB = 177,
  P_DLY_LPF = 178,
  P_DLY_MIX = 179,
  P_DLY_SND = 180,

  P_REV_SIZE = 181,
  P_REV_PRE = 182,
  P_REV_COL = 183,
  P_REV_CHO = 184,
  P_REV_MIX = 185,
  P_REV_SND = 186,

  P_MA_V = 187,
  P_MA_T = 188,
  P_MA_SH = 189,

  P_UN_V = 190,
  P_UN_DET = 191,
  P_UN_PHS = 192,
  P_UN_PAN = 193,

  /* milestone 1.5 POLY KEY params */
  P_KEY_PH = 194,
  P_KEY_NP = 195,
  P_KEY_VP = 196,
  P_KEY_VS = 197,

  /* milestone 1.55 POLY KEY params / milestone 1.56 POLY basePitch param */
  P_KEY_BP = 194,
  P_KEY_IDX = 195,
  P_KEY_STL = 196
};
