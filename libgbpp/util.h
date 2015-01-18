/*
 *   Copyright (C) 2010 by Claudemiro Alves Feitosa Neto
 *   <dimiro1@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licences>
 */

#ifndef _UTIL_H_
#define _UTIL_H_

namespace gbpp {
	
#define test_bit(t, pos)                        \
    (t & (1 << pos))

/* get Bit */
#define get_bit(t, pos)                         \
    (test_bit(t, pos) ? 1 : 0)

/* set Bit */
#define set_bit(t, pos)                         \
    t = t | (1 << pos)

/* flip Bit */
#define flip_bit(t, pos)                        \
    if(test_bit(t, pos)) {                      \
        clear_bit(t, pos);                      \
    } else {                                    \
        set_bit(t, pos);                        \
    }

/* clear Bit */
#define clear_bit(t, pos)                       \
    t = t & ~(1 << pos)

}

#endif /* _UTIL_H_ */
