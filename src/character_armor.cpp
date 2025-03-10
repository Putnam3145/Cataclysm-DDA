#include "bionics.h"
#include "character.h"
#include "flag.h"
#include "itype.h"
#include "make_static.h"
#include "map.h"
#include "memorial_logger.h"
#include "mutation.h"
#include "output.h"

static const bionic_id bio_ads( "bio_ads" );
static const efftype_id effect_onfire( "onfire" );

static const trait_id trait_HOLLOW_BONES( "HOLLOW_BONES" );
static const trait_id trait_LIGHT_BONES( "LIGHT_BONES" );
static const trait_id trait_SEESLEEP( "SEESLEEP" );

bool Character::can_interface_armor() const
{
    bool okay = std::any_of( my_bionics->begin(), my_bionics->end(),
    []( const bionic & b ) {
        return b.powered && b.info().has_flag( STATIC( json_character_flag( "BIONIC_ARMOR_INTERFACE" ) ) );
    } );
    return okay;
}

resistances Character::mutation_armor( bodypart_id bp ) const
{
    resistances res;
    for( const trait_id &iter : get_mutations() ) {
        res += iter->damage_resistance( bp );
    }

    return res;
}

float Character::mutation_armor( bodypart_id bp, damage_type dt ) const
{
    return mutation_armor( bp ).type_resist( dt );
}

float Character::mutation_armor( bodypart_id bp, const damage_unit &du ) const
{
    return mutation_armor( bp ).get_effective_resist( du );
}

int Character::get_armor_bash( bodypart_id bp ) const
{
    return get_armor_bash_base( bp ) + armor_bash_bonus;
}

int Character::get_armor_cut( bodypart_id bp ) const
{
    return get_armor_cut_base( bp ) + armor_cut_bonus;
}

int Character::get_armor_bullet( bodypart_id bp ) const
{
    return get_armor_bullet_base( bp ) + armor_bullet_bonus;
}

// TODO: Reduce duplication with below function
int Character::get_armor_type( damage_type dt, bodypart_id bp ) const
{
    switch( dt ) {
        case damage_type::PURE:
        case damage_type::BIOLOGICAL:
            return 0;
        case damage_type::BASH:
            return get_armor_bash( bp );
        case damage_type::CUT:
            return get_armor_cut( bp );
        case damage_type::STAB:
            return get_armor_cut( bp ) * 0.8f;
        case damage_type::BULLET:
            return get_armor_bullet( bp );
        case damage_type::ACID:
        case damage_type::HEAT:
        case damage_type::COLD:
        case damage_type::ELECTRIC: {
            int ret = 0;
            for( const item &i : worn ) {
                if( i.covers( bp ) ) {
                    ret += i.damage_resist( dt );
                }
            }

            ret += mutation_armor( bp, dt );
            return ret;
        }
        case damage_type::NONE:
        case damage_type::NUM:
            // Let it error below
            break;
    }

    debugmsg( "Invalid damage type: %d", dt );
    return 0;
}

std::map<bodypart_id, int> Character::get_all_armor_type( damage_type dt,
        const std::map<bodypart_id, std::vector<const item *>> &clothing_map ) const
{
    std::map<bodypart_id, int> ret;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        ret.emplace( bp, 0 );
    }

    for( std::pair<const bodypart_id, int> &per_bp : ret ) {
        const bodypart_id &bp = per_bp.first;
        switch( dt ) {
            case damage_type::PURE:
            case damage_type::BIOLOGICAL:
                // Characters cannot resist this
                return ret;
            /* BASH, CUT, STAB, and BULLET don't benefit from the clothing_map optimization */
            // TODO: Fix that
            case damage_type::BASH:
                per_bp.second += get_armor_bash( bp );
                break;
            case damage_type::CUT:
                per_bp.second += get_armor_cut( bp );
                break;
            case damage_type::STAB:
                per_bp.second += get_armor_cut( bp ) * 0.8f;
                break;
            case damage_type::BULLET:
                per_bp.second += get_armor_bullet( bp );
                break;
            case damage_type::ACID:
            case damage_type::HEAT:
            case damage_type::COLD:
            case damage_type::ELECTRIC: {
                for( const item *it : clothing_map.at( bp ) ) {
                    per_bp.second += it->damage_resist( dt );
                }

                per_bp.second += mutation_armor( bp, dt );
                break;
            }
            case damage_type::NONE:
            case damage_type::NUM:
                debugmsg( "Invalid damage type: %d", dt );
                return ret;
        }
    }

    return ret;
}

int Character::get_armor_bash_base( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.bash_resist();
        }
    }
    for( const bionic_id &bid : get_bionics() ) {
        const auto bash_prot = bid->bash_protec.find( bp.id() );
        if( bash_prot != bid->bash_protec.end() ) {
            ret += bash_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::BASH );
    return ret;
}

int Character::get_armor_cut_base( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.cut_resist();
        }
    }
    for( const bionic_id &bid : get_bionics() ) {
        const auto cut_prot = bid->cut_protec.find( bp.id() );
        if( cut_prot != bid->cut_protec.end() ) {
            ret += cut_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::CUT );
    return ret;
}

