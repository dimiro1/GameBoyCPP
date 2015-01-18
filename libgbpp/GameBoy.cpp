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

#include "GameBoy.h"

namespace gbpp {
	
	/**
	 * This method is called 60 times a second
	 * This do a frame
	 */
	void GameBoy::frame() {
		int cycles;
		while(cpu.can_execute()) {
			cycles = cpu.execute();
			lcd.update_graphics(cycles);
		}
	}
	
	// FIXME: change this, I dont like to mantain an joypad state variable
	void GameBoy::key_pressed(const int key) {
		bool previously_unset = false;
		if(test_bit(memory.get_joypad_state(), key) == false) {
			previously_unset = true;
		}
		
		memory.clear_joypad_state(key);
		
		byte key_req = memory.read_byte(Memory::P1);
		bool req_int = false;
		
		if(!is_directional(key) && !(test_bit(key_req, 5))) {
			req_int = true;
		} else if(is_directional(key) && (test_bit(key_req, 4))) {
			req_int = true;
		}
		
		if(req_int && !previously_unset) {
			cpu.request_interrupt(Cpu::JOYPAD_INTERRUPT);
		}
	}
	
	bool GameBoy::is_directional(const int key) const {
		return (key == KEY_RIGHT || key == KEY_LEFT || key == KEY_UP || key == KEY_DOWN);
	}
	
	// FIXME: change this, I dont like to mantain an joypad state variable
	void GameBoy::key_released(const int key) {
		memory.set_joypad_state(key);
	}

	void GameBoy::power_on(const string game, const bool skip_bios) const {
		cartridge.load(game);
		memory.reset(skip_bios);
		lcd.reset();
		if(!skip_bios) {
			cpu.reset(0x0);
		}
		if(!cartridge.is_rom_loaded()) {
			throw BadCartridge("Could not load the rom.");
		}
	}
	
	void GameBoy::use_color_scheme(const int scheme) {
		lcd.use_color_scheme(scheme);
	}

	// TODO: What put here?
	void GameBoy::power_off() {
	}
}