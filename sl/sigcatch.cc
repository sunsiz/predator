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

#include "config.h"
#include "sigcatch.hh"

#include "util.hh"

#include <map>

#include <boost/foreach.hpp>

#ifndef _NSIG
#   define _NSIG 0x100
#endif

static volatile sig_atomic_t sig_flags[_NSIG];

static std::map<int /* signum */, sighandler_t> backup;

static void generic_signal_handler(int signum) {
    sig_flags[signum] = static_cast<sig_atomic_t>(true);
}

bool SignalCatcher::install(int signum) {
    // each signal may be installed at most once, check it in the debug build
    CL_BREAK_IF(hasKey(backup, signum));

    const sighandler_t old = signal(signum, generic_signal_handler);
    if (SIG_ERR == old)
        return false;

    ::backup[signum] = old;
    return true;
}

bool SignalCatcher::cleanup() {
    bool ok = true;

    BOOST_FOREACH(auto item, ::backup) {
        if (SIG_ERR == signal(item.first, item.second))
            ok = false;
    }

    if (!ok)
        // we already have a (non-recoverable) problem
        return false;

    // clear static data
    ::backup.clear();
    for (int i = 0; i < _NSIG; ++i)
        sig_flags[i] = static_cast<sig_atomic_t>(false);

    return true;
}

bool SignalCatcher::caught(int signum) {
    if (!sig_flags[signum])
        return false;

    sig_flags[signum] = static_cast<sig_atomic_t>(false);
    return true;
}

bool SignalCatcher::caught(int *signum) {
    for (int i = 0; i < _NSIG; ++i) {
        if (caught(i)) {
            *signum = i;
            return true;
        }
    }

    return false;
}