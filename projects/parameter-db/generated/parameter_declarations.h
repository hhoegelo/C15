#pragma once

/******************************************************************************/
/**	@file       parameter_declarations.h
    @date       2020-04-29, 13:13
    @version	1.7B-5
    @author     M. Seeber
    @brief      descriptors for all parameter-related details
    @todo
*******************************************************************************/

namespace C15
{

  namespace Descriptors
  {

    enum class ParameterType
    {
      None,
      Hardware_Source,
      Hardware_Amount,
      Macro_Control,
      Macro_Time,
      Global_Modulateable,
      Global_Unmodulateable,
      Local_Modulateable,
      Local_Unmodulateable
    };

    enum class SmootherSection
    {
      None,
      Global,
      Poly,
      Mono
    };

    enum class SmootherClock
    {
      Sync,
      Audio,
      Fast,
      Slow
    };

    enum class ParameterSignal
    {
      None,
      Global_Signal,
      Quasipoly_Signal,
      Truepoly_Signal,
      Mono_Signal
    };

  }  // namespace C15::Descriptors

  namespace Properties
  {

    enum class SmootherScale
    {
      None,
      Linear,
      Parabolic,
      Cubic,
      S_Curve,
      Expon_Gain,
      Expon_Osc_Pitch,
      Expon_Lin_Pitch,
      Expon_Shaper_Drive,
      Expon_Mix_Drive,
      Expon_Env_Time
    };

  }  // namespace C15::Properties

  namespace Parameters
  {

    enum class Hardware_Sources
    {
      Pedal_1,
      Pedal_2,
      Pedal_3,
      Pedal_4,
      Bender,
      Aftertouch,
      Ribbon_1,
      Ribbon_2,
      _LENGTH_
    };

    enum class Hardware_Amounts
    {
      Ped_1_to_MC_A,
      Ped_1_to_MC_B,
      Ped_1_to_MC_C,
      Ped_1_to_MC_D,
      Ped_1_to_MC_E,
      Ped_1_to_MC_F,
      Ped_2_to_MC_A,
      Ped_2_to_MC_B,
      Ped_2_to_MC_C,
      Ped_2_to_MC_D,
      Ped_2_to_MC_E,
      Ped_2_to_MC_F,
      Ped_3_to_MC_A,
      Ped_3_to_MC_B,
      Ped_3_to_MC_C,
      Ped_3_to_MC_D,
      Ped_3_to_MC_E,
      Ped_3_to_MC_F,
      Ped_4_to_MC_A,
      Ped_4_to_MC_B,
      Ped_4_to_MC_C,
      Ped_4_to_MC_D,
      Ped_4_to_MC_E,
      Ped_4_to_MC_F,
      Bend_to_MC_A,
      Bend_to_MC_B,
      Bend_to_MC_C,
      Bend_to_MC_D,
      Bend_to_MC_E,
      Bend_to_MC_F,
      AT_to_MC_A,
      AT_to_MC_B,
      AT_to_MC_C,
      AT_to_MC_D,
      AT_to_MC_E,
      AT_to_MC_F,
      Rib_1_to_MC_A,
      Rib_1_to_MC_B,
      Rib_1_to_MC_C,
      Rib_1_to_MC_D,
      Rib_1_to_MC_E,
      Rib_1_to_MC_F,
      Rib_2_to_MC_A,
      Rib_2_to_MC_B,
      Rib_2_to_MC_C,
      Rib_2_to_MC_D,
      Rib_2_to_MC_E,
      Rib_2_to_MC_F,
      _LENGTH_
    };

    enum class Macro_Controls
    {
      None,
      MC_A,
      MC_B,
      MC_C,
      MC_D,
      MC_E,
      MC_F,
      _LENGTH_
    };

    enum class Macro_Times
    {
      None,
      MC_Time_A,
      MC_Time_B,
      MC_Time_C,
      MC_Time_D,
      MC_Time_E,
      MC_Time_F,
      _LENGTH_
    };

    enum class Global_Modulateables
    {
      Split_Split_Point,
      Master_Volume,
      Master_Tune,
      _LENGTH_
    };

    enum class Global_Unmodulateables
    {
      Scale_Base_Key,
      Scale_Offset_0,
      Scale_Offset_1,
      Scale_Offset_2,
      Scale_Offset_3,
      Scale_Offset_4,
      Scale_Offset_5,
      Scale_Offset_6,
      Scale_Offset_7,
      Scale_Offset_8,
      Scale_Offset_9,
      Scale_Offset_10,
      Scale_Offset_11,
      _LENGTH_
    };

