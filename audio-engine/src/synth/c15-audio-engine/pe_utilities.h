/******************************************************************************/
/** @file		pe_utilities.h
    @date       2018-03-13
    @version    1.0
    @author     Matthias Seeber
    @brief		quasi-dynamic list objects necessary for ID handling
                (tcd_decoder, param_engine)
                (currently, no debugging capabilities/functions present)
    @todo
*******************************************************************************/

#pragma once

#include <stdint.h>
#include <vector>
#include "pe_defines_config.h"
#include "pe_utilities.h"
#include "pe_defines_labels.h"

/* improving code readability */
enum class PARAM_CLOCK_TYPES
{
  PARAM_SYNC = 0,
  PARAM_AUDIO = 1,
  PARAM_FAST = 2,
  PARAM_SLOW = 3
};

enum class PARAM_POLY_TYPES
{
  PARAM_MONO = 0,
  PARAM_POLY = 1
};

enum class PARAM_SPREAD_TYPES
{
  PARAM_SINGLE = 0,
  PARAM_SPREAD = 1
};

template <typename T> struct id_list
{
  /* local variables */
  T m_data[sig_number_of_params];
  uint32_t m_length = 0;
  /* list operations */

  void reset()
  {
    m_length = 0;
  }

  void add(const T _id)
  {
    m_data[m_length] = _id;
    m_length++;
  }
};

template <typename T> struct dual_id_list
{
  /* local variables */
  id_list<T> m_data[2];
  /* list operations */

  void reset()
  {
    m_data[0].reset();
    m_data[1].reset();
  }

  void add(const uint32_t _listId, const T _id)
  {
    m_data[_listId].add(_id);
  }
};

template <typename T> struct polyDual_id_list
{
  /* local variables */
  dual_id_list<T> m_data[dsp_poly_types];
  /* list operations */

  void reset()
  {
    for(uint32_t i = 0; i < dsp_poly_types; i++)
    {
      m_data[i].reset();
    }
  }

  void add(const PARAM_POLY_TYPES _polyId, const uint32_t _listId, const T _id)
  {
    m_data[static_cast<uint32_t>(_polyId)].add(_listId, _id);
  }
};

template <typename T> struct poly_id_list
{
  /* local variables */
  id_list<T> m_data[dsp_poly_types];

  /* list operations */
  void reset()
  {
    for(uint32_t i = 0; i < dsp_poly_types; i++)
    {
      m_data[i].reset();
    }
  }

  void add(const PARAM_POLY_TYPES _polyId, const T _id)
  {
    m_data[static_cast<uint32_t>(_polyId)].add(_id);
  }
};

struct env_id_list
{
  /* local variables */
  uint32_t m_data[sig_number_of_envelopes];
  uint32_t m_length = 0;
  /* list operations */
  void reset();
  void add(const uint32_t _id);
};

struct dual_env_id_list
{
  /* local variables */
  env_id_list m_data[2];
  /* list operations */
  void reset();
  void add(const uint32_t _listId, const uint32_t _id);
};

struct new_clock_id_list
{
  inline void add(PARAM_CLOCK_TYPES _clockType, PARAM_POLY_TYPES _polyType, uint32_t _id)
  {
    m_data[static_cast<int>(_clockType)][static_cast<int>(_polyType)].push_back(_id);
  }

  inline const std::vector<uint32_t> &get(PARAM_CLOCK_TYPES _clockType, PARAM_POLY_TYPES _polyType) const
  {
    return m_data[static_cast<int>(_clockType)][static_cast<int>(_polyType)];
  }

 private:
  std::vector<uint32_t> m_data[dsp_clock_types][dsp_poly_types];
};

struct new_dual_clock_id_list
{
  inline void add(PARAM_SPREAD_TYPES _spreadType, PARAM_CLOCK_TYPES _clockType, PARAM_POLY_TYPES _polyType,
                  ParameterLabel _id)
  {
    m_data[static_cast<uint32_t>(_spreadType)][static_cast<int>(_clockType)][static_cast<int>(_polyType)].push_back(
        _id);
  }

  inline const std::vector<ParameterLabel> &get(PARAM_SPREAD_TYPES _spreadType, PARAM_CLOCK_TYPES _clockType,
                                                PARAM_POLY_TYPES _polyType) const
  {
    return m_data[static_cast<uint32_t>(_spreadType)][static_cast<int>(_clockType)][static_cast<int>(_polyType)];
  }

 private:
  std::vector<ParameterLabel> m_data[dsp_spread_types][dsp_clock_types][dsp_poly_types];
};
