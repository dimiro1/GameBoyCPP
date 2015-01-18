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

#ifndef _LCD_H_
#define _LCD_H_

#include "types.h"
#include "util.h"
#include "Components.h"

namespace gbpp {
	enum Color {
		WHITE,
		LIGHT_GRAY,
		DARK_GRAY,
		BLACK
	};

	static const Color color_indexes[] = {
		WHITE,
		LIGHT_GRAY,
		DARK_GRAY,
		BLACK
	};
	
	static const string color_schemes_names[] = {
		"Gray Shades",
		"Game Boy Classic",
		"KIGB",
		"bgb",
		"NO$GMB",
		"GameBoy Pocket",
		"Psychadelic", // FROM javaBoy 0.92
		"Takedown",
		"Dark Brown",
        "Super GameBoy"
	};
	
	static const int color_schemes[][3][4] = {
		{{0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000}, // BG
		 {0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000}, // OBP0
		 {0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000}},// OBP1

		{{0x9bbc0f, 0x8bac0f, 0x306230, 0x0f380f},
	     {0x9bbc0f, 0x8bac0f, 0x306230, 0x0f380f},
	     {0x9bbc0f, 0x8bac0f, 0x306230, 0x0f380f}},
		
		{{0xE7E7DE, 0xADB594, 0x318C8C, 0x292929},
		 {0xFFFFFF, 0xE7C6BD, 0xAD7373, 0x292929},
		 {0xFFFFFF, 0xE7C6BD, 0xAD7373, 0x292929}},

		{{0xefffde, 0xADD794, 0x529273, 0x183442},
		 {0xefffde, 0xADD794, 0x529273, 0x183442},
		 {0xefffde, 0xADD794, 0x529273, 0x183442}},
		
		{{0xffe78c, 0xdeb55a, 0x9c7b39, 0x4a3918},
	     {0xffe78c, 0xdeb55a, 0x9c7b39, 0x4a3918},
	     {0xffe78c, 0xdeb55a, 0x9c7b39, 0x4a3918}},
	
		{{0xc3cfa1, 0x8b9570, 0x4e533d, 0x1f1f1f},
	     {0xc3cfa1, 0x8b9570, 0x4e533d, 0x1f1f1f},
	     {0xc3cfa1, 0x8b9570, 0x4e533d, 0x1f1f1f}},

	    {{0xFFC0FF, 0x8080FF, 0xC000C0, 0x800080},
	     {0xFFFF40, 0xC0C000, 0xFF4040, 0x800000},
	     {0x80FFFF, 0x00C0C0, 0x008080, 0x004000}},
	
		{{0xe7d69c, 0xb5a56b, 0x7b7363, 0x393929},
	     {0xe7d69c, 0xb5a56b, 0x7b7363, 0x393929},
	     {0xe7d69c, 0xb5a56b, 0x7b7363, 0x393929}},
	
		{{0xfceae4, 0xc4ae94, 0x947a4c, 0x4c2a04},
	     {0xfceae4, 0xec9a54, 0x844204, 0x000000},
	     {0xfceae4, 0xec9a54, 0x844204, 0x000000}},

		{{0xfefef7, 0xfef7c0, 0xe29494, 0x414141},
         {0xfefef7, 0xfef7c0, 0xe29494, 0x414141},
         {0xfefef7, 0xfef7c0, 0xe29494, 0x414141}}
	};

	class Lcd {
	private:
		enum { MODE_0, MODE_1, MODE_2, MODE_3 };
		enum { BGP, OBP0, OBP1 };
		enum { RED, GREEN, BLUE };
		
		Lcd();
		int scanline_counter;
		int selected_color_scheme;
		void reset_scanline_counter();
		void clear_screen();
		byte get_current_mode() const;
		bool is_background_enabled() const;
		bool is_window_enabled() const;
		bool is_sprites_enabled() const;
		void draw_background();
		void draw_window();
		void draw_sprites();
		void scanline();
		Color get_color(const byte color_num, const word addr) const;
		Color background_color(const int x, const int y) const;
		int rgb_to_hex(const byte r, const byte g, const byte b) const;
		byte get_red(const int hexcolor) const;
		byte get_green(const int hexcolor) const;
		byte get_blue(const int hexcolor) const;
		byte get_red_from_colorscheme(const int scheme_color_index, const int color) const;
		byte get_green_from_colorscheme(const int scheme_color_index, const int color) const;
		byte get_blue_from_colorscheme(const int scheme_color_index, const int color) const;
		
	public:
		static const int WIDTH  = 160;
		static const int HEIGHT = 144;
		
		static const int VBLANK_MAX = 153;
		static const int HBLANK     = 456;
		static const int VBLANK = 144;
		
		static const int CYCLES_MODE0 = 375;
		static const int CYCLES_MODE1 = 456;
		static const int CYCLES_MODE2 =  82;
		static const int CYCLES_MODE3 = 172;
		
		// TODO: This must be private, I need find a way to make an get for this attribute
		// The buffer could be an Array with 144*160 positions or
		// an Array [144][160]=0xFFFF
		// those ways is easier to encapsulate
		byte screen[HEIGHT][WIDTH][3];
		
		void use_color_scheme(const int scheme);
		void reset();
		void set_lcd_status();
		bool is_lcd_enabled() const;
		void update_graphics(const int cycles);
		static Lcd& get_instance();
	};
}

#endif /* _LCD_H_ */
