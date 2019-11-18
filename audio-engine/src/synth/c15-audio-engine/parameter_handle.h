#pragma once

/******************************************************************************/
/** @file       parameter_handle.h
    @date
    @version    1.7-0
    @author     M. Seeber
    @brief      new main parameter handle
    @todo
*******************************************************************************/

#include "parameter_storage.h"
#include "parameter-db/generated/parameter_list.h"

namespace Engine
{

  namespace Handle
  {

    struct Clock_Handle
    {
      uint32_t m_index = 0, m_div_fast = 0, m_div_slow = 0;
      bool m_fast = false, m_slow = false;
      inline void init(const uint32_t _samplerate)
      {
        m_div_fast = _samplerate / C15::Config::clock_rates[0][1];
        m_div_slow = _samplerate / C15::Config::clock_rates[0][2];
      }
      inline void render()
      {
        m_fast = (m_index % m_div_fast) == 0;
        m_index = m_index % m_div_slow;
        m_slow = m_index == 0;
        m_index++;
      }
    };

    struct Time_Handle
    {
      float m_convert[4] = {}, m_dx_audio = 0.0f, m_dx_fast = 0.0f, m_dx_slow = 0.0f, m_millisecond = 0.0f,
            m_sample_inc = 0.0f;
      inline void init(const uint32_t _samplerate)
      {
        float rate = static_cast<float>(_samplerate);
        m_millisecond = 1e-3f * rate;
        m_sample_inc = 1.0f / rate;
        m_convert[1] = 1.0f;
        m_convert[2] = rate / static_cast<float>(C15::Config::clock_rates[0][1]);
        m_convert[3] = rate / static_cast<float>(C15::Config::clock_rates[0][2]);
      }
      inline float clip(const float _value)
      {
        return _value > 1.0f ? 1.0f : _value;
      }
      inline float eval_ms(const uint32_t _type, const float _value)
      {
        return clip(m_convert[_type] / ((_value * m_millisecond) + 1.0f));
      }
      inline void update_ms(const float _value)
      {
        m_dx_audio = eval_ms(1, _value);
        m_dx_fast = eval_ms(2, _value);
        m_dx_slow = eval_ms(3, _value);
      }
    };

