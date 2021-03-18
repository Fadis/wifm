#ifndef SMFP_WAVESINK_HPP
#define SMFP_WAVESINK_HPP

#include <cstdint>
#include <array>
#include <algorithm>
#include <sndfile.h>

namespace smfp {
  class wavesink {
  public:
    wavesink( const char *filename, uint32_t sample_rate );
    ~wavesink();
    void play_if_not_playing() const;
    bool buffer_is_ready() const;
    void operator()( const float data );
    template< size_t i >
    void operator()( const std::array< int16_t, i > &data ) {
      sf_write_short( file, data.data(), i );
    }
    template< size_t i >
    void operator()( const std::array< float, i > &data ) {
      std::array< int16_t, i > idata;
      std::transform( data.begin(), data.end(), idata.begin(), []( const float &value ) { return int16_t( value * 32767 ); } );
      sf_write_short( file, idata.data(), i );
    }
    void operator()( const std::vector< float > &data ) {
      std::vector< int16_t > idata( data.size() );
      std::transform( data.begin(), data.end(), idata.begin(), []( const float &value ) { return int16_t( value * 32767 ); } );
      sf_write_short( file, idata.data(), idata.size() );
    }
  private:
    SF_INFO config;
    SNDFILE* file;
  };
}

#endif

