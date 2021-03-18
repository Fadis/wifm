#ifndef SMFP_MIXER_HPP
#define SMFP_MIXER_HPP

#include <cmath>
#include <chrono>
#include <limits>

namespace smfp {
  class mixer_t {
  public:
    mixer_t( std::chrono::nanoseconds step_ );
    template< typename Channels >
    float operator()( Channels &channels ) {
      float val_sum = 0.0f;
      float env_sum = 0.0f;
      for( auto &channel: channels ) {
        const auto [env,val] = channel( step );
        val_sum += val;
        if( env != -std::numeric_limits< float >::infinity() ) {
          env_sum += std::pow( 10.f, env / 40.f );
        }
      }
      auto env_sum_db = 40.f * std::log10( env_sum );
      requested_scale = get_scale( env_sum_db );
      if( current_scale < requested_scale )
        current_scale += ( requested_scale - current_scale ) * spms;
      else
        current_scale += ( requested_scale - current_scale ) * spms / 100.f;
      float value = val_sum * std::pow( 10.f, -current_scale / 40.f );
      if( env_sum_db - current_scale > 0.f && value > 1.f ) {
        current_scale = env_sum_db;
        value = std::min( std::max( value, -1.f ), 1.f );
      }
      return val_sum * std::pow( 10.f, -current_scale / 40.f );
    }
    template< typename Channels, typename Iterator >
    void operator()( Channels &channels, Iterator begin, Iterator end ) {
      for( auto iter = begin; iter != end; ++iter )
        *iter = ( *this )( channels );
    }
  public:
    float get_scale( float x ) const;
    float current_scale;
    float requested_scale;
    std::chrono::nanoseconds step;
    float spms;
  };
}

#endif

