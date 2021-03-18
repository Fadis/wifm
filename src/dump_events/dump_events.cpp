#include <smfp/header.hpp>
#include <smfp/track.hpp>
#include <smfp/midi_parser.hpp>
#include <smfp/dummy_handler.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdint>
#include <boost/program_options.hpp>
#include <stamp/mapped_file.hpp>

int main( int argc, char* argv[] ) {
  boost::program_options::options_description options("Options");
  options.add_options()
    ("help,h",    "show this message")
    ("input,i", boost::program_options::value<std::string>(),  "input file");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("input") ) { 
    std::cout << options << std::endl;
    return 0;
  }
  stamp::mapped_file f( params["input"].as< std::string >() );
  auto [iter,header] = smfp::decode_smf_header( f.begin(), f.end() );
  smfp::smf_tracks_t tracks( header, iter, f.end() );
  std::vector< smfp::dummy_handler > handlers( 64, smfp::dummy_handler() );
  smfp::midi_parser_t midip( handlers );
  auto unit_step = std::chrono::nanoseconds( 1000ul * 1000ul * 1000ul / 44100ul );
  uint64_t count = 0;
  while( !tracks.end() ) {
    auto sleep = tracks.get_distance();
    std::this_thread::sleep_for( sleep );
    count += sleep.count();
    std::cout << "=== " << count << " === " << std::endl;
    tracks( sleep, midip );
  }
}

