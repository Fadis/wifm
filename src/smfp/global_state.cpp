#include <smfp/global_state.hpp>
namespace smfp {
  std::string to_string( midi_standard_id_t v ) {
    if( v == midi_standard_id_t::gm ) return "GM";
    else if( v == midi_standard_id_t::gm2 ) return "GM2";
    else if( v == midi_standard_id_t::xg ) return "XG";
    else if( v == midi_standard_id_t::gs ) return "GS";
    else return "unknown";
  }
}

