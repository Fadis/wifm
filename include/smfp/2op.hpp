#ifndef SMFP_2OP_HPP
#define SMFP_2OP_HPP

#include <chrono>
#include <stamp/setter.hpp>
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>
#include <smfp/fmeg.hpp>

namespace smfp {
  struct fm_2op_nofb_config_t {
    fm_2op_nofb_config_t() {}
    fm_2op_nofb_config_t( const nlohmann::json &config );
    nlohmann::json dump() const;
    LIBSTAMP_SETTER( upper )
    LIBSTAMP_SETTER( lower )
    fmeg_config_t< 1u > lower;
    fmeg_config_t< 0u > upper;
  };
  fm_2op_nofb_config_t lerp(
    const fm_2op_nofb_config_t &,
    const fm_2op_nofb_config_t &,
    float
  );
  class fm_2op_nofb_t {
  public:
    using config_type = fm_2op_nofb_config_t;
    fm_2op_nofb_t( const fm_2op_nofb_config_t &config );
    fm_2op_nofb_t( const nlohmann::json &config );
    nlohmann::json dump() const;
    void set_config( const channel_state_t &cst, const fm_2op_nofb_config_t &config );
    void note_on( const channel_state_t &cst, const active_note_t &nst );
    void note_off( const channel_state_t &cst );
    void clear( const channel_state_t &cst );
    void set_frequency( const channel_state_t &cst, float freq );
    std::tuple< float, float > operator()( std::chrono::nanoseconds step );
    template< typename Iterator >
    void operator()( std::chrono::nanoseconds step, Iterator begin, Iterator end ) {
      for( auto iter = begin; iter != end; ++iter )
        *iter = std::get< 1 >( (*this)( step ) );
    }
    void set_variable( channel_variable_id_t /*id*/, note_t /*at*/, const channel_state_t &/*cst*/ );
    void set_program( const channel_state_t&,  uint8_t );
    void set_volume( const channel_state_t &cst, float value );
    template< typename Iterator >
    void system_exclusive( const channel_state_t &, Iterator, Iterator ) {
    }
    bool is_end() const;
  private:
    fmeg_t< 1u > lower;
    fmeg_t< 0u > upper;
  };
}

#endif

