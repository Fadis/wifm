#include <smfp/get_node.hpp>
#include <smfp/exceptions.hpp>
namespace smfp {
  nlohmann::json get_node(
    const nlohmann::json &config,
    const std::string &name
  ) {
    if( !config.is_object() ) throw invalid_instrument_config( "get_node: rootがobjectでない" );
    if( config.find( name ) == config.end() )
      throw invalid_instrument_config( std::string( "get_node: " ) + name + "が存在しない" );
    return config[ name ];
  }
}

