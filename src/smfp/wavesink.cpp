#include <smfp/wavesink.hpp>

namespace smfp {
  wavesink::wavesink( const char *filename, uint32_t sample_rate ) {
    config.frames = 0;
    config.samplerate = sample_rate;
    config.channels = 1;
    config.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
    config.sections = 0;
    config.seekable = 1;
    file = sf_open( filename, SFM_WRITE, &config );
  }
  wavesink::~wavesink() {
    sf_write_sync( file );
    sf_close( file );
  }
  void wavesink::play_if_not_playing() const {
  }
  bool wavesink::buffer_is_ready() const {
    return true;
  }
  void wavesink::operator()( const float data ) {
    int16_t ibuf[ 1 ];
    std::transform( &data, &data + 1, ibuf, []( const float &value ) { return int16_t( value * 32767 ); } );
    sf_write_short( file, ibuf, 1 );
  }
}

