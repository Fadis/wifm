/*
Copyright (c) 2020 Naomasa Matsubayashi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <boost/container/flat_map.hpp>
#include "ifm/fft.h"
#include "ifm/spectrum_image.h"

namespace ifm {
  spectrum_image::spectrum_image(
    uint8_t note_,
    float sample_rate_,
    uint32_t resolution_
  ) :  fft( resolution_, sample_rate_ ), note( note_ ), resolution( resolution_ ), sample_rate( sample_rate_ ) {
  }
  std::tuple< std::vector< float >, float > spectrum_image::operator()(
    const std::vector< float > &audio
  ) {
    const float expected = float( 24 ) / resolution * sample_rate;
    const float freq = std::exp2( ( ( float( note ) +  3.f ) / 12.f ) ) * 6.875f;
    const float step = expected / freq;
    const unsigned int block_size = ( resolution * step ) + 1;
    unsigned int interval = float( sample_rate ) / 100;
    const auto width = get_width();
    std::vector< float > resampled( resolution );
    std::vector< float > results;
    unsigned int delay = 0;
    bool in_delay = true;
    for( unsigned int offset = 0; offset + block_size < audio.size(); offset += interval ) {
      resample( audio.data() + offset, resampled.data(), step );
      auto result = fft.forward( resampled );
      result.resize( width );
      unsigned int active_width = result.size() / 24;
      if( in_delay ) {
        float sum = 0.f;
        for( unsigned int x = 1; x != active_width; ++x )
          sum += result[ x * 24 ];
        if( sum < 0.0001f ) {
          ++delay;
        }
        else
          in_delay = false;
      }
      if( !in_delay )
        results.insert( results.end(), result.begin(), result.end() );
    }
    return std::make_tuple( results, delay * 0.01 );
  }
  unsigned int spectrum_image::get_width() const {
    return 20000.f * 24.f / ( std::exp2( ( ( float( note ) + 3.f ) / 12.f ) ) * 6.875f );
    //return float( resolution ) * 20000.f / sample_rate;
  }
  void spectrum_image::resample( const float *src, float *dest, float step ) const {
    for( unsigned int i = 0; i != resolution; ++i ) {
      float pos = step * i;
      unsigned int index = std::floor( pos );
      dest[ i ] = interpolate( src[ index ], src[ index + 1 ], pos - index );
    }
  }
  float spectrum_image::interpolate( float a, float b, float c ) {
    return a * ( 1.f - c ) + b * c;
  }
  std::tuple<
    std::vector< float >,
    std::vector< float >
  >
  get_important_samples(
    const std::vector< float > &images,
    unsigned int width,
    const std::vector< float > &ec,
    unsigned int y,
    float thres,
    unsigned int max_samples
  ) {
    std::vector< float > freqs;
    std::vector< float > weights;
    {
      boost::container::flat_map< float, float > top;
      boost::container::flat_map< float, float > sorted;
      for( unsigned int x = 0; x != width; ++x ) {
        const auto v = images[ y * width + x ];
        top.insert( std::make_pair( v, float( x ) ) );
      }
      auto begin = std::next( top.begin(), std::max( int( top.size() ) - int( max_samples ), 0 ) );
      auto end = top.end();
      sorted.clear();
      for( auto iter = begin; iter != end; ++iter ) {
        sorted.insert( std::make_pair( iter->second, iter->first ) );
      }
      for( auto &[x,v]: sorted ) {
        if( ec[ y ] != 0 && x/24.f < thres ) {
          freqs.push_back( x/24.f );
          weights.push_back( v / ec[ y ] );
        }
      }
    }
    return std::make_tuple( std::move( freqs ), std::move( weights ) ); 
  }
  std::tuple<
    std::vector< float >,
    float,
    unsigned int
  > get_envelope(
    const std::vector< float > &images,
    unsigned int width
  ) {
    unsigned int highest = 0u;
    float highest_sum = 0.f;
    const unsigned int height = images.size() / width;
    std::vector< float > ec( height );
    for( unsigned int y = 0; y != height; ++y ) {
      float sum = 0;
      for( unsigned int x = 0; x != width; ++x )
        sum += images[ y * width + x ];
      ec[ y ] = sum;
      if( sum > highest_sum ) {
        highest_sum = sum;
        highest = y;
      }
      if( ec[ y ] == 0 ) {
        ec.resize( y );
        ec.shrink_to_fit();
        break;
      }
    }
    return std::make_tuple( std::move( ec ), highest_sum, highest );
  }
}

