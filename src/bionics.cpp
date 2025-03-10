#include "bionics.h"

#include <algorithm> //std::min
#include <climits>
#include <cmath>
#include <cstdlib>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_type.h"
#include "assign.h"
#include "avatar.h"
#include "avatar_action.h"
#include "ballistics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_martial_arts.h"
#include "colony.h"
#include "color.h"
#include "damage.h"
#include "debug.h"
#include "dispersion.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "generic_factory.h"
#include "handle_liquid.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "material.h"
#include "memorial_logger.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mutation.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "projectile.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "teleport.h"
#include "translations.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"

static const activity_id ACT_OPERATION( "ACT_OPERATION" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_antifungal( "antifungal" );
static const efftype_id effect_assisted( "assisted" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_heating_bionic( "heating_bionic" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_iodine( "iodine" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_operating( "operating" );
static const efftype_id effect_paralysepoison( "paralysepoison" );
static const efftype_id effect_pblue( "pblue" );
static const efftype_id effect_pkill_l( "pkill_l" );
static const efftype_id effect_pkill1( "pkill1" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_stung( "stung" );
static const efftype_id effect_teleglow( "teleglow" );
static const efftype_id effect_tetanus( "tetanus" );
static const efftype_id effect_took_flumed( "took_flumed" );
static const efftype_id effect_took_prozac( "took_prozac" );
static const efftype_id effect_took_prozac_bad( "took_prozac_bad" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_under_operation( "under_operation" );
static const efftype_id effect_venom_dmg( "venom_dmg" );
static const efftype_id effect_venom_weaken( "venom_weaken" );
static const efftype_id effect_visuals( "visuals" );

static const material_id fuel_type_battery( "battery" );
static const material_id fuel_type_metabolism( "metabolism" );
static const material_id fuel_type_muscle( "muscle" );
static const material_id fuel_type_sun_light( "sunlight" );
static const material_id fuel_type_wind( "wind" );

static const itype_id itype_anesthetic( "anesthetic" );
static const itype_id itype_radiocontrol( "radiocontrol" );
static const itype_id itype_remotevehcontrol( "remotevehcontrol" );
static const itype_id itype_water_clean( "water_clean" );

static const fault_id fault_bionic_salvaged( "fault_bionic_salvaged" );

static const skill_id skill_computer( "computer" );
static const skill_id skill_electronics( "electronics" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_mechanics( "mechanics" );

static const bionic_id bio_adrenaline( "bio_adrenaline" );
static const bionic_id bio_blade_weapon( "bio_blade_weapon" );
static const bionic_id bio_blood_anal( "bio_blood_anal" );
static const bionic_id bio_blood_filter( "bio_blood_filter" );
static const bionic_id bio_claws_weapon( "bio_claws_weapon" );
static const bionic_id bio_cqb( "bio_cqb" );
static const bionic_id bio_ears( "bio_ears" );
static const bionic_id bio_emp( "bio_emp" );
static const bionic_id bio_evap( "bio_evap" );
static const bionic_id bio_flashbang( "bio_flashbang" );
static const bionic_id bio_geiger( "bio_geiger" );
static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_hydraulics( "bio_hydraulics" );
static const bionic_id bio_jointservo( "bio_jointservo" );
static const bionic_id bio_lighter( "bio_lighter" );
static const bionic_id bio_lockpick( "bio_lockpick" );
static const bionic_id bio_magnet( "bio_magnet" );
static const bionic_id bio_meteorologist( "bio_meteorologist" );
static const bionic_id bio_nanobots( "bio_nanobots" );
static const bionic_id bio_painkiller( "bio_painkiller" );
static const bionic_id bio_radscrubber( "bio_radscrubber" );
static const bionic_id bio_remote( "bio_remote" );
static const bionic_id bio_resonator( "bio_resonator" );
static const bionic_id bio_shockwave( "bio_shockwave" );
static const bionic_id bio_teleport( "bio_teleport" );
static const bionic_id bio_time_freeze( "bio_time_freeze" );
static const bionic_id bio_tools( "bio_tools" );
static const bionic_id bio_torsionratchet( "bio_torsionratchet" );
static const bionic_id bio_water_extractor( "bio_water_extractor" );
static const bionic_id bio_tools_extend( "bio_tools_extend" );
// Aftershock stuff!
static const bionic_id afs_bio_dopamine_stimulators( "afs_bio_dopamine_stimulators" );

static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_DEBUG_BIONICS( "DEBUG_BIONICS" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PROF_AUTODOC( "PROF_AUTODOC" );
static const trait_id trait_PROF_MED( "PROF_MED" );
static const trait_id trait_THRESH_MEDICAL( "THRESH_MEDICAL" );

static const json_character_flag json_flag_BIONIC_GUN( "BIONIC_GUN" );
static const json_character_flag json_flag_BIONIC_NPC_USABLE( "BIONIC_NPC_USABLE" );
static const json_character_flag json_flag_BIONIC_WEAPON( "BIONIC_WEAPON" );
static const json_character_flag json_flag_BIONIC_TOGGLED( "BIONIC_TOGGLED" );

struct Character::auto_toggle_bionic_result {
    bool can_burn_fuel = false;
    bool has_burnable_fuel = false;
    material_id burnable_fuel_id;
    enum class fuel_type_t {
        metabolism,
        perpetual,
        remote,
        other
    };
    fuel_type_t fuel_type = fuel_type_t::other;
    int fuel_energy = 0;
    float effective_efficiency = 0.0f;
    int current_fuel_stock = 0;
};

namespace
{
generic_factory<bionic_data> bionic_factory( "bionic" );
std::vector<bionic_id> faulty_bionics;
} //namespace

void bionic::initialize_pseudo_items()
{
    bionic_data bid( info() );

    bool inherit_use_bionic_power = bid.has_flag( flag_USES_BIONIC_POWER );

    if( bid.has_flag( json_flag_BIONIC_GUN ) || bid.has_flag( json_flag_BIONIC_WEAPON ) ) {
        if( !bid.fake_weapon.is_empty() && bid.fake_weapon.is_valid() ) {
            weapon = item( bid.fake_weapon );
            if( inherit_use_bionic_power ) {
                weapon.set_flag( flag_USES_BIONIC_POWER );
            }
        }
    } else if( bid.has_flag( json_flag_BIONIC_TOGGLED ) ) {
        for( const itype_id &id : bid.toggled_pseudo_items ) {
            if( !id.is_empty() && id.is_valid() ) {
                toggled_pseudo_items.emplace_back( id );
            }
        }
    }

    for( const itype_id &id : bid.passive_pseudo_items ) {
        if( !id.is_empty() && id.is_valid() ) {
            passive_pseudo_items.emplace_back( item( id ) );
        }
    }

    if( inherit_use_bionic_power ) {
        for( item &pseudo : passive_pseudo_items ) {
            pseudo.set_flag( flag_USES_BIONIC_POWER );
        }

        for( item &pseudo : toggled_pseudo_items ) {
            pseudo.set_flag( flag_USES_BIONIC_POWER );
        }
    }
}

/** @relates string_id */
template<>
const bionic_data &string_id<bionic_data>::obj() const
{
    return bionic_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<bionic_data>::is_valid() const
{
    return bionic_factory.is_valid( *this );
}

std::vector<bodypart_id> get_occupied_bodyparts( const bionic_id &bid )
{
    std::vector<bodypart_id> parts;
    for( const std::pair<const string_id<body_part_type>, size_t> &element : bid->occupied_bodyparts ) {
        if( element.second > 0 ) {
            parts.push_back( element.first.id() );
        }
    }
    return parts;
}

bool bionic_data::has_flag( const json_character_flag &flag ) const
{
    return flags.count( flag ) > 0;
}

bool bionic_data::has_active_flag( const json_character_flag &flag ) const
{
    return active_flags.count( flag ) > 0;
}

bool bionic_data::has_inactive_flag( const json_character_flag &flag ) const
{
    return inactive_flags.count( flag ) > 0;
}

itype_id bionic_data::itype() const
{
    // For now we just assume that the bionic id matches the corresponding item
    // id (as strings).
    return itype_id( id.str() );
}

bool bionic_data::is_included( const bionic_id &id ) const
{
    return std::find( included_bionics.begin(), included_bionics.end(), id ) != included_bionics.end();
}

static social_modifiers load_bionic_social_mods( const JsonObject &jo )
{
    social_modifiers ret;
    jo.read( "lie", ret.lie );
    jo.read( "persuade", ret.persuade );
    jo.read( "intimidate", ret.intimidate );
    return ret;
}

void bionic_data::load( const JsonObject &jsobj, const std::string & )
{

    mandatory( jsobj, was_loaded, "id", id );
    mandatory( jsobj, was_loaded, "name", name );
    mandatory( jsobj, was_loaded, "description", description );

    optional( jsobj, was_loaded, "cant_remove_reason", cant_remove_reason );
    // uses assign because optional doesn't handle loading units as strings
    assign( jsobj, "react_cost", power_over_time, false, 0_kJ );
    assign( jsobj, "capacity", capacity, false, 0_kJ );
    assign( jsobj, "weight_capacity_bonus", weight_capacity_bonus, false );
    assign( jsobj, "act_cost", power_activate, false, 0_kJ );
    assign( jsobj, "deact_cost", power_deactivate, false, 0_kJ );
    assign( jsobj, "trigger_cost", power_trigger, false, 0_kJ );

    optional( jsobj, was_loaded, "time", charge_time, 0 );

    optional( jsobj, was_loaded, "flags", flags );
    optional( jsobj, was_loaded, "active_flags", active_flags );
    optional( jsobj, was_loaded, "inactive_flags", inactive_flags );

    optional( jsobj, was_loaded, "fuel_efficiency", fuel_efficiency, 0 );
    optional( jsobj, was_loaded, "passive_fuel_efficiency", passive_fuel_efficiency, 0 );

    optional( jsobj, was_loaded, "passive_pseudo_items", passive_pseudo_items );
    optional( jsobj, was_loaded, "toggled_pseudo_items", toggled_pseudo_items );
    optional( jsobj, was_loaded, "fake_weapon", fake_weapon, itype_id() );

    optional( jsobj, was_loaded, "spell_on_activation", spell_on_activate );

    optional( jsobj, was_loaded, "weight_capacity_modifier", weight_capacity_modifier, 1.0f );
    optional( jsobj, was_loaded, "exothermic_power_gen", exothermic_power_gen );
    optional( jsobj, was_loaded, "power_gen_emission", power_gen_emission );
    optional( jsobj, was_loaded, "coverage_power_gen_penalty", coverage_power_gen_penalty );
    optional( jsobj, was_loaded, "is_remote_fueled", is_remote_fueled );

    optional( jsobj, was_loaded, "learned_spells", learned_spells );
    optional( jsobj, was_loaded, "learned_proficiencies", proficiencies );
    optional( jsobj, was_loaded, "canceled_mutations", canceled_mutations );
    optional( jsobj, was_loaded, "mutation_conflicts", mutation_conflicts );
    optional( jsobj, was_loaded, "included_bionics", included_bionics );
    optional( jsobj, was_loaded, "included", included );
    optional( jsobj, was_loaded, "upgraded_bionic", upgraded_bionic );
    optional( jsobj, was_loaded, "fuel_options", fuel_opts );
    optional( jsobj, was_loaded, "fuel_capacity", fuel_capacity );

    optional( jsobj, was_loaded, "available_upgrades", available_upgrades );

    optional( jsobj, was_loaded, "installation_requirement", installation_requirement );

    optional( jsobj, was_loaded, "vitamin_absorb_mod", vitamin_absorb_mod, 1.0f );

    optional( jsobj, was_loaded, "dupes_allowed", dupes_allowed, false );
    optional( jsobj, was_loaded, "auto_deactivates", autodeactivated_bionics );

    int enchant_num = 0;
    for( JsonValue jv : jsobj.get_array( "enchantments" ) ) {
        std::string enchant_name = "INLINE_ENCH_" + name + "_" + std::to_string( enchant_num++ );
        enchantments.push_back( enchantment::load_inline_enchantment( jv, "", enchant_name ) );
    }

    if( jsobj.has_array( "stat_bonus" ) ) {
        // clear data first so that copy-from can override it
        stat_bonus.clear();
        for( JsonArray ja : jsobj.get_array( "stat_bonus" ) ) {
            stat_bonus.emplace( io::string_to_enum<character_stat>( ja.get_string( 0 ) ),
                                ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "encumbrance" ) ) {
        // clear data first so that copy-from can override it
        encumbrance.clear();
        for( JsonArray ja : jsobj.get_array( "encumbrance" ) ) {
            encumbrance.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                                 ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "occupied_bodyparts" ) ) {
        // clear data first so that copy-from can override it
        occupied_bodyparts.clear();
        for( JsonArray ja : jsobj.get_array( "occupied_bodyparts" ) ) {
            occupied_bodyparts.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                                        ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "env_protec" ) ) {
        // clear data first so that copy-from can override it
        env_protec.clear();
        for( JsonArray ja : jsobj.get_array( "env_protec" ) ) {
            env_protec.emplace( bodypart_str_id( ja.get_string( 0 ) ), ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "bash_protec" ) ) {
        // clear data first so that copy-from can override it
        bash_protec.clear();
        for( JsonArray ja : jsobj.get_array( "bash_protec" ) ) {
            bash_protec.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                                 ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "cut_protec" ) ) {
        // clear data first so that copy-from can override it
        cut_protec.clear();
        for( JsonArray ja : jsobj.get_array( "cut_protec" ) ) {
            cut_protec.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                                ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_array( "bullet_protec" ) ) {
        // clear data first so that copy-from can override it
        bullet_protec.clear();
        for( JsonArray ja : jsobj.get_array( "bullet_protec" ) ) {
            bullet_protec.emplace( bodypart_str_id( ja.get_string( 0 ) ),
                                   ja.get_int( 1 ) );
        }
    }
    if( jsobj.has_object( "social_modifiers" ) ) {
        JsonObject sm = jsobj.get_object( "social_modifiers" );
        social_mods = load_bionic_social_mods( sm );
    }

    activated = has_flag( STATIC( json_character_flag( json_flag_BIONIC_TOGGLED ) ) ) ||
                power_activate > 0_kJ ||
                charge_time > 0;

    if( has_flag( STATIC( json_character_flag( "BIONIC_FAULTY" ) ) ) ) {
        faulty_bionics.push_back( id );
    }
}

void bionic_data::load_bionic( const JsonObject &jo, const std::string &src )
{
    bionic_factory.load( jo, src );
}

const std::vector<bionic_data> &bionic_data::get_all()
{
    return bionic_factory.get_all();
}

void bionic_data::check_bionic_consistency()
{
    for( const bionic_data &bio : get_all() ) {

        if( !bio.installation_requirement.is_empty() && !bio.installation_requirement.is_valid() ) {
            debugmsg( "Bionic %s uses undefined requirement_id %s", bio.id.c_str(),
                      bio.installation_requirement.c_str() );
        }
        if( bio.has_flag( json_flag_BIONIC_GUN ) && bio.has_flag( json_flag_BIONIC_WEAPON ) ) {
            debugmsg( "Bionic %s specified as both gun and weapon bionic", bio.id.c_str() );
        }
        if( ( bio.has_flag( json_flag_BIONIC_GUN ) || bio.has_flag( json_flag_BIONIC_WEAPON ) ) &&
            bio.fake_weapon.is_empty() ) {
            debugmsg( "Bionic %s specified as gun or weapon bionic is missing its fake_weapon id",
                      bio.id.c_str() );
        }
        if( bio.has_flag( json_flag_BIONIC_WEAPON ) && !bio.has_flag( flag_BIONIC_TOGGLED ) ) {
            debugmsg( "Bionic \"%s\" specified as weapon bionic is not flagged as \"BIONIC_TOGGLED\"",
                      bio.id.c_str() );
        }

        for( const itype_id &pseudo : bio.passive_pseudo_items ) {
            if( pseudo.is_empty() ) {
                debugmsg( "Bionic \"%s\" has an empty passive_pseudo_item",
                          bio.id.c_str() );
            } else if( !item::type_is_defined( pseudo ) ) {
                debugmsg( "Bionic \"%s\" has unknown passive_pseudo_item \"%s\"",
                          bio.id.c_str(), pseudo.c_str() );
            }
        }

        for( const itype_id &pseudo : bio.toggled_pseudo_items ) {
            if( pseudo.is_empty() ) {
                debugmsg( "Bionic %s has an empty toggled_pseudo_item",
                          bio.id.c_str() );
            } else if( !item::type_is_defined( pseudo ) ) {
                debugmsg( "Bionic \"%s\" has unknown toggled_pseudo_item \"%s\"",
                          bio.id.c_str(), pseudo.c_str() );
            }
        }

        for( const trait_id &mid : bio.canceled_mutations ) {
            if( !mid.is_valid() ) {
                debugmsg( "Bionic %s cancels undefined mutation %s",
                          bio.id.c_str(), mid.c_str() );
            }
        }
        for( const enchantment_id &eid : bio.id->enchantments ) {
            if( !eid.is_valid() ) {
                debugmsg( "Bionic %s uses undefined enchantment %s", bio.id.c_str(), eid.c_str() );
            }
        }
        for( const bionic_id &bid : bio.included_bionics ) {
            if( !bid.is_valid() ) {
                debugmsg( "Bionic %s includes undefined bionic %s",
                          bio.id.c_str(), bid.c_str() );
            }
            if( !bid->occupied_bodyparts.empty() ) {
                debugmsg( "Bionic %s (included by %s) consumes slots, those should be part of the containing bionic instead.",
                          bid.c_str(), bio.id.c_str() );
            }
        }
        for( const bionic_id &bid : bio.autodeactivated_bionics ) {
            if( !bid.is_valid() ) {
                debugmsg( "Bionic \"%s\" has auto_deactivated bionic with invalid id \"%s\"", bio.id.c_str(),
                          bid.c_str() );
            }
        }
        if( bio.upgraded_bionic ) {
            if( bio.upgraded_bionic == bio.id ) {
                debugmsg( "Bionic %s is upgraded with itself", bio.id.c_str() );
            } else if( !bio.upgraded_bionic.is_valid() ) {
                debugmsg( "Bionic %s upgrades undefined bionic %s",
                          bio.id.c_str(), bio.upgraded_bionic.c_str() );
            }
        }
        if( !item::type_is_defined( bio.itype() ) && !bio.included ) {
            debugmsg( "Bionic %s has no defined item version", bio.id.c_str() );
        }
    }
}

bionic_data::bionic_data() : name( no_translation( "bad bionic" ) ),
    description( no_translation( "This bionic was not set up correctly, this is a bug" ) )
{
}

static void force_comedown( effect &eff )
{
    if( eff.is_null() || eff.get_effect_type() == nullptr || eff.get_duration() <= 1_turns ) {
        return;
    }

    eff.set_duration( std::min( eff.get_duration(), eff.get_int_dur_factor() ) );
}

void npc::discharge_cbm_weapon()
{
    if( cbm_weapon_index < 0 ) {
        return;
    }
    const bionic &bio = ( *my_bionics )[cbm_weapon_index];
    mod_power_level( -bio.info().power_activate );
    weapon = real_weapon;
    cbm_weapon_index = -1;
}

void npc::check_or_use_weapon_cbm( const bionic_id &cbm_id )
{
    // if we're already using a bio_weapon, keep using it
    if( cbm_weapon_index >= 0 ) {
        return;
    }
    const float allowed_ratio = static_cast<int>( rules.cbm_reserve ) / 100.0f;
    const units::energy free_power = get_power_level() - get_max_power_level() * allowed_ratio;
    if( free_power <= 0_mJ ) {
        return;
    }

    int index = 0;
    bool found = false;
    for( bionic &i : *my_bionics ) {
        if( i.id == cbm_id && !i.powered ) {
            found = true;
            break;
        }
        index += 1;
    }
    if( !found ) {
        return;
    }
    bionic &bio = ( *my_bionics )[index];

    if( bio.info().has_flag( json_flag_BIONIC_GUN ) ) {
        if( !bio.has_weapon() ) {
            debugmsg( "NPC tried to activate gun bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            return;
        }

        const item cbm_weapon = bio.get_weapon();
        bool not_allowed = !rules.has_flag( ally_rule::use_guns ) ||
                           ( rules.has_flag( ally_rule::use_silent ) && !cbm_weapon.is_silent() );
        if( is_player_ally() && not_allowed ) {
            return;
        }

        int ammo_count = weapon.ammo_remaining( this );
        const int ups_drain = weapon.get_gun_ups_drain();
        if( ups_drain > 0 ) {
            ammo_count = ammo_count / ups_drain;
        }
        const int cbm_ammo = free_power /  bio.info().power_activate;

        if( weapon_value( weapon, ammo_count ) < weapon_value( cbm_weapon, cbm_ammo ) ) {
            real_weapon = weapon;
            weapon = cbm_weapon;
            cbm_weapon_index = index;
        }
    } else if( bio.info().has_flag( json_flag_BIONIC_WEAPON ) && !weapon.has_flag( flag_NO_UNWIELD ) &&
               free_power > bio.info().power_activate ) {

        if( !bio.has_weapon() ) {
            debugmsg( "NPC tried to activate weapon bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            return;
        }

        if( is_armed() ) {
            stow_item( weapon );
        }
        add_msg_if_player_sees( pos(), m_info, _( "%s activates their %s." ),
                                disp_name(), bio.info().name );

        weapon = bio.get_weapon();
        mod_power_level( -bio.info().power_activate );
        bio.powered = true;
        cbm_weapon_index = index;
    }
}

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
bool Character::activate_bionic( int b, bool eff_only, bool *close_bionics_ui )
{
    bionic &bio = ( *my_bionics )[b];
    const bool mounted = is_mounted();
    if( bio.incapacitated_time > 0_turns ) {
        add_msg( m_info, _( "Your %s is shorting out and can't be activated." ),
                 bio.info().name );
        return false;
    }

    // Special compatibility code for people who updated saves with their claws out
    if( ( weapon.typeId().str() == bio_claws_weapon.str() &&
          bio.id == bio_claws_weapon ) ||
        ( weapon.typeId().str() == bio_blade_weapon.str() &&
          bio.id == bio_blade_weapon ) ) {
        return deactivate_bionic( b );
    }

    // eff_only means only do the effect without messing with stats or displaying messages
    if( !eff_only ) {
        if( bio.powered ) {
            // It's already on!
            return false;
        }
        if( !enough_power_for( bio.id ) ) {
            add_msg_if_player( m_info, _( "You don't have the power to activate your %s." ),
                               bio.info().name );
            return false;
        }

        const auto_toggle_bionic_result result = auto_toggle_bionic( b, true );
        if( result.can_burn_fuel && !result.has_burnable_fuel ) {
            return false;
        }

        if( !bio.activate_spell( *this ) ) {
            // the spell this bionic uses was unable to be cast
            return false;
        }

        // We can actually activate now, do activation-y things
        mod_power_level( -bio.info().power_activate );

        bio.powered = bio.info().has_flag( json_flag_BIONIC_TOGGLED ) || bio.info().charge_time > 0;

        if( bio.info().charge_time > 0 ) {
            bio.charge_timer = bio.info().charge_time;
        }
        if( !bio.id->enchantments.empty() ) {
            recalculate_enchantment_cache();
        }
    }

    auto add_msg_activate = [&]() {
        if( !eff_only ) {
            add_msg_if_player( m_info, _( "You activate your %s." ), bio.info().name );
        }
    };
    auto refund_power = [&]() {
        if( !eff_only ) {
            mod_power_level( bio.info().power_activate );
        }
    };

    item tmp_item;
    avatar &player_character = get_avatar();
    map &here = get_map();
    // On activation effects go here
    if( bio.info().has_flag( json_flag_BIONIC_GUN ) ) {
        if( !bio.has_weapon() ) {
            debugmsg( "tried to activate gun bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            return false;
        }

        add_msg_activate();
        refund_power(); // Power usage calculated later, in avatar_action::fire
        if( close_bionics_ui ) {
            *close_bionics_ui = true;
        }
        avatar_action::fire_ranged_bionic( player_character, bio.get_weapon(),
                                           bio.info().power_activate );
    } else if( bio.info().has_flag( json_flag_BIONIC_WEAPON ) ) {
        if( !bio.has_weapon() ) {
            debugmsg( "tried to activate weapon bionic \"%s\" without fake_weapon",
                      bio.info().id.str() );
            return false;
        }

        if( weapon.has_flag( flag_NO_UNWIELD ) ) {
            cata::optional<int> active_bio_weapon_index = active_bionic_weapon_index();
            if( active_bio_weapon_index && deactivate_bionic( *active_bio_weapon_index, eff_only ) ) {
                // restore state and try again
                refund_power();
                bio.powered = false;
                // note: deep recursion is not possible, as `deactivate_bionic` won't return true second time
                return activate_bionic( b, eff_only, close_bionics_ui );
            }

            add_msg_if_player( m_info, _( "Deactivate your %s first!" ), weapon.tname() );
            refund_power();
            bio.powered = false;
            return false;
        }

        if( !weapon.is_null() ) {
            const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );
            if( !dispose_item( item_location( *this, &weapon ), query ) ) {
                refund_power();
                bio.powered = false;
                return false;
            }
        }

        add_msg_activate();
        weapon = bio.get_weapon();
        weapon.invlet = '#';
        //if( bio.ammo_count > 0 ) {
        //    weapon.ammo_set( bio.ammo_loaded, bio.ammo_count );
        //    avatar_action::fire_wielded_weapon( player_character );
        //}
    } else if( bio.id == bio_evap ) {
        add_msg_activate();
        const w_point weatherPoint = *get_weather().weather_precise;
        int humidity = get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                           g->is_sheltered( player_character.pos() ) );
        // thirst units = 5 mL
        int water_available = std::lround( humidity * 3.0 / 100.0 );
        if( water_available == 0 ) {
            bio.powered = false;
            add_msg_if_player( m_bad, _( "There is not enough humidity in the air for your %s to function." ),
                               bio.info().name );
            return false;
        } else if( water_available == 1 ) {
            add_msg_if_player( m_mixed,
                               _( "Your %s issues a low humidity warning.  Efficiency will be reduced." ),
                               bio.info().name );
        }
    } else if( bio.id == bio_cqb ) {
        add_msg_activate();
        const avatar *you = as_avatar();
        if( you && !martial_arts_data->pick_style( *you ) ) {
            bio.powered = false;
            add_msg_if_player( m_info, _( "You change your mind and turn it off." ) );
            return false;
        }
    } else if( bio.id == bio_resonator ) {
        add_msg_activate();
        //~Sound of a bionic sonic-resonator shaking the area
        sounds::sound( pos(), 30, sounds::sound_t::combat, _( "VRRRRMP!" ), false, "bionic",
                       static_cast<std::string>( bio_resonator ) );
        for( const tripoint &bashpoint : here.points_in_radius( pos(), 1 ) ) {
            here.bash( bashpoint, 110 );
            // Multibash effect, so that doors &c will fall
            here.bash( bashpoint, 110 );
            here.bash( bashpoint, 110 );
        }

        mod_moves( -100 );
    } else if( bio.id == bio_time_freeze ) {
        if( mounted ) {
            refund_power();
            add_msg_if_player( m_info, _( "You cannot activate %s while mounted." ), bio.info().name );
            return false;
        }
        add_msg_activate();

        mod_moves( units::to_kilojoule( get_power_level() ) );
        set_power_level( 0_kJ );
        add_msg_if_player( m_good, _( "Your speed suddenly increases!" ) );
        if( one_in( 3 ) ) {
            add_msg_if_player( m_bad, _( "Your muscles tear with the strain." ) );
            apply_damage( nullptr, bodypart_id( "arm_l" ), rng( 5, 10 ) );
            apply_damage( nullptr, bodypart_id( "arm_r" ), rng( 5, 10 ) );
            apply_damage( nullptr, bodypart_id( "leg_l" ), rng( 7, 12 ) );
            apply_damage( nullptr, bodypart_id( "leg_r" ), rng( 7, 12 ) );
            apply_damage( nullptr, bodypart_id( "torso" ), rng( 5, 15 ) );
        }
        if( one_in( 5 ) ) {
            add_effect( effect_teleglow, rng( 5_minutes, 40_minutes ) );
        }
    } else if( bio.id == bio_teleport ) {
        if( mounted ) {
            refund_power();
            add_msg_if_player( m_info, _( "You cannot activate %s while mounted." ), bio.info().name );
            return false;
        }
        add_msg_activate();

        teleport::teleport( *this );
        mod_moves( -100 );
    } else if( bio.id == bio_blood_anal ) {
        add_msg_activate();
        conduct_blood_analysis();
    } else if( bio.id == bio_blood_filter ) {
        add_msg_activate();
        static const std::vector<efftype_id> removable = {{
                effect_fungus, effect_dermatik, effect_bloodworms,
                effect_tetanus, effect_poison, effect_badpoison, effect_stung,
                effect_pkill1, effect_pkill2, effect_pkill3, effect_pkill_l,
                effect_drunk, effect_cig, effect_high, effect_hallu, effect_visuals,
                effect_pblue, effect_iodine, effect_datura,
                effect_took_xanax, effect_took_prozac, effect_took_prozac_bad,
                effect_took_flumed, effect_antifungal, effect_venom_weaken,
                effect_venom_dmg, effect_paralysepoison
            }
        };

        for( const string_id<effect_type> &eff : removable ) {
            remove_effect( eff );
        }
        // Purging the substance won't remove the fatigue it caused
        force_comedown( get_effect( effect_adrenaline ) );
        force_comedown( get_effect( effect_meth ) );
        set_painkiller( 0 );
        set_stim( 0 );
        mod_moves( -100 );
    } else if( bio.id == bio_torsionratchet ) {
        add_msg_activate();
        add_msg_if_player( m_info, _( "Your torsion ratchet locks onto your joints." ) );
    } else if( bio.id == bio_jointservo ) {
        add_msg_activate();
        add_msg_if_player( m_info, _( "You can now run faster, assisted by joint servomotors." ) );
    } else if( bio.id == bio_lighter ) {
        const cata::optional<tripoint> pnt = choose_adjacent( _( "Start a fire where?" ) );
        if( pnt && here.is_flammable( *pnt ) ) {
            add_msg_activate();
            here.add_field( *pnt, fd_fire, 1 );
            mod_moves( -100 );
        } else {
            refund_power();
            add_msg_if_player( m_info, _( "There's nothing to light there." ) );
            return false;
        }
    } else if( bio.id == bio_geiger ) {
        add_msg_activate();
        add_msg_if_player( m_info, _( "Your radiation level: %d" ), get_rad() );
    } else if( bio.id == bio_radscrubber ) {
        add_msg_activate();
        if( get_rad() > 4 ) {
            mod_rad( -5 );
        } else {
            set_rad( 0 );
        }
    } else if( bio.id == bio_adrenaline ) {
        add_msg_activate();
        if( has_effect( effect_adrenaline ) ) {
            add_msg_if_player( m_bad, _( "Safeguards kick in, and the bionic refuses to activate!" ) );
            refund_power();
            return false;
        } else {
            add_msg_activate();
            add_effect( effect_adrenaline, 20_minutes );
        }
    } else if( bio.id == bio_emp ) {
        if( const cata::optional<tripoint> pnt = choose_adjacent( _( "Create an EMP where?" ) ) ) {
            add_msg_activate();
            explosion_handler::emp_blast( *pnt );
            mod_moves( -100 );
        } else {
            refund_power();
            return false;
        }
    } else if( bio.id == bio_hydraulics ) {
        add_msg_activate();
        add_msg_if_player( m_good, _( "Your muscles hiss as hydraulic strength fills them!" ) );
        //~ Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, sounds::sound_t::activity, _( "HISISSS!" ), false, "bionic",
                       static_cast<std::string>( bio_hydraulics ) );
    } else if( bio.id == bio_water_extractor ) {
        bool no_target = true;
        bool extracted = false;
        for( item &it : here.i_at( pos() ) ) {
            static const units::volume volume_per_water_charge = 500_ml;
            if( it.is_corpse() ) {
                const int avail = it.get_var( "remaining_water", it.volume() / volume_per_water_charge );
                if( avail > 0 ) {
                    no_target = false;
                    if( query_yn( _( "Extract water from the %s" ),
                                  colorize( it.tname(), it.color_in_inventory() ) ) ) {
                        item water( itype_water_clean, calendar::turn, avail );
                        water.set_item_temperature( 0.00001 * it.temperature );
                        if( liquid_handler::consume_liquid( water ) ) {
                            add_msg_activate();
                            extracted = true;
                            it.set_var( "remaining_water", static_cast<int>( water.charges ) );
                        }
                        break;
                    }
                }
            }
        }
        if( no_target ) {
            add_msg_if_player( m_bad, _( "There is no suitable corpse on this tile." ) );
        }
        if( !extracted ) {
            refund_power();
            return false;
        }
    } else if( bio.id == bio_magnet ) {
        add_msg_activate();
        static const std::set<material_id> affected_materials =
        { material_id( "iron" ), material_id( "steel" ), material_id( "hardsteel" ), material_id( "budget_steel" ) };
        // Remember all items that will be affected, then affect them
        // Don't "snowball" by affecting some items multiple times
        std::vector<std::pair<item, tripoint>> affected;
        const units::mass weight_cap = weight_capacity();
        for( const tripoint &p : here.points_in_radius( pos(), 10 ) ) {
            if( p == pos() || !here.has_items( p ) || here.has_flag( ter_furn_flag::TFLAG_SEALED, p ) ) {
                continue;
            }

            map_stack stack = here.i_at( p );
            for( auto it = stack.begin(); it != stack.end(); it++ ) {
                if( it->weight() < weight_cap &&
                    it->made_of_any( affected_materials ) ) {
                    affected.emplace_back( std::make_pair( *it, p ) );
                    stack.erase( it );
                    break;
                }
            }
        }

        for( const std::pair<item, tripoint> &pr : affected ) {
            projectile proj;
            proj.speed  = 50;
            proj.impact = damage_instance::physical( pr.first.weight() / 250_gram, 0, 0, 0 );
            // make the projectile stop one tile short to prevent hitting the player
            proj.range = rl_dist( pr.second, pos() ) - 1;
            proj.proj_effects = {{ "NO_ITEM_DAMAGE", "DRAW_AS_LINE", "NO_DAMAGE_SCALING", "JET" }};

            dealt_projectile_attack dealt = projectile_attack(
                                                proj, pr.second, pos(), dispersion_sources{ 0 }, this );
            here.add_item_or_charges( dealt.end_point, pr.first );
        }

        mod_moves( -100 );
    } else if( bio.id == bio_lockpick ) {
        if( !is_avatar() ) {
            return false;
        }
        cata::optional<tripoint> target = lockpick_activity_actor::select_location( player_character );
        if( target.has_value() ) {
            add_msg_activate();
            assign_activity(
                player_activity( lockpick_activity_actor::use_bionic( here.getabs( *target ) ) ) );
            if( close_bionics_ui ) {
                *close_bionics_ui = true;
            }
        } else {
            refund_power();
            return false;
        }
    } else if( bio.id == bio_flashbang ) {
        add_msg_activate();
        explosion_handler::flashbang( pos(), true );
        mod_moves( -100 );
    } else if( bio.id == bio_shockwave ) {
        add_msg_activate();
        explosion_handler::shockwave( pos(), 3, 4, 2, 8, true );
        add_msg_if_player( m_neutral, _( "You unleash a powerful shockwave!" ) );
        mod_moves( -100 );
    } else if( bio.id == bio_meteorologist ) {
        add_msg_activate();
        // Calculate local wind power
        int vehwindspeed = 0;
        if( optional_vpart_position vp = here.veh_at( pos() ) ) {
            // vehicle velocity in mph
            vehwindspeed = std::abs( vp->vehicle().velocity / 100 );
        }
        const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
        /* cache g->get_temperature( player location ) since it is used twice. No reason to recalc */
        weather_manager &weather = get_weather();
        const int player_local_temp = weather.get_temperature( player_character.pos() );
        /* windpower defined in internal velocity units (=.01 mph) */
        double windpower = 100.0f * get_local_windpower( weather.windspeed + vehwindspeed,
                           cur_om_ter, pos(), weather.winddirection, g->is_sheltered( pos() ) );
        add_msg_if_player( m_info, _( "Temperature: %s." ), print_temperature( player_local_temp ) );
        const w_point weatherPoint = *weather.weather_precise;
        add_msg_if_player( m_info, _( "Relative Humidity: %s." ),
                           print_humidity(
                               get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                       g->is_sheltered( player_character.pos() ) ) ) );
        add_msg_if_player( m_info, _( "Pressure: %s." ),
                           print_pressure( static_cast<int>( weatherPoint.pressure ) ) );
        add_msg_if_player( m_info, _( "Wind Speed: %.1f %s." ),
                           convert_velocity( static_cast<int>( windpower ), VU_WIND ),
                           velocity_units( VU_WIND ) );
        add_msg_if_player( m_info, _( "Feels Like: %s." ),
                           print_temperature(
                               get_local_windchill( weatherPoint.temperature, weatherPoint.humidity,
                                       windpower / 100 ) + player_local_temp ) );
        std::string dirstring = get_dirstring( weather.winddirection );
        add_msg_if_player( m_info, _( "Wind Direction: From the %s." ), dirstring );
    } else if( bio.id == bio_remote ) {
        add_msg_activate();
        int choice = uilist( _( "Perform which function:" ), {
            _( "Control vehicle" ), _( "RC radio" )
        } );
        if( choice >= 0 && choice <= 1 ) {
            item ctr;
            if( choice == 0 ) {
                ctr = item( "remotevehcontrol", calendar::turn_zero );
            } else {
                ctr = item( "radiocontrol", calendar::turn_zero );
            }
            ctr.charges = units::to_kilojoule( get_power_level() );
            int power_use = invoke_item( &ctr );
            mod_power_level( units::from_kilojoule( -power_use ) );
            bio.powered = ctr.active;
        } else {
            bio.powered = g->remoteveh() != nullptr || !get_value( "remote_controlling" ).empty();
        }
    } else if( bio.info().is_remote_fueled ) {
        std::vector<item *> cables = items_with( []( const item & it ) {
            return it.has_flag( flag_CABLE_SPOOL );
        } );
        bool has_cable = !cables.empty();
        bool free_cable = false;
        bool success = false;
        if( !has_cable ) {
            add_msg_if_player( m_info,
                               _( "You need a jumper cable connected to a power source to drain power from it." ) );
        } else {
            for( item *cable : cables ) {
                const std::string state = cable->get_var( "state" );
                if( state == "cable_charger" ) {
                    add_msg_if_player( m_info,
                                       _( "Cable is plugged-in to the CBM but it has to be also connected to the power source." ) );
                }
                if( state == "cable_charger_link" ) {
                    add_msg_activate();
                    success = true;
                    add_msg_if_player( m_info,
                                       _( "You are plugged to the vehicle.  It will charge you if it has some juice in it." ) );
                }
                if( state == "solar_pack_link" ) {
                    add_msg_activate();
                    success = true;
                    add_msg_if_player( m_info,
                                       _( "You are plugged to a solar pack.  It will charge you if it's unfolded and in sunlight." ) );
                }
                if( state == "UPS_link" ) {
                    add_msg_activate();
                    success = true;
                    add_msg_if_player( m_info,
                                       _( "You are plugged to a UPS.  It will charge you if it has some juice in it." ) );
                }
                if( state == "solar_pack" || state == "UPS" ) {
                    add_msg_if_player( m_info,
                                       _( "You have a cable plugged to a portable power source, but you need to plug it in to the CBM." ) );
                }
                if( state == "pay_out_cable" ) {
                    add_msg_if_player( m_info,
                                       _( "You have a cable plugged to a vehicle, but you need to plug it in to the CBM." ) );
                }
                if( state == "attach_first" ) {
                    free_cable = true;
                }
            }

            if( free_cable ) {
                add_msg_if_player( m_info,
                                   _( "You have at least one free cable in your inventory that you could use to plug yourself in." ) );
            }
        }
        if( !success ) {
            refund_power();
            bio.powered = false;
            return false;
        }
    } else {
        add_msg_activate();

        const std::vector<bionic_id> &deactivated_bionics = bio.info().autodeactivated_bionics;
        if( !deactivated_bionics.empty() ) {
            for( bionic &bio : *my_bionics ) {
                if( std::find( deactivated_bionics.begin(), deactivated_bionics.end(),
                               bio.id ) != deactivated_bionics.end() ) {
                    if( bio.powered ) {
                        bio.powered = false;
                        add_msg_if_player( m_info, _( "Your %s automatically turn off." ),
                                           bio.info().name );
                    }
                }
            }
        }
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    calc_encumbrance();
    reset();

    // Also reset crafting inventory cache if this bionic spawned a fake item
    if( bio.has_weapon() || !bio.info().passive_pseudo_items.empty() ||
        !bio.info().toggled_pseudo_items.empty() ) {
        invalidate_pseudo_items();
        invalidate_crafting_inventory();
    }

    return true;
}

cata::optional<int> Character::active_bionic_weapon_index() const
{
    if( weapon.is_null() ) {
        return cata::nullopt;
    }

    for( int i = 0; i < static_cast<int>( my_bionics->size() ); i++ ) {
        const bionic &bio = ( *my_bionics )[ i ];
        // TODO: Better match weapons to their CBM
        if( bio.powered && !bio.info().fake_weapon.is_empty() && !bio.info().fake_weapon.is_null() &&
            weapon.typeId() == bio.info().fake_weapon ) {
            return i;
        }
    }

    return cata::nullopt;
}

ret_val<bool> Character::can_deactivate_bionic( int b, bool eff_only ) const
{
    bionic &bio = ( *my_bionics )[b];

    if( bio.incapacitated_time > 0_turns ) {
        return ret_val<bool>::make_failure( _( "Your %s is shorting out and can't be deactivated." ),
                                            bio.info().name );
    }

    if( !eff_only ) {
        if( !bio.powered ) {
            // It's already off!
            return ret_val<bool>::make_failure();
        }
        if( !bio.info().has_flag( json_flag_BIONIC_TOGGLED ) ) {
            // It's a fire-and-forget bionic, we can't turn it off but have to wait for
            //it to run out of charge
            return ret_val<bool>::make_failure( _( "You can't deactivate your %s manually!" ),
                                                bio.info().name );
        }
        if( get_power_level() < bio.info().power_deactivate ) {
            return ret_val<bool>::make_failure( _( "You don't have the power to deactivate your %s." ),
                                                bio.info().name );
        }
    }

    return ret_val<bool>::make_success();
}

bool Character::deactivate_bionic( int b, bool eff_only )
{
    const auto can_deactivate = can_deactivate_bionic( b, eff_only );

    if( !can_deactivate.success() ) {
        if( !can_deactivate.str().empty() ) {
            add_msg( m_info,  can_deactivate.str() );
        }
        return false;
    }

    bionic &bio = ( *my_bionics )[b];

    if( bio.info().is_remote_fueled ) {
        reset_remote_fuel();
    }

    // Just do the effect, no stat changing or messages
    if( !eff_only ) {
        //We can actually deactivate now, do deactivation-y things
        mod_power_level( -bio.info().power_deactivate );
        bio.powered = false;
        add_msg_if_player( m_neutral, _( "You deactivate your %s." ), bio.info().name );
    }

    // Deactivation effects go here
    if( bio.info().has_flag( json_flag_BIONIC_WEAPON ) && !bio.info().fake_weapon.is_empty() ) {
        if( weapon.typeId() == bio.info().fake_weapon ) {
            add_msg_if_player( _( "You withdraw your %s." ), weapon.tname() );
            if( get_player_view().sees( pos() ) ) {
                if( male ) {
                    add_msg_if_npc( m_info, _( "<npcname> withdraws his %s." ), weapon.tname() );
                } else {
                    add_msg_if_npc( m_info, _( "<npcname> withdraws her %s." ), weapon.tname() );
                }
            }
            bio.set_weapon( weapon );
            weapon = item();
        }
    } else if( bio.id == bio_cqb ) {
        martial_arts_data->selected_style_check();
    } else if( bio.id == bio_remote ) {
        if( g->remoteveh() != nullptr && !has_active_item( itype_remotevehcontrol ) ) {
            g->setremoteveh( nullptr );
        } else if( !get_value( "remote_controlling" ).empty() &&
                   !has_active_item( itype_radiocontrol ) ) {
            set_value( "remote_controlling", "" );
        }
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    calc_encumbrance();
    reset();
    if( !bio.id->enchantments.empty() ) {
        recalculate_enchantment_cache();
    }

    // Also reset crafting inventory cache if this bionic spawned a fake item
    if( bio.has_weapon() || !bio.info().passive_pseudo_items.empty() ||
        !bio.info().toggled_pseudo_items.empty() ) {
        invalidate_pseudo_items();
        invalidate_crafting_inventory();
    }

    // Compatibility with old saves without the toolset hammerspace
    if( !eff_only && bio.id == bio_tools && !has_bionic( bio_tools_extend ) ) {
        // E X T E N D    T O O L S
        add_bionic( bio_tools_extend );
    }

    return true;
}

Character::auto_toggle_bionic_result Character::auto_toggle_bionic( const int b, const bool start )
{
    auto_toggle_bionic_result result;
    bionic &bio = ( *my_bionics )[b];
    if( bio.info().fuel_opts.empty() && !bio.info().is_remote_fueled ) {
        return result;
    }
    result.can_burn_fuel = true;
    std::vector<material_id> fuel_available = get_fuel_available( bio.id );

    const bool is_remote_fueled = bio.info().is_remote_fueled;
    material_id remote_fuel;
    if( is_remote_fueled ) {
        remote_fuel = find_remote_fuel();
        if( !remote_fuel.is_empty() ) {
            fuel_available.emplace_back( remote_fuel );
        }
    }

    bool toggle_off = false;
    bool keep_off = false;
    if( fuel_available.empty() ) {
        if( bio.powered || start ) {
            if( start ) {
                add_msg_player_or_npc( m_bad, _( "Your %s does not have enough fuel to start." ),
                                       _( "<npcname>'s %s does not have enough fuel to start." ),
                                       bio.info().name );
                if( bio.powered ) {
                    deactivate_bionic( b );
                }
            } else {
                add_msg_player_or_npc( m_info,
                                       _( "Your %s runs out of fuel and turns off." ),
                                       _( "<npcname>'s %s runs out of fuel and turns off." ),
                                       bio.info().name );
                if( bio.powered ) {
                    bio.powered = false;
                    deactivate_bionic( b, true );
                }
            }
            toggle_off = true;
        } else {
            keep_off = true;
        }
    } else {
        std::string msg_player;
        std::string msg_npc;
        for( const material_id &fuel : fuel_available ) {
            const int fuel_energy = fuel->get_fuel_data().energy;
            const bool is_metabolism_powered = fuel == fuel_type_metabolism;
            const bool is_perpetual_fuel = fuel->get_fuel_data().is_perpetual_fuel;
            const bool is_remote_fuel = is_remote_fueled && fuel == remote_fuel;
            float effective_efficiency = get_effective_efficiency( b, bio.info().fuel_efficiency );
            if( is_remote_fuel && fuel == fuel_type_sun_light ) {
                effective_efficiency *= item_worn_with_flag( flag_SOLARPACK_ON ).type->solar_efficiency;
            }
            int current_fuel_stock = 0;
            if( is_metabolism_powered ) {
                current_fuel_stock = std::max( 0.0f, get_stored_kcal() - 0.8f *
                                               get_healthy_kcal() );
            } else if( is_perpetual_fuel ) {
                current_fuel_stock = 1;
            } else if( is_remote_fuel ) {
                current_fuel_stock = std::stoi( get_value( "rem_" + fuel.str() ) );
                if( current_fuel_stock <= 0 ) {
                    remove_value( "rem_" + fuel.str() );
                }
            } else {
                current_fuel_stock = std::stoi( get_value( fuel.str() ) );
                if( current_fuel_stock <= 0 ) {
                    remove_value( fuel.str() );
                }
            }

            if( result.has_burnable_fuel ) {
                // if we already found a burnable fuel we can skip the following
                // code which only generates fuel-not-found messages and assigns
                // found fuel to the result. code before this has side-effects
                // so cannot be skipped.
                continue;
            }

            if( bio.get_safe_fuel_thresh() > 0
                && get_power_level() + units::from_kilojoule( fuel_energy ) * effective_efficiency >
                get_max_power_level() * std::min( 1.0f, bio.get_safe_fuel_thresh() ) ) {
                if( bio.powered || start ) {
                    if( !start ) {
                        if( is_metabolism_powered ) {
                            msg_player = _( "Your %s turns off to not waste calories." );
                            msg_npc = _( "<npcname>'s %s turns off to not waste calories." );
                        } else if( is_perpetual_fuel ) {
                            msg_player = _( "Your %s turns off after filling your power banks." );
                            msg_npc = _( "<npcname>'s %s turns off after filling their power banks." );
                        } else {
                            msg_player = _( "Your %s turns off to not waste fuel." );
                            msg_npc = _( "<npcname>'s %s turns off to not waste fuel." );
                        }
                    } else if( get_max_power_level() == 0_mJ ) {
                        msg_player = _( "Your %s cannot be started because you don't have any bionic power storage." );
                        msg_npc = _( "<npcname>'s %s cannot be started because they don't have any bionic power storage." );
                    } else if( get_power_level() != get_max_power_level() ) {
                        msg_player = _( "Your %s cannot be started due to fuel saving." );
                        msg_npc = _( "<npcname>'s %s cannot be started due to fuel saving." );
                    } else {
                        msg_player = _( "Your %s cannot be started because your power banks are full." );
                        msg_npc = _( "<npcname>'s %s cannot be started because their power banks are full." );
                    }
                }
            } else if( current_fuel_stock <= 0 ) {
                if( bio.powered || start ) {
                    if( !start ) {
                        if( is_metabolism_powered ) {
                            msg_player =
                                _( "Stored calories are below the safe threshold, your %s shuts down to preserve your health." );
                            msg_npc = _( "Stored calories are below the safe threshold, <npcname>'s %s shuts down to preserve their health." );
                        } else {
                            msg_player = _( "Your %s runs out of fuel and turns off." );
                            msg_npc = _( "<npcname>'s %s runs out of fuel and turns off." );
                        }
                    } else {
                        if( is_metabolism_powered ) {
                            msg_player = _( "Your %s cannot be started because your calories are below safe levels." );
                            msg_npc = _( "<npcname>'s %s cannot be started because their calories are below safe levels." );
                        } else {
                            msg_player = _( "Your %s doesn't have enough fuel to start." );
                            msg_npc = _( "<npcname>'s %s doesn't have enough fuel to start." );
                        }
                    }
                }
            } else {
                result.has_burnable_fuel = true;
                result.burnable_fuel_id = fuel;
                if( is_metabolism_powered ) {
                    result.fuel_type = auto_toggle_bionic_result::fuel_type_t::metabolism;
                } else if( is_perpetual_fuel ) {
                    result.fuel_type = auto_toggle_bionic_result::fuel_type_t::perpetual;
                } else if( is_remote_fuel ) {
                    result.fuel_type = auto_toggle_bionic_result::fuel_type_t::remote;
                } else {
                    result.fuel_type = auto_toggle_bionic_result::fuel_type_t::other;
                }
                result.fuel_energy = fuel_energy;
                result.effective_efficiency = effective_efficiency;
                result.current_fuel_stock = current_fuel_stock;
            }
        }
        if( !result.has_burnable_fuel ) {
            if( bio.powered || start ) {
                add_msg_player_or_npc( m_info, msg_player, msg_npc, bio.info().name );
                if( bio.powered ) {
                    bio.powered = false;
                    deactivate_bionic( b, true );
                }
                toggle_off = true;
            } else {
                keep_off = true;
            }
        }
    }

    if( !toggle_off && !bio.powered && !start && bio.is_auto_start_on() ) {
        const float start_threshold = bio.get_auto_start_thresh();
        if( get_power_level() <= start_threshold * get_max_power_level() ) {
            if( !keep_off ) {
                activate_bionic( b );
            } else if( calendar::once_every( 1_hours ) ) {
                add_msg_player_or_npc( m_bad, _( "Your %s does not have enough fuel to use Auto Start." ),
                                       _( "<npcname>'s %s does not have enough fuel to use Auto Start." ),
                                       bio.info().name );
            }
        }
    }

    return result;
}

void Character::burn_fuel( const int b, const auto_toggle_bionic_result &result )
{
    bionic &bio = ( *my_bionics )[b];
    if( !bio.powered || !result.can_burn_fuel || !result.has_burnable_fuel ) {
        return;
    }

    map &here = get_map();
    weather_manager &weather = get_weather();
    switch( result.fuel_type ) {
        case auto_toggle_bionic_result::fuel_type_t::metabolism: {
            const int kcal_consumed = result.fuel_energy;
            // 1kcal = 4184 J
            const units::energy power_gain = kcal_consumed * 4184_J * result.effective_efficiency;
            mod_stored_kcal( -kcal_consumed, true );
            mod_power_level( power_gain );
            break;
        }
        case auto_toggle_bionic_result::fuel_type_t::perpetual:
            if( result.burnable_fuel_id == fuel_type_sun_light && g->is_in_sunlight( pos() ) ) {
                const weather_type_id &wtype = current_weather( pos() );
                const float tick_sunlight = incident_sunlight( wtype, calendar::turn );
                const double intensity = tick_sunlight / default_daylight_level();
                mod_power_level( units::from_kilojoule( result.fuel_energy ) * intensity *
                                 result.effective_efficiency );
            } else if( result.burnable_fuel_id == fuel_type_wind ) {
                int vehwindspeed = 0;
                const optional_vpart_position vp = here.veh_at( pos() );
                if( vp ) {
                    // vehicle velocity in mph
                    vehwindspeed = std::abs( vp->vehicle().velocity / 100 );
                }
                const double windpower = get_local_windpower( weather.windspeed + vehwindspeed,
                                         overmap_buffer.ter( global_omt_location() ), pos(), weather.winddirection,
                                         g->is_sheltered( pos() ) );
                mod_power_level( units::from_kilojoule( result.fuel_energy ) * windpower *
                                 result.effective_efficiency );
            } else if( result.burnable_fuel_id == fuel_type_muscle ) {
                // simply return
            }
            break;
        case auto_toggle_bionic_result::fuel_type_t::remote: {
            const int unconsumed = consume_remote_fuel( 1 );
            int current_fuel_stock = result.current_fuel_stock;
            if( unconsumed == 0 ) {
                mod_power_level( units::from_kilojoule( result.fuel_energy ) * result.effective_efficiency );
                current_fuel_stock -= 1;
            } else {
                current_fuel_stock = 0;
            }
            set_value( "rem_" + result.burnable_fuel_id.str(), std::to_string( current_fuel_stock ) );
            break;
        }
        case auto_toggle_bionic_result::fuel_type_t::other:
            set_value( result.burnable_fuel_id.str(), std::to_string( result.current_fuel_stock - 1 ) );
            update_fuel_storage( result.burnable_fuel_id );
            mod_power_level( units::from_kilojoule( result.fuel_energy ) * result.effective_efficiency );
            break;
    }

    heat_emission( b, result.fuel_energy );
    here.emit_field( pos(), bio.info().power_gen_emission );
}

void Character::passive_power_gen( int b )
{
    const bionic &bio = ( *my_bionics )[b];
    const float passive_fuel_efficiency = bio.info().passive_fuel_efficiency;
    if( bio.info().fuel_opts.empty() || bio.is_this_fuel_powered( fuel_type_muscle ) ||
        passive_fuel_efficiency == 0.0 ) {
        return;
    }
    const float effective_passive_efficiency = get_effective_efficiency( b, passive_fuel_efficiency );
    const std::vector<material_id> &fuel_available = get_fuel_available( bio.id );
    map &here = get_map();
    weather_manager &weather = get_weather();

    for( const material_id &fuel : fuel_available ) {
        const int fuel_energy = fuel->get_fuel_data().energy;
        if( !fuel->get_fuel_data().is_perpetual_fuel ) {
            continue;
        }

        if( fuel == fuel_type_sun_light ) {
            const double modifier = g->natural_light_level( pos().z ) / default_daylight_level();
            mod_power_level( units::from_kilojoule( fuel_energy ) * modifier * effective_passive_efficiency );
        } else if( fuel == fuel_type_wind ) {
            int vehwindspeed = 0;
            const optional_vpart_position vp = here.veh_at( pos() );
            if( vp ) {
                // vehicle velocity in mph
                vehwindspeed = std::abs( vp->vehicle().velocity / 100 );
            }
            const double windpower = get_local_windpower( weather.windspeed + vehwindspeed,
                                     overmap_buffer.ter( global_omt_location() ), pos(), weather.winddirection,
                                     g->is_sheltered( pos() ) );
            mod_power_level( units::from_kilojoule( fuel_energy ) * windpower * effective_passive_efficiency );
        } else {
            mod_power_level( units::from_kilojoule( fuel_energy ) * effective_passive_efficiency );
        }

        heat_emission( b, fuel_energy );
        here.emit_field( pos(), bio.info().power_gen_emission );

    }
}

material_id Character::find_remote_fuel( bool look_only )
{
    material_id remote_fuel;
    map &here = get_map();

    const std::vector<item *> cables = items_with( []( const item & it ) {
        return it.active && it.has_flag( flag_CABLE_SPOOL );
    } );

    for( const item *cable : cables ) {

        const cata::optional<tripoint> target = cable->get_cable_target( this, pos() );
        if( !target ) {
            if( here.is_outside( pos() ) && !is_night( calendar::turn ) &&
                cable->get_var( "state" ) == "solar_pack_link" ) {
                if( !look_only ) {
                    set_value( "sunlight", "1" );
                }
                remote_fuel = fuel_type_sun_light;
            }

            if( cable->get_var( "state" ) == "UPS_link" ) {
                if( !look_only ) {
                    int remote_battery = 0;
                    for( const item *i : all_items_with_flag( flag_IS_UPS ) ) {
                        if( i->get_var( "cable" ) == "plugged_in" ) {
                            remote_battery = i->ammo_remaining();
                        }
                    }
                    remote_battery = std::min( remote_battery, units::to_kilojoule( max_power_level ) );
                    set_value( "rem_battery", std::to_string( remote_battery ) );
                }
                remote_fuel = fuel_type_battery;
            }
            continue;
        }
        const optional_vpart_position vp = here.veh_at( *target );
        if( !vp ) {
            continue;
        }
        if( !look_only ) {
            set_value( "rem_battery", std::to_string( vp->vehicle().fuel_left( itype_id( "battery" ),
                       true ) ) );
        }
        remote_fuel = fuel_type_battery;
    }

    return remote_fuel;
}

int Character::consume_remote_fuel( int amount )
{
    int unconsumed_amount = amount;
    const std::vector<item *> cables = items_with( []( const item & it ) {
        return it.active && it.has_flag( flag_CABLE_SPOOL );
    } );

    map &here = get_map();
    for( const item *cable : cables ) {
        const cata::optional<tripoint> target = cable->get_cable_target( this, pos() );
        if( target ) {
            const optional_vpart_position vp = here.veh_at( *target );
            if( !vp ) {
                continue;
            }
            unconsumed_amount = vp->vehicle().discharge_battery( amount );
        }
    }

    if( unconsumed_amount > 0 ) {
        for( const item *i : all_items_with_flag( flag_IS_UPS ) ) {
            if( i->get_var( "cable" ) == "plugged_in" ) {
                unconsumed_amount -= const_cast<item *>( i )->ammo_consume( unconsumed_amount, tripoint_zero,
                                     nullptr );
            }
            break;
        }
    }

    return unconsumed_amount;
}

void Character::reset_remote_fuel()
{
    if( get_bionic_fueled_with( fuel_type_sun_light ).empty() ) {
        remove_value( "sunlight" );
    }
    remove_value( "rem_battery" );
}

void Character::heat_emission( int b, int fuel_energy )
{
    const bionic &bio = ( *my_bionics )[b];
    if( !bio.info().exothermic_power_gen ) {
        return;
    }
    const float efficiency = bio.info().fuel_efficiency;

    const int heat_prod = fuel_energy * ( 1.0f - efficiency );
    const int heat_level = std::min( heat_prod / 10, 4 );
    const emit_id hotness = emit_id( "emit_hot_air" + std::to_string( heat_level ) + "_cbm" );
    map &here = get_map();
    if( hotness.is_valid() ) {
        const int heat_spread = std::max( heat_prod / 10 - heat_level, 1 );
        here.emit_field( pos(), hotness, heat_spread );
    }
    for( const std::pair<const bodypart_str_id, size_t> &bp : bio.info().occupied_bodyparts ) {
        add_effect( effect_heating_bionic, 2_seconds, bp.first.id(), false, heat_prod );
    }
}

float Character::get_effective_efficiency( int b, float fuel_efficiency )
{
    const bionic &bio = ( *my_bionics )[b];
    const cata::optional<float> &coverage_penalty = bio.info().coverage_power_gen_penalty;
    float effective_efficiency = fuel_efficiency;
    if( coverage_penalty ) {
        int coverage = 0;
        const std::map< bodypart_str_id, size_t > &occupied_bodyparts = bio.info().occupied_bodyparts;
        for( const std::pair< const bodypart_str_id, size_t > &elem : occupied_bodyparts ) {
            for( const item &i : worn ) {
                if( i.covers( elem.first ) && !i.has_flag( flag_ALLOWS_NATURAL_ATTACKS ) &&
                    !i.has_flag( flag_SEMITANGIBLE ) &&
                    !i.has_flag( flag_PERSONAL ) && !i.has_flag( flag_AURA ) ) {
                    coverage += i.get_coverage( elem.first.id() );
                }
            }
        }
        effective_efficiency = fuel_efficiency * ( 1.0 - ( coverage / ( 100.0 *
                               occupied_bodyparts.size() ) )
                               * coverage_penalty.value() );
    }
    return effective_efficiency;
}

/**
 * @param p the player
 * @param bio the bionic that is meant to be recharged.
 * @param amount the amount of power that is to be spent recharging the bionic.
 * @return indicates whether we successfully charged the bionic.
 */
static bool attempt_recharge( Character &p, bionic &bio, units::energy &amount )
{
    const bionic_data &info = bio.info();
    units::energy power_cost = info.power_over_time;
    bool recharged = false;

    if( power_cost > 0_kJ ) {
        if( info.has_flag( STATIC( json_character_flag( "BIONIC_ARMOR_INTERFACE" ) ) ) ) {
            // Don't spend any power on armor interfacing unless we're wearing active powered armor.
            bool powered_armor = std::any_of( p.worn.begin(), p.worn.end(),
            []( const item & w ) {
                return w.active && w.is_power_armor();
            } );
            if( !powered_armor ) {
                const units::energy armor_power_cost = 1_kJ;
                power_cost -= armor_power_cost;
            }
        }
        if( p.get_power_level() >= power_cost ) {
            // Set the recharging cost and charge the bionic.
            amount = power_cost;
            bio.charge_timer = info.charge_time;
            recharged = true;
        }
    }

    return recharged;
}

void Character::process_bionic( const int b )
{
    bionic &bio = ( *my_bionics )[b];
    const auto_toggle_bionic_result result = auto_toggle_bionic( b, false );

    // Only powered bionics should be processed
    if( !bio.powered ) {
        passive_power_gen( b );
        return;
    }

    // These might be affected by environmental conditions, status effects, faulty bionics, etc.
    int discharge_rate = 1;

    units::energy cost = 0_mJ;

    bio.charge_timer = std::max( 0, bio.charge_timer - discharge_rate );
    if( bio.charge_timer <= 0 ) {
        if( bio.info().charge_time > 0 ) {
            if( bio.info().has_flag( STATIC( json_character_flag( "BIONIC_POWER_SOURCE" ) ) ) ) {
                // Convert fuel to bionic power
                burn_fuel( b, result );
                // Reset timer
                bio.charge_timer = bio.info().charge_time;
            } else {
                // Try to recharge our bionic if it is made for it
                bool recharged = attempt_recharge( *this, bio, cost );
                if( !recharged ) {
                    // No power to recharge, so deactivate
                    bio.powered = false;
                    add_msg_if_player( m_neutral, _( "Your %s powers down." ), bio.info().name );
                    // This purposely bypasses the deactivation cost
                    deactivate_bionic( b, true );
                    return;
                }
                if( cost > 0_mJ ) {
                    mod_power_level( -cost );
                }
            }
        }
    }

    // Bionic effects on every turn they are active go here.
    if( bio.id == bio_remote ) {
        if( g->remoteveh() == nullptr && get_value( "remote_controlling" ).empty() ) {
            bio.powered = false;
            add_msg_if_player( m_warning, _( "Your %s has lost connection and is turning off." ),
                               bio.info().name );
        }
    } else if( bio.id == bio_hydraulics ) {
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, sounds::sound_t::activity, _( "HISISSS!" ), false, "bionic",
                       static_cast<std::string>( bio_hydraulics ) );
    } else if( bio.id == bio_nanobots ) {
        std::forward_list<bodypart_id> bleeding_bp_parts;
        for( const bodypart_id &bp : get_all_body_parts() ) {
            if( has_effect( effect_bleed, bp.id() ) ) {
                bleeding_bp_parts.push_front( bp );
            }
        }
        std::vector<bodypart_id> damaged_hp_parts;
        for( const std::pair<const bodypart_str_id, bodypart> &part : get_body() ) {
            const int hp_cur = part.second.get_hp_cur();
            if( hp_cur > 0 && hp_cur < part.second.get_hp_max() ) {
                damaged_hp_parts.push_back( part.first.id() );
            }
        }
        if( damaged_hp_parts.empty() && bleeding_bp_parts.empty() ) {
            // Nothing to heal. Return the consumed power and exit early
            mod_power_level( cost );
            return;
        }
        for( const bodypart_id &i : bleeding_bp_parts ) {
            // effectively reduces by 1 intensity level
            if( get_stored_kcal() >= 15 ) {
                get_effect( effect_bleed, i ).mod_duration( -get_effect( effect_bleed, i ).get_int_dur_factor() );
                mod_stored_kcal( -15 );
            } else {
                bleeding_bp_parts.clear();
                break;
            }
        }
        if( calendar::once_every( 60_turns ) ) {
            if( get_stored_kcal() >= 5 && !damaged_hp_parts.empty() ) {
                const bodypart_id part_to_heal = damaged_hp_parts[ rng( 0, damaged_hp_parts.size() - 1 ) ];
                heal( part_to_heal, 1 );
                mod_stored_kcal( -5 );
            }
        }
    } else if( bio.id == bio_painkiller ) {
        const int pkill = get_painkiller();
        const int pain = get_pain();
        const units::energy trigger_cost = bio.info().power_trigger;
        int max_pkill = std::min( 150, pain );
        if( pkill < max_pkill ) {
            mod_painkiller( 1 );
            mod_power_level( -trigger_cost );
        }

        // Only dull pain so extreme that we can't pkill it safely
        if( pkill >= 150 && pain > pkill && get_stim() > -150 ) {
            mod_pain( -1 );
            // Negative side effect: negative stim
            mod_stim( -1 );
            mod_power_level( -trigger_cost );
        }
    } else if( bio.id == bio_gills ) {
        if( has_effect( effect_asthma ) ) {
            add_msg_if_player( m_good,
                               _( "You feel your throat open up and air filling your lungs!" ) );
            remove_effect( effect_asthma );
        }
    } else if( bio.id == bio_evap ) {
        // Aero-Evaporator provides water at 60 watts with 2 L / kWh efficiency
        // which is 10 mL per 5 minutes.  Humidity can modify the amount gained.
        if( calendar::once_every( 5_minutes ) ) {
            const w_point weatherPoint = *get_weather().weather_precise;
            int humidity = get_local_humidity( weatherPoint.humidity, get_weather().weather_id,
                                               g->is_sheltered( pos() ) );
            // in thirst units = 5 mL water
            int water_available = std::lround( humidity * 3.0 / 100.0 );
            // At 50% relative humidity or more, the player will draw 10 mL
            // At 16% relative humidity or less, the bionic will give up
            if( water_available == 0 ) {
                add_msg_if_player( m_bad,
                                   _( "There is not enough humidity in the air for your %s to function." ),
                                   bio.info().name );
                deactivate_bionic( b );
            } else if( water_available == 1 ) {
                add_msg_if_player( m_mixed,
                                   _( "Your %s issues a low humidity warning.  Efficiency is reduced." ),
                                   bio.info().name );
            }

            mod_thirst( -water_available );
        }

        if( get_thirst() < -40 ) {
            add_msg_if_player( m_good,
                               _( "You are properly hydrated.  Your %s chirps happily." ),
                               bio.info().name );
            deactivate_bionic( b );
        }
    } else if( bio.id == afs_bio_dopamine_stimulators ) {
        // Aftershock
        add_morale( MORALE_FEELING_GOOD, 20, 20, 30_minutes, 20_minutes, true );
    }
}

void Character::roll_critical_bionics_failure( const bodypart_id &bp )
{
    const bodypart_id bp_to_hurt = bp->main_part;
    if( one_in( get_part_hp_cur( bp_to_hurt ) / 4 ) ) {
        set_part_hp_cur( bp_to_hurt, 0 );
    }
}

void Character::bionics_uninstall_failure( int difficulty, int success, float adjusted_skill )
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful removal.  We use this to determine consequences for failing.
    success = std::abs( success );

    // failure level is decided by how far off the character was from a successful removal, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    const int failure_level = static_cast<int>( std::sqrt( success * 4.0 * difficulty /
                              adjusted_skill ) );
    const int fail_type = std::min( 5, failure_level );

    if( fail_type <= 0 ) {
        add_msg( m_neutral, _( "The removal fails without incident." ) );
        return;
    }

    add_msg( m_neutral, _( "The removal is a failure." ) );
    std::set<bodypart_id> bp_hurt;
    switch( fail_type ) {
        case 1:
            if( !has_trait( trait_NOPAIN ) ) {
                add_msg_if_player( m_bad, _( "It really hurts!" ) );
                mod_pain( rng( 10, 30 ) );
            }
            break;

        case 2:
        case 3:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );
                    apply_damage( this, bp, rng( 5, 10 ), true );
                    add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                           body_part_name_accusative( bp ) );
                }
            }
            break;

        case 4:
        case 5:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );

                    apply_damage( this, bp, rng( 25, 50 ), true );
                    roll_critical_bionics_failure( bp );

                    add_msg_player_or_npc( m_bad, _( "Your %s is severely damaged." ),
                                           _( "<npcname>'s %s is severely damaged." ),
                                           body_part_name_accusative( bp ) );
                }
            }
            break;
    }

}

void Character::bionics_uninstall_failure( monster &installer, Character &patient, int difficulty,
        int success, float adjusted_skill )
{

    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful removal.  We use this to determine consequences for failing.
    success = std::abs( success );

    // failure level is decided by how far off the monster was from a successful removal, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    const int failure_level = static_cast<int>( std::sqrt( success * 4.0 * difficulty /
                              adjusted_skill ) );
    const int fail_type = std::min( 5, failure_level );

    bool u_see = sees( patient );

    if( u_see || patient.is_avatar() ) {
        if( fail_type <= 0 ) {
            add_msg( m_neutral, _( "The removal fails without incident." ) );
            return;
        }
        switch( rng( 1, 5 ) ) {
            case 1:
                add_msg( m_mixed, _( "The %s flub the operation." ), installer.name() );
                break;
            case 2:
                add_msg( m_mixed, _( "The %s messes up the operation." ), installer.name() );
                break;
            case 3:
                add_msg( m_mixed, _( "The operation fails." ) );
                break;
            case 4:
                add_msg( m_mixed, _( "The operation is a failure." ) );
                break;
            case 5:
                add_msg( m_mixed, _( "The %s screws up the operation." ), installer.name() );
                break;
        }
    }
    std::set<bodypart_id> bp_hurt;
    switch( fail_type ) {
        case 1:
            if( !has_trait( trait_NOPAIN ) ) {
                patient.add_msg_if_player( m_bad, _( "It really hurts!" ) );
                patient.mod_pain( rng( 10, 30 ) );
            }
            break;

        case 2:
        case 3:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );
                    patient.apply_damage( this, bp, rng( failure_level, failure_level * 2 ), true );
                    if( u_see ) {
                        patient.add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                                       body_part_name_accusative( bp ) );
                    }
                }
            }
            break;

        case 4:
        case 5:
            for( const bodypart_id &bp : get_all_body_parts() ) {
                if( has_effect( effect_under_operation, bp.id() ) ) {
                    if( bp_hurt.count( bp->main_part ) > 0 ) {
                        continue;
                    }
                    bp_hurt.emplace( bp->main_part );

                    patient.apply_damage( this, bp, rng( 25, 50 ), true );
                    roll_critical_bionics_failure( bp );

                    if( u_see ) {
                        patient.add_msg_player_or_npc( m_bad, _( "Your %s is severely damaged." ),
                                                       _( "<npcname>'s %s is severely damaged." ),
                                                       body_part_name_accusative( bp ) );
                    }
                }
            }
            break;
    }
}

