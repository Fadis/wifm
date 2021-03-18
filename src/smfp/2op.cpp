#include <smfp/2op.hpp>
#include <smfp/get_volume.hpp>

namespace smfp {
  fm_2op_nofb_config_t::fm_2op_nofb_config_t( const nlohmann::json &config ) :
    lower( get_node( config, "lower" ) ),
    upper( get_node( config, "upper" ) ) {}
  nlohmann::json fm_2op_nofb_config_t::dump() const {
    return {
      { "lower", lower.dump() },
      { "upper", upper.dump() }
    };
  }
  fm_2op_nofb_config_t lerp(
    const fm_2op_nofb_config_t &l,
    const fm_2op_nofb_config_t &r,
    float pos
  ) {
    return fm_2op_nofb_config_t()
      .set_lower( lerp( l.lower, r.lower, pos ) )
      .set_upper( lerp( l.upper, r.upper, pos ) );
  }
  fm_2op_nofb_t::fm_2op_nofb_t( const fm_2op_nofb_config_t &config ) :
    lower( config.lower ),
    upper( config.upper ) {}
  fm_2op_nofb_t::fm_2op_nofb_t( const nlohmann::json &config ) :
    fm_2op_nofb_t( fm_2op_nofb_config_t( config ) ) {}
  nlohmann::json fm_2op_nofb_t::dump() const {
    return {
      { "lower", lower.dump() },
      { "upper", upper.dump() }
    };
  }
  void fm_2op_nofb_t::set_config( const channel_state_t &cst, const fm_2op_nofb_config_t &config ) {
    lower.set_config( cst, config.lower );
    upper.set_config( cst, config.upper );
  }
  void fm_2op_nofb_t::note_on( const channel_state_t &cst, const active_note_t &nst ) {
    if( ( nst.channel_note >> 8 ) == 10 ) return;
    lower.note_on( cst, nst );
    upper.note_on( cst, nst );
    upper.set_volume( cst, 0.f );
    lower.set_volume( cst, get_volume( cst, nst ) );
  }
  void fm_2op_nofb_t::note_off( const channel_state_t &cst ) {
    lower.note_off( cst );
    upper.note_off( cst );
  }
  void fm_2op_nofb_t::clear( const channel_state_t &cst ) {
    lower.clear( cst );
    upper.clear( cst );
  }
  void fm_2op_nofb_t::set_frequency( const channel_state_t &cst, float freq ) {
    lower.set_frequency( cst, freq );
    upper.set_frequency( cst, freq );
  }
  std::tuple< float, float > fm_2op_nofb_t::operator()( std::chrono::nanoseconds step ) {
    const float top = 0.f;
    auto [uenv,uval] = upper( step, &top );
    return lower( step, &uval );
  }
  void fm_2op_nofb_t::set_variable( channel_variable_id_t /*id*/, note_t /*at*/, const channel_state_t &/*cst*/ ) {
  }
  void fm_2op_nofb_t::set_program( const channel_state_t&,  uint8_t ) {
  }
  void fm_2op_nofb_t::set_volume( const channel_state_t &cst, float value ) {
    lower.set_volume( cst, value );
  }
  bool fm_2op_nofb_t::is_end() const {
    return lower.is_end();
  }
}

