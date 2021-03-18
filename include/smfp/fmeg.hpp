#ifndef SMFP_FMEG_HPP
#define SMFP_FMEG_HPP

#include <cmath>
#include <chrono>
#include <tuple>
#include <nlohmann/json.hpp>
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/get_node.hpp>
#include <smfp/envelope_generator.hpp>
#include <smfp/fm.hpp>

namespace smfp {
  template< size_t i >
  struct fmeg_config_t {
    fmeg_config_t() {}
    fmeg_config_t( const nlohmann::json &config ) :
      eg( get_node( config, "eg" ) ),
      fm( get_node( config, "fm" ) ) {}
    nlohmann::json dump() const {
      return {
        { "eg", eg.dump() },
        { "fm", fm.dump() }
      };
    }
    LIBSTAMP_SETTER( eg )
    LIBSTAMP_SETTER( fm )
    envelope_generator_config_t eg;
    fm_config_t< i > fm;
  };
  template< size_t i >
  fmeg_config_t< i > lerp(
    const fmeg_config_t< i > &l,
    const fmeg_config_t< i > &r,
    float pos
  ) {
    return fmeg_config_t< i >()
      .set_eg( lerp( l.eg, r.eg, pos ) )
      .set_fm( lerp( l.fm, r.fm, pos ) );
  }

  template< size_t i >
  struct fmeg_t {
    using config_type = fmeg_config_t< i >;
    fmeg_t( const fmeg_config_t< i > &config ) :
      eg( config.eg ),
      fm( config.fm ) {}
    fmeg_t( const nlohmann::json &config ) :
      fmeg_t( fmeg_config_t< i >( config ) ) {}
    nlohmann::json dump() const {
      return {
        { "eg", eg.dump() },
        { "fm", fm.dump() }
      };
    }
    void set_config( const channel_state_t &cst, const fmeg_config_t< i > &config ) {
      eg.set_config( cst, config.eg );
      fm.set_config( cst, config.fm );
    }
    void note_on( const channel_state_t &cst, const active_note_t &nst ) {
      eg.note_on( cst, nst );
      fm.note_on( cst, nst );
    }
    void note_off( const channel_state_t &cst ) {
      eg.note_off( cst );
    }
    void clear( const channel_state_t &cst ) {
      eg.clear( cst );
    }
    void set_frequency( const channel_state_t &cst, float freq ) {
      fm.set_frequency( cst, freq );
    }
    template< typename Iterator >
    std::tuple< float, float > operator()( std::chrono::nanoseconds step, Iterator input ) {
      auto envelope = eg( step );
      if( envelope == -std::numeric_limits< float >::infinity() ) return std::make_tuple( envelope, 0.f );
      return std::make_tuple( envelope, std::pow( 10.f, envelope / 40.f ) * fm( step, input ) );
    }
    void set_volume( const channel_state_t &cst, float vol ) {
      eg.set_volume( cst, vol );
    }
    bool is_end() const {
      return eg.is_end();
    }
    envelope_generator_t eg;
    fm_t< i > fm;
  };
}

#endif

