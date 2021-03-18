#ifndef SMFP_DECODE_INTEGER_HPP
#define SMFP_DECODE_INTEGER_HPP

#include <utility>
#include <type_traits>
#include <iterator>

namespace smfp {
  template< typename T, typename Iterator, typename Sentinel >
  auto decode_integer( const Iterator &begin, const Sentinel &end ) ->
    std::enable_if_t<
      std::is_convertible_v< decltype( *std::declval< Iterator >() ), unsigned char > &&
      std::is_convertible_v< decltype( std::declval< const Iterator& >() == std::declval< const Sentinel >() ), bool >,
      std::tuple< Iterator, T >
    > {
    if( std::distance( begin, end ) < int( sizeof( T ) ) ) throw invalid_variable();
    auto cur = begin;
    uint32_t value = 0u;
    for( size_t i = 0u; i != sizeof( T ); ++i ) {
      value <<= 8;
      value |= static_cast< unsigned char >( *cur );
      ++cur;
    }
    return std::make_pair( cur, value );
  }
}

#endif

