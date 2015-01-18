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

#include "Memory.h"
#include "Cartridge.h"
#include "Lcd.h"

namespace gbpp {
	
	Memory::Memory() : eram_enabled(false) {}
	
	void Memory::reset() {
		reset(true);
	}
	
	void Memory::reset(const bool _skip_bios) {
		current_rom_bank = 1;
		current_ram_bank = 0;
		joypad_state = 0xFF;
		mbc_mode = 0;
		skip_bios = _skip_bios;

		// Special registers
		// If using Bios, it is not necessary set this registers
		// Because Bios does this.
		ram[P1]    = 0xFF;
		ram[DIV]   = 0xAF;
		ram[TIMA]  = 0x00;
		ram[TMA]   = 0x00;
		ram[TAC]   = 0x00;
		ram[NR_10] = 0x80;
		ram[NR_11] = 0xBF;
		ram[NR_12] = 0xF3;
		ram[NR_14] = 0xBF;
		ram[NR_21] = 0x3F;
		ram[NR_22] = 0x00;
		ram[NR_24] = 0xBF;
		ram[NR_30] = 0x7F;
		ram[NR_31] = 0xFF;
		ram[NR_32] = 0x9F;
		ram[NR_34] = 0xBF;
		ram[NR_41] = 0xFF;
		ram[NR_42] = 0x00;
		ram[NR_43] = 0x00;
		ram[NR_44] = 0xBF;
		ram[NR_50] = 0x77;
		ram[NR_51] = 0xF3;
		ram[NR_52] = 0xF1;
		ram[LCDC]  = 0x91;
		ram[SCY]   = 0x00;
		ram[SCX]   = 0x00;
		ram[LYC]   = 0x00;
		ram[BGP]   = 0xFC;
		ram[OBP0]  = 0xFF;
		ram[OBP1]  = 0xFF;
		ram[WY]    = 0x00;
		ram[WX]    = 0x00;
		ram[IE]    = 0x00;
		
		// Awesome!! Bios initialize this too :)
		for (int i = 0; i < 0x2000; i++) {
			eram[i] = cartridge.read_byte(0xA000 + i);
		}
	}

	byte Memory::read_byte(const word addr) {
		switch(addr & 0xF000) {
		case 0x0000:
			if(!skip_bios) {
				if(cpu.is_in_bios() && (addr < 0x100)) {
					return BIOS[addr];
				}
			}
			return cartridge.read_byte(addr);
		case 0x1000:
		case 0x2000:
		case 0x3000:
			return cartridge.read_byte(addr);
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
			return cartridge.read_byte((addr - 0x4000) + (current_rom_bank * 0x4000));
		case 0xA000:
		case 0xB000:
			return eram[(addr - 0xA000) + (current_ram_bank * 0x2000)];
		case 0xF000:
			switch(addr & 0x0FFF) {
			case 0xF00:
				return joypad_mem_state();
			}
		}
		return ram[addr];

		
		// Veryyy sloooow
		/*if(!skip_bios) {
			if(cpu.is_in_bios() && (addr < 0x100)) {
				return BIOS[addr];
			}
		}
		if(addr < 0x4000) {
			return cartridge.read_byte(addr);
		} else if(addr >= 0x4000 && addr < 0x8000) { // Read only memory from cartridge
			return cartridge.read_byte((addr - 0x4000) + (current_rom_bank * 0x4000));
		} else if((addr >= 0xA000) && (addr < 0xC000)) {
			return eram[(addr - 0xA000) + (current_ram_bank * 0x2000)];
		} else if(addr == P1) { // Joypad
			return joypad_mem_state();
		}
		return ram[addr];*/
	}

	word Memory::read_word(const word addr) {
		return ((read_byte(addr + 1) << 8) | read_byte(addr));
	}

	byte Memory::readhi(const word addr) {
		return read_byte(addr + P1);
	}

