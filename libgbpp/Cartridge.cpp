/*
 *   Copyright (C) 2010, 2011 by Claudemiro Alves Feitosa Neto
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

#include <cstdlib>
#include "Cartridge.h"

namespace gbpp {

	/**
	 * Load a Cartridge file.
	 */
	void Cartridge::load(const string game) {
		ifstream game_file(game.c_str(), ios::in | ios::binary);
	
		if(!game_file.is_open()) {
			throw BadCartridge("Error loading: " + game);
		}
		game_file.seekg(0x100, ios::beg);
		game_file.read(reinterpret_cast<char *>(&header), sizeof(struct header));
		
		rom = new byte[GET_ROM_SIZE(header.rom_size)];

		game_file.seekg(0, ios::beg);
		game_file.read(reinterpret_cast<char *>(rom),
			GET_ROM_SIZE(header.rom_size));

		verify_checksum();
		detect_type();

		//debug_header();
		game_file.close();
		rom_loaded = true;
	}
	
	bool Cartridge::is_rom_loaded() {
		return rom_loaded;
	}
	
	byte* Cartridge::get_title() {
		return header.title;
	}
	
	Type Cartridge::get_type() {
		return type;
	}

	/**
	 * Detect the Cartridge type.
	 * TODO: Detect more MBC types
	 */
	void Cartridge::detect_type() {
		switch (header.type) {
			case 0x0:
				type = NONE;
				break;
			case 0x1:
			case 0x2:
			case 0x3:
				type = MBC1;
				break;
			case 0x5:
			case 0x6:
				type = MBC2;
				break;
			default:
				throw BadCartridge("GBPP does not support this MBC type.");
				break;
		}
	}
	
	// I think this is not necessary.
	// Bios make this verification.
	void Cartridge::verify_checksum() const {
		int sum = 0;
		for(int i = 0x134; i <= 0x14C; i++) {
			sum = (sum - rom[i] - 1);
		}
		if((sum & 0xFF) != header.header_checksum) {
			throw BadCartridge("Invalid Checksum.");
		}
	}

	byte Cartridge::read_byte(const word addr) const {
		return rom[addr];
	}
	
	void Cartridge::debug_header() const {
		printf("Rom Title: %s\n", header.title);
		printf("Publisher: %s\n", header.publisher);
		printf("SGB: %d\n", header.sgb);
		printf("MBC: %s\n", TypeName[type].c_str());
		printf("Rom Size: %dKB\n", 32 << header.rom_size);
		printf("Ram Size: %dKB\n", header.ram_size);
		printf("Destination: %s\n", (header.destination ? "Japanese" : "Non-Japanese"));
		printf("Old Publisher: %d\n", header.old_publisher);
		printf("Version: %d\n", header.version);
		printf("Header checksum: %d\n", header.header_checksum);
		printf("Global checksum: %d\n", header.global_checksum);
	}
	
	Cartridge& Cartridge::get_instance() {
		static Cartridge inst;
		return inst;
	}
}