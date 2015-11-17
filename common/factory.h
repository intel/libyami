/*
 *  Copyright (C) 2015 Intel Corporation
 *      Author: U. Artie Eoff <ullysses.a.eoff@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef __COMMON_FACTORY_H__
#define __COMMON_FACTORY_H__

#include <map>
#include <string>

template < class T >
class Factory {
public:
    typedef T* Type;
    typedef Type (*Creator) (void);
    typedef std::string KeyType;
    typedef std::map<KeyType, Creator> Creators;
    typedef typename Creators::iterator iterator;
    typedef typename Creators::const_iterator const_iterator;

    /**
     * Register class C with @param key.  Returns true if class C is
     * successfully registered using @param key.  If @param key is already
     * registered, returns false and does not register class C with
     * @param key.
     */
    template < class C >
    static bool register_(const KeyType& key)
    {
        std::pair<iterator, bool> result =
            getCreators().insert(std::make_pair(key, create<C>));
        return result.second;
    }

    /**
     * Create and return a new object that is registered with @param key.
     * If @param key does not exist, then returns NULL.  The caller is
     * responsible for managing the lifetime of the returned object.
     */
    static Type create(const KeyType& key)
    {
        Creators& creators = getCreators();
        const const_iterator creator(creators.find(key));
        if (creator != creators.end())
            return creator->second();
        return NULL;
    }

private:
    template < class C >
    static Type create()
    {
        return new C;
    }
    static Creators& getCreators()
    {
        static Creators creators;
        return creators;
    }
};

#endif // __COMMON_FACTORY_H__
