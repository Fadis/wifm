#include <cmath>
#include <limits>
#include <smfp/get_volume.hpp>

namespace smfp {
  float get_volume( const channel_state_t &cst, const active_note_t &nst ) {
    auto cv = cst.get< channel_variable_id_t::volume >();
    if( cv == -std::numeric_limits< float >::infinity() || nst.velocity == 0u || nst.polyphonic_key_pressure == 0u )
      return -std::numeric_limits< float >::infinity();
    return
      cst.get< channel_variable_id_t::volume >() +
      40 * std::log10( nst.velocity / 127.f ) +
      40 * std::log10( nst.polyphonic_key_pressure / 127.f );
  }
}

