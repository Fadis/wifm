#ifndef SMFP_MIDI_PARSER_HPP
#define SMFP_MIDI_PARSER_HPP
#include <smfp/channel_state.hpp>
#include <smfp/active_note.hpp>
#include <smfp/exceptions.hpp>
#include <smfp/header.hpp>
#include <smfp/track.hpp>
#include <smfp/get_volume.hpp>
#include <smfp/get_frequency.hpp>
#include <cmath>
#include <stack>
#include <array>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <vector>
#include <cstdint>
#include <bit>
#include <thread>
#include <type_traits>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace smfp {
  namespace mi = boost::multi_index;
  struct by_note {};
  struct by_order {};
  struct by_slot {};
  template< typename Handler >
  class midi_parser_t {
    using slot_map_t = mi::multi_index_container<
      active_note_t,
      mi::indexed_by<
        mi::ordered_unique<
          mi::tag< by_note >,
          mi::member< active_note_t, uint16_t, &active_note_t::channel_note >
        >,
        mi::ordered_unique<
          mi::tag< by_order >,
          mi::member< active_note_t, note_order_t, &active_note_t::order >
        >,
        mi::ordered_unique<
          mi::tag< by_slot >,
          mi::member< active_note_t, slot_t, &active_note_t::slot >
        >
      >
    >;
  public:
    midi_parser_t( Handler &handler_ ) : handler( handler_ ), note_count( 0 ) {
      for( uint32_t i = 0; i != handler.size(); ++i )
        available_slots.push( i );
      channel_state.resize( handler.size(), channel_state_t( &global_state ) );
    }
    template< typename Iterator >
    void operator()( uint8_t status, const Iterator &begin, const Iterator &end ) {
      if( ( status & 0xF0 ) == 0x80 ) {
        const auto channel = status & 0x0F;
        const auto note = *begin;
        note_off( channel, note );
      }
      else if( ( status & 0xF0 ) == 0x90 ) {
        const auto channel = status & 0x0F;
        const auto note = *begin;
        const auto velocity = *std::next( begin );
        if( velocity != 0u ) {
          auto slot = get_slot( channel, note );
          note_on( slot, channel, note, velocity );
        }
        else note_off( channel, note );
      }
      else if( ( status & 0xF0 ) == 0xA0 ) {
        const auto channel = status & 0x0F;
        const auto note = *begin;
        const auto value = *std::next( begin );
        polyphonic_key_pressure( channel, note, value );
      }
      else if( ( status & 0xF0 ) == 0xB0 ) {
        const auto channel = status & 0x0F;
        const auto target = *begin;
        const auto value = *std::next( begin );
        control_change( channel, target, value );
      }
      else if( ( status & 0xF0 ) == 0xC0 ) {
        const auto channel = status & 0x0F;
        const auto prog = *begin;
        program_change( channel, prog );
      }
      else if( ( status & 0xF0 ) == 0xD0 ) {
        const auto channel = status & 0x0F;
        const auto pressure = *begin;
        channel_pressure( channel, pressure );
      }
      else if( ( status & 0xF0 ) == 0xE0 ) {
        const auto channel = status & 0x0F;
        const auto lsb = *begin;
        const auto msb = *std::next( begin );
        auto uvalue = int32_t( lsb ) | ( int32_t( msb ) << 7 );
        auto svalue = ( uvalue > 8191 ) ? ( uvalue - 16384 ) : uvalue;
        pitch_bend( channel, svalue );
      }
      else if( status == 0xF0 ) {
        system_exclusive( begin, end );
      }
      else if( status == 0xF1 ) {
      }
      else if( status == 0xF2 ) {
      }
      else if( status == 0xF3 ) {
      }
      else if( status == 0xF6 ) {
      }
      else if( status == 0xFF ) {
      }
      else throw invalid_midi_message();
    }
  private:
    void clear( const active_note_t &slot_info ) {
      auto &cst = channel_state[ slot_info.channel_note >> 8 ];
      handler[ slot_info.slot ].clear( cst );
    }
    slot_t get_slot( channel_t channel, note_t note ) {
      const auto cn = ( uint16_t( channel ) << 8 )| note;
      const std::array< slot_map_t*, 3u > slot_maps{ &note_off_map, &delayed_note_off_map, &note_on_map };
      for( auto slot_map: slot_maps ) {
        if( !slot_map->empty() ) {
          auto same_note = slot_map->get< by_note >().find( cn );
          if( same_note != slot_map->get< by_note >().end() ) {
            clear( *same_note );
            const auto slot = same_note->slot;
            slot_map->get< by_note >().erase( same_note );
            return slot;
          }
        }
      }
      if( !available_slots.empty() ) {
        auto slot = available_slots.top();
        available_slots.pop();
        return slot;
      }
      for( auto slot_map: slot_maps ) {
        if( !slot_map->empty() ) {
          auto oldest = slot_map->get< by_order >().begin();
          clear( *oldest );
          slot_map->get< by_order >().erase( oldest );
          return oldest->slot;
        }
      }
      throw slot_lost();
    }
    void note_on( slot_t slot, channel_t channel, note_t note, uint8_t velocity ) {
      const auto cn = ( uint16_t( channel ) << 8 )| note;
      auto &cst = channel_state[ channel ];
      auto note_info = active_note_t()
        .set_slot( slot )
        .set_channel_note( cn )
        .set_order( ++note_count )
        .set_velocity( velocity )
        .set_polyphonic_key_pressure( 127u );
      auto [slot_iter,is_new] = note_on_map.insert( note_info );
      if( !is_new ) throw invalid_midi_operation();
      handler[ slot ].note_on( cst, note_info );
    }
    void note_off( channel_t channel, note_t note ) {
      auto &cst = channel_state[ channel ];
      const auto cn = ( uint16_t( channel ) << 8 )| note;
      if( !note_on_map.empty() ) {
        auto same_note = note_on_map.get< by_note >().find( cn );
        if( same_note != note_on_map.get< by_note >().end() ) {
          auto note_info = *same_note;
          if( cst[ channel_variable_id_t::hold1 ] <= 0x3000u )
            note_off_map.insert( note_info );
          else
            delayed_note_off_map.insert( note_info );
          note_on_map.get< by_note >().erase( same_note );
          if( cst[ channel_variable_id_t::hold1 ] <= 0x3000u )
            handler[ same_note->slot ].note_off( cst );
        }
      }
    }
    void set_hold1_msb( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst[ channel_variable_id_t::hold1 ];
      uint16_t shifted = uint16_t( value ) << 7;
      if( old == shifted ) return;
      cst[ channel_variable_id_t::hold1 ] = shifted;
      if( !( shifted <= 0x3000u && old > 0x3000u ) ) return;
      auto begin = delayed_note_off_map.get< by_note >().lower_bound( uint16_t( channel ) << 8 );
      auto end = delayed_note_off_map.get< by_note >().lower_bound( uint16_t( channel + 1 ) << 8 );
      std::vector< slot_t > removed;
      for( auto iter = begin; iter != end; ++iter ) {
        note_off_map.insert( *iter );
        removed.push_back( iter->slot );
      }
      for( auto slot : removed )
        delayed_note_off_map.get< by_slot >().erase( slot );
      for( auto slot : removed )
        handler[ slot ].note_off( cst );
    }
    template< typename F >
    void run_on_existing_note( channel_t channel, note_t note, F &&func ) {
      const std::array< slot_map_t*, 3u > slot_maps{ &note_on_map, &delayed_note_off_map, &note_off_map };
      for( auto slot_map: slot_maps ){
        auto found = slot_map->get< by_note >().find( ( uint16_t( channel ) << 8 ) | note );
        if( found != slot_map->get< by_note >().end() ) {
          func( *found );
          break;
        }
      }
    }
    template< typename F >
    void for_each_notes_in_channel( channel_t channel, F &&func ) {
      const std::array< slot_map_t*, 3u > slot_maps{ &note_on_map, &delayed_note_off_map, &note_off_map };
      for( auto slot_map: slot_maps ){
        auto begin = slot_map->get< by_note >().lower_bound( uint16_t( channel ) << 8 );
        auto end = slot_map->get< by_note >().lower_bound( uint16_t( channel + 1 ) << 8 );
        for( auto iter = begin; iter != end; ++iter ) func( *iter );
      }
    }
    template< typename F >
    void for_each_notes( F &&func ) {
      const std::array< slot_map_t*, 3u > slot_maps{ &note_on_map, &delayed_note_off_map, &note_off_map };
      for( auto slot_map: slot_maps ){
        auto begin = slot_map->get< by_note >().begin();
        auto end = slot_map->get< by_note >().end();
        for( auto iter = begin; iter != end; ++iter ) func( *iter );
      }
    }
    void recalculate_volume( channel_t channel ) {
      auto &cst = channel_state[ channel ];
      for_each_notes_in_channel( channel, [&]( const auto &note_info ) {
        handler[ note_info.slot ].set_volume( cst, get_volume( cst, note_info ) );
      } );
    }
#define SMFP_CHANNEL_VOLUME_CHANGE( name ) \
    void set_ ## name ## _msb ( channel_t channel, uint8_t value ) {\
      auto &cst = channel_state[ channel ];\
      auto old = cst[ channel_variable_id_t :: name ];\
      auto new_ = ( uint16_t( value ) << 7 ) | ( old & 0x7F ); \
      if( old != new_ ) { \
        cst[ channel_variable_id_t :: name ] = new_;\
        recalculate_volume( channel );\
      } \
    } \
    void set_ ## name ## _lsb ( channel_t channel, uint8_t value ) {\
      auto &cst = channel_state[ channel ];\
      auto old = cst[ channel_variable_id_t :: name ];\
      auto new_ = ( uint16_t( value ) & 0x7F ) | ( old & 0x3F80 ); \
      if( old != new_ ) { \
        cst[ channel_variable_id_t :: name ] = new_;\
        recalculate_volume( channel );\
      } \
    }
    SMFP_CHANNEL_VOLUME_CHANGE( volume )
    SMFP_CHANNEL_VOLUME_CHANGE( expression )
    SMFP_CHANNEL_VOLUME_CHANGE( breath )
    SMFP_CHANNEL_VOLUME_CHANGE( foot )
    SMFP_CHANNEL_VOLUME_CHANGE( soft )
    void polyphonic_key_pressure( channel_t channel, note_t note, uint8_t /*value*/ ) {
      auto &cst = channel_state[ channel ];
      run_on_existing_note( channel, note, [&]( const auto &note_info ) {
        handler[ note_info.slot ].set_volume( cst, get_volume( cst, note_info ) );
      } );
    }
    void channel_pressure( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst.pressure;
      auto new_ = value;
      if( old != new_ ) {
        cst.set_pressure( new_ );
        recalculate_volume( channel );
      }
    }
    void program_change( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst.program;
      auto new_ = value;
      if( old != new_ ) {
        cst.set_program( new_ );
        for_each_notes_in_channel( channel, [&]( const auto &note_info ) {
          handler[ note_info.slot ].set_program ( cst, new_ );
        });
      }
    }
    void set_portamento_time_msb ( channel_t channel, uint8_t value ) {
      set_portamento_time< true >( channel, value );
    }
    void set_portamento_time_lsb ( channel_t channel, uint8_t value ) {
      set_portamento_time< false >( channel, value );
    }
    template< bool msb >
    void set_portamento_time( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst[ channel_variable_id_t:: portamento_time ];
      auto new_ = msb ?
        ( uint16_t( value ) << 7 ) | ( old & 0x7F ) :
        ( uint16_t( value ) & 0x7F ) | ( old & 0x3F80 );
      if( old != new_ ) {
        cst[ channel_variable_id_t :: portamento_time ] = new_;
        if( cst[ channel_variable_id_t :: portamento_switch ] < 0x2000u ) return;
        for_each_notes_in_channel( channel, [&]( const auto &note_info ) {
          handler[ note_info.slot ].set_variable(
            channel_variable_id_t::portamento_time, 0,
            cst
          );
        } );
      }
    }
    void set_portamento_switch_msb ( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst[ channel_variable_id_t:: portamento_switch ];
      auto new_ = ( uint16_t( value ) << 7 ) | ( old & 0x7F );
      if( old != new_ ) {
        cst[ channel_variable_id_t :: portamento_time ] = new_;
        if( cst[ channel_variable_id_t :: portamento_time ] == 0u ) return;
        for_each_notes_in_channel( channel, [&]( const auto &note_info ) {
          handler[ note_info.slot ].set_variable(
            channel_variable_id_t::portamento_time, 0,
            cst
          );
        } );
      }
    }
