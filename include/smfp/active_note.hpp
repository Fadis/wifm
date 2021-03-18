#ifndef SMFP_ACTIVE_NOTE_HPP
#define SMFP_ACTIVE_NOTE_HPP

#include <cstdint>
#include <smfp/types.hpp>
#include <stamp/setter.hpp>

namespace smfp {
  struct active_note_t {
    active_note_t() : channel_note( 0 ), order( 0 ), slot( 0 ), velocity( 0x7F ), polyphonic_key_pressure( 0x7F ) {}
    LIBSTAMP_SETTER( channel_note )
    LIBSTAMP_SETTER( order )
    LIBSTAMP_SETTER( slot )
    LIBSTAMP_SETTER( velocity )
    LIBSTAMP_SETTER( polyphonic_key_pressure )
    uint16_t channel_note;
    note_order_t order;
    slot_t slot;
    uint8_t velocity;
    uint8_t polyphonic_key_pressure;
  };
}

#endif

