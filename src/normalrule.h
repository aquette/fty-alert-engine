/*
Copyright (C) 2014 - 2015 Eaton

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*! \file normalrule.h
 *  \author Alena Chernikava <AlenaChernikava@Eaton.com>
 *  \brief Representation of normal rule
 */
#ifndef SRC_NORMALRULE_H
#define SRC_NORMALRULE_H

#include <cxxtools/jsondeserializer.h>
#include "luarule.h"
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
// because of zsys
#include <czmq.h>
class NormalRule : public LuaRule
{
public:
    NormalRule(){};

    /*
     * \brief parse json and check lua and fill the object
     *
     * ATTENTION: throws, if bad JSON
     *
     * \return 1 if rule has other type
     *         2 if lua function has errors
     *         0 if everything is ok
     */
    int fill(cxxtools::JsonDeserializer &json)
    {
        const cxxtools::SerializationInfo *si = json.si();
        if ( si->findMember("single") == NULL ) {
            return 1;
        }
        zsys_info ("it is SINGLE rule");
        auto single = si->getMember("single");
        if ( single.category () != cxxtools::SerializationInfo::Object ) {
            zsys_info ("Root of json must be an object with property 'single'.");
            throw std::runtime_error("Root of json must be an object with property 'single'.");
        }

        // target
        auto target = single.getMember("target");
        if ( target.category () != cxxtools::SerializationInfo::Array ) {
            zsys_info ("property 'target' in json must be an Array");
            throw std::runtime_error ("property 'target' in json must be an Array");
        }
        target >>= _metrics;

        // serialize to json, so we have actual json without the trash
        std::stringstream output_json;
        cxxtools::JsonSerializer serializer(output_json);
        serializer.beautify(false);   // not so nice to read, but very compact
        serializer.serialize((*si));
        output_json >> _json_representation;

        single.getMember("rule_name") >>= _name;
        single.getMember("element") >>= _element;
        // values
        // values are not required for single rule
        if ( single.findMember("values") != NULL ) {
            std::map<std::string,double> tmp_values;
            auto values = single.getMember("values");
            if ( values.category () != cxxtools::SerializationInfo::Array ) {
                zsys_info ("parameter 'values' in json must be an array.");
                throw std::runtime_error("parameter 'values' in json must be an array");
            }
            values >>= tmp_values;
            globalVariables(tmp_values);
        }

        // outcomes
        auto outcomes = single.getMember("results");
        if ( outcomes.category () != cxxtools::SerializationInfo::Array ) {
            zsys_info ("parameter 'results' in json must be an array.");
            throw std::runtime_error ("parameter 'results' in json must be an array.");
        }
        outcomes >>= _outcomes;

        std::string tmp;
        single.getMember("evaluation") >>= tmp;
        try {
            code(tmp);
        }
        catch ( const std::exception &e ) {
            zsys_warning ("something with lua function: %s", e.what());
            return 2;
        }
        return 0;
    }

    friend int readRule (std::istream &f, Rule **rule);

};

#endif // SRC_NORMALRULE_H_
