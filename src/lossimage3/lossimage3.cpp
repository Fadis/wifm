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
#include <charconv>
#include <random>
#include <boost/container/flat_map.hpp>
#include <boost/program_options.hpp>
#include <omp.h>
#include <nlohmann/json.hpp>
#include "ifm/spectrum_image.h"
#include "ifm/load_monoral.h"
#include "ifm/bessel.h"
//#include "ifm/fm.h"
#include "ifm/adam.h"
#include "ifm/2op.h"
#include "ifm/exp_match.h"
#include <wasserstein.h>
int main( int argc, char *argv[] ) {
  boost::program_options::options_description options("オプション");
  options.add_options()
    ("help,h",    "ヘルプを表示")
    ("bessel,b", boost::program_options::value<std::string>()->default_value( "bessel.mp" ), "ベッセル関数近似係数")
    ("input,i", boost::program_options::value<std::string>(), "入力ファイル")
    ("resolution,r", boost::program_options::value<int>()->default_value(13),  "分解能")
    ("damped,d", boost::program_options::value<bool>()->default_value(false),  "減衰振動")
    ("note,n", boost::program_options::value<int>()->default_value(60), "音階")
    ("verbose,v", boost::program_options::value<bool>()->default_value(false), "詳細を表示");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("input") ) {
    std::cout << options << std::endl;
    return 0;
  }

  std::vector< std::pair< float, float > > pre;
  {
    std::ifstream bessel_file( params["bessel"].as<std::string>(), std::ofstream::binary );
    nlohmann::json bessel = nlohmann::json::from_msgpack( bessel_file );
    for( const auto &v: bessel )
      pre.emplace_back( v.at( 0 ), v.at( 1 ) );
  }

  const auto [audio,sample_rate] = ifm::load_monoral( params["input"].as<std::string>(), true );
  ifm::spectrum_image conv( params["note"].as<int>(), sample_rate, 1 << params["resolution"].as<int>() );
  auto [image,delay] = conv( audio );
  unsigned int width = conv.get_width();
  auto [ec,highest_sum,highest] = ifm::get_envelope( image, width );
  auto [expected_freqs,expected_weights] = ifm::get_important_samples(
    image, width, ec, highest, 20.f, 40
  );
  //const auto expected_sum = std::accumulate( expected_weights.begin(), expected_weights.end(), 0.f );
  //for( auto &v: expected_weights ) v /= expected_sum;
  wasserstein::wasserstein_t< float > wass;
  std::vector< float > generated_freqs;
  std::vector< float > generated_weights;
  std::vector< float > n;
  std::array< float, ifm::max_harmony + 3 > bessel;
  for( auto wi = 0u; wi != 24 * 20; ++wi ) {
    const float w = wi/24.f;
    for( auto b = 0.f; b < 20.f; b += 0.1f ) {
      ifm::generate_bessel( b, pre, bessel );
      ifm::forward_2op_nofb( w, bessel, generated_freqs, generated_weights, n );
      auto thres = std::lower_bound( generated_freqs.begin(), generated_freqs.end(), 20.f );
      if( thres != generated_freqs.end() ) {
        const auto under_thres = std::distance( generated_freqs.begin(), thres );
        generated_freqs.resize( under_thres );
        generated_weights.resize( under_thres );
      }
      //const auto generated_sum = std::accumulate( generated_weights.begin(), generated_weights.end(), 0.f );
      //for( auto &v: generated_weights ) v /= generated_sum;
      auto diff = wass.forward( expected_freqs, expected_weights, generated_freqs, generated_weights );
      std::cout << w << "\t" << b << "\t" << diff << std::endl;
    }
    std::cout << std::endl;
  }
}

