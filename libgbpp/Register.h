/*
 *   Copyright (C) 2011 by Claudemiro Alves Feitosa Neto
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

#ifndef _REGISTER_H_
#define _REGISTER_H_

#include "types.h"
#include "util.h"

// From Byus bsnes - super game boy emulator

namespace gbpp {
  struct Register {
    virtual operator unsigned() const = 0;
    virtual unsigned operator=(unsigned x) = 0;
    Register& operator=(const Register &x) { operator=((unsigned)x); return *this; }
  };

  struct Register8 : Register {
    byte data;
    operator unsigned() const { return data; }
    unsigned operator=(unsigned x) { return data = x; }
  };

  struct RegisterF : Register {
    bool z, n, h, c;
    operator unsigned() const { return (z << 7) | (n << 6) | (h << 5) | (c << 4); }
    unsigned operator=(unsigned x) { z = x & 0x80; n = x & 0x40; h = x & 0x20; c = x & 0x10; return *this; }
    bool& operator[](unsigned r) {
      static bool* table[] = { &z, &n, &h, &c };
      return *table[r];
    }
  };

  struct Register16 : Register {
    word data;
    operator unsigned() const { return data; }
    unsigned operator=(unsigned x) { return data = x; }
  };

  struct RegisterAF : Register {
    Register8 &hi;
    RegisterF &lo;
    operator unsigned() const { return (hi << 8) | (lo << 0); }
    unsigned operator=(unsigned x) { hi = x >> 8; lo = x >> 0; return *this; }
    RegisterAF(Register8 &hi, RegisterF &lo) : hi(hi), lo(lo) {}
  };

  struct RegisterW : Register {
    Register8 &hi, &lo;
    operator unsigned() const { return (hi << 8) | (lo << 0); }
    unsigned operator=(unsigned x) { hi = x >> 8; lo = x >> 0; return *this; }
    RegisterW(Register8 &hi, Register8 &lo) : hi(hi), lo(lo) {}
  };
}

#endif /* _REGISTER_H_ */
