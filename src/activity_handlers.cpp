#include "activity_handlers.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <ostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "action.h"
#include "activity_type.h"
#include "advanced_inv.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "bodypart.h"
#include "butchery_requirements.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "construction.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "effect_source.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "fault.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "handle_liquid.h"
#include "harvest.h"
#include "iexamine.h"
#include "inventory.h"
#include "item.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_stack.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "line.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "martialarts.h"
#include "memory_fast.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "npc.h"
#include "omdata.h"
#include "optional.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "proficiency.h"
#include "ranged.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_interact.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

enum class creature_size : int;

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const activity_id ACT_ADV_INVENTORY( "ACT_ADV_INVENTORY" );
static const activity_id ACT_ARMOR_LAYERS( "ACT_ARMOR_LAYERS" );
static const activity_id ACT_ATM( "ACT_ATM" );
static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_BLEED( "ACT_BLEED" );
static const activity_id ACT_BUTCHER( "ACT_BUTCHER" );
static const activity_id ACT_BUTCHER_FULL( "ACT_BUTCHER_FULL" );
static const activity_id ACT_CHOP_LOGS( "ACT_CHOP_LOGS" );
static const activity_id ACT_CHOP_PLANKS( "ACT_CHOP_PLANKS" );
static const activity_id ACT_CHOP_TREE( "ACT_CHOP_TREE" );
static const activity_id ACT_CHURN( "ACT_CHURN" );
static const activity_id ACT_CLEAR_RUBBLE( "ACT_CLEAR_RUBBLE" );
static const activity_id ACT_CONSUME_DRINK_MENU( "ACT_CONSUME_DRINK_MENU" );
static const activity_id ACT_CONSUME_FOOD_MENU( "ACT_CONSUME_FOOD_MENU" );
static const activity_id ACT_CONSUME_MEDS_MENU( "ACT_CONSUME_MEDS_MENU" );
static const activity_id ACT_CONSUME_FUEL_MENU( "ACT_CONSUME_FUEL_MENU" );
static const activity_id ACT_DISMEMBER( "ACT_DISMEMBER" );
static const activity_id ACT_DISSECT( "ACT_DISSECT" );
static const activity_id ACT_EAT_MENU( "ACT_EAT_MENU" );
static const activity_id ACT_FERTILIZE_PLOT( "ACT_FERTILIZE_PLOT" );
static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );
static const activity_id ACT_FIELD_DRESS( "ACT_FIELD_DRESS" );
static const activity_id ACT_FILL_LIQUID( "ACT_FILL_LIQUID" );
static const activity_id ACT_FILL_PIT( "ACT_FILL_PIT" );
static const activity_id ACT_FIND_MOUNT( "ACT_FIND_MOUNT" );
static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_FORAGE( "ACT_FORAGE" );
static const activity_id ACT_GAME( "ACT_GAME" );
static const activity_id ACT_GENERIC_GAME( "ACT_GENERIC_GAME" );
static const activity_id ACT_GUNMOD_ADD( "ACT_GUNMOD_ADD" );
static const activity_id ACT_HACKSAW( "ACT_HACKSAW" );
static const activity_id ACT_HAND_CRANK( "ACT_HAND_CRANK" );
static const activity_id ACT_HEATING( "ACT_HEATING" );
static const activity_id ACT_JACKHAMMER( "ACT_JACKHAMMER" );
static const activity_id ACT_LONGSALVAGE( "ACT_LONGSALVAGE" );
static const activity_id ACT_MEND_ITEM( "ACT_MEND_ITEM" );
static const activity_id ACT_MIND_SPLICER( "ACT_MIND_SPLICER" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_MULTIPLE_CHOP_PLANKS( "ACT_MULTIPLE_CHOP_PLANKS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const activity_id ACT_MULTIPLE_MINE( "ACT_MULTIPLE_MINE" );
static const activity_id ACT_MULTIPLE_FARM( "ACT_MULTIPLE_FARM" );
static const activity_id ACT_MULTIPLE_FISH( "ACT_MULTIPLE_FISH" );
static const activity_id ACT_OPERATION( "ACT_OPERATION" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_PLANT_SEED( "ACT_PLANT_SEED" );
static const activity_id ACT_PRY_NAILS( "ACT_PRY_NAILS" );
static const activity_id ACT_PULP( "ACT_PULP" );
static const activity_id ACT_QUARTER( "ACT_QUARTER" );
static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );
static const activity_id ACT_ROBOT_CONTROL( "ACT_ROBOT_CONTROL" );
static const activity_id ACT_SKIN( "ACT_SKIN" );
static const activity_id ACT_SOCIALIZE( "ACT_SOCIALIZE" );
static const activity_id ACT_SPELLCASTING( "ACT_SPELLCASTING" );
static const activity_id ACT_START_ENGINES( "ACT_START_ENGINES" );
static const activity_id ACT_START_FIRE( "ACT_START_FIRE" );
static const activity_id ACT_STUDY_SPELL( "ACT_STUDY_SPELL" );
static const activity_id ACT_TIDY_UP( "ACT_TIDY_UP" );
static const activity_id ACT_TOOLMOD_ADD( "ACT_TOOLMOD_ADD" );
static const activity_id ACT_TRAIN( "ACT_TRAIN" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );
static const activity_id ACT_TREE_COMMUNION( "ACT_TREE_COMMUNION" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );
static const activity_id ACT_VIBE( "ACT_VIBE" );
static const activity_id ACT_WAIT( "ACT_WAIT" );
static const activity_id ACT_WAIT_NPC( "ACT_WAIT_NPC" );
static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );
static const activity_id ACT_WAIT_WEATHER( "ACT_WAIT_WEATHER" );
static const activity_id ACT_WASH( "ACT_WASH" );
static const activity_id ACT_WEAR( "ACT_WEAR" );
static const activity_id ACT_MULTIPLE_DIS( "ACT_MULTIPLE_DIS" );

static const efftype_id effect_blind( "blind" );
static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_under_operation( "under_operation" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_animal( "animal" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_burnt_out_bionic( "burnt_out_bionic" );
static const itype_id itype_log( "log" );
static const itype_id itype_mind_scan_robofac( "mind_scan_robofac" );
static const itype_id itype_muscle( "muscle" );
static const itype_id itype_nail( "nail" );
static const itype_id itype_pipe( "pipe" );
static const itype_id itype_rebar( "rebar" );
static const itype_id itype_scrap( "scrap" );
static const itype_id itype_sheet_metal( "sheet_metal" );
static const itype_id itype_spike( "spike" );
static const itype_id itype_splinter( "splinter" );
static const itype_id itype_stick_long( "stick_long" );
static const itype_id itype_steel_chunk( "steel_chunk" );
static const itype_id itype_wire( "wire" );
static const itype_id itype_chain( "chain" );

static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_electronics( "electronics" );
static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_CUT_FINE( "CUT_FINE" );

static const species_id species_HUMAN( "HUMAN" );
static const species_id species_ZOMBIE( "ZOMBIE" );

static const json_character_flag json_flag_CANNIBAL( "CANNIBAL" );
static const json_character_flag json_flag_PSYCHOPATH( "PSYCHOPATH" );
static const json_character_flag json_flag_SAPIOVORE( "SAPIOVORE" );

static const bionic_id bio_painkiller( "bio_painkiller" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_STOCKY_TROGLO( "STOCKY_TROGLO" );

using namespace activity_handlers;

const std::map< activity_id, std::function<void( player_activity *, Character * )> >
activity_handlers::do_turn_functions = {
    { ACT_FILL_LIQUID, fill_liquid_do_turn },
    { ACT_PICKAXE, pickaxe_do_turn },
    { ACT_PULP, pulp_do_turn },
    { ACT_GAME, game_do_turn },
    { ACT_GENERIC_GAME, generic_game_do_turn },
    { ACT_START_FIRE, start_fire_do_turn },
    { ACT_VIBE, vibe_do_turn },
    { ACT_HAND_CRANK, hand_crank_do_turn },
    { ACT_WEAR, wear_do_turn },
    { ACT_MULTIPLE_FISH, multiple_fish_do_turn },
    { ACT_MULTIPLE_CONSTRUCTION, multiple_construction_do_turn },
    { ACT_MULTIPLE_MINE, multiple_mine_do_turn },
    { ACT_MULTIPLE_BUTCHER, multiple_butcher_do_turn },
    { ACT_MULTIPLE_FARM, multiple_farm_do_turn },
    { ACT_FETCH_REQUIRED, fetch_do_turn },
    { ACT_BUILD, build_do_turn },
    { ACT_EAT_MENU, eat_menu_do_turn },
    { ACT_VEHICLE_DECONSTRUCTION, vehicle_deconstruction_do_turn },
    { ACT_VEHICLE_REPAIR, vehicle_repair_do_turn },
    { ACT_MULTIPLE_CHOP_TREES, chop_trees_do_turn },
    { ACT_CONSUME_FOOD_MENU, consume_food_menu_do_turn },
    { ACT_CONSUME_DRINK_MENU, consume_drink_menu_do_turn },
    { ACT_CONSUME_MEDS_MENU, consume_meds_menu_do_turn },
    { ACT_CONSUME_FUEL_MENU, consume_fuel_menu_do_turn },
    { ACT_MOVE_LOOT, move_loot_do_turn },
    { ACT_ADV_INVENTORY, adv_inventory_do_turn },
    { ACT_ARMOR_LAYERS, armor_layers_do_turn },
    { ACT_ATM, atm_do_turn },
    { ACT_FISH, fish_do_turn },
    { ACT_REPAIR_ITEM, repair_item_do_turn },
    { ACT_BLEED, butcher_do_turn },
    { ACT_BUTCHER, butcher_do_turn },
    { ACT_BUTCHER_FULL, butcher_do_turn },
    { ACT_TRAVELLING, travel_do_turn },
    { ACT_FIELD_DRESS, butcher_do_turn },
    { ACT_SKIN, butcher_do_turn },
    { ACT_QUARTER, butcher_do_turn },
    { ACT_DISMEMBER, butcher_do_turn },
    { ACT_DISSECT, butcher_do_turn },
    { ACT_HACKSAW, hacksaw_do_turn },
    { ACT_PRY_NAILS, pry_nails_do_turn },
    { ACT_CHOP_TREE, chop_tree_do_turn },
    { ACT_CHOP_LOGS, chop_tree_do_turn },
    { ACT_TIDY_UP, tidy_up_do_turn },
    { ACT_CHOP_PLANKS, chop_tree_do_turn },
    { ACT_TIDY_UP, tidy_up_do_turn },
    { ACT_JACKHAMMER, jackhammer_do_turn },
    { ACT_FIND_MOUNT, find_mount_do_turn },
    { ACT_FILL_PIT, fill_pit_do_turn },
    { ACT_MULTIPLE_CHOP_PLANKS, multiple_chop_planks_do_turn },
    { ACT_FERTILIZE_PLOT, fertilize_plot_do_turn },
    { ACT_OPERATION, operation_do_turn },
    { ACT_ROBOT_CONTROL, robot_control_do_turn },
    { ACT_TREE_COMMUNION, tree_communion_do_turn },
    { ACT_STUDY_SPELL, study_spell_do_turn},
    { ACT_WAIT_STAMINA, wait_stamina_do_turn },
    { ACT_MULTIPLE_DIS, multiple_dis_do_turn }
};

const std::map< activity_id, std::function<void( player_activity *, Character * )> >
activity_handlers::finish_functions = {
    { ACT_BLEED, butcher_finish },
    { ACT_BUTCHER, butcher_finish },
    { ACT_BUTCHER_FULL, butcher_finish },
    { ACT_FIELD_DRESS, butcher_finish },
    { ACT_SKIN, butcher_finish },
    { ACT_QUARTER, butcher_finish },
    { ACT_DISMEMBER, butcher_finish },
    { ACT_DISSECT, butcher_finish },
    { ACT_FIRSTAID, firstaid_finish },
    { ACT_FISH, fish_finish },
    { ACT_FORAGE, forage_finish },
    { ACT_LONGSALVAGE, longsalvage_finish },
    { ACT_PICKAXE, pickaxe_finish },
    { ACT_START_FIRE, start_fire_finish },
    { ACT_TRAIN, train_finish },
    { ACT_CHURN, churn_finish },
    { ACT_PLANT_SEED, plant_seed_finish },
    { ACT_VEHICLE, vehicle_finish },
    { ACT_START_ENGINES, start_engines_finish },
    { ACT_PULP, pulp_finish },
    { ACT_REPAIR_ITEM, repair_item_finish },
    { ACT_HEATING, heat_item_finish },
    { ACT_MEND_ITEM, mend_item_finish },
    { ACT_GUNMOD_ADD, gunmod_add_finish },
    { ACT_TOOLMOD_ADD, toolmod_add_finish },
    { ACT_CLEAR_RUBBLE, clear_rubble_finish },
    { ACT_WAIT, wait_finish },
    { ACT_WAIT_WEATHER, wait_weather_finish },
    { ACT_WAIT_NPC, wait_npc_finish },
    { ACT_WAIT_STAMINA, wait_stamina_finish },
    { ACT_SOCIALIZE, socialize_finish },
    { ACT_OPERATION, operation_finish },
    { ACT_VIBE, vibe_finish },
    { ACT_ATM, atm_finish },
    { ACT_EAT_MENU, eat_menu_finish },
    { ACT_CONSUME_FOOD_MENU, eat_menu_finish },
    { ACT_CONSUME_DRINK_MENU, eat_menu_finish },
    { ACT_CONSUME_MEDS_MENU, eat_menu_finish },
    { ACT_CONSUME_FUEL_MENU, eat_menu_finish },
    { ACT_WASH, washing_finish },
    { ACT_HACKSAW, hacksaw_finish },
    { ACT_PRY_NAILS, pry_nails_finish },
    { ACT_CHOP_TREE, chop_tree_finish },
    { ACT_CHOP_LOGS, chop_logs_finish },
    { ACT_CHOP_PLANKS, chop_planks_finish },
    { ACT_JACKHAMMER, jackhammer_finish },
    { ACT_FILL_PIT, fill_pit_finish },
    { ACT_ROBOT_CONTROL, robot_control_finish },
    { ACT_MIND_SPLICER, mind_splicer_finish },
    { ACT_SPELLCASTING, spellcasting_finish },
    { ACT_STUDY_SPELL, study_spell_finish }
};

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<butcher_type>( butcher_type data )
{
    switch( data ) {
    case butcher_type::BLEED: return "BLEED";
    case butcher_type::DISMEMBER: return "DISMEMBER";
    case butcher_type::DISSECT: return "DISSECT";
    case butcher_type::FIELD_DRESS: return "FIELD_DRESS";
    case butcher_type::FULL: return "FULL";
    case butcher_type::QUARTER: return "QUARTER";
    case butcher_type::QUICK: return "QUICK";
    case butcher_type::SKIN: return "SKIN";
    case butcher_type::NUM_TYPES: break;
    }
    cata_fatal( "Invalid valid_target" );
}
// *INDENT-ON*

} // namespace io

bool activity_handlers::resume_for_multi_activities( Character &you )
{
    if( !you.backlog.empty() ) {
        player_activity &back_act = you.backlog.front();
        if( back_act.is_multi_type() ) {
            you.assign_activity( you.backlog.front().id() );
            you.backlog.clear();
            return true;
        }
    }
    return false;
}

static bool check_butcher_cbm( const int roll )
{
    // Failure rates for dissection rolls
    // 90% at roll 0, 72% at roll 1, 60% at roll 2, 51% @ 3, 45% @ 4, 40% @ 5, ... , 25% @ 10
    // Roll is roughly a rng(0, -3 + 1st_aid + fine_cut_quality + 1/2 electronics + small_dex_bonus)
    // Roll is reduced by corpse damage level, but to no less then 0
    add_msg_debug( debugmode::DF_ACT_BUTCHER, "Roll = %i", roll );
    add_msg_debug( debugmode::DF_ACT_BUTCHER, "Failure chance = %f%%",
                   ( 9.0f / ( 10.0f + roll * 2.5f ) ) * 100.0f );
    const bool failed = x_in_y( 9, ( 10 + roll * 2.5 ) );
    return !failed;
}

static void butcher_cbm_item( const itype_id &what, const tripoint &pos,
                              const time_point &age, const int roll, const std::vector<flag_id> &flags,
                              const std::vector<fault_id> &faults )
{
    if( roll < 0 ) {
        return;
    }
    map &here = get_map();
    if( item::find_type( what )->bionic ) {
        item cbm( check_butcher_cbm( roll ) ? what : itype_burnt_out_bionic, age );
        for( const flag_id &flg : flags ) {
            cbm.set_flag( flg );
        }
        for( const fault_id &flt : faults ) {
            cbm.faults.emplace( flt );
        }
        add_msg( m_good, _( "You discover a %s!" ), cbm.tname() );
        here.add_item( pos, cbm );
    } else if( check_butcher_cbm( roll ) ) {
        item something( what, age );
        for( const flag_id &flg : flags ) {
            something.set_flag( flg );
        }
        for( const fault_id &flt : faults ) {
            something.faults.emplace( flt );
        }
        add_msg( m_good, _( "You discover a %s!" ), something.tname() );
        here.add_item( pos, something );
    } else {
        add_msg( m_bad, _( "You discover only damaged organs." ) );
    }
}

static void butcher_cbm_group(
    const item_group_id &group, const tripoint &pos, const time_point &age, const int roll,
    const std::vector<flag_id> &flags, const std::vector<fault_id> &faults )
{
    if( roll < 0 ) {
        return;
    }

    map &here = get_map();
    //To see if it spawns a random additional CBM
    if( check_butcher_cbm( roll ) ) {
        //The CBM works
        const std::vector<item *> spawned = here.put_items_from_loc( group, pos, age );
        for( item *it : spawned ) {
            for( const flag_id &flg : flags ) {
                it->set_flag( flg );
            }
            for( const fault_id &flt : faults ) {
                it->faults.emplace( flt );
            }
            add_msg( m_good, _( "You discover a %s!" ), it->tname() );
        }
    } else {
        //There is a burnt out CBM
        item cbm( itype_burnt_out_bionic, age );
        for( const flag_id &flg : flags ) {
            cbm.set_flag( flg );
        }
        for( const fault_id &flt : faults ) {
            cbm.faults.emplace( flt );
        }
        add_msg( m_good, _( "You discover a %s!" ), cbm.tname() );
        here.add_item( pos, cbm );
    }
}

static void set_up_butchery( player_activity &act, Character &you, butcher_type action )
{
    const int factor = you.max_quality( action == butcher_type::DISSECT ? qual_CUT_FINE : qual_BUTCHER,
                                        PICKUP_RANGE );

    const item &corpse_item = *act.targets.back();
    const mtype &corpse = *corpse_item.get_mtype();

    if( action != butcher_type::DISSECT ) {
        if( factor == INT_MIN ) {
            you.add_msg_if_player( m_info,
                                   _( "None of your cutting tools are suitable for butchering." ) );
            act.set_to_null();
            return;
        } else if( factor < 0 && one_in( 3 ) ) {
            you.add_msg_if_player( m_bad,
                                   _( "You don't trust the quality of your tools, but carry on anyway." ) );
        }
    }

    if( action == butcher_type::DISSECT ) {
        switch( factor ) {
            case INT_MIN:
                you.add_msg_if_player( m_info, _( "None of your tools are sharp and precise enough to do that." ) );
                act.set_to_null();
                return;
            case 1:
                you.add_msg_if_player( m_info, _( "You could use a better tool, but this will do." ) );
                break;
            case 2:
                you.add_msg_if_player( m_info, _( "This tool is great, but you still would like a scalpel." ) );
                break;
            case 3:
                you.add_msg_if_player( m_info, _( "You dissect the corpse with a trusty scalpel." ) );
                break;
            case 5:
                you.add_msg_if_player( m_info,
                                       _( "You dissect the corpse with a sophisticated system of surgical grade scalpels." ) );
                break;
        }
    }

    const std::pair<float, requirement_id> butchery_requirements =
        corpse.harvest->get_butchery_requirements().get_fastest_requirements( you.crafting_inventory(),
                corpse.size, action );

    // Requirements for the various types
    const requirement_id butchery_requirement = butchery_requirements.second;

    if( !butchery_requirement->can_make_with_inventory(
            you.crafting_inventory( you.pos(), PICKUP_RANGE ), is_crafting_component ) ) {
        std::string popup_output = _( "You can't butcher this; you are missing some tools.\n" );

        for( const std::string &str : butchery_requirement->get_folded_components_list(
                 45, c_light_gray, you.crafting_inventory( you.pos(), PICKUP_RANGE ), is_crafting_component ) ) {
            popup_output += str + '\n';
        }
        for( const std::string &str : butchery_requirement->get_folded_tools_list(
                 45, c_light_gray, you.crafting_inventory( you.pos(), PICKUP_RANGE ) ) ) {
            popup_output += str + '\n';
        }

        act.set_to_null();
        popup( popup_output );
        return;
    }

    if( action == butcher_type::BLEED && ( corpse_item.has_flag( flag_BLED ) ||
                                           corpse_item.has_flag( flag_QUARTERED ) || corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ||
                                           corpse_item.has_flag( flag_FIELD_DRESS ) ) ) {
        you.add_msg_if_player( m_info, _( "This corpse hase already been bled." ) );
        act.targets.pop_back();
        return;
    }

    if( action == butcher_type::DISSECT && ( corpse_item.has_flag( flag_QUARTERED ) ||
            corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) ) {
        you.add_msg_if_player( m_info,
                               _( "It would be futile to search for implants inside this badly damaged corpse." ) );
        act.targets.pop_back();
        return;
    }

    if( action == butcher_type::FIELD_DRESS && ( corpse_item.has_flag( flag_FIELD_DRESS ) ||
            corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) ) {
        you.add_msg_if_player( m_info, _( "This corpse is already field dressed." ) );
        act.targets.pop_back();
        return;
    }

    if( action == butcher_type::SKIN && corpse_item.has_flag( flag_SKINNED ) ) {
        you.add_msg_if_player( m_info, _( "This corpse is already skinned." ) );
        act.targets.pop_back();
        return;
    }

    if( action == butcher_type::QUARTER ) {
        if( corpse.size == creature_size::tiny ) {
            you.add_msg_if_player( m_bad, _( "This corpse is too small to quarter without damaging." ),
                                   corpse.nname() );
            act.targets.pop_back();
            return;
        }
        if( corpse_item.has_flag( flag_QUARTERED ) ) {
            you.add_msg_if_player( m_bad, _( "This is already quartered." ), corpse.nname() );
            act.targets.pop_back();
            return;
        }
        if( !( corpse_item.has_flag( flag_FIELD_DRESS ) ||
               corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) &&
            corpse_item.get_mtype()->harvest->has_entry_type( "offal" ) ) {
            you.add_msg_if_player( m_bad, _( "You need to perform field dressing before quartering." ),
                                   corpse.nname() );
            act.targets.pop_back();
            return;
        }
    }

    // applies to all butchery actions
    const bool is_human = corpse.id == mtype_id::NULL_ID() || ( corpse.in_species( species_HUMAN ) &&
                          !corpse.in_species( species_ZOMBIE ) );
    if( is_human && !( you.has_trait_flag( json_flag_CANNIBAL ) ||
                       you.has_trait_flag( json_flag_PSYCHOPATH ) ||
                       you.has_trait_flag( json_flag_SAPIOVORE ) ) ) {

        if( you.is_avatar() ) {
            if( query_yn( _( "Would you dare desecrate the mortal remains of a fellow human being?" ) ) ) {
                switch( rng( 1, 3 ) ) {
                    case 1:
                        you.add_msg_if_player( m_bad, _( "You clench your teeth at the prospect of this gruesome job." ) );
                        break;
                    case 2:
                        you.add_msg_if_player( m_bad, _( "This will haunt you in your dreams." ) );
                        break;
                    case 3:
                        you.add_msg_if_player( m_bad,
                                               _( "You try to look away, but this gruesome image will stay on your mind for some time." ) );
                        break;
                }
                get_player_character().add_morale( MORALE_BUTCHER, -50, 0, 2_days, 3_hours );
            } else {
                you.add_msg_if_player( m_good, _( "It needs a coffin, not a knife." ) );
                act.targets.pop_back();
                return;
            }
        } else {
            you.add_morale( MORALE_BUTCHER, -50, 0, 2_days, 3_hours );
        }
    }

    act.moves_left = butcher_time_to_cut( you, corpse_item, action ) * butchery_requirements.first;

    // We have a valid target, so preform the full finish function
    // instead of just selecting the next valid target
    act.index = false;
}

int butcher_time_to_cut( Character &you, const item &corpse_item, const butcher_type action )
{
    const mtype &corpse = *corpse_item.get_mtype();
    const int factor = you.max_quality( action == butcher_type::DISSECT ? qual_CUT_FINE : qual_BUTCHER,
                                        PICKUP_RANGE );

    int time_to_cut;
    switch( corpse.size ) {
        // Time (roughly) in turns to cut up the corpse
        case creature_size::tiny:
            time_to_cut = 150;
            break;
        case creature_size::small:
            time_to_cut = 300;
            break;
        case creature_size::medium:
            time_to_cut = 450;
            break;
        case creature_size::large:
            time_to_cut = 600;
            break;
        case creature_size::huge:
            time_to_cut = 1800;
            break;
        default:
            debugmsg( "ERROR: Invalid creature_size on %s", corpse.nname() );
            time_to_cut = 450; // default to medium
            break;
    }

    // At factor 0, base 100 time_to_cut remains 100. At factor 50, it's 50 , at factor 75 it's 25
    time_to_cut *= std::max( 25, 100 - factor );

    switch( action ) {
        case butcher_type::QUICK:
        case butcher_type::BLEED:
            break;
        case butcher_type::FULL:
            if( !corpse_item.has_flag( flag_FIELD_DRESS ) || corpse_item.has_flag( flag_FIELD_DRESS_FAILED ) ) {
                time_to_cut *= 6;
            } else {
                time_to_cut *= 4;
            }
            break;
        case butcher_type::FIELD_DRESS:
        case butcher_type::SKIN:
            time_to_cut *= 2;
            break;
        case butcher_type::QUARTER:
            time_to_cut /= 4;
            if( time_to_cut < 1200 ) {
                time_to_cut = 1200;
            }
            break;
        case butcher_type::DISMEMBER:
            time_to_cut /= 10;
            if( time_to_cut < 600 ) {
                time_to_cut = 600;
            }
            break;
        case butcher_type::DISSECT:
            time_to_cut *= 6;
            break;
        case butcher_type::NUM_TYPES:
            debugmsg( "ERROR: Invalid butcher_type" );
            break;
    }

    if( corpse_item.has_flag( flag_QUARTERED ) ) {
        time_to_cut /= 4;
    }
    time_to_cut *= ( 1.0f - ( get_player_character().get_num_crafting_helpers( 3 ) / 10.0f ) );
    return time_to_cut;
}

// this function modifies the input weight by its damage level, depending on the bodypart
static int corpse_damage_effect( int weight, const std::string &entry_type, int damage_level )
{
    const float slight_damage = 0.9f;
    const float damage = 0.75f;
    const float high_damage = 0.5f;
    const int destroyed = 0;

    switch( damage_level ) {
        case 2:
            // "damaged"
            if( entry_type == "offal" ) {
                return std::round( weight * damage );
            }
            if( entry_type == "skin" ) {
                return std::round( weight * damage );
            }
            if( entry_type == "flesh" ) {
                return std::round( weight * slight_damage );
            }
            break;
        case 3:
            // "mangled"
            if( entry_type == "offal" ) {
                return destroyed;
            }
            if( entry_type == "skin" ) {
                return std::round( weight * high_damage );
            }
            if( entry_type == "bone" ) {
                return std::round( weight * slight_damage );
            }
            if( entry_type == "flesh" ) {
                return std::round( weight * damage );
            }
            break;
        case 4:
            // "pulped"
            if( entry_type == "offal" ) {
                return destroyed;
            }
            if( entry_type == "skin" ) {
                return destroyed;
            }
            if( entry_type == "bone" ) {
                return std::round( weight * damage );
            }
            if( entry_type == "flesh" ) {
                return std::round( weight * high_damage );
            }
            break;
        default:
            // "bruised" modifier is almost impossible to avoid; also includes no modifier (zero damage)
            break;
    }
    return weight;
}

static void butchery_drops_harvest( item *corpse_item, const mtype &mt, Character &you,
                                    const std::function<int()> &roll_butchery, butcher_type action,
                                    double yield_multiplier )
{
    you.add_msg_if_player( m_neutral, mt.harvest->message() );
    int monster_weight = to_gram( mt.weight );
    monster_weight += std::round( monster_weight * rng_float( -0.1, 0.1 ) );
    if( corpse_item->has_flag( flag_QUARTERED ) ) {
        monster_weight *= 0.95;
    }
    if( corpse_item->has_flag( flag_GIBBED ) ) {
        monster_weight = std::round( 0.85 * monster_weight );
        if( action != butcher_type::FIELD_DRESS ) {
            you.add_msg_if_player( m_bad,
                                   _( "You salvage what you can from the corpse, but it is badly damaged." ) );
        }
    }
    if( corpse_item->has_flag( flag_SKINNED ) ) {
        monster_weight = std::round( 0.85 * monster_weight );
    }
    int monster_weight_remaining = monster_weight;
    int practice = 4 + roll_butchery();

    if( mt.harvest.is_null() ) {
        debugmsg( "ERROR: %s has no harvest entry.", mt.id.c_str() );
        return;
    }

    map &here = get_map();
    for( const harvest_entry &entry : *mt.harvest ) {
        const int butchery = roll_butchery();
        const float min_num = entry.base_num.first + butchery * entry.scale_num.first;
        const float max_num = entry.base_num.second + butchery * entry.scale_num.second;
        int roll = 0;
        // mass_ratio will override the use of base_num, scale_num, and max
        if( entry.mass_ratio != 0.00f ) {
            roll = static_cast<int>( std::round( entry.mass_ratio * monster_weight ) );
            roll = corpse_damage_effect( roll, entry.type, corpse_item->damage_level() );
        } else if( entry.type != "bionic" && entry.type != "bionic_group" ) {
            roll = std::min<int>( entry.max, std::round( rng_float( min_num, max_num ) ) );
            // will not give less than min_num defined in the JSON
            roll = std::max<int>( corpse_damage_effect( roll, entry.type, corpse_item->damage_level() ),
                                  entry.base_num.first );
        }
        itype_id drop_id = itype_id::NULL_ID();
        const itype *drop = nullptr;
        if( entry.type != "bionic_group" ) {
            drop_id = itype_id( entry.drop );
            drop = item::find_type( drop_id );
        }

        // BIONIC handling - no code for DISSECT to let the bionic drop fall through
        if( entry.type == "bionic" || entry.type == "bionic_group" ) {
            if( action == butcher_type::FIELD_DRESS ) {
                if( drop != nullptr && !drop->bionic ) {
                    if( one_in( 3 ) ) {
                        you.add_msg_if_player( m_bad,
                                               _( "You notice some strange organs, perhaps harvestable via careful dissection." ) );
                    }
                    continue;
                }
                you.add_msg_if_player( m_bad,
                                       _( "You suspect there might be bionics implanted in this corpse, that careful dissection might reveal." ) );
                continue;
            }
            if( action == butcher_type::QUICK ||
                action == butcher_type::FULL ||
                action == butcher_type::DISMEMBER ) {
                if( drop != nullptr && !drop->bionic ) {
                    if( one_in( 3 ) ) {
                        you.add_msg_if_player( m_bad,
                                               _( "Your butchering tool destroys a strange organ.  Perhaps a more surgical approach would allow harvesting it." ) );
                    }
                    continue;
                }
                switch( rng( 1, 3 ) ) {
                    case 1:
                        you.add_msg_if_player( m_bad,
                                               _( "Your butchering tool encounters something implanted in this corpse, but your rough cuts destroy it." ) );
                        break;
                    case 2:
                        you.add_msg_if_player( m_bad,
                                               _( "You find traces of implants in the body, but you care only for the flesh." ) );
                        break;
                    case 3:
                        you.add_msg_if_player( m_bad,
                                               _( "You found some bionics in the body, but harvesting them would require more surgical approach." ) );
                        break;
                }
                continue;
            }
        }
        if( action == butcher_type::DISSECT ) {
            int roll = roll_butchery() - corpse_item->damage_level();
            roll = roll < 0 ? 0 : roll;
            roll = std::min( entry.max, roll );
            add_msg_debug( debugmode::DF_ACT_BUTCHER, "Roll penalty for corpse damage = %s",
                           0 - corpse_item->damage_level() );
            if( entry.type == "bionic" ) {
                butcher_cbm_item( drop_id, you.pos(), calendar::turn, roll, entry.flags, entry.faults );
            } else if( entry.type == "bionic_group" ) {
                butcher_cbm_group( item_group_id( entry.drop ), you.pos(), calendar::turn, roll,
                                   entry.flags, entry.faults );
            }
            continue;
        }

        // Check if monster was gibbed, and handle accordingly
        if( corpse_item->has_flag( flag_GIBBED ) && ( entry.type == "flesh" || entry.type == "bone" ) ) {
            roll /= 2;
        }

        if( corpse_item->has_flag( flag_SKINNED ) && entry.type == "skin" ) {
            roll = 0;
        }

        // QUICK BUTCHERY
        if( action == butcher_type::QUICK ) {
            if( entry.type == "flesh" ) {
                roll /= 4;
            } else if( entry.type == "bone" ) { // NOLINT(bugprone-branch-clone)
                roll /= 2;
            } else if( corpse_item->get_mtype()->size >= creature_size::medium && entry.type == "skin" ) {
                roll /= 2;
            } else if( entry.type == "offal" ) {
                roll /= 5;
            } else {
                continue;
            }
        }
        // RIP AND TEAR
        if( action == butcher_type::DISMEMBER ) {
            if( entry.type == "flesh" ) {
                roll /= 6;
            } else {
                continue;
            }
        }
        // field dressing ignores skin, flesh, and blood
        if( action == butcher_type::FIELD_DRESS ) {
            if( entry.type == "bone" ) {
                roll = rng( 0, roll / 2 );
            }
            if( entry.type == "flesh" ) {
                continue;
            }
            if( entry.type == "skin" ) {
                continue;
            }
        }

        // you only get the blood from bleeding
        if( action == butcher_type::BLEED ) {
            if( entry.type != "blood" ) {
                continue;
            }
        }

        // you only get the skin from skinning
        if( action == butcher_type::SKIN ) {
            if( entry.type != "skin" ) {
                continue;
            }
            if( corpse_item->has_flag( flag_FIELD_DRESS_FAILED ) ) {
                roll = rng( 0, roll );
            }
        }

        // field dressing removed innards and bones from meatless limbs
        if( ( action == butcher_type::FULL || action == butcher_type::QUICK ) &&
            corpse_item->has_flag( flag_FIELD_DRESS ) ) {
            if( entry.type == "offal" ) {
                continue;
            }
            if( entry.type == "bone" ) {
                roll = ( roll / 2 ) + rng( roll / 2, roll );
            }
        }
        // unskillfull field dressing may damage the skin, meat, and other parts
        if( ( action == butcher_type::FULL || action == butcher_type::QUICK ) &&
            corpse_item->has_flag( flag_FIELD_DRESS_FAILED ) ) {
            if( entry.type == "offal" ) {
                continue;
            }
            if( entry.type == "bone" ) {
                roll = ( roll / 2 ) + rng( roll / 2, roll );
            }
            if( entry.type == "flesh" || entry.type == "skin" ) {
                roll = rng( 0, roll );
            }
        }
        // quartering ruins skin
        if( corpse_item->has_flag( flag_QUARTERED ) ) {
            if( entry.type == "skin" ) {
                //not continue to show fail effect
                roll = 0;
            } else {
                roll /= 4;
            }
        }

        if( entry.type != "bionic_group" ) {
            // divide total dropped weight by drop's weight to get amount
            if( entry.mass_ratio != 0.00f ) {
                // apply skill before converting to items, but only if mass_ratio is defined
                roll *= yield_multiplier;
                monster_weight_remaining -= roll;
                roll = std::ceil( static_cast<double>( roll ) /
                                  to_gram( drop->weight ) );
            } else {
                monster_weight_remaining -= roll * to_gram( drop->weight );
            }

            if( roll <= 0 ) {
                you.add_msg_if_player( m_bad, _( "You fail to harvest: %s" ), drop->nname( 1 ) );
                continue;
            }
            if( drop->phase == phase_id::LIQUID ) {
                item obj( drop, calendar::turn, roll );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( 0.00001 * corpse_item->temperature );
                    if( obj.goes_bad() ) {
                        obj.set_rot( corpse_item->get_rot() );
                    }
                }
                for( const flag_id &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.faults.emplace( flt );
                }

                // TODO: smarter NPC liquid handling
                // If we're not bleeding the animal we don't care about the blood being wasted
                if( you.is_npc() || action != butcher_type::BLEED ) {
                    drop_on_map( you, item_drop_reason::deliberate, { obj }, you.pos() );
                } else {
                    liquid_handler::handle_all_liquid( obj, 1 );
                }
            } else if( drop->count_by_charges() ) {
                item obj( drop, calendar::turn, roll );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( 0.00001 * corpse_item->temperature );
                    if( obj.goes_bad() ) {
                        obj.set_rot( corpse_item->get_rot() );
                    }
                }
                for( const flag_id &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.faults.emplace( flt );
                }
                if( !you.backlog.empty() && you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ) {
                    obj.set_var( "activity_var", you.name );
                }
                here.add_item_or_charges( you.pos(), obj );
            } else {
                item obj( drop, calendar::turn );
                obj.set_mtype( &mt );
                if( obj.has_temperature() ) {
                    obj.set_item_temperature( 0.00001 * corpse_item->temperature );
                    if( obj.goes_bad() ) {
                        obj.set_rot( corpse_item->get_rot() );
                    }
                }
                for( const flag_id &flg : entry.flags ) {
                    obj.set_flag( flg );
                }
                for( const fault_id &flt : entry.faults ) {
                    obj.faults.emplace( flt );
                }
                if( !you.backlog.empty() && you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ) {
                    obj.set_var( "activity_var", you.name );
                }
                for( int i = 0; i != roll; ++i ) {
                    here.add_item_or_charges( you.pos(), obj );
                }
            }
            you.add_msg_if_player( m_good, _( "You harvest: %s" ), drop->nname( roll ) );
        }
        practice++;
    }
    // 20% of the original corpse weight is not an item, but liquid gore
    monster_weight_remaining -= monster_weight / 5;
    // add the remaining unusable weight as rotting garbage
    if( monster_weight_remaining > 0 && action != butcher_type::BLEED ) {
        if( action == butcher_type::FIELD_DRESS ) {
            // 25% of the corpse weight is what's removed during field dressing
            monster_weight_remaining -= monster_weight * 3 / 4;
        } else if( action == butcher_type::SKIN ) {
            monster_weight_remaining -= monster_weight * 0.85;
        } else {
            // a carcass is 75% of the weight of the unmodified creature's weight
            if( ( corpse_item->has_flag( flag_FIELD_DRESS ) ||
                  corpse_item->has_flag( flag_FIELD_DRESS_FAILED ) ) &&
                !corpse_item->has_flag( flag_QUARTERED ) ) {
                monster_weight_remaining -= monster_weight / 4;
            } else if( corpse_item->has_flag( flag_QUARTERED ) ) {
                monster_weight_remaining -= ( monster_weight - ( monster_weight * 3 / 4 / 4 ) );
            }
            if( corpse_item->has_flag( flag_SKINNED ) ) {
                monster_weight_remaining -= monster_weight * 0.15;
            }
        }
        const itype_id &leftover_id = mt.harvest->leftovers;
        const int item_charges = monster_weight_remaining / to_gram( (
                                     item::find_type( leftover_id ) )->weight );
        if( item_charges > 0 ) {
            item ruined_parts( leftover_id, calendar::turn, item_charges );
            ruined_parts.set_mtype( &mt );
            ruined_parts.set_item_temperature( 0.00001 * corpse_item->temperature );
            ruined_parts.set_rot( corpse_item->get_rot() );
            if( !you.backlog.empty() && you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ) {
                ruined_parts.set_var( "activity_var", you.name );
            }
            here.add_item_or_charges( you.pos(), ruined_parts );
        }
    }

    if( action == butcher_type::DISSECT ) {
        you.practice( skill_firstaid, std::max( 0, practice ), std::max( mt.size - creature_size::medium,
                      0 ) + 4 );
    } else {
        you.practice( skill_survival, std::max( 0, practice ), std::max( mt.size - creature_size::medium,
                      0 ) + 4 );
    }
}

static void butchery_quarter( item *corpse_item, const Character &you )
{
    corpse_item->set_flag( flag_QUARTERED );
    you.add_msg_if_player( m_good,
                           _( "You roughly slice the corpse of %s into four parts and set them aside." ),
                           corpse_item->get_mtype()->nname() );
    map &here = get_map();
    // 4 quarters (one exists, add 3, flag does the rest)
    for( int i = 1; i <= 3; i++ ) {
        here.add_item_or_charges( you.pos(), *corpse_item, true );
    }
}

void activity_handlers::butcher_finish( player_activity *act, Character *you )
{
    // No targets means we are done
    if( act->targets.empty() ) {
        act->set_to_null();
        resume_for_multi_activities( *you );
        return;
    }

    item_location target = act->targets.back();

    // Corpses can disappear (rezzing!), so check for that
    if( !target || !target->is_corpse() ) {
        you->add_msg_if_player( m_info, _( "There's no corpse to butcher!" ) );
        act->set_to_null();
        return;
    }

    butcher_type action = butcher_type::QUICK;
    if( act->id() == ACT_BUTCHER ) {
        action = butcher_type::QUICK;
    } else if( act->id() == ACT_BUTCHER_FULL ) {
        action = butcher_type::FULL;
    } else if( act->id() == ACT_FIELD_DRESS ) {
        action = butcher_type::FIELD_DRESS;
    } else if( act->id() == ACT_QUARTER ) {
        action = butcher_type::QUARTER;
    } else if( act->id() == ACT_DISSECT ) {
        action = butcher_type::DISSECT;
    } else if( act->id() == ACT_BLEED ) {
        action = butcher_type::BLEED;
    } else if( act->id() == ACT_SKIN ) {
        action = butcher_type::SKIN;
    } else if( act->id() == ACT_DISMEMBER ) {
        action = butcher_type::DISMEMBER;
    }

    // index is a bool that determines if we are ready to start the next target
    if( act->index ) {
        set_up_butchery( *act, *you, action );
        return;
    }

    item &corpse_item = *target;
    const mtype *corpse = corpse_item.get_mtype();
    const field_type_id type_blood = corpse->bloodType();
    const field_type_id type_gib = corpse->gibType();

    if( action == butcher_type::QUARTER ) {
        butchery_quarter( &corpse_item, *you );
        act->index = true;
        return;
    }

    const int tool_quality = you->max_quality( action == butcher_type::DISSECT ? qual_CUT_FINE :
                             qual_BUTCHER, PICKUP_RANGE );
    const int skill_level = [&]() {
        // DISSECT has special case factor calculation and results.
        if( action == butcher_type::DISSECT ) {
            return you->get_skill_level( skill_firstaid ) + tool_quality +
                   you->get_skill_level( skill_electronics ) / 2;
        }
        return you->get_skill_level( skill_survival );
    }
    ();
    add_msg_debug( debugmode::DF_ACT_BUTCHER, "Skill: %s", skill_level );

    const auto roll_butchery = [&]() {
        double skill_shift = 0.0;
        ///\EFFECT_SURVIVAL randomly increases butcher rolls
        skill_shift += rng_float( 0, skill_level - 3 );
        ///\EFFECT_DEX >8 randomly increases butcher rolls, slightly, <8 decreases
        skill_shift += rng_float( 0, you->dex_cur - 8 ) / 4.0;

        if( tool_quality < 0 ) {
            skill_shift -= rng_float( 0, -tool_quality / 5.0 );
        }

        return static_cast<int>( std::round( skill_shift ) );
    };

    map &here = get_map();
    if( action == butcher_type::DISMEMBER ) {
        here.add_splatter( type_gib, you->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
    }

    //all BUTCHERY types - FATAL FAILURE
    if( action != butcher_type::DISSECT && roll_butchery() <= ( -15 ) && one_in( 2 ) ) {
        switch( rng( 1, 3 ) ) {
            case 1:
                you->add_msg_if_player( m_warning,
                                        _( "You hack up the corpse so unskillfully, that there is nothing left to salvage from this bloody mess." ) );
                break;
            case 2:
                you->add_msg_if_player( m_warning,
                                        _( "You wanted to cut the corpse, but instead you hacked the meat, spilled the guts all over it, and made a bloody mess." ) );
                break;
            case 3:
                you->add_msg_if_player( m_warning,
                                        _( "You made so many mistakes during the process that you doubt even vultures will be interested in what's left of it." ) );
                break;
        }

        // Remove the target from the map
        target.remove_item();
        act->targets.pop_back();

        here.add_splatter( type_gib, you->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        here.add_splatter( type_blood, you->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
        for( int i = 1; i <= corpse->size; i++ ) {
            here.add_splatter_trail( type_gib, you->pos(), random_entry( here.points_in_radius( you->pos(),
                                     corpse->size + 1 ) ) );
            here.add_splatter_trail( type_blood, you->pos(), random_entry( here.points_in_radius( you->pos(),
                                     corpse->size + 1 ) ) );
        }

        // Ready to move on to the next item, if there is one
        act->index = true;
        return;
    }
    // A number between 0 and 1 that represents how much usable material can be harvested
    // as a fraction of the maximum possible amount.
    const double yield_multiplier = [&]() {
        const double skill_score = skill_level / 10.0;
        const double tool_score = ( tool_quality + 50.0 ) / 100.0;
        const double dex_score = you->dex_cur / 20.0;
        return 0.5 * clamp( skill_score, 0.0, 1.0 ) +
               0.3 * clamp( tool_score, 0.0, 1.0 ) +
               0.2 * clamp( dex_score, 0.0, 1.0 );
    }
    ();
    // all action types - yields
    butchery_drops_harvest( &corpse_item, *corpse, *you, roll_butchery, action, yield_multiplier );
    // after this point, if there was a liquid handling from the harvest,
    // and the liquid handling was interrupted, then the activity was canceled,
    // therefore operations on this activity's targets and values may be invalidated.
    // reveal hidden items / hidden content
    if( action != butcher_type::FIELD_DRESS && action != butcher_type::SKIN &&
        action != butcher_type::BLEED ) {
        for( item *content : corpse_item.all_items_top() ) {
            if( ( roll_butchery() + 10 ) * 5 > rng( 0, 100 ) ) {
                //~ %1$s - item name, %2$s - monster name
                you->add_msg_if_player( m_good, _( "You discover a %1$s in the %2$s!" ), content->tname(),
                                        corpse->nname() );
                here.add_item_or_charges( you->pos(), *content );
            } else if( content->is_bionic() ) {
                here.spawn_item( you->pos(), itype_burnt_out_bionic, 1, 0, calendar::turn );
            }
        }
    }

    //end messages and effects
    switch( action ) {
        case butcher_type::QUARTER:
            break;
        case butcher_type::QUICK:
            you->add_msg_if_player( m_good,
                                    _( "You apply few quick cuts to the %s and leave what's left of it for scavengers." ),
                                    corpse_item.tname() );

            // Remove the target from the map
            target.remove_item();
            if( !act->targets.empty() ) {
                act->targets.pop_back();
            }
            break;
        case butcher_type::FULL:
            you->add_msg_if_player( m_good, _( "You finish butchering the %s." ), corpse_item.tname() );

            // Remove the target from the map
            target.remove_item();
            if( !act->targets.empty() ) {
                act->targets.pop_back();
            }
            break;
        case butcher_type::FIELD_DRESS:
            // partial failure
            if( roll_butchery() < 0 ) {
                switch( rng( 1, 3 ) ) {
                    case 1:
                        you->add_msg_if_player( m_warning,
                                                _( "You unskillfully hack up the corpse and chop off some excess body parts.  You're left wondering how you did so poorly." ) );
                        break;
                    case 2:
                        you->add_msg_if_player( m_warning,
                                                _( "Your unskilled hands slip and damage the corpse.  You still hope it's not a total waste though." ) );
                        break;
                    case 3:
                        you->add_msg_if_player( m_warning,
                                                _( "You did something wrong and hacked the corpse badly.  Maybe it's still recoverable." ) );
                        break;
                }
                corpse_item.set_flag( flag_FIELD_DRESS_FAILED );

                here.add_splatter( type_gib, you->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                here.add_splatter( type_blood, you->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                for( int i = 1; i <= corpse->size; i++ ) {
                    here.add_splatter_trail( type_gib, you->pos(), random_entry( here.points_in_radius( you->pos(),
                                             corpse->size + 1 ) ) );
                    here.add_splatter_trail( type_blood, you->pos(), random_entry( here.points_in_radius( you->pos(),
                                             corpse->size + 1 ) ) );
                }

            } else {
                // success
                switch( rng( 1, 3 ) ) {
                    case 1:
                        you->add_msg_if_player( m_good, _( "You field dress the %s." ), corpse->nname() );
                        break;
                    case 2:
                        you->add_msg_if_player( m_good,
                                                _( "You slice the corpse's belly and remove intestines and organs, until you're confident that it will not rot from inside." ) );
                        break;
                    case 3:
                        you->add_msg_if_player( m_good,
                                                _( "You remove guts and excess parts, preparing the corpse for later use." ) );
                        break;
                }
                corpse_item.set_flag( flag_FIELD_DRESS );

                here.add_splatter( type_gib, you->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                here.add_splatter( type_blood, you->pos(), rng( corpse->size + 2, ( corpse->size + 1 ) * 2 ) );
                for( int i = 1; i <= corpse->size; i++ ) {
                    here.add_splatter_trail( type_gib, you->pos(), random_entry( here.points_in_radius( you->pos(),
                                             corpse->size + 1 ) ) );
                    here.add_splatter_trail( type_blood, you->pos(), random_entry( here.points_in_radius( you->pos(),
                                             corpse->size + 1 ) ) );
                }

            }
            if( !act->targets.empty() ) {
                act->targets.pop_back();
            }
            break;
        case butcher_type::SKIN:
            switch( rng( 1, 4 ) ) {
                case 1:
                    you->add_msg_if_player( m_good, _( "You skin the %s." ), corpse->nname() );
                    break;
                case 2:
                    you->add_msg_if_player( m_good, _( "You carefully remove the hide from the %s" ),
                                            corpse->nname() );
                    break;
                case 3:
                    you->add_msg_if_player( m_good,
                                            _( "The %s is challenging to skin, but you get a good hide from it." ),
                                            corpse->nname() );
                    break;
                case 4:
                    you->add_msg_if_player( m_good, _( "With a few deft slices you take the skin from the %s" ),
                                            corpse->nname() );
                    break;
            }
            corpse_item.set_flag( flag_SKINNED );
            if( !act->targets.empty() ) {
                act->targets.pop_back();
            }
            break;
        case butcher_type::BLEED:
            you->add_msg_if_player( m_good, _( "You bleed the %s." ), corpse->nname() );
            corpse_item.set_flag( flag_BLED );
            if( !act->targets.empty() ) {
                act->targets.pop_back();
            }
            break;
        case butcher_type::DISMEMBER:
            switch( rng( 1, 3 ) ) {
                case 1:
                    you->add_msg_if_player( m_good, _( "You hack the %s apart." ), corpse_item.tname() );
                    break;
                case 2:
                    you->add_msg_if_player( m_good, _( "You lop the limbs off the %s." ), corpse_item.tname() );
                    break;
                case 3:
                    you->add_msg_if_player( m_good, _( "You cleave the %s into pieces." ), corpse_item.tname() );
            }

            // Remove the target from the map
            target.remove_item();
            if( !act->targets.empty() ) {
                act->targets.pop_back();
            }
            break;
        case butcher_type::DISSECT:
            you->add_msg_if_player( m_good, _( "You finish dissecting the %s." ), corpse_item.tname() );

            // Remove the target from the map
            target.remove_item();
            if( !act->targets.empty() ) {
                act->targets.pop_back();
            }
            break;
        case butcher_type::NUM_TYPES:
            debugmsg( "ERROR: Invalid butcher_type" );
            break;
    }

    // Ready to move on to the next item, if there is one (for example if multibutchering)
    act->index = true;
    // if its mutli-tile butchering,then restart the backlog.
    resume_for_multi_activities( *you );
}

void activity_handlers::fill_liquid_do_turn( player_activity *act, Character *you )
{
    player_activity &act_ref = *act;
    try {
        // 1. Gather the source item.
        vehicle *source_veh = nullptr;
        const tripoint source_pos = act_ref.coords.at( 0 );
        map &here = get_map();
        map_stack source_stack = here.i_at( source_pos );
        map_stack::iterator on_ground;
        monster *source_mon = nullptr;
        item liquid;
        const liquid_source_type source_type = static_cast<liquid_source_type>( act_ref.values.at( 0 ) );
        int part_num = -1;
        int veh_charges = 0;
        switch( source_type ) {
            case liquid_source_type::VEHICLE:
                source_veh = veh_pointer_or_null( here.veh_at( source_pos ) );
                if( source_veh == nullptr ) {
                    throw std::runtime_error( "could not find source vehicle for liquid transfer" );
                }
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                part_num = static_cast<int>( act_ref.values.at( 1 ) );
                veh_charges = liquid.charges;
                break;
            case liquid_source_type::INFINITE_MAP:
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = item::INFINITE_CHARGES;
                break;
            case liquid_source_type::MAP_ITEM:
                if( static_cast<size_t>( act_ref.values.at( 1 ) ) >= source_stack.size() ) {
                    throw std::runtime_error( "could not find source item on ground for liquid transfer" );
                }
                on_ground = source_stack.begin();
                std::advance( on_ground, act_ref.values.at( 1 ) );
                liquid = *on_ground;
                break;
            case liquid_source_type::MONSTER:
                Creature *c = get_creature_tracker().creature_at( source_pos );
                source_mon = dynamic_cast<monster *>( c );
                if( source_mon == nullptr ) {
                    debugmsg( "could not find source creature for liquid transfer" );
                    act_ref.set_to_null();
                }
                deserialize_from_string( liquid, act_ref.str_values.at( 0 ) );
                liquid.charges = 1;
                break;
        }

        static const units::volume volume_per_second = units::from_liter( 4.0F / 6.0F );
        const int charges_per_second = std::max( 1, liquid.charges_per_volume( volume_per_second ) );
        liquid.charges = std::min( charges_per_second, liquid.charges );
        const int original_charges = liquid.charges;
        if( liquid.has_temperature() && liquid.specific_energy < 0 ) {
            liquid.set_item_temperature( temp_to_kelvin( std::max( get_weather().get_temperature( you->pos() ),
                                         temperatures::cold ) ) );
        }

        // 2. Transfer charges.
        const vehicle *veh = nullptr;
        size_t part;
        switch( static_cast<liquid_target_type>( act_ref.values.at( 2 ) ) ) {
            case liquid_target_type::VEHICLE: {
                const optional_vpart_position vp = here.veh_at( act_ref.coords.at( 1 ) );
                if( act_ref.values.size() > 4 && vp ) {
                    const vpart_reference vpr( vp->vehicle(), act_ref.values.at( 4 ) );
                    veh = &vp->vehicle();
                    part = act_ref.values.at( 4 );
                    if( source_veh &&
                        source_veh->fuel_left( liquid.typeId(), false, ( veh ? std::function<bool( const vehicle_part & )> { [&]( const vehicle_part & pa )
                {
                    return &( veh->part( part ) ) != &pa;
                    }
                                                                                                                           } : return_true<const vehicle_part &> ) ) <= 0 ) {
                        act_ref.set_to_null();
                        return;
                    }
                    you->pour_into( vpr, liquid );

                } else {
                    throw std::runtime_error( "could not find target vehicle for liquid transfer" );
                }
                break;
            }
            case liquid_target_type::CONTAINER:
                you->pour_into( *act_ref.targets.at( 0 ), liquid );
                break;
            case liquid_target_type::MAP:
                if( iexamine::has_keg( act_ref.coords.at( 1 ) ) ) {
                    iexamine::pour_into_keg( act_ref.coords.at( 1 ), liquid );
                } else {
                    here.add_item_or_charges( act_ref.coords.at( 1 ), liquid );
                    you->add_msg_if_player( _( "You pour %1$s onto the ground." ), liquid.tname() );
                    liquid.charges = 0;
                }
                break;
            case liquid_target_type::MONSTER:
                liquid.charges = 0;
                break;
        }

        const int removed_charges = original_charges - liquid.charges;
        if( removed_charges == 0 ) {
            // Nothing has been transferred, target must be full.
            act_ref.set_to_null();
            return;
        }

        // 3. Remove charges from source.
        switch( source_type ) {
            case liquid_source_type::VEHICLE:
                if( part_num != -1 ) {
                    source_veh->drain( part_num, removed_charges );
                    liquid.charges = veh_charges - removed_charges;
                    // If there's no liquid left in this tank we're done, otherwise
                    // we need to update our liquid serialization to reflect how
                    // many charges are actually left for the next time we come
                    // around this loop.
                    if( !liquid.charges ) {
                        act_ref.set_to_null();
                    } else {
                        if( act_ref.str_values.empty() ) {
                            act_ref.str_values.emplace_back( );
                        }
                        act_ref.str_values.at( 0 ) = serialize( liquid );
                    }
                } else {
                    source_veh->drain( liquid.typeId(), removed_charges, ( veh ? std::function<bool( vehicle_part & )> { [&]( vehicle_part & pa )
                    {
                        return &( veh->part( part ) ) != &pa;
                    }
                                                                                                                       } : return_true<vehicle_part &> ) );
                }
                break;
            case liquid_source_type::MAP_ITEM:
                on_ground->charges -= removed_charges;
                if( on_ground->charges <= 0 ) {
                    source_stack.erase( on_ground );
                    if( here.ter( source_pos )->has_examine( iexamine::gaspump ) ) {
                        add_msg( _( "With a clang and a shudder, the %s pump goes silent." ),
                                 liquid.type_name( 1 ) );
                    } else if( here.furn( source_pos )->has_examine( iexamine::fvat_full ) ) {
                        add_msg( _( "You squeeze the last drops of %s from the vat." ),
                                 liquid.type_name( 1 ) );
                        map_stack items_here = here.i_at( source_pos );
                        if( items_here.empty() ) {
                            here.furn_set( source_pos, f_fvat_empty );
                        }
                    }
                    act_ref.set_to_null();
                }
                break;
            case liquid_source_type::INFINITE_MAP:
                // nothing, the liquid source is infinite
                break;
            case liquid_source_type::MONSTER:
                // liquid source charges handled in monexamine::milk_source
                if( liquid.charges == 0 ) {
                    act_ref.set_to_null();
                }
                break;
        }

        if( removed_charges < original_charges ) {
            // Transferred less than the available charges -> target must be full
            act_ref.set_to_null();
        }

    } catch( const std::runtime_error &err ) {
        debugmsg( "error in activity data: \"%s\"", err.what() );
        act_ref.set_to_null();
        return;
    }
}

void activity_handlers::firstaid_finish( player_activity *act, Character *you )
{
    static const std::string iuse_name_string( "heal" );

    item &it = *act->targets.front();
    item *used_tool = it.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for healing" );
        act->set_to_null();
        return;
    }

    const use_function *use_fun = used_tool->get_use( iuse_name_string );
    const heal_actor *actor = dynamic_cast<const heal_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    // TODO: Store the patient somehow, retrieve here
    Character &patient = *you;
    const bodypart_id healed = bodypart_id( act->str_values[0] );
    const int charges_consumed = actor->finish_using( *you, patient, *used_tool, healed );
    you->consume_charges( it, charges_consumed );

    // Erase activity and values.
    act->set_to_null();
    act->values.clear();
}

void activity_handlers::forage_finish( player_activity *act, Character *you )
{
    // Don't forage if we aren't next to the bush - otherwise we get weird bugs
    bool next_to_bush = false;
    map &here = get_map();
    for( const tripoint &pnt : here.points_in_radius( you->pos(), 1 ) ) {
        if( here.getabs( pnt ) == act->placement ) {
            next_to_bush = true;
            break;
        }
    }

    if( !next_to_bush ) {
        act->set_to_null();
        return;
    }

    const int veggy_chance = rng( 1, 100 );
    bool found_something = false;

    item_group_id group_id;
    ter_str_id next_ter;

    switch( season_of_year( calendar::turn ) ) {
        case SPRING:
            group_id = item_group_id( "forage_spring" );
            next_ter = ter_str_id( "t_underbrush_harvested_spring" );
            break;
        case SUMMER:
            group_id = item_group_id( "forage_summer" );
            next_ter = ter_str_id( "t_underbrush_harvested_summer" );
            break;
        case AUTUMN:
            group_id = item_group_id( "forage_autumn" );
            next_ter = ter_str_id( "t_underbrush_harvested_autumn" );
            break;
        case WINTER:
            group_id = item_group_id( "forage_winter" );
            next_ter = ter_str_id( "t_underbrush_harvested_winter" );
            break;
        default:
            debugmsg( "Invalid season" );
    }

    const tripoint bush_pos = here.getlocal( act->placement );
    here.ter_set( bush_pos, next_ter );

    // Survival gives a bigger boost, and Perception is leveled a bit.
    // Both survival and perception affect time to forage

    ///\EFFECT_PER slightly increases forage success chance
    ///\EFFECT_SURVIVAL increases forage success chance
    if( veggy_chance < you->get_skill_level( skill_survival ) * 3 + you->per_cur - 2 ) {
        const std::vector<item *> dropped =
            here.put_items_from_loc( group_id, you->pos(), calendar::turn );
        for( item *it : dropped ) {
            add_msg( m_good, _( "You found: %s!" ), it->tname() );
            found_something = true;
            if( it->has_flag( flag_FORAGE_POISON ) && one_in( 10 ) ) {
                it->set_flag( flag_HIDDEN_POISON );
                it->poison = rng( 2, 7 );
            }
            if( it->has_flag( flag_FORAGE_HALLU ) && !it->has_flag( flag_HIDDEN_POISON ) && one_in( 10 ) ) {
                it->set_flag( flag_HIDDEN_HALLU );
            }
        }
    }
    // 10% to drop a item/items from this group.
    if( one_in( 10 ) ) {
        const std::vector<item *> dropped =
            here.put_items_from_loc( item_group_id( "trash_forest" ), you->pos(), calendar::turn );
        for( item * const &it : dropped ) {
            add_msg( m_good, _( "You found: %s!" ), it->tname() );
            found_something = true;
        }
    }

    if( !found_something ) {
        add_msg( _( "You didn't find anything." ) );
    }

    iexamine::practice_survival_while_foraging( you );

    act->set_to_null();

    here.maybe_trigger_trap( bush_pos, *you, true );
}

void activity_handlers::generic_game_turn_handler( player_activity *act, Character *you,
        int morale_bonus, int morale_max_bonus )
{
    // Consume battery charges for every minute spent playing
    if( calendar::once_every( 1_minutes ) ) {
        if( !act->targets.empty() ) {
            item &game_item = *act->targets.front();
            bool fail = game_item.ammo_consume( game_item.ammo_required(), tripoint_zero, you ) == 0;
            if( fail ) {
                act->moves_left = 0;
                add_msg( m_info, _( "The %s runs out of batteries." ), game_item.tname() );
                return;
            }
        }
        //1 points/min, almost 2 hours to fill
        you->add_morale( MORALE_GAME, morale_bonus, morale_max_bonus );
    }
}

void activity_handlers::generic_game_do_turn( player_activity *act, Character *you )
{
    generic_game_turn_handler( act, you, 4, 60 );
}

void activity_handlers::game_do_turn( player_activity *act, Character *you )
{
    generic_game_turn_handler( act, you, 1, 100 );
}

void activity_handlers::longsalvage_finish( player_activity *act, Character *you )
{
    static const std::string salvage_string = "salvage";
    item &main_tool = you->i_at( act->index );
    map &here = get_map();
    map_stack items = here.i_at( you->pos() );
    item *salvage_tool = main_tool.get_usable_item( salvage_string );
    if( salvage_tool == nullptr ) {
        debugmsg( "Lost tool used for long salvage" );
        act->set_to_null();
        return;
    }

    const use_function *use_fun = salvage_tool->get_use( salvage_string );
    const salvage_actor *actor = dynamic_cast<const salvage_actor *>( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    for( item &it : items ) {
        if( actor->valid_to_cut_up( it ) ) {
            item_location item_loc( map_cursor( you->pos() ), &it );
            actor->cut_up( *you, *salvage_tool, item_loc );
            return;
        }
    }

    add_msg( _( "You finish salvaging." ) );
    act->set_to_null();
}

void activity_handlers::pickaxe_do_turn( player_activity *act, Character * )
{
    const tripoint &pos = get_map().getlocal( act->placement );
    sfx::play_activity_sound( "tool", "pickaxe", sfx::get_heard_volume( pos ) );
    // each turn is too much
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a Pickaxe at work!
        sounds::sound( pos, 30, sounds::sound_t::destructive_activity, _( "CHNK!  CHNK!  CHNK!" ) );
    }
}

void activity_handlers::pickaxe_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    const tripoint pos( here.getlocal( act->placement ) );
    // Invalidate the activity early to prevent a query from mod_pain()
    act->set_to_null();
    if( you->is_avatar() ) {
        const int helpersize = get_player_character().get_num_crafting_helpers( 3 );
        if( here.is_bashable( pos ) && here.has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, pos ) &&
            here.ter( pos ) != t_tree ) {
            // Tunneling through solid rock is sweaty, backbreaking work
            // Betcha wish you'd opted for the J-Hammer
            if( you->has_trait( trait_STOCKY_TROGLO ) ) {
                you->mod_pain( std::max( 0, ( 1 * static_cast<int>( rng( 0, 3 ) ) ) - helpersize ) );
            } else {
                you->mod_pain( std::max( 0, ( 2 * static_cast<int>( rng( 1, 3 ) ) ) - helpersize ) );
            }
        }
    }
    you->add_msg_player_or_npc( m_good,
                                _( "You finish digging." ),
                                _( "<npcname> finishes digging." ) );
    here.destroy( pos, true );
    if( !act->targets.empty() ) {
        item &it = *act->targets.front();
        you->consume_charges( it, it.ammo_required() );
    } else {
        debugmsg( "pickaxe activity targets empty" );
    }
    if( resume_for_multi_activities( *you ) ) {
        for( item &elem : here.i_at( pos ) ) {
            elem.set_var( "activity_var", you->name );
        }
    }
}

void activity_handlers::pulp_do_turn( player_activity *act, Character *you )
{
    map &here = get_map();
    const tripoint &pos = here.getlocal( act->placement );

    // Stabbing weapons are a lot less effective at pulping
    const int cut_power = std::max( you->weapon.damage_melee( damage_type::CUT ),
                                    you->weapon.damage_melee( damage_type::STAB ) / 2 );

    ///\EFFECT_STR increases pulping power, with diminishing returns
    float pulp_power = std::sqrt( ( you->str_cur + you->weapon.damage_melee( damage_type::BASH ) ) *
                                  ( cut_power + 1.0f ) );
    float pulp_effort = you->str_cur + you->weapon.damage_melee( damage_type::BASH );
    // Multiplier to get the chance right + some bonus for survival skill
    pulp_power *= 40 + you->get_skill_level( skill_survival ) * 5;

    const int mess_radius = you->weapon.has_flag( flag_MESSY ) ? 2 : 1;

    int moves = 0;
    // use this to collect how many corpse are pulped
    int &num_corpses = act->index;
    map_stack corpse_pile = here.i_at( pos );
    for( item &corpse : corpse_pile ) {
        const mtype *corpse_mtype = corpse.get_mtype();
        if( !corpse.is_corpse() || !corpse.can_revive() ||
            ( std::find( act->str_values.begin(), act->str_values.end(), "auto_pulp_no_acid" ) !=
              act->str_values.end() && corpse_mtype->bloodType().obj().has_acid ) ) {
            // Don't smash non-rezing corpses //don't smash acid zombies when auto pulping
            continue;
        }

        while( corpse.damage() < corpse.max_damage() ) {
            // Increase damage as we keep smashing ensuring we eventually smash the target.
            if( x_in_y( pulp_power, corpse.volume() / units::legacy_volume_factor ) ) {
                corpse.inc_damage( damage_type::BASH );
                if( corpse.damage() == corpse.max_damage() ) {
                    num_corpses++;
                }
            }

            if( x_in_y( pulp_power, corpse.volume() / units::legacy_volume_factor ) ) {
                // Splatter some blood around
                // Splatter a bit more randomly, so that it looks cooler
                const int radius = mess_radius + x_in_y( pulp_power, 500 ) + x_in_y( pulp_power, 1000 );
                const tripoint dest( pos + point( rng( -radius, radius ), rng( -radius, radius ) ) );
                const field_type_id type_blood = ( mess_radius > 1 && x_in_y( pulp_power, 10000 ) ) ?
                                                 corpse.get_mtype()->gibType() :
                                                 corpse.get_mtype()->bloodType();
                here.add_splatter_trail( type_blood, pos, dest );
            }

            you->mod_stamina( -pulp_effort );

            if( one_in( 4 ) ) {
                // Smashing may not be butchery, but it involves some zombie anatomy
                you->practice( skill_survival, 2, 2 );
            }

            float stamina_ratio = static_cast<float>( you->get_stamina() ) / you->get_stamina_max();
            moves += 100 / std::max( 0.25f,
                                     stamina_ratio ) * you->exertion_adjusted_move_multiplier( act->exertion_level() );
            if( stamina_ratio < 0.33 || you->is_npc() ) {
                you->moves = std::min( 0, you->moves - moves );
                return;
            }
            if( moves >= you->moves ) {
                // Enough for this turn;
                you->moves -= moves;
                return;
            }
        }
        corpse.set_flag( flag_PULPED );
    }
    // If we reach this, all corpses have been pulped, finish the activity
    act->moves_left = 0;
    if( num_corpses == 0 ) {
        you->add_msg_if_player( m_bad, _( "The corpse moved before you could finish smashing it!" ) );
        return;
    }
    // TODO: Factor in how long it took to do the smashing.
    you->add_msg_player_or_npc( ngettext( "The corpse is thoroughly pulped.",
                                          "The corpses are thoroughly pulped.", num_corpses ),
                                ngettext( "<npcname> finished pulping the corpse.",
                                          "<npcname> finished pulping the corpses.", num_corpses ) );
}

void activity_handlers::pulp_finish( player_activity *act, Character *you )
{
    if( you->is_npc() ) {
        npc *guy = dynamic_cast<npc *>( you );
        guy->revert_after_activity();
    } else {
        act->set_to_null();
    }
}

void activity_handlers::start_fire_finish( player_activity *act, Character *you )
{
    static const std::string iuse_name_string( "firestarter" );

    item &it = *act->targets.front();
    item *used_tool = it.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for starting fire" );
        act->set_to_null();
        return;
    }

    const use_function *use_fun = used_tool->get_use( iuse_name_string );
    const firestarter_actor *actor = dynamic_cast<const firestarter_actor *>
                                     ( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    it.ammo_consume( it.type->charges_to_use(), tripoint_zero, you );

    you->practice( skill_survival, act->index, 5 );

    firestarter_actor::resolve_firestarter_use( *you, act->placement );
    act->set_to_null();
}

void activity_handlers::start_fire_do_turn( player_activity *act, Character *you )
{
    map &here = get_map();
    if( !here.is_flammable( act->placement ) ) {
        try_fuel_fire( *act, *you, true );
        if( !here.is_flammable( act->placement ) ) {
            you->add_msg_if_player( m_info, _( "There's nothing to light there." ) );
            you->cancel_activity();
            return;
        }
    }

    // Sometimes when an item is dropped it causes the whole item* to get set to null.
    // This null pointer gets cast to a reference at some point, causing UB and
    // segfaults. It looks like something is supposed to catch this, maybe
    // get_safe_reference in item.cpp, but it's not working so we check for a
    // null pointer here.
    //
    if( act->targets.front().get_item() == nullptr ) {
        add_msg( m_bad, _( "You have lost the item you were using to start the fire." ) );
        you->cancel_activity();
        return;
    }

    item &firestarter = *act->targets.front();
    if( firestarter.has_flag( flag_REQUIRES_TINDER ) ) {
        if( !here.tinder_at( act->placement ) ) {
            you->add_msg_if_player( m_info, _( "This item requires tinder to light." ) );
            you->cancel_activity();
            return;
        }
    }

    const use_function *usef = firestarter.type->get_use( "firestarter" );
    if( usef == nullptr || usef->get_actor_ptr() == nullptr ) {
        add_msg( m_bad, _( "You have lost the item you were using to start the fire." ) );
        you->cancel_activity();
        return;
    }

    you->mod_moves( -you->moves );
    const firestarter_actor *actor = dynamic_cast<const firestarter_actor *>( usef->get_actor_ptr() );
    const float light = actor->light_mod( you->pos() );
    act->moves_left -= light * 100;
    if( light < 0.1 ) {
        add_msg( m_bad, _( "There is not enough sunlight to start a fire now.  You stop trying." ) );
        you->cancel_activity();
    }
}

static bool magic_train( player_activity *act, Character *you )
{
    if( !you ) {
        return false;
    }
    const spell_id &sp_id = spell_id( act->name );
    if( sp_id.is_valid() ) {
        const bool knows = get_player_character().magic->knows_spell( sp_id );
        if( knows ) {
            spell &studying = you->magic->get_spell( sp_id );
            const int expert_multiplier = act->values.empty() ? 0 : act->values[0];
            const int xp = roll_remainder( studying.exp_modifier( *you ) * expert_multiplier );
            studying.gain_exp( xp );
            you->add_msg_if_player( m_good, _( "You learn a little about the spell: %s" ),
                                    sp_id->name );
        } else {
            you->magic->learn_spell( act->name, *you );
            // you can decline to learn this spell , as it may lock you out of other magic.
            if( you->magic->knows_spell( sp_id ) ) {
                add_msg( m_good, _( "You learn %s." ), sp_id->name.translated() );
            } else {
                act->set_to_null();
            }
        }
        return true;
    }
    return false;
}

void activity_handlers::train_finish( player_activity *act, Character *you )
{
    const skill_id sk( act->name );
    if( sk.is_valid() ) {
        const Skill &skill = sk.obj();
        std::string skill_name = skill.name();
        int old_skill_level = you->get_knowledge_level( sk );
        you->practice( sk, 100, old_skill_level + 2 );
        int new_skill_level = you->get_knowledge_level( sk );
        if( old_skill_level != new_skill_level ) {
            add_msg( m_good, _( "You finish training %s to level %d." ),
                     skill_name, new_skill_level );
            get_event_bus().send<event_type::gains_skill_level>( you->getID(), sk, new_skill_level );
        } else {
            add_msg( m_good, _( "You get some training in %s." ), skill_name );
        }
        act->set_to_null();
        return;
    }

    const proficiency_id &proficiency = proficiency_id( act->name );
    if( proficiency.is_valid() ) {
        you->practice_proficiency( proficiency, 15_minutes );
        add_msg( m_good, _( "You get some training in %s." ), proficiency->name() );
        act->set_to_null();
        return;
    }

    const matype_id &ma_id = matype_id( act->name );
    if( ma_id.is_valid() ) {
        const martialart &mastyle = ma_id.obj();
        // Trained martial arts,
        get_event_bus().send<event_type::learns_martial_art>( you->getID(), ma_id );
        you->martial_arts_data->learn_style( mastyle.id, you->is_avatar() );
    } else if( !magic_train( act, you ) ) {
        debugmsg( "train_finish without a valid skill or style or spell name" );
    }

    act->set_to_null();
}

void activity_handlers::vehicle_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    //Grab this now, in case the vehicle gets shifted
    const optional_vpart_position vp = here.veh_at( here.getlocal( tripoint( act->values[0],
                                       act->values[1],
                                       you->posz() ) ) );
    veh_interact::complete_vehicle( *you );
    // complete_vehicle set activity type to NULL if the vehicle
    // was completely dismantled, otherwise the vehicle still exist and
    // is to be examined again.
    if( act->is_null() ) {
        if( npc *guy = dynamic_cast<npc *>( you ) ) {
            guy->revert_after_activity();
            guy->set_moves( 0 );
        }
        return;
    }
    act->set_to_null();
    if( !you->is_npc() ) {
        if( act->values.size() < 7 ) {
            dbg( D_ERROR ) << "game:process_activity: invalid ACT_VEHICLE values: "
                           << act->values.size();
            debugmsg( "process_activity invalid ACT_VEHICLE values:%d",
                      act->values.size() );
        } else {
            if( vp ) {
                here.invalidate_map_cache( here.get_abs_sub().z );
                // TODO: Z (and also where the activity is queued)
                // Or not, because the vehicle coordinates are dropped anyway
                if( !resume_for_multi_activities( *you ) ) {
                    g->exam_vehicle( vp->vehicle(), point( act->values[ 2 ], act->values[ 3 ] ) );
                }
                return;
            } else {
                dbg( D_ERROR ) << "game:process_activity: ACT_VEHICLE: vehicle not found";
                debugmsg( "process_activity ACT_VEHICLE: vehicle not found" );
            }
        }
    }
}

void activity_handlers::hand_crank_do_turn( player_activity *act, Character *you )
{
    // Hand-crank chargers seem to range from 2 watt (very common easily verified)
    // to 10 watt (suspicious claims from some manufacturers) sustained output.
    // It takes 2.4 minutes to produce 1kj at just slightly under 7 watts (25 kj per hour)
    // time-based instead of speed based because it's a sustained activity
    item &hand_crank_item = *act->targets.front();

    int time_to_crank = to_seconds<int>( 144_seconds );
    // Modify for weariness
    time_to_crank /= you->exertion_adjusted_move_multiplier( act->exertion_level() );
    if( calendar::once_every( time_duration::from_seconds( time_to_crank ) ) ) {
        if( hand_crank_item.ammo_capacity( ammotype( "battery" ) ) > hand_crank_item.ammo_remaining() ) {
            hand_crank_item.ammo_set( itype_id( "battery" ), hand_crank_item.ammo_remaining() + 1 );
        } else {
            act->moves_left = 0;
            add_msg( m_info, _( "You've charged the battery completely." ) );
        }
    }
    if( you->get_fatigue() >= fatigue_levels::DEAD_TIRED ) {
        act->moves_left = 0;
        add_msg( m_info, _( "You're too exhausted to keep cranking." ) );
    }

}

void activity_handlers::vibe_do_turn( player_activity *act, Character *you )
{
    //Using a vibrator takes time (10 minutes), not speed
    //Linear increase in morale during action with a small boost at end
    //Deduct 1 battery charge for every minute in use, or vibrator is much less effective
    item &vibrator_item = *act->targets.front();

    if( you->encumb( bodypart_id( "mouth" ) ) >= 30 ) {
        act->moves_left = 0;
        add_msg( m_bad, _( "You have trouble breathing, and stop." ) );
    }

    if( calendar::once_every( 1_minutes ) ) {
        if( vibrator_item.ammo_remaining( you ) > 0 ) {
            vibrator_item.ammo_consume( 1, you->pos(), you );
            you->add_morale( MORALE_FEELING_GOOD, 3, 40 );
            if( vibrator_item.ammo_remaining( you ) == 0 ) {
                add_msg( m_info, _( "The %s runs out of batteries." ), vibrator_item.tname() );
            }
        } else {
            //twenty minutes to fill
            you->add_morale( MORALE_FEELING_GOOD, 1, 40 );
        }
    }
    // Dead Tired: different kind of relaxation needed
    if( you->get_fatigue() >= fatigue_levels::DEAD_TIRED ) {
        act->moves_left = 0;
        add_msg( m_info, _( "You're too tired to continue." ) );
    }

    // Vibrator requires that you be able to move around, stretch, etc, so doesn't play
    // well with roots.  Sorry.  :-(
}

void activity_handlers::start_engines_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    // Find the vehicle by looking for a remote vehicle first, then by player relative coordinates
    vehicle *veh = g->remoteveh();
    map &here = get_map();
    if( !veh ) {
        const tripoint pos = act->placement + get_player_character().pos();
        veh = veh_pointer_or_null( here.veh_at( pos ) );
        if( !veh ) {
            return;
        }
    }

    int attempted = 0;
    int non_muscle_attempted = 0;
    int started = 0;
    int non_muscle_started = 0;
    int non_combustion_started = 0;
    const bool take_control = act->values[0];

    for( size_t e = 0; e < veh->engines.size(); ++e ) {
        if( veh->is_engine_on( e ) ) {
            attempted++;
            if( !veh->is_engine_type( e, itype_muscle ) &&
                !veh->is_engine_type( e, itype_animal ) ) {
                non_muscle_attempted++;
            }
            if( veh->start_engine( e ) ) {
                started++;
                if( !veh->is_engine_type( e, itype_muscle ) &&
                    !veh->is_engine_type( e, itype_animal ) ) {
                    non_muscle_started++;
                } else {
                    non_combustion_started++;
                }
            }
        }
    }

    //Did any engines start?
    veh->engine_on = started;
    //init working engine noise
    sfx::do_vehicle_engine_sfx();

    if( attempted == 0 ) {
        add_msg( m_info, _( "The %s doesn't have an engine!" ), veh->name );
    } else if( non_muscle_attempted > 0 ) {
        //Some non-muscle engines tried to start
        if( non_muscle_attempted == non_muscle_started ) {
            //All of the non-muscle engines started
            add_msg( ngettext( "The %s's engine starts up.",
                               "The %s's engines start up.", non_muscle_started ), veh->name );
        } else if( non_muscle_started > 0 ) {
            //Only some of the non-muscle engines started
            add_msg( ngettext( "One of the %s's engines start up.",
                               "Some of the %s's engines start up.", non_muscle_started ), veh->name );
        } else if( non_combustion_started > 0 ) {
            //Non-combustions "engines" started
            add_msg( _( "The %s is ready for movement." ), veh->name );
        } else {
            //All of the non-muscle engines failed
            add_msg( m_bad, ngettext( "The %s's engine fails to start.",
                                      "The %s's engines fail to start.", non_muscle_attempted ), veh->name );
        }
    }

    if( take_control && !veh->engine_on && !veh->velocity ) {
        you->controlling_vehicle = false;
        add_msg( _( "You let go of the controls." ) );
    }
}

enum class repeat_type : int {
    // INIT should be zero. In some scenarios (vehicle welder), activity value default to zero.
    INIT = 0,    // Haven't found repeat value yet.
    ONCE,        // Repeat just once
    FOREVER,     // Repeat for as long as possible
    FULL,        // Repeat until damage==0
    EVENT,       // Repeat until something interesting happens
    CANCEL,      // Stop repeating
};

using I = std::underlying_type_t<repeat_type>;
static constexpr bool operator>=( const I &lhs, const repeat_type &rhs )
{
    return lhs >= static_cast<I>( rhs );
}

static constexpr bool operator<=( const I &lhs, const repeat_type &rhs )
{
    return lhs <= static_cast<I>( rhs );
}

static constexpr I operator-( const repeat_type &lhs, const repeat_type &rhs )
{
    return static_cast<I>( lhs ) - static_cast<I>( rhs );
}

static repeat_type repeat_menu( const std::string &title, repeat_type last_selection )
{
    uilist rmenu;
    rmenu.text = title;

    rmenu.addentry( static_cast<int>( repeat_type::ONCE ), true, '1', _( "Repeat once" ) );
    rmenu.addentry( static_cast<int>( repeat_type::FOREVER ), true, '2',
                    _( "Repeat until reinforced" ) );
    rmenu.addentry( static_cast<int>( repeat_type::FULL ), true, '3',
                    _( "Repeat until fully repaired, but don't reinforce" ) );
    rmenu.addentry( static_cast<int>( repeat_type::EVENT ), true, '4',
                    _( "Repeat until success/failure/level up" ) );
    rmenu.addentry( static_cast<int>( repeat_type::INIT ), true, '5', _( "Back to item selection" ) );

    rmenu.selected = last_selection - repeat_type::ONCE;
    rmenu.query();

    if( rmenu.ret >= repeat_type::INIT && rmenu.ret <= repeat_type::EVENT ) {
        return static_cast<repeat_type>( rmenu.ret );
    }

    return repeat_type::CANCEL;
}

// HACK: This is a part of a hack to provide pseudo items for long repair activity
// Note: similar hack could be used to implement all sorts of vehicle pseudo-items
//  and possibly CBM pseudo-items too.
struct weldrig_hack {
    cata::optional<vpart_reference> part;
    item pseudo;

    weldrig_hack() : part( cata::nullopt ) { }

    bool init( const player_activity &act ) {
        if( act.coords.empty() || act.str_values.size() < 2 ) {
            return false;
        }

        const optional_vpart_position vp = get_map().veh_at( act.coords[0] );
        if( !vp ) {
            return false;
        }

        itype_id tool_id( act.get_str_value( 1, "" ) );
        pseudo = item( tool_id, calendar::turn );
        part = vp->part_with_tool( tool_id );
        return part.has_value();
    }

    item &get_item() {
        if( !part ) {
            // null item should be handled just fine
            return null_item_reference();
        }

        item pseudo_magazine( pseudo.magazine_default() );
        pseudo.put_in( pseudo_magazine, item_pocket::pocket_type::MAGAZINE_WELL );
        pseudo.ammo_set( itype_battery, part->vehicle().drain( itype_battery,
                         pseudo.ammo_capacity( ammotype( "battery" ) ) ) );
        return pseudo;
    }

    void clean_up() {
        if( !part ) {
            return;
        }

        part->vehicle().charge_battery( pseudo.ammo_remaining() ); // return unused charges
    }

    ~weldrig_hack() {
        clean_up();
    }
};

void activity_handlers::repair_item_finish( player_activity *act, Character *you )
{
    const std::string iuse_name_string = act->get_str_value( 0, "repair_item" );
    repeat_type repeat = static_cast<repeat_type>( act->get_value( 0,
                         static_cast<int>( repeat_type::INIT ) ) );
    weldrig_hack w_hack;
    item_location *ploc = nullptr;

    if( !act->targets.empty() ) {
        ploc = &act->targets[0];
    }

    item &main_tool = !w_hack.init( *act ) ?
                      ploc ?
                      **ploc : you->i_at( act->index ) : w_hack.get_item();

    item *used_tool = main_tool.get_usable_item( iuse_name_string );
    if( used_tool == nullptr ) {
        debugmsg( "Lost tool used for long repair" );
        act->set_to_null();
        return;
    }

    const use_function *use_fun = used_tool->get_use( iuse_name_string );
    // TODO: De-uglify this block. Something like get_use<iuse_actor_type>() maybe?
    const repair_item_actor *actor = dynamic_cast<const repair_item_actor *>
                                     ( use_fun->get_actor_ptr() );
    if( actor == nullptr ) {
        debugmsg( "iuse_actor type descriptor and actual type mismatch" );
        act->set_to_null();
        return;
    }

    // Valid Repeat choice and target, attempt repair.
    if( repeat != repeat_type::INIT && act->targets.size() >= 2 ) {
        item_location &fix_location = act->targets[1];

        // Remember our level: we want to stop retrying on level up
        const int old_level = you->get_skill_level( actor->used_skill );
        const repair_item_actor::attempt_hint attempt = actor->repair( *you, *used_tool,
                fix_location );
        if( attempt != repair_item_actor::AS_CANT ) {
            if( ploc && ploc->where() == item_location::type::map ) {
                used_tool->ammo_consume( used_tool->ammo_required(), ploc->position(), you );
            } else {
                you->consume_charges( *used_tool, used_tool->ammo_required() );
            }
        }

        // TODO: Allow setting this in the actor
        // TODO: Don't use charges_to_use: welder has 50 charges per use, soldering iron has 1
        if( !used_tool->ammo_sufficient( you ) ) {
            you->add_msg_if_player( _( "Your %s ran out of charges." ), used_tool->tname() );
            act->set_to_null();
            return;
        }

        // Print message explaining why we stopped
        // But only if we didn't destroy the item (because then it's obvious)
        const bool destroyed = attempt == repair_item_actor::AS_DESTROYED;
        const bool cannot_continue_repair = attempt == repair_item_actor::AS_CANT ||
                                            destroyed || !actor->can_repair_target( *you, *fix_location, !destroyed );
        if( cannot_continue_repair ) {
            // Cannot continue to repair target, select another target.
            // **Warning**: as soon as the item is popped back, it is destroyed and can't be used anymore!
            act->targets.pop_back();
        }

        const bool event_happened = attempt == repair_item_actor::AS_FAILURE ||
                                    attempt == repair_item_actor::AS_SUCCESS ||
                                    old_level != you->get_skill_level( actor->used_skill );

        const bool need_input =
            ( repeat == repeat_type::ONCE ) ||
            ( repeat == repeat_type::EVENT && event_happened ) ||
            ( repeat == repeat_type::FULL && ( cannot_continue_repair || fix_location->damage() <= 0 ) );
        if( need_input ) {
            repeat = repeat_type::INIT;
        }
    }
    // Check tool is valid before we query target and Repeat choice.
    if( !actor->can_use_tool( *you, *used_tool, true ) ) {
        act->set_to_null();
        return;
    }

    // target selection and validation.
    while( act->targets.size() < 2 ) {
        item_location item_loc = game_menus::inv::repair( *you, actor, &main_tool );

        if( item_loc == item_location::nowhere ) {
            you->add_msg_if_player( m_info, _( "Never mind." ) );
            act->set_to_null();
            return;
        }
        if( actor->can_repair_target( *you, *item_loc, true ) ) {
            act->targets.emplace_back( item_loc );
            repeat = repeat_type::INIT;
        }
    }

    const item &fix = *act->targets[1];
    if( repeat == repeat_type::INIT ) {
        const int level = you->get_skill_level( actor->used_skill );
        repair_item_actor::repair_type action_type = actor->default_action( fix, level );
        if( action_type == repair_item_actor::RT_NOTHING ) {
            you->add_msg_if_player( _( "You won't learn anything more by doing that." ) );
        }

        const std::pair<float, float> chance = actor->repair_chance( *you, fix, action_type );
        if( chance.first <= 0.0f ) {
            action_type = repair_item_actor::RT_PRACTICE;
        }

        std::string title = string_format( _( "%s %s\n" ),
                                           repair_item_actor::action_description( action_type ),
                                           fix.tname() );
        ammotype current_ammo;
        std::string ammo_name;
        if( used_tool->has_flag( flag_USES_BIONIC_POWER ) ) {
            ammo_name = _( "bionic power" );

        } else {
            if( used_tool->ammo_current().is_null() ) {
                current_ammo = item_controller->find_template( used_tool->ammo_default() )->ammo->type;
            } else {
                current_ammo = item_controller->find_template( used_tool->ammo_current() )->ammo->type;
            }

            ammo_name = item::nname( used_tool->ammo_current() );
        }

        int ammo_remaining = used_tool->ammo_remaining( you );

        std::set<itype_id> valid_entries = actor->get_valid_repair_materials( fix );
        const inventory &crafting_inv = you->crafting_inventory();
        std::function<bool( const item & )> filter;
        if( fix.is_filthy() ) {
            filter = []( const item & component ) {
                return component.allow_crafting_component();
            };
        } else {
            filter = is_crafting_component;
        }
        std::vector<std::string> material_list;
        for( const auto &component_id : valid_entries ) {
            if( item::count_by_charges( component_id ) ) {
                if( crafting_inv.has_charges( component_id, 1 ) ) {
                    material_list.push_back( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                            crafting_inv.charges_of( component_id ) ) );
                }
            } else if( crafting_inv.has_amount( component_id, 1, false, filter ) ) {
                material_list.push_back( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                        crafting_inv.amount_of( component_id, false ) ) );
            }
        }

        title += string_format( _( "Charges: <color_light_blue>%s/%s</color> %s (%s per use)\n" ),
                                ammo_remaining, used_tool->ammo_capacity( current_ammo ),
                                ammo_name,
                                used_tool->ammo_required() );
        title += string_format( _( "Materials available: %s\n" ), join( material_list, ", " ) );
        title += string_format( _( "Skill used: <color_light_blue>%s (%s)</color>\n" ),
                                actor->used_skill.obj().name(), level );
        title += string_format( _( "Success chance: <color_light_blue>%.1f</color>%%\n" ),
                                100.0f * chance.first );
        title += string_format( _( "Damage chance: <color_light_blue>%.1f</color>%%" ),
                                100.0f * chance.second );

        if( act->values.empty() ) {
            act->values.resize( 1 );
        }
        do {
            repeat = repeat_menu( title, repeat );

            if( repeat == repeat_type::CANCEL ) {
                act->set_to_null();
                return;
            }
            act->values[0] = static_cast<int>( repeat );
            // BACK selected, redo target selection next.
            if( repeat == repeat_type::INIT ) {
                you->activity.targets.pop_back();
                return;
            }
            if( repeat == repeat_type::FULL && fix.damage() <= 0 ) {
                you->add_msg_if_player( m_info, _( "Your %s is already fully repaired." ), fix.tname() );
                repeat = repeat_type::INIT;
            }
        } while( repeat == repeat_type::INIT );
    }
    // Otherwise keep retrying
    act->moves_left = actor->move_cost;
}

