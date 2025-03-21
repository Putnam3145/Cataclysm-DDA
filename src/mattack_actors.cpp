#include "mattack_actors.h"

#include <algorithm>
#include <limits>
#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "creature_tracker.h"
#include "enums.h"
#include "game.h"
#include "generic_factory.h"
#include "gun_mode.h"
#include "item.h"
#include "item_pocket.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "viewer.h"

static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_laserlocked( "laserlocked" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_sensor_stun( "sensor_stun" );
static const efftype_id effect_targeted( "targeted" );
static const efftype_id effect_was_laserlocked( "was_laserlocked" );

static const trait_id trait_TOXICFLESH( "TOXICFLESH" );

void leap_actor::load_internal( const JsonObject &obj, const std::string & )
{
    // Mandatory:
    max_range = obj.get_float( "max_range" );
    // Optional:
    min_range = obj.get_float( "min_range", 1.0f );
    allow_no_target = obj.get_bool( "allow_no_target", false );
    move_cost = obj.get_int( "move_cost", 150 );
    min_consider_range = obj.get_float( "min_consider_range", 0.0f );
    max_consider_range = obj.get_float( "max_consider_range", 200.0f );
}

std::unique_ptr<mattack_actor> leap_actor::clone() const
{
    return std::make_unique<leap_actor>( *this );
}

bool leap_actor::call( monster &z ) const
{
    if( !z.has_dest() || !z.can_act() || !z.move_effects( false ) ) {
        return false;
    }

    std::vector<tripoint> options;
    const tripoint_abs_ms target_abs = z.get_dest();
    const float best_float = rl_dist( z.get_location(), target_abs );
    if( best_float < min_consider_range || best_float > max_consider_range ) {
        return false;
    }

    // We wanted the float for range check
    // int here will make the jumps more random
    int best = std::numeric_limits<int>::max();
    if( !allow_no_target && z.attack_target() == nullptr ) {
        return false;
    }
    map &here = get_map();
    const tripoint target = here.getlocal( target_abs );
    std::multimap<int, tripoint> candidates;
    for( const tripoint &candidate : here.points_in_radius( z.pos(), max_range ) ) {
        if( candidate == z.pos() ) {
            continue;
        }
        float leap_dist = trigdist ? trig_dist( z.pos(), candidate ) :
                          square_dist( z.pos(), candidate );
        if( leap_dist > max_range || leap_dist < min_range ) {
            continue;
        }
        int candidate_dist = rl_dist( candidate, target );
        if( candidate_dist >= best_float ) {
            continue;
        }
        candidates.emplace( candidate_dist, candidate );
    }
    for( const auto &candidate : candidates ) {
        const int &cur_dist = candidate.first;
        const tripoint &dest = candidate.second;
        if( cur_dist > best ) {
            break;
        }
        if( !z.sees( dest ) ) {
            continue;
        }
        if( !g->is_empty( dest ) ) {
            continue;
        }
        bool blocked_path = false;
        // check if monster has a clear path to the proposed point
        std::vector<tripoint> line = here.find_clear_path( z.pos(), dest );
        for( auto &i : line ) {
            if( here.impassable( i ) ) {
                blocked_path = true;
                break;
            }
        }
        // don't leap into water if you could drown (#38038)
        if( z.is_aquatic_danger( dest ) ) {
            blocked_path = true;
        }
        if( blocked_path ) {
            continue;
        }

        options.push_back( dest );
        best = cur_dist;
    }

    if( options.empty() ) {
        return false;    // Nowhere to leap!
    }

    z.moves -= move_cost;
    viewer &player_view = get_player_view();
    const tripoint chosen = random_entry( options );
    bool seen = player_view.sees( z ); // We can see them jump...
    z.setpos( chosen );
    seen |= player_view.sees( z ); // ... or we can see them land
    if( seen ) {
        add_msg( _( "The %s leaps!" ), z.name() );
    }

    return true;
}

std::unique_ptr<mattack_actor> mon_spellcasting_actor::clone() const
{
    return std::make_unique<mon_spellcasting_actor>( *this );
}

void mon_spellcasting_actor::load_internal( const JsonObject &obj, const std::string & )
{
    mandatory( obj, was_loaded, "spell_data", spell_data );
    translation monster_message;
    optional( obj, was_loaded, "monster_message", monster_message,
              //~ "<Monster Display name> cast <Spell Name> on <Target name>!"
              to_translation( "%1$s casts %2$s at %3$s!" ) );
    spell_data.trigger_message = monster_message;
}

bool mon_spellcasting_actor::call( monster &mon ) const
{
    if( !mon.can_act() ) {
        return false;
    }

    if( !mon.attack_target() ) {
        // this is an attack. there is no reason to attack if there isn't a real target.
        return false;
    }

    const tripoint target = spell_data.self ? mon.pos() : mon.attack_target()->pos();
    spell spell_instance = spell_data.get_spell();
    spell_instance.set_message( spell_data.trigger_message );

    // Bail out if the target is out of range.
    if( !spell_data.self && rl_dist( mon.pos(), target ) > spell_instance.range() ) {
        return false;
    }

    std::string target_name;
    if( const Creature *target_monster = get_creature_tracker().creature_at( target ) ) {
        target_name = target_monster->disp_name();
    }

    add_msg_if_player_sees( target, spell_instance.message(), mon.disp_name(),
                            spell_instance.name(), target_name );

    avatar fake_player;
    mon.moves -= spell_instance.casting_time( fake_player, true );
    spell_instance.cast_all_effects( mon, target );

    return true;
}

melee_actor::melee_actor()
{
    damage_max_instance = damage_instance::physical( 9, 0, 0, 0 );
    min_mul = 0.5f;
    max_mul = 1.0f;
    move_cost = 100;
}

void melee_actor::load_internal( const JsonObject &obj, const std::string & )
{
    // Optional:
    if( obj.has_array( "damage_max_instance" ) ) {
        damage_max_instance = load_damage_instance( obj.get_array( "damage_max_instance" ) );
    } else if( obj.has_object( "damage_max_instance" ) ) {
        damage_max_instance = load_damage_instance( obj );
    }

    optional( obj, was_loaded, "attack_chance", attack_chance, 100 );
    optional( obj, was_loaded, "accuracy", accuracy, INT_MIN );
    optional( obj, was_loaded, "min_mul", min_mul, 0.5f );
    optional( obj, was_loaded, "max_mul", max_mul, 1.0f );
    optional( obj, was_loaded, "move_cost", move_cost, 100 );
    optional( obj, was_loaded, "range", range, 1 );
    optional( obj, was_loaded, "no_adjacent", no_adjacent, false );
    optional( obj, was_loaded, "dodgeable", dodgeable, true );
    optional( obj, was_loaded, "blockable", blockable, true );

    optional( obj, was_loaded, "range", range, 1 );
    optional( obj, was_loaded, "throw_strength", throw_strength, 0 );

    optional( obj, was_loaded, "miss_msg_u", miss_msg_u,
              to_translation( "The %s lunges at you, but you dodge!" ) );
    optional( obj, was_loaded, "no_dmg_msg_u", no_dmg_msg_u,
              to_translation( "The %1$s bites your %2$s, but fails to penetrate armor!" ) );
    optional( obj, was_loaded, "hit_dmg_u", hit_dmg_u,
              to_translation( "The %1$s bites your %2$s!" ) );
    optional( obj, was_loaded, "miss_msg_npc", miss_msg_npc,
              to_translation( "The %s lunges at <npcname>, but they dodge!" ) );
    optional( obj, was_loaded, "no_dmg_msg_npc", no_dmg_msg_npc,
              to_translation( "The %1$s bites <npcname>'s %2$s, but fails to penetrate armor!" ) );
    optional( obj, was_loaded, "hit_dmg_npc", hit_dmg_npc,
              to_translation( "The %1$s bites <npcname>'s %2$s!" ) );
    optional( obj, was_loaded, "throw_msg_u", throw_msg_u,
              to_translation( "The force of the %s's attack sends you flying!" ) );
    optional( obj, was_loaded, "throw_msg_npc", throw_msg_npc,
              to_translation( "The force of the %s's attack sends <npcname> flying!" ) );

    if( obj.has_array( "body_parts" ) ) {
        for( JsonArray sub : obj.get_array( "body_parts" ) ) {
            const bodypart_str_id bp( sub.get_string( 0 ) );
            const float prob = sub.get_float( 1 );
            body_parts.add_or_replace( bp, prob );
        }
    }

    if( obj.has_array( "effects" ) ) {
        for( JsonObject eff : obj.get_array( "effects" ) ) {
            effects.push_back( load_mon_effect_data( eff ) );
        }
    }
}

Creature *melee_actor::find_target( monster &z ) const
{
    if( !z.can_act() ) {
        return nullptr;
    }

    Creature *target = z.attack_target();

    if( target == nullptr || ( no_adjacent && z.is_adjacent( target, false ) ) ) {
        return nullptr;
    }

    if( range > 1 ) {
        if( !z.sees( *target ) ||
            !get_map().clear_path( z.pos(), target->pos(), range, 1, 200 ) ) {
            return nullptr;
        }

    } else if( !z.is_adjacent( target, false ) ) {
        return nullptr;
    }

    return target;
}

bool melee_actor::call( monster &z ) const
{
    if( attack_chance != 100 && !x_in_y( attack_chance, 100 ) ) {
        return false;
    }

    Creature *target = find_target( z );
    if( target == nullptr ) {
        return false;
    }

    // If we are biting, make sure the target is grabbed if our z is humanoid
    if( dynamic_cast<const bite_actor *>( this ) && z.type->bodytype == "human" &&
        !target->has_effect( effect_grabbed ) ) {
        return false;
    }

    z.mod_moves( -move_cost );

    add_msg_debug( debugmode::DF_MATTACK, "%s attempting to melee_attack %s", z.name(),
                   target->disp_name() );

    // Dodge check
    const int acc = accuracy >= 0 ? accuracy : z.type->melee_skill;
    int hitspread = target->deal_melee_attack( &z, dice( acc, 10 ) );
    if( dodgeable ) {
        if( hitspread < 0 ) {
            game_message_type msg_type = target->is_avatar() ? m_warning : m_info;
            sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                     sfx::get_heard_angle( z.pos() ) );
            target->add_msg_player_or_npc( msg_type, miss_msg_u, miss_msg_npc, z.name() );
            return true;
        }
    }

    // Damage instance calculation
    damage_instance damage = damage_max_instance;
    double multiplier = rng_float( min_mul, max_mul );
    damage.mult_damage( multiplier );

    // Pick body part
    bodypart_str_id bp_hit = body_parts.empty() ?
                             target->select_body_part( &z, hitspread ).id() :
                             *body_parts.pick();

    bodypart_id bp_id = bodypart_id( bp_hit );

    // Block our hit
    if( blockable ) {
        target->block_hit( &z, bp_id, damage );
    }

    // Take damage
    dealt_damage_instance dealt_damage = target->deal_damage( &z, bp_id, damage );
    dealt_damage.bp_hit = bp_id;

    // On hit effects
    target->on_hit( &z, bp_id );

    int damage_total = dealt_damage.total_damage();
    add_msg_debug( debugmode::DF_MATTACK, "%s's melee_attack did %d damage", z.name(), damage_total );
    if( damage_total > 0 ) {
        on_damage( z, *target, dealt_damage );
    } else {
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( m_neutral, no_dmg_msg_u, no_dmg_msg_npc, z.name(),
                                       body_part_name_accusative( bp_id ) );
    }
    if( throw_strength > 0 ) {
        z.remove_effect( effect_grabbing );
        g->fling_creature( target, coord_to_angle( z.pos(), target->pos() ),
                           throw_strength );
        target->add_msg_player_or_npc( m_bad, throw_msg_u, throw_msg_npc, z.name() );
    }

    return true;
}