int Character::get_armor_bullet_base( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        if( i.covers( bp ) ) {
            ret += i.bullet_resist();
        }
    }

    for( const bionic_id &bid : get_bionics() ) {
        const auto bullet_prot = bid->bullet_protec.find( bp.id() );
        if( bullet_prot != bid->bullet_protec.end() ) {
            ret += bullet_prot->second;
        }
    }

    ret += mutation_armor( bp, damage_type::BULLET );
    return ret;
}

int Character::get_armor_acid( bodypart_id bp ) const
{
    return get_armor_type( damage_type::ACID, bp );
}

int Character::get_env_resist( bodypart_id bp ) const
{
    float ret = 0;
    for( const item &i : worn ) {
        // Head protection works on eyes too (e.g. baseball cap)
        if( i.covers( bp ) || ( bp == body_part_eyes && i.covers( body_part_head ) ) ) {
            ret += i.get_env_resist();
        }
    }

    for( const bionic_id &bid : get_bionics() ) {
        const auto EP = bid->env_protec.find( bp.id() );
        if( EP != bid->env_protec.end() ) {
            ret += EP->second;
        }
    }

    if( bp == body_part_eyes && has_trait( trait_SEESLEEP ) ) {
        ret += 8;
    }
    return ret;
}

std::map<bodypart_id, int> Character::get_armor_fire( const
        std::map<bodypart_id, std::vector<const item *>> &clothing_map ) const
{
    return get_all_armor_type( damage_type::HEAT, clothing_map );
}

static void item_armor_enchantment_adjust( Character &guy, damage_unit &du, item &armor )
{
    switch( du.type ) {
        case damage_type::ACID:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_ACID );
            break;
        case damage_type::BASH:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BASH );
            break;
        case damage_type::BIOLOGICAL:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BIO );
            break;
        case damage_type::COLD:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_COLD );
            break;
        case damage_type::CUT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_CUT );
            break;
        case damage_type::ELECTRIC:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_ELEC );
            break;
        case damage_type::HEAT:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_HEAT );
            break;
        case damage_type::STAB:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_STAB );
            break;
        case damage_type::BULLET:
            du.amount = armor.calculate_by_enchantment( guy, du.amount, enchant_vals::mod::ITEM_ARMOR_BULLET );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

// adjusts damage unit depending on type by enchantments.
// the ITEM_ enchantments only affect the damage resistance for that one item, while the others affect all of them
static void armor_enchantment_adjust( Character &guy, damage_unit &du )
{
    switch( du.type ) {
        case damage_type::ACID:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ACID );
            break;
        case damage_type::BASH:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BASH );
            break;
        case damage_type::BIOLOGICAL:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BIO );
            break;
        case damage_type::COLD:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_COLD );
            break;
        case damage_type::CUT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_CUT );
            break;
        case damage_type::ELECTRIC:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_ELEC );
            break;
        case damage_type::HEAT:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_HEAT );
            break;
        case damage_type::STAB:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_STAB );
            break;
        case damage_type::BULLET:
            du.amount = guy.calculate_by_enchantment( du.amount, enchant_vals::mod::ARMOR_BULLET );
            break;
        default:
            return;
    }
    du.amount = std::max( 0.0f, du.amount );
}

static void destroyed_armor_msg( Character &who, const std::string &pre_damage_name )
{
    if( who.is_avatar() ) {
        get_memorial().add(
            //~ %s is armor name
            pgettext( "memorial_male", "Worn %s was completely destroyed." ),
            pgettext( "memorial_female", "Worn %s was completely destroyed." ),
            pre_damage_name );
    }
    who.add_msg_player_or_npc( m_bad, _( "Your %s is completely destroyed!" ),
                               _( "<npcname>'s %s is completely destroyed!" ),
                               pre_damage_name );
}