#define SMFP_SET_CHANNEL_CONTROL( name ) \
    void set_ ## name ## _msb ( channel_t channel, uint8_t value ) {\
      set_ ## name < true >( channel, value ); \
    } \
    void set_ ## name ## _lsb ( channel_t channel, uint8_t value ) {\
      set_ ## name < false >( channel, value ); \
    }\
    template< bool msb > \
    void set_ ## name ( channel_t channel, uint8_t value ) {\
      auto &cst = channel_state[ channel ];\
      auto old = cst[ channel_variable_id_t:: name ];\
      auto new_ = msb ? \
        ( uint16_t( value ) << 7 ) | ( old & 0x7F ) : \
        ( uint16_t( value ) & 0x7F ) | ( old & 0x3F80 ); \
      if( old != new_ ) {\
        cst[ channel_variable_id_t :: name ] = new_;\
        for_each_notes_in_channel( channel, [&]( const auto &note_info ) {\
          handler[ note_info.slot ].set_variable (\
            channel_variable_id_t:: name, 0,\
            cst \
          );\
        } );\
      }\
    }
    SMFP_SET_CHANNEL_CONTROL( bank );
    SMFP_SET_CHANNEL_CONTROL( modulation );
    SMFP_SET_CHANNEL_CONTROL( balance );
    SMFP_SET_CHANNEL_CONTROL( pan );
    SMFP_SET_CHANNEL_CONTROL( effect1 );
    SMFP_SET_CHANNEL_CONTROL( effect2 );
    SMFP_SET_CHANNEL_CONTROL( reverb );
    SMFP_SET_CHANNEL_CONTROL( tremolo );
    SMFP_SET_CHANNEL_CONTROL( chorus );
    SMFP_SET_CHANNEL_CONTROL( celeste );
    SMFP_SET_CHANNEL_CONTROL( phaser );
    SMFP_SET_CHANNEL_CONTROL( general_purpose1 );
    SMFP_SET_CHANNEL_CONTROL( general_purpose2 );
    SMFP_SET_CHANNEL_CONTROL( general_purpose3 );
    SMFP_SET_CHANNEL_CONTROL( general_purpose4 );
    SMFP_SET_CHANNEL_CONTROL( general_purpose5 );
    SMFP_SET_CHANNEL_CONTROL( general_purpose6 );
    SMFP_SET_CHANNEL_CONTROL( general_purpose7 );
    SMFP_SET_CHANNEL_CONTROL( general_purpose8 );
    SMFP_SET_CHANNEL_CONTROL( legato );
    SMFP_SET_CHANNEL_CONTROL( hold2 );
    SMFP_SET_CHANNEL_CONTROL( variation );
    SMFP_SET_CHANNEL_CONTROL( timbre );
    SMFP_SET_CHANNEL_CONTROL( release );
    SMFP_SET_CHANNEL_CONTROL( attack );
    SMFP_SET_CHANNEL_CONTROL( brightness );
    SMFP_SET_CHANNEL_CONTROL( decay );
    SMFP_SET_CHANNEL_CONTROL( vibrato_rate );
    SMFP_SET_CHANNEL_CONTROL( vibrato_depth );
    SMFP_SET_CHANNEL_CONTROL( vibrato_delay );
    void set_nrpn_msb ( channel_t channel, uint8_t value ) {
      set_nrpn< true >( channel, value );
    }
    void set_nrpn_lsb ( channel_t channel, uint8_t value ) {
      set_nrpn< false >( channel, value );
    }
    template< bool msb >
    void set_nrpn( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst[ channel_variable_id_t::nrpn ];
      auto new_ = msb ?
        ( uint16_t( value ) << 7 ) | ( old & 0x7F ) :
        ( uint16_t( value ) & 0x7F ) | ( old & 0x3F80 );
      cst[ channel_variable_id_t::nrpn ] = new_;
      cst[ channel_variable_id_t::rpn ] = 0xFFFF;
      cst[ channel_variable_id_t::data_entry ] = 0xC000;
    }
    void set_rpn_msb ( channel_t channel, uint8_t value ) {
      set_rpn< true >( channel, value );
    }
    void set_rpn_lsb ( channel_t channel, uint8_t value ) {
      set_rpn< false >( channel, value );
    }
    template< bool msb >
    void set_rpn( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst[ channel_variable_id_t::rpn ];
      auto new_ = msb ?
        ( uint16_t( value ) << 7 ) | ( old & 0x7F ) :
        ( uint16_t( value ) & 0x7F ) | ( old & 0x3F80 );
      cst[ channel_variable_id_t::rpn ] = new_;
      cst[ channel_variable_id_t::nrpn ] = 0xFFFF;
      cst[ channel_variable_id_t::data_entry ] = 0xC000;
    }
    void set_data_entry_msb ( channel_t channel, uint8_t value ) {
      set_data_entry< true >( channel, value );
    }
    void set_data_entry_lsb ( channel_t channel, uint8_t value ) {
      set_data_entry< false >( channel, value );
    }
