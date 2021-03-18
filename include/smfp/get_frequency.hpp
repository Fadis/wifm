#ifndef SMFP_GET_FREQUENCY_HPP
#define SMFP_GET_FREQUENCY_HPP

#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>

namespace smfp {
  float get_frequency( const channel_state_t &cst, const active_note_t &nst );
}

#endif

