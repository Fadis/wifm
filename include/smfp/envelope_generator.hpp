#ifndef SMFP_ENVELOPE_GENERATOR_HPP
#define SMFP_ENVELOPE_GENERATOR_HPP
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>
#include <cmath>
#include <chrono>
#include <limits>
#include <numeric>
#include <algorithm>
#include <type_traits>
#include <tuple>
#include <cstdint>
#include <type_traits>
#include <nlohmann/json.hpp>
#include <stamp/setter.hpp>
namespace smfp {
  nlohmann::json is_valid_eg_config(
    const nlohmann::json &config
  );
  struct envelope_generator_config_t {
    envelope_generator_config_t();
    envelope_generator_config_t( const nlohmann::json &config );
    nlohmann::json dump() const;
    LIBSTAMP_SETTER( delay )
    LIBSTAMP_SETTER( default_attack1 )
    LIBSTAMP_SETTER( default_attack2 )
    LIBSTAMP_SETTER( attack_mid )
    LIBSTAMP_SETTER( hold )
    LIBSTAMP_SETTER( default_decay1 )
    LIBSTAMP_SETTER( default_decay2 )
    LIBSTAMP_SETTER( decay_mid )
    LIBSTAMP_SETTER( sustain )
    LIBSTAMP_SETTER( default_release )
    float delay;
    float default_attack1;
    float default_attack2;
    float attack_mid;
    float hold;
    float default_decay1;
    float default_decay2;
    float decay_mid;
    float sustain;
    float default_release;
  };
  envelope_generator_config_t lerp(
    const envelope_generator_config_t&,
    const envelope_generator_config_t&,
    float
  );
  class envelope_generator_t {
    constexpr static float lowest = -4.f;
  public:
    using config_type = envelope_generator_config_t;
    envelope_generator_t( const nlohmann::json &config );
    envelope_generator_t( const envelope_generator_config_t &config );
    nlohmann::json dump() const;
    void set_config( const channel_state_t&, const envelope_generator_config_t &config_ );
    void note_on( const channel_state_t &cst, const active_note_t &/*nst*/ );
    void clear( const channel_state_t & );
    void note_off( const channel_state_t & );
    void operator()( std::chrono::nanoseconds step, float *begin, float *end );
    float operator()( std::chrono::nanoseconds step );
    bool is_end() const;
    void set_volume( const channel_state_t &, float vol );
  private:
    void init_delay();
    void init_attack1();
    void init_attack2();
    void init_hold();
    void init_decay1();
    void init_decay2();
    void init_sustain();
    void init_release();
    void init_end();
    float calc_delay( std::chrono::nanoseconds step );
    float calc_attack1( std::chrono::nanoseconds step );
    float calc_attack2( std::chrono::nanoseconds step );
    float calc_hold( std::chrono::nanoseconds step );
    float calc_decay1( std::chrono::nanoseconds step );
    float calc_decay2( std::chrono::nanoseconds step );
    float calc_sustain( std::chrono::nanoseconds step );
    float calc_release( std::chrono::nanoseconds step );
    float calc_end( std::chrono::nanoseconds step );
    envelope_generator_config_t config;
    float current_level;
    float current_tangent;
    float ( envelope_generator_t::*state )( std::chrono::nanoseconds );
    std::chrono::nanoseconds at;
    float attack1;
    float attack2;
    float decay1;
    float decay2;
    float decay_mid;
    float release;
    float volume;
  };
}

#endif
