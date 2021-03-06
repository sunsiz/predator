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

#ifndef H_GUARD_CLDEBUG_H
#define H_GUARD_CLDEBUG_H

#include <iostream>

struct cl_accessor;
struct cl_operand;
struct cl_type;

namespace CodeStorage {
    struct Insn;
}

void cltToStream(std::ostream &str, const struct cl_type *clt, unsigned depth);
void acToStream(std::ostream &str, const struct cl_accessor *ac, bool oneline);
void operandToStream(std::ostream &str, const struct cl_operand &op);
void insnToStream(std::ostream &str, const CodeStorage::Insn &insn);

inline std::ostream& operator<<(std::ostream &str, const struct cl_type &clt)
{
    cltToStream(str, &clt, /* oneline */ 0U);
    return str;
}

inline std::ostream& operator<<(std::ostream &str, const struct cl_accessor &ac)
{
    acToStream(str, &ac, /* oneline */ true);
    return str;
}

inline std::ostream& operator<<(std::ostream &str, const struct cl_operand &op)
{
    operandToStream(str, op);
    return str;
}

inline std::ostream& operator<<(std::ostream &str, const CodeStorage::Insn &in)
{
    insnToStream(str, in);
    return str;
}

#endif /* H_GUARD_CLDEBUG_H */
