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

#include "Lcd.h"

namespace gbpp {
	
	Lcd::Lcd() : scanline_counter(0), selected_color_scheme(0) {
	}

    void Lcd::reset() {
        clear_screen();
    }

    void Lcd::clear_screen() {}

	// TODO: This code is very ugly. I can write something better!.
    void Lcd::set_lcd_status() {
        byte status = memory.read_byte(Memory::STAT);

        if(!is_lcd_enabled()) {
			reset_scanline_counter();
            memory.write_byte(Memory::LY, 0);
            status = (status & 0xFC) | 1;
            memory.write_byte(Memory::STAT, status);
            return;
        }

        byte LY = memory.read_byte(Memory::LY);
        byte current_mode = get_current_mode();

        byte mode = MODE_0;
        bool req_int = false;

        if(LY >= VBLANK) { // Vertical blank
            mode = MODE_1;
            status = (status & 0xFC) | 1;
            req_int = test_bit(status, 4);
        } else {
			if(scanline_counter <= CYCLES_MODE2) { // accessing OAM
				mode = MODE_2;
				status = (status & 0xFC) | 2;
                req_int = test_bit(status, 5);
			} else if(scanline_counter <= CYCLES_MODE3) { // accessing VRAM
				mode = MODE_3;
				status |= 3;
			} else { // Horizontal blank
				mode = MODE_0;
				status &= 0xFC;
                req_int = test_bit(status, 3);
			}
        }

        if(req_int && (mode != current_mode)) {
            cpu.request_interrupt(Cpu::LCD_INTERRUPT);
        }

        memory.write_byte(Memory::STAT, status);
    }

	/*void Lcd::start_mode_zero() {}
	void Lcd::start_mode_one() {}
	void Lcd::start_mode_two() {}
	void Lcd::start_mode_three() {}*/

    void Lcd::update_graphics(const int cycles) {
        set_lcd_status();

        if(is_lcd_enabled()) {
            scanline_counter += cycles;
		}
		
		if(memory.read_byte(Memory::LY) > VBLANK_MAX) {
			memory.write_byte(Memory::LY, 0);
		}
		
		if(scanline_counter > HBLANK) {
			scanline();
		}
    }

	void Lcd::reset_scanline_counter() {
		if(scanline_counter > HBLANK) {
			scanline_counter -= HBLANK; // adjust scanline_counter, to be very accurate	
		} else {
			scanline_counter = 0;
		}
	}
	
	void Lcd::scanline() {
		if(!is_lcd_enabled()) {
			return;
		}
		reset_scanline_counter();

		byte LY = memory.read_byte(Memory::LY);
		byte status = memory.read_byte(Memory::STAT);

		if(LY == VBLANK) {
			cpu.request_interrupt(Cpu::VBLANK_INTERRUPT);
		}
		
		if(LY > VBLANK_MAX) {
			memory.write_byte(Memory::LY, 0);
		}
		if(LY < VBLANK) {
			if(is_lcd_enabled()) {
				// inneficient. draw_background and draw_window could be done at the same time
				draw_background();
		        draw_window();
		        draw_sprites();
			}
		}
		
		if(LY == memory.read_byte(Memory::LYC)) {
            set_bit(status, 2);
            if(test_bit(status, 6)) {
                cpu.request_interrupt(Cpu::LCD_INTERRUPT);
            }
        } else {
            clear_bit(status, 2);
        }
        memory.write_byte(Memory::STAT, status);
		
		memory.increment_ly();
	}

    byte Lcd::get_current_mode() const {
        return memory.read_byte(Memory::STAT) & 0x3;
    }

    bool Lcd::is_lcd_enabled() const {
        return test_bit(memory.read_byte(Memory::LCDC), 7);
    }

    bool Lcd::is_window_enabled() const {
        return test_bit(memory.read_byte(Memory::LCDC), 5);
    }

    bool Lcd::is_background_enabled() const {
        return test_bit(memory.read_byte(Memory::LCDC), 0);
    }

    bool Lcd::is_sprites_enabled() const {
        return test_bit(memory.read_byte(Memory::LCDC), 1);
    }