    enum class Local_Modulateables
    {
      Unison_Detune,
      Voice_Grp_Volume,
      Voice_Grp_Tune,
      Mono_Grp_Glide,
      Env_A_Att,
      Env_A_Dec_1,
      Env_A_BP,
      Env_A_Dec_2,
      Env_A_Sus,
      Env_A_Rel,
      Env_A_Gain,
      Env_B_Att,
      Env_B_Dec_1,
      Env_B_BP,
      Env_B_Dec_2,
      Env_B_Sus,
      Env_B_Rel,
      Env_B_Gain,
      Env_C_Att,
      Env_C_Dec_1,
      Env_C_BP,
      Env_C_Dec_2,
      Env_C_Sus,
      Env_C_Rel,
      Osc_A_Pitch,
      Osc_A_Fluct,
      Osc_A_PM_Self,
      Osc_A_PM_B,
      Osc_A_PM_FB,
      Osc_A_Phase,
      Shp_A_Drive,
      Shp_A_Mix,
      Shp_A_FB_Mix,
      Shp_A_Ring_Mod,
      Osc_B_Pitch,
      Osc_B_Fluct,
      Osc_B_PM_Self,
      Osc_B_PM_A,
      Osc_B_PM_FB,
      Osc_B_Phase,
      Shp_B_Drive,
      Shp_B_Mix,
      Shp_B_FB_Mix,
      Shp_B_Ring_Mod,
      Comb_Flt_In_A_B,
      Comb_Flt_Pitch,
      Comb_Flt_Decay,
      Comb_Flt_Decay_Gate,
      Comb_Flt_AP_Tune,
      Comb_Flt_AP_Res,
      Comb_Flt_LP_Tune,
      Comb_Flt_PM,
      SV_Flt_In_A_B,
      SV_Flt_Comb_Mix,
      SV_Flt_Cut,
      SV_Flt_Res,
      SV_Flt_Spread,
      SV_Flt_LBH,
      SV_Flt_FM,
      FB_Mix_Osc,
      FB_Mix_Osc_Src,
      FB_Mix_Comb,
      FB_Mix_Comb_Src,
      FB_Mix_SVF,
      FB_Mix_SVF_Src,
      FB_Mix_FX,
      FB_Mix_FX_Src,
      FB_Mix_Rvb,
      FB_Mix_Drive,
      FB_Mix_Lvl,
      Out_Mix_A_Lvl,
      Out_Mix_B_Lvl,
      Out_Mix_Comb_Lvl,
      Out_Mix_SVF_Lvl,
      Out_Mix_Drive,
      Out_Mix_Lvl,
      Out_Mix_To_FX,
      Flanger_Time_Mod,
      Flanger_Rate,
      Flanger_Time,
      Flanger_Feedback,
      Flanger_Mix,
      Flanger_AP_Mod,
      Flanger_AP_Tune,
      Flanger_Tremolo,
      Cabinet_Drive,
      Cabinet_Tilt,
      Cabinet_Hi_Cut,
      Cabinet_Cab_Lvl,
      Cabinet_Mix,
      Gap_Flt_Center,
      Gap_Flt_Gap,
      Gap_Flt_Bal,
      Gap_Flt_Mix,
      Echo_Time,
      Echo_Stereo,
      Echo_Feedback,
      Echo_Mix,
      Echo_Send,
      Reverb_Size,
      Reverb_Color,
      Reverb_Mix,
      Reverb_Send,
      _LENGTH_
    };