	void Memory::write_byte(const word addr, const byte data) {
		byte current_clock_freq;
		
		// This area is prohibited.
		if (((addr >= 0xFEA0) && (addr < 0xFF00)) || ((addr >= 0xFF4C) && (addr < 0xFF80))) {
			return;
		}

		switch(addr & 0xF000) {
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000:
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
			mbc_switch(addr, data);
			break;
		case 0xA000:
		case 0xB000:
			if(eram_enabled) {
				eram[(addr - 0xA000) + (current_ram_bank * 0x2000)] = data;
			}
			break;
		case 0xC000:
		case 0xD000:
			ram[addr] = data;
			break;
		case 0xE000:
			ram[addr] = data;
			ram[addr - 0x2000] = data;
			break;
		case 0xF000:
			switch(addr & 0x0FFF) {
			case 0xA00:
			case 0xB00:
			case 0xC00:
			case 0xD00:
				ram[addr] = data;
				ram[addr - 0x2000] = data;
				break;
			case 0xF04: // DIV
				cpu.reset_divider_counter();
				ram[addr] = 0;
				break;
			case 0xF07: // TAC
				current_clock_freq = cpu.get_clock_frequency();
				ram[addr] = data;
				if(current_clock_freq != cpu.get_clock_frequency()) {
					cpu.set_clock_frequency();
				}
				break;
			case 0xF46: // DMA
				dma_transfer(data);
				break;
			case 0xF44: // LY
				ram[addr] = 0;
				break;
			case 0xF45: // LYC
				ram[addr] = data;
				break;
			case 0xF0F: // IF
				ram[addr] = data & 0x1F;
				break;
			case 0xFFF: // IE
				ram[addr] = data & 0x1F;
				break;
			default:
				ram[addr] = data;
				break;
			}
			break;
		default:
			ram[addr] = data;
			break;
		}

		// Veryyy sloooow
		/*if(addr < 0x8000) {
			mbc_switch(addr, data);
		} else if((addr >= 0xA000) && (addr < 0xC000)) {
			if(eram_enabled) {
				eram[(addr - 0xA000) + (current_ram_bank * 0x2000)] = data;
			}
		} else if((addr >= 0xC000) && (addr < 0xE000)) {
			ram[addr] = data;
		} else if((addr >= 0xE000) && (addr < 0xFE00)) {
			ram[addr] = data;
			ram[addr - 0x2000] = data;
		} else if(addr == DIV) {
			cpu.reset_divider_counter();
			ram[addr] = 0;
		} else if(addr == DMA) {
			dma_transfer(data);
		} else if(addr == LY) {
			ram[addr] = 0;
		} else if(addr == LYC) {
			ram[addr] = data;
		} else if(addr == IF) {
			ram[addr] = data & 0x1F;
		} else if(addr == IE) {
			ram[addr] = data & 0x1F;
		} else if(addr == TAC) {
			byte current_clock_freq = cpu.get_clock_frequency();
			ram[addr] = data;
			if(current_clock_freq != cpu.get_clock_frequency()) {
				cpu.set_clock_frequency();
			}
		// This area is prohibited.
		} else if ((addr >= 0xFEA0) && (addr < 0xFF00)) {
		} else if ((addr >= 0xFF4C) && (addr < 0xFF80)) {
		} else {
			ram[addr] = data;
		}*/
	}
	
	byte Memory::get_joypad_state() {
		return joypad_state;
	}
	
	void Memory::set_joypad_state(const byte key) {
		set_bit(joypad_state, key);
	}
	
	void Memory::clear_joypad_state(const byte key) {
		clear_bit(joypad_state, key);
	}
	
	byte Memory::joypad_mem_state() const {
		byte res = ram[P1];
		
		res ^= 0xFF; // Flip all bits
		
		// buttons
		if(!(test_bit(res, 4))) {
			byte top_joypad = joypad_state >> 4;
			top_joypad |= 0xF0; // turn the top 4 bits on
			res &= top_joypad;
		} else if(!(test_bit(res, 5))) {
			byte bottom_joypad = joypad_state & 0xF;
			bottom_joypad |= 0xF0;
			res &= bottom_joypad;
		}
		return res;
	}
	
	void Memory::dma_transfer(const byte data) {
		word addr = (data * 0x100);
		for(int i = 0; i < 0xA0; i++) {
			write_byte(OAM + i, read_byte(addr + i));
		}
	}

