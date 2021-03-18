#ifndef SMFP_EXCEPTIONS_HPP
#define SMFP_EXCEPTIONS_HPP
#include <stdexcept>
namespace smfp {
#define SMFP_EXCEPTION( base, name ) \
  struct name : public std:: base { \
    using std:: base :: base ; \
    name () : std:: base( # name ) {} \
  };
  SMFP_EXCEPTION( runtime_error, invalid_variable )
  SMFP_EXCEPTION( runtime_error, invalid_variable_length_quantity )
  SMFP_EXCEPTION( runtime_error, invalid_smf_header )
  SMFP_EXCEPTION( runtime_error, invalid_smf_track )
  SMFP_EXCEPTION( runtime_error, invalid_midi_message )
  SMFP_EXCEPTION( runtime_error, invalid_midi_operation )
  SMFP_EXCEPTION( runtime_error, slot_lost )
  SMFP_EXCEPTION( runtime_error, invalid_instrument_config )
}
#endif