    enum class Local_Unmodulateables
    {
      Unison_Voices,
      Unison_Phase,
      Unison_Pan,
      Voice_Grp_Mute,
      Voice_Grp_Fade_From,
      Voice_Grp_Fade_Range,
      Mono_Grp_Enable,
      Mono_Grp_Prio,
      Mono_Grp_Legato,
      Env_A_Lvl_Vel,
      Env_A_Att_Vel,
      Env_A_Dec_1_Vel,
      Env_A_Dec_2_Vel,
      Env_A_Rel_Vel,
      Env_A_Lvl_KT,
      Env_A_Time_KT,
      Env_A_Att_Curve,
      Env_A_Elevate,
      Env_B_Lvl_Vel,
      Env_B_Att_Vel,
      Env_B_Dec_1_Vel,
      Env_B_Dec_2_Vel,
      Env_B_Rel_Vel,
      Env_B_Lvl_KT,
      Env_B_Time_KT,
      Env_B_Att_Curve,
      Env_B_Elevate,
      Env_C_Lvl_Vel,
      Env_C_Att_Vel,
      Env_C_Rel_Vel,
      Env_C_Lvl_KT,
      Env_C_Time_KT,
      Env_C_Att_Curve,
      Env_C_Retr_H,
      Osc_A_Pitch_KT,
      Osc_A_Pitch_Env_C,
      Osc_A_Fluct_Env_C,
      Osc_A_PM_Self_Env_A,
      Osc_A_PM_Self_Shp,
      Osc_A_PM_B_Env_B,
      Osc_A_PM_B_Shp,
      Osc_A_PM_FB_Env_C,
      Osc_A_Chirp,
      Osc_A_Reset,
      Shp_A_Drive_Env_A,
      Shp_A_Fold,
      Shp_A_Asym,
      Shp_A_FB_Env_C,
      Osc_B_Pitch_KT,
      Osc_B_Pitch_Env_C,
      Osc_B_Fluct_Env_C,
      Osc_B_PM_Self_Env_B,
      Osc_B_PM_Self_Shp,
      Osc_B_PM_A_Env_A,
      Osc_B_PM_A_Shp,
      Osc_B_PM_FB_Env_C,
      Osc_B_Chirp,
      Osc_B_Reset,
      Shp_B_Drive_Env_B,
      Shp_B_Fold,
      Shp_B_Asym,
      Shp_B_FB_Env_C,
      Comb_Flt_Pitch_KT,
      Comb_Flt_Pitch_Env_C,
      Comb_Flt_Decay_KT,
      Comb_Flt_AP_KT,
      Comb_Flt_AP_Env_C,
      Comb_Flt_LP_KT,
      Comb_Flt_LP_Env_C,
      Comb_Flt_PM_A_B,
      SV_Flt_Cut_KT,
      SV_Flt_Cut_Env_C,
      SV_Flt_Res_KT,
      SV_Flt_Res_Env_C,
      SV_Flt_Par,
      SV_Flt_FM_A_B,
      FB_Mix_Fold,
      FB_Mix_Asym,
      FB_Mix_Lvl_KT,
      Out_Mix_A_Pan,
      Out_Mix_B_Pan,
      Out_Mix_Comb_Pan,
      Out_Mix_SVF_Pan,
      Out_Mix_Fold,
      Out_Mix_Asym,
      Out_Mix_Key_Pan,
      Flanger_Phase,
      Flanger_Stereo,
      Flanger_Cross_FB,
      Flanger_Hi_Cut,
      Flanger_Envelope,
      Cabinet_Fold,
      Cabinet_Asym,
      Cabinet_Lo_Cut,
      Gap_Flt_Stereo,
      Gap_Flt_Res,
      Echo_Cross_FB,
      Echo_Hi_Cut,
      Reverb_Pre_Dly,
      Reverb_Chorus,
      _LENGTH_
    };

  }  // namespace C15::Parameters

  namespace Smoothers
  {

    enum class Global_Sync
    {
      _LENGTH_
    };

    enum class Global_Audio
    {
      _LENGTH_
    };

    enum class Global_Fast
    {
      Master_Volume,
      _LENGTH_
    };

    enum class Global_Slow
    {
      Master_Tune,
      Scale_Base_Key,
      Scale_Offset_0,
      Scale_Offset_1,
      Scale_Offset_2,
      Scale_Offset_3,
      Scale_Offset_4,
      Scale_Offset_5,
      Scale_Offset_6,
      Scale_Offset_7,
      Scale_Offset_8,
      Scale_Offset_9,
      Scale_Offset_10,
      Scale_Offset_11,
      _LENGTH_
    };

    enum class Poly_Sync
    {
      Voice_Grp_Fade_From,
      Voice_Grp_Fade_Range,
      Env_A_Lvl_Vel,
      Env_A_Att_Vel,
      Env_A_Dec_1_Vel,
      Env_A_Dec_2_Vel,
      Env_A_Rel_Vel,
      Env_A_Lvl_KT,
      Env_A_Time_KT,
      Env_A_Att_Curve,
      Env_B_Lvl_Vel,
      Env_B_Att_Vel,
      Env_B_Dec_1_Vel,
      Env_B_Dec_2_Vel,
      Env_B_Rel_Vel,
      Env_B_Lvl_KT,
      Env_B_Time_KT,
      Env_B_Att_Curve,
      Env_C_Lvl_Vel,
      Env_C_Att_Vel,
      Env_C_Rel_Vel,
      Env_C_Lvl_KT,
      Env_C_Time_KT,
      Env_C_Att_Curve,
      Env_C_Retr_H,
      Osc_A_Reset,
      Osc_B_Reset,
      _LENGTH_
    };

    enum class Poly_Audio
    {
      Unison_Phase,
      Osc_A_Phase,
      Osc_B_Phase,
      _LENGTH_
    };

