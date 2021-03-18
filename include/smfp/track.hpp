#ifndef SMFP_TRACK_HPP
#define SMFP_TRACK_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <tuple>
#include <stamp/setter.hpp>
#include <smfp/header.hpp>
#include <smfp/exceptions.hpp>
#include <smfp/decode_variable_length_quantity.hpp>

namespace smfp {
  using step_t = int32_t;
  using track_id_t = uint32_t;
  template< typename Iterator, typename Sentinel >
  struct smf_track_t {
    smf_track_t() : id( 0 ), begin( Iterator() ), cur( Iterator() ), end( Sentinel() ), left( 0 ), next( nullptr ), current_status_byte( 0 ) {}
    LIBSTAMP_SETTER( id )
    LIBSTAMP_SETTER( begin )
    LIBSTAMP_SETTER( end )
    LIBSTAMP_SETTER( cur )
    track_id_t id;
    Iterator begin;
    Iterator cur;
    Sentinel end;
    step_t left;
    smf_track_t *next;
    uint8_t current_status_byte;
  };
  template< typename Iterator, typename Sentinel >
  class smf_tracks_t {
  public:
    using track_t = smf_track_t< Iterator, Sentinel >;
    smf_tracks_t(
      const smf_header_t &header_,
      Iterator begin,
      Sentinel end
    ) : header( header_ ), gbegin( begin ), head( nullptr ), overrun( 0 ), nspb( 60ull * 1000ull * 1000ull * 1000ull / 120u ) {
      auto cur = begin;
      auto track_count = 0u;
      for( unsigned int i = 0; i != header.ntrks &&  cur != end; ++i ) {
        if( std::distance( cur, end ) < 8 ) throw invalid_smf_track();
        constexpr std::array< unsigned char, 4u > magic{ 'M', 'T', 'r', 'k' };
        if( !std::equal( magic.begin(), magic.end(), cur ) ) throw invalid_smf_track();
        cur = std::next( cur, 4 );
        auto [data,length] = decode_integer< uint32_t >( cur, end );
        cur = data;
        if( std::distance( cur, end ) < length ) throw invalid_smf_track();
        auto next = std::next( cur, length );
        tracks[ track_count ] =
          track_t()
            .set_id( track_count )
            .set_begin( cur )
            .set_end( next )
            .set_cur( cur );
        parse_time( tracks.data() + track_count );
        cur = next;
        ++track_count;
      }
      for( auto &v: tracks ) {
        insert( &v );
      }
    }
    template< typename Handler >
    void operator()( std::chrono::nanoseconds adv, Handler &handler ) {
      step_t step;
      if( header.qnres ) {
        step = ( uint64_t( adv.count() + overrun ) * header.qnres ) / nspb;
        overrun = std::max( int64_t( adv.count() - ( step * nspb / header.qnres - overrun ) ), int64_t( 0 ) );
      }
      else {
        step = ( uint64_t( adv.count() ) + overrun ) / header.time_unit.count();
        overrun = std::max( int64_t( adv.count() - ( step * header.time_unit.count() - overrun ) ), int64_t( 0 ) );
      }
      while( 1 ) {
        if( head ) {
          if( head->left <= step ) {
            update_current_status_byte( head );
            if( head->current_status_byte == 0xFF ) {
              auto cur = head->cur;
              const auto type = *cur;
              if( type == 0x51 ) {
                set_tempo( std::next( cur ), get_message_end( head ) );
              }
              else {
                auto end = get_message_end( head );
                handler( head->current_status_byte, head->cur, end );
              }
            }
            else {
              auto end = get_message_end( head );
              handler( head->current_status_byte, head->cur, end );
            }
            auto old_head = pop();
            sub_left( old_head->left );
            step -= old_head->left;
            old_head->left = 0;
            next( old_head );
            insert( old_head );
          }
          else {
            sub_left( step );
            break;
          }
        }
        else break;
      }
    }
    std::chrono::nanoseconds get_distance() const {
      if( !head ) return std::chrono::nanoseconds( 0 );
      if( header.qnres )
        return std::chrono::nanoseconds( std::max( ( uint64_t( head->left ) * nspb ) / header.qnres - overrun, uint64_t( nspb / header.qnres ) ) );
      else
        return std::chrono::nanoseconds( std::max( head->left * header.time_unit.count() - overrun, uint64_t( nspb / header.qnres ) ) );
    }
    bool end() const {
      return !head;
    }
  private:
    track_t *pop() {
      auto old_head = head;
      head = head->next;
      return old_head;
    }
    void sub_left( step_t step ) {
      auto cur = head;
      while( cur ) {
        cur->left -= step;
        cur = cur->next;
      }
    }
    void set_tempo( Iterator begin, Iterator /*end*/ ) {
      auto cur = begin;
      const auto length = *cur;
      if( length != 0x03 ) {
        throw invalid_midi_message();
      }
      ++cur;
      uint32_t new_uspb = *cur;
      ++cur;
      new_uspb <<= 8;
      new_uspb |= *cur;
      ++cur;
      new_uspb <<= 8;
      new_uspb |= *cur;
      nspb = new_uspb * 1000ull;
    }
    void update_current_status_byte( track_t *track ) {
      const uint8_t message_head = *track->cur;
      if( ( message_head & 0x80 ) == 0x00 ) return;
      track->current_status_byte = message_head;
      track->cur = std::next( track->cur );
    }
    Iterator get_message_end( const track_t *track ) {
      if( track->cur == track->end ) return track->cur;
      const uint8_t message_head = track->current_status_byte;
      std::size_t len = 0u;
      if( ( message_head & 0xF0 ) == 0x80 ) len = 2;
      else if( ( message_head & 0xF0 ) == 0x90 ) len = 2;
      else if( ( message_head & 0xF0 ) == 0xA0 ) len = 2;
      else if( ( message_head & 0xF0 ) == 0xB0 ) len = 2;
      else if( ( message_head & 0xF0 ) == 0xC0 ) len = 1;
      else if( ( message_head & 0xF0 ) == 0xD0 ) len = 1;
      else if( ( message_head & 0xF0 ) == 0xE0 ) len = 2;
      else if( message_head == 0xF0 )
        len = std::distance( track->cur, std::find( track->cur, track->end, 0xF7 ) );
      else if( message_head == 0xF1 ) len = 1;
      else if( message_head == 0xF2 ) len = 2;
      else if( message_head == 0xF3 ) len = 1;
      else if( message_head == 0xF6 ) len = 0;
      else if( message_head == 0xF8 ) len = 0;
      else if( message_head == 0xFA ) len = 0;
      else if( message_head == 0xFB ) len = 0;
      else if( message_head == 0xFC ) len = 0;
      else if( message_head == 0xFE ) len = 0; 
      else if( message_head == 0xFF ) {
        if( std::distance( track->cur, track->end ) < 2 ) {
          throw invalid_midi_message();
        }
        auto length = *std::next( track->cur );
        len = length + 2;
      }
      else {
        throw invalid_midi_message();
      }
      if( std::distance( track->cur, track->end ) < int( len ) ) {
        throw invalid_midi_message();
      }
      return std::next( track->cur, len );
    }
    void consume_message( track_t *track ) {
      track->cur = get_message_end( track );
    }
    void parse_time( track_t *track ) {
      if( track->cur == track->end ) return;
      const auto &[message_head,left] = decode_variable_length_quantity( track->cur, track->end );
      track->cur = message_head;
      track->left = left;
    }
    void next( track_t *track ) {
      if( track->left != 0 ) throw invalid_midi_operation();
      consume_message( track );
      parse_time( track );
    }
    void insert( track_t *track ) {
      if( track->cur == track->end ) return;
      if( !head ) {
        head = track;
        return;
      }
      else if( head->left > track->left ) {
        track->next = head;
        head = track;
        return;
      }
      else {
        auto prev = head;
        auto cur = head->next;
        while( cur ) {
          if( cur->left > track->left ) {
            prev->next = track;
            track->next = cur;
            return;
          }
          prev = cur;
          cur = cur->next;
        }
        prev->next = track;
        track->next = nullptr;
      }
    }
    smf_header_t header;
    Iterator gbegin;
    track_t *head;
    uint64_t overrun;
    std::vector< std::tuple< step_t, Iterator, Iterator > > events;
    std::array< track_t, 32u > tracks;
    uint64_t nspb;
  };
}

#endif

