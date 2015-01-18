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

#include "Cpu.h"
#include "Memory.h"
#include "Cartridge.h"
#include "Lcd.h"

// Singleton accessors
#define cpu Cpu::get_instance()
#define memory Memory::get_instance()
#define cartridge Cartridge::get_instance()
#define lcd Lcd::get_instance()