    enum class Poly_Fast
    {
      Unison_Pan,
      Voice_Grp_Volume,
      Voice_Grp_Mute,
      Env_A_BP,
      Env_A_Sus,
      Env_A_Gain,
      Env_A_Elevate,
      Env_B_BP,
      Env_B_Sus,
      Env_B_Gain,
      Env_B_Elevate,
      Env_C_BP,
      Env_C_Sus,
      Osc_A_PM_Self,
      Osc_A_PM_B,
      Osc_A_PM_FB,
      Shp_A_Drive,
      Shp_A_Fold,
      Shp_A_Asym,
      Shp_A_Mix,
      Shp_A_FB_Mix,
      Shp_A_Ring_Mod,
      Osc_B_PM_Self,
      Osc_B_PM_A,
      Osc_B_PM_FB,
      Shp_B_Drive,
      Shp_B_Fold,
      Shp_B_Asym,
      Shp_B_Mix,
      Shp_B_FB_Mix,
      Shp_B_Ring_Mod,
      Comb_Flt_In_A_B,
      SV_Flt_In_A_B,
      SV_Flt_Comb_Mix,
      SV_Flt_LBH,
      SV_Flt_Par,
      FB_Mix_Osc,
      FB_Mix_Osc_Src,
      FB_Mix_Comb,
      FB_Mix_Comb_Src,
      FB_Mix_SVF,
      FB_Mix_SVF_Src,
      FB_Mix_FX,
      FB_Mix_FX_Src,
      FB_Mix_Rvb,
      FB_Mix_Drive,
      FB_Mix_Fold,
      FB_Mix_Asym,
      FB_Mix_Lvl,
      Out_Mix_A_Lvl,
      Out_Mix_A_Pan,
      Out_Mix_B_Lvl,
      Out_Mix_B_Pan,
      Out_Mix_Comb_Lvl,
      Out_Mix_Comb_Pan,
      Out_Mix_SVF_Lvl,
      Out_Mix_SVF_Pan,
      Out_Mix_Drive,
      Out_Mix_Fold,
      Out_Mix_Asym,
      Out_Mix_Lvl,
      Out_Mix_Key_Pan,
      Out_Mix_To_FX,
      _LENGTH_
    };

    enum class Poly_Slow
    {
      Unison_Detune,
      Voice_Grp_Tune,
      Mono_Grp_Glide,
      Env_A_Att,
      Env_A_Dec_1,
      Env_A_Dec_2,
      Env_A_Rel,
      Env_B_Att,
      Env_B_Dec_1,
      Env_B_Dec_2,
      Env_B_Rel,
      Env_C_Att,
      Env_C_Dec_1,
      Env_C_Dec_2,
      Env_C_Rel,
      Osc_A_Pitch,
      Osc_A_Pitch_KT,
      Osc_A_Pitch_Env_C,
      Osc_A_Fluct,
      Osc_A_Fluct_Env_C,
      Osc_A_PM_Self_Env_A,
      Osc_A_PM_Self_Shp,
      Osc_A_PM_B_Env_B,
      Osc_A_PM_B_Shp,
      Osc_A_PM_FB_Env_C,
      Osc_A_Chirp,
      Shp_A_Drive_Env_A,
      Shp_A_FB_Env_C,
      Osc_B_Pitch,
      Osc_B_Pitch_KT,
      Osc_B_Pitch_Env_C,
      Osc_B_Fluct,
      Osc_B_Fluct_Env_C,
      Osc_B_PM_Self_Env_B,
      Osc_B_PM_Self_Shp,
      Osc_B_PM_A_Env_A,
      Osc_B_PM_A_Shp,
      Osc_B_PM_FB_Env_C,
      Osc_B_Chirp,
      Shp_B_Drive_Env_B,
      Shp_B_FB_Env_C,
      Comb_Flt_Pitch,
      Comb_Flt_Pitch_KT,
      Comb_Flt_Pitch_Env_C,
      Comb_Flt_Decay,
      Comb_Flt_Decay_KT,
      Comb_Flt_Decay_Gate,
      Comb_Flt_AP_Tune,
      Comb_Flt_AP_KT,
      Comb_Flt_AP_Env_C,
      Comb_Flt_AP_Res,
      Comb_Flt_LP_Tune,
      Comb_Flt_LP_KT,
      Comb_Flt_LP_Env_C,
      Comb_Flt_PM,
      Comb_Flt_PM_A_B,
      SV_Flt_Cut,
      SV_Flt_Cut_KT,
      SV_Flt_Cut_Env_C,
      SV_Flt_Res,
      SV_Flt_Res_KT,
      SV_Flt_Res_Env_C,
      SV_Flt_Spread,
      SV_Flt_FM,
      SV_Flt_FM_A_B,
      FB_Mix_Lvl_KT,
      _LENGTH_
    };

    enum class Mono_Sync
    {
      _LENGTH_
    };

    enum class Mono_Audio
    {
      _LENGTH_
    };

    enum class Mono_Fast
    {
      Flanger_Time_Mod,
      Flanger_Feedback,
      Flanger_Cross_FB,
      Flanger_Mix,
      Flanger_Tremolo,
      Cabinet_Drive,
      Cabinet_Tilt,
      Cabinet_Cab_Lvl,
      Cabinet_Mix,
      Gap_Flt_Bal,
      Gap_Flt_Mix,
      Echo_Feedback,
      Echo_Cross_FB,
      Echo_Mix,
      Echo_Send,
      Reverb_Mix,
      Reverb_Send,
      _LENGTH_
    };