void activity_handlers::heat_item_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    if( act->targets.size() != 1 ) {
        debugmsg( "invalid arguments to ACT_HEATING" );
        return;
    }
    item_location &loc = act->targets[ 0 ];
    item *const heat = loc.get_item();
    if( heat == nullptr ) {
        return;
    }
    item *const food = heat->get_food();
    if( food == nullptr ) {
        debugmsg( "item %s is not food", heat->typeId().str() );
        return;
    }
    item &target = *food;
    if( target.has_own_flag( flag_FROZEN ) ) {
        target.apply_freezerburn();
        if( target.has_flag( flag_EATEN_COLD ) ) {
            target.cold_up();
            you->add_msg_if_player( m_info,
                                    _( "You defrost the food, but don't heat it up, since you enjoy it cold." ) );
        } else {
            target.heat_up();
            you->add_msg_if_player( m_info, _( "You defrost and heat up the food." ) );
        }
    } else {
        target.heat_up();
        you->add_msg_if_player( m_info, _( "You heat up the food." ) );
    }
}

void activity_handlers::mend_item_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    if( act->targets.size() != 1 ) {
        debugmsg( "invalid arguments to ACT_MEND_ITEM" );
        return;
    }

    item_location &target = act->targets[ 0 ];

    const auto f = target->faults.find( fault_id( act->name ) );
    if( f == target->faults.end() ) {
        debugmsg( "item %s does not have fault %s", target->tname(), act->name );
        return;
    }

    if( act->str_values.empty() ) {
        debugmsg( "missing mending_method id for ACT_MEND_ITEM." );
        return;
    }

    const mending_method *method = fault_id( act->name )->find_mending_method( act->str_values[0] );
    if( !method ) {
        debugmsg( "invalid mending_method id for ACT_MEND_ITEM." );
        return;
    }

    const inventory inv = you->crafting_inventory();
    const requirement_data &reqs = method->requirements.obj();
    if( !reqs.can_make_with_inventory( inv, is_crafting_component ) ) {
        add_msg( m_info, _( "You are currently unable to mend the %s." ), target->tname() );
    }
    for( const auto &e : reqs.get_components() ) {
        you->consume_items( e );
    }
    for( const auto &e : reqs.get_tools() ) {
        you->consume_tools( e );
    }
    you->invalidate_crafting_inventory();

    target->faults.erase( *f );
    if( method->turns_into ) {
        target->faults.emplace( *method->turns_into );
    }
    // also_mends removes not just the fault picked to be mended, but this as well.
    if( method->also_mends ) {
        target->faults.erase( *method->also_mends );
    }
    if( act->name == "fault_gun_blackpowder" || act->name == "fault_gun_dirt" ) {
        target->set_var( "dirt", 0 );
    }

    //get skill list from mending method, iterate through and give xp
    for( const std::pair<const skill_id, int> &e : method->skills ) {
        you->practice( e.first, 10, static_cast<int>( e.second * 1.25 ) );
    }

    add_msg( m_good, method->success_msg.translated(), target->tname() );
}

