/*
 * Copyright (C) 2010 Kamil Dudka <kdudka@redhat.com>
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

#include "config.h"
#include "symseg.hh"

#include <cl/cl_msg.hh>

#include "symheap.hh"
#include "symutil.hh"

#include <boost/foreach.hpp>

bool haveSeg(const SymHeap &sh, TValId atAddr, TValId pointingTo,
             const EObjKind kind)
{
    if (VT_ABSTRACT != sh.valTarget(atAddr))
        // not an abstract object
        return false;

    TValId seg = sh.valRoot(atAddr);
    if (kind != sh.valTargetKind(seg))
        // kind mismatch
        return false;

    if (OK_DLS == kind) {
        seg = dlSegPeer(sh, seg);
        if (OK_DLS != sh.valTargetKind(seg))
            // invalid peer
            return false;
    }

    // compare the end-points
    SymHeap &writable = const_cast<SymHeap &>(sh);
    const TValId valNext = nextValFromSeg(writable, seg);
    return (valNext == pointingTo);
}

bool haveDlSegAt(const SymHeap &sh, TValId atAddr, TValId peerAddr) {
    if (atAddr <= 0 || peerAddr <= 0)
        // no valid targets
        return false;

    if (VT_ABSTRACT != sh.valTarget(atAddr)
            || VT_ABSTRACT != sh.valTarget(peerAddr))
        // not abstract objects
        return false;

    const TValId seg = sh.valRoot(atAddr);
    if (OK_DLS != sh.valTargetKind(seg))
        // not a DLS
        return false;

    const TValId peer = dlSegPeer(sh, seg);
    if (OK_DLS != sh.valTargetKind(peer))
        // invalid peer
        return false;

    // compare the end-points
    return (segHeadAt(sh, peer) == peerAddr);
}

TValId segClone(SymHeap &sh, const TValId root) {
    const TValId dup = objClone(sh, root);

    if (OK_DLS == sh.valTargetKind(root)) {
        // we need to clone the peer as well
        const TValId peer = dlSegPeer(sh, root);
        const TValId dupPeer = sh.valClone(peer);

        // dig the 'peer' selectors of the cloned objects
        const TOffset offpSeg  = sh.segBinding(dup).prev;
        const TOffset offpPeer = sh.segBinding(dupPeer).prev;

        // resolve selectors -> sub-objects
        const PtrHandle ppSeg (sh, sh.valByOffset(dup, offpSeg));
        const PtrHandle ppPeer(sh, sh.valByOffset(dupPeer, offpPeer));

        // now cross the 'peer' pointers
        ppSeg .setValue(segHeadAt(sh, dupPeer));
        ppPeer.setValue(segHeadAt(sh, dup));
    }

    return dup;
}

TValId lookThrough(const SymHeap &sh, TValId val, TValSet *pSeen) {
    if (VT_RANGE == sh.valTarget(val))
        // not supported yet
        return VAL_INVALID;

    const TOffset off = sh.valOffset(val);

    while (0 < val) {
        if (pSeen && !insertOnce(*pSeen, val))
            // an already seen value
            break;

        const EValueTarget code = sh.valTarget(val);
        if (!isAbstract(code))
            // a non-abstract object reached
            break;

        const TValId root = sh.valRoot(val);
        const TValId seg = segPeer(sh, root);
        if (sh.segMinLength(seg))
            // non-empty abstract object reached
            break;

        const EObjKind kind = sh.valTargetKind(seg);
        if (OK_OBJ_OR_NULL == kind) {
            // we always end up with VAL_NULL if OK_OBJ_OR_NULL is removed
            val = VAL_NULL;
            continue;
        }

        // jump to next value while taking the 'head' offset into consideration
        const TValId valNext = nextValFromSeg(sh, seg);
        const BindingOff &bOff = sh.segBinding(seg);
        val = const_cast<SymHeap &>(sh).valByOffset(valNext, off - bOff.head);
    }

    return val;
}

bool dlSegCheckConsistency(const SymHeap &sh) {
    TValList addrs;
    sh.gatherRootObjects(addrs, isAbstract);
    BOOST_FOREACH(const TValId at, addrs) {
        if (OK_DLS != sh.valTargetKind(at))
            // we are interested in OK_DLS here
            continue;

        if (at <= 0) {
            CL_ERROR("OK_DLS with invalid address detected");
            return false;
        }

        const TValId peer = dlSegPeer(sh, at);
        if (peer <= 0) {
            CL_ERROR("OK_DLS with invalid peer detected");
            return false;
        }

        if (OK_DLS != sh.valTargetKind(peer)) {
            CL_ERROR("DLS peer not a DLS");
            return false;
        }

        // check the consistency of Neq predicates
        const TMinLen len1 = sh.segMinLength(at);
        const TMinLen len2 = sh.segMinLength(peer);
        if (len1 != len2) {
            CL_ERROR("peer of a DLS " << len1 << "+ is a DLS" << len2 << "+");
            return false;
        }
    }

    // all OK
    return true;
}