    enum class Mono_Slow
    {
      Flanger_Phase,
      Flanger_Rate,
      Flanger_Time,
      Flanger_Stereo,
      Flanger_Hi_Cut,
      Flanger_Envelope,
      Flanger_AP_Mod,
      Flanger_AP_Tune,
      Cabinet_Fold,
      Cabinet_Asym,
      Cabinet_Hi_Cut,
      Cabinet_Lo_Cut,
      Gap_Flt_Center,
      Gap_Flt_Stereo,
      Gap_Flt_Gap,
      Gap_Flt_Res,
      Echo_Time,
      Echo_Stereo,
      Echo_Hi_Cut,
      Reverb_Size,
      Reverb_Pre_Dly,
      Reverb_Color,
      Reverb_Chorus,
      _LENGTH_
    };

  }  // namespace C15::Smoothers

  namespace Signals
  {

    enum class Global_Signals
    {
      Master_Tune,
      Scale_Base_Key_Pos,
      Scale_Base_Key_Start,
      Scale_Base_Key_Dest,
      Scale_Offset_0,
      Scale_Offset_1,
      Scale_Offset_2,
      Scale_Offset_3,
      Scale_Offset_4,
      Scale_Offset_5,
      Scale_Offset_6,
      Scale_Offset_7,
      Scale_Offset_8,
      Scale_Offset_9,
      Scale_Offset_10,
      Scale_Offset_11,
      _LENGTH_
    };

    enum class Quasipoly_Signals
    {
      Osc_A_PM_Self_Shp,
      Osc_A_PM_B_Shp,
      Osc_A_Phase,
      Osc_A_Reset,
      Osc_A_Chirp,
      Shp_A_Fold,
      Shp_A_Asym,
      Shp_A_Mix,
      Shp_A_FB_Mix,
      Shp_A_Ring_Mod,
      Osc_B_PM_Self_Shp,
      Osc_B_PM_A_Shp,
      Osc_B_Phase,
      Osc_B_Reset,
      Osc_B_Chirp,
      Shp_B_Fold,
      Shp_B_Asym,
      Shp_B_Mix,
      Shp_B_FB_Mix,
      Shp_B_Ring_Mod,
      Comb_Flt_In_A_B,
      Comb_Flt_AP_Res,
      Comb_Flt_PM,
      Comb_Flt_PM_A_B,
      SV_Flt_In_A_B,
      SV_Flt_Comb_Mix,
      SV_Flt_FM_A_B,
      SV_Flt_LBH_1,
      SV_Flt_LBH_2,
      SV_Flt_Par_1,
      SV_Flt_Par_2,
      SV_Flt_Par_3,
      SV_Flt_Par_4,
      FB_Mix_Osc,
      FB_Mix_Osc_Src,
      FB_Mix_Comb,
      FB_Mix_Comb_Src,
      FB_Mix_SVF,
      FB_Mix_SVF_Src,
      FB_Mix_FX,
      FB_Mix_FX_Src,
      FB_Mix_Rvb,
      FB_Mix_Drive,
      FB_Mix_Fold,
      FB_Mix_Asym,
      Out_Mix_Drive,
      Out_Mix_Fold,
      Out_Mix_Asym,
      Out_Mix_Lvl,
      _LENGTH_
    };

    enum class Truepoly_Signals
    {
      Unison_PolyPhase,
      Env_A_Mag,
      Env_A_Tmb,
      Env_B_Mag,
      Env_B_Tmb,
      Env_C_Clip,
      Env_C_Uncl,
      Env_G_Sig,
      Osc_A_Freq,
      Osc_A_Fluct_Env_C,
      Osc_A_PM_Self_Env_A,
      Osc_A_PM_B_Env_B,
      Osc_A_PM_FB_Env_C,
      Shp_A_Drive_Env_A,
      Shp_A_FB_Env_C,
      Osc_B_Freq,
      Osc_B_Fluct_Env_C,
      Osc_B_PM_Self_Env_B,
      Osc_B_PM_A_Env_A,
      Osc_B_PM_FB_Env_C,
      Shp_B_Drive_Env_B,
      Shp_B_FB_Env_C,
      Comb_Flt_Freq,
      Comb_Flt_Bypass,
      Comb_Flt_Freq_Env_C,
      Comb_Flt_Decay,
      Comb_Flt_AP_Freq,
      Comb_Flt_LP_Freq,
      SV_Flt_F1_Cut,
      SV_Flt_F2_Cut,
      SV_Flt_F1_FM,
      SV_Flt_F2_FM,
      SV_Flt_Res_Damp,
      SV_Flt_Res_FMax,
      FB_Mix_Lvl,
      FB_Mix_HPF,
      Out_Mix_A_Left,
      Out_Mix_A_Right,
      Out_Mix_B_Left,
      Out_Mix_B_Right,
      Out_Mix_Comb_Left,
      Out_Mix_Comb_Right,
      Out_Mix_SVF_Left,
      Out_Mix_SVF_Right,
      _LENGTH_
    };