    // FIXME: Rewrite this. I can write something better.
    void Lcd::draw_background() {
        if(is_background_enabled()) {
            word tile_data = 0;
            word background_memory = 0;
            bool is_unsigned = true;

            byte scroll_y = memory.read_byte(Memory::SCY);
            byte scroll_x = memory.read_byte(Memory::SCX);

            if(test_bit(memory.read_byte(Memory::LCDC), 4)) {
                tile_data = 0x8000;
            } else {
                tile_data = 0x8800;
                is_unsigned = false;
            }

            if(test_bit(memory.read_byte(Memory::LCDC), 3)) {
                background_memory = 0x9C00;
            } else {
                background_memory = 0x9800;
            }

            byte y_pos = scroll_y + memory.read_byte(Memory::LY);

            word tile_row = static_cast<byte>(y_pos / 8) * 32;
            for(int pixel = 0; pixel < WIDTH; pixel++) {
                byte x_pos = pixel + scroll_x;

                word tile_col = (x_pos / 8);

                sword tile_num;
                word tile_address = background_memory + tile_row + tile_col;

                if(is_unsigned) {
                    tile_num = static_cast<byte>(memory.read_byte(tile_address));
                } else {
                    tile_num = static_cast<sbyte>(memory.read_byte(tile_address));
                }

                word tile_location = tile_data;

                if(is_unsigned) {
                    tile_location += (tile_num * 16);
                } else {
                    tile_location += (tile_num + 128) * 16;
                }

                byte line = y_pos % 8;
                line *= 2;

                byte data1 = memory.read_byte(tile_location + line);
                byte data2 = memory.read_byte(tile_location + line + 1);

                int color_bit = x_pos % 8;
                color_bit -= 7;
                color_bit *= -1;

                int color_num = get_bit(data2, color_bit);
                color_num <<= 1;
                color_num |= get_bit(data1, color_bit);

                Color col = get_color(color_num, Memory::BGP);

				int red = get_red_from_colorscheme(BGP, col);
				int green = get_green_from_colorscheme(BGP, col);
				int blue = get_blue_from_colorscheme(BGP, col);

                int LY = memory.read_byte(Memory::LY);

                if((LY >= 0) && (LY < 144) && (pixel >= 0) && (pixel < 160)) {
                    screen[LY][pixel][RED] = red;
	                screen[LY][pixel][GREEN] = green;
	                screen[LY][pixel][BLUE] = blue;
                }
            }
        }
    }

    void Lcd::draw_window() {
        if(is_window_enabled()) {
	        word tile_data = 0;
	        word window_memory = 0;
	        bool is_unsigned = true;
			int LY = memory.read_byte(Memory::LY);

	        byte window_y = memory.read_byte(Memory::WY);
	        byte window_x = memory.read_byte(Memory::WX) - 7;

			// FIXME: Not sure on this. It work for all games I have tested
			if(memory.read_byte(Memory::LY) < window_y) {
				return;
			}

			if(test_bit(memory.read_byte(Memory::LCDC), 4)) {
				tile_data = 0x8000;
			} else {
				tile_data = 0x8800;
				is_unsigned = false;
			}
			
			if(test_bit(memory.read_byte(Memory::LCDC), 6)) {
				window_memory = 0x9C00;
			} else {
				window_memory = 0x9800;
			}
			
	        byte y_pos = LY - window_y;

	        word tile_row = y_pos / 8 * 32;
	        for(int pixel = window_x; pixel < WIDTH; pixel++) {
	            byte x_pos = 0;
	
				if(pixel >= window_x) {
					x_pos = pixel - window_x;
				}

	            word tile_col = (x_pos / 8);

	            sword tile_num;
	            word tile_address = window_memory + tile_row + tile_col;

				if(is_unsigned) {
					tile_num = static_cast<byte>(memory.read_byte(tile_address));
				} else {
					tile_num = static_cast<sbyte>(memory.read_byte(tile_address));
				}
	            
	            word tile_location = tile_data;

	           	if(is_unsigned) {
                    tile_location += (tile_num * 16);
                } else {
                    tile_location += (tile_num + 128) * 16;
                }

	            byte line = y_pos % 8;
	            line *= 2;

	            byte data1 = memory.read_byte(tile_location + line);
	            byte data2 = memory.read_byte(tile_location + line + 1);

	            int color_bit = x_pos % 8;
	            color_bit -= 7;
	            color_bit *= -1;

	            int color_num = get_bit(data2, color_bit);
	            color_num <<= 1;
	            color_num |= get_bit(data1, color_bit);
	
				// NOTE: Color #0 transparency in the window
				if(color_num == 0 && !test_bit(memory.read_byte(Memory::LCDC), 1)) {
					continue;
				}

	            Color col = get_color(color_num, Memory::BGP);

				int red = get_red_from_colorscheme(BGP, col);
				int green = get_green_from_colorscheme(BGP, col);
				int blue = get_blue_from_colorscheme(BGP, col);

	            if((LY >= 0) && (LY < 144) && (pixel >= 0) && (pixel < 160)) {
	                screen[LY][pixel][RED] = red;
	                screen[LY][pixel][GREEN] = green;
	                screen[LY][pixel][BLUE] = blue;
	            }
	        }
        }
    }