bool Character::has_enough_anesth( const itype &cbm, Character &patient )
{
    if( !cbm.bionic ) {
        debugmsg( "has_enough_anesth( const itype *cbm ): %s is not a bionic", cbm.get_id().str() );
        return false;
    }

    if( patient.has_bionic( bio_painkiller ) || patient.has_trait( trait_NOPAIN ) ||
        has_trait( trait_DEBUG_BIONICS ) ) {
        return true;
    }

    const int weight = units::to_kilogram( patient.bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                        cbm.bionic->difficulty * 2 * weight;

    return req_anesth.can_make_with_inventory( crafting_inventory(), is_crafting_component );
}

bool Character::has_enough_anesth( const itype &cbm )
{
    if( has_bionic( bio_painkiller ) || has_trait( trait_NOPAIN ) ||
        has_trait( trait_DEBUG_BIONICS ) ) {
        return true;
    }
    const int weight = units::to_kilogram( bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                        cbm.bionic->difficulty * 2 * weight;
    if( !req_anesth.can_make_with_inventory( crafting_inventory(),
            is_crafting_component ) ) {
        std::string buffer = _( "You don't have enough anesthetic to perform the installation." );
        buffer += "\n";
        buffer += req_anesth.list_missing();
        popup( buffer, PF_NONE );
        return false;
    }
    return true;
}

void Character::consume_anesth_requirement( const itype &cbm, Character &patient )
{
    const int weight = units::to_kilogram( patient.bodyweight() ) / 10;
    const requirement_data req_anesth = *requirement_id( "anesthetic" ) *
                                        cbm.bionic->difficulty * 2 * weight;
    for( const auto &e : req_anesth.get_components() ) {
        consume_items( e, 1, is_crafting_component );
    }
    for( const auto &e : req_anesth.get_tools() ) {
        consume_tools( e );
    }
    invalidate_crafting_inventory();
}

bool Character::has_installation_requirement( const bionic_id &bid )
{
    if( bid->installation_requirement.is_empty() ) {
        return false;
    }

    if( !bid->installation_requirement->can_make_with_inventory( crafting_inventory(),
            is_crafting_component ) ) {
        std::string buffer = _( "You don't have the required components to perform the installation." );
        buffer += "\n";
        buffer += bid->installation_requirement->list_missing();
        popup( buffer, PF_NONE );
        return false;
    }

    return true;
}

void Character::consume_installation_requirement( const bionic_id &bid )
{
    for( const auto &e : bid->installation_requirement->get_components() ) {
        consume_items( e, 1, is_crafting_component );
    }
    for( const auto &e : bid->installation_requirement->get_tools() ) {
        consume_tools( e );
    }
    invalidate_crafting_inventory();
}

// bionic manipulation adjusted skill
float Character::bionics_adjusted_skill( bool autodoc, int skill_level ) const
{
    int pl_skill = bionics_pl_skill( autodoc, skill_level );

    // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                           static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>( 10.0 ) );
    adjusted_skill *= env_surgery_bonus( 1 ) + get_effect_int( effect_assisted );
    return adjusted_skill;
}

int Character::bionics_pl_skill( bool autodoc, int skill_level ) const
{
    skill_id most_important_skill;
    skill_id important_skill;
    skill_id least_important_skill;

    if( autodoc ) {
        most_important_skill = skill_firstaid;
        important_skill = skill_computer;
        least_important_skill = skill_electronics;
    } else {
        most_important_skill = skill_electronics;
        important_skill = skill_firstaid;
        least_important_skill = skill_mechanics;
    }

    int pl_skill;
    if( skill_level == -1 ) {
        pl_skill = int_cur                                  * 4 +
                   get_skill_level( most_important_skill )  * 4 +
                   get_skill_level( important_skill )       * 3 +
                   get_skill_level( least_important_skill ) * 1;
    } else {
        // override chance as though all values were skill_level if it is provided
        pl_skill = 12 * skill_level;
    }

    // Medical residents have some idea what they're doing
    if( has_trait( trait_PROF_MED ) ) {
        pl_skill += 3;
    }

    // People trained in bionics gain an additional advantage towards using it
    if( has_trait( trait_PROF_AUTODOC ) ) {
        pl_skill += 7;
    }
    return pl_skill;
}

int bionic_success_chance( bool autodoc, int skill_level, int difficulty, const Character &target )
{
    return bionic_manip_cos( target.bionics_adjusted_skill( autodoc, skill_level ), difficulty );
}

// bionic manipulation chance of success
int bionic_manip_cos( float adjusted_skill, int bionic_difficulty )
{
    if( get_player_character().has_trait( trait_DEBUG_BIONICS ) ) {
        return 100;
    }

    int chance_of_success = 0;
    // we will base chance_of_success on a ratio of skill and difficulty
    // when skill=difficulty, this gives us 1.  skill < difficulty gives a fraction.
    float skill_difficulty_parameter = static_cast<float>( adjusted_skill /
                                       ( 4.0 * bionic_difficulty ) );

    // when skill == difficulty, chance_of_success is 50%. Chance of success drops quickly below that
    // to reserve bionics for characters with the appropriate skill.  For more difficult bionics, the
    // curve flattens out just above 80%
    chance_of_success = static_cast<int>( ( 100 * skill_difficulty_parameter ) /
                                          ( skill_difficulty_parameter + std::sqrt( 1 / skill_difficulty_parameter ) ) );

    return chance_of_success;
}

bool Character::can_uninstall_bionic( const bionic_id &b_id, Character &installer, bool autodoc,
                                      int skill_level )
{
    // if malfunctioning bionics doesn't have associated item it gets a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( b_id->itype() ) ) {
        const itype *type = item::find_type( b_id->itype() );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    if( !has_bionic( b_id ) ) {
        popup( _( "%s don't have this bionic installed." ), disp_name() );
        return false;
    }

    Character &player_character = get_player_character();

    for( const bionic_id &bid : get_bionics() ) {
        if( bid->is_included( b_id ) ) {
            popup( _( "%s must remove the %s bionic to remove the %s." ), installer.disp_name(),
                   bid->name, b_id->name );
            return false;
        }
    }

    if( b_id->cant_remove_reason.has_value() ) {
        popup( string_format( b_id->cant_remove_reason.value(), disp_name( true ), disp_name() ) );
        return false;
    }

    // removal of bionics adds +2 difficulty over installation
    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficulty + 2,
                            installer );

    if( chance_of_success >= 100 ) {
        if( !player_character.query_yn(
                _( "Are you sure you wish to uninstall the selected bionic?" ),
                100 - chance_of_success ) ) {
            return false;
        }
    } else {
        if( !player_character.query_yn(
                _( "WARNING: %i percent chance of SEVERE damage to all body parts!  Continue anyway?" ),
                ( 100 - static_cast<int>( chance_of_success ) ) ) ) {
            return false;
        }
    }

    return true;
}

bool Character::uninstall_bionic( const bionic_id &b_id, Character &installer, bool autodoc,
                                  int skill_level )
{
    // if malfunctioning bionics doesn't have associated item it gets a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( b_id->itype() ) ) {
        const itype *type = item::find_type( b_id->itype() );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    // removal of bionics adds +2 difficulty over installation
    int pl_skill = bionics_pl_skill( autodoc, skill_level );
    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficulty + 2, installer );

    // Surgery is imminent, retract claws or blade if active
    for( size_t i = 0; i < installer.my_bionics->size(); i++ ) {
        const bionic &bio = ( *installer.my_bionics )[ i ];
        if( bio.powered && bio.info().has_flag( json_flag_BIONIC_WEAPON ) ) {
            installer.deactivate_bionic( i );
        }
    }

    int success = chance_of_success - rng( 1, 100 );
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        perform_uninstall( b_id, difficulty, success, b_id->capacity, pl_skill );
        return true;
    }
    assign_activity( ACT_OPERATION, to_moves<int>( difficulty * 20_minutes ) );

    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( units::to_kilojoule( b_id->capacity ) );
    activity.values.push_back( pl_skill );
    activity.str_values.emplace_back( "uninstall" );
    activity.str_values.push_back( b_id.str() );
    activity.str_values.emplace_back( "" ); // installer_name is unused for uninstall
    if( autodoc ) {
        activity.str_values.emplace_back( "true" );
    } else {
        activity.str_values.emplace_back( "false" );
    }
    for( const std::pair<const bodypart_str_id, size_t> &elem : b_id->occupied_bodyparts ) {
        add_effect( effect_under_operation, difficulty * 20_minutes, elem.first.id(), true, difficulty );
    }

    return true;
}

