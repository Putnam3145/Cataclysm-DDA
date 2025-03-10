#pragma once
#ifndef CATA_SRC_TALKER_ITEM_H
#define CATA_SRC_TALKER_ITEM_H

#include <functional>
#include <iosfwd>
#include <list>
#include <vector>

#include "coordinates.h"
#include "npc.h"
#include "talker.h"
#include "type_id.h"

class item;

struct tripoint;

/*
 * Talker wrapper class for Character.  well, ideally, but since Character is such a mess,
 * it's the wrapper class for player
 * Should never be invoked directly.  Only talker_avatar and talker_npc are really valid.
 */
class talker_item: public talker
{
    public:
        explicit talker_item( item_location *new_me ): me_it( new_me ) {
        }
        ~talker_item() override = default;

        // underlying element accessor functions
        item_location *get_item() override {
            return me_it;
        }
        item_location *get_item() const override {
            return me_it;
        }
        // identity and location
        std::string disp_name() const override;
        int posx() const override;
        int posy() const override;
        int posz() const override;
        tripoint pos() const override;
        tripoint_abs_omt global_omt_location() const override;

        std::string get_value( const std::string &var_name ) const override;
        void set_value( const std::string &var_name, const std::string &value ) override;
        void remove_value( const std::string & ) override;

    protected:
        talker_item() = default;
        item_location *me_it;
};
#endif // CATA_SRC_TALKER_ITEM_H
