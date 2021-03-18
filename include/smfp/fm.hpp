#ifndef SMFP_FM_HPP
#define SMFP_FM_HPP
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>
#include <cmath>
#include <chrono>
#include <array>
#include <limits>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <type_traits>
#include <cstdint>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <nlohmann/json.hpp>
#include <smfp/get_frequency.hpp>
#include <stamp/setter.hpp>
namespace smfp {
  nlohmann::json is_valid_fm_config(
    const nlohmann::json &config,
    size_t operator_count
  );
  template< size_t operator_count = 2u >
  struct fm_config_t {
    fm_config_t() : scale( 1 ) {
      std::fill( modulation.begin(), modulation.end(), 0 );
    }
    fm_config_t( const nlohmann::json &config ) : scale( 1 ) {
      std::fill( modulation.begin(), modulation.end(), 0 );
      is_valid_fm_config( config, operator_count );
#define SMFP_FM_PARAM_WITH_DEFAULT( name, default ) \
  if( config.find( # name ) == config.end() ) name = default ; \
  else name = config[ # name ];
      SMFP_FM_PARAM_WITH_DEFAULT( scale, 1 )
      size_t i = 0;
      if( config.find( "modulation" ) != config.end() )
        for( const auto &v: config[ "modulation" ] )
          modulation[ i++ ] = float( v );
    }
    nlohmann::json dump() const {
      return {
        { "scale", scale },
        { "modulation", modulation }
      };
    }
    LIBSTAMP_SETTER( scale )
    LIBSTAMP_SETTER( modulation )
    float scale;
    std::array< float, operator_count > modulation;
  };
  template< size_t operator_count >
  fm_config_t< operator_count > lerp(
    const fm_config_t< operator_count > &l,
    const fm_config_t< operator_count > &r,
    float pos
  ) {
    fm_config_t< operator_count > temp;
    temp.set_scale( std::lerp( l.scale, r.scale, pos ) );
    for( size_t i = 0u; i != operator_count; ++i )
      temp.modulation[ i ] = std::lerp( l.modulation[ i ], r.modulation[ i ], pos );
    return temp;
  }
  template< size_t operator_count = 2u >
  class fm_t {
  public:
    using config_type = fm_config_t< operator_count >;
    fm_t( const fm_config_t< operator_count > &config_ ) : config( config_ ), at( 0 ), shift( 0 ), tangent( 0 ) {}
    fm_t( const nlohmann::json &config_ ) : 
      fm_t( fm_config_t< operator_count >( config_ ) ) {}
    nlohmann::json dump() const {
      return {
        { "config", config.dump() },
        { "at", at },
        { "shift", shift },
        { "tangent", tangent }
      };
    }
    void set_config( const channel_state_t&, const fm_config_t< operator_count > &config_ ) {
      config = config_;
    }
    void note_on( const channel_state_t &cst, const active_note_t &nst ) {
      at = 0;
      shift = 0; 
      tangent = get_frequency( cst, nst ) * config.scale;
    }
    void set_frequency( const channel_state_t &/*cst*/, float freq ) {
      auto new_tangent = freq * config.scale;
      shift = shift + ( tangent - new_tangent ) * at;
      tangent = new_tangent;
    }
    template< typename Iterator >
    float operator()( std::chrono::nanoseconds step, Iterator input ) {
      auto diff = std::accumulate( config.modulation.begin(), config.modulation.end(), 0.f, [&]( auto sum, auto v ) {
        return sum + *( input++ ) * v;
      });
      auto value = std::sin( ( diff + tangent * at + shift ) * float( M_PI ) * 2.f );
      at += std::chrono::duration_cast< std::chrono::duration< float > >( step ).count();
      if( at >= 1.f ) {
        shift = tangent * at;
        at = 0;
      }
      return value;
    }
  private:
    fm_config_t< operator_count > config;
    float at;
    float shift;
    float tangent;
  };
}
#endif