    enum class Mono_Signals
    {
      Flanger_Time_Mod,
      Flanger_Tremolo,
      Flanger_LFO_L,
      Flanger_LFO_R,
      Flanger_Time_L,
      Flanger_Time_R,
      Flanger_FB_Local,
      Flanger_FB_Cross,
      Flanger_LPF,
      Flanger_DRY,
      Flanger_WET,
      Flanger_APF_L,
      Flanger_APF_R,
      Cabinet_Drive,
      Cabinet_Fold,
      Cabinet_Asym,
      Cabinet_Pre_Sat,
      Cabinet_Sat,
      Cabinet_Tilt,
      Cabinet_LPF,
      Cabinet_HPF,
      Cabinet_Dry,
      Cabinet_Wet,
      Gap_Flt_Res,
      Gap_Flt_LF_L,
      Gap_Flt_HF_L,
      Gap_Flt_LF_R,
      Gap_Flt_HF_R,
      Gap_Flt_HP_LP,
      Gap_Flt_In_LP,
      Gap_Flt_HP_Out,
      Gap_Flt_LP_Out,
      Gap_Flt_In_Out,
      Echo_Send,
      Echo_Time_L,
      Echo_Time_R,
      Echo_FB_Local,
      Echo_FB_Cross,
      Echo_LPF,
      Echo_Dry,
      Echo_Wet,
      Reverb_Chorus,
      Reverb_Send,
      Reverb_Size,
      Reverb_Feedback,
      Reverb_Bal,
      Reverb_Pre,
      Reverb_HPF,
      Reverb_LPF,
      Reverb_Dry,
      Reverb_Wet,
      _LENGTH_
    };

  }  // namespace C15::Signals

