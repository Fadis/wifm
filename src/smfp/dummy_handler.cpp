#include <iostream>
#include <smfp/dummy_handler.hpp>
#include <smfp/get_volume.hpp>
#include <smfp/get_frequency.hpp>

namespace smfp {
  void dummy_handler::note_on( const channel_state_t &cst, const active_note_t &nst ) {
    std::cout << "note_on " << int( nst.channel_note & 0xFF ) << " " << int( cst.program ) << " " << get_frequency( cst, nst ) << " " << get_volume( cst, nst ) << std::endl;
    std::cout << "  " << cst.to_json().dump() << std::endl;
    note_info = nst;
  }
  void dummy_handler::clear( const channel_state_t & ) {
    std::cout << "clear " << int( note_info.channel_note & 0xFF ) << std::endl;
  }
  void dummy_handler::note_off( const channel_state_t & ) {
    std::cout << "note_off " << int( note_info.channel_note & 0xFF ) << std::endl;
  }
  void dummy_handler::set_program( const channel_state_t&,  uint8_t value ) {
    std::cout << "set_program " << int( value ) << std::endl;
  }
  void dummy_handler::set_variable( channel_variable_id_t id, note_t at, const channel_state_t &cst ) {
    std::cout << "set_variable " << int( id ) << "[" << int( at ) << "] = " << cst[ id ] << std::endl;
  }
  void dummy_handler::set_volume( const channel_state_t &, float value ) {
    std::cout << "set_volume " << value << std::endl;
  }
  void dummy_handler::set_frequency( const channel_state_t &, float value ) {
    std::cout << "set_frequency " << value << std::endl;
  }
}

