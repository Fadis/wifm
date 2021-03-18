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
int main( int argc, char *argv[] ) {
  boost::program_options::options_description options("オプション");
  options.add_options()
    ("help,h",    "ヘルプを表示")
    ("bessel,b", boost::program_options::value<std::string>()->default_value( "bessel.mp" ), "ベッセル関数近似係数")
    ("input,i", boost::program_options::value<std::string>(), "入力ファイル")
    ("resolution,r", boost::program_options::value<int>()->default_value(13),  "分解能")
    ("damped,d", boost::program_options::value<bool>()->default_value(false),  "減衰振動")
    ("note,n", boost::program_options::value<int>()->default_value(60), "音階")
    ("omega,w", boost::program_options::value<float>()->default_value(0), "ω")
    ("thres,t", boost::program_options::value<unsigned int>()->default_value(20), "何倍音まで比較の対象にするか")
    ("verbose,v", boost::program_options::value<bool>()->default_value(false), "詳細を表示");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("input") ) {
    std::cout << options << std::endl;
    return 0;
  }
  auto pre = ifm::load_bessel_approx_2019_precomp_array( params["bessel"].as<std::string>() );
  const auto [audio,sample_rate] = ifm::load_monoral( params["input"].as<std::string>(), true );
  ifm::spectrum_image conv( params["note"].as<int>(), sample_rate, 1 << params["resolution"].as<int>() );
  auto [image,delay] = conv( audio );
  const unsigned int width = conv.get_width();
  const unsigned int height = image.size() / width;
  std::vector< float > freqs;
  std::vector< float > weights;
  boost::container::flat_map< float, float > top;
  boost::container::flat_map< float, float > sorted;
  auto [ec,highest_sum,highest] = ifm::get_envelope( image, width );
  ifm::optimize_2op_nofb optimizer(
    std::move( pre ), params["thres"].as<unsigned int>()
  );
  std::vector< float > em( ec.size(), 0 );
  float w = 0.f;
  if( !params[ "omega" ].as< float >() ) {
    auto [expected_freqs,expected_weights] = ifm::get_important_samples(
      image, width, ec, highest, 20.f, 20
    );
    auto [w_,b] = optimizer(
      std::move( expected_freqs ),
      std::move( expected_weights )
    );
    em[ highest ] = b;
    w = w_;
    std::cout << highest * 0.01f << " " << w << " " << em[ highest ] << std::endl;
  }
  else {
    auto [expected_freqs,expected_weights] = ifm::get_important_samples(
      image, width, ec, highest, 20.f, 20
    );
    auto w_ = params[ "omega" ].as< float >(); 
    auto b = optimizer(
      std::move( expected_freqs ),
      std::move( expected_weights ),
      w_
    );
    em[ highest ] = b;
    w = w_;
    std::cout << highest * 0.01f << " " << w << " " << em[ highest ] << std::endl;
  }
  for( size_t y = highest + 1u; y != height; ++y ) {
    auto [expected_freqs,expected_weights] = ifm::get_important_samples(
      image, width, ec, y, 20.f, 20
    );
    if( !expected_freqs.empty() ) {
      em[ y ] = optimizer(
        std::move( expected_freqs ),
        std::move( expected_weights ),
        w, em[ y - 1 ]
      );
    }
  }
  for( int y = int( highest ) - 1u; y != -1; --y ) {
    auto [expected_freqs,expected_weights] = ifm::get_important_samples(
      image, width, ec, y, 20.f, 20
    );
    if( !expected_freqs.empty() ) {
      em[ y ] = optimizer(
        std::move( expected_freqs ),
        std::move( expected_weights ),
        w, em[ y + 1 ]
      );
    }
  }
  auto highest_ec = *std::max_element( ec.begin(), ec.end() );
  for( auto &v: ec ) v /= highest_ec;
  auto highest_em = *std::max_element( em.begin(), em.end() );
  for( auto &v: em ) v /= highest_em;
  bool damped = params[ "damped" ].as< bool >();
  auto ece = ifm::get_attack_and_decay_linear( ec.data(), ec.size(), 0.01f, damped );
  auto eme = ifm::get_attack_and_decay_linear( em.data(), em.size(), 0.01f, damped );
  const nlohmann::json json = {
    { "upper", {
      { "eg", eme.dump() },
      { "fm", {
        { "scale", w },
        { "modulation", nlohmann::json::array() }
      }}
    }},
    { "lower", {
      { "eg", ece.dump() },
      { "fm", {
        { "scale", 1.0 },
        { "modulation", nlohmann::json::array( { highest_em } ) }
      }}
    }}
  };
  std::cout << json.dump( 2 ) << std::endl;
}