void melee_actor::on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const
{
    if( target.is_avatar() ) {
        sfx::play_variant_sound( "mon_bite", "bite_hit", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        sfx::do_player_death_hurt( dynamic_cast<Character &>( target ), false );
    }
    game_message_type msg_type = target.attitude_to( get_player_character() ) ==
                                 Creature::Attitude::FRIENDLY ?
                                 m_bad : m_neutral;
    const bodypart_id &bp = dealt.bp_hit ;
    target.add_msg_player_or_npc( msg_type, hit_dmg_u, hit_dmg_npc, z.name(),
                                  body_part_name_accusative( bp ) );

    for( const mon_effect_data &eff : effects ) {
        if( x_in_y( eff.chance, 100 ) ) {
            const bodypart_id affected_bp = eff.affect_hit_bp ? bp : eff.bp.id();
            target.add_effect( eff.id, time_duration::from_turns( eff.duration ), affected_bp, eff.permanent );
        }
    }
}

std::unique_ptr<mattack_actor> melee_actor::clone() const
{
    return std::make_unique<melee_actor>( *this );
}

bite_actor::bite_actor() = default;

void bite_actor::load_internal( const JsonObject &obj, const std::string &src )
{
    // Infection chance is a % (i.e. 5/100)
    melee_actor::load_internal( obj, src );
    infection_chance = obj.get_int( "infection_chance", 5 );
}

void bite_actor::on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const
{
    melee_actor::on_damage( z, target, dealt );

    if( x_in_y( infection_chance, 100 ) ) {
        const bodypart_id &hit = dealt.bp_hit;
        if( target.has_effect( effect_bite, hit.id() ) ) {
            target.add_effect( effect_bite, 40_minutes, hit, true );
        } else if( target.has_effect( effect_infected, hit.id() ) ) {
            target.add_effect( effect_infected, 25_minutes, hit, true );
        } else {
            target.add_effect( effect_bite, 1_turns, hit, true );
        }
    }

    if( target.has_trait( trait_TOXICFLESH ) ) {
        z.add_effect( effect_poison, 5_minutes );
        z.add_effect( effect_badpoison, 5_minutes );
    }
}

std::unique_ptr<mattack_actor> bite_actor::clone() const
{
    return std::make_unique<bite_actor>( *this );
}

gun_actor::gun_actor() : description( to_translation( "The %1$s fires its %2$s!" ) ),
    targeting_sound( to_translation( "beep-beep-beep!" ) )
{
}

void gun_actor::load_internal( const JsonObject &obj, const std::string & )
{
    obj.read( "gun_type", gun_type, true );

    obj.read( "ammo_type", ammo_type );

    if( obj.has_array( "fake_skills" ) ) {
        for( JsonArray cur : obj.get_array( "fake_skills" ) ) {
            fake_skills[skill_id( cur.get_string( 0 ) )] = cur.get_int( 1 );
        }
    }

    obj.read( "fake_str", fake_str );
    obj.read( "fake_dex", fake_dex );
    obj.read( "fake_int", fake_int );
    obj.read( "fake_per", fake_per );

    for( JsonArray mode : obj.get_array( "ranges" ) ) {
        if( mode.size() < 2 || mode.get_int( 0 ) > mode.get_int( 1 ) ) {
            obj.throw_error( "incomplete or invalid range specified", "ranges" );
        }
        ranges.emplace( std::make_pair<int, int>( mode.get_int( 0 ), mode.get_int( 1 ) ),
                        gun_mode_id( mode.size() > 2 ? mode.get_string( 2 ) : "" ) );
    }

    obj.read( "max_ammo", max_ammo );

    obj.read( "move_cost", move_cost );

    obj.read( "description", description );
    obj.read( "failure_msg", failure_msg );
    if( !obj.read( "no_ammo_sound", no_ammo_sound ) ) {
        no_ammo_sound = to_translation( "Click." );
    }

    obj.read( "targeting_cost", targeting_cost );

    obj.read( "require_targeting_player", require_targeting_player );
    obj.read( "require_targeting_npc", require_targeting_npc );
    obj.read( "require_targeting_monster", require_targeting_monster );

    obj.read( "targeting_timeout", targeting_timeout );
    obj.read( "targeting_timeout_extend", targeting_timeout_extend );

    if( !obj.read( "targeting_sound", targeting_sound ) ) {
        targeting_sound = to_translation( "Beep." );
    }

    obj.read( "targeting_volume", targeting_volume );

    laser_lock = obj.get_bool( "laser_lock", false );

    obj.read( "require_sunlight", require_sunlight );
}

std::unique_ptr<mattack_actor> gun_actor::clone() const
{
    return std::make_unique<gun_actor>( *this );
}

bool gun_actor::call( monster &z ) const
{
    Creature *target;

    if( z.friendly ) {
        int max_range = 0;
        for( const auto &e : ranges ) {
            max_range = std::max( std::max( max_range, e.first.first ), e.first.second );
        }

        int hostiles; // hostiles which cannot be engaged without risking friendly fire
        target = z.auto_find_hostile_target( max_range, hostiles );
        if( !target ) {
            if( hostiles > 0 ) {
                add_msg_if_player_sees( z, m_warning,
                                        ngettext( "Pointed in your direction, the %s emits an IFF warning beep.",
                                                  "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                                  hostiles ),
                                        z.name(), hostiles );
            }
            return false;
        }

    } else {
        target = z.attack_target();
        if( !target || !z.sees( *target ) ) {
            return false;
        }
    }

    int dist = rl_dist( z.pos(), target->pos() );
    for( const auto &e : ranges ) {
        if( dist >= e.first.first && dist <= e.first.second ) {
            shoot( z, *target, e.second );
            return true;
        }
    }
    return false;
}

void gun_actor::shoot( monster &z, Creature &target, const gun_mode_id &mode ) const
{
    if( require_sunlight && !g->is_in_sunlight( z.pos() ) ) {
        if( one_in( 3 ) ) {
            add_msg_if_player_sees( z, failure_msg.translated(), z.name() );
        }
        return;
    }

    const bool require_targeting = ( require_targeting_player && target.is_avatar() ) ||
                                   ( require_targeting_npc && target.is_npc() ) ||
                                   ( require_targeting_monster && target.is_monster() );
    const bool not_targeted = require_targeting && !z.has_effect( effect_targeted );
    const bool not_laser_locked = require_targeting && laser_lock &&
                                  !target.has_effect( effect_was_laserlocked );

    if( not_targeted || not_laser_locked ) {
        if( targeting_volume > 0 && !targeting_sound.empty() ) {
            sounds::sound( z.pos(), targeting_volume, sounds::sound_t::alarm,
                           targeting_sound );
        }
        if( not_targeted ) {
            z.add_effect( effect_targeted, time_duration::from_turns( targeting_timeout ) );
        }
        if( not_laser_locked ) {
            target.add_effect( effect_laserlocked, time_duration::from_turns( targeting_timeout ) );
            target.add_effect( effect_was_laserlocked, time_duration::from_turns( targeting_timeout ) );
            target.add_msg_if_player( m_warning,
                                      _( "You're not sure why you've got a laser dot on you…" ) );
        }

        z.moves -= targeting_cost;
        return;
    }

    z.moves -= move_cost;

    item gun( gun_type );
    gun.gun_set_mode( mode );

    itype_id ammo = ammo_type;
    if( ammo.is_null() ) {
        if( gun.magazine_integral() ) {
            ammo = gun.ammo_default();
        } else {
            ammo = item( gun.magazine_default() ).ammo_default();
        }
    }

    if( !ammo.is_null() ) {
        if( gun.magazine_integral() ) {
            gun.ammo_set( ammo, z.ammo[ammo] );
        } else {
            item mag( gun.magazine_default() );
            mag.ammo_set( ammo, z.ammo[ammo] );
            gun.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
        }
    }

    if( z.has_effect( effect_stunned ) || z.has_effect( effect_sensor_stun ) ) {
        return;
    }

    if( !gun.ammo_sufficient( nullptr ) ) {
        if( !no_ammo_sound.empty() ) {
            sounds::sound( z.pos(), 10, sounds::sound_t::combat, no_ammo_sound );
        }
        return;
    }

    standard_npc tmp( _( "The " ) + z.name(), z.pos(), {}, 8,
                      fake_str, fake_dex, fake_int, fake_per );
    tmp.worn.emplace_back( "backpack" );
    tmp.set_fake( true );
    tmp.set_attitude( z.friendly ? NPCATT_FOLLOW : NPCATT_KILL );
    tmp.recoil = 0; // no need to aim

    for( const auto &pr : fake_skills ) {
        tmp.set_skill_level( pr.first, pr.second );
    }

    tmp.weapon = gun;
    tmp.i_add( item( "UPS_off", calendar::turn, 1000 ) );

    add_msg_if_player_sees( z, m_warning, description.translated(), z.name(), tmp.weapon.tname() );

    z.ammo[ammo] -= tmp.fire_gun( target.pos(), gun.gun_current_mode().qty );

    if( require_targeting ) {
        z.add_effect( effect_targeted, time_duration::from_turns( targeting_timeout_extend ) );
    }

    if( laser_lock ) {
        // To prevent spamming laser locks when the player can tank that stuff somehow
        target.add_effect( effect_was_laserlocked, 5_turns );
    }
}