std::string Character::absorb_hit( Creature *, const bodypart_id &bp, damage_instance &dam )
{
    std::list<item> worn_remains;
    bool armor_destroyed = false;

    for( damage_unit &elem : dam.damage_units ) {
        if( elem.amount < 0 ) {
            // Prevents 0 damage hits (like from hallucinations) from ripping armor
            elem.amount = 0;
            continue;
        }

        // The bio_ads CBM absorbs damage before hitting armor
        if( has_active_bionic( bio_ads ) ) {
            if( elem.amount > 0 && get_power_level() > 24_kJ ) {
                if( elem.type == damage_type::BASH ) {
                    elem.amount -= rng( 1, 2 );
                } else if( elem.type == damage_type::CUT ) {
                    elem.amount -= rng( 1, 4 );
                } else if( elem.type == damage_type::STAB || elem.type == damage_type::BULLET ) {
                    elem.amount -= rng( 1, 8 );
                }
                mod_power_level( -bio_ads->power_trigger );
            }
            if( elem.amount < 0 ) {
                elem.amount = 0;
            }
        }

        armor_enchantment_adjust( *this, elem );

        // Only the outermost armor can be set on fire
        bool outermost = true;
        // The worn vector has the innermost item first, so
        // iterate reverse to damage the outermost (last in worn vector) first.
        for( auto iter = worn.rbegin(); iter != worn.rend(); ) {
            item &armor = *iter;

            if( !armor.covers( bp ) ) {
                ++iter;
                continue;
            }

            const std::string pre_damage_name = armor.tname();
            bool destroy = false;

            item_armor_enchantment_adjust( *this, elem, armor );
            // Heat damage can set armor on fire
            // Even though it doesn't cause direct physical damage to it
            if( outermost && elem.type == damage_type::HEAT && elem.amount >= 1.0f ) {
                // TODO: Different fire intensity values based on damage
                fire_data frd{ 2 };
                destroy = armor.burn( frd );
                int fuel = roll_remainder( frd.fuel_produced );
                if( fuel > 0 ) {
                    add_effect( effect_onfire, time_duration::from_turns( fuel + 1 ), bp, false, 0, false,
                                true );
                }
            }

            if( !destroy ) {
                destroy = armor_absorb( elem, armor, bp );
            }

            if( destroy ) {
                if( get_player_view().sees( *this ) ) {
                    SCT.add( point( posx(), posy() ), direction::NORTH, remove_color_tags( pre_damage_name ),
                             m_neutral, _( "destroyed" ), m_info );
                }
                destroyed_armor_msg( *this, pre_damage_name );
                armor_destroyed = true;
                armor.on_takeoff( *this );
                for( const item *it : armor.all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
                    worn_remains.push_back( *it );
                }
                // decltype is the type name of the iterator, note that reverse_iterator::base returns the
                // iterator to the next element, not the one the revers_iterator points to.
                // http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
                iter = decltype( iter )( worn.erase( --( iter.base() ) ) );
            } else {
                ++iter;
                outermost = false;
            }
        }

        passive_absorb_hit( bp, elem );

        if( elem.type == damage_type::BASH ) {
            if( has_trait( trait_LIGHT_BONES ) ) {
                elem.amount *= 1.4;
            }
            if( has_trait( trait_HOLLOW_BONES ) ) {
                elem.amount *= 1.8;
            }
        }

        elem.amount = std::max( elem.amount, 0.0f );
    }
    map &here = get_map();
    for( item &remain : worn_remains ) {
        here.add_item_or_charges( pos(), remain );
    }
    if( armor_destroyed ) {
        drop_invalid_inventory();
    }
    return {};
}


bool Character::armor_absorb( damage_unit &du, item &armor, const bodypart_id &bp )
{
    if( rng( 1, 100 ) > armor.get_coverage( bp ) ) {
        return false;
    }

    // TODO: add some check for power armor
    armor.mitigate_damage( du );

    // We want armor's own resistance to this type, not the resistance it grants
    const float armors_own_resist = armor.damage_resist( du.type, true );
    if( armors_own_resist > 1000.0f ) {
        // This is some weird type that doesn't damage armors
        return false;
    }

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming ineffective or being destroyed.
    const int num_parts_covered = armor.get_covered_body_parts().count();
    if( !one_in( num_parts_covered ) ) {
        return false;
    }

    // Don't damage armor as much when bypassed by armor piercing
    // Most armor piercing damage comes from bypassing armor, not forcing through
    const float raw_dmg = du.amount;
    if( raw_dmg > armors_own_resist ) {
        // If damage is above armor value, the chance to avoid armor damage is
        // 50% + 50% * 1/dmg
        if( one_in( raw_dmg ) || one_in( 2 ) ) {
            return false;
        }
    } else {
        // Sturdy items and power armors never take chip damage.
        // Other armors have 0.5% of getting damaged from hits below their armor value.
        if( armor.has_flag( flag_STURDY ) || armor.is_power_armor() || !one_in( 200 ) ) {
            return false;
        }
    }

    const material_type &material = armor.get_random_material();
    std::string damage_verb = ( du.type == damage_type::BASH ) ? material.bash_dmg_verb() :
                              material.cut_dmg_verb();

    const std::string pre_damage_name = armor.tname();
    const std::string pre_damage_adj = armor.get_base_material().dmg_adj( armor.damage_level() );

    // add "further" if the damage adjective and verb are the same
    std::string format_string = ( pre_damage_adj == damage_verb ) ?
                                _( "Your %1$s is %2$s further!" ) : _( "Your %1$s is %2$s!" );
    add_msg_if_player( m_bad, format_string, pre_damage_name, damage_verb );
    //item is damaged
    if( is_avatar() ) {
        SCT.add( point( posx(), posy() ), direction::NORTH, remove_color_tags( pre_damage_name ), m_neutral,
                 damage_verb,
                 m_info );
    }

    return armor.mod_damage( armor.has_flag( flag_FRAGILE ) ?
                             rng( 2 * itype::damage_scale, 3 * itype::damage_scale ) : itype::damage_scale, du.type );
}