void Character::perform_uninstall( const bionic_id &bid, int difficulty, int success,
                                   const units::energy &power_lvl, int pl_skill )
{
    map &here = get_map();
    if( success > 0 ) {
        get_event_bus().send<event_type::removes_cbm>( getID(), bid );

        // until bionics can be flagged as non-removable
        add_msg_player_or_npc( m_neutral, _( "Your parts are jiggled back into their familiar places." ),
                               _( "<npcname>'s parts are jiggled back into their familiar places." ) );
        add_msg( m_good, _( "Successfully removed %s." ), bid.obj().name );
        remove_bionic( bid );

        // remove power bank provided by bionic
        mod_max_power_level( -power_lvl );

        item cbm( "burnt_out_bionic" );
        if( item::type_is_defined( bid->itype() ) ) {
            cbm = item( bid.c_str() );
        }
        cbm.set_flag( flag_FILTHY );
        cbm.set_flag( flag_NO_STERILE );
        cbm.set_flag( flag_NO_PACKED );
        cbm.faults.emplace( fault_bionic_salvaged );
        here.add_item( pos(), cbm );

        invalidate_pseudo_items();
    } else {
        get_event_bus().send<event_type::fails_to_remove_cbm>( getID(), bid );
        // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
        float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                               static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>
                               ( 10.0 ) );
        bionics_uninstall_failure( difficulty, success, adjusted_skill );

    }
    here.invalidate_map_cache( here.get_abs_sub().z );
}

