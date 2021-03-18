#ifndef SMFP_GLOBAL_STATE_HPP
#define SMFP_GLOBAL_STATE_HPP

#include <string>
#include <stamp/setter.hpp>

namespace smfp {
  enum class midi_standard_id_t {
    gm,
    gm2,
    xg,
    gs
  };
  std::string to_string( midi_standard_id_t v );
  struct global_state_t {
    global_state_t() : standard( midi_standard_id_t::gm ) {}
    LIBSTAMP_SETTER( standard )
    midi_standard_id_t standard;
  };
}

#endif