  namespace PID
  {
    enum ParameterID
    {
      None = -1,
      Env_A_Att = 0,
      Env_A_Dec_1 = 2,
      Env_A_BP = 4,
      Env_A_Dec_2 = 6,
      Env_A_Sus = 8,
      Env_A_Rel = 10,
      Env_A_Gain = 12,
      Env_A_Lvl_Vel = 14,
      Env_A_Att_Vel = 15,
      Env_A_Rel_Vel = 16,
      Env_A_Lvl_KT = 17,
      Env_A_Time_KT = 18,
      Env_B_Att = 19,
      Env_B_Dec_1 = 21,
      Env_B_BP = 23,
      Env_B_Dec_2 = 25,
      Env_B_Sus = 27,
      Env_B_Rel = 29,
      Env_B_Gain = 31,
      Env_B_Lvl_Vel = 33,
      Env_B_Att_Vel = 34,
      Env_B_Rel_Vel = 35,
      Env_B_Lvl_KT = 36,
      Env_B_Time_KT = 37,
      Env_C_Att = 38,
      Env_C_Dec_1 = 40,
      Env_C_BP = 42,
      Env_C_Dec_2 = 44,
      Env_C_Rel = 46,
      Env_C_Lvl_Vel = 48,
      Env_C_Att_Vel = 49,
      Env_C_Rel_Vel = 50,
      Env_C_Lvl_KT = 51,
      Env_C_Time_KT = 52,
      Osc_A_Pitch = 53,
      Osc_A_Pitch_KT = 55,
      Osc_A_Pitch_Env_C = 56,
      Osc_A_Fluct = 57,
      Osc_A_Fluct_Env_C = 59,
      Osc_A_PM_Self = 60,
      Osc_A_PM_Self_Env_A = 62,
      Osc_A_PM_Self_Shp = 63,
      Osc_A_PM_B = 64,
      Osc_A_PM_B_Env_B = 66,
      Osc_A_PM_B_Shp = 67,
      Osc_A_PM_FB = 68,
      Osc_A_PM_FB_Env_C = 70,
      Shp_A_Drive = 71,
      Shp_A_Drive_Env_A = 73,
      Shp_A_Fold = 74,
      Shp_A_Asym = 75,
      Shp_A_Mix = 76,
      Shp_A_FB_Mix = 78,
      Shp_A_FB_Env_C = 80,
      Shp_A_Ring_Mod = 81,
      Osc_B_Pitch = 83,
      Osc_B_Pitch_KT = 85,
      Osc_B_Pitch_Env_C = 86,
      Osc_B_Fluct = 87,
      Osc_B_Fluct_Env_C = 89,
      Osc_B_PM_Self = 90,
      Osc_B_PM_Self_Env_B = 92,
      Osc_B_PM_Self_Shp = 93,
      Osc_B_PM_A = 94,
      Osc_B_PM_A_Env_A = 96,
      Osc_B_PM_A_Shp = 97,
      Osc_B_PM_FB = 98,
      Osc_B_PM_FB_Env_C = 100,
      Shp_B_Drive = 101,
      Shp_B_Drive_Env_B = 103,
      Shp_B_Fold = 104,
      Shp_B_Asym = 105,
      Shp_B_Mix = 106,
      Shp_B_FB_Mix = 108,
      Shp_B_FB_Env_C = 110,
      Shp_B_Ring_Mod = 111,
      Comb_Flt_In_A_B = 113,
      Comb_Flt_Pitch = 115,
      Comb_Flt_Pitch_KT = 117,
      Comb_Flt_Pitch_Env_C = 118,
      Comb_Flt_Decay = 119,
      Comb_Flt_Decay_KT = 121,
      Comb_Flt_AP_Tune = 123,
      Comb_Flt_AP_KT = 125,
      Comb_Flt_AP_Env_C = 126,
      Comb_Flt_AP_Res = 127,
      Comb_Flt_LP_Tune = 129,
      Comb_Flt_LP_KT = 131,
      Comb_Flt_LP_Env_C = 132,
      Comb_Flt_PM = 133,
      Comb_Flt_PM_A_B = 135,
      SV_Flt_In_A_B = 136,
      SV_Flt_Comb_Mix = 138,
      SV_Flt_Cut = 140,
      SV_Flt_Cut_KT = 142,
      SV_Flt_Cut_Env_C = 143,
      SV_Flt_Res = 144,
      SV_Flt_Res_KT = 146,
      SV_Flt_Res_Env_C = 147,
      SV_Flt_Spread = 148,
      SV_Flt_LBH = 150,
      SV_Flt_Par = 152,
      SV_Flt_FM = 153,
      SV_Flt_FM_A_B = 155,
      FB_Mix_Comb = 156,
      FB_Mix_SVF = 158,
      FB_Mix_FX = 160,
      FB_Mix_Rvb = 162,
      FB_Mix_Drive = 164,
      FB_Mix_Fold = 166,
      FB_Mix_Asym = 167,
      FB_Mix_Lvl_KT = 168,
      Out_Mix_A_Lvl = 169,
      Out_Mix_A_Pan = 171,
      Out_Mix_B_Lvl = 172,
      Out_Mix_B_Pan = 174,
      Out_Mix_Comb_Lvl = 175,
      Out_Mix_Comb_Pan = 177,
      Out_Mix_SVF_Lvl = 178,
      Out_Mix_SVF_Pan = 180,
      Out_Mix_Drive = 181,
      Out_Mix_Fold = 183,
      Out_Mix_Asym = 184,
      Out_Mix_Lvl = 185,
      Out_Mix_Key_Pan = 187,
      Cabinet_Drive = 188,
      Cabinet_Fold = 190,
      Cabinet_Asym = 191,
      Cabinet_Tilt = 192,
      Cabinet_Hi_Cut = 194,
      Cabinet_Lo_Cut = 196,
      Cabinet_Cab_Lvl = 197,
      Cabinet_Mix = 199,
      Gap_Flt_Center = 201,
      Gap_Flt_Stereo = 203,
      Gap_Flt_Gap = 204,
      Gap_Flt_Res = 206,
      Gap_Flt_Bal = 207,
      Gap_Flt_Mix = 209,
      Flanger_Time_Mod = 211,
      Flanger_Phase = 213,
      Flanger_Rate = 214,
      Flanger_Time = 216,
      Flanger_Stereo = 218,
      Flanger_Feedback = 219,
      Flanger_Cross_FB = 221,
      Flanger_Hi_Cut = 222,
      Flanger_Mix = 223,
      Echo_Time = 225,
      Echo_Stereo = 227,
      Echo_Feedback = 229,
      Echo_Cross_FB = 231,
      Echo_Hi_Cut = 232,
      Echo_Mix = 233,
      Reverb_Size = 235,
      Reverb_Pre_Dly = 237,
      Reverb_Color = 238,
      Reverb_Chorus = 240,
      Reverb_Mix = 241,
      MC_A = 243,
      MC_B = 244,
      MC_C = 245,
      MC_D = 246,
      Master_Volume = 247,
      Master_Tune = 248,
      Unison_Voices = 249,
      Unison_Detune = 250,
      Unison_Phase = 252,
      Unison_Pan = 253,
      Pedal_1 = 254,
      Ped_1_to_MC_A = 255,
      Ped_1_to_MC_B = 256,
      Ped_1_to_MC_C = 257,
      Ped_1_to_MC_D = 258,
      Pedal_2 = 259,
      Ped_2_to_MC_A = 260,
      Ped_2_to_MC_B = 261,
      Ped_2_to_MC_C = 262,
      Ped_2_to_MC_D = 263,
      Pedal_3 = 264,
      Ped_3_to_MC_A = 265,
      Ped_3_to_MC_B = 266,
      Ped_3_to_MC_C = 267,
      Ped_3_to_MC_D = 268,
      Pedal_4 = 269,
      Ped_4_to_MC_A = 270,
      Ped_4_to_MC_B = 271,
      Ped_4_to_MC_C = 272,
      Ped_4_to_MC_D = 273,
      Bender = 274,
      Bend_to_MC_A = 275,
      Bend_to_MC_B = 276,
      Bend_to_MC_C = 277,
      Bend_to_MC_D = 278,
      Aftertouch = 279,
      AT_to_MC_A = 280,
      AT_to_MC_B = 281,
      AT_to_MC_C = 282,
      AT_to_MC_D = 283,
      Ribbon_1 = 284,
      Rib_1_to_MC_A = 285,
      Rib_1_to_MC_B = 286,
      Rib_1_to_MC_C = 287,
      Rib_1_to_MC_D = 288,
      Ribbon_2 = 289,
      Rib_2_to_MC_A = 290,
      Rib_2_to_MC_B = 291,
      Rib_2_to_MC_C = 292,
      Rib_2_to_MC_D = 293,
      Env_A_Att_Curve = 294,
      Env_B_Att_Curve = 295,
      Env_C_Att_Curve = 296,
      Env_C_Sus = 297,
      FB_Mix_Lvl = 299,
      Osc_A_Phase = 301,
      Osc_B_Phase = 302,
      Osc_A_Chirp = 303,
      Osc_B_Chirp = 304,
      Comb_Flt_Decay_Gate = 305,
      Flanger_Envelope = 307,
      Flanger_AP_Mod = 308,
      Flanger_AP_Tune = 310,
      Scale_Base_Key = 312,
      Scale_Offset_1 = 313,
      Scale_Offset_2 = 314,
      Scale_Offset_3 = 315,
      Scale_Offset_4 = 316,
      Scale_Offset_5 = 317,
      Scale_Offset_6 = 318,
      Scale_Offset_7 = 319,
      Scale_Offset_8 = 320,
      Scale_Offset_9 = 321,
      Scale_Offset_10 = 322,
      Scale_Offset_11 = 323,
      MC_Time_A = 324,
      MC_Time_B = 325,
      MC_Time_C = 326,
      MC_Time_D = 327,
      Env_A_Dec_1_Vel = 328,
      Env_A_Dec_2_Vel = 330,
      Env_A_Elevate = 332,
      Env_B_Dec_1_Vel = 334,
      Env_B_Dec_2_Vel = 336,
      Env_B_Elevate = 338,
      Env_C_Retr_H = 340,
      Echo_Send = 342,
      Reverb_Send = 344,
      FB_Mix_Osc = 346,
      FB_Mix_Osc_Src = 348,
      FB_Mix_Comb_Src = 350,
      FB_Mix_SVF_Src = 352,
      FB_Mix_FX_Src = 354,
      Split_Split_Point = 356,
      Voice_Grp_Volume = 358,
      Voice_Grp_Tune = 360,
      Out_Mix_To_FX = 362,
      Mono_Grp_Enable = 364,
      Mono_Grp_Prio = 365,
      Mono_Grp_Legato = 366,
      Mono_Grp_Glide = 367,
      MC_E = 369,
      MC_Time_E = 370,
      MC_F = 371,
      MC_Time_F = 372,
      Ped_1_to_MC_E = 373,
      Ped_1_to_MC_F = 374,
      Ped_2_to_MC_E = 375,
      Ped_2_to_MC_F = 376,
      Ped_3_to_MC_E = 377,
      Ped_3_to_MC_F = 378,
      Ped_4_to_MC_E = 379,
      Ped_4_to_MC_F = 380,
      Bend_to_MC_E = 381,
      Bend_to_MC_F = 382,
      AT_to_MC_E = 383,
      AT_to_MC_F = 384,
      Rib_1_to_MC_E = 385,
      Rib_1_to_MC_F = 386,
      Rib_2_to_MC_E = 387,
      Rib_2_to_MC_F = 388,
      Flanger_Tremolo = 389,
      Scale_Offset_0 = 391,
      Osc_A_Reset = 393,
      Osc_B_Reset = 394,
      Voice_Grp_Mute = 395,
      Voice_Grp_Fade_From = 396,
      Voice_Grp_Fade_Range = 397
    };

  }  // namespace C15::PID

}  // namespace C15