bool Character::uninstall_bionic( const bionic &target_cbm, monster &installer, Character &patient,
                                  float adjusted_skill )
{
    viewer &player_view = get_player_view();
    if( installer.ammo[itype_anesthetic] <= 0 ) {
        add_msg_if_player_sees( installer, _( "The %s's anesthesia kit looks empty." ), installer.name() );
        return false;
    }

    item bionic_to_uninstall = item( target_cbm.id.str(), calendar::turn_zero );
    const itype *itemtype = bionic_to_uninstall.type;
    int difficulty = itemtype->bionic->difficulty;
    int chance_of_success = bionic_manip_cos( adjusted_skill, difficulty + 2 );
    int success = chance_of_success - rng( 1, 100 );

    const time_duration duration = difficulty * 20_minutes;
    // don't stack up the effect
    if( !installer.has_effect( effect_operating ) ) {
        installer.add_effect( effect_operating, duration + 5_turns );
    }

    if( patient.is_avatar() ) {
        add_msg( m_bad,
                 _( "You feel a tiny pricking sensation in your right arm, and lose all sensation before abruptly blacking out." ) );
    } else {
        add_msg_if_player_sees( installer, m_bad,
                                _( "The %1$s gently inserts a syringe into %2$s's arm and starts injecting something while holding them down." ),
                                installer.name(), patient.disp_name() );
    }

    installer.ammo[itype_anesthetic] -= 1;

    patient.add_effect( effect_narcosis, duration );
    patient.add_effect( effect_sleep, duration );

    if( patient.is_avatar() ) {
        add_msg( _( "You fall asleep and %1$s starts operating." ), installer.disp_name() );
    } else {
        add_msg_if_player_sees( patient, _( "%1$s falls asleep and %2$s starts operating." ),
                                patient.disp_name(), installer.disp_name() );
    }

    if( success > 0 ) {

        if( patient.is_avatar() ) {
            add_msg( m_neutral, _( "Your parts are jiggled back into their familiar places." ) );
            add_msg( m_mixed, _( "Successfully removed %s." ), target_cbm.info().name );
        } else if( patient.is_npc() && player_view.sees( patient ) ) {
            add_msg( m_neutral, _( "%s's parts are jiggled back into their familiar places." ),
                     patient.disp_name() );
            add_msg( m_mixed, _( "Successfully removed %s." ), target_cbm.info().name );
        }

        // remove power bank provided by bionic
        patient.mod_max_power_level( -target_cbm.info().capacity );
        patient.remove_bionic( target_cbm.id );
        item cbm( "burnt_out_bionic" );
        if( item::type_is_defined( target_cbm.info().itype() ) ) {
            cbm = bionic_to_uninstall;
        }
        cbm.set_flag( flag_FILTHY );
        cbm.set_flag( flag_NO_STERILE );
        cbm.set_flag( flag_NO_PACKED );
        cbm.faults.emplace( fault_bionic_salvaged );
        get_map().add_item( patient.pos(), cbm );
    } else {
        bionics_uninstall_failure( installer, patient, difficulty, success, adjusted_skill );
    }

    return false;
}

