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

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <cstdio>
#include <iostream>
#include <string>
using std::string;

#include <iomanip>
#include <cassert>
#include "types.h"

namespace gbpp {
	
	// Exceptions
	class InvalidAddress {
	public:
		InvalidAddress(const string _msg) : msg(_msg) {}
		InvalidAddress(const word addr) {
			char buffer[6];
			sprintf(buffer, "%d", (int)addr);
			msg = buffer;
		}
		InvalidAddress() : msg("Invalid Address.") {}
		inline string what() const { return "InvalidAddress: " + msg; }
	private:
		string msg;
	};
	
	// Proxy Class: I discard the first 0x8000 bytes, because those bytes are accessible from cartridge
	class InternalRam {
	private:
		static const int SIZE = 0x8000;
		byte ram[SIZE];
	public:
		// ram[addr]=data
		byte &operator[](const word addr) {
			if(addr < SIZE) {
				throw InvalidAddress(addr);
			}
			return ram[addr - SIZE];
		}
		
		// ram[addr]
		const byte operator[](const word addr) const {
			if(addr < SIZE) {
				throw InvalidAddress(addr);
			}
			return ram[addr - SIZE];
		}
	};

	class Memory {
	public:
		static const int ROM_BANK_SIZE = 0x2000;

		static const int P1    = 0xFF00;
		static const int SC    = 0xFF02;
		static const int DIV   = 0xFF04;
		static const int TIMA  = 0xFF05;
		static const int TMA   = 0xFF06;
		static const int TAC   = 0xFF07;
		static const int IF    = 0xFF0F;
		static const int NR_10 = 0xFF10;
		static const int NR_11 = 0xFF11;
		static const int NR_12 = 0xFF12;
		static const int NR_14 = 0xFF14;
		static const int NR_21 = 0xFF16;
		static const int NR_22 = 0xFF17;
		static const int NR_24 = 0xFF19;
		static const int NR_30 = 0xFF1A;
		static const int NR_31 = 0xFF1B;
		static const int NR_32 = 0xFF1C;
		static const int NR_34 = 0xFF1E;
		static const int NR_41 = 0xFF20;
		static const int NR_42 = 0xFF21;
		static const int NR_43 = 0xFF22;
		static const int NR_44 = 0xFF23;
		static const int NR_50 = 0xFF24;
		static const int NR_51 = 0xFF25;
		static const int NR_52 = 0xFF26;
		static const int LCDC  = 0xFF40;
		static const int STAT  = 0xFF41;
		static const int SCY   = 0xFF42;
		static const int SCX   = 0xFF43;
		static const int LY    = 0xFF44;
		static const int LYC   = 0xFF45;
		static const int DMA   = 0xFF46;
		static const int BGP   = 0xFF47;
		static const int OBP0  = 0xFF48;
		static const int OBP1  = 0xFF49;
		static const int WY    = 0xFF4A;
		static const int WX    = 0xFF4B;
		static const int IE    = 0xFFFF;

		// start addresses
		static const int ROM0 = 0x0000;
		static const int ROM1 = 0x4000; 
		static const int VRAM = 0x8000; // video RAM
		static const int BTD0 = 0x8000; // background tile data 0
		static const int BTD1 = 0x8800; // background tile data 1
		static const int BTM0 = 0x9800; // background tile map 0
		static const int BTM1 = 0x9C00; // background tile map 1
		static const int RAM1 = 0xA000; // switchable RAM
		static const int RAM0 = 0xC000; // internal RAM
		static const int ECHO = 0xE000; // echo of internal RAM
		static const int OAM  = 0xFE00; // object attribute
		
		byte read_byte(const word addr);
		word read_word(const word addr);
		byte readhi(const word addr);
		void write_byte(const word addr, const byte data);
		void write_word(const word addr, const word data);
		void writehi(const word addr, const byte data);

		void increment_tima();
		void increment_div();
		void increment_ly();
		void dma_transfer(const byte data);
		void reset();
		void reset(const bool _skip_bios);
		byte get_joypad_state();
		void set_joypad_state(const byte key);
		void clear_joypad_state(const byte key);
		static Memory& get_instance();
	private:
		Memory();
		InternalRam ram; // addresses >= 0x8000
		byte eram[ROM_BANK_SIZE * 4]; // external RAM
		byte joypad_state;
		bool skip_bios;
		bool eram_enabled;
		int mbc_mode;
		byte current_rom_bank;
		byte current_ram_bank;
		void mbc_switch(const word addr, byte data);
		byte joypad_mem_state() const;
	};

}

#endif /* _MEMORY_H_ */
