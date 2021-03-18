#include <cmath>
#include <smfp/envelope_generator.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>
namespace smfp {
#define SMFP_CHECK_CONFIG_PARAM( name ) \
    if( config.find( # name ) != config.end() ) \
      if( !config[ # name ].is_number() ) \
        throw invalid_instrument_config( "envelope_generator_config_t: " # name "が設定されていない" );
#define SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( name ) \
  if( config.find( # name ) != config.end() ) name = config[ # name ];
  envelope_generator_config_t::envelope_generator_config_t() :
    delay( 0 ), default_attack1( 0 ), default_attack2( 0 ), attack_mid( 0 ),
    hold( 0 ), default_decay1( 0 ), default_decay2( 0 ), decay_mid( 0 ),
    sustain( -4.01 ), default_release( 0 ) {}
  envelope_generator_config_t::envelope_generator_config_t( const nlohmann::json &config ) :
    delay( 0 ), default_attack1( 0 ), default_attack2( 0 ), attack_mid( 0 ),
    hold( 0 ), default_decay1( 0 ), default_decay2( 0 ), decay_mid( 0 ),
    sustain( -4.01 ), default_release( 0 ) {
    if( !config.is_object() ) throw invalid_instrument_config( "envelope_generator_config_t: rootがobjectでない" );
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( delay )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( default_attack1 )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( default_attack2 )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( attack_mid )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( hold )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( default_decay1 )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( default_decay2 )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( decay_mid )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( sustain )
    SMFP_ENVELOPE_GENERATOR_PARAM_WITH_DEFAULT( default_release )
  }
  nlohmann::json envelope_generator_config_t::dump() const {
    return {
      { "delay", delay },
      { "default_attack1", default_attack1 },
      { "default_attack2", default_attack2 },
      { "attack_mid", attack_mid },
      { "hold", hold },
      { "default_decay1", default_decay1 },
      { "default_decay2", default_decay2 },
      { "decay_mid", decay_mid },
      { "sustain", sustain },
      { "default_release", default_release }
    };
  }
  envelope_generator_config_t lerp(
    const envelope_generator_config_t &l,
    const envelope_generator_config_t &r,
    float pos
  ) {
    return envelope_generator_config_t()
      .set_delay( std::lerp( l.delay, r.delay, pos ) )
      .set_default_attack1( std::lerp( l.default_attack1, r.default_attack1, pos ) )
      .set_default_attack2( std::lerp( l.default_attack2, r.default_attack2, pos ) )
      .set_attack_mid( std::lerp( l.attack_mid, r.attack_mid, pos ) )
      .set_hold( std::lerp( l.hold, r.hold, pos ) )
      .set_default_decay1( std::lerp( l.default_decay1, r.default_decay1, pos ) )
      .set_default_decay2( std::lerp( l.default_decay2, r.default_decay2, pos ) )
      .set_decay_mid( std::lerp( l.decay_mid, r.decay_mid, pos ) )
      .set_sustain( std::lerp( l.sustain, r.sustain, pos ) )
      .set_default_release( std::lerp( l.default_release, r.default_release, pos ) );
  }
  nlohmann::json is_valid_eg_config(
    const nlohmann::json &config
  ) {
    if( !config.is_object() ) throw invalid_instrument_config( "envelope_generator_config_t: rootがobjectでない" );
    SMFP_CHECK_CONFIG_PARAM( delay )
    SMFP_CHECK_CONFIG_PARAM( default_attack1 )
    SMFP_CHECK_CONFIG_PARAM( default_attack2 )
    SMFP_CHECK_CONFIG_PARAM( attack_mid )
    SMFP_CHECK_CONFIG_PARAM( hold )
    SMFP_CHECK_CONFIG_PARAM( default_decay1 )
    SMFP_CHECK_CONFIG_PARAM( default_decay2 )
    SMFP_CHECK_CONFIG_PARAM( decay_mid )
    SMFP_CHECK_CONFIG_PARAM( sustain )
    SMFP_CHECK_CONFIG_PARAM( default_release )
    return config;
  }
  envelope_generator_t::envelope_generator_t( const envelope_generator_config_t &config_ ) :
    config( config_ ),
    current_level( 0 ), current_tangent( 0 ),
    state( &envelope_generator_t::calc_end ),
    at( 0 ),
    attack1( 0 ), attack2( 0 ), decay1( 0 ), decay2( 0 ), release( 0 ),
    volume( 0 ) {}
  envelope_generator_t::envelope_generator_t( const nlohmann::json &config_ ) :
    envelope_generator_t( envelope_generator_config_t( config_ ) ) {}
  void envelope_generator_t::set_config( const channel_state_t&, const envelope_generator_config_t &config_ ) {
    config = config_;
  }
  nlohmann::json envelope_generator_t::dump() const {
    return {
      { "config", config.dump() },
      { "attack1", attack1 },
      { "attack2", attack2 },
      { "decay1", decay1 },
      { "decay2", decay2 },
      { "release", release }
    };
  }
  void envelope_generator_t::note_on( const channel_state_t &cst, const active_note_t &/*nst*/ ) {
    at = std::chrono::nanoseconds( 0 );
    attack1 = config.default_attack1 * ( cst.get< channel_variable_id_t::tvf_tva_envelope_attack_time >() + 1.f );
    attack2 = config.default_attack2 * ( cst.get< channel_variable_id_t::tvf_tva_envelope_attack_time >() + 1.f );
    decay1 = config.default_decay1 * ( cst.get< channel_variable_id_t::tvf_tva_envelope_decay_time >() + 1.f );
    decay2 = config.default_decay2 * ( cst.get< channel_variable_id_t::tvf_tva_envelope_decay_time >() + 1.f );
    release = config.default_release * ( cst.get< channel_variable_id_t::tvf_tva_envelope_release_time >() + 1.f );
    init_delay();
  }
  void envelope_generator_t::clear( const channel_state_t & ) {
    init_end();
  }
  void envelope_generator_t::note_off( const channel_state_t & ) {
    init_release();
  }
  void envelope_generator_t::operator()( std::chrono::nanoseconds step, float *begin, float *end ) {
    for( auto iter = begin; iter != end; ++iter )
      *iter += ( this->*state )( step );
  }
  float envelope_generator_t::operator()( std::chrono::nanoseconds step ) {
    auto value = ( this->*state )( step );
    return value;
  }
  bool envelope_generator_t::is_end() const {
    return state == &envelope_generator_t::calc_end;
  }
  void envelope_generator_t::set_volume( const channel_state_t &, float vol ) {
    volume = vol;
  }
#define SMFP_ENVELOPE_GENERATOR_INIT( name, val, level, tangent, next ) \
  void envelope_generator_t::init_ ## name () { \
    if( val != 0 ) { \
      at = std::chrono::nanoseconds( 0 ); \
      current_level = level ; \
      current_tangent = tangent; \
      state = &envelope_generator_t::calc_ ## name ; \
    } \
    else init_ ## next (); \
  }
  SMFP_ENVELOPE_GENERATOR_INIT( delay, config.delay, lowest, lowest, attack1 )
  SMFP_ENVELOPE_GENERATOR_INIT( attack1, attack1, lowest, ( config.attack_mid - lowest ) / attack1, attack2 )
  SMFP_ENVELOPE_GENERATOR_INIT( attack2, attack2, config.attack_mid, ( 1.f - config.attack_mid ) / attack2, hold )
  SMFP_ENVELOPE_GENERATOR_INIT( hold, config.hold, 1, 0, decay1 )
  SMFP_ENVELOPE_GENERATOR_INIT( decay1, decay1, 1, -( 1.f - config.decay_mid ) / decay1, decay2 )
  SMFP_ENVELOPE_GENERATOR_INIT( decay2, decay2, config.decay_mid, -( config.decay_mid - config.sustain ) / decay2, sustain )
  void envelope_generator_t::init_sustain() {
    if( config.sustain > lowest ) {
      at = std::chrono::nanoseconds( 0 );
      current_level = config.sustain ;
      current_tangent = 0;
      state = &envelope_generator_t::calc_sustain ;
    }
    else init_end();
  }
  void envelope_generator_t::init_release() {
    if( release != 0 && current_level > 0 ) {
      at = std::chrono::nanoseconds( 0 );
      if( config.sustain >= lowest ) current_tangent = -( config.sustain - lowest ) / release;
      else if( config.decay_mid ) current_tangent = -( config.decay_mid - lowest ) / release;
      else current_tangent = -( 1 - lowest ) / release;
      state = &envelope_generator_t::calc_release;
    }
    else init_end();
  }
  void envelope_generator_t::init_end() {
    at = std::chrono::nanoseconds( 0 );
    current_level = 0 ;
    current_tangent = 0;
    state = &envelope_generator_t::calc_end ;
  }
#define SMFP_ENVELOPE_GENERATOR_CALC( name, val, next ) \
  float envelope_generator_t::calc_ ## name ( std::chrono::nanoseconds step ) { \
    current_level += std::chrono::duration_cast< std::chrono::duration< float > >( step ).count() * current_tangent; \
    at += step; \
    if( std::chrono::duration_cast< std::chrono::duration< float > >( at ).count() >= val ) \
      init_ ## next (); \
    return ( -1.f + current_level ) * 48.f + volume; \
  }
  SMFP_ENVELOPE_GENERATOR_CALC( delay, config.delay, attack1 )
  SMFP_ENVELOPE_GENERATOR_CALC( attack1, attack1, attack2 )
  SMFP_ENVELOPE_GENERATOR_CALC( attack2, attack2, hold )
  SMFP_ENVELOPE_GENERATOR_CALC( hold, config.hold, decay1 )
  SMFP_ENVELOPE_GENERATOR_CALC( decay1, decay1, decay2 )
  SMFP_ENVELOPE_GENERATOR_CALC( decay2, decay2, sustain )
  float envelope_generator_t::calc_sustain( std::chrono::nanoseconds ) {
    return ( -1.f + config.sustain ) * 48.f + volume;
  }
  float envelope_generator_t::calc_release( std::chrono::nanoseconds step ) {
    current_level += std::chrono::duration_cast< std::chrono::duration< float > >( step ).count() * current_tangent;
    at += step;
     if( current_level <= lowest )
       init_end();
    return ( -1.f + current_level ) * 48.f + volume;
  }
  float envelope_generator_t::calc_end( std::chrono::nanoseconds ) {
    return -std::numeric_limits< float >::infinity();
  }
}