#define SMFP_SET_PARAMETER( name ) \
      {\
        cst[ channel_variable_id_t :: name ] = new_;\
        for_each_notes_in_channel( channel, [&]( const auto &note_info ) {\
          handler[ note_info.slot ].set_variable (\
            channel_variable_id_t:: name, 0,\
            cst \
          );\
        } );\
      }
#define SMFP_SET_DRUM_PARAMETER( name ) \
      { \
        cst[ channel_variable_id_t( int( channel_variable_id_t:: name ) + int( cst[ channel_variable_id_t::nrpn ] & 0x7F ) ) ] = new_; \
        for_each_notes_in_channel( channel, [&]( const auto &note_info ) { \
          handler[ note_info.slot ].set_variable ( \
            channel_variable_id_t:: name, ( cst[ channel_variable_id_t::nrpn ] & 0x7F ), \
            cst \
          ); \
        } ); \
      }
    template< bool msb >
    void set_data_entry( channel_t channel, uint8_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst[ channel_variable_id_t::data_entry ];
      if( ( old & 0xC000 ) == 0 ) old = 0xC000;
      auto new_ = msb ?
        ( uint16_t( value ) << 7 ) | ( old & 0x407F ) :
        ( uint16_t( value ) & 0x7F ) | ( old & 0xBF80 );
      if( old != new_ ) {
        cst[ channel_variable_id_t::data_entry ] = new_;
        if( ( new_ & 0xC000 ) == 0 ) {
          if( cst[ channel_variable_id_t::rpn ] == 0u )
            SMFP_SET_PARAMETER( pitch_bend_sensitivity )
          else if( cst[ channel_variable_id_t::rpn ] == 1u )
            SMFP_SET_PARAMETER( master_fine_tune )
          else if( cst[ channel_variable_id_t::rpn ] == 2u )
            SMFP_SET_PARAMETER( master_coarse_tune )
          else if( cst[ channel_variable_id_t::rpn ] == 5u )
            SMFP_SET_PARAMETER( modulation_depth_range )
          else if( cst[ channel_variable_id_t::rpn ] == 7u ) {
            cst[ channel_variable_id_t::rpn ] = 0xFFFF;
            cst[ channel_variable_id_t::nrpn ] = 0xFFFF;
            cst[ channel_variable_id_t::data_entry ] = 0xC000;
          }
          else if( cst[ channel_variable_id_t::rpn ] == 5u )
            SMFP_SET_PARAMETER( modulation_depth_range )
          else if( global_state.standard == midi_standard_id_t::gs ) {
            if( cst[ channel_variable_id_t::nrpn ] == 0x0088 )
              SMFP_SET_PARAMETER( vibrato_rate_gs )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x0089 )
              SMFP_SET_PARAMETER( vibrato_depth_gs )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x008A )
              SMFP_SET_PARAMETER( vibrato_delay_gs )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00A0 )
              SMFP_SET_PARAMETER( tvf_cutoff_freq )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00A1 )
              SMFP_SET_PARAMETER( tvf_resonance )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00E3 )
              SMFP_SET_PARAMETER( tvf_tva_envelope_attack_time )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00E4 )
              SMFP_SET_PARAMETER( tvf_tva_envelope_decay_time )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00E6 )
              SMFP_SET_PARAMETER( tvf_tva_envelope_release_time )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x18 )
              SMFP_SET_DRUM_PARAMETER( drum_pitch )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x1A )
              SMFP_SET_DRUM_PARAMETER( drum_tva )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x1D )
              SMFP_SET_DRUM_PARAMETER( drum_reverb )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x1E )
              SMFP_SET_DRUM_PARAMETER( drum_chorus )
          }
          else if( global_state.standard == midi_standard_id_t::xg ) {
            if( cst[ channel_variable_id_t::nrpn ] == 0x0088 )
              SMFP_SET_PARAMETER( vibrato_rate_xg )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x0089 )
              SMFP_SET_PARAMETER( vibrato_depth_xg )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x008A )
              SMFP_SET_PARAMETER( vibrato_delay_xg )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00A0 )
              SMFP_SET_PARAMETER( tvf_cutoff_freq )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00A1 )
              SMFP_SET_PARAMETER( tvf_resonance )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00A4 )
              SMFP_SET_PARAMETER( hpf_cutoff_freq )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00A5 )
              SMFP_SET_PARAMETER( hpf_resonance )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B0 )
              SMFP_SET_PARAMETER( eq_bass )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B1 )
              SMFP_SET_PARAMETER( eq_treble )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B2 )
              SMFP_SET_PARAMETER( eq_mid_bass )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B3 )
              SMFP_SET_PARAMETER( eq_mid_treble )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B4 )
              SMFP_SET_PARAMETER( eq_bass_frequency )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B5 )
              SMFP_SET_PARAMETER( eq_treble_frequency )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B6 )
              SMFP_SET_PARAMETER( eq_mid_bass_frequency )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00B7 )
              SMFP_SET_PARAMETER( eq_mid_treble_frequency )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00E3 )
              SMFP_SET_PARAMETER( tvf_tva_envelope_attack_time )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00E4 )
              SMFP_SET_PARAMETER( tvf_tva_envelope_decay_time )
            else if( cst[ channel_variable_id_t::nrpn ] == 0x00E6 )
              SMFP_SET_PARAMETER( tvf_tva_envelope_release_time )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x18 )
              SMFP_SET_DRUM_PARAMETER( drum_pitch )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x1A )
              SMFP_SET_DRUM_PARAMETER( drum_tva )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x1D )
              SMFP_SET_DRUM_PARAMETER( drum_reverb )
            else if( ( cst[ channel_variable_id_t::nrpn ] >> 7 ) == 0x1E )
              SMFP_SET_DRUM_PARAMETER( drum_chorus )
          }
        }
      }
    }
    void control_change( channel_t channel, uint8_t target, uint8_t value ) {
      if( target == 0 ) set_bank_msb( channel, value );
      else if( target == 1 ) set_modulation_msb( channel, value );
      else if( target == 2 ) set_breath_msb( channel, value );
      else if( target == 4 ) set_foot_msb( channel, value );
      else if( target == 5 ) set_portamento_time_msb( channel, value );
      else if( target == 6 ) set_data_entry_msb( channel, value );
      else if( target == 7 ) set_volume_msb( channel, value );
      else if( target == 8 ) set_balance_msb( channel, value );
      else if( target == 10 ) set_pan_msb( channel, value );
      else if( target == 11 ) set_expression_msb( channel, value );
      else if( target == 12 ) set_effect1_msb( channel, value );
      else if( target == 13 ) set_effect2_msb( channel, value );
      else if( target == 16 ) set_general_purpose1_msb( channel, value );
      else if( target == 17 ) set_general_purpose2_msb( channel, value );
      else if( target == 18 ) set_general_purpose3_msb( channel, value );
      else if( target == 19 ) set_general_purpose4_msb( channel, value );
      else if( target == 32 ) set_bank_lsb( channel, value );
      else if( target == 33 ) set_modulation_lsb( channel, value );
      else if( target == 34 ) set_breath_lsb( channel, value );
      else if( target == 36 ) set_foot_lsb( channel, value );
      else if( target == 37 ) set_portamento_time_lsb( channel, value );
      else if( target == 38 ) set_data_entry_lsb( channel, value );
      else if( target == 39 ) set_volume_lsb( channel, value );
      else if( target == 40 ) set_balance_lsb( channel, value );
      else if( target == 42 ) set_pan_lsb( channel, value );
      else if( target == 43 ) set_expression_lsb( channel, value );
      else if( target == 44 ) set_effect1_lsb( channel, value );
      else if( target == 45 ) set_effect2_lsb( channel, value );
      else if( target == 48 ) set_general_purpose1_lsb( channel, value );
      else if( target == 49 ) set_general_purpose2_lsb( channel, value );
      else if( target == 50 ) set_general_purpose3_lsb( channel, value );
      else if( target == 51 ) set_general_purpose4_lsb( channel, value );
      else if( target == 64 ) set_hold1_msb( channel, value );
      else if( target == 65 ) set_portamento_switch_msb( channel, value );
      else if( target == 67 ) set_soft_msb( channel, value );
      else if( target == 68 ) set_legato_msb( channel, value );
      else if( target == 69 ) set_hold2_msb( channel, value );
      else if( target == 70 ) set_variation_msb( channel, value );
      else if( target == 71 ) set_timbre_msb( channel, value );
      else if( target == 72 ) set_release_msb( channel, value );
      else if( target == 73 ) set_attack_msb( channel, value );
      else if( target == 74 ) set_brightness_msb( channel, value );
      else if( target == 75 ) set_decay_msb( channel, value );
      else if( target == 76 ) set_vibrato_rate_msb( channel, value );
      else if( target == 77 ) set_vibrato_depth_msb( channel, value );
      else if( target == 78 ) set_vibrato_delay_msb( channel, value );
      else if( target == 80 ) set_general_purpose5_msb( channel, value );
      else if( target == 81 ) set_general_purpose6_msb( channel, value );
      else if( target == 82 ) set_general_purpose7_msb( channel, value );
      else if( target == 83 ) set_general_purpose8_msb( channel, value );
      else if( target == 91 ) set_reverb_msb( channel, value );
      else if( target == 92 ) set_tremolo_msb( channel, value );
      else if( target == 93 ) set_chorus_msb( channel, value );
      else if( target == 94 ) set_celeste_msb( channel, value );
      else if( target == 95 ) set_phaser_msb( channel, value );
      else if( target == 98 ) set_nrpn_lsb( channel, value );
      else if( target == 99 ) set_nrpn_msb( channel, value );
      else if( target == 100 ) set_rpn_lsb( channel, value );
      else if( target == 101 ) set_rpn_msb( channel, value );
      else {
        //throw invalid_midi_message();
      }
    }
    void pitch_bend( channel_t channel, int16_t value ) {
      auto &cst = channel_state[ channel ];
      auto old = cst.pitch_bend;
      auto new_ = value;
      if( old != new_ ) {
        cst.set_pitch_bend( new_ );
        for_each_notes_in_channel( channel, [&]( const auto &note_info ) {
          handler[ note_info.slot ].set_frequency( cst, get_frequency( cst, note_info ) );
        });
      }
    }
    template< typename Iterator >
    bool xg_system_exclusive( uint32_t address, Iterator begin, Iterator end ) {
      if( address == 0x7e ) {
        if( std::distance( begin, end ) != 1 ) {
          throw invalid_midi_message();
        }
        if( *begin == 0 ) {
          global_state.set_standard( midi_standard_id_t::xg );
          return true;
        }
        else {
          throw invalid_midi_message();
        }
      }
      return false;
    }
    template< typename Iterator >
    bool gs_system_exclusive( uint32_t address, Iterator begin, Iterator end ) {
      if( address == (( 0x40 << 14 )|( 0x7f )) ) {
        if( std::distance( begin, end ) != 1 ) {
          throw invalid_midi_message();
        }
        if( *begin == 0 ) {
          global_state.set_standard( midi_standard_id_t::gs );
          return true;
        }
      }
      return false;
    }
    template< typename Iterator >
    bool gm_system_exclusive( uint8_t address, Iterator /*begin*/, Iterator /*end*/ ) {
      if( address == 0x01 ) {
        global_state.set_standard( midi_standard_id_t::gm );
        return true;
      }
      else if( address == 0x02 ) {
        global_state.set_standard( midi_standard_id_t::gm );
        return true;
      }
      else if( address == 0x03 ) {
        global_state.set_standard( midi_standard_id_t::gm2 );
        return true;
      }
      return false;
    }
    template< typename Iterator >
    bool non_realtime_universal_system_exclusive( uint8_t /*address*/, uint8_t sub_id1, uint8_t sub_id2, Iterator begin, Iterator end ) {
      if( sub_id1 == 0x09 ) {
        return gm_system_exclusive( sub_id2, begin, end );
      }
      return false;
      
    }
    template< typename Iterator >
    void system_exclusive( Iterator begin, Iterator end ) {
      constexpr std::array< uint8_t, 3u > xg{ 0x43, 0x10, 0x4c };
      constexpr std::array< uint8_t, 9u > gs{ 0x41, 0x10, 0x42 };
      auto old = global_state.standard;
      if( begin == end ) {
        throw invalid_midi_message();
      }
      auto cur = begin;
      auto length = *cur - 1;
      ++cur;
      if( std::distance( cur, end ) < length ) {
        throw invalid_midi_message();
      }
      auto sysex_begin = cur;
      auto sysex_end = std::next( sysex_begin, length );
      bool known_sysex = false;
      if( length > 1 && *cur == 0x7e ) {
        if( length < 4 ) {
          throw invalid_midi_message();
        }
        ++cur;
        const auto address = *cur;
        ++cur;
        const auto sub_id1 = *cur;
        ++cur;
        const auto sub_id2 = *cur;
        ++cur;
        known_sysex = non_realtime_universal_system_exclusive( address, sub_id1, sub_id2, cur, sysex_end );
      }
      else if( size_t( length ) > xg.size() && std::equal( xg.begin(), xg.end(), cur, std::next( cur, xg.size() ) ) ) {
        if( size_t( length ) < xg.size() + 4 ) {
          throw invalid_midi_message();
        }
        cur = std::next( cur, xg.size() );
        uint32_t address = *cur;
        ++cur;
        address <<= 7;
        address |= *cur;
        ++cur;
        address <<= 7;
        address |= *cur;
        ++cur;
        known_sysex = xg_system_exclusive( address, cur, end );
      }
      else if( size_t( length ) > gs.size() && std::equal( gs.begin(), gs.end(), cur, std::next( cur, gs.size() ) ) ) {
        if( size_t( length ) < gs.size() + 3 ) {
          throw invalid_midi_message();
        }
        cur = std::next( cur, gs.size() );
        auto sum = 128 - ( std::accumulate( cur, std::prev( sysex_end ), 0 ) ) % 128;
        if( sum == 128 ) sum = 0;
        if( *std::prev( sysex_end ) != sum ) {
          throw invalid_midi_message();
        }
        uint32_t address = *cur;
        ++cur;
        address <<= 7;
        address |= *cur;
        ++cur;
        address <<= 7;
        address |= *cur;
        ++cur;
        known_sysex = gs_system_exclusive( address, cur, std::prev( sysex_end ) );
      }
      if( !known_sysex ) {
        for_each_notes( [&]( const auto &note_info ) {
          handler[ note_info.slot ].system_exclusive( channel_state[ note_info.channel_note >> 8 ], sysex_begin, sysex_end );
        } );
      }
      else if( global_state.standard != old ) {
        for_each_notes( [&]( const auto &note_info ) {
          handler[ note_info.slot ].clear( channel_state[ note_info.channel_note >> 8 ] );
        } );
      }
    }
    Handler &handler;
    slot_map_t note_on_map;
    slot_map_t note_off_map;
    slot_map_t delayed_note_off_map;
    std::stack< slot_t > available_slots;
    std::vector< channel_state_t > channel_state;
    uint64_t note_count;
    global_state_t global_state;
  };
}

#endif

