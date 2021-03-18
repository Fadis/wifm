#ifndef SMFP_MULTI_INSTRUMENTS_HPP
#define SMFP_MULTI_INSTRUMENTS_HPP

#include <utility>
#include <boost/container/flat_map.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>

namespace smfp {
  template< typename T >
  struct multi_instruments_config_t {
    multi_instruments_config_t() {}
    multi_instruments_config_t( const nlohmann::json &config ) {
      if( !config.is_object() ) throw invalid_instrument_config( "multi_instruments_config_t: rootがobjectでない" );
      boost::container::flat_map< std::string, std::shared_ptr< T > > instruments;
      if( config.find( "instruments" ) == config.end() ) throw invalid_instrument_config( "multi_instruments_config_t: instrumentsがない" );
      if( !config[ "instruments" ].is_object() )  throw invalid_instrument_config( "multi_instruments_config_t: instrumentsがobjectでない" );
      if( config.find( "sets" ) == config.end() ) throw invalid_instrument_config( "multi_instruments_config_t: setsがない" );
      if( !config[ "sets" ].is_object() )  throw invalid_instrument_config( "multi_instruments_config_t: setsがobjectでない" );
      if( config.find( "bank" ) == config.end() ) throw invalid_instrument_config( "multi_instruments_config_t: bankがない" );
      if( !config[ "bank" ].is_object() )  throw invalid_instrument_config( "multi_instruments_config_t: bankがobjectでない" );
      for( const auto &[key,value]: config[ "instruments" ].items() ) {
        std::shared_ptr< T > inst( new T( value ) );
        instruments.insert( std::make_pair( std::string( key ), std::move( inst ) ) );
      }
      boost::container::flat_map< std::pair< std::string, program_id_t >, std::shared_ptr< T > > sets;
      for( const auto &[set_name,value]: config[ "sets" ].items() ) {
        if( !value.is_object() ) throw invalid_instrument_config( "multi_instruments_config_t: setsの値がobjectでない" );
        for( const auto &[prog_id_str,inst_name]: value.items() ) {
          auto iter = prog_id_str.begin();
          uint32_t prog_id;
          if( !boost::spirit::qi::parse( iter, prog_id_str.end(), boost::spirit::qi::uint_, prog_id ) )
            throw invalid_instrument_config( "multi_instruments_config_t: setsのprogram_idをパースできない" );
          if( prog_id > 127u ) throw invalid_instrument_config( "multi_instruments_config_t: setsのprogram_idが大きすぎる" );
          const auto inst_iter = instruments.find( std::string( inst_name ) );
          if( inst_iter == instruments.end() ) throw invalid_instrument_config( "multi_instruments_config_t: setsで指定されたinstrumentが存在しない" );
          sets.insert( std::make_pair(
            std::make_pair( std::string( set_name ), program_id_t( prog_id ) ),
            inst_iter->second
          ) );
        }
      }
      for( const auto &[bank_id_str,set_name]: config[ "bank" ].items() ) {
        auto iter = bank_id_str.begin();
        uint32_t bank_id;
        if( !boost::spirit::qi::parse( iter, bank_id_str.end(), boost::spirit::qi::uint_, bank_id ) )
          throw invalid_instrument_config( "multi_instruments_config_t: bankのbank_idをパースできない" );
        if( bank_id > 0x3FFFu ) throw invalid_instrument_config( "multi_instruments_config_t: bankのbank_idが大きすぎる" );
        const auto lower = sets.lower_bound( std::make_pair( std::string( set_name ), program_id_t( 0u ) ) );
        const auto upper = sets.upper_bound( std::make_pair( std::string( set_name ), program_id_t( 127u ) ) );
        if( lower == upper ) throw invalid_instrument_config( "multi_instruments_config_t: bankで指定されたsetが存在しない" );
        std::transform(
          lower, upper, std::inserter( bank, bank.end() ), [&]( const auto &v ) {
            return std::make_pair( std::make_pair( bank_id, v.first.second ), v.second );
          }
        );
      }
      if( bank.empty() ) throw invalid_instrument_config( "multi_instruments_config_t: 楽器の設定が1つもない" );
    }
    std::shared_ptr< T > get() {
      return bank.begin()->second;
    }
    std::shared_ptr< T > get( const channel_state_t &cst ) {
      const auto primary = bank.find(
        std::make_pair(
          uint32_t( cst.get< channel_variable_id_t::bank >() ),
          program_id_t( cst.program )
        )
      );
      if( primary != bank.end() ) return primary->second;
      const auto secondary = bank.find(
        std::make_pair(
          uint32_t( 0 ),
          program_id_t( cst.program )
        )
      );
      if( secondary != bank.end() ) return secondary->second;
      return bank.begin()->second;
    }
    nlohmann::json dump() const {
      auto root = nlohmann::json::object();
      for( const auto &[key,value]: bank ) {
        std::string key_str;
        boost::spirit::karma::generate( std::back_inserter( key_str ), boost::spirit::karma::uint_, key.first );
        key_str += '.';
        boost::spirit::karma::generate( std::back_inserter( key_str ), boost::spirit::karma::uint_, key.second );
        root[ key_str ] = value.dump();
      }
      return root;
    }
    boost::container::flat_map< std::pair< uint32_t, program_id_t >, std::shared_ptr< T > > bank;
  };
  template< typename T >
  class multi_instrument_t {
  public:
    using config_type = multi_instruments_config_t< typename T::config_type >;
    multi_instrument_t( const std::shared_ptr< multi_instruments_config_t< typename T::config_type > > &config_ ) :
      config( config_ ), backend( config_->get() ) {}
    nlohmann::json dump() const {
      return {
        { "config", config->dump() },
        { "backend", backend.dump() }
      };
    }
    void note_on( const channel_state_t &cst, const active_note_t &nst ) {
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
    void set_program( const channel_state_t &cst,  uint8_t prog ) {
      backend.set_config( cst, config->get( cst ) );
      backend.clear( cst );
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
  };
}

#endif
