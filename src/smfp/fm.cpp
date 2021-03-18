#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>
#include <smfp/fm.hpp>
#include <nlohmann/json.hpp>
namespace smfp {
#define SMFP_CHECK_CONFIG_PARAM( name ) \
    if( config.find( # name ) != config.end() ) \
      if( !config[ # name ].is_number() ) \
        throw invalid_instrument_config( "fm_config_t: " # name "が設定されていない" );
  nlohmann::json is_valid_fm_config(
    const nlohmann::json &config,
    size_t operator_count
  ) {
    if( !config.is_object() ) throw invalid_instrument_config( "fm_config_t: rootがobjectでない" );
    SMFP_CHECK_CONFIG_PARAM( scale )
    if( config.find( "modulation" ) == config.end() )
      throw invalid_instrument_config( "fm_config_t: modulationが設定されていない" );
    if( !config[ "modulation" ].is_array() )
      throw invalid_instrument_config( "fm_config_t: modulationが配列でない" );
    if( config[ "modulation" ].size() != operator_count )
      throw invalid_instrument_config( "fm_config_t: modulationのサイズがオペレータ数と一致しない" );
    for( const auto &v: config[ "modulation" ] )
      if( !v.is_number() )
        throw invalid_instrument_config( "fm_config_t: modulationの要素が数値でない" );
    return config;
  }
}