void activity_handlers::gunmod_add_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    // first unpack all of our arguments
    if( act->values.size() != 4 ) {
        debugmsg( "Insufficient arguments to ACT_GUNMOD_ADD" );
        return;
    }

    item &gun = *act->targets.at( 0 );
    item &mod = *act->targets.at( 1 );

    // chance of success (%)
    const int roll = act->values[1];
    // chance of damage (%)
    const int risk = act->values[2];

    // any tool charges used during installation
    const itype_id tool( act->name );
    const int qty = act->values[3];

    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Invalid arguments in ACT_GUNMOD_ADD" );
        return;
    }

    if( !tool.is_empty() && qty > 0 ) {
        you->use_charges( tool, qty );
    }

    if( rng( 0, 100 ) <= roll ) {
        add_msg( m_good, _( "You successfully attached the %1$s to your %2$s." ), mod.tname(),
                 gun.tname() );
        gun.put_in( you->i_rem( &mod ), item_pocket::pocket_type::MOD );

    } else if( rng( 0, 100 ) <= risk ) {
        if( gun.inc_damage() ) {
            // Remove irremovable mods prior to destroying the gun
            for( item *mod : gun.gunmods() ) {
                if( mod->is_irremovable() ) {
                    you->remove_item( *mod );
                }
            }
            add_msg( m_bad, _( "You failed at installing the %s and destroyed your %s!" ), mod.tname(),
                     gun.tname() );
            you->i_rem( &gun );
        } else {
            add_msg( m_bad, _( "You failed at installing the %s and damaged your %s!" ), mod.tname(),
                     gun.tname() );
        }

    } else {
        add_msg( m_info, _( "You failed at installing the %s." ), mod.tname() );
    }
}

