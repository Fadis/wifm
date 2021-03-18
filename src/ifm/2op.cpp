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

#include <cstdint>
#include <cmath>
#include <array>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <boost/container/flat_map.hpp>
#include <omp.h>
#include "ifm/bessel.h"
#include "ifm/adam.h"
#include "ifm/2op.h"
namespace ifm {
  std::tuple< float, float > lossimage(
    const float *expected,
    const std::vector< std::pair< float, float > > &pre,
    unsigned int,
    float b,
    const std::array< int, max_harmony > &n
  ) {
    constexpr float a = 1;
    std::array< float, max_harmony + 3 > bessel{ 0 };
    std::array< float, max_harmony > generated{ 0 };
    float l1 = 0;
    float l2 = 0;
    for( unsigned int i = 0; i < ( max_harmony + 3 ); ++i ) {
      bessel[ i ] = ifm::bessel_kind1_approx_2019( b, i, l1, l2, pre );
      l2 = l1;
      l1 = bessel[ i ];
    }
    float bscale = 0;
    for( unsigned int i = 0; i != max_harmony; ++i ) {
      bscale += std::abs( bessel[ i ] );
      if( i < max_harmony - 2 ) bscale += std::abs( bessel[ i + 2 ] );
    }
    for( unsigned int i = 0; i != max_harmony - 2; ++i ) {
      if( n[ i ] != -1 ) generated[ i ] = a * bessel[ n[ i ] ]/bscale;
      else generated[ i ] = 0;
      if( i < max_harmony - 2 && n[ i + 2 ] != -1 )
        generated[ i ] += a * bessel[ n[ i + 2 ] ]/bscale;
    }
    float loss = 0;
    float d = 0;
    for( unsigned int i = 0; i != max_harmony; ++i ) {
      if( n[ i ] != -1 ) {
        float e2 = std::sqrt( expected[ i ] * expected[ i ] );
          float g2 = std::sqrt( generated[ i ] * generated[ i ] );
          float s = e2 - g2;
          loss += std::sqrt( s * s );
          d += std::sqrt( s * s );
      }
      else {
        float e2 = std::sqrt( expected[ i ] * expected[ i ] );
        loss += e2;
      }
    }
    return std::make_tuple( loss, d );
  }
  void generate_bessel(
    float b,
    const std::vector< std::pair< float, float > > &pre,
    std::array< float, max_harmony + 3 > &bessel
  ) {
    float l1 = 0;
    float l2 = 0;
    for( auto i = 0u; i != max_harmony - 2; ++i ) {
      bessel[ i ] = ifm::bessel_kind1_approx_2019( b, i, l1, l2, pre );
      l2 = l1;
      l1 = bessel[ i ];
    }
  }
  void forward_2op_nofb(
    float w,
    const std::array< float, max_harmony + 3 > &bessel,
    std::vector< float > &generated_freqs,
    std::vector< float > &generated_weights,
    std::vector< float > &n
  ) {
    generated_freqs.clear();
    generated_weights.clear();
    n.clear();
    boost::container::flat_multimap< float, std::pair< int, float > > generated;
    for( unsigned int i = 0; i != max_harmony - 2; ++i ) {
      float scale = bessel[ i ] ;
      float freq = std::abs( 1 + i * w );
      if( freq != 0.f && freq < 20000.f ) {
        generated.insert( std::make_pair( freq, std::make_pair( int( i ), scale ) ) );
      }
    }
    for( unsigned int i = 1; i != max_harmony - 2; ++i ) {
      float scale = bessel[ i ] ;
      float freq = std::abs( 1 - i * w );
      if( freq != 0.0f && freq < 20000.f ) {
        generated.insert( std::make_pair( freq, std::make_pair( -int( i ), scale ) ) );
      }
    }
    std::transform(
      generated.begin(), generated.end(),
      std::back_inserter( generated_freqs ),
      []( const auto &v ) { return v.first; }
    );
    std::transform(
      generated.begin(), generated.end(),
      std::back_inserter( generated_weights ),
      []( const auto &v ) { return std::abs( v.second.second ); }
    );
    std::transform(
      generated.begin(), generated.end(),
      std::back_inserter( n ),
      []( const auto &v ) { return std::abs( v.second.first ); }
    );
  }
  std::tuple< float, float > backward_2op_nofb(
    float w,
    const std::array< float, max_harmony + 3 > &bessel,
    const std::vector< float > &n,
    const std::vector< float > &generated_freqs_grad,
    const std::vector< float > &generated_weights_grad
  ) {
    float db = 0.f;
    float dw = 0.f;
    for( unsigned int i = 0; i != generated_freqs_grad.size(); ++i ) {
      const std::array< float, 5 > j{
        bessel[ std::abs( n[ i ] - 1 ) ],
        bessel[ std::abs( n[ i ] ) ],
        bessel[ std::abs( n[ i ] + 1 ) ],
        bessel[ std::abs( n[ i ] + 2 ) ],
        bessel[ std::abs( n[ i ] + 3 ) ]
      };
      const auto dfm_b = 0.5f * ( j[ 0 ] - j[ 2 ] );
      const auto freq = 1 + n[ i ] * w;
      auto dfm_w = 0.f;
      if( freq >= 1.f ) dfm_w = n[ i ];
      else if( freq >= 0.f ) dfm_w = -n[ i ];
      else dfm_w = n[ i ];
      dw += dfm_w * generated_freqs_grad[ i ];
      db += dfm_b * generated_weights_grad[ i ];
    }
    return std::make_tuple( dw, db );
  }
  float backward_2op_nofb(
    const std::array< float, max_harmony + 3 > &bessel,
    const std::vector< float > &n,
    const std::vector< float > &generated_weights_grad
  ) {
    float db = 0.f;
    for( unsigned int i = 0; i != generated_weights_grad.size(); ++i ) {
      const std::array< float, 5 > j{
        bessel[ std::abs( n[ i ] - 1 ) ],
        bessel[ std::abs( n[ i ] ) ],
        bessel[ std::abs( n[ i ] + 1 ) ],
        bessel[ std::abs( n[ i ] + 2 ) ],
        bessel[ std::abs( n[ i ] + 3 ) ]
      };
      const auto dfm_b = 0.5f * ( j[ 0 ] - j[ 2 ] );
      db += dfm_b * generated_weights_grad[ i ];
    }
    return db;
  }

