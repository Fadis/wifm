#ifndef SMFP_DUMMY_HANDLER_HPP
#define SMFP_DUMMY_HANDLER_HPP

#include <iostream>
#include <smfp/types.hpp>
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>

namespace smfp {
  class dummy_handler {
  public:
    void note_on( const channel_state_t &cst, const active_note_t &nst );
    void clear( const channel_state_t & );
    void note_off( const channel_state_t & );
    void set_program( const channel_state_t&,  uint8_t value );
    void set_variable( channel_variable_id_t id, note_t at, const channel_state_t &cst );
    void set_volume( const channel_state_t &, float value );
    void set_frequency( const channel_state_t &, float value );
    template< typename Iterator >
    void system_exclusive( const channel_state_t &, Iterator begin, Iterator end ) {
      std::cout << "sysex " << std::hex;
      for( auto iter = begin; iter != end; ++iter )
        std::cout << uint32_t( *iter ) << " ";
      std::cout << std::dec << std::endl;
    }
  private:
    active_note_t note_info;
  };
}
#endif
