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
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <omp.h>
#include <smfp/envelope_generator.hpp>
#include "ifm/exp_match.h"
#include "ifm/adam.h"
namespace ifm {
  namespace {
    float
    get_squared_error( const float *begin, const float *end, float b ) {
      if( begin == end ) return 0.f;
      const auto last = std::prev( end );
      if( begin == last ) return 0.f;
      const auto a = *begin;
      const float scale = std::distance( begin, last );
      size_t i = 0;
      float sum = 0;
      for( auto iter = begin; iter != end; ++iter, ++i ) {
        const auto pos = float( i ) / scale;
        const auto l = ( 1.f - pos ) * a + pos * b;
        const auto diff = l - *iter;
        sum += diff * diff;
      }
      return sum;
    }
    std::vector< float > get_smoothed( const float *begin, const float *end ) {
      std::vector< float > temp;
      temp.push_back( *begin );
      temp.push_back( *begin );
      temp.push_back( *begin );
      temp.insert( temp.end(), begin, end );
      temp.push_back( temp.back() );
      temp.push_back( temp.back() );
      temp.push_back( temp.back() );
      std::vector< float > smoothed;
      for( auto i = 3u; i != temp.size() - 3u; ++i )
        smoothed.push_back(
          0.036f * temp[ i - 3u ] +
          0.113f * temp[ i - 2u ] +
          0.216f * temp[ i - 1u ] +
          0.269f * temp[ i + 0u ] +
          0.216f * temp[ i + 1u ] +
          0.113f * temp[ i + 2u ] +
          0.036f * temp[ i + 3u ]
        );
      auto max = smoothed[ 0 ];
      for( auto &v: smoothed ) v += ( 0.f - max );
      return smoothed;
    }
    std::tuple< float, float, float, float >
    linear_match( const float *expected, unsigned int size, float dt ) {
      float lowest_error = std::numeric_limits< float >::max();
      size_t lowest_sep1 = 0;
      size_t lowest_sep2 = 0;
      float level1 = 0;
      float level2 = 0;
      std::vector< float > smoothed = get_smoothed( expected, expected + size );
      for( size_t sep1 = 0; sep1 != size; ++sep1 ) {
        for( size_t sep2 = sep1; sep2 != size; ++sep2 ) {
          const auto error =
            get_squared_error( smoothed.data(), std::next( smoothed.data(), sep1 ), *std::next( smoothed.data(), sep1 ) ) +
            get_squared_error( std::next( smoothed.data(), sep1 ), std::next( smoothed.data(), sep2 ), *std::next( smoothed.data(), sep2 ) ) +
            get_squared_error( std::next( smoothed.data(), sep2 ), std::next( smoothed.data(), size ), *std::next( smoothed.data(), sep2 ) );
          if( lowest_error >= error ) {
            lowest_error = error;
            lowest_sep1 = sep1;
            lowest_sep2 = sep2;
            level1 = smoothed[ sep1 ];
            level2 = smoothed[ sep2 ];
          }
        }
      }
      return std::make_tuple( lowest_sep1 * dt, ( lowest_sep2 - lowest_sep1 ) * dt, level1, level2 );
    }
    std::tuple< float, float, float >
    linear_match_without_sustain( const float *expected, unsigned int size, float dt ) {
      std::cout << "f " << size << std::endl;
      float lowest_error = std::numeric_limits< float >::max();
      size_t lowest_sep1 = 0;
      float level1 = 0;
      std::vector< float > smoothed = get_smoothed( expected, expected + size );
      for( size_t sep1 = 0; sep1 != size; ++sep1 ) {
        const auto error =
          get_squared_error( smoothed.data(), std::next( smoothed.data(), sep1 ), *std::next( smoothed.data(), sep1 ) ) +
          get_squared_error( std::next( smoothed.data(), sep1 ), std::next( smoothed.data(), size ), -5 );
        if( lowest_error >= error ) {
          lowest_error = error;
          lowest_sep1 = sep1;
          level1 = smoothed[ sep1 ];
        }
      }
      return std::make_tuple( lowest_sep1 * dt, ( size - lowest_sep1 ) * dt, level1 );
    }
  }
  smfp::envelope_generator_config_t
  get_attack_and_decay_linear( const float *expected, unsigned int size, float dt, bool damped ) {
    std::vector< float > expected_db;
    expected_db.reserve( size );
    std::transform( expected, expected + size, std::back_inserter( expected_db ), []( auto v ) { return 40 * std::log10( v ); } );
    const auto highest = std::max_element( expected_db.begin(), expected_db.end() );
    const auto highest_pos = std::distance( expected_db.begin(), highest );
    if( highest_pos == 0 ) damped = true;
    const auto [ decay1, decay2, decay_mid, sustain ] = linear_match( std::next( expected_db.data(), highest_pos ), size - highest_pos, dt );
    if( damped ) {
      return smfp::envelope_generator_config_t()
        .set_default_attack2( 0.01 )
        .set_default_decay1( decay1 )
        .set_default_decay2( decay2 )
        .set_decay_mid( ( decay_mid + 48 ) / 48 )
        .set_sustain( ( sustain + 48 ) / 48 )
        .set_default_release( 1.0 );
    }
    else {
      std::vector< float > reversed( std::make_reverse_iterator( highest ), std::make_reverse_iterator( expected_db.begin() ) );
      const auto [attack2, attack1, attack_mid ] = linear_match_without_sustain( reversed.data(), highest_pos, dt );
      return smfp::envelope_generator_config_t()
        .set_default_attack1( attack1 )
        .set_default_attack2( attack2 )
        .set_default_decay1( decay1 )
        .set_default_decay2( decay2 )
        .set_attack_mid( ( attack_mid + 48 ) / 48 )
        .set_decay_mid( ( decay_mid + 48 ) / 48 )
        .set_sustain( ( sustain + 48 ) / 48 )
        .set_default_release( 1.0 );
    }
  }
}