  std::tuple< float, float > find_b_2op(
    const float *expected,
    const std::vector< std::pair< float, float > > &pre,
    unsigned int,
    float b,
    const std::array< int, max_harmony > &n
  ) {
    constexpr float a = 1;
    std::array< float, max_harmony + 3 > bessel{ 0 };
    std::array< float, max_harmony > generated{ 0 };
    float loss = 0;
    ifm::adam< float > bopt( 0.001, 0.9, 0.999 );
    for( unsigned int cycle = 0; cycle != 50000; ++cycle ) {
      float l1 = 0;
      float l2 = 0;
      for( unsigned int i = 0; i < ( max_harmony + 3 ); ++i ) {
        bessel[ i ] = ifm::bessel_kind1_approx_2019( b, i, l1, l2, pre );
        l2 = l1;
        l1 = bessel[ i ];
      }
      float bscale = 0;
      for( unsigned int i = 0; i != max_harmony; ++i ) {
        bscale += std::abs( bessel[ i ] );
        if( i < max_harmony - 2 ) bscale += std::abs( bessel[ i + 2 ] );
      }
      float dbscale = 0.5 * ( bessel[ 0 ] - bessel[ 2 ] ) + bessel[ 2 ] - bessel[ max_harmony - 4 ] - bessel[ max_harmony - 3 ] + 0.5 * ( bessel[ max_harmony - 4 ] - bessel[ max_harmony - 2 ] ) + 0.5 * ( bessel[ max_harmony - 3 ] - bessel[ max_harmony - 1 ] );
      for( unsigned int i = 0; i != max_harmony - 2; ++i ) {
        if( n[ i ] != -1 ) generated[ i ] = a * bessel[ n[ i ] ]/bscale;
        else generated[ i ] = 0;
        if( i < max_harmony - 2 && n[ i + 2 ] != -1 )
          generated[ i ] += a * bessel[ n[ i + 2 ] ]/bscale;
      }
      float grad_b = 0;
      loss = 0;
      for( unsigned int i = 0; i != max_harmony; ++i ) {
        if( n[ i ] != -1 ) {
          float e2 = std::sqrt( expected[ i ] * expected[ i ] );
          float g2 = std::sqrt( generated[ i ] * generated[ i ] );
          float s = e2 - g2;
          loss += std::sqrt( s * s );
          std::array< float, 5 > j{
            bessel[ n[ i ] - 1 ],
            bessel[ n[ i ] ],
            bessel[ n[ i ] + 1 ],
            bessel[ n[ i ] + 2 ],
            bessel[ n[ i ] + 3 ]
          };
          float j13 = j[1]+j[3];
          float aaj13 = j13*j13;
          float grad_b_b1 = e2-std::sqrt(aaj13/(bscale*bscale));
          if( grad_b_b1 != 0 ) {
            if( aaj13 > 1.0e-20 ) {
              float grad_b__ = (0.5*j13*
               (e2 - std::sqrt(aaj13/(bscale*bscale)))*
               ((-j[0] + j[4])*bscale + 
                (2.*j[1] + 2.*j[3])*dbscale))/
                (std::sqrt(grad_b_b1*grad_b_b1)*std::sqrt(aaj13/(bscale*bscale))*
                 (bscale*bscale*bscale));
              grad_b += grad_b__;
            }
          }
        }
        else {
          float e2 = std::sqrt( expected[ i ] * expected[ i ] );
          loss += e2;
        }
      }
      float diff = bopt( grad_b );
      b -= diff;
      b = std::max( b, 0.f );
    }
    return std::make_tuple( loss, b );
  }
  std::tuple< float, float > optimize_2op_nofb::operator()(
    const std::vector< float > &expected_freqs,
    const std::vector< float > &expected_weights
  ) {
    ifm::adam< float > wopt( 0.001f*100, 0.9f, 0.999f );
    ifm::adam< float > bopt( 0.001f*100, 0.9f, 0.999f );
    w = 5.f;
    b = 5.f;
    float best_w = w;
    float best_b = b;
    float lowest_diff = std::numeric_limits< float >::max();
    for( unsigned int cycle = 0; cycle != 20000; ++cycle ) {
      ifm::generate_bessel( b, pre, bessel );
      ifm::forward_2op_nofb( w, bessel, generated_freqs, generated_weights, n );
      const auto thres_iter = std::lower_bound( generated_freqs.begin(), generated_freqs.end(), float( thres ) );
      if( thres_iter != generated_freqs.end() ) {
        const auto under_thres = std::distance( generated_freqs.begin(), thres_iter );
        generated_freqs.resize( under_thres );
        generated_weights.resize( under_thres );
      }
      const auto diff = wass.forward( expected_freqs, expected_weights, generated_freqs, generated_weights );
      if( diff < lowest_diff ) {
        lowest_diff = diff;
        best_w = w;
        best_b = b;
      }
      const auto [ generated_freqs_grad, generated_weights_grad ] = wass.backward(
        expected_freqs,
        expected_weights,
        generated_freqs,
        generated_weights,
        diff,
        0.0001f,
        0.0001f
      );
      const auto [ dw, db ] = backward_2op_nofb(
        w,
        bessel,
        n,
        generated_freqs_grad,
        generated_weights_grad
      );
      float w_diff = wopt( dw );
      w += w_diff;
      float b_diff = bopt( db );
      b += b_diff;
    }
    w = std::abs( best_w );
    b = std::abs( best_b );
    return std::make_tuple( w, b );
  }
  float optimize_2op_nofb::operator()(
    const std::vector< float > &expected_freqs,
    const std::vector< float > &expected_weights,
    float w_
  ) {
    ifm::adam< float > bopt( 0.001f*100, 0.9f, 0.999f );
    w = w_;
    b = 5.f;
    float best_b = b;
    float lowest_diff = std::numeric_limits< float >::max();
    for( unsigned int cycle = 0; cycle != 20000; ++cycle ) {
      ifm::generate_bessel( b, pre, bessel );
      ifm::forward_2op_nofb( w, bessel, generated_freqs, generated_weights, n );
      const auto thres_iter = std::lower_bound( generated_freqs.begin(), generated_freqs.end(), float( thres ) );
      if( thres_iter != generated_freqs.end() ) {
        const auto under_thres = std::distance( generated_freqs.begin(), thres_iter );
        generated_freqs.resize( under_thres );
        generated_weights.resize( under_thres );
      }
      const auto diff = wass.forward( expected_freqs, expected_weights, generated_freqs, generated_weights );
      if( diff < lowest_diff ) {
        lowest_diff = diff;
        best_b = b;
      }
      const auto generated_weights_grad = wass.backward_partial(
        expected_freqs,
        expected_weights,
        generated_freqs,
        generated_weights,
        diff,
        0.0001f
      );
      const auto db = backward_2op_nofb(
        bessel,
        n,
        generated_weights_grad
      );
      float b_diff = bopt( db );
      b += b_diff;
    }
    b = std::abs( best_b );
    //std::cout << lowest_diff << " " << best_b << std::endl;
    return b;
  }
  float optimize_2op_nofb::operator()(
    const std::vector< float > &expected_freqs,
    const std::vector< float > &expected_weights,
    float w_,
    float b_
  ) {
    ifm::adam< float > bopt( 0.001f, 0.9f, 0.999f );
    w = w_;
    b = b_;
    float best_b = b;
    const float orig_b = b;
    float lowest_diff = std::numeric_limits< float >::max();
    for( unsigned int retry = 0; retry != 5; ++retry ) {
      for( unsigned int cycle = 0; cycle != 1000; ++cycle ) {
        ifm::generate_bessel( b, pre, bessel );
        ifm::forward_2op_nofb( w, bessel, generated_freqs, generated_weights, n );
        const auto thres_iter = std::lower_bound( generated_freqs.begin(), generated_freqs.end(), float( thres ) );
        auto full_size = generated_freqs.size();
        if( thres_iter != generated_freqs.end() ) {
          const auto under_thres = std::distance( generated_freqs.begin(), thres_iter );
          generated_freqs.resize( under_thres );
          generated_weights.resize( under_thres );
        }
        auto reduced_size = generated_freqs.size();
        const auto diff = wass.forward( expected_freqs, expected_weights, generated_freqs, generated_weights );
        if( diff < lowest_diff ) {
          lowest_diff = diff;
          best_b = b;
        }
        const auto generated_weights_grad = wass.backward_partial(
          expected_freqs,
          expected_weights,
          generated_freqs,
          generated_weights,
          diff,
          0.0001f
        );
        const auto db = backward_2op_nofb(
          bessel,
          n,
          generated_weights_grad
        );
        if( std::isnan( db ) ) {
          std::cout << "=== begin " << full_size << " " << reduced_size << std::endl;
          for( auto v: generated_freqs ) std::cout << v << " ";
          std::cout << std::endl;
          for( auto v: generated_weights ) std::cout << v << " ";
          std::cout << std::endl;
          for( auto v: generated_weights_grad ) std::cout << v << " ";
          std::cout << std::endl;
          std::cout << "=== end" << std::endl;
        }
        const float b_diff = bopt( db );
        b += b_diff;
      }
      if( orig_b != b && lowest_diff < 1.f ) break;
    }
    b = std::abs( best_b );
    //std::cout << lowest_diff << " " << best_b << std::endl;
    return b;
  }
  std::array< int, max_harmony > generate_n(
    unsigned int freq
  ) {
    std::array< int, max_harmony > n;
    std::fill( n.begin(), n.end(), -1 );
    for( int i = 0; i != max_harmony; ++i )
      if( i % freq == 0 )
        n[ i ] = i / freq;
    return n;
  }
  std::tuple< float, float > find_b_2op(
    const float *expected,
    const std::vector< std::pair< float, float > > &pre,
    unsigned int freq
  ) {
    float lowest_loss = std::numeric_limits< float >::max();
    float best_b = 0;
    std::array< int, max_harmony > n = generate_n( freq );
    for( unsigned int try_count = 0; try_count != 5; ++try_count ) {
      const auto [loss,b] = find_b_2op( expected, pre, freq, try_count, n );
      if( lowest_loss > loss ) {
        best_b = b;
        lowest_loss = loss;
      }
    }
    return std::make_tuple( lowest_loss, best_b );
  }
  std::tuple< float, unsigned int, float > find_b_2op(
    const float *expected,
    const std::vector< std::pair< float, float > > &pre
  ) {
    constexpr unsigned int min_freq = 1;
    constexpr unsigned int max_freq = 20;
    std::vector< std::tuple< float, float > > results( max_freq );
#pragma omp parallel for
    for( unsigned int freq = min_freq; freq < max_freq; ++freq ) {
      results[ freq ] = find_b_2op( expected, pre, freq );
    }
    float lowest_loss = std::numeric_limits< float >::max();
    float best_b = 0;
    unsigned int best_freq = 0;
    for( unsigned int freq = min_freq; freq != max_freq; ++freq ) {
      if( lowest_loss > std::get< 0 >( results[ freq ] ) ) {
        best_b = std::get< 1 >( results[ freq ] );
        best_freq = freq;
        lowest_loss = std::get< 0 >( results[ freq ] );
      }
    }
    return std::make_tuple( lowest_loss, best_freq, best_b );
  }
}