void activity_handlers::toolmod_add_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    if( act->targets.size() != 2 || !act->targets[0] || !act->targets[1] ) {
        debugmsg( "Incompatible arguments to ACT_TOOLMOD_ADD" );
        return;
    }
    item &tool = *act->targets[0];
    item &mod = *act->targets[1];
    you->add_msg_if_player( m_good, _( "You successfully attached the %1$s to your %2$s." ),
                            mod.tname(), tool.tname() );
    mod.set_flag( flag_IRREMOVABLE );
    tool.put_in( mod, item_pocket::pocket_type::MOD );
    act->targets[1].remove_item();
}

void activity_handlers::clear_rubble_finish( player_activity *act, Character *you )
{
    const tripoint &pos = act->placement;
    map &here = get_map();
    you->add_msg_if_player( m_info, _( "You clear up the %s." ),
                            here.furnname( pos ) );
    here.furn_set( pos, f_null );

    act->set_to_null();

    here.maybe_trigger_trap( pos, *you, true );
}

void activity_handlers::wear_do_turn( player_activity *act, Character *you )
{
    activity_on_turn_wear( *act, *you );
}

// This activity opens the menu (it's not meant to queue consumption of items)
void activity_handlers::eat_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    avatar_action::eat( player_character, game_menus::inv::consume( player_character ) );
}

