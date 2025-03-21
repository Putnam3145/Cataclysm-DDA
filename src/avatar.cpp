#include "avatar.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "action.h"
#include "activity_type.h"
#include "activity_actor_definitions.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_assert.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "field_type.h"
#include "game.h"
#include "game_constants.h"
#include "help.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "kill_tracker.h"
#include "make_static.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_memory.h"
#include "martialarts.h"
#include "messages.h"
#include "mission.h"
#include "morale.h"
#include "morale_types.h"
#include "move_mode.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_activity.h"
#include "profession.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "talker.h"
#include "talker_avatar.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

static const activity_id ACT_READ( "ACT_READ" );

static const bionic_id bio_cloak( "bio_cloak" );
static const bionic_id bio_cqb( "bio_cqb" );
static const bionic_id bio_soporific( "bio_soporific" );

static const efftype_id effect_alarm_clock( "alarm_clock" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_depressants( "depressants" );
static const efftype_id effect_happy( "happy" );
static const efftype_id effect_irradiated( "irradiated" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_pkill( "pkill" );
static const efftype_id effect_sad( "sad" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_sleep_deprived( "sleep_deprived" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );
static const efftype_id effect_stim( "stim" );
static const efftype_id effect_stim_overdose( "stim_overdose" );
static const efftype_id effect_stunned( "stunned" );

static const itype_id itype_guidebook( "guidebook" );

static const trait_id trait_ARACHNID_ARMS( "ARACHNID_ARMS" );
static const trait_id trait_ARACHNID_ARMS_OK( "ARACHNID_ARMS_OK" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_CHITIN2( "CHITIN2" );
static const trait_id trait_CHITIN3( "CHITIN3" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_COMPOUND_EYES( "COMPOUND_EYES" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_INSECT_ARMS( "INSECT_ARMS" );
static const trait_id trait_INSECT_ARMS_OK( "INSECT_ARMS_OK" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PROF_DICEMASTER( "PROF_DICEMASTER" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_STIMBOOST( "STIMBOOST" );
static const trait_id trait_THICK_SCALES( "THICK_SCALES" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WALKER( "WEB_WALKER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );
static const trait_id trait_WEBBED( "WEBBED" );
static const trait_id trait_WHISKERS( "WHISKERS" );
static const trait_id trait_WHISKERS_RAT( "WHISKERS_RAT" );

static const json_character_flag json_flag_ALARMCLOCK( "ALARMCLOCK" );

avatar::avatar()
{
    player_map_memory = std::make_unique<map_memory>();
    show_map_memory = true;
    active_mission = nullptr;
    grab_type = object_type::NONE;
    calorie_diary.push_front( daily_calories{} );
}

avatar::~avatar() = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
avatar::avatar( avatar && ) = default;
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
avatar &avatar::operator=( avatar && ) = default;

static void swap_npc( npc &one, npc &two, npc &tmp )
{
    tmp = std::move( one );
    one = std::move( two );
    two = std::move( tmp );
}

void avatar::control_npc( npc &np )
{
    if( !np.is_player_ally() ) {
        debugmsg( "control_npc() called on non-allied npc %s", np.name );
        return;
    }
    if( !shadow_npc ) {
        shadow_npc = std::make_unique<npc>();
        shadow_npc->op_of_u.trust = 10;
        shadow_npc->op_of_u.value = 10;
        shadow_npc->set_attitude( NPCATT_FOLLOW );
    }
    npc tmp;
    // move avatar character data into shadow npc
    swap_character( *shadow_npc, tmp );
    // swap target npc with shadow npc
    swap_npc( *shadow_npc, np, tmp );
    // move shadow npc character data into avatar
    swap_character( *shadow_npc, tmp );
    // the avatar character is no longer a follower NPC
    g->remove_npc_follower( getID() );
    // the previous avatar character is now a follower
    g->add_npc_follower( np.getID() );
    np.set_fac( faction_id( "your_followers" ) );
    // perception and mutations may have changed, so reset light level caches
    g->reset_light_level();
    // center the map on the new avatar character
    g->vertical_shift( posz() );
    g->update_map( *this );
}

void avatar::toggle_map_memory()
{
    show_map_memory = !show_map_memory;
}

bool avatar::should_show_map_memory()
{
    return show_map_memory;
}

bool avatar::save_map_memory()
{
    return player_map_memory->save( get_map().getabs( pos() ) );
}

void avatar::load_map_memory()
{
    player_map_memory->load( get_map().getabs( pos() ) );
}

void avatar::prepare_map_memory_region( const tripoint &p1, const tripoint &p2 )
{
    player_map_memory->prepare_region( p1, p2 );
}

const memorized_terrain_tile &avatar::get_memorized_tile( const tripoint &pos ) const
{
    return player_map_memory->get_tile( pos );
}

void avatar::memorize_tile( const tripoint &pos, const std::string &ter, const int subtile,
                            const int rotation )
{
    player_map_memory->memorize_tile( pos, ter, subtile, rotation );
}

void avatar::memorize_symbol( const tripoint &pos, const int symbol )
{
    player_map_memory->memorize_symbol( pos, symbol );
}

int avatar::get_memorized_symbol( const tripoint &p ) const
{
    return player_map_memory->get_symbol( p );
}

void avatar::clear_memorized_tile( const tripoint &pos )
{
    player_map_memory->clear_memorized_tile( pos );
}

std::vector<mission *> avatar::get_active_missions() const
{
    return active_missions;
}

std::vector<mission *> avatar::get_completed_missions() const
{
    return completed_missions;
}

std::vector<mission *> avatar::get_failed_missions() const
{
    return failed_missions;
}

mission *avatar::get_active_mission() const
{
    return active_mission;
}

void avatar::reset_all_missions()
{
    active_mission = nullptr;
    active_missions.clear();
    completed_missions.clear();
    failed_missions.clear();
}

tripoint_abs_omt avatar::get_active_mission_target() const
{
    if( active_mission == nullptr ) {
        return overmap::invalid_tripoint;
    }
    return active_mission->get_target();
}

void avatar::set_active_mission( mission &cur_mission )
{
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &cur_mission );
    if( iter == active_missions.end() ) {
        debugmsg( "new active mission %d is not in the active_missions list", cur_mission.get_id() );
    } else {
        active_mission = &cur_mission;
    }
}

void avatar::on_mission_assignment( mission &new_mission )
{
    active_missions.push_back( &new_mission );
    set_active_mission( new_mission );
}

void avatar::on_mission_finished( mission &cur_mission )
{
    if( cur_mission.has_failed() ) {
        failed_missions.push_back( &cur_mission );
        add_msg_if_player( m_bad, _( "Mission \"%s\" is failed." ), cur_mission.name() );
    } else {
        completed_missions.push_back( &cur_mission );
        add_msg_if_player( m_good, _( "Mission \"%s\" is successfully completed." ),
                           cur_mission.name() );
    }
    const auto iter = std::find( active_missions.begin(), active_missions.end(), &cur_mission );
    if( iter == active_missions.end() ) {
        debugmsg( "completed mission %d was not in the active_missions list", cur_mission.get_id() );
    } else {
        active_missions.erase( iter );
    }
    if( &cur_mission == active_mission ) {
        if( active_missions.empty() ) {
            active_mission = nullptr;
        } else {
            active_mission = active_missions.front();
        }
    }
}

bool avatar::read( item_location &book, item_location ereader )
{
    if( !book ) {
        add_msg( m_info, _( "Never mind." ) );
        return false;
    }

    std::vector<std::string> fail_messages;
    const Character *reader = get_book_reader( *book, fail_messages );
    if( reader == nullptr ) {
        // We can't read, and neither can our followers
        for( const std::string &reason : fail_messages ) {
            add_msg( m_bad, reason );
        }
        return false;
    }

    // spells are handled in a different place
    // src/iuse_actor.cpp -> learn_spell_actor::use
    if( book->get_use( "learn_spell" ) ) {
        book->get_use( "learn_spell" )->call( *this, *book, book->active, pos() );
        return true;
    }

    bool continuous = false;
    const int time_taken = time_to_read( *book, *reader );
    add_msg_debug( debugmode::DF_ACT_READ, "avatar::read time_taken = %s",
                   to_string_writable( time_duration::from_moves( time_taken ) ) );

    // If the player hasn't read this book before, skim it to get an idea of what's in it.
    if( !has_identified( book->typeId() ) ) {
        if( reader != this ) {
            add_msg( m_info, fail_messages[0] );
            add_msg( m_info, _( "%s reads aloud…" ), reader->disp_name() );
        }

        assign_activity(
            player_activity(
                read_activity_actor(
                    time_taken,
                    book,
                    ereader,
                    false
                ) ) );

        return true;
    }

    if( book->typeId() == itype_guidebook ) {
        // special guidebook effect: print a misc. hint when read
        if( reader != this ) {
            add_msg( m_info, fail_messages[0] );
            dynamic_cast<const npc &>( *reader ).say( get_hint() );
        } else {
            add_msg( m_info, get_hint() );
        }
        get_event_bus().send<event_type::reads_book>( getID(), book->typeId() );
        mod_moves( -100 );
        return false;
    }

    const cata::value_ptr<islot_book> &type = book->type->book;
    const skill_id &skill = type->skill;
    const std::string skill_name = skill ? skill.obj().name() : "";

    // Find NPCs to join the study session:
    std::map<npc *, std::string> learners;
    //reading only for fun
    std::map<npc *, std::string> fun_learners;
    std::map<npc *, std::string> nonlearners;
    for( npc *elem : get_crafting_helpers() ) {
        const book_mastery mastery = elem->get_book_mastery( *book );
        const bool morale_req = elem->fun_to_read( *book ) || elem->has_morale_to_read();

        // Note that the reader cannot be a nonlearner
        // since a reader should always have enough morale to read
        // and at the very least be able to understand the book

        if( elem->is_deaf() && elem != reader ) {
            nonlearners.insert( { elem, _( " (deaf)" ) } );

        } else if( mastery == book_mastery::MASTERED && elem->fun_to_read( *book ) ) {
            fun_learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : "" } );

        } else if( mastery == book_mastery::LEARNING && morale_req ) {
            learners.insert( {elem, elem == reader ? _( " (reading aloud to you)" ) : ""} );
        } else {
            std::string reason = _( " (uninterested)" );

            if( !morale_req ) {
                reason = _( " (too sad)" );

            } else if( mastery == book_mastery::CANT_UNDERSTAND ) {
                reason = string_format( _( " (needs %d %s)" ), type->req, skill_name );

            } else if( mastery == book_mastery::MASTERED ) {
                reason = string_format( _( " (already has %d %s)" ), type->level, skill_name );

            }
            nonlearners.insert( { elem, reason } );
        }
    }

    int learner_id = -1;

    //only show the menu if there's useful information or multiple options
    if( skill || !nonlearners.empty() || !fun_learners.empty() ) {
        uilist menu;

        // Some helpers to reduce repetition:
        auto length = []( const std::pair<npc *, std::string> &elem ) {
            return utf8_width( elem.first->disp_name() ) + utf8_width( elem.second );
        };

        auto max_length = [&length]( const std::map<npc *, std::string> &m ) {
            auto max_ele = std::max_element( m.begin(),
                                             m.end(), [&length]( const std::pair<npc *, std::string> &left,
            const std::pair<npc *, std::string> &right ) {
                return length( left ) < length( right );
            } );
            return max_ele == m.end() ? 0 : length( *max_ele );
        };

        auto get_text =
        [&]( const std::map<npc *, std::string> &m, const std::pair<npc *, std::string> &elem ) {
            const int lvl = elem.first->get_knowledge_level( skill );
            const std::string lvl_text = skill ? string_format( _( " | current level: %d" ), lvl ) : "";
            const std::string name_text = elem.first->disp_name() + elem.second;
            return string_format( "%s%s", left_justify( name_text, max_length( m ) ), lvl_text );
        };

        auto add_header = [&menu]( const std::string & str ) {
            menu.addentry( -1, false, -1, "" );
            uilist_entry header( -1, false, -1, str, c_yellow, c_yellow );
            header.force_color = true;
            menu.entries.push_back( header );
        };

        menu.title = !skill ? string_format( _( "Reading %s" ), book->type_name() ) :
                     //~ %1$s: book name, %2$s: skill name, %3$d and %4$d: skill levels
                     string_format( _( "Reading %1$s (can train %2$s from %3$d to %4$d)" ), book->type_name(),
                                    skill_name, type->req, type->level );


        menu.addentry( 0, true, '0', _( "Read once" ) );

        const int lvl = get_knowledge_level( skill );
        menu.addentry( 2 + getID().get_value(), lvl < type->level, '1',
                       string_format( _( "Read until you gain a level | current level: %d" ), lvl ) );

        // select until player gets level by default
        if( lvl < type->level ) {
            menu.selected = 1;
        }

        if( !learners.empty() ) {
            add_header( _( "Read until this NPC gains a level:" ) );
            for( const std::pair<npc *const, std::string> &elem : learners ) {
                menu.addentry( 2 + elem.first->getID().get_value(), true, -1,
                               get_text( learners, elem ) );
            }
        }

        if( !fun_learners.empty() ) {
            add_header( _( "Reading for fun:" ) );
            for( const std::pair<npc *const, std::string> &elem : fun_learners ) {
                menu.addentry( -1, false, -1, get_text( fun_learners, elem ) );
            }
        }

        if( !nonlearners.empty() ) {
            add_header( _( "Not participating:" ) );
            for( const std::pair<npc *const, std::string> &elem : nonlearners ) {
                menu.addentry( -1, false, -1, get_text( nonlearners, elem ) );
            }
        }

        menu.query( true );
        if( menu.ret == UILIST_CANCEL ) {
            add_msg( m_info, _( "Never mind." ) );
            return false;
        } else if( menu.ret >= 2 ) {
            continuous = true;
            learner_id = menu.ret - 2;
        }
    }

    const bool is_martialarts = book->type->use_methods.count( "MA_MANUAL" );
    if( is_martialarts ) {

        if( martial_arts_data->has_martialart( martial_art_learned_from( *book->type ) ) ) {
            add_msg_if_player( m_info, _( "You already know all this book has to teach." ) );
            return false;
        }

        uilist menu;
        menu.title = string_format( _( "Train %s from manual:" ),
                                    martial_art_learned_from( *book->type )->name );
        menu.addentry( 1, true, '1', _( "Train once" ) );
        menu.addentry( 2, true, '0', _( "Train until tired or success" ) );
        menu.query( true );
        if( menu.ret == UILIST_CANCEL ) {
            add_msg( m_info, _( "Never mind." ) );
            return false;
        } else if( menu.ret == 1 ) {
            continuous = false;
        } else {    // menu.ret == 2
            continuous = true;
        }
    }

    add_msg( m_info, _( "Now reading %s, %s to stop early." ),
             book->type_name(), press_x( ACTION_PAUSE ) );

    // Print some informational messages
    if( reader != this ) {
        add_msg( m_info, fail_messages[0] );
        add_msg( m_info, _( "%s reads aloud…" ), reader->disp_name() );
    } else if( !learners.empty() || !fun_learners.empty() ) {
        add_msg( m_info, _( "You read aloud…" ) );
    }

    if( learners.size() == 1 ) {
        add_msg( m_info, _( "%s studies with you." ), learners.begin()->first->disp_name() );
    } else if( !learners.empty() ) {
        const std::string them = enumerate_as_string( learners.begin(),
        learners.end(), [&]( const std::pair<npc *, std::string> &elem ) {
            return elem.first->disp_name();
        } );
        add_msg( m_info, _( "%s study with you." ), them );
    }

    // Don't include the reader as it would be too redundant.
    std::set<std::string> readers;
    for( const std::pair<npc *const, std::string> &elem : fun_learners ) {
        if( elem.first != reader ) {
            readers.insert( elem.first->disp_name() );
        }
    }

    if( readers.size() == 1 ) {
        add_msg( m_info, _( "%s reads with you for fun." ), readers.begin()->c_str() );
    } else if( !readers.empty() ) {
        const std::string them = enumerate_as_string( readers );
        add_msg( m_info, _( "%s read with you for fun." ), them );
    }

    if( std::min( fine_detail_vision_mod(), reader->fine_detail_vision_mod() ) > 1.0 ) {
        add_msg( m_warning,
                 _( "It's difficult for %s to see fine details right now.  Reading will take longer than usual." ),
                 reader->disp_name() );
    }

    const int intelligence = get_int();
    const bool complex_penalty = type->intel > std::min( intelligence, reader->get_int() ) &&
                                 !reader->has_trait( trait_PROF_DICEMASTER );
    const Character *complex_player = reader->get_int() < intelligence ? reader : this;
    if( complex_penalty ) {
        add_msg( m_warning,
                 _( "This book is too complex for %s to easily understand.  It will take longer to read." ),
                 complex_player->disp_name() );
    }

    // push an identifier of martial art book to the action handling
    if( is_martialarts &&
        get_stamina() < get_stamina_max() / 10 )  {
        add_msg( m_info, _( "You are too exhausted to train martial arts." ) );
        return false;
    }

    assign_activity(
        player_activity(
            read_activity_actor(
                time_taken,
                book,
                ereader,
                continuous,
                learner_id
            ) ) );

    return true;
}


void avatar::grab( object_type grab_type, const tripoint &grab_point )
{
    const auto update_memory =
    [this]( const object_type gtype, const tripoint & gpoint, const bool erase ) {
        map &m = get_map();
        if( gtype == object_type::VEHICLE ) {
            const optional_vpart_position vp = m.veh_at( pos() + gpoint );
            if( vp ) {
                const vehicle &veh = vp->vehicle();
                for( const tripoint &target : veh.get_points() ) {
                    if( erase ) {
                        clear_memorized_tile( m.getabs( target ) );
                    }
                    m.set_memory_seen_cache_dirty( target );
                }
            }
        } else if( gtype != object_type::NONE ) {
            if( erase ) {
                clear_memorized_tile( m.getabs( pos() + gpoint ) );
            }
            m.set_memory_seen_cache_dirty( pos() + gpoint );
        }
    };
    // Mark the area covered by the previous vehicle/furniture/etc for re-memorizing.
    update_memory( this->grab_type, this->grab_point, false );
    // Clear the map memory for the area covered by the vehicle/furniture/etc to
    // eliminate ghost vehicles/furnitures/etc.
    // FIXME: change map memory to memorize all memorizable objects and only erase vehicle part memory.
    update_memory( grab_type, grab_point, true );

    this->grab_type = grab_type;
    this->grab_point = grab_point;

    path_settings->avoid_rough_terrain = grab_type != object_type::NONE;
}

object_type avatar::get_grab_type() const
{
    return grab_type;
}

bool avatar::has_identified( const itype_id &item_id ) const
{
    return items_identified.count( item_id ) > 0;
}

void avatar::identify( const item &item )
{
    if( has_identified( item.typeId() ) ) {
        return;
    }
    if( !item.is_book() ) {
        debugmsg( "tried to identify non-book item" );
        return;
    }

    const auto &book = item; // alias
    cata_assert( !has_identified( item.typeId() ) );
    items_identified.insert( item.typeId() );
    cata_assert( has_identified( item.typeId() ) );

    const auto &reading = item.type->book;
    const skill_id &skill = reading->skill;

    add_msg( _( "You skim %s to find out what's in it." ), book.type_name() );
    if( skill && get_skill_level_object( skill ).can_train() ) {
        add_msg( m_info, _( "Can bring your %s knowledge to %d." ),
                 skill.obj().name(), reading->level );
        if( reading->req != 0 ) {
            add_msg( m_info, _( "Requires %s knowledge level %d to understand." ),
                     skill.obj().name(), reading->req );
        }
    }

    if( reading->intel != 0 ) {
        add_msg( m_info, _( "Requires intelligence of %d to easily read." ), reading->intel );
    }
    //It feels wrong to use a pointer to *this, but I can't find any other player pointers in this method.
    if( book_fun_for( book, *this ) != 0 ) {
        add_msg( m_info, _( "Reading this book affects your morale by %d." ), book_fun_for( book, *this ) );
    }

    if( book.type->use_methods.count( "MA_MANUAL" ) ) {
        const matype_id style_to_learn = martial_art_learned_from( *book.type );
        add_msg( m_info, _( "You can learn %s style from it." ), style_to_learn->name );
        add_msg( m_info, _( "This fighting style is %s to learn." ),
                 martialart_difficulty( style_to_learn ) );
        add_msg( m_info, _( "It would be easier to master if you'd have skill expertise in %s." ),
                 style_to_learn->primary_skill->name() );
        add_msg( m_info, _( "A training session with this book takes %s." ),
                 to_string( time_duration::from_minutes( reading->time ) ) );
    } else {
        add_msg( m_info, ngettext( "A chapter of this book takes %d minute to read.",
                                   "A chapter of this book takes %d minutes to read.", reading->time ),
                 reading->time );
    }

    std::vector<std::string> recipe_list;
    for( const auto &elem : reading->recipes ) {
        // Practice recipes are hidden. They're not written down in the book, they're
        // just things that the avatar can figure out with help from the book.
        if( elem.recipe->is_practice() ) {
            continue;
        }
        // If the player knows it, they recognize it even if it's not clearly stated.
        if( elem.is_hidden() && !knows_recipe( elem.recipe ) ) {
            continue;
        }
        recipe_list.push_back( elem.name() );
    }
    if( !recipe_list.empty() ) {
        std::string recipe_line =
            string_format( ngettext( "This book contains %1$zu crafting recipe: %2$s",
                                     "This book contains %1$zu crafting recipes: %2$s",
                                     recipe_list.size() ),
                           recipe_list.size(),
                           enumerate_as_string( recipe_list ) );
        add_msg( m_info, recipe_line );
    }
    if( recipe_list.size() != reading->recipes.size() ) {
        add_msg( m_info, _( "It might help you figuring out some more recipes." ) );
    }
}

void avatar::clear_identified()
{
    items_identified.clear();
}

void avatar::wake_up()
{
    if( has_effect( effect_sleep ) ) {
        if( calendar::turn - get_effect( effect_sleep ).get_start_time() > 2_hours ) {
            print_health();
        }
        // alarm was set and player hasn't slept through the alarm.
        if( has_effect( effect_alarm_clock ) && !has_effect( effect_slept_through_alarm ) ) {
            add_msg( _( "It looks like you woke up before your alarm." ) );
            remove_effect( effect_alarm_clock );
        } else if( has_effect( effect_slept_through_alarm ) ) {
            if( has_flag( json_flag_ALARMCLOCK ) ) {
                add_msg( m_warning, _( "It looks like you've slept through your internal alarm…" ) );
            } else {
                add_msg( m_warning, _( "It looks like you've slept through the alarm…" ) );
            }
        }
    }
    Character::wake_up();
}

void avatar::vomit()
{
    if( stomach.contains() != 0_ml ) {
        // Remove all joy from previously eaten food and apply the penalty
        rem_morale( MORALE_FOOD_GOOD );
        rem_morale( MORALE_FOOD_HOT );
        // bears must suffer too
        rem_morale( MORALE_HONEY );
        // 1.5 times longer
        add_morale( MORALE_VOMITED, -2 * units::to_milliliter( stomach.contains() / 50 ), -40, 90_minutes,
                    45_minutes, false );

    } else {
        add_msg( m_warning, _( "You retched, but your stomach is empty." ) );
    }
    Character::vomit();
}

nc_color avatar::basic_symbol_color() const
{
    if( has_effect( effect_onfire ) ) {
        return c_red;
    }
    if( has_effect( effect_stunned ) ) {
        return c_light_blue;
    }
    if( has_effect( effect_boomered ) ) {
        return c_pink;
    }
    if( has_active_mutation( trait_id( "SHELL2" ) ) ) {
        return c_magenta;
    }
    if( underwater ) {
        return c_blue;
    }
    if( has_active_bionic( bio_cloak ) ||
        is_wearing_active_optcloak() || has_trait( trait_DEBUG_CLOAK ) ) {
        return c_dark_gray;
    }
    return move_mode->symbol_color();
}

int avatar::print_info( const catacurses::window &w, int vStart, int, int column ) const
{
    return vStart + fold_and_print( w, point( column, vStart ), getmaxx( w ) - column - 1, c_dark_gray,
                                    _( "You (%s)" ),
                                    get_name() ) - 1;
}

void avatar::disp_morale()
{
    int equilibrium = calc_focus_equilibrium();

    int fatigue_penalty = 0;
    if( get_fatigue() >= fatigue_levels::MASSIVE_FATIGUE && equilibrium > 20 ) {
        fatigue_penalty = equilibrium - 20;
        equilibrium = 20;
    } else if( get_fatigue() >= fatigue_levels::EXHAUSTED && equilibrium > 40 ) {
        fatigue_penalty = equilibrium - 40;
        equilibrium = 40;
    } else if( get_fatigue() >= fatigue_levels::DEAD_TIRED && equilibrium > 60 ) {
        fatigue_penalty = equilibrium - 60;
        equilibrium = 60;
    } else if( get_fatigue() >= fatigue_levels::TIRED && equilibrium > 80 ) {
        fatigue_penalty = equilibrium - 80;
        equilibrium = 80;
    }

    int pain_penalty = 0;
    if( get_perceived_pain() && !has_trait( trait_CENOBITE ) ) {
        pain_penalty = calc_focus_equilibrium( true ) - equilibrium - fatigue_penalty;
    }

    morale->display( equilibrium, pain_penalty, fatigue_penalty );
}

int avatar::calc_focus_equilibrium( bool ignore_pain ) const
{
    int focus_equilibrium = 100;

    if( activity.id() == ACT_READ ) {
        const item_location book = activity.targets[0];
        if( book && book->is_book() ) {
            const cata::value_ptr<islot_book> &bt = book->type->book;
            // apply a penalty when we're actually learning something
            const SkillLevel &skill_level = get_skill_level_object( bt->skill );
            if( skill_level.can_train() && skill_level < bt->level ) {
                focus_equilibrium -= 50;
            }
        }
    }

    int eff_morale = get_morale_level();
    // Factor in perceived pain, since it's harder to rest your mind while your body hurts.
    // Cenobites don't mind, though
    if( !ignore_pain && !has_trait( trait_CENOBITE ) ) {
        int perceived_pain = get_perceived_pain();
        if( has_trait( trait_MASOCHIST ) ) {
            if( perceived_pain > 20 ) {
                eff_morale = eff_morale - ( perceived_pain - 20 );
            }
        } else {
            eff_morale = eff_morale - perceived_pain;
        }
    }

    if( eff_morale < -99 ) {
        // At very low morale, focus is at it's minimum
        focus_equilibrium = 1;
    } else if( eff_morale <= 50 ) {
        // At -99 to +50 morale, each point of morale gives or takes 1 point of focus
        focus_equilibrium += eff_morale;
    } else {
        /* Above 50 morale, we apply strong diminishing returns.
        * Each block of 50 takes twice as many morale points as the previous one:
        * 150 focus at 50 morale (as before)
        * 200 focus at 150 morale (100 more morale)
        * 250 focus at 350 morale (200 more morale)
        * ...
        * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
        */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while( focus_equilibrium < 400 ) {
            if( morale_left > 50 * block_multiplier ) {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_equilibrium += 50;
                block_multiplier *= 2;
            } else {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1 focus, and then we're done.
                focus_equilibrium += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if( focus_equilibrium < 1 ) {
        focus_equilibrium = 1;
    } else if( focus_equilibrium > 400 ) {
        focus_equilibrium = 400;
    }
    return focus_equilibrium;
}

int avatar::calc_focus_change() const
{
    int focus_gap = calc_focus_equilibrium() - get_focus();

    // handle negative gain rates in a symmetric manner
    int base_change = 1;
    if( focus_gap < 0 ) {
        base_change = -1;
        focus_gap = -focus_gap;
    }

    int gain = focus_gap * base_change;

    // Fatigue will incrementally decrease any focus above related cap
    if( ( get_fatigue() >= fatigue_levels::TIRED && get_focus() > 80 ) ||
        ( get_fatigue() >= fatigue_levels::DEAD_TIRED && get_focus() > 60 ) ||
        ( get_fatigue() >= fatigue_levels::EXHAUSTED && get_focus() > 40 ) ||
        ( get_fatigue() >= fatigue_levels::MASSIVE_FATIGUE && get_focus() > 20 ) ) {

        //it can fall faster then 1
        if( gain > -1 ) {
            gain = -1;
        }
    }
    return gain;
}

void avatar::update_mental_focus()
{
    // calc_focus_change() returns percentile focus, applying it directly
    // to focus pool is an implicit / 100.
    focus_pool += 10 * calc_focus_change();
}

int avatar::limb_dodge_encumbrance() const
{
    float leg_encumbrance = 0.0f;
    float torso_encumbrance = 0.0f;
    const std::vector<bodypart_id> legs =
        get_all_body_parts_of_type( body_part_type::type::leg );
    const std::vector<bodypart_id> torsos =
        get_all_body_parts_of_type( body_part_type::type::torso );

    for( const bodypart_id &leg : legs ) {
        leg_encumbrance += encumb( leg );
    }
    if( !legs.empty() ) {
        leg_encumbrance /= legs.size() * 10.0f;
    }

    for( const bodypart_id &torso : torsos ) {
        torso_encumbrance += encumb( torso );
    }
    if( !torsos.empty() ) {
        torso_encumbrance /= torsos.size() * 10.0f;
    }

    return std::floor( torso_encumbrance + leg_encumbrance );
}

void avatar::reset_stats()
{
    const int current_stim = get_stim();

    // Trait / mutation buffs
    if( has_trait( trait_THICK_SCALES ) ) {
        add_miss_reason( _( "Your thick scales get in the way." ), 2 );
    }
    if( has_trait( trait_CHITIN2 ) || has_trait( trait_CHITIN3 ) || has_trait( trait_CHITIN_FUR3 ) ) {
        add_miss_reason( _( "Your chitin gets in the way." ), 1 );
    }
    if( has_trait( trait_COMPOUND_EYES ) && !wearing_something_on( bodypart_id( "eyes" ) ) ) {
        mod_per_bonus( 2 );
    }
    if( has_trait( trait_INSECT_ARMS ) ) {
        add_miss_reason( _( "Your insect limbs get in the way." ), 2 );
    }
    if( has_trait( trait_INSECT_ARMS_OK ) ) {
        if( !wearing_something_on( bodypart_id( "torso" ) ) ) {
            mod_dex_bonus( 1 );
        } else {
            mod_dex_bonus( -1 );
            add_miss_reason( _( "Your clothing restricts your insect arms." ), 1 );
        }
    }
    if( has_trait( trait_WEBBED ) ) {
        add_miss_reason( _( "Your webbed hands get in the way." ), 1 );
    }
    if( has_trait( trait_ARACHNID_ARMS ) ) {
        add_miss_reason( _( "Your arachnid limbs get in the way." ), 4 );
    }
    if( has_trait( trait_ARACHNID_ARMS_OK ) ) {
        if( !wearing_something_on( bodypart_id( "torso" ) ) ) {
            mod_dex_bonus( 2 );
        } else if( !exclusive_flag_coverage( STATIC( flag_id( "OVERSIZE" ) ) )
                   .test( body_part_torso ) ) {
            mod_dex_bonus( -2 );
            add_miss_reason( _( "Your clothing constricts your arachnid limbs." ), 2 );
        }
    }
    const auto set_fake_effect_dur = [this]( const efftype_id & type, const time_duration & dur ) {
        effect &eff = get_effect( type );
        if( eff.get_duration() == dur ) {
            return;
        }

        if( eff.is_null() && dur > 0_turns ) {
            add_effect( type, dur, true );
        } else if( dur > 0_turns ) {
            eff.set_duration( dur );
        } else {
            remove_effect( type );
        }
    };
    // Painkiller
    set_fake_effect_dur( effect_pkill, 1_turns * get_painkiller() );

    // Pain
    if( get_perceived_pain() > 0 ) {
        const stat_mod ppen = get_pain_penalty();
        mod_str_bonus( -ppen.strength );
        mod_dex_bonus( -ppen.dexterity );
        mod_int_bonus( -ppen.intelligence );
        mod_per_bonus( -ppen.perception );
        if( ppen.dexterity > 0 ) {
            add_miss_reason( _( "Your pain distracts you!" ), static_cast<unsigned>( ppen.dexterity ) );
        }
    }

    // Radiation
    set_fake_effect_dur( effect_irradiated, 1_turns * get_rad() );
    // Morale
    const int morale = get_morale_level();
    set_fake_effect_dur( effect_happy, 1_turns * morale );
    set_fake_effect_dur( effect_sad, 1_turns * -morale );

    // Stimulants
    set_fake_effect_dur( effect_stim, 1_turns * current_stim );
    set_fake_effect_dur( effect_depressants, 1_turns * -current_stim );
    if( has_trait( trait_STIMBOOST ) ) {
        set_fake_effect_dur( effect_stim_overdose, 1_turns * ( current_stim - 60 ) );
    } else {
        set_fake_effect_dur( effect_stim_overdose, 1_turns * ( current_stim - 30 ) );
    }
    // Starvation
    const float bmi = get_bmi();
    if( bmi < character_weight_category::underweight ) {
        const int str_penalty = std::floor( ( 1.0f - ( bmi - 13.0f ) / 3.0f ) * get_str_base() );
        add_miss_reason( _( "You're weak from hunger." ),
                         static_cast<unsigned>( ( get_starvation() + 300 ) / 1000 ) );
        mod_str_bonus( -str_penalty );
        mod_dex_bonus( -( str_penalty / 2 ) );
        mod_int_bonus( -( str_penalty / 2 ) );
    }
    // Thirst
    if( get_thirst() >= 200 ) {
        // We die at 1200
        const int dex_mod = -get_thirst() / 200;
        add_miss_reason( _( "You're weak from thirst." ), static_cast<unsigned>( -dex_mod ) );
        mod_str_bonus( -get_thirst() / 200 );
        mod_dex_bonus( dex_mod );
        mod_int_bonus( -get_thirst() / 200 );
        mod_per_bonus( -get_thirst() / 200 );
    }
    if( get_sleep_deprivation() >= SLEEP_DEPRIVATION_HARMLESS ) {
        set_fake_effect_dur( effect_sleep_deprived, 1_turns * get_sleep_deprivation() );
    } else if( has_effect( effect_sleep_deprived ) ) {
        remove_effect( effect_sleep_deprived );
    }

    // Dodge-related effects
    mod_dodge_bonus( mabuff_dodge_bonus() - limb_dodge_encumbrance() );
    // Whiskers don't work so well if they're covered
    if( has_trait( trait_WHISKERS ) && !natural_attack_restricted_on( bodypart_id( "mouth" ) ) ) {
        mod_dodge_bonus( 1 );
    }
    if( has_trait( trait_WHISKERS_RAT ) && !natural_attack_restricted_on( bodypart_id( "mouth" ) ) ) {
        mod_dodge_bonus( 2 );
    }
    // depending on mounts size, attacks will hit the mount and use their dodge rating.
    // if they hit the player, the player cannot dodge as effectively.
    if( is_mounted() ) {
        mod_dodge_bonus( -4 );
    }
    // Spider hair is basically a full-body set of whiskers, once you get the brain for it
    if( has_trait( trait_CHITIN_FUR3 ) ) {
        static const bodypart_str_id parts[] {
            body_part_head, body_part_arm_r, body_part_arm_l,
            body_part_leg_r, body_part_leg_l
        };
        for( const bodypart_str_id &bp : parts ) {
            if( !wearing_something_on( bp ) ) {
                mod_dodge_bonus( +1 );
            }
        }
        // Torso handled separately, bigger bonus
        if( !wearing_something_on( bodypart_id( "torso" ) ) ) {
            mod_dodge_bonus( 4 );
        }
    }

    // Apply static martial arts buffs
    martial_arts_data->ma_static_effects( *this );

    if( calendar::once_every( 1_minutes ) ) {
        update_mental_focus();
    }

    // Effects
    for( const auto &maps : *effects ) {
        for( const auto &i : maps.second ) {
            const auto &it = i.second;
            bool reduced = resists_effect( it );
            mod_str_bonus( it.get_mod( "STR", reduced ) );
            mod_dex_bonus( it.get_mod( "DEX", reduced ) );
            mod_per_bonus( it.get_mod( "PER", reduced ) );
            mod_int_bonus( it.get_mod( "INT", reduced ) );
        }
    }

    Character::reset_stats();

    recalc_sight_limits();
    recalc_speed_bonus();

}

int avatar::get_str_base() const
{
    return Character::get_str_base() + std::max( 0, str_upgrade );
}

int avatar::get_dex_base() const
{
    return Character::get_dex_base() + std::max( 0, dex_upgrade );
}

int avatar::get_int_base() const
{
    return Character::get_int_base() + std::max( 0, int_upgrade );
}

int avatar::get_per_base() const
{
    return Character::get_per_base() + std::max( 0, per_upgrade );
}

int avatar::kill_xp() const
{
    return g->get_kill_tracker().kill_xp();
}

// based on  D&D 5e level progression
static const std::array<int, 20> xp_cutoffs = { {
        300, 900, 2700, 6500, 14000,
        23000, 34000, 48000, 64000, 85000,
        100000, 120000, 140000, 165000, 195000,
        225000, 265000, 305000, 355000, 405000
    }
};

int avatar::free_upgrade_points() const
{
    const int xp = kill_xp();
    int lvl = 0;
    for( const int &xp_lvl : xp_cutoffs ) {
        if( xp >= xp_lvl ) {
            lvl++;
        } else {
            break;
        }
    }
    return lvl - str_upgrade - dex_upgrade - int_upgrade - per_upgrade;
}

void avatar::upgrade_stat_prompt( const character_stat &stat )
{
    const int free_points = free_upgrade_points();

    if( free_points <= 0 ) {
        std::array<int, 20>::const_iterator xp_next_level = std::lower_bound( xp_cutoffs.begin(),
                xp_cutoffs.end(), kill_xp() );
        if( xp_next_level == xp_cutoffs.end() ) {
            popup( _( "You've already reached maximum level." ) );
        } else {
            popup( _( "Needs %d more experience to gain next level." ),
                   *xp_next_level - kill_xp() );
        }
        return;
    }

    std::string stat_string;
    switch( stat ) {
        case character_stat::STRENGTH:
            stat_string = _( "strength" );
            break;
        case character_stat::DEXTERITY:
            stat_string = _( "dexterity" );
            break;
        case character_stat::INTELLIGENCE:
            stat_string = _( "intelligence" );
            break;
        case character_stat::PERCEPTION:
            stat_string = _( "perception" );
            break;
        case character_stat::DUMMY_STAT:
            stat_string = _( "invalid stat" );
            debugmsg( "Tried to use invalid stat" );
            break;
        default:
            return;
    }

    if( query_yn( _( "Are you sure you want to raise %s?  %d points available." ), stat_string,
                  free_points ) ) {
        switch( stat ) {
            case character_stat::STRENGTH:
                str_upgrade++;
                break;
            case character_stat::DEXTERITY:
                dex_upgrade++;
                break;
            case character_stat::INTELLIGENCE:
                int_upgrade++;
                break;
            case character_stat::PERCEPTION:
                per_upgrade++;
                break;
            case character_stat::DUMMY_STAT:
                debugmsg( "Tried to use invalid stat" );
                break;
        }
    }
    recalc_hp();
}

faction *avatar::get_faction() const
{
    return g->faction_manager_ptr->get( faction_id( "your_followers" ) );
}

void avatar::set_movement_mode( const move_mode_id &new_mode )
{
    if( can_switch_to( new_mode ) ) {
        if( is_hauling() && new_mode->stop_hauling() ) {
            stop_hauling();
        }
        add_msg( new_mode->change_message( true, get_steed_type() ) );
        move_mode = new_mode;
        // crouching affects visibility
        get_map().set_seen_cache_dirty( pos().z );
    } else {
        add_msg( new_mode->change_message( false, get_steed_type() ) );
    }
}

void avatar::toggle_run_mode()
{
    if( is_running() ) {
        set_movement_mode( move_mode_id( "walk" ) );
    } else {
        set_movement_mode( move_mode_id( "run" ) );
    }
}

void avatar::toggle_crouch_mode()
{
    if( is_crouching() ) {
        set_movement_mode( move_mode_id( "walk" ) );
    } else {
        set_movement_mode( move_mode_id( "crouch" ) );
    }
}

void avatar::toggle_prone_mode()
{
    if( is_prone() ) {
        set_movement_mode( move_mode_id( "walk" ) );
    } else {
        set_movement_mode( move_mode_id( "prone" ) );
    }
}
void avatar::activate_crouch_mode()
{
    if( !is_crouching() ) {
        set_movement_mode( move_mode_id( "crouch" ) );
    }
}

void avatar::reset_move_mode()
{
    if( !is_walking() ) {
        set_movement_mode( move_mode_id( "walk" ) );
    }
}

void avatar::cycle_move_mode()
{
    const move_mode_id next = current_movement_mode()->cycle();
    set_movement_mode( next );
    // if a movemode is disabled then just cycle to the next one
    if( !movement_mode_is( next ) ) {
        set_movement_mode( next->cycle() );
    }
}

bool avatar::wield( item_location target )
{
    return wield( *target, target.obtain_cost( *this ) );
}

bool avatar::wield( item &target )
{
    invalidate_inventory_validity_cache();
    return wield( target,
                  item_handling_cost( target, true,
                                      is_worn( target ) ? INVENTORY_HANDLING_PENALTY / 2 :
                                      INVENTORY_HANDLING_PENALTY ) );
}

bool avatar::wield( item &target, const int obtain_cost )
{
    if( is_wielding( target ) ) {
        return true;
    }

    if( weapon.has_item( target ) ) {
        add_msg( m_info, _( "You need to put the bag away before trying to wield something from it." ) );
        return false;
    }

    if( !can_wield( target ).success() ) {
        return false;
    }

    bool combine_stacks = target.can_combine( weapon );
    if( !combine_stacks && !unwield() ) {
        return false;
    }
    cached_info.erase( "weapon_value" );
    if( target.is_null() ) {
        return true;
    }

    // Wielding from inventory is relatively slow and does not improve with increasing weapon skill.
    // Worn items (including guns with shoulder straps) are faster but still slower
    // than a skilled player with a holster.
    // There is an additional penalty when wielding items from the inventory whilst currently grabbed.

    bool worn = is_worn( target );
    const int mv = obtain_cost;

    if( worn ) {
        target.on_takeoff( *this );
    }

    add_msg_debug( debugmode::DF_AVATAR, "wielding took %d moves", mv );
    moves -= mv;

    if( has_item( target ) ) {
        item removed = i_rem( &target );
        if( combine_stacks ) {
            weapon.combine( removed );
        } else {
            weapon = removed;
        }
    } else {
        if( combine_stacks ) {
            weapon.combine( target );
        } else {
            weapon = target;
        }
    }

    last_item = weapon.typeId();
    recoil = MAX_RECOIL;

    weapon.on_wield( *this );

    get_event_bus().send<event_type::character_wields_item>( getID(), last_item );

    inv->update_invlet( weapon );
    inv->update_cache_with_item( weapon );

    return true;
}

bool avatar::invoke_item( item *used, const tripoint &pt, int pre_obtain_moves )
{
    const std::map<std::string, use_function> &use_methods = used->type->use_methods;
    const int num_methods = use_methods.size();

    const bool has_relic = used->has_relic_activation();
    if( use_methods.empty() && !has_relic ) {
        return false;
    } else if( num_methods == 1 && !has_relic ) {
        return invoke_item( used, use_methods.begin()->first, pt, pre_obtain_moves );
    } else if( num_methods == 0 && has_relic ) {
        return used->use_relic( *this, pt );
    }

    uilist umenu;

    umenu.text = string_format( _( "What to do with your %s?" ), used->tname() );
    umenu.hilight_disabled = true;

    for( const auto &e : use_methods ) {
        const auto res = e.second.can_call( *this, *used, false, pt );
        umenu.addentry_desc( MENU_AUTOASSIGN, res.success(), MENU_AUTOASSIGN, e.second.get_name(),
                             res.str() );
    }
    if( has_relic ) {
        umenu.addentry_desc( MENU_AUTOASSIGN, true, MENU_AUTOASSIGN, _( "Use relic" ),
                             _( "Activate this relic." ) );
    }

    umenu.desc_enabled = std::any_of( umenu.entries.begin(),
    umenu.entries.end(), []( const uilist_entry & elem ) {
        return !elem.desc.empty();
    } );

    umenu.query();

    int choice = umenu.ret;
    // Use the relic
    if( choice == num_methods ) {
        return used->use_relic( *this, pt );
    }
    if( choice < 0 || choice >= num_methods ) {
        return false;
    }

    const std::string &method = std::next( use_methods.begin(), choice )->first;

    return invoke_item( used, method, pt, pre_obtain_moves );
}

bool avatar::invoke_item( item *used )
{
    return Character::invoke_item( used );
}

bool avatar::invoke_item( item *used, const std::string &method, const tripoint &pt,
                          int pre_obtain_moves )
{
    if( pre_obtain_moves == -1 ) {
        pre_obtain_moves = moves;
    }
    return Character::invoke_item( used, method, pt, pre_obtain_moves );
}

bool avatar::invoke_item( item *used, const std::string &method )
{
    return Character::invoke_item( used, method );
}

void avatar::advance_daily_calories()
{
    calorie_diary.push_front( daily_calories{} );
    if( calorie_diary.size() > 30 ) {
        calorie_diary.pop_back();
    }
}

void avatar::add_spent_calories( int cal )
{
    calorie_diary.front().spent += cal;
}

void avatar::add_gained_calories( int cal )
{
    calorie_diary.front().gained += cal;
}

void avatar::log_activity_level( float level )
{
    calorie_diary.front().activity_levels[level]++;
}

void avatar::daily_calories::save_activity( JsonOut &json ) const
{
    json.member( "activity" );
    json.start_array();
    for( const std::pair<const float, int> &level : activity_levels ) {
        if( level.second > 0 ) {
            json.start_array();
            json.write( level.first );
            json.write( level.second );
            json.end_array();
        }
    }
    json.end_array();
}

void avatar::daily_calories::read_activity( const JsonObject &data )
{
    if( data.has_array( "activity" ) ) {
        double act_level;
        for( JsonArray ja : data.get_array( "activity" ) ) {
            act_level = ja.next_float();
            activity_levels[ act_level ] = ja.next_int();
        }
        return;
    }
    // Fallback to legacy format for backward compatibility
    JsonObject jo = data.get_object( "activity" );
    for( const std::pair<const std::string, float> &member : activity_levels_map ) {
        int times;
        jo.read( member.first, times );
        activity_levels.at( member.second ) = times;
    }
}

std::string avatar::total_daily_calories_string() const
{
    const std::string header_string =
        colorize( "       Minutes at each exercise level            Calories per day", c_white ) + "\n" +
        colorize( "  Day  None Light Moderate Brisk Active Extra    Gained  Spent  Total",
                  c_yellow ) + "\n";
    const std::string format_string =
        " %4d  %4d  %4d     %4d  %4d   %4d  %4d    %6d %6d";

    const float light_ex_thresh = ( NO_EXERCISE + LIGHT_EXERCISE ) / 2.0f;
    const float mod_ex_thresh = ( LIGHT_EXERCISE + MODERATE_EXERCISE ) / 2.0f;
    const float brisk_ex_thresh = ( MODERATE_EXERCISE + BRISK_EXERCISE ) / 2.0f;
    const float active_ex_thresh = ( BRISK_EXERCISE + ACTIVE_EXERCISE ) / 2.0f;
    const float extra_ex_thresh = ( ACTIVE_EXERCISE + EXTRA_EXERCISE ) / 2.0f;

    std::string ret = header_string;

    // Start with today in the first row, day number from start of cataclysm
    int today = day_of_season<int>( calendar::turn ) + 1;
    int day_offset = 0;
    for( const daily_calories &day : calorie_diary ) {
        // Yes, this is clunky.
        // Perhaps it should be done in log_activity_level? But that's called a lot more often.
        int no_exercise = 0;
        int light_exercise = 0;
        int moderate_exercise = 0;
        int brisk_exercise = 0;
        int active_exercise = 0;
        int extra_exercise = 0;
        for( const std::pair<const float, int> &level : day.activity_levels ) {
            if( level.second > 0 ) {
                if( level.first < light_ex_thresh ) {
                    no_exercise += level.second;
                } else if( level.first < mod_ex_thresh ) {
                    light_exercise += level.second;
                } else if( level.first < brisk_ex_thresh ) {
                    moderate_exercise += level.second;
                } else if( level.first < active_ex_thresh ) {
                    brisk_exercise += level.second;
                } else if( level.first < extra_ex_thresh ) {
                    active_exercise += level.second;
                } else {
                    extra_exercise += level.second;
                }
            }
        }
        std::string row_data = string_format( format_string, today + day_offset--,
                                              5 * no_exercise,
                                              5 * light_exercise,
                                              5 * moderate_exercise,
                                              5 * brisk_exercise,
                                              5 * active_exercise,
                                              5 * extra_exercise,
                                              day.gained, day.spent );
        // Alternate gray and white text for row data
        if( day_offset % 2 == 0 ) {
            ret += colorize( row_data, c_white );
        } else {
            ret += colorize( row_data, c_light_gray );
        }

        // Color-code each day's net calories
        std::string total_kcals = string_format( " %6d", day.total() );
        if( day.total() > 4000 ) {
            ret += colorize( total_kcals, c_light_cyan );
        } else if( day.total() > 2000 ) {
            ret += colorize( total_kcals, c_cyan );
        } else if( day.total() > 250 ) {
            ret += colorize( total_kcals, c_light_blue );
        } else if( day.total() < -4000 ) {
            ret += colorize( total_kcals, c_pink );
        } else if( day.total() < -2000 ) {
            ret += colorize( total_kcals, c_red );
        } else if( day.total() < -250 ) {
            ret += colorize( total_kcals, c_light_red );
        } else {
            ret += colorize( total_kcals, c_light_gray );
        }
        ret += "\n";
    }
    return ret;
}

std::unique_ptr<talker> get_talker_for( avatar &me )
{
    return std::make_unique<talker_avatar>( &me );
}
std::unique_ptr<talker> get_talker_for( avatar *me )
{
    return std::make_unique<talker_avatar>( me );
}

void avatar::randomize_hobbies()
{
    hobbies.clear();
    std::vector<profession_id> choices = profession::get_all_hobbies();

    int random = rng( 0, 5 );

    if( random >= 1 ) {
        add_random_hobby( choices );
    }
    if( random >= 3 ) {
        add_random_hobby( choices );
    }
    if( random >= 5 ) {
        add_random_hobby( choices );
    }
}

void avatar::add_random_hobby( std::vector<profession_id> &choices )
{
    const profession_id hobby = random_entry_removed( choices );
    hobbies.insert( &*hobby );

    // Add or remove traits from hobby
    for( const trait_id &trait : hobby->get_locked_traits() ) {
        toggle_trait( trait );
    }
}

void avatar::reassign_item( item &it, int invlet )
{
    bool remove_old = true;
    if( invlet ) {
        item *prev = invlet_to_item( invlet );
        if( prev != nullptr ) {
            remove_old = it.typeId() != prev->typeId();
            inv->reassign_item( *prev, it.invlet, remove_old );
        }
    }

    if( !invlet || inv_chars.valid( invlet ) ) {
        const auto iter = inv->assigned_invlet.find( it.invlet );
        bool found = iter != inv->assigned_invlet.end();
        if( found ) {
            inv->assigned_invlet.erase( iter );
        }
        if( invlet && ( !found || it.invlet != invlet ) ) {
            inv->assigned_invlet[invlet] = it.typeId();
        }
        inv->reassign_item( it, invlet, remove_old );
    }
}

void avatar::add_pain_msg( int val, const bodypart_id &bp ) const
{
    if( has_trait( trait_NOPAIN ) ) {
        return;
    }
    if( bp == bodypart_id( "bp_null" ) ) {
        if( val > 20 ) {
            add_msg_if_player( _( "Your body is wracked with excruciating pain!" ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your body is wracked with terrible pain!" ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your body is wracked with pain!" ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your body pains you!" ) );
        } else {
            add_msg_if_player( _( "Your body aches." ) );
        }
    } else {
        if( val > 20 ) {
            add_msg_if_player( _( "Your %s is wracked with excruciating pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your %s is wracked with terrible pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your %s is wracked with pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your %s pains you!" ),
                               body_part_name_accusative( bp ) );
        } else {
            add_msg_if_player( _( "Your %s aches." ),
                               body_part_name_accusative( bp ) );
        }
    }
}

// ids of martial art styles that are available with the bio_cqb bionic.
static const std::vector<matype_id> bio_cqb_styles{ {
        matype_id{ "style_aikido" },
        matype_id{ "style_biojutsu" },
        matype_id{ "style_boxing" },
        matype_id{ "style_capoeira" },
        matype_id{ "style_crane" },
        matype_id{ "style_dragon" },
        matype_id{ "style_judo" },
        matype_id{ "style_karate" },
        matype_id{ "style_krav_maga" },
        matype_id{ "style_leopard" },
        matype_id{ "style_muay_thai" },
        matype_id{ "style_ninjutsu" },
        matype_id{ "style_pankration" },
        matype_id{ "style_snake" },
        matype_id{ "style_taekwondo" },
        matype_id{ "style_tai_chi" },
        matype_id{ "style_tiger" },
        matype_id{ "style_wingchun" },
        matype_id{ "style_zui_quan" }
    }};

bool character_martial_arts::pick_style( const avatar &you ) // Style selection menu
{
    enum style_selection {
        KEEP_HANDS_FREE = 0,
        STYLE_OFFSET
    };

    // If there are style already, cursor starts there
    // if no selected styles, cursor starts from no-style

    // Any other keys quit the menu
    const std::vector<matype_id> &selectable_styles = you.has_active_bionic(
                bio_cqb ) ? bio_cqb_styles :
            ma_styles;

    input_context ctxt( "MELEE_STYLE_PICKER", keyboard_mode::keycode );
    ctxt.register_action( "SHOW_DESCRIPTION" );

    uilist kmenu;
    kmenu.text = string_format( _( "Select a style.\n"
                                   "\n"
                                   "STR: <color_white>%d</color>, DEX: <color_white>%d</color>, "
                                   "PER: <color_white>%d</color>, INT: <color_white>%d</color>\n"
                                   "Press [<color_yellow>%s</color>] for more info.\n" ),
                                you.get_str(), you.get_dex(), you.get_per(), you.get_int(),
                                ctxt.get_desc( "SHOW_DESCRIPTION" ) );
    ma_style_callback callback( static_cast<size_t>( STYLE_OFFSET ), selectable_styles );
    kmenu.callback = &callback;
    kmenu.input_category = "MELEE_STYLE_PICKER";
    kmenu.additional_actions.emplace_back( "SHOW_DESCRIPTION", translation() );
    kmenu.desc_enabled = true;
    kmenu.addentry_desc( KEEP_HANDS_FREE, true, 'h',
                         keep_hands_free ? _( "Keep hands free (on)" ) : _( "Keep hands free (off)" ),
                         _( "When this is enabled, player won't wield things unless explicitly told to." ) );

    kmenu.selected = STYLE_OFFSET;

    for( size_t i = 0; i < selectable_styles.size(); i++ ) {
        const auto &style = selectable_styles[i].obj();
        //Check if this style is currently selected
        const bool selected = selectable_styles[i] == style_selected;
        std::string entry_text = style.name.translated();
        if( selected ) {
            kmenu.selected = i + STYLE_OFFSET;
            entry_text = colorize( entry_text, c_pink );
        }
        kmenu.addentry_desc( i + STYLE_OFFSET, true, -1, entry_text, style.description.translated() );
    }

    kmenu.query();
    int selection = kmenu.ret;

    if( selection >= STYLE_OFFSET ) {
        style_selected = selectable_styles[selection - STYLE_OFFSET];
        martialart_use_message( you );
    } else if( selection == KEEP_HANDS_FREE ) {
        keep_hands_free = !keep_hands_free;
    } else {
        return false;
    }

    return true;
}

bool avatar::wield_contents( item &container, item *internal_item, bool penalties, int base_cost )
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( internal_item == nullptr ) {
        std::vector<std::string> opts;
        std::list<item *> container_contents = container.all_items_top();
        std::transform( container_contents.begin(), container_contents.end(),
        std::back_inserter( opts ), []( const item * elem ) {
            return elem->display_name();
        } );
        if( opts.size() > 1 ) {
            int pos = uilist( _( "Wield what?" ), opts );
            if( pos < 0 ) {
                return false;
            }
            internal_item = *std::next( container_contents.begin(), pos );
        } else {
            internal_item = container_contents.front();
        }
    }

    return Character::wield_contents( container, internal_item, penalties, base_cost );
}

void avatar::try_to_sleep( const time_duration &dur )
{
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( pos() );
    const trap &trap_at_pos = here.tr_at( pos() );
    const ter_id ter_at_pos = here.ter( pos() );
    const furn_id furn_at_pos = here.furn( pos() );
    bool plantsleep = false;
    bool fungaloid_cosplay = false;
    bool websleep = false;
    bool webforce = false;
    bool websleeping = false;
    bool in_shell = false;
    bool watersleep = false;
    if( has_trait( trait_CHLOROMORPH ) ) {
        plantsleep = true;
        if( ( ter_at_pos == t_dirt || ter_at_pos == t_pit ||
              ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow ||
              ter_at_pos == t_grass ) && !vp &&
            furn_at_pos == f_null ) {
            add_msg_if_player( m_good, _( "You relax as your roots embrace the soil." ) );
        } else if( vp ) {
            add_msg_if_player( m_bad, _( "It's impossible to sleep in this wheeled pot!" ) );
        } else if( furn_at_pos != f_null ) {
            add_msg_if_player( m_bad,
                               _( "The humans' furniture blocks your roots.  You can't get comfortable." ) );
        } else { // Floor problems
            add_msg_if_player( m_bad, _( "Your roots scrabble ineffectively at the unyielding surface." ) );
        }
    } else if( has_trait( trait_M_SKIN3 ) ) {
        fungaloid_cosplay = true;
        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos() ) ) {
            add_msg_if_player( m_good,
                               _( "Our fibers meld with the ground beneath us.  The gills on our neck begin to seed the air with spores as our awareness fades." ) );
        }
    }
    if( has_trait( trait_WEB_WALKER ) ) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if( has_trait( trait_THRESH_SPIDER ) && ( has_trait( trait_WEB_SPINNER ) ||
            ( has_trait( trait_WEB_WEAVER ) ) ) ) {
        webforce = true;
    }
    if( websleep || webforce ) {
        int web = here.get_field_intensity( pos(), fd_web );
        if( !webforce ) {
            // At this point, it's kinda weird, but surprisingly comfy...
            if( web >= 3 ) {
                add_msg_if_player( m_good,
                                   _( "These thick webs support your weight, and are strangely comfortable…" ) );
                websleeping = true;
            } else if( web > 0 ) {
                add_msg_if_player( m_info,
                                   _( "You try to sleep, but the webs get in the way.  You brush them aside." ) );
                here.remove_field( pos(), fd_web );
            }
        } else {
            // Here, you're just not comfortable outside a nice thick web.
            if( web >= 3 ) {
                add_msg_if_player( m_good, _( "You relax into your web." ) );
                websleeping = true;
            } else {
                add_msg_if_player( m_bad,
                                   _( "You try to sleep, but you feel exposed and your spinnerets keep twitching." ) );
                add_msg_if_player( m_info, _( "Maybe a nice thick web would help you sleep." ) );
            }
        }
    }
    if( has_active_mutation( trait_SHELL2 ) ) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    if( has_trait( trait_WATERSLEEP ) ) {
        if( underwater ) {
            add_msg_if_player( m_good,
                               _( "You lay beneath the waves' embrace, gazing up through the water's surface…" ) );
            watersleep = true;
        } else if( here.has_flag_ter( ter_furn_flag::TFLAG_SWIMMABLE, pos() ) ) {
            add_msg_if_player( m_good, _( "You settle into the water and begin to drowse…" ) );
            watersleep = true;
        }
    }
    if( !plantsleep && ( furn_at_pos.obj().comfort > static_cast<int>( comfort_level::neutral ) ||
                         ter_at_pos == t_improvised_shelter ||
                         trap_at_pos.comfort > static_cast<int>( comfort_level::neutral ) ||
                         in_shell || websleeping || watersleep ||
                         vp.part_with_feature( "SEAT", true ) ||
                         vp.part_with_feature( "BED", true ) ) ) {
        add_msg_if_player( m_good, _( "This is a comfortable place to sleep." ) );
    } else if( !plantsleep && !fungaloid_cosplay && !watersleep ) {
        if( !vp && ter_at_pos != t_floor ) {
            add_msg_if_player( ter_at_pos.obj().movecost <= 2 ?
                               _( "It's a little hard to get to sleep on this %s." ) :
                               _( "It's hard to get to sleep on this %s." ),
                               ter_at_pos.obj().name() );
        } else if( vp ) {
            if( vp->part_with_feature( VPFLAG_AISLE, true ) ) {
                add_msg_if_player(
                    //~ %1$s: vehicle name, %2$s: vehicle part name
                    _( "It's a little hard to get to sleep on this %2$s in %1$s." ),
                    vp->vehicle().disp_name(),
                    vp->part_with_feature( VPFLAG_AISLE, true )->part().name( false ) );
            } else {
                add_msg_if_player(
                    //~ %1$s: vehicle name
                    _( "It's hard to get to sleep in %1$s." ),
                    vp->vehicle().disp_name() );
            }
        }
    }
    add_msg_if_player( _( "You start trying to fall asleep." ) );
    if( has_active_bionic( bio_soporific ) ) {
        bio_soporific_powered_at_last_sleep_check = has_power();
        if( bio_soporific_powered_at_last_sleep_check ) {
            // The actual bonus is applied in sleep_spot( p ).
            add_msg_if_player( m_good, _( "Your soporific inducer starts working its magic." ) );
        } else {
            add_msg_if_player( m_bad, _( "Your soporific inducer doesn't have enough power to operate." ) );
        }
    }
    assign_activity( player_activity( try_sleep_activity_actor( dur ) ) );
}

bool avatar::query_yn( const std::string &mes ) const
{
    return ::query_yn( mes );
}
