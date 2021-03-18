#include <cmath>
#include <smfp/get_frequency.hpp>

namespace smfp {
  float get_frequency( const channel_state_t &cst, const active_note_t &nst ) {
    return std::exp2f( ( ( float( nst.channel_note & 0xFF ) + cst.get_pitch_bend() + 3.f ) / 12.f ) ) * 6.875f;
  }
}