void activity_handlers::consume_food_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    avatar_action::eat( player_character, game_menus::inv::consume_food( player_character ) );
}

void activity_handlers::consume_drink_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    avatar_action::eat( player_character, game_menus::inv::consume_drink( player_character ) );
}

void activity_handlers::consume_meds_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    avatar_action::eat( player_character, game_menus::inv::consume_meds( player_character ) );
}

void activity_handlers::consume_fuel_menu_do_turn( player_activity *, Character * )
{
    avatar &player_character = get_avatar();
    avatar_action::eat( player_character, game_menus::inv::consume_fuel( player_character ), true );
}

void activity_handlers::move_loot_do_turn( player_activity *act, Character *you )
{
    activity_on_turn_move_loot( *act, *you );
}

void activity_handlers::adv_inventory_do_turn( player_activity *, Character *you )
{
    you->cancel_activity();
    create_advanced_inv();
}

void activity_handlers::travel_do_turn( player_activity *act, Character *you )
{
    if( !you->omt_path.empty() ) {
        you->omt_path.pop_back();
        if( you->omt_path.empty() ) {
            you->add_msg_if_player( m_info, _( "You have reached your destination." ) );
            act->set_to_null();
            return;
        }
        const tripoint_abs_omt next_omt = you->omt_path.back();
        tripoint_abs_ms waypoint;
        if( you->omt_path.size() == 1 ) {
            // if next omt is the final one, target its midpoint
            waypoint = midpoint( project_bounds<coords::ms>( next_omt ) );
        } else {
            // otherwise target the middle of the edge nearest to our current location
            const tripoint_abs_ms cur_omt_mid = midpoint( project_bounds<coords::ms>
                                                ( you->global_omt_location() ) );
            waypoint = clamp( cur_omt_mid, project_bounds<coords::ms>( next_omt ) );
        }
        map &here = get_map();
        // TODO: fix point types
        tripoint centre_sub = here.getlocal( waypoint.raw() );
        if( !here.passable( centre_sub ) ) {
            tripoint_range<tripoint> candidates = here.points_in_radius( centre_sub, 2 );
            for( const tripoint &elem : candidates ) {
                if( here.passable( elem ) ) {
                    centre_sub = elem;
                    break;
                }
            }
        }
        const std::vector<tripoint> route_to = here.route( you->pos(), centre_sub,
                                               you->get_pathfinding_settings(),
                                               you->get_path_avoid() );
        if( !route_to.empty() ) {
            const activity_id act_travel = ACT_TRAVELLING;
            you->set_destination( route_to, player_activity( act_travel ) );
        } else {
            you->add_msg_if_player( _( "You cannot reach that destination" ) );
        }
    } else {
        you->add_msg_if_player( m_info, _( "You have reached your destination." ) );
    }
    act->set_to_null();
}

