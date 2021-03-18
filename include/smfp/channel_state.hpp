#ifndef SMFP_CHANNEL_STATE_HPP
#define SMFP_CHANNEL_STATE_HPP

#include <cstdint>
#include <array>
#include <type_traits>
#include <nlohmann/json.hpp>
#include <stamp/setter.hpp>
#include <smfp/types.hpp>
#include <smfp/global_state.hpp>

namespace smfp {
  enum class channel_variable_id_t {
    bank = 0,
    modulation,
    breath,
    foot,
    portamento_time,
    data_entry,
    volume,
    balance,
    pan,
    expression,
    effect1,
    effect2,
    reverb,
    tremolo,
    chorus,
    celeste,
    phaser,
    general_purpose1,
    general_purpose2,
    general_purpose3,
    general_purpose4,
    general_purpose5,
    general_purpose6,
    general_purpose7,
    general_purpose8,
    hold1,
    portamento_switch,
    sostenuto,
    soft,
    legato,
    hold2,
    variation,
    timbre,
    release,
    attack,
    brightness,
    decay,
    vibrato_rate,
    vibrato_depth,
    vibrato_delay,
    nrpn,
    rpn,
    pitch_bend_sensitivity,
    master_fine_tune,
    master_coarse_tune,
    modulation_depth_range,
    vibrato_rate_gs,
    vibrato_depth_gs,
    vibrato_delay_gs,
    vibrato_rate_xg,
    vibrato_depth_xg,
    vibrato_delay_xg,
    tvf_cutoff_freq,
    tvf_resonance,
    hpf_cutoff_freq,
    hpf_resonance,
    eq_bass,
    eq_treble,
    eq_mid_bass,
    eq_mid_treble,
    eq_bass_frequency,
    eq_treble_frequency,
    eq_mid_bass_frequency,
    eq_mid_treble_frequency,
    tvf_tva_envelope_attack_time,
    tvf_tva_envelope_decay_time,
    tvf_tva_envelope_release_time,
    drum_pitch,
    drum_pan = drum_pitch + 128, 
    drum_tva = drum_pan + 128,
    drum_reverb = drum_tva + 128,
    drum_chorus = drum_reverb + 128,
    count = drum_chorus + 128
  };
  std::string to_string( channel_variable_id_t v );
  struct channel_state_t {
    channel_state_t( const global_state_t *gst );
    channel_state_t( const channel_state_t& ) = default;
    channel_state_t( channel_state_t&& ) = default;
    channel_state_t &operator=( const channel_state_t& ) = default;
    channel_state_t &operator=( channel_state_t&& ) = default;
    nlohmann::json to_json() const;
    auto &operator[]( channel_variable_id_t id ) {
      return control[ int( id ) ];
    }
    const auto &operator[]( channel_variable_id_t id ) const {
      return control[ int( id ) ];
    }

#define SMFP_CHANNEL_STATE_DIRECT( name ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , uint16_t > { \
      return control[ int( channel_variable_id_t :: name ) ]; \
    }
#define SMFP_CHANNEL_STATE_SHIFT( name, count ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , uint8_t > { \
      return uint8_t( control[ int( channel_variable_id_t :: name ) ] >> count ); \
    }
#define SMFP_CHANNEL_STATE_UFLOAT( name ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , float > { \
      return control[ int( channel_variable_id_t :: name ) ] / float( 0x3FFFu ); \
    }
#define SMFP_CHANNEL_STATE_UFLOAT_SHIFT( name ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , float > { \
      return control[ int( channel_variable_id_t :: name ) ] / float( 0x2000u ) - 1.f; \
    }
#define SMFP_CHANNEL_STATE_UFLOAT_SCALE( name, denom, scale ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , float > { \
      return control[ int( channel_variable_id_t :: name ) ] / float( denom ) * scale; \
    }
#define SMFP_CHANNEL_STATE_SFLOAT( name, denom ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , float > { \
      return ( control[ int( channel_variable_id_t :: name ) ] < denom ) ? \
        ( control[ int( channel_variable_id_t :: name ) ] / float( denom ) ) : \
        ( ( int16_t( control[ int( channel_variable_id_t :: name ) ] ) - ( denom * 2 ) ) / float( denom ) ); \
    }
#define SMFP_CHANNEL_STATE_BOOL( name, threshold ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , bool > { \
      return control[ int( channel_variable_id_t :: name ) ] < threshold; \
    }
#define SMFP_CHANNEL_STATE_FCUFLOAT( name ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name ,float > { \
      auto coarse = ( control[ int( channel_variable_id_t:: name ) ] >> 7 ); \
      auto fine = ( control[ int( channel_variable_id_t:: name ) ] & 0x7F ) / 128.f; \
      return coarse + fine; \
    }
