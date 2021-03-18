#ifndef SMFP_DECODE_VARIABLE_LENGTH_QUANTITY_HPP
#define SMFP_DECODE_VARIABLE_LENGTH_QUANTITY_HPP

#include <utility>
#include <tuple>
#include <type_traits>
#include <smfp/exceptions.hpp>

namespace smfp {
  template< typename Iterator, typename Sentinel >
  auto decode_variable_length_quantity( const Iterator &begin, const Sentinel &end ) ->
    std::enable_if_t<
      std::is_convertible_v< decltype( *std::declval< Iterator >() ), unsigned char > &&
      std::is_convertible_v< decltype( std::declval< const Iterator& >() == std::declval< const Sentinel >() ), bool >,
      std::tuple< Iterator, uint32_t >
    > {
    if( begin == end ) throw invalid_variable_length_quantity();
    if( *begin < 0x7F ) return std::make_tuple( std::next( begin ), *begin );
    auto cur = begin;
    uint32_t value = 0;
    for( size_t i = 0u; i != 5u; ++i ) {
      const auto fragment_with_flag = static_cast< unsigned char >( *cur );
      const auto fragment = fragment_with_flag & 0x7F;
      const auto cont = fragment_with_flag & 0x80;
      if( i == 4 ) {
        if( fragment > 0x0F ) throw invalid_variable_length_quantity();
        if( cont ) throw invalid_variable_length_quantity(); 
      }
      value <<= 7;
      value |= fragment & 0x7F;
      ++cur;
      if( !cont ) break;
    }
    return std::make_tuple( cur, value );
  }
}

#endif