void activity_handlers::armor_layers_do_turn( player_activity *, Character *you )
{
    you->cancel_activity();
    you->sort_armor();
}

void activity_handlers::atm_do_turn( player_activity *, Character *you )
{
    iexamine::atm( *you, you->pos() );
}

// fish-with-rod fish catching function.
static void rod_fish( Character *you, const std::vector<monster *> &fishables )
{
    map &here = get_map();
    //if the vector is empty (no fish around) the player is still given a small chance to get a (let us say it was hidden) fish
    if( fishables.empty() ) {
        const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup(
                    mongroup_id( "GROUP_FISH" ) );
        const mtype_id fish_mon = random_entry_ref( fish_group );
        here.add_item_or_charges( you->pos(), item::make_corpse( fish_mon, calendar::turn + rng( 0_turns,
                                  3_hours ) ) );
        you->add_msg_if_player( m_good, _( "You caught a %s." ), fish_mon.obj().nname() );
    } else {
        monster *chosen_fish = random_entry( fishables );
        chosen_fish->fish_population -= 1;
        if( chosen_fish->fish_population <= 0 ) {
            g->catch_a_monster( chosen_fish, you->pos(), you, 50_hours );
        } else {
            here.add_item_or_charges( you->pos(), item::make_corpse( chosen_fish->type->id,
                                      calendar::turn + rng( 0_turns,
                                              3_hours ) ) );
            you->add_msg_if_player( m_good, _( "You caught a %s." ), chosen_fish->type->nname() );
        }
    }
    for( item &elem : here.i_at( you->pos() ) ) {
        if( elem.is_corpse() && !elem.has_var( "activity_var" ) ) {
            elem.set_var( "activity_var", you->name );
        }
    }
}

void activity_handlers::fish_do_turn( player_activity *act, Character *you )
{
    item &it = *act->targets.front();
    int fish_chance = 1;
    int survival_skill = you->get_skill_level( skill_survival );
    if( it.has_flag( flag_FISH_POOR ) ) {
        survival_skill += dice( 1, 6 );
    } else if( it.has_flag( flag_FISH_GOOD ) ) {
        // Much better chances with a good fishing implement.
        survival_skill += dice( 4, 9 );
        survival_skill *= 2;
    }
    std::vector<monster *> fishables = g->get_fishable_monsters( act->coord_set );
    // Fish are always there, even if it doesn't seem like they are visible!
    if( fishables.empty() ) {
        fish_chance += survival_skill / 2;
    } else {
        // if they are visible however, it implies a larger population
        for( monster *elem : fishables ) {
            fish_chance += elem->fish_population;
        }
        fish_chance += survival_skill;
    }
    // no matter the population of fish, your skill and tool limits the ease of catching.
    fish_chance = std::min( survival_skill * 10, fish_chance );
    if( x_in_y( fish_chance, 600000 ) ) {
        you->add_msg_if_player( m_good, _( "You feel a tug on your line!" ) );
        rod_fish( you, fishables );
    }
    if( calendar::once_every( 60_minutes ) ) {
        you->practice( skill_survival, rng( 1, 3 ) );
    }

}

void activity_handlers::fish_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    you->add_msg_if_player( m_info, _( "You finish fishing" ) );
    if( !you->backlog.empty() && you->backlog.front().id() == ACT_MULTIPLE_FISH ) {
        you->backlog.clear();
        you->assign_activity( ACT_TIDY_UP );
    }
}

void activity_handlers::repair_item_do_turn( player_activity *act, Character *you )
{
    // Moves are decremented based on a combination of speed and good vision (not in the dark, farsighted, etc)
    const float exertion_mult = you->exertion_adjusted_move_multiplier( act->exertion_level() );
    const int effective_moves = you->moves / ( you->fine_detail_vision_mod() * exertion_mult );
    if( effective_moves <= act->moves_left ) {
        act->moves_left -= effective_moves;
        you->moves = 0;
    } else {
        you->moves -= act->moves_left * you->fine_detail_vision_mod();
        act->moves_left = 0;
    }
}

void activity_handlers::butcher_do_turn( player_activity * /*act*/, Character *you )
{
    you->mod_stamina( -20 );
}

void activity_handlers::wait_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "You finish waiting." ) );
    act->set_to_null();
}

void activity_handlers::wait_weather_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "You finish waiting." ) );
    act->set_to_null();
}

void activity_handlers::find_mount_do_turn( player_activity *act, Character *you )
{
    //npc only activity
    if( you->is_avatar() ) {
        act->set_to_null();
        return;
    }
    npc &guy = dynamic_cast<npc &>( *you );
    auto strong_monster = guy.chosen_mount.lock();
    monster *mon = strong_monster.get();
    if( !mon ) {
        act->set_to_null();
        guy.revert_after_activity();
        return;
    }
    if( rl_dist( guy.pos(), mon->pos() ) <= 1 ) {
        if( mon->has_effect( effect_controlled ) ) {
            mon->remove_effect( effect_controlled );
        }
        if( you->can_mount( *mon ) ) {
            act->set_to_null();
            guy.revert_after_activity();
            guy.chosen_mount = weak_ptr_fast<monster>();
            you->mount_creature( *mon );
        } else {
            act->set_to_null();
            guy.revert_after_activity();
            return;
        }
    } else {
        const std::vector<tripoint> route = route_adjacent( *you, guy.chosen_mount.lock()->pos() );
        if( route.empty() ) {
            act->set_to_null();
            guy.revert_after_activity();
            mon->remove_effect( effect_controlled );
            return;
        } else {
            you->activity = player_activity();
            mon->add_effect( effect_controlled, 40_turns );
            you->set_destination( route, player_activity( ACT_FIND_MOUNT ) );
        }
    }
}

void activity_handlers::wait_npc_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "%s finishes with you…" ), act->str_values[0] );
    act->set_to_null();
}

void activity_handlers::wait_stamina_do_turn( player_activity *act, Character *you )
{
    int stamina_threshold = you->get_stamina_max();
    if( !act->values.empty() ) {
        stamina_threshold = act->values[0];
        // remember initial stamina, only for waiting-with-threshold
        if( act->values.size() == 1 ) {
            act->values.push_back( you->get_stamina() );
        }
    }
    if( you->get_stamina() >= stamina_threshold ) {
        wait_stamina_finish( act, you );
    }
}

void activity_handlers::wait_stamina_finish( player_activity *act, Character *you )
{
    if( !act->values.empty() ) {
        const int stamina_threshold = act->values[0];
        const int stamina_initial = ( act->values.size() > 1 ) ? act->values[1] : you->get_stamina();
        if( you->get_stamina() < stamina_threshold && you->get_stamina() <= stamina_initial ) {
            debugmsg( "Failed to wait until stamina threshold %d reached, only at %d. You may not be regaining stamina.",
                      act->values.front(), you->get_stamina() );
        }
    } else if( you->get_stamina() < you->get_stamina_max() ) {
        you->add_msg_if_player( _( "You are bored of waiting, so you stop." ) );
    } else {
        you->add_msg_if_player( _( "You finish waiting and feel refreshed." ) );
    }
    act->set_to_null();
}

void activity_handlers::socialize_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( _( "%s finishes chatting with you." ), act->str_values[0] );
    act->set_to_null();
}

void activity_handlers::operation_do_turn( player_activity *act, Character *you )
{
    /**
    - values[0]: Difficulty
    - values[1]: success
    - values[2]: max_power_level
    - values[3]: pl_skill
    - str_values[0]: install/uninstall
    - str_values[1]: bionic_id
    - str_values[2]: installer_name
    - str_values[3]: bool autodoc
    */
    enum operation_values_ids {
        operation_type = 0,
        cbm_id = 1,
        installer_name = 2,
        is_autodoc = 3
    };
    const bionic_id bid( act->str_values[cbm_id] );
    const bool autodoc = act->str_values[is_autodoc] == "true";
    Character &player_character = get_player_character();
    const bool u_see = player_character.sees( you->pos() ) &&
                       ( !player_character.has_effect( effect_narcosis ) ||
                         player_character.has_bionic( bio_painkiller ) || player_character.has_trait( trait_NOPAIN ) );

    const int difficulty = act->values.front();

    const std::vector<bodypart_id> bps = get_occupied_bodyparts( bid );

    const time_duration half_op_duration = difficulty * 10_minutes;
    const time_duration message_freq = difficulty * 2_minutes;
    time_duration time_left = time_duration::from_moves( act->moves_left );

    map &here = get_map();
    if( autodoc && here.inbounds( you->pos() ) ) {
        const std::list<tripoint> autodocs = here.find_furnitures_with_flag_in_radius( you->pos(), 1,
                                             ter_furn_flag::TFLAG_AUTODOC );

        if( !here.has_flag_furn( ter_furn_flag::TFLAG_AUTODOC_COUCH, you->pos() ) || autodocs.empty() ) {
            you->remove_effect( effect_under_operation );
            act->set_to_null();

            if( u_see ) {
                add_msg( m_bad, _( "The autodoc suffers a catastrophic failure." ) );

                you->add_msg_player_or_npc( m_bad,
                                            _( "The Autodoc's failure damages you greatly." ),
                                            _( "The Autodoc's failure damages <npcname> greatly." ) );
            }
            if( !bps.empty() ) {
                for( const bodypart_id &bp : bps ) {
                    you->make_bleed( effect_source::empty(), bp, 1_turns, difficulty, true );
                    you->apply_damage( nullptr, bp, 20 * difficulty );

                    if( u_see ) {
                        you->add_msg_player_or_npc( m_bad, _( "Your %s is ripped open." ),
                                                    _( "<npcname>'s %s is ripped open." ), body_part_name_accusative( bp ) );
                    }

                    if( bp == bodypart_id( "eyes" ) ) {
                        you->add_effect( effect_blind, 1_hours );
                    }
                }
            } else {
                you->make_bleed( effect_source::empty(), bodypart_id( "bp_null" ), 1_turns, difficulty, true );
                you->apply_damage( nullptr, bodypart_id( "torso" ), 20 * difficulty );
            }
        }
    }

    if( time_left > half_op_duration ) {
        if( !bps.empty() ) {
            for( const bodypart_id &bp : bps ) {
                if( calendar::once_every( message_freq ) && u_see && autodoc ) {
                    you->add_msg_player_or_npc( m_info,
                                                _( "The Autodoc is meticulously cutting your %s open." ),
                                                _( "The Autodoc is meticulously cutting <npcname>'s %s open." ),
                                                body_part_name_accusative( bp ) );
                }
            }
        } else {
            if( calendar::once_every( message_freq ) && u_see ) {
                you->add_msg_player_or_npc( m_info,
                                            _( "The Autodoc is meticulously cutting you open." ),
                                            _( "The Autodoc is meticulously cutting <npcname> open." ) );
            }
        }
    } else if( time_left == half_op_duration ) {
        if( act->str_values[operation_type] == "uninstall" ) {
            if( u_see && autodoc ) {
                add_msg( m_info, _( "The Autodoc attempts to carefully extract the bionic." ) );
            }

            if( you->has_bionic( bid ) ) {
                you->perform_uninstall( bid, act->values[0], act->values[1],
                                        units::from_millijoule( act->values[2] ), act->values[3] );
            } else {
                debugmsg( _( "Tried to uninstall %s, but you don't have this bionic installed." ), bid.c_str() );
                you->remove_effect( effect_under_operation );
                act->set_to_null();
            }
        } else {
            if( u_see && autodoc ) {
                add_msg( m_info, _( "The Autodoc attempts to carefully insert the bionic." ) );
            }

            if( bid.is_valid() ) {
                const bionic_id upbid = bid->upgraded_bionic;
                you->perform_install( bid, upbid, act->values[0], act->values[1], act->values[3],
                                      act->str_values[installer_name], bid->canceled_mutations, you->pos() );
            } else {
                debugmsg( _( "%s is no a valid bionic_id" ), bid.c_str() );
                you->remove_effect( effect_under_operation );
                act->set_to_null();
            }
        }
    } else if( act->values[1] > 0 ) {
        if( !bps.empty() ) {
            for( const bodypart_id &bp : bps ) {
                if( calendar::once_every( message_freq ) && u_see && autodoc ) {
                    you->add_msg_player_or_npc( m_info,
                                                _( "The Autodoc is stitching your %s back up." ),
                                                _( "The Autodoc is stitching <npcname>'s %s back up." ),
                                                body_part_name_accusative( bp ) );
                }
            }
        } else {
            if( calendar::once_every( message_freq ) && u_see && autodoc ) {
                you->add_msg_player_or_npc( m_info,
                                            _( "The Autodoc is stitching you back up." ),
                                            _( "The Autodoc is stitching <npcname> back up." ) );
            }
        }
    } else {
        if( calendar::once_every( message_freq ) && u_see && autodoc ) {
            you->add_msg_player_or_npc( m_bad,
                                        _( "The Autodoc is moving erratically through the rest of its program, not actually stitching your wounds." ),
                                        _( "The Autodoc is moving erratically through the rest of its program, not actually stitching <npcname>'s wounds." ) );
        }
    }

    // Makes sure NPC is still under anesthesia
    if( you->has_effect( effect_narcosis ) ) {
        const time_duration remaining_time = you->get_effect_dur( effect_narcosis );
        if( remaining_time < time_left ) {
            const time_duration top_off_time = time_left - remaining_time;
            you->add_effect( effect_narcosis, top_off_time );
            you->add_effect( effect_sleep, top_off_time );
        }
    } else {
        you->add_effect( effect_narcosis, time_left );
        you->add_effect( effect_sleep, time_left );
    }
}