#define SMFP_CHANNEL_STATE_LUFLOAT( name, min_, scale ) \
    template< channel_variable_id_t vid > \
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t:: name , float > { \
      return ( ( std::min( std::max( uint16_t( min_ ), control[ int( channel_variable_id_t :: name ) ] ), uint16_t( 0x3FFFu - ( min_ ) ) ) - ( min_ ) ) / float( 0x2000u - ( min_ ) ) - 1.f ) * scale; \
    }
    SMFP_CHANNEL_STATE_DIRECT( bank )
    SMFP_CHANNEL_STATE_FCUFLOAT( modulation )
    SMFP_CHANNEL_STATE_UFLOAT( balance )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( pan )
    SMFP_CHANNEL_STATE_UFLOAT( effect1 )
    SMFP_CHANNEL_STATE_UFLOAT( effect2 )
    SMFP_CHANNEL_STATE_UFLOAT( reverb )
    SMFP_CHANNEL_STATE_UFLOAT( tremolo )
    SMFP_CHANNEL_STATE_UFLOAT( chorus )
    SMFP_CHANNEL_STATE_UFLOAT( celeste )
    SMFP_CHANNEL_STATE_UFLOAT( phaser )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose1 )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose2 )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose3 )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose4 )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose5 )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose6 )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose7 )
    SMFP_CHANNEL_STATE_UFLOAT( general_purpose8 )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( legato )
    SMFP_CHANNEL_STATE_UFLOAT( hold2 )
    SMFP_CHANNEL_STATE_SHIFT( variation, 7 )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( timbre )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( release )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( attack )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( brightness )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( decay )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( vibrato_rate )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( vibrato_depth )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( vibrato_delay )
    SMFP_CHANNEL_STATE_DIRECT( rpn )
    SMFP_CHANNEL_STATE_DIRECT( nrpn )
    SMFP_CHANNEL_STATE_SHIFT( pitch_bend_sensitivity, 7 )
    SMFP_CHANNEL_STATE_FCUFLOAT( modulation_depth_range )
    SMFP_CHANNEL_STATE_LUFLOAT( vibrato_rate_gs, 0x0E << 7, 1.f )
    SMFP_CHANNEL_STATE_LUFLOAT( vibrato_depth_gs, 0x0E << 7, 1.f )
    SMFP_CHANNEL_STATE_LUFLOAT( vibrato_delay_gs, 0x0E << 7, 1.f )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( vibrato_rate_xg )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( vibrato_depth_xg )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( vibrato_delay_xg )
    SMFP_CHANNEL_STATE_LUFLOAT( tvf_cutoff_freq, 0x0E << 7, 1.f )
    SMFP_CHANNEL_STATE_LUFLOAT( tvf_resonance, 0x0E << 7, 1.f )
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( hpf_cutoff_freq );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( hpf_resonance );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_bass );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_treble );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_mid_bass );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_mid_treble );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_bass_frequency );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_treble_frequency );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_mid_bass_frequency );
    SMFP_CHANNEL_STATE_UFLOAT_SHIFT( eq_mid_treble_frequency );
    SMFP_CHANNEL_STATE_LUFLOAT( tvf_tva_envelope_attack_time, 0x0E << 7, 1.f )
    SMFP_CHANNEL_STATE_LUFLOAT( tvf_tva_envelope_decay_time, 0x0E << 7, 1.f )
    SMFP_CHANNEL_STATE_LUFLOAT( tvf_tva_envelope_release_time, 0x0E << 7, 1.f )
    /*drum_pitch,
    drum_tva = drum_pitch + 128,
    drum_reverb = drump_tva + 128,
    drum_chorus = drum_reverb + 128,
    */
    template< channel_variable_id_t vid >
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t::portamento_time, float > {
      return
        ( control[ int( channel_variable_id_t:: portamento_switch ) ] < 0x2000u ) ?
        0.f :
        ( control[ int( channel_variable_id_t::portamento_time ) ] / float( 0x3fffu ) );
    }
    template< channel_variable_id_t vid >
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t::volume, float > {
      const std::array< float, 6u > values{
        control[ int( channel_variable_id_t::volume ) ] / float( 0x3FFF ),
        control[ int( channel_variable_id_t::expression ) ] / float( 0x3FFF ),
        control[ int( channel_variable_id_t::foot ) ] / float( 0x3FFF ),
        control[ int( channel_variable_id_t::breath ) ] / float( 0x3FFF ),
        pressure / float( 0x7F ),
        ( ( control[ int( channel_variable_id_t::soft ) ] >> 7 ) + 0x100u ) / float( 0x17Fu )
      };
      if( std::find( values.begin(), values.end(), 0.f ) != values.end() ) return -std::numeric_limits< float >::infinity();
      return std::accumulate( values.begin(), values.end(), 0.f, []( auto sum, auto v ) {
        return sum + 40 * std::log10( v );
      } );
    }
    template< channel_variable_id_t vid >
    auto get() const -> std::enable_if_t< vid == channel_variable_id_t::master_fine_tune, float > {
      auto fine = ( uint32_t( control[ int( channel_variable_id_t::master_fine_tune ) ] ) ) / float( 0x2000u ) - 1.f;
      auto coarse = float( control[ int( channel_variable_id_t::master_coarse_tune ) ] >> 7 ) - 64.f;
      return coarse + fine;
    }
    float get_pitch_bend() const;
    LIBSTAMP_SETTER( program )
    LIBSTAMP_SETTER( pitch_bend )
    LIBSTAMP_SETTER( pressure )
    const global_state_t *global_state;
    program_id_t program;
    pitch_t pitch_bend;
    uint8_t pressure;
    std::array< uint16_t, int( channel_variable_id_t::count ) > control;
  };
}

#endif

