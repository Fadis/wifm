#include <smfp/mixer.hpp>

namespace smfp {
  mixer_t::mixer_t( std::chrono::nanoseconds step_ ) :
    current_scale( 0 ),
    requested_scale( 0 ),
    step( step_ ),
    spms( std::chrono::duration_cast< std::chrono::duration< float > >( step_ ).count() * 1000.f ) {}
  float mixer_t::get_scale( float x ) const {
    if( x < -20.f ) return 0;
    else if( x < 0.f ) return ( 1.f / 40.f ) * x * x + x + 10.f;
    else return x + 10.f;
  }
}