ret_val<bool> Character::is_installable( const item_location &loc, const bool by_autodoc ) const
{
    const item *it = loc.get_item();
    const itype *itemtype = it->type;
    const bionic_id &bid = itemtype->bionic->id;

    const auto has_trait_lambda = [this]( const trait_id & candidate ) {
        return has_trait( candidate );
    };

    if( it->has_flag( flag_FILTHY ) ) {
        // NOLINTNEXTLINE(cata-text-style): single space after the period for symmetry
        const std::string msg = by_autodoc ? _( "/!\\ CBM is highly contaminated. /!\\" ) :
                                _( "CBM is filthy." );
        return ret_val<bool>::make_failure( msg );
    } else if( it->has_flag( flag_NO_STERILE ) ) {
        const std::string msg = by_autodoc ?
                                // NOLINTNEXTLINE(cata-text-style): single space after the period for symmetry
                                _( "/!\\ CBM is not sterile. /!\\ Please use autoclave to sterilize." ) :
                                _( "CBM is not sterile." );
        return ret_val<bool>::make_failure( msg );
    } else if( it->has_fault( fault_id( "fault_bionic_salvaged" ) ) ) {
        return ret_val<bool>::make_failure( _( "CBM already deployed.  Please reset to factory state." ) );
    } else if( has_bionic( bid ) && !bid->dupes_allowed ) {
        return ret_val<bool>::make_failure( _( "CBM is already installed." ) );
    } else if( !can_install_cbm_on_bp( get_occupied_bodyparts( bid ) ) ) {
        return ret_val<bool>::make_failure( _( "CBM not compatible with patient's body." ) );
    } else if( std::any_of( bid->mutation_conflicts.begin(), bid->mutation_conflicts.end(),
                            has_trait_lambda ) ) {
        return ret_val<bool>::make_failure( _( "CBM not compatible with patient's body." ) );
    } else if( bid->upgraded_bionic &&
               !has_bionic( bid->upgraded_bionic ) &&
               it->is_upgrade() ) {
        return ret_val<bool>::make_failure( _( "No base version installed." ) );
    } else if( std::any_of( bid->available_upgrades.begin(),
                            bid->available_upgrades.end(),
                            std::bind( &Character::has_bionic, this,
                                       std::placeholders::_1 ) ) ) {
        return ret_val<bool>::make_failure( _( "Superior version installed." ) );
    } else if( is_npc() && !bid->has_flag( json_flag_BIONIC_NPC_USABLE ) ) {
        return ret_val<bool>::make_failure( _( "CBM not compatible with patient." ) );
    } else if( units::energy_max - get_max_power_level() < bid->capacity ) {
        return ret_val<bool>::make_failure( _( "Max power capacity already reached." ) );
    }

    return ret_val<bool>::make_success( std::string() );
}

