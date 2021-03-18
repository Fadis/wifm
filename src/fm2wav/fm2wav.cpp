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
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include <smfp/2op.hpp>
#include <smfp/variable.hpp>
#include <smfp/wavesink.hpp>

int main( int argc, char *argv[] ) {
  boost::program_options::options_description options("オプション");
  options.add_options()
    ("help,h",    "ヘルプを表示")
    ("config,c", boost::program_options::value<std::string>(),  "設定ファイル")
    ("program,p", boost::program_options::value<std::string>()->default_value("piano"),  "楽器名")
    ("output,o", boost::program_options::value<std::string>(),  "出力ファイル")
    ("note,n", boost::program_options::value<int>()->default_value(60),  "音階");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("config") || !params.count("output") || !params.count("program") ) {
    std::cout << options << std::endl;
    return 0;
  }
  nlohmann::json config;
  {
    nlohmann::json configs;
    std::ifstream config_file( params[ "config" ].as< std::string >() );
    config_file >> configs;
    const auto prog = params[ "program" ].as< std::string >();
    if( configs[ "instruments" ].find( prog ) == configs[ "instruments" ].end() ) {
      std::cerr << "指定された楽器が存在しない" << std::endl;
      return 1;
    }
    config = configs[ "instruments" ][ prog ];
  }
  using inst_t = smfp::variable_t< smfp::fm_2op_nofb_t >;
  using config_t = inst_t::config_type;
  std::shared_ptr< config_t > config_p( new config_t( config ) );
  inst_t inst( config_p );
  smfp::global_state_t gst;
  smfp::channel_state_t cst( &gst );
  cst[ smfp::channel_variable_id_t::volume ] = 0x3FFF;
  cst[ smfp::channel_variable_id_t::expression ] = 0x3FFF;
  cst[ smfp::channel_variable_id_t::foot ] = 0x3FFF;
  cst[ smfp::channel_variable_id_t::breath ] = 0x3FFF;
  cst.set_pressure( 0x7F );
  smfp::active_note_t nst;
  nst.set_channel_note( params[ "note" ].as<int>() & 0x7F );
  inst.note_on( cst, nst );
  auto unit_step = std::chrono::nanoseconds( 1000ul * 1000ul * 1000ul / 44100ul );
  std::vector< float > buf( 441 );
  smfp::wavesink sink( params["output"].as< std::string >().c_str(), 44100 );
  for( size_t i = 0; i != 1000; ++i ) {
    inst( unit_step, buf.begin(), buf.end() );
    sink( buf );
    auto sleep = unit_step * buf.size();
    if( inst.is_end() ) break;
  }
}