void activity_handlers::operation_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    if( act->str_values[3] == "true" ) {
        if( act->values[1] > 0 ) {
            add_msg( m_good,
                     _( "The Autodoc returns to its resting position after successfully performing the operation." ) );
            const std::list<tripoint> autodocs = here.find_furnitures_with_flag_in_radius( you->pos(), 1,
                                                 ter_furn_flag::TFLAG_AUTODOC );
            sounds::sound( autodocs.front(), 10, sounds::sound_t::music,
                           _( "a short upbeat jingle: \"Operation successful\"" ), true,
                           "Autodoc",
                           "success" );
        } else {
            add_msg( m_bad,
                     _( "The Autodoc jerks back to its resting position after failing the operation." ) );
            const std::list<tripoint> autodocs = here.find_furnitures_with_flag_in_radius( you->pos(), 1,
                                                 ter_furn_flag::TFLAG_AUTODOC );
            sounds::sound( autodocs.front(), 10, sounds::sound_t::music,
                           _( "a sad beeping noise: \"Operation failed\"" ), true,
                           "Autodoc",
                           "failure" );
        }
    } else {
        if( act->values[1] > 0 ) {
            add_msg( m_good,
                     _( "The operation is a success." ) );
        } else {
            add_msg( m_bad,
                     _( "The operation is a failure." ) );
        }
    }
    you->remove_effect( effect_under_operation );
    act->set_to_null();
}

void activity_handlers::churn_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    you->add_msg_if_player( _( "You finish churning up the earth here." ) );
    here.ter_set( here.getlocal( act->placement ), t_dirtmound );
    // Go back to what we were doing before
    // could be player zone activity, or could be NPC multi-farming
    act->set_to_null();
    resume_for_multi_activities( *you );
}

void activity_handlers::plant_seed_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    tripoint examp = here.getlocal( act->placement );
    const itype_id seed_id( act->str_values[0] );
    std::list<item> used_seed;
    if( item::count_by_charges( seed_id ) ) {
        used_seed = you->use_charges( seed_id, 1 );
    } else {
        used_seed = you->use_amount( seed_id, 1 );
    }
    if( !used_seed.empty() ) {
        used_seed.front().set_age( 0_turns );
        if( used_seed.front().has_var( "activity_var" ) ) {
            used_seed.front().erase_var( "activity_var" );
        }
        used_seed.front().set_flag( json_flag_HIDDEN_ITEM );
        here.add_item_or_charges( examp, used_seed.front() );
        if( here.has_flag_furn( ter_furn_flag::TFLAG_PLANTABLE, examp ) ) {
            here.furn_set( examp, furn_str_id( here.furn( examp )->plant->transform ) );
        } else {
            here.set( examp, t_dirt, f_plant_seed );
        }
        you->add_msg_player_or_npc( _( "You plant some %s." ), _( "<npcname> plants some %s." ),
                                    item::nname( seed_id ) );
    }
    // Go back to what we were doing before
    // could be player zone activity, or could be NPC multi-farming
    act->set_to_null();
    resume_for_multi_activities( *you );
}

void activity_handlers::build_do_turn( player_activity *act, Character *you )
{
    map &here = get_map();
    partial_con *pc = here.partial_con_at( here.getlocal( act->placement ) );
    // Maybe the player and the NPC are working on the same construction at the same time
    if( !pc ) {
        if( you->is_npc() ) {
            // if player completes the work while NPC still in activity loop
            you->activity = player_activity();
            you->set_moves( 0 );
        } else {
            you->cancel_activity();
        }
        add_msg( m_info, _( "%s did not find an unfinished construction at the activity spot." ),
                 you->disp_name() );
        return;
    }
    // if you ( or NPC ) are finishing someone else's started construction...
    const construction &built = pc->id.obj();
    if( !you->has_trait( trait_DEBUG_HS ) && !you->meets_skill_requirements( built ) ) {
        add_msg( m_info, _( "%s can't work on this construction anymore." ), you->disp_name() );
        you->cancel_activity();
        if( you->is_npc() ) {
            you->activity = player_activity();
            you->set_moves( 0 );
        }
        return;
    }
    // item_counter represents the percent progress relative to the base batch time
    // stored precise to 5 decimal places ( e.g. 67.32 percent would be stored as 6732000 )
    const int old_counter = pc->counter;

    // Base moves for construction with no speed modifier or assistants
    // Must ensure >= 1 so we don't divide by 0;
    const double base_total_moves = std::max( 1, built.time );
    // Current expected total moves, includes construction speed modifiers and assistants
    const double cur_total_moves = std::max( 1, built.adjusted_time() );
    // Delta progress in moves adjusted for current crafting speed
    const double delta_progress = you->get_moves() * base_total_moves / cur_total_moves;
    // Current progress in moves
    const double current_progress = old_counter * base_total_moves / 10000000.0 +
                                    delta_progress;
    // Current progress as a percent of base_total_moves to 2 decimal places
    pc->counter = std::round( current_progress / base_total_moves * 10000000.0 );

    you->set_moves( 0 );

    pc->counter = std::min( pc->counter, 10000000 );
    // If construction_progress has reached 100% or more
    if( pc->counter >= 10000000 ) {
        // Activity is canceled in complete_construction()
        complete_construction( you->as_character() );
    }
}

void activity_handlers::tidy_up_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_fish_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_construction_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_mine_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_chop_planks_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_butcher_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}
void activity_handlers::multiple_dis_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}


void activity_handlers::vehicle_deconstruction_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::vehicle_repair_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::chop_trees_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::multiple_farm_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::fetch_do_turn( player_activity *act, Character *you )
{
    generic_multi_activity_handler( *act, *you );
}

void activity_handlers::vibe_finish( player_activity *act, Character *you )
{
    you->add_msg_if_player( m_good, _( "You feel much better." ) );
    you->add_morale( MORALE_FEELING_GOOD, 10, 40 );
    act->set_to_null();
}

void activity_handlers::atm_finish( player_activity *act, Character * )
{
    // ATM sets index to 0 to indicate it's finished.
    if( !act->index ) {
        act->set_to_null();
    }
}

void activity_handlers::eat_menu_finish( player_activity *, Character * )
{
    // Only exists to keep the eat activity alive between turns
}

void activity_handlers::hacksaw_do_turn( player_activity *act, Character * )
{
    sfx::play_activity_sound( "tool", "hacksaw", sfx::get_heard_volume( act->placement ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a metal sawing tool at work!
        sounds::sound( act->placement, 15, sounds::sound_t::destructive_activity, _( "grnd grnd grnd" ) );
    }
}

void activity_handlers::hacksaw_finish( player_activity *act, Character *you )
{
    const tripoint &pos = act->placement;
    map &here = get_map();
    const ter_id ter = here.ter( pos );

    if( here.furn( pos ) == f_rack ) {
        here.furn_set( pos, f_null );
        here.spawn_item( you->pos(), itype_pipe, rng( 1, 3 ) );
        here.spawn_item( you->pos(), itype_steel_chunk );
    } else if( ter == t_chainfence || ter == t_chaingate_c || ter == t_chaingate_l ) {
        here.ter_set( pos, t_dirt );
        here.spawn_item( you->pos(), itype_pipe, 6 );
        here.spawn_item( you->pos(), itype_wire, 20 );
    } else if( ter == t_chainfence_posts ) {
        here.ter_set( pos, t_dirt );
        here.spawn_item( you->pos(), itype_pipe, 6 );
    } else if( ter == t_window_bars_alarm ) {
        here.ter_set( pos, t_window_alarm );
        here.spawn_item( you->pos(), itype_rebar, rng( 1, 8 ) );
    } else if( ter == t_window_bars_curtains || ter == t_window_bars_domestic ) {
        here.ter_set( pos, t_window_domestic );
        here.spawn_item( you->pos(), itype_rebar, rng( 1, 8 ) );
    } else if( ter == t_window_bars ) {
        here.ter_set( pos, t_window_empty );
        here.spawn_item( you->pos(), itype_rebar, rng( 1, 8 ) );
    } else if( ter == t_window_enhanced ) {
        here.ter_set( pos, t_window_reinforced );
        here.spawn_item( you->pos(), itype_spike, rng( 1, 4 ) );
    } else if( ter == t_window_enhanced_noglass ) {
        here.ter_set( pos, t_window_reinforced_noglass );
        here.spawn_item( you->pos(), itype_spike, rng( 1, 4 ) );
    } else if( ter == t_reb_cage ) {
        here.ter_set( pos, t_pit );
        here.spawn_item( you->pos(), itype_spike, 19 );
        here.spawn_item( you->pos(), itype_scrap, 8 );
    } else if( ter == t_retractable_gate_c || ter == t_retractable_gate_l ) {
        here.ter_set( pos, t_strconc_floor );
        here.spawn_item( pos, itype_chain, rng( 1, 2 ) );
        here.spawn_item( pos, itype_wire, rng( 8, 22 ) );
    } else if( ter == t_bars ) {
        if( here.ter( pos + point_east ) == t_sewage || here.ter( pos + point_south )
            == t_sewage ||
            here.ter( pos + point_west ) == t_sewage || here.ter( pos + point_north ) ==
            t_sewage ) {
            here.ter_set( pos, t_sewage );
            here.spawn_item( you->pos(), itype_pipe, 3 );
        } else {
            here.ter_set( pos, t_floor );
            here.spawn_item( you->pos(), itype_pipe, 3 );
        }
    } else if( ter == t_door_bar_c || ter == t_door_bar_locked ) {
        here.ter_set( pos, t_mdoor_frame );
        here.spawn_item( you->pos(), itype_pipe, 12 );
    } else if( ter == t_metal_grate_window || ter == t_metal_grate_window_with_curtain ||
               ter == t_metal_grate_window_with_curtain_open ) {
        here.ter_set( pos, t_window_reinforced );
        here.spawn_item( you->pos(), itype_pipe, rng( 1, 12 ) );
        here.spawn_item( you->pos(), itype_sheet_metal, 4 );
    } else if( ter == t_metal_grate_window_noglass ||
               ter == t_metal_grate_window_with_curtain_noglass ||
               ter == t_metal_grate_window_with_curtain_open_noglass ) {
        here.ter_set( pos, t_window_reinforced_noglass );
        here.spawn_item( you->pos(), itype_pipe, rng( 1, 12 ) );
        here.spawn_item( you->pos(), itype_sheet_metal, 4 );
    }

    you->add_msg_if_player( m_good, _( "You finish cutting the metal." ) );

    act->set_to_null();
}

void activity_handlers::pry_nails_do_turn( player_activity *act, Character * )
{
    sfx::play_activity_sound( "tool", "hammer", sfx::get_heard_volume( act->placement ) );
}

void activity_handlers::pry_nails_finish( player_activity *act, Character *you )
{
    const tripoint &pnt = act->placement;
    map &here = get_map();
    const ter_id type = here.ter( pnt );

    int nails = 0;
    int boards = 0;
    ter_id newter;
    if( type == t_fence ) {
        nails = 6;
        boards = 3;
        newter = t_fence_post;
        you->add_msg_if_player( _( "You pry out the fence post." ) );
    } else if( type == t_window_boarded ) {
        nails = 8;
        boards = 4;
        newter = t_window_frame;
        you->add_msg_if_player( _( "You pry the boards from the window." ) );
    } else if( type == t_window_boarded_noglass ) {
        nails = 8;
        boards = 4;
        newter = t_window_empty;
        you->add_msg_if_player( _( "You pry the boards from the window frame." ) );
    } else if( type == t_door_boarded || type == t_door_boarded_damaged ||
               type == t_rdoor_boarded || type == t_rdoor_boarded_damaged ||
               type == t_door_boarded_peep || type == t_door_boarded_damaged_peep ) {
        nails = 8;
        boards = 4;
        if( type == t_door_boarded ) {
            newter = t_door_c;
        } else if( type == t_door_boarded_damaged ) {
            newter = t_door_b;
        } else if( type == t_door_boarded_peep ) {
            newter = t_door_c_peep;
        } else if( type == t_door_boarded_damaged_peep ) {
            newter = t_door_b_peep;
        } else if( type == t_rdoor_boarded ) {
            newter = t_rdoor_c;
        } else { // if (type == t_rdoor_boarded_damaged)
            newter = t_rdoor_b;
        }
        you->add_msg_if_player( _( "You pry the boards from the door." ) );
    }
    you->practice( skill_fabrication, 1, 1 );
    here.spawn_item( you->pos(), itype_nail, 0, nails );
    here.spawn_item( you->pos(), itype_2x4, boards );
    here.ter_set( pnt, newter );
    act->set_to_null();
}

void activity_handlers::chop_tree_do_turn( player_activity *act, Character * )
{
    map &here = get_map();
    sfx::play_activity_sound( "tool", "axe", sfx::get_heard_volume( here.getlocal( act->placement ) ) );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a wood chopping tool at work!
        sounds::sound( here.getlocal( act->placement ), 15, sounds::sound_t::activity, _( "CHK!" ) );
    }
}

void activity_handlers::chop_tree_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    const tripoint &pos = here.getlocal( act->placement );

    tripoint direction;
    if( !you->is_npc() ) {
        if( you->backlog.empty() || you->backlog.front().id() != ACT_MULTIPLE_CHOP_TREES ) {
            while( true ) {
                if( const cata::optional<tripoint> dir = choose_direction(
                            _( "Select a direction for the tree to fall in." ) ) ) {
                    direction = *dir;
                    break;
                }
                // try again
            }
        }
    } else {
        creature_tracker &creatures = get_creature_tracker();
        for( const tripoint &elem : here.points_in_radius( pos, 1 ) ) {
            bool cantuse = false;
            tripoint direc = elem - pos;
            tripoint proposed_to = pos + point( 3 * direction.x, 3 * direction.y );
            std::vector<tripoint> rough_tree_line = line_to( pos, proposed_to );
            for( const tripoint &elem : rough_tree_line ) {
                if( creatures.creature_at( elem ) ) {
                    cantuse = true;
                    break;
                }
            }
            if( !cantuse ) {
                direction = direc;
            }
        }
    }

    const tripoint to = pos + 3 * direction.xy() + point( rng( -1, 1 ), rng( -1, 1 ) );
    std::vector<tripoint> tree = line_to( pos, to, rng( 1, 8 ) );
    for( const tripoint &elem : tree ) {
        here.batter( elem, 300, 5 );
        here.ter_set( elem, t_trunk );
    }

    here.ter_set( pos, t_stump );
    you->add_msg_if_player( m_good, _( "You finish chopping down a tree." ) );
    // sound of falling tree
    sfx::play_variant_sound( "misc", "timber",
                             sfx::get_heard_volume( here.getlocal( act->placement ) ) );
    get_event_bus().send<event_type::cuts_tree>( you->getID() );
    act->set_to_null();
    resume_for_multi_activities( *you );
}

void activity_handlers::chop_logs_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    const tripoint &pos = here.getlocal( act->placement );
    int log_quan;
    int stick_quan;
    int splint_quan;
    if( here.ter( pos ) == t_trunk ) {
        log_quan = rng( 2, 3 );
        stick_quan = rng( 0, 1 );
        splint_quan = 0;
    } else if( here.ter( pos ) == t_stump ) {
        log_quan = rng( 0, 2 );
        stick_quan = 0;
        splint_quan = rng( 5, 15 );
    } else {
        log_quan = 0;
        stick_quan = 0;
        splint_quan = 0;
    }
    for( int i = 0; i != log_quan; ++i ) {
        item obj( itype_log, calendar::turn );
        obj.set_var( "activity_var", you->name );
        here.add_item_or_charges( pos, obj );
    }
    for( int i = 0; i != stick_quan; ++i ) {
        item obj( itype_stick_long, calendar::turn );
        obj.set_var( "activity_var", you->name );
        here.add_item_or_charges( pos, obj );
    }
    for( int i = 0; i != splint_quan; ++i ) {
        item obj( itype_splinter, calendar::turn );
        obj.set_var( "activity_var", you->name );
        here.add_item_or_charges( pos, obj );
    }
    here.ter_set( pos, t_dirt );
    you->add_msg_if_player( m_good, _( "You finish chopping wood." ) );

    act->set_to_null();
    resume_for_multi_activities( *you );
}

void activity_handlers::chop_planks_finish( player_activity *act, Character *you )
{
    const int max_planks = 10;
    /** @EFFECT_FABRICATION increases number of planks cut from a log */
    int planks = normal_roll( 2 + you->get_skill_level( skill_id( "fabrication" ) ), 1 );
    int wasted_planks = max_planks - planks;
    int scraps = rng( wasted_planks, wasted_planks * 3 );
    planks = std::min( planks, max_planks );

    map &here = get_map();
    if( planks > 0 ) {
        here.spawn_item( here.getlocal( act->placement ), itype_2x4, planks, 0, calendar::turn );
        you->add_msg_if_player( m_good, _( "You produce %d planks." ), planks );
    }
    if( scraps > 0 ) {
        here.spawn_item( here.getlocal( act->placement ), itype_splinter, scraps, 0, calendar::turn );
        you->add_msg_if_player( m_good, _( "You produce %d splinters." ), scraps );
    }
    if( planks < max_planks / 2 ) {
        you->add_msg_if_player( m_bad, _( "You waste a lot of the wood." ) );
    }
    act->set_to_null();
    resume_for_multi_activities( *you );
}

void activity_handlers::jackhammer_do_turn( player_activity *act, Character * )
{
    map &here = get_map();
    sfx::play_activity_sound( "tool", "jackhammer",
                              sfx::get_heard_volume( here.getlocal( act->placement ) ) );
    if( calendar::once_every( 1_minutes ) ) {
        sounds::sound( here.getlocal( act->placement ), 15, sounds::sound_t::destructive_activity,
                       //~ Sound of a jackhammer at work!
                       _( "TATATATATATATAT!" ) );
    }
}

void activity_handlers::jackhammer_finish( player_activity *act, Character *you )
{
    map &here = get_map();
    const tripoint &pos = here.getlocal( act->placement );

    here.destroy( pos, true );

    you->add_msg_player_or_npc( m_good,
                                _( "You finish drilling." ),
                                _( "<npcname> finishes drilling." ) );
    act->set_to_null();
    if( !act->targets.empty() ) {
        item &it = *act->targets.front();
        it.ammo_consume( it.ammo_required(), tripoint_zero, you );
    } else {
        debugmsg( "jackhammer activity targets empty" );
    }
    if( resume_for_multi_activities( *you ) ) {
        for( item &elem : here.i_at( pos ) ) {
            elem.set_var( "activity_var", you->name );
        }
    }
}