bool Character::can_install_bionics( const itype &type, Character &installer, bool autodoc,
                                     int skill_level )
{
    if( !type.bionic ) {
        debugmsg( "Tried to install NULL bionic" );
        return false;
    }
    if( has_trait( trait_DEBUG_BIONICS ) ) {
        return true;
    }
    if( is_mounted() ) {
        return false;
    }

    const bionic_id &bioid = type.bionic->id;
    const int difficult = type.bionic->difficulty;

    // if we're doing self install
    if( !autodoc && installer.is_avatar() ) {
        return installer.has_enough_anesth( type ) &&
               installer.has_installation_requirement( bioid );
    }

    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficult, installer );

    std::vector<std::string> conflicting_muts;
    for( const trait_id &mid : bioid->canceled_mutations ) {
        if( has_trait( mid ) ) {
            conflicting_muts.push_back( mid->name() );
        }
    }

    if( !conflicting_muts.empty() &&
        !query_yn(
            _( "Installing this bionic will remove the conflicting traits: %s.  Continue anyway?" ),
            enumerate_as_string( conflicting_muts ) ) ) {
        return false;
    }

    const std::map<bodypart_id, int> &issues = bionic_installation_issues( bioid );
    // show all requirements which are not satisfied
    if( !issues.empty() ) {
        std::string detailed_info;
        for( const std::pair<const bodypart_id, int> &elem : issues ) {
            //~ <Body part name>: <number of slots> more slot(s) needed.
            detailed_info += string_format( _( "\n%s: %i more slot(s) needed." ),
                                            body_part_name_as_heading( elem.first, 1 ),
                                            elem.second );
        }
        popup( _( "Not enough space for bionic installation!%s" ), detailed_info );
        return false;
    }

    Character &player_character = get_player_character();
    if( chance_of_success >= 100 ) {
        if( !player_character.query_yn(
                _( "Are you sure you wish to install the selected bionic?" ),
                100 - chance_of_success ) ) {
            return false;
        }
    } else {
        if( !player_character.query_yn(
                _( "WARNING: %i percent chance of failure that may result in damage, pain, or a faulty installation!  Continue anyway?" ),
                ( 100 - chance_of_success ) ) ) {
            return false;
        }
    }

    return true;
}