    void Lcd::draw_sprites() {
        if(is_sprites_enabled()) {
			bool use_8x16 = false;
			if(test_bit(memory.read_byte(Memory::LCDC), 2)) {
				use_8x16 = true;
			}
			
			for(int sprite = 39; sprite >= 0; sprite--) {
				byte index = sprite * 4;
				byte y_pos = memory.read_byte(Memory::OAM + index) - 16;
				byte x_pos = memory.read_byte(Memory::OAM + index + 1) - 8;
				byte tile_location = memory.read_byte(Memory::OAM + index + 2);
				byte attributes = memory.read_byte(Memory::OAM + index + 3);
				bool bg_priority = test_bit(attributes, 7);

				bool y_flip = test_bit(attributes, 6);
				bool x_flip = test_bit(attributes, 5);

				int LY = memory.read_byte(Memory::LY);
				
				byte y_size = 8;
				if(use_8x16) {
					y_size = 16;
					// Not sure pandocs says: In 8x16 sprite mode,
					// the least significant bit of the sprite pattern number is ignored and treated as 0.
					clear_bit(tile_location, 0);
				}
				
				if((LY >= y_pos) && (LY < (y_pos + y_size))) {
					int line = LY - y_pos;
					
					if(y_flip) {
						line -= (y_size - 1);
						line *= -1;
					}
					
					line *= 2;
					word data_address = (Memory::VRAM + tile_location * 16) + line;
					byte data1 = memory.read_byte(data_address);
					byte data2 = memory.read_byte(data_address + 1);
					
					for(int tile_pixel = 7; tile_pixel >= 0; tile_pixel--) {
						
						int color_bit = tile_pixel;
						
						if(x_flip) {
							color_bit -= 7;
							color_bit *= -1;
						}
						
						int color_num = get_bit(data2, color_bit);
		                color_num <<= 1;
		                color_num |= get_bit(data1, color_bit);
		
						if(color_num == 0) {
							continue;
						}

						word color_address = test_bit(attributes, 4) ? Memory::OBP1 : Memory::OBP0;

		                Color col = get_color(color_num, color_address);

						int scheme_color_index = get_bit(attributes, 4) + 1;

						int red = get_red_from_colorscheme(scheme_color_index, col);
						int green = get_green_from_colorscheme(scheme_color_index, col);
						int blue = get_blue_from_colorscheme(scheme_color_index, col);

		                int x_pix = 0 - tile_pixel;
						x_pix += 7;
						int pixel = x_pos + x_pix;

						if((LY < 0) || (LY >= 144) || (pixel < 0) || (pixel >= 160)) {
							continue;
		                }

						// OBJ-to-BG Priority (0=OBJ Above BG, 1=OBJ Behind BG color 1-3)
						// (Used for both BG and Window. BG color 0 is always behind OBJ)
						if(bg_priority && (background_color(LY, pixel) != WHITE)) {
							continue;
						}

						screen[LY][pixel][RED] = red;
		                screen[LY][pixel][GREEN] = green;
		                screen[LY][pixel][BLUE] = blue;
					}
				}
			}
        }
    }

	// FIXME: This code could be better
	Color Lcd::background_color(const int x, const int y) const {
		int hexcolor = rgb_to_hex(screen[x][y][RED], screen[x][y][GREEN], screen[x][y][BLUE]);
		for(int i = 0; i < 3; i++) {
			if(color_schemes[selected_color_scheme][BGP][i] == hexcolor) {
				return color_indexes[i];
			}
		}
		// TODO: Throw an exception here.
	}

	void Lcd::use_color_scheme(const int scheme) {
		selected_color_scheme = scheme;
	}

	Color Lcd::get_color(const byte index, const word addr) const {
	    Color color;
	    byte palette = memory.read_byte(addr);
		int color_index = (palette >> (index * 2)) & 0x3;
		return color_indexes[color_index];
	}
	
	inline int Lcd::rgb_to_hex(const byte r, const byte g, const byte b) const {
		return ((r << 16) | (g << 8) | b) & 0xFFFFFF;
	}

	inline byte Lcd::get_red(const int hexcolor) const {
		return (hexcolor >> 16) & 0xFF;
	}

	inline byte Lcd::get_green(const int hexcolor) const {
		return (hexcolor >> 8) & 0xFF;
	}

	inline byte Lcd::get_blue(const int hexcolor) const {
		return hexcolor & 0xFF;
	}
	
	inline byte Lcd::get_red_from_colorscheme(const int scheme_color_index, const int color) const {
		int hexcolor = color_schemes[selected_color_scheme][scheme_color_index][color];
		return get_red(hexcolor);
	}

	inline byte Lcd::get_green_from_colorscheme(const int scheme_color_index, const int color) const {
		int hexcolor = color_schemes[selected_color_scheme][scheme_color_index][color];
		return get_green(hexcolor);
	}

	inline byte Lcd::get_blue_from_colorscheme(const int scheme_color_index, const int color) const {
		int hexcolor = color_schemes[selected_color_scheme][scheme_color_index][color];
		return get_blue(hexcolor);
	}

    Lcd& Lcd::get_instance() {
        static Lcd inst;
        return inst;
    }

}
