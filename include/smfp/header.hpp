#ifndef SMFP_HEADER_HPP
#define SMFP_HEADER_HPP

#include <cstdint>
#include <array>
#include <type_traits>
#include <chrono>
#include <tuple>
#include <stamp/setter.hpp>
#include <smfp/exceptions.hpp>
#include <smfp/decode_integer.hpp>

namespace smfp {
  enum class smf_format_t {
    single_multi_channel_track = 0,
    one_or_more_simultanious_tracks = 1,
    one_or_more_sequentially_independent_tracks = 2
  };
  struct smf_header_t {
    smf_header_t() : format( smf_format_t::single_multi_channel_track ), ntrks( 0 ), qnres( 0 ) {}
    LIBSTAMP_SETTER( format )
    LIBSTAMP_SETTER( ntrks )
    LIBSTAMP_SETTER( time_unit )
    LIBSTAMP_SETTER( qnres )
    smf_format_t format;
    uint16_t ntrks;
    std::chrono::nanoseconds time_unit;
    uint32_t qnres;
  };
  template< typename Iterator, typename Sentinel >
  auto decode_smf_header( const Iterator &begin, const Sentinel &end ) ->
    std::enable_if_t<
      std::is_convertible_v< decltype( *std::declval< Iterator >() ), unsigned char > &&
      std::is_convertible_v< decltype( std::declval< const Iterator& >() == std::declval< const Sentinel >() ), bool >,
      std::tuple< Iterator, smf_header_t >
    > {
    constexpr std::array< unsigned char, 8u > magic{ 'M', 'T', 'h', 'd', '\0', '\0', '\0', '\x06' };
    if( std::distance( begin, end ) < 14 ) throw invalid_smf_header();
    if( !std::equal( magic.begin(), magic.end(), begin ) ) throw invalid_smf_header();
    const auto format_head = std::next( begin, 8 );
    const auto [ntrks_head,format] = decode_integer< uint16_t >( format_head, end );
    const auto [division_head,ntrks] = decode_integer< uint16_t >( ntrks_head, end );
    const auto [head_end,division] = decode_integer< uint16_t >( division_head, end );
    if( division < 0x8000u ) {
      if( division == 0x0000u ) throw invalid_smf_header();
      return std::make_tuple(
        head_end,
        smf_header_t()
          .set_format( smf_format_t( format ) )
          .set_ntrks( ntrks )
          .set_qnres( division )
      );
    }
    else {
      const uint32_t frame_rate = ~static_cast< int8_t >( division >> 7 );
      const uint32_t frame_division = division & 0x7Fu;
      if( frame_rate == 0u ) throw invalid_smf_header();
      if( division == 0u ) throw invalid_smf_header();
      return std::make_tuple(
        head_end,
        smf_header_t()
          .set_format( smf_format_t( format ) )
          .set_ntrks( ntrks )
          .set_time_unit( std::chrono::nanoseconds( ( 1000ull * 1000ull * 1000ull ) / ( frame_rate * frame_division ) ) )
      );
    }
  }
}

#endif