float Character::env_surgery_bonus( int radius ) const
{
    float bonus = 1.0f;
    map &here = get_map();
    for( const tripoint &cell : here.points_in_radius( pos(), radius ) ) {
        if( here.furn( cell )->surgery_skill_multiplier ) {
            bonus = std::max( bonus, *here.furn( cell )->surgery_skill_multiplier );
        }
    }
    return bonus;
}

bool Character::install_bionics( const itype &type, Character &installer, bool autodoc,
                                 int skill_level )
{
    if( !type.bionic ) {
        debugmsg( "Tried to install NULL bionic" );
        return false;
    }

    const bionic_id &bioid = type.bionic->id;
    const bionic_id &upbioid = bioid->upgraded_bionic;
    const int difficulty = type.bionic->difficulty;
    int pl_skill = installer.bionics_pl_skill( autodoc, skill_level );
    int chance_of_success = bionic_success_chance( autodoc, skill_level, difficulty, installer );

    // Practice skills only if conducting manual installation
    if( !autodoc ) {
        installer.practice( skill_electronics, static_cast<int>( ( 100 - chance_of_success ) * 1.5 ) );
        installer.practice( skill_firstaid, static_cast<int>( ( 100 - chance_of_success ) * 1.0 ) );
        installer.practice( skill_mechanics, static_cast<int>( ( 100 - chance_of_success ) * 0.5 ) );
    }

    int success = chance_of_success - rng( 0, 99 );
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        perform_install( bioid, upbioid, difficulty, success, pl_skill, "NOT_MED",
                         bioid->canceled_mutations, pos() );
        return true;
    }
    assign_activity( ACT_OPERATION, to_moves<int>( difficulty * 20_minutes ) );
    activity.values.push_back( difficulty );
    activity.values.push_back( success );
    activity.values.push_back( units::to_millijoule( bioid->capacity ) );
    activity.values.push_back( pl_skill );
    activity.str_values.emplace_back( "install" );
    activity.str_values.push_back( bioid.str() );

    if( installer.has_trait( trait_PROF_MED ) || installer.has_trait( trait_PROF_AUTODOC ) ) {
        activity.str_values.push_back( installer.disp_name( true ) );
    } else {
        activity.str_values.emplace_back( "NOT_MED" );
    }
    if( autodoc ) {
        activity.str_values.emplace_back( "true" );
    } else {
        activity.str_values.emplace_back( "false" );
    }
    for( const std::pair<const bodypart_str_id, size_t> &elem : bioid->occupied_bodyparts ) {
        add_effect( effect_under_operation, difficulty * 20_minutes, elem.first.id(), true, difficulty );
    }

    return true;
}

void Character::perform_install( const bionic_id &bid, const bionic_id &upbid, int difficulty,
                                 int success, int pl_skill, const std::string &installer_name,
                                 const std::vector<trait_id> &trait_to_rem, const tripoint &patient_pos )
{
    if( success > 0 ) {
        get_event_bus().send<event_type::installs_cbm>( getID(), bid );
        if( upbid != bionic_id( "" ) ) {
            remove_bionic( upbid );
            //~ %1$s - name of the bionic to be upgraded (inferior), %2$s - name of the upgraded bionic (superior).
            add_msg( m_good, _( "Successfully upgraded %1$s to %2$s." ),
                     upbid.obj().name, bid.obj().name );
        } else {
            //~ %s - name of the bionic.
            add_msg( m_good, _( "Successfully installed %s." ), bid.obj().name );
        }

        add_bionic( bid );

        if( !trait_to_rem.empty() ) {
            for( const trait_id &tid : trait_to_rem ) {
                if( has_trait( tid ) ) {
                    remove_mutation( tid );
                }
            }
        }

    } else {
        get_event_bus().send<event_type::fails_to_install_cbm>( getID(), bid );

        // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
        float adjusted_skill = static_cast<float>( pl_skill ) - std::min( static_cast<float>( 40 ),
                               static_cast<float>( pl_skill ) - static_cast<float>( pl_skill ) / static_cast<float>
                               ( 10.0 ) );
        bionics_install_failure( bid, installer_name, difficulty, success, adjusted_skill, patient_pos );
    }
    map &here = get_map();
    here.invalidate_map_cache( here.get_abs_sub().z );
}

void Character::bionics_install_failure( const bionic_id &bid, const std::string &installer,
        int difficulty, int success, float adjusted_skill, const tripoint &patient_pos )
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful install.  We use this to determine consequences for failing.
    success = std::abs( success );

    // failure level is decided by how far off the character was from a successful install, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    int failure_level = static_cast<int>( std::sqrt( success * 4.0 * difficulty / adjusted_skill ) );
    int fail_type = ( failure_level > 5 ? 5 : failure_level );
    bool drop_cbm = false;
    add_msg( m_neutral, _( "The installation is a failure." ) );

    if( installer != "NOT_MED" ) {
        //~"Complications" is USian medical-speak for "unintended damage from a medical procedure".
        add_msg( m_neutral, _( "%s training helps to minimize the complications." ),
                 installer );
        // In addition to the bonus, medical residents know enough OR protocol to avoid botching.
        // Take MD and be immune to faulty bionics.
        if( fail_type > 3 ) {
            fail_type = rng( 1, 3 );
        }
    }
    if( fail_type <= 0 ) {
        add_msg( m_neutral, _( "The installation fails without incident." ) );
        drop_cbm = true;
    } else {
        std::set<bodypart_id> bp_hurt;
        switch( fail_type ) {

            case 1:
                if( !( has_trait( trait_NOPAIN ) ) ) {
                    add_msg_if_player( m_bad, _( "It really hurts!" ) );
                    mod_pain( rng( 10, 30 ) );
                }
                drop_cbm = true;
                break;

            case 2:
            case 3: {
                add_msg( m_bad, _( "The installation is faulty!" ) );
                std::vector<bionic_id> valid;
                std::copy_if( begin( faulty_bionics ), end( faulty_bionics ), std::back_inserter( valid ),
                [&]( const bionic_id & id ) {
                    return !has_bionic( id );
                } );

                // We've got all the bad bionics!
                if( valid.empty() ) {
                    if( has_max_power() ) {
                        units::energy old_power = get_max_power_level();
                        add_msg( m_bad, _( "%s lose power capacity!" ), disp_name() );
                        set_max_power_level( units::from_kilojoule( rng( 0,
                                             units::to_kilojoule( get_max_power_level() ) - 25 ) ) );
                        if( is_avatar() ) {
                            get_memorial().add(
                                pgettext( "memorial_male", "Lost %d units of power capacity." ),
                                pgettext( "memorial_female", "Lost %d units of power capacity." ),
                                units::to_kilojoule( old_power - get_max_power_level() ) );
                        }
                    }
                    // TODO: What if we can't lose power capacity?  No penalty?
                } else {
                    const bionic_id &id = random_entry( valid );
                    add_bionic( id );
                    get_event_bus().send<event_type::installs_faulty_cbm>( getID(), id );
                }

                break;
            }
            case 4:
            case 5: {
                for( const bodypart_id &bp : get_all_body_parts() ) {
                    if( has_effect( effect_under_operation, bp.id() ) ) {
                        if( bp_hurt.count( bp->main_part ) > 0 ) {
                            continue;
                        }
                        bp_hurt.emplace( bp->main_part );

                        apply_damage( this, bp, rng( 25, 50 ), true );
                        roll_critical_bionics_failure( bp );

                        add_msg_player_or_npc( m_bad, _( "Your %s is damaged." ), _( "<npcname>'s %s is damaged." ),
                                               body_part_name_accusative( bp ) );
                    }
                }
                drop_cbm = true;
                break;
            }
        }
    }
    if( drop_cbm ) {
        item cbm( bid.c_str() );
        cbm.set_flag( flag_NO_STERILE );
        cbm.set_flag( flag_NO_PACKED );
        cbm.faults.emplace( fault_bionic_salvaged );
        get_map().add_item( patient_pos, cbm );
    }
}

std::string list_occupied_bps( const bionic_id &bio_id, const std::string &intro,
                               const bool each_bp_on_new_line )
{
    if( bio_id->occupied_bodyparts.empty() ) {
        return "";
    }
    std::string desc = intro;
    for( const std::pair<const bodypart_str_id, size_t> &elem : bio_id->occupied_bodyparts ) {
        desc += ( each_bp_on_new_line ? "\n" : " " );
        //~ <Bodypart name> (<number of occupied slots> slots);
        desc += string_format( _( "%s (%i slots);" ),
                               body_part_name_as_heading( elem.first.id(), 1 ),
                               elem.second );
    }
    return desc;
}

std::vector<bionic_id> Character::get_bionics() const
{
    std::vector<bionic_id> result;
    for( const bionic &b : *my_bionics ) {
        result.push_back( b.id );
    }
    return result;
}

bool Character::has_bionic( const bionic_id &b ) const
{
    for( const bionic_id &bid : get_bionics() ) {
        if( bid == b ) {
            return true;
        }
    }
    return false;
}

bool Character::has_active_bionic( const bionic_id &b ) const
{
    for( const bionic &i : *my_bionics ) {
        if( i.id == b ) {
            return ( i.powered && i.incapacitated_time == 0_turns );
        }
    }
    return false;
}

bool Character::has_any_bionic() const
{
    return !get_bionics().empty();
}

int Character::get_used_bionics_slots( const bodypart_id &bp ) const
{
    int used_slots = 0;
    for( const bionic_id &bid : get_bionics() ) {
        auto search = bid->occupied_bodyparts.find( bp.id() );
        if( search != bid->occupied_bodyparts.end() ) {
            used_slots += search->second;
        }
    }

    return used_slots;
}

std::map<bodypart_id, int> Character::bionic_installation_issues( const bionic_id &bioid ) const
{
    std::map<bodypart_id, int> issues;
    if( !get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
        return issues;
    }
    for( const std::pair<const string_id<body_part_type>, size_t> &elem : bioid->occupied_bodyparts ) {
        const int lacked_slots = elem.second - get_free_bionics_slots( elem.first );
        if( lacked_slots > 0 ) {
            issues.emplace( elem.first, lacked_slots );
        }
    }
    return issues;
}

int Character::get_total_bionics_slots( const bodypart_id &bp ) const
{
    const bodypart_str_id &id = bp.id();
    int mut_bio_slots = 0;
    for( const trait_id &mut : get_mutations() ) {
        mut_bio_slots += mut->bionic_slot_bonus( id );
    }
    return bp->bionic_slots() + mut_bio_slots;
}

int Character::get_free_bionics_slots( const bodypart_id &bp ) const
{
    return get_total_bionics_slots( bp ) - get_used_bionics_slots( bp );
}

void Character::add_bionic( const bionic_id &b )
{
    if( has_bionic( b ) && !b->dupes_allowed ) {
        debugmsg( "Tried to install bionic %s that is already installed!", b.c_str() );
        return;
    }

    const units::energy pow_up = b->capacity;
    mod_max_power_level( pow_up );
    if( pow_up > 0_J ) {
        add_msg_if_player( m_good, _( "Increased storage capacity by %i." ),
                           units::to_kilojoule( pow_up ) );
    }

    my_bionics->push_back( bionic( b, get_free_invlet( *this ) ) );
    if( b == bio_tools || b == bio_ears ) {
        activate_bionic( my_bionics->size() - 1 );
    }

    for( const bionic_id &inc_bid : b->included_bionics ) {
        add_bionic( inc_bid );
    }

    for( const std::pair<const spell_id, int> &spell_pair : b->learned_spells ) {
        const spell_id learned_spell = spell_pair.first;
        if( learned_spell->spell_class != trait_id( "NONE" ) ) {
            const trait_id spell_class = learned_spell->spell_class;
            // spells you learn from a bionic overwrite the opposite spell class.
            // for best UX, include those spell classes in "canceled_mutations"
            if( !has_trait( spell_class ) ) {
                set_mutation( spell_class );
                on_mutation_gain( spell_class );
                add_msg_if_player( spell_class->desc() );
            }
        }
        if( !magic->knows_spell( learned_spell ) ) {
            magic->learn_spell( learned_spell, *this, true );
        }
        spell &known_spell = magic->get_spell( learned_spell );
        // spells you learn from installing a bionic upgrade spells you know if they are the same
        if( known_spell.get_level() < spell_pair.second ) {
            known_spell.set_level( spell_pair.second );
        }
    }

    for( const proficiency_id &learned : b->proficiencies ) {
        add_proficiency( learned );
    }

    calc_encumbrance();
    recalc_sight_limits();
    if( !b->enchantments.empty() ) {
        recalculate_enchantment_cache();
    }
    effect_on_conditions::process_reactivate( *this );

    invalidate_pseudo_items();
}

void Character::remove_bionic( const bionic_id &b )
{
    bionic_collection new_my_bionics;
    // any spells you should not forget due to still having a bionic installed that has it.
    std::set<spell_id> cbm_spells;
    bool skipped_installed = false;
    for( bionic &i : *my_bionics ) {
        // if we have multiples of the same bionic only remove one
        if( b == i.id && !skipped_installed ) {
            skipped_installed = true;
            continue;
        }

        // Linked bionics: if either is removed, the other is removed as well.
        if( b->is_included( i.id ) || i.id->is_included( b ) ) {
            continue;
        }

        for( const std::pair<const spell_id, int> &spell_pair : i.id->learned_spells ) {
            cbm_spells.emplace( spell_pair.first );
        }

        new_my_bionics.push_back( bionic( i.id, i.invlet ) );
    }

    // any spells you learn from installing a bionic you forget.
    for( const std::pair<const spell_id, int> &spell_pair : b->learned_spells ) {
        if( cbm_spells.count( spell_pair.first ) == 0 ) {
            magic->forget_spell( spell_pair.first );
        }
    }

    for( const proficiency_id &lost : b->proficiencies ) {
        lose_proficiency( lost );
    }

    *my_bionics = new_my_bionics;
    calc_encumbrance();
    recalc_sight_limits();
    if( !b->enchantments.empty() ) {
        recalculate_enchantment_cache();
    }
    effect_on_conditions::process_reactivate( *this );
}

int Character::num_bionics() const
{
    return my_bionics->size();
}

bionic &Character::bionic_at_index( int i )
{
    return ( *my_bionics )[i];
}

void Character::clear_bionics()
{
    my_bionics->clear();
}

void reset_bionics()
{
    faulty_bionics.clear();
    bionic_factory.reset();
}

void bionic::set_flag( const std::string &flag )
{
    bionic_tags.insert( flag );
}

void bionic::remove_flag( const std::string &flag )
{
    bionic_tags.erase( flag );
}

bool bionic::has_flag( const std::string &flag ) const
{
    return bionic_tags.find( flag ) != bionic_tags.end();
}

int bionic::get_quality( const quality_id &quality ) const
{
    if( weapon.typeId().is_empty() ) {
        return INT_MIN;
    }

    return weapon.get_quality( quality );
}

bool bionic::has_weapon() const
{
    return !weapon.typeId().is_empty() && !weapon.typeId().is_null();
}

item bionic::get_weapon() const
{
    return weapon;
}

void bionic::set_weapon( item &new_weapon )
{
    weapon = new_weapon;
}

