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

#ifndef _GAME_BOY_H_
#define _GAME_BOY_H_

#include <iostream>
#include <string>
using std::string;

#include "util.h"
#include "types.h"
#include "version.h"
#include "Components.h"

namespace gbpp {

	class GameBoy {
	public:
		static const int WIDTH  = Lcd::WIDTH;
		static const int HEIGHT = Lcd::HEIGHT;

		GameBoy() {}
		enum {
			KEY_RIGHT,
			KEY_LEFT,
			KEY_UP,
			KEY_DOWN,
			KEY_A,
			KEY_B,
			KEY_SELECT,
			KEY_START
		};

		static const unsigned int FPS = 60;

		void frame();
		void power_on(const string game, const bool skip_bios) const;
		void power_off();
		
		bool is_directional(const int key) const;
		void use_color_scheme(const int scheme);

		void key_pressed(const int key);
		void key_released(const int key);
	};

	static const string KEY_NAMES[8] = {
		"RIGHT",
		"LEFT",
		"UP",
		"DOWN",
		"A",
		"B",
		"SELECT",
		"START"
	};
}

#endif /* _GAME_BOY_H_ */