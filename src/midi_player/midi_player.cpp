#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>
#include <smfp/header.hpp>
#include <smfp/track.hpp>
#include <smfp/get_volume.hpp>
#include <smfp/get_frequency.hpp>
#include <smfp/midi_parser.hpp>
#include <smfp/dummy_handler.hpp>
#include <smfp/envelope_generator.hpp>
#include <smfp/mixer.hpp>
#include <smfp/fm.hpp>
#include <smfp/fmeg.hpp>
#include <smfp/2op.hpp>
#include <smfp/variable.hpp>
#include <smfp/multi_instruments.hpp>
#include <smfp/wavesink.hpp>
#include <chrono>
#include <array>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include <stamp/mapped_file.hpp>

int main( int argc, char* argv[] ) {
  boost::program_options::options_description options("Options");
  options.add_options()
    ("help,h",    "show this message")
    ("config,c", boost::program_options::value<std::string>(), "config file")
    ("input,i", boost::program_options::value<std::string>(), "input file")
    ("output,o", boost::program_options::value<std::string>(), "output file");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("config")|| !params.count("input") || !params.count("output") ) { 
    std::cout << options << std::endl;
    return 0;
  }
  std::ifstream config_file( params["config"].as<std::string>() );
  nlohmann::json config;
  config_file >> config;
  using inst_t = smfp::multi_instrument_t< smfp::variable_t< smfp::fm_2op_nofb_t > >;
  using config_t = inst_t::config_type;
  std::shared_ptr< config_t > config_p( new config_t( config ) );
  smfp::multi_instrument_t< smfp::variable_t< smfp::fm_2op_nofb_t > > inst( config_p );
  stamp::mapped_file f( params["input"].as< std::string >() );
  auto [iter,header] = smfp::decode_smf_header( f.begin(), f.end() );
  smfp::smf_tracks_t tracks( header, iter, f.end() );
  std::vector< inst_t > handlers( 64, inst );
  smfp::midi_parser_t midip( handlers );
  auto unit_step = std::chrono::nanoseconds( 1000ul * 1000ul * 1000ul / 44100ul );
  smfp::mixer_t mixer( unit_step );
  std::vector< float > buf( 441 );
  smfp::wavesink sink( params["output"].as< std::string >().c_str(), 44100 );
  uint64_t count = 0;
  while( !tracks.end() ) {
    mixer( handlers, buf.begin(), buf.end() );
    sink( buf );
    auto sleep = unit_step * 441;
    count += sleep.count();
    tracks( sleep, midip );
  }
}

