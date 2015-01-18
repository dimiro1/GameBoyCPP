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

#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include <string>
using std::string;

#include <iostream>
using std::ios;

#include <fstream>
using std::ifstream;

#include "types.h"
#include "Components.h"

#define GET_ROM_SIZE(size) (0x8000 << size)

namespace gbpp {
	
	enum Type { NONE, MBC1, MBC2 };
	
	static const string TypeName[] = {
		"NONE",
		"MBC1",
		"MBC2"
	};

	class Cartridge {
	public:
		void load(const string game);
		byte read_byte(const word addr) const;
		Type get_type();
		byte *get_title();
		bool is_rom_loaded();
		static Cartridge& get_instance();
	private:
		Cartridge() : rom_loaded(false) {}
		
		byte *rom;
		bool rom_loaded;
		
		struct header {
			byte entry[4];           // Usually "NOP; JP 0150h"
			byte nintendo_logo[48];  // Used by the BIOS to verify checksum
			byte title[16];          // Uppercase, padded with 0
			byte publisher[2];       // Used by newer GameBoy games
			byte sgb;                // Value of 3 indicates SGB support
			byte type;               // MBC type/extras
			byte rom_size;           // Usually between 0 and 7 Size = 32kB << [0148h]
			byte ram_size;           // Size of external RAM
			byte destination;        // 0 for Japan market, 1 otherwise
			byte old_publisher;      // Used by older GameBoy games
			byte version;            // Version of the game, usually 0
			byte header_checksum;    // Checked by BIOS before loading
			word global_checksum;    // Simple summation, not checked
		};

		enum Type type;
		struct header header;
	
		void detect_type();
		void verify_checksum() const;
		void debug_header() const;
	};

	// Exceptions
	class BadCartridge {
	public:
		BadCartridge(const string _msg) : msg(_msg) {}
		BadCartridge() : msg("Error in Cartridge.") {}
		inline string what() const { return "BadCartridge: " + msg; }
	private:
		string msg;
	};
}

#endif /* _CARTRIDGE_H_ */