std::vector<const item *> bionic::get_available_pseudo_items( bool include_weapon ) const
{
    std::vector<const item *> ret;
    for( const item &pseudo : passive_pseudo_items ) {
        ret.push_back( &pseudo );
    }

    if( powered && info().has_flag( flag_BIONIC_TOGGLED ) ) {
        for( const item &pseudo : toggled_pseudo_items ) {
            ret.push_back( &pseudo );
        }

        if( include_weapon && has_weapon() ) {
            ret.push_back( &weapon );
        }
    }

    return ret;
}

bool bionic::is_this_fuel_powered( const material_id &this_fuel ) const
{
    const std::vector<material_id> fuel_op = info().fuel_opts;
    return std::find( fuel_op.begin(), fuel_op.end(), this_fuel ) != fuel_op.end();
}

void bionic::toggle_safe_fuel_mod()
{
    if( !info().fuel_opts.empty() || info().is_remote_fueled ) {
        uilist tmenu;
        tmenu.text = _( "Chose Safe Fuel Level Threshold" );
        tmenu.addentry( 1, true, 'o', _( "Full Power" ) );
        if( get_auto_start_thresh() < 0.80 ) {
            tmenu.addentry( 2, true, 't', _( "Above 80 %%" ) );
        }
        if( get_auto_start_thresh() < 0.55 ) {
            tmenu.addentry( 3, true, 'f', _( "Above 55 %%" ) );
        }
        if( get_auto_start_thresh() < 0.30 ) {
            tmenu.addentry( 4, true, 's', _( "Above 30 %%" ) );
        }
        tmenu.addentry( 5, true, 'd', _( "Disabled" ) );
        tmenu.query();

        switch( tmenu.ret ) {
            case 1:
                set_safe_fuel_thresh( 1.0f );
                break;
            case 2:
                set_safe_fuel_thresh( 0.80f );
                break;
            case 3:
                set_safe_fuel_thresh( 0.55f );
                break;
            case 4:
                set_safe_fuel_thresh( 0.30f );
                break;
            case 5:
                set_safe_fuel_thresh( -1.0f );
            default:
                break;
        }
    }
}

void bionic::toggle_auto_start_mod()
{
    if( info().fuel_opts.empty() && !info().is_remote_fueled ) {
        return;
    }
    if( !is_auto_start_on() ) {
        uilist tmenu;
        tmenu.text = _( "Chose Start Power Level Threshold" );
        tmenu.addentry( 1, true, 'o', _( "No Power Left" ) );
        if( get_safe_fuel_thresh() > 0.25 ) {
            tmenu.addentry( 2, true, 't', _( "Below 25 %%" ) );
        }
        if( get_safe_fuel_thresh() > 0.50 ) {
            tmenu.addentry( 3, true, 'f', _( "Below 50 %%" ) );
        }
        if( get_safe_fuel_thresh() > 0.75 ) {
            tmenu.addentry( 4, true, 's', _( "Below 75 %%" ) );
        }
        tmenu.query();

        switch( tmenu.ret ) {
            case 1:
                set_auto_start_thresh( 0.0 );
                break;
            case 2:
                set_auto_start_thresh( 0.25 );
                break;
            case 3:
                set_auto_start_thresh( 0.5 );
                break;
            case 4:
                set_auto_start_thresh( 0.75 );
                break;
            default:
                break;
        }
    } else {
        set_auto_start_thresh( -1.0 );
    }
}

void bionic::set_auto_start_thresh( float val )
{
    auto_start_threshold = val;
}

float bionic::get_auto_start_thresh() const
{
    return auto_start_threshold;
}

bool bionic::is_auto_start_on() const
{
    return get_auto_start_thresh() > -1.0;
}

float bionic::get_safe_fuel_thresh() const
{
    return safe_fuel_threshold;
}

bool bionic::is_safe_fuel_on() const
{
    return get_safe_fuel_thresh() < 2.0;
}

void bionic::set_safe_fuel_thresh( float val )
{
    safe_fuel_threshold = val;
}

void bionic::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id );
    json.member( "invlet", static_cast<int>( invlet ) );
    json.member( "powered", powered );
    json.member( "charge", charge_timer );
    json.member( "bionic_tags", bionic_tags );
    if( incapacitated_time > 0_turns ) {
        json.member( "incapacitated_time", incapacitated_time );
    }
    if( is_auto_start_on() ) {
        json.member( "auto_start_threshold", auto_start_threshold );
    }
    if( is_safe_fuel_on() ) {
        json.member( "safe_fuel_threshold", safe_fuel_threshold );
    }

    if( has_weapon() ) {
        json.member( "weapon", weapon );
    }

    json.end_object();
}

void bionic::deserialize( const JsonObject &jo )
{
    id = bionic_id( jo.get_string( "id" ) );
    invlet = jo.get_int( "invlet" );
    powered = jo.get_bool( "powered" );
    charge_timer = jo.get_int( "charge" );

    initialize_pseudo_items();

    if( jo.has_int( "incapacitated_time" ) ) {
        incapacitated_time = 1_turns * jo.get_int( "incapacitated_time" );
    }
    if( jo.has_float( "auto_start_threshold" ) ) {
        auto_start_threshold = jo.get_float( "auto_start_threshold" );
    }
    if( jo.has_float( "safe_fuel_threshold" ) ) {
        safe_fuel_threshold = jo.get_float( "safe_fuel_threshold" );
    }
    if( jo.has_array( "bionic_tags" ) ) {
        for( const std::string line : jo.get_array( "bionic_tags" ) ) {
            bionic_tags.insert( line );
        }
    }

    if( jo.has_member( "weapon" ) ) {
        jo.read( "weapon", weapon, true );
    }

    // Suppress errors when loading old games
    // TODO: add old ammo to new weapon?
    if( jo.has_string( "ammo_loaded" ) ) {
        jo.get_string( "ammo_loaded" );
    }
    if( jo.has_int( "ammo_count" ) ) {
        jo.get_int( "ammo_count" );
    }
}

std::vector<bionic_id> bionics_cancelling_trait( const std::vector<bionic_id> &bios,
        const trait_id &tid )
{
    // Vector of bionics to return
    std::vector<bionic_id> bionics_cancelling;

    // Search through the vector of of bionics, and see if the trait is cancelled by one of them
    for( const bionic_id &bid : bios ) {
        for( const trait_id &trait : bid->canceled_mutations ) {
            if( trait == tid ) {
                bionics_cancelling.emplace_back( bid );
            }
        }
    }

    return bionics_cancelling;
}

void Character::introduce_into_anesthesia( const time_duration &duration, Character &installer,
        bool needs_anesthesia )   //used by the Autodoc
{
    if( installer.has_trait( trait_DEBUG_BIONICS ) ) {
        installer.add_msg_if_player( m_info,
                                     _( "You tell the pain to bug off and proceed with the operation." ) );
        return;
    }
    installer.add_msg_player_or_npc( m_info,
                                     _( "You set up the operation step-by-step, configuring the Autodoc to manipulate a CBM." ),
                                     _( "<npcname> sets up the operation, configuring the Autodoc to manipulate a CBM." ) );

    add_msg_player_or_npc( m_info,
                           _( "You settle into position, sliding your right wrist into the couch's strap." ),
                           _( "<npcname> settles into position, sliding their wrist into the couch's strap." ) );
    if( needs_anesthesia ) {
        //post-threshold medical mutants do not fear operations.
        if( has_trait( trait_THRESH_MEDICAL ) ) {
            add_msg_if_player( m_mixed,
                               _( "You feel excited as the operation starts." ) );
        }

        add_msg_if_player( m_mixed,
                           _( "You feel a tiny pricking sensation in your right arm, and lose all sensation before abruptly blacking out." ) );

        //post-threshold medical mutants with Deadened don't need anesthesia due to their inability to feel pain
    } else {
        //post-threshold medical mutants do not fear operations.
        if( has_trait( trait_THRESH_MEDICAL ) ) {
            add_msg_if_player( m_mixed,
                               _( "You feel excited as the Autodoc slices painlessly into you.  You enjoy the sight of scalpels slicing you apart." ) );
        } else {
            add_msg_if_player( m_mixed,
                               _( "You stay very, very still, focusing intently on an interesting stain on the ceiling, as the Autodoc slices painlessly into you." ) );
        }
    }

    //Pain junkies feel sorry about missed pain from operation.
    if( has_trait( trait_MASOCHIST ) || has_trait( trait_MASOCHIST_MED ) ||
        has_trait( trait_CENOBITE ) ) {
        add_msg_if_player( m_mixed,
                           _( "As your consciousness slips away, you feel regret that you won't be able to enjoy the operation." ) );
    }

    if( has_effect( effect_narcosis ) ) {
        const time_duration remaining_time = get_effect_dur( effect_narcosis );
        if( remaining_time < duration ) {
            const time_duration top_off_time = duration - remaining_time;
            add_effect( effect_narcosis, top_off_time );
            fall_asleep( top_off_time );
        }
    } else {
        add_effect( effect_narcosis, duration );
        fall_asleep( duration );
    }
}

bool Character::can_fuel_bionic_with( const item &it ) const
{
    // the item needs fuel data, or it needs to be a magazine with an item with fuel data.
    if( !it.is_fuel() ) {
        if( it.is_magazine() ) {
            if( !item( it.ammo_current() ).is_fuel() ) {
                return false;
            }
        } else {
            return false;
        }
    }
    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fuel : bid->fuel_opts ) {
            if( fuel == it.get_base_material().id ) {
                return true;
            }
            if( it.type->magazine && fuel == item( it.ammo_current() ).get_base_material().id ) {
                return true;
            }
        }

    }
    return false;
}

std::vector<bionic_id> Character::get_bionic_fueled_with( const item &it ) const
{
    std::vector<bionic_id> bionics;

    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fuel : bid->fuel_opts ) {
            if( fuel == it.get_base_material().id ||
                ( it.type->magazine && fuel == item( it.ammo_current() ).get_base_material().id ) ) {
                bionics.emplace_back( bid );
            }
        }
    }

    return bionics;
}

std::vector<bionic_id> Character::get_bionic_fueled_with( const material_id &mat ) const
{
    std::vector<bionic_id> bionics;

    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fuel : bid->fuel_opts ) {
            if( fuel == mat ) {
                bionics.emplace_back( bid );
            }
        }
    }

    return bionics;
}

std::vector<bionic_id> Character::get_fueled_bionics() const
{
    std::vector<bionic_id> bionics;
    for( const bionic_id &bid : get_bionics() ) {
        if( !bid->fuel_opts.empty() ) {
            bionics.emplace_back( bid );
        }
    }
    return bionics;
}

bionic_id Character::get_most_efficient_bionic( const std::vector<bionic_id> &bids ) const
{
    float temp_eff = 0.0f;
    bionic_id bio( "null" );
    for( const bionic_id &bid : bids ) {
        if( bid->fuel_efficiency > temp_eff ) {
            temp_eff = bid->fuel_efficiency;
            bio = bid;
        }
    }
    return bio;
}

bionic_id Character::get_remote_fueled_bionic() const
{
    for( const bionic_id &bid : get_bionics() ) {
        if( bid->is_remote_fueled ) {
            return bid;
        }
    }
    return bionic_id();
}

std::vector<material_id> Character::get_fuel_available( const bionic_id &bio ) const
{
    std::vector<material_id> stored_fuels;
    for( const material_id &fuel : bio->fuel_opts ) {
        if( !get_value( fuel.str() ).empty() || fuel->get_fuel_data().is_perpetual_fuel ) {
            stored_fuels.emplace_back( fuel );
        }
    }
    return stored_fuels;
}

int Character::get_fuel_capacity( const material_id &fuel ) const
{
    int amount_stored = 0;
    if( !get_value( fuel.str() ).empty() ) {
        amount_stored = std::stoi( get_value( fuel.str() ) );
    }
    int capacity = 0;
    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fl : bid->fuel_opts ) {
            if( get_value( bid.str() ).empty() || get_value( bid.str() ) == fl.str() ) {
                if( fl == fuel ) {
                    capacity += bid->fuel_capacity;
                }
            }
        }
    }
    return capacity - amount_stored;
}

int Character::get_total_fuel_capacity( const material_id &fuel ) const
{
    int capacity = 0;
    for( const bionic_id &bid : get_bionics() ) {
        for( const material_id &fl : bid->fuel_opts ) {
            if( get_value( bid.str() ).empty() || get_value( bid.str() ) == fl.str() ) {
                if( fl == fuel ) {
                    capacity += bid->fuel_capacity;
                }
            }
        }
    }
    return capacity;
}

void Character::update_fuel_storage( const material_id &fuel )
{

    if( get_value( fuel.str() ).empty() ) {
        for( const bionic_id &bid : get_bionic_fueled_with( fuel ) ) {
            remove_value( bid.c_str() );
        }
        return;
    }

    std::vector<bionic_id> bids = get_bionic_fueled_with( fuel );
    if( bids.empty() ) {
        return;
    }
    int amount_fuel_loaded = std::stoi( get_value( fuel.str() ) );
    std::vector<bionic_id> loaded_bio;

    // Sort bionic in order of decreasing capacity
    // To fill the bigger ones firts.
    bool swap = true;
    while( swap ) {
        swap = false;
        for( size_t i = 0; i < bids.size() - 1; i++ ) {
            if( bids[i + 1]->fuel_capacity > bids[i]->fuel_capacity ) {
                std::swap( bids[i + 1], bids[i] );
                swap = true;
            }
        }
    }

    for( const bionic_id &bid : bids ) {
        remove_value( bid.c_str() );
        if( bid->fuel_capacity <= amount_fuel_loaded ) {
            amount_fuel_loaded -= bid->fuel_capacity;
            loaded_bio.emplace_back( bid );
        } else if( amount_fuel_loaded != 0 ) {
            loaded_bio.emplace_back( bid );
            break;
        }
    }

    for( const bionic_id &bd : loaded_bio ) {
        set_value( bd.str(), fuel.str() );
    }

}

int Character::get_mod_stat_from_bionic( const character_stat &Stat ) const
{
    int ret = 0;
    for( const bionic_id &bid : get_bionics() ) {
        const auto St_bn = bid->stat_bonus.find( Stat );
        if( St_bn != bid->stat_bonus.end() ) {
            ret += St_bn->second;
        }
    }
    return ret;
}

float Character::bionic_armor_bonus( const bodypart_id &bp, damage_type dt ) const
{
    float result = 0.0f;
    if( dt == damage_type::CUT || dt == damage_type::STAB ) {
        for( const bionic_id &bid : get_bionics() ) {
            const auto cut_prot = bid->cut_protec.find( bp.id() );
            if( cut_prot != bid->cut_protec.end() ) {
                result += cut_prot->second;
            }
        }
    } else if( dt == damage_type::BASH ) {
        for( const bionic_id &bid : get_bionics() ) {
            const auto bash_prot = bid->bash_protec.find( bp.id() );
            if( bash_prot != bid->bash_protec.end() ) {
                result += bash_prot->second;
            }
        }
    } else if( dt == damage_type::BULLET ) {
        for( const bionic_id &bid : get_bionics() ) {
            const auto bullet_prot = bid->bullet_protec.find( bp.id() );
            if( bullet_prot != bid->bullet_protec.end() ) {
                result += bullet_prot->second;
            }
        }
    }

    return result;
}