    template <class Layer> struct Parameter_Handle
    {
      Layer_Storage m_layer[static_cast<uint32_t>(Layer::_LENGTH_)];
      Global_Storage m_global;
      const uint32_t m_layer_count = static_cast<uint32_t>(Layer::_LENGTH_);
      inline void init_modMatrix()
      {
        for(uint32_t l = 0; l < m_layer_count; l++)
        {
          m_layer[l].m_assignment.reset();
        }
        for(uint32_t s = 0; s < m_global.m_source_count; s++)
        {
          auto source = m_global.m_source[s];
          source.m_offset = s * m_layer[0].m_macro_count;
          for(uint32_t i = 0; i < m_layer[0].m_macro_count; i++)
          {
            const uint32_t index = source.m_offset + i;
            for(uint32_t l = 0; l < m_layer_count; l++)
            {
              m_layer[l].m_amount[index].m_sourceId = s;
            }
          }
        }
      }
      inline void init_macro(const C15::ParameterDescriptor _element)
      {
        for(uint32_t i = 0; i < m_layer_count; i++)
        {
          auto param = get_macro(i, _element.m_param.m_index);
          param->m_id = _element.m_param.m_index;
          param->m_index = _element.m_param.m_index - 1;
          param->m_base = param->m_position = _element.m_initial;
        }
      }
      inline void init_macro_time(const C15::ParameterDescriptor _element)
      {
        for(uint32_t i = 0; i < m_layer_count; i++)
        {
          auto param = get_macro(i, _element.m_param.m_index);
          param->m_time.init(_element.m_ae.m_scaleId, _element.m_ae.m_scaleFactor, _element.m_ae.m_scaleOffset,
                             _element.m_initial);
        }
      }
      inline void init_modulateable(const C15::ParameterDescriptor _element)
      {
        for(uint32_t i = 0; i < m_layer_count; i++)
        {
          auto param = get_target(i, _element.m_param.m_index);
          param->m_polarity = _element.m_ae.m_polarity;
          param->m_render.m_section = _element.m_ae.m_smoother.m_section;
          param->m_render.m_clock = _element.m_ae.m_smoother.m_clock;
          param->m_render.m_index = _element.m_ae.m_smoother.m_index;
          param->m_scaling.m_id = _element.m_ae.m_scaleId;
          param->m_scaling.m_factor = _element.m_ae.m_scaleFactor;
          param->m_scaling.m_offset = _element.m_ae.m_scaleOffset;
          param->m_position = param->depolarize(_element.m_initial);
        }
      }
      inline void init_unmodulateable(const C15::ParameterDescriptor _element)
      {
        for(uint32_t i = 0; i < m_layer_count; i++)
        {
          auto param = get_direct(i, _element.m_param.m_index);
          param->m_render.m_section = _element.m_ae.m_smoother.m_section;
          param->m_render.m_clock = _element.m_ae.m_smoother.m_clock;
          param->m_render.m_index = _element.m_ae.m_smoother.m_index;
          param->m_scaling.m_id = _element.m_ae.m_scaleId;
          param->m_scaling.m_factor = _element.m_ae.m_scaleFactor;
          param->m_scaling.m_offset = _element.m_ae.m_scaleOffset;
          param->m_position = _element.m_initial;
        }
      }
      inline void init_global(const C15::ParameterDescriptor _element)
      {
        auto param = get_global(_element.m_param.m_index);
        param->m_render.m_section = _element.m_ae.m_smoother.m_section;
        param->m_render.m_clock = _element.m_ae.m_smoother.m_clock;
        param->m_render.m_index = _element.m_ae.m_smoother.m_index;
        param->m_scaling.m_id = _element.m_ae.m_scaleId;
        param->m_scaling.m_factor = _element.m_ae.m_scaleFactor;
        param->m_scaling.m_offset = _element.m_ae.m_scaleOffset;
        param->m_position = _element.m_initial;
      }
      inline HW_Src_Param* get_hw_src(const uint32_t _id)
      {
        return &m_global.m_source[_id];
      }
      inline Direct_Param* get_global(const uint32_t _id)
      {
        return &m_global.m_direct[_id];
      }
      inline HW_Amt_Param* get_hw_amt(const uint32_t _layerId, const uint32_t _id)
      {
        return &m_layer[_layerId].m_amount[_id];
      }
      inline Macro_Param* get_macro(const uint32_t _layerId, const uint32_t _id)
      {
        return &m_layer[_layerId].m_macro[_id];
      }
      inline Time_Param* get_macro_time(const uint32_t _layerId, const uint32_t _id)
      {
        return &m_layer[_layerId].m_macro[_id].m_time;
      }
      inline Direct_Param* get_direct(const uint32_t _layerId, const uint32_t _id)
      {
        return &m_layer[_layerId].m_direct[_id];
      }
      inline Target_Param* get_target(const uint32_t _layerId, const uint32_t _id)
      {
        return &m_layer[_layerId].m_target[_id];
      }
      inline HW_Src_Param* get(const C15::Parameters::Hardware_Sources _id)
      {
        return &m_global.m_source[static_cast<uint32_t>(_id)];
      }
      inline Direct_Param* get(const C15::Parameters::Global_Parameters _id)
      {
        return &m_global.m_direct[static_cast<uint32_t>(_id)];
      }
      inline HW_Amt_Param* get(const Layer _layerId, const C15::Parameters::Hardware_Amounts _id)
      {
        return &m_layer[static_cast<uint32_t>(_layerId)].m_amount[static_cast<uint32_t>(_id)];
      }
      inline Macro_Param* get(const Layer _layerId, const C15::Parameters::Macro_Controls _id)
      {
        return &m_layer[static_cast<uint32_t>(_layerId)].m_macro[static_cast<uint32_t>(_id)];
      }
      inline Direct_Param* get(const Layer _layerId, const C15::Parameters::Unmodulateable_Parameters _id)
      {
        return &m_layer[static_cast<uint32_t>(_layerId)].m_direct[static_cast<uint32_t>(_id)];
      }
      inline Target_Param* get(const Layer _layerId, const C15::Parameters::Modulateable_Parameters _id)
      {
        return &m_layer[static_cast<uint32_t>(_layerId)].m_target[static_cast<uint32_t>(_id)];
      }
      inline Target_Param* chainFirst(const uint32_t _layer, const uint32_t _mcId)
      {
        return &m_layer[_layer].m_target[m_layer[_layer].m_assignment.first(_mcId)];
      }
      inline bool chainRunning(const uint32_t _layer)
      {
        return m_layer[_layer].m_assignment.running();
      }
      inline Target_Param* chainNext(const uint32_t _layer)
      {
        return &m_layer[_layer].m_target[m_layer[_layer].m_assignment.next()];
      }
    };

  }  // namepsace Engine::Handle

  using Param_Handle = Handle::Parameter_Handle<C15::Properties::LayerId>;

}  // namespace Engine