	void Memory::write_word(const word addr, const word data) {
		write_byte(addr, (data & 0xFF));
		write_byte((addr + 1), (data >> 8) & 0xFF);
	}

	void Memory::writehi(const word addr, byte data) {
		write_byte(addr + P1, data);
	}

	void Memory::mbc_switch(const word addr, byte data) {
		if(cartridge.get_type() == NONE) {
			return;
		}
		switch(addr & 0xF000) {
		case 0x0000:
		case 0x1000:
			// ENABLE RAM
			// The least significant bit of the upper address byte must be zero to enable/disable cart RAM
			if(cartridge.get_type() == MBC2) {
				if(get_bit(addr, 8) == 1) {
					return;
				}
			}
			eram_enabled = ((data & 0xF) == 0xA);
			break;
		case 0x2000:
		case 0x3000:
			// Change LO Rom Bank
			if(cartridge.get_type() == MBC2) {
				if(get_bit(addr, 8) == 0) {
					return;
				}
				current_rom_bank |= (data & 0xF);
			} else {
				if(data == 0) {
					data = 1;
				}
				current_rom_bank &= 0xE0;
				current_rom_bank |= (data & 0x1F);
			}
			if(current_rom_bank == 0x20 || current_rom_bank == 0x40 || current_rom_bank == 0x60) {
				current_rom_bank++;
			}
			break;
		case 0x4000:
		case 0x5000:
			// 2bit register can be used to select a RAM Bank in range from 00-03h,
			// or to specify the upper two bits (Bit 5-6) of the ROM Bank number,
			// depending on the current ROM/RAM Mode.
			if(mbc_mode) {
				current_ram_bank = (data & 0x03);
			} else {
				current_ram_bank = 0; // RAM Bank 00h can be used during Mode 0
				current_rom_bank &= 0x1F;
				current_rom_bank |= ((data & 0x3) << 5);
			}
			break;
		case 0x6000:
		case 0x7000:
			// 0x0 = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
			// 0x1 = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
			mbc_mode = (data & 0x1);
			break;
		}

		// SLOW
		// ENABLE RAM
		/*if(addr < 0x2000) {
			// The least significant bit of the upper address byte must be zero to enable/disable cart RAM
			if(cartridge.get_type() == MBC2) {
				if(get_bit(addr, 8) == 1) {
					return;
				}
			}
			eram_enabled = ((data & 0xF) == 0xA);
		// Change LO Rom Bank
		} else if(addr >= 0x2000 && addr < 0x4000) {
			if(cartridge.get_type() == MBC2) {
				if(get_bit(addr, 8) == 0) {
					return;
				}
				current_rom_bank |= (data & 0xF);
			} else {
				if(data == 0) {
					data = 1;
				}
				current_rom_bank &= 0xE0;
				current_rom_bank |= (data & 0x1F);
			}
			if(current_rom_bank == 0x20 || current_rom_bank == 0x40 || current_rom_bank == 0x60) {
				current_rom_bank++;
			}
		// 2bit register can be used to select a RAM Bank in range from 00-03h,
		// or to specify the upper two bits (Bit 5-6) of the ROM Bank number,
		// depending on the current ROM/RAM Mode.
		} else if(addr >= 0x4000 && addr < 0x6000) {
			if(mbc_mode) {
				current_ram_bank = (data & 0x03);
			} else {
				current_ram_bank = 0; // RAM Bank 00h can be used during Mode 0
				current_rom_bank &= 0x1F;
				current_rom_bank |= ((data & 0x3) << 5);
			}
		// Change rom or ram banking
		} else if(addr >= 0x6000 && addr < 0x8000) {
			// 0x0 = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
			// 0x1 = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
			mbc_mode = (data & 0x1);
		}*/
		//if(current_rom_bank == 0) {
			//std::cout << "ROM: " << (int)current_rom_bank << std::endl;
		//}
	}

	void Memory::increment_tima() {
		ram[TIMA]++;
	}
	
	void Memory::increment_ly() {
		ram[LY]++;
	}
	
	void Memory::increment_div() {
		ram[DIV]++;
	}
	
	Memory& Memory::get_instance() {
		static Memory inst;
		return inst;
	}

}
