/*
 * Copyright (C) 2011 Kamil Dudka <kdudka@redhat.com>
 *
 * This file is part of predator.
 *
 * predator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * predator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with predator.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef H_GUARD_SYM_ENTS_H
#define H_GUARD_SYM_ENTS_H

#include "config.h"

#include "symid.hh"

#include <queue>
#include <vector>

#include <boost/foreach.hpp>

#ifdef NDEBUG
    // aggressive optimization
#   define DCAST static_cast
#else
#   define DCAST dynamic_cast
#endif

struct IHeapEntity {
    virtual ~IHeapEntity() { }
    virtual IHeapEntity* clone() const = 0;
};

class EntStore {
    public:
        EntStore() { }
        inline EntStore(const EntStore &);
        inline ~EntStore();

        template <typename T> inline T assignId(IHeapEntity *);
        template <typename T> inline void releaseEnt(const T id);

        template <typename T> T lastId() const {
            return static_cast<T>(this->ents_.size() - 1);
        }

        template <typename T> bool outOfRange(const T id) const {
            return (this->lastId<T>() < id) || (id < 0);
        }

        template <class TEnt, typename TId>
        inline void getEntRO(const TEnt **, const TId id);

        template <class TEnt, typename TId>
        inline void getEntRW(TEnt **, const TId id);

    private:
        // intentionally not implemented
        EntStore& operator=(const EntStore &);

        std::vector<IHeapEntity *>      ents_;

#if SE_RECYCLE_HEAP_IDS
        std::queue<unsigned>            freeIds_;
#endif
};

template <typename T> T EntStore::assignId(IHeapEntity *ptr) {
#if SE_RECYCLE_HEAP_IDS
    if (!this->freeIds_.empty()) {
        const T id = static_cast<T>(this->freeIds_.front());
        this->freeIds_.pop();
        this->ents_[id] = ptr;
        CL_DEBUG("reusing heap ID #" << id 
                << " (heap size is " << this->ents_.size() << ")");
        return id;
    }
#endif
    this->ents_.push_back(ptr);
    return this->lastId<T>();
}

template <typename T> void EntStore::releaseEnt(const T id) {
#if SE_RECYCLE_HEAP_IDS
    freeIds_.push(id);
#endif
    IHeapEntity *&e = ents_[id];
    delete e;
    e = 0;
}

EntStore::EntStore(const EntStore &ref):
    ents_(ref.ents_)
{
    // deep copy of all heap entities
    BOOST_FOREACH(IHeapEntity *&ent, ents_)
        if (ent)
            ent = ent->clone();
}

EntStore::~EntStore() {
    BOOST_FOREACH(const IHeapEntity *ent, ents_)
        delete ent;
}

template <class TEnt, typename TId>
inline void EntStore::getEntRO(const TEnt **pEnt, const TId id) {
    // if this fails, the ID has never been valid
    CL_BREAK_IF(this->outOfRange(id));

    // if this fails, the ID is no longer valid
    const IHeapEntity *ptr = ents_[id];
    CL_BREAK_IF(!ptr);

    // if this fails, the entity has type that is incompatible with your request
    const TEnt *ent = DCAST<const TEnt *>(ptr);
    CL_BREAK_IF(!ent);

    // all OK!
    *pEnt = ent;
}

template <class TEnt, typename TId>
inline void EntStore::getEntRW(TEnt **pEnt, const TId id) {
    const TEnt *entRO;
    this->getEntRO(&entRO, id);

    TEnt *entRW = const_cast<TEnt *>(entRO);
    *pEnt = entRW;
}

#endif /* H_GUARD_SYM_ENTS_H */