#ifndef SMFP_GET_VOLUME_HPP
#define SMFP_GET_VOLUME_HPP

#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>

namespace smfp {
  float get_volume( const channel_state_t &cst, const active_note_t &nst );
}

#endif