void activity_handlers::fill_pit_do_turn( player_activity *act, Character * )
{
    sfx::play_activity_sound( "tool", "shovel", 100 );
    if( calendar::once_every( 1_minutes ) ) {
        //~ Sound of a shovel filling a pit or mound at work!
        sounds::sound( act->placement, 10, sounds::sound_t::activity, _( "hsh!" ) );
    }
}

void activity_handlers::fill_pit_finish( player_activity *act, Character *you )
{
    const tripoint &pos = act->placement;
    map &here = get_map();
    const ter_id ter = here.ter( pos );
    const ter_id old_ter = ter;

    if( ter == t_pit || ter == t_pit_spiked || ter == t_pit_glass ||
        ter == t_pit_corpsed ) {
        here.ter_set( pos, t_pit_shallow );
    } else {
        here.ter_set( pos, t_dirt );
    }
    you->add_msg_if_player( m_good, _( "You finish filling up %s." ), old_ter.obj().name() );

    act->set_to_null();
}

std::vector<tripoint> get_sorted_tiles_by_distance( const tripoint &abspos,
        const std::unordered_set<tripoint> &tiles )
{
    const auto cmp = [abspos]( tripoint a, tripoint b ) {
        const int da = rl_dist( abspos, a );
        const int db = rl_dist( abspos, b );

        return da < db;
    };

    std::vector<tripoint> sorted( tiles.begin(), tiles.end() );
    std::sort( sorted.begin(), sorted.end(), cmp );

    return sorted;
}

template<typename fn>
static void cleanup_tiles( std::unordered_set<tripoint> &tiles, fn &cleanup )
{
    auto it = tiles.begin();
    map &here = get_map();
    while( it != tiles.end() ) {
        auto current = it++;

        const tripoint &tile_loc = here.getlocal( *current );

        if( cleanup( tile_loc ) ) {
            tiles.erase( current );
        }
    }
}

static void perform_zone_activity_turn( Character *you,
                                        const zone_type_id &ztype,
                                        const std::function<bool( const tripoint & )> &tile_filter,
                                        const std::function<void ( Character &you, const tripoint & )> &tile_action,
                                        const std::string &finished_msg )
{
    const zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    const tripoint abspos = here.getabs( you->pos() );
    std::unordered_set<tripoint> unsorted_tiles = mgr.get_near( ztype, abspos );

    cleanup_tiles( unsorted_tiles, tile_filter );

    // sort remaining tiles by distance
    const std::vector<tripoint> &tiles = get_sorted_tiles_by_distance( abspos, unsorted_tiles );

    for( const tripoint &tile : tiles ) {
        const tripoint &tile_loc = here.getlocal( tile );

        std::vector<tripoint> route = here.route( you->pos(), tile_loc, you->get_pathfinding_settings(),
                                      you->get_path_avoid() );
        if( route.size() > 1 ) {
            route.pop_back();

            you->set_destination( route, you->activity );
            you->activity.set_to_null();
            return;
        } else {
            // we are at destination already
            /* Perform action */
            tile_action( *you, tile_loc );
            if( you->moves <= 0 ) {
                return;
            }
        }
    }
    add_msg( m_info, finished_msg );
    you->activity.set_to_null();
}

void activity_handlers::fertilize_plot_do_turn( player_activity *act, Character *you )
{
    itype_id fertilizer;
    auto check_fertilizer = [&]( bool ask_user = true ) -> void {
        if( act->str_values.empty() )
        {
            act->str_values.emplace_back( "" );
        }
        fertilizer = itype_id( act->str_values[0] );

        /* If unspecified, or if we're out of what we used before, ask */
        if( ask_user && ( fertilizer.is_empty() || !you->has_charges( fertilizer, 1 ) ) )
        {
            fertilizer = iexamine::choose_fertilizer( *you, "plant",
                    false /* Don't confirm action with player */ );
            act->str_values[0] = fertilizer.str();
        }
    };

    auto have_fertilizer = [&]() {
        return !fertilizer.is_empty() && you->has_charges( fertilizer, 1 );
    };

    const auto reject_tile = [&]( const tripoint & tile ) {
        check_fertilizer();
        ret_val<bool> can_fert = iexamine::can_fertilize( *you, tile, fertilizer );
        return !can_fert.success();
    };

    const auto fertilize = [&]( Character & you, const tripoint & tile ) {
        check_fertilizer();
        if( have_fertilizer() ) {
            iexamine::fertilize_plant( you, tile, fertilizer );
            if( !have_fertilizer() ) {
                add_msg( m_info, _( "You have run out of %s." ), item::nname( fertilizer ) );
            }
        }
    };

    check_fertilizer();
    if( !have_fertilizer() ) {
        act->set_to_null();
        return;
    }

    perform_zone_activity_turn( you,
                                zone_type_FARM_PLOT,
                                reject_tile,
                                fertilize,
                                _( "You fertilized every plot you could." ) );
}

void activity_handlers::robot_control_do_turn( player_activity *act, Character *you )
{
    if( act->monsters.empty() ) {
        debugmsg( "No monster assigned in ACT_ROBOT_CONTROL" );
        act->set_to_null();
        return;
    }
    const shared_ptr_fast<monster> z = act->monsters[0].lock();

    if( !z || !iuse::robotcontrol_can_target( you, *z ) ) {
        you->add_msg_if_player( _( "Target lost.  IFF override failed." ) );
        act->set_to_null();
        return;
    }

    // TODO: Add some kind of chance of getting the target's attention
}

void activity_handlers::robot_control_finish( player_activity *act, Character *you )
{
    act->set_to_null();

    if( act->monsters.empty() ) {
        debugmsg( "No monster assigned in ACT_ROBOT_CONTROL" );
        return;
    }

    shared_ptr_fast<monster> z = act->monsters[0].lock();
    act->monsters.clear();

    if( !z || !iuse::robotcontrol_can_target( you, *z ) ) {
        you->add_msg_if_player( _( "Target lost.  IFF override failed." ) );
        return;
    }

    you->add_msg_if_player( _( "You unleash your override attack on the %s." ), z->name() );

    /** @EFFECT_INT increases chance of successful robot reprogramming, vs difficulty */
    /** @EFFECT_COMPUTER increases chance of successful robot reprogramming, vs difficulty */
    const int computer_skill = you->get_skill_level( skill_id( "computer" ) );
    const float randomized_skill = rng( 2, you->int_cur ) + computer_skill;
    float success = computer_skill - 3 * z->type->difficulty / randomized_skill;
    if( z->has_flag( MF_RIDEABLE_MECH ) ) {
        success = randomized_skill - rng( 1, 11 );
    }
    // rideable mechs are not hostile, they have no AI, they do not resist control as much.
    if( success >= 0 ) {
        you->add_msg_if_player( _( "You successfully override the %s's IFF protocols!" ),
                                z->name() );
        z->friendly = -1;
        if( z->has_flag( MF_RIDEABLE_MECH ) ) {
            z->add_effect( effect_pet, 1_turns, true );
        }
    } else if( success >= -2 ) {
        //A near success
        you->add_msg_if_player( _( "The %s short circuits as you attempt to reprogram it!" ), z->name() );
        //damage it a little
        z->apply_damage( you, bodypart_id( "torso" ), rng( 1, 10 ) );
        if( z->is_dead() ) {
            you->practice( skill_id( "computer" ), 10 );
            // Do not do the other effects if the robot died
            return;
        }
        if( one_in( 3 ) ) {
            you->add_msg_if_player( _( "…and turns friendly!" ) );
            //did the robot became friendly permanently?
            if( one_in( 3 ) ) {
                //it did
                z->friendly = -1;
            } else {
                // it didn't
                z->friendly = rng( 5, 40 );
            }
        }
    } else {
        you->add_msg_if_player( _( "…but the robot refuses to acknowledge you as an ally!" ) );
    }
    you->practice( skill_computer, 10 );
}

void activity_handlers::tree_communion_do_turn( player_activity *act, Character *you )
{
    // There's an initial rooting process.
    if( act->values.front() > 0 ) {
        act->values.front() -= 1;
        if( act->values.front() == 0 ) {
            if( you->has_trait( trait_id( trait_SPIRITUAL ) ) ) {
                you->add_msg_if_player( m_good, _( "The ancient tree spirits answer your call." ) );
            } else {
                you->add_msg_if_player( m_good, _( "Your communion with the trees has begun." ) );
            }
        }
        return;
    }
    // Information is received every minute.
    if( !calendar::once_every( 1_minutes ) ) {
        return;
    }
    // Breadth-first search forest tiles until one reveals new overmap tiles.
    std::queue<tripoint_abs_omt> q;
    std::unordered_set<tripoint_abs_omt> seen;
    tripoint_abs_omt loc = you->global_omt_location();
    q.push( loc );
    seen.insert( loc );
    const std::function<bool( const oter_id & )> filter = []( const oter_id & ter ) {
        return ter.obj().is_wooded() || ter.obj().get_name() == "field";
    };
    while( !q.empty() ) {
        tripoint_abs_omt tpt = q.front();
        if( overmap_buffer.reveal( tpt, 3, filter ) ) {
            if( you->has_trait( trait_SPIRITUAL ) ) {
                you->add_morale( MORALE_TREE_COMMUNION, 2, 30, 8_hours, 6_hours );
            } else {
                you->add_morale( MORALE_TREE_COMMUNION, 1, 15, 2_hours, 1_hours );
            }
            if( one_in( 128 ) ) {
                you->add_msg_if_player( "%s", SNIPPET.random_from_category( "tree_communion" ).value_or(
                                            translation() ) );
            }
            return;
        }
        for( const tripoint_abs_omt &neighbor : points_in_radius( tpt, 1 ) ) {
            if( seen.find( neighbor ) != seen.end() ) {
                continue;
            }
            seen.insert( neighbor );
            if( !overmap_buffer.ter( neighbor ).obj().is_wooded() ) {
                continue;
            }
            q.push( neighbor );
        }
        q.pop();
    }
    you->add_msg_if_player( m_info, _( "The trees have shown you what they will." ) );
    act->set_to_null();
}

static void blood_magic( Character *you, int cost )
{
    std::vector<uilist_entry> uile;
    std::vector<bodypart_id> parts;
    int i = 0;
    for( const bodypart_id &bp : you->get_all_body_parts( get_body_part_flags::only_main ) ) {
        const int hp_cur = you->get_part_hp_cur( bp );
        uilist_entry entry( i, hp_cur > cost, i + 49, body_part_hp_bar_ui_text( bp ) );

        const std::pair<std::string, nc_color> &hp = get_hp_bar( hp_cur, you->get_part_hp_max( bp ) );
        entry.ctxt = colorize( hp.first, hp.second );
        uile.emplace_back( entry );
        parts.push_back( bp );
        i++;
    }
    int action = -1;
    while( action < 0 ) {
        action = uilist( _( "Choose part\nto draw blood from." ), uile );
    }
    you->mod_part_hp_cur( parts[action], - cost );
    you->mod_pain( std::max( 1, cost / 3 ) );
}

void activity_handlers::spellcasting_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    const int level_override = act->get_value( 0 );
    spell_id sp( act->name );

    // if level is -1 then we know it's a player spell, otherwise we build it from the ground up
    spell temp_spell( sp );
    spell &spell_being_cast = ( level_override == -1 ) ? you->magic->get_spell( sp ) : temp_spell;

    // if level != 1 then we need to set the spell's level
    if( level_override != -1 ) {
        spell_being_cast.set_level( level_override );
    }

    const bool no_fail = act->get_value( 1 ) == 1;
    const bool no_mana = act->get_value( 2 ) == 0;

    // choose target for spell (if the spell has a range > 0)
    tripoint target = you->pos();
    bool target_is_valid = false;
    if( spell_being_cast.range() > 0 && !spell_being_cast.is_valid_target( spell_target::none ) &&
        !spell_being_cast.has_flag( spell_flag::RANDOM_TARGET ) ) {
        if( you->is_avatar() ) {
            do {
                avatar &you_avatar = *you->as_avatar();
                std::vector<tripoint> trajectory = target_handler::mode_spell( you_avatar, spell_being_cast,
                                                   no_fail,
                                                   no_mana );
                if( !trajectory.empty() ) {
                    target = trajectory.back();
                    target_is_valid = spell_being_cast.is_valid_target( you_avatar, target );
                    if( !( spell_being_cast.is_valid_target( spell_target::ground ) || you_avatar.sees( target ) ) ) {
                        target_is_valid = false;
                    }
                } else {
                    target_is_valid = false;
                }
                if( !target_is_valid ) {
                    if( query_yn( _( "Stop casting spell?  Time spent will be lost." ) ) ) {
                        return;
                    }
                }
            } while( !target_is_valid );
        } else {
            if( act->coords.empty() ) {
                debugmsg( "ERROR: npc tried to cast a spell without a target." );
            } else {
                const tripoint local_target = get_map().getlocal( act->coords.front() );
                if( spell_being_cast.is_valid_target( *you, local_target ) ) {
                    target = local_target;
                } else {
                    npc_attack_spell npc_spell( spell_being_cast.id() );
                    // recalculate effectiveness because it's been a few turns since the npc started casting.
                    const npc_attack_rating effectiveness = npc_spell.evaluate( *you->as_npc(),
                                                            you->last_target.lock().get() );
                    if( effectiveness < 0 ) {
                        add_msg_debug( debugmode::debug_filter::DF_NPC, "%s cancels casting %s, target lost",
                                       you->disp_name(), spell_being_cast.name() );
                        return;
                    } else {
                        target = effectiveness.target();
                    }
                }
            }
        }
    } else if( spell_being_cast.has_flag( spell_flag::RANDOM_TARGET ) ) {
        const cata::optional<tripoint> target_ = spell_being_cast.random_valid_target( *you, you->pos() );
        if( !target_ ) {
            you->add_msg_if_player( game_message_params{ m_bad, gmf_bypass_cooldown },
                                    _( "Your spell can't find a suitable target." ) );
            return;
        }
        target = *target_;
    }

    // no turning back now. it's all said and done.
    bool success = no_fail || rng_float( 0.0f, 1.0f ) >= spell_being_cast.spell_fail( *you );
    int exp_gained = spell_being_cast.casting_exp( *you );
    if( !success ) {
        you->add_msg_if_player( game_message_params{ m_bad, gmf_bypass_cooldown },
                                _( "You lose your concentration!" ) );
        if( !spell_being_cast.is_max_level() && level_override == -1 ) {
            // still get some experience for trying
            spell_being_cast.gain_exp( exp_gained / 5 );
            you->add_msg_if_player( m_good, _( "You gain %i experience.  New total %i." ), exp_gained / 5,
                                    spell_being_cast.xp() );
        }
        return;
    }

    if( spell_being_cast.has_flag( spell_flag::VERBAL ) ) {
        sounds::sound( you->pos(), you->get_shout_volume() / 2, sounds::sound_t::speech,
                       _( "cast a spell" ),
                       false );
    }

    you->add_msg_if_player( spell_being_cast.message(), spell_being_cast.name() );

    // this is here now so that the spell first consume its components then casts its effects, necessary to cast
    // spells with the components in hand.
    spell_being_cast.use_components( *you );

    spell_being_cast.cast_all_effects( *you, target );

    if( !no_mana ) {
        // pay the cost
        int cost = spell_being_cast.energy_cost( *you );
        switch( spell_being_cast.energy_source() ) {
            case magic_energy_type::mana:
                you->magic->mod_mana( *you, -cost );
                break;
            case magic_energy_type::stamina:
                you->mod_stamina( -cost );
                break;
            case magic_energy_type::bionic:
                you->mod_power_level( -units::from_kilojoule( cost ) );
                break;
            case magic_energy_type::hp:
                blood_magic( you, cost );
                break;
            case magic_energy_type::none:
            default:
                break;
        }

    }
    if( level_override == -1 ) {
        if( !spell_being_cast.is_max_level() ) {
            // reap the reward
            int old_level = spell_being_cast.get_level();
            if( old_level == 0 ) {
                spell_being_cast.gain_level();
                you->add_msg_if_player( m_good,
                                        _( "Something about how this spell works just clicked!  You gained a level!" ) );
            } else {
                spell_being_cast.gain_exp( exp_gained );
                you->add_msg_if_player( m_good, _( "You gain %i experience.  New total %i." ), exp_gained,
                                        spell_being_cast.xp() );
            }
            if( spell_being_cast.get_level() != old_level ) {
                // Level 0-1 message is printed above - notify player when leveling up further
                if( old_level > 0 ) {
                    you->add_msg_if_player( m_good, _( "You gained a level in %s!" ), spell_being_cast.name() );
                }
            }
        }
    }
}

void activity_handlers::study_spell_do_turn( player_activity *act, Character *you )
{
    // Stop if there is not enough light to study
    if( you->fine_detail_vision_mod() > 4 ) {
        act->values[2] = -1;
        act->moves_left = 0;
        return;
    }
    // str_value 1 is "study" if we already know the spell, and want to study it more
    if( act->get_str_value( 1 ) == "study" ) {
        spell &studying = you->magic->get_spell( spell_id( act->name ) );
        // If we are studying to gain a level, keep studying until level changes
        if( act->get_str_value( 0 ) == "gain_level" ) {
            if( studying.get_level() < act->get_value( 1 ) ) {
                act->moves_left = 1000000;
            } else {
                act->moves_left = 0;
            }
        }
        const int old_level = studying.get_level();
        // Gain some experience from studying
        const int xp = roll_remainder( studying.exp_modifier( *you ) / to_turns<float>( 6_seconds ) );
        act->values[0] += xp;
        studying.gain_exp( xp );
        you->practice( studying.skill(), xp, studying.get_difficulty() );

        // Notify player if the spell leveled up
        if( studying.get_level() > old_level ) {
            you->add_msg_if_player( m_good, _( "You gained a level in %s!" ), studying.name() );
        }
    }
}

void activity_handlers::study_spell_finish( player_activity *act, Character *you )
{
    act->set_to_null();
    const int total_exp_gained = act->get_value( 0 );

    if( act->get_str_value( 1 ) == "study" ) {
        you->add_msg_if_player( m_good, _( "You gained %i experience from your study session." ),
                                total_exp_gained );
    } else if( act->get_str_value( 1 ) == "learn" && act->values[2] == 0 ) {
        you->magic->learn_spell( act->name, *you );
    }
    if( act->values[2] == -1 ) {
        you->add_msg_if_player( m_bad, _( "It's too dark to read." ) );
    }
}

//This is just used for robofac_intercom_mission_2
void activity_handlers::mind_splicer_finish( player_activity *act, Character *you )
{
    act->set_to_null();

    if( act->targets.size() != 1 || !act->targets[0] ) {
        debugmsg( "Incompatible arguments to: activity_handlers::mind_splicer_finish" );
        return;
    }
    item &data_card = *act->targets[0];
    you->add_msg_if_player( m_info, _( "…you finally find the memory banks." ) );
    you->add_msg_if_player( m_info, _( "The kit makes a copy of the data inside the bionic." ) );
    data_card.clear_items();
    data_card.put_in( item( itype_mind_scan_robofac ), item_pocket::pocket_type::SOFTWARE );
}
