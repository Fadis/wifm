#ifndef SMFP_VARIABLE_HPP
#define SMFP_VARIABLE_HPP

#include <utility>
#include <boost/container/flat_map.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>

namespace smfp {
  template< typename T >
  struct variable_config_t {
    variable_config_t() {}
    variable_config_t( const nlohmann::json &config ) {
      if( !config.is_object() ) throw invalid_instrument_config( "variable_config_t: rootがobjectでない" );
      for( const auto &[key_str,value]: config.items() ) {
        auto iter = key_str.begin();
        uint32_t key = 0;
        if( !boost::spirit::qi::parse( iter, key_str.end(), boost::spirit::qi::uint_, key ) )
          throw invalid_instrument_config( "variable_config_t: keyをパースできない" );
        if( key > 127u ) throw invalid_instrument_config( "variable_config_t: keyが大きすぎる" );
        keyframes.insert( std::make_pair( note_t( key ), T( value ) ) );
      }
    }
    nlohmann::json dump() const {
      auto root = nlohmann::json::object();
      for( auto &[key,value]: keyframes ) {
        std::string key_str;
        boost::spirit::karma::generate( std::back_inserter( key_str ), boost::spirit::karma::uint_, key );
        root[ key_str ] = value.dump();
      }
      return root;
    }
    T get( note_t note ) const {
      if( keyframes.empty() ) return T();
      const auto upper = keyframes.lower_bound( note );
      if( upper == keyframes.end() )
        return std::prev( upper )->second;
      else if( upper->first == note )
        return upper->second;
      else if( upper == keyframes.begin() )
        return upper->second;
      const auto lower = std::prev( upper );
      return lerp( lower->second, upper->second, float( note - lower->first )/float( upper->first - lower->first ) );
    }
    boost::container::flat_map< note_t, T > keyframes;
  };
  template< typename T >
  class variable_t {
  public:
    using config_type = variable_config_t< typename T::config_type >;
    variable_t( const std::shared_ptr< config_type > &config_ ) :
      config( config_ ), backend( config_->get( 0 ) ), current_note( 0 ) {}
    nlohmann::json dump() const {
      return {
        { "config", config->dump() },
        { "backend", backend.dump() }
      };
    }
    void set_config( const channel_state_t &cst, const std::shared_ptr< config_type > &config_ ) {
      config = config_;
      backend.clear( cst );
      backend.set_config( cst, config->get( current_note ) );
    }
    void note_on( const channel_state_t &cst, const active_note_t &nst ) {
      backend.set_config( cst, config->get( nst.channel_note & 0xFFu ) );
      backend.note_on( cst, nst );
    }
    void note_off( const channel_state_t &cst ) {
      backend.note_off( cst );
    }
    void clear( const channel_state_t &cst ) {
      backend.clear( cst );
    }
    void set_frequency( const channel_state_t &cst, float freq ) {
      backend.set_frequency( cst, freq );
    }
    std::tuple< float, float > operator()( std::chrono::nanoseconds step ) {
      return backend( step );
    }
    template< typename Iterator >
    void operator()( std::chrono::nanoseconds step, Iterator begin, Iterator end ) {
      return backend( step, begin, end );
    }
    void set_variable( channel_variable_id_t id, note_t at, const channel_state_t &cst ) {
      backend.set_variable( id, at, cst );
    }
    void set_program( const channel_state_t &cst, uint8_t prog ) {
      backend.set_program( cst, prog );
    }
    void set_volume( const channel_state_t &cst, float value ) {
      backend.set_volume( cst, value );
    }
    template< typename Iterator >
    void system_exclusive( const channel_state_t &cst, Iterator begin, Iterator end ) {
      backend.system_exclusive( cst, begin, end );
    }
    bool is_end() const {
      return backend.is_end();
    }
  private:
    std::shared_ptr< config_type > config;
    T backend;
    note_t current_note;
  };
}

#endif

