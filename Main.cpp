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

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <getopt.h>
#include "libgbpp/GameBoy.h"

using namespace gbpp;

unsigned int WIDTH = GameBoy::WIDTH;
unsigned int HEIGHT = GameBoy::HEIGHT;

// Screen magnification
int magnification = 1;
int color_scheme = 0;
bool hflag = false;
bool cflag = false;
bool vflag = false;
bool skip_bios_flag = false;
char *stream_name;
unsigned int fps = 0;

GameBoy game_boy;

void show_version();
void show_usage();
void show_copyright();
void init_sdl();
void init_opengl();
void emulation_loop();
void update_screen();
void check_input(SDL_Event event);
void check_colorscheme_change(const SDL_Event event);

int main(int argc, char *argv[]) {
	int option_index = 0;
	int c;
	
	static struct option long_options[] = {
		{"version", no_argument, 0, 'v'},
		{"skip-bios", no_argument, 0, 'k'},
		{"magnification", required_argument, 0, 'm'},
		{"color-scheme", required_argument, 0, 's'},
		{"help", no_argument, 0, 'h'},
		{"copyright", no_argument, 0, 'c'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, "s:hcvm:k", long_options, &option_index)) != -1) {
		switch (c) {
		case 'm':
			if(atoi(optarg) >= 1 && atoi(optarg) <= 4) {
				magnification = atoi(optarg);
			}
			break;
		case 's':
			if(atoi(optarg) >= 0 && atoi(optarg) <= 9) {
				color_scheme = atoi(optarg);
			}
			break;
		case 'k':
			skip_bios_flag = true;
			break;
		case 'h':
			hflag = true;
			break;
		case 'c':
			cflag = true;
			break;
		case 'v':
			vflag = true;
			break;
		}
	}
	
	WIDTH *= magnification;
	HEIGHT *= magnification;
	
	stream_name = argv[0];
	argv += optind;
	
	if(argc < 2 || hflag) {
		show_usage();
		exit(EXIT_SUCCESS);
	}
	if(vflag) {
		show_version();
		exit(EXIT_SUCCESS);
	}
	if(cflag) {
		show_copyright();
		exit(EXIT_SUCCESS);
	}
	
	try {
		game_boy.use_color_scheme(color_scheme);
		game_boy.power_on(argv[0], skip_bios_flag);
	} catch(BadCartridge e) {
		std::cerr << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
	
	init_sdl();
	init_opengl();
	emulation_loop();
	return EXIT_SUCCESS;
}

void init_sdl() {
	char env[] = "SDL_VIDEO_CENTERED=1";
	putenv(env);
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		std::cerr << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
		exit(EXIT_FAILURE);
	}
	if(SDL_SetVideoMode(WIDTH, HEIGHT, 8, SDL_OPENGL) == NULL) {
		std::cerr << "Unable to set video mode: " << SDL_GetError() << std::endl;
		exit(EXIT_FAILURE);
	}
	atexit(SDL_Quit);
	SDL_WM_SetCaption("Loading...", NULL);
}

void init_opengl() {
	//glViewport(0, 0, WIDTH, HEIGHT);
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//glOrtho(0, WIDTH, HEIGHT, 0, -1.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glShadeModel(GL_FLAT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DITHER);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_FOG);
	glClearColor(0, 0, 0, 0);
}

void update_screen() {
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 	//glLoadIdentity();
 	glRasterPos2i(-1, 1);
	glPixelZoom(magnification, magnification * -1);
 	glDrawPixels(Lcd::WIDTH, Lcd::HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, lcd.screen); // TODO: hide the lcd from here.
	glFlush();
	SDL_GL_SwapBuffers();
}

void check_input(const SDL_Event event) {
	int key = -1;
	if(event.type == SDL_KEYDOWN) {
		switch(event.key.keysym.sym) {
			case SDLK_z:
				key = GameBoy::KEY_B;
				break;
			case SDLK_x:
				key = GameBoy::KEY_A;
				break;
			case SDLK_RETURN:
				key = GameBoy::KEY_START;
				break;
			case SDLK_SPACE:
				key = GameBoy::KEY_SELECT;
				break;
			case SDLK_RIGHT:
				key = GameBoy::KEY_RIGHT;
				break;
			case SDLK_LEFT:
				key = GameBoy::KEY_LEFT;
				break;
			case SDLK_UP:
				key = GameBoy::KEY_UP;
				break;
			case SDLK_DOWN:
				key = GameBoy::KEY_DOWN;
		}
		if(key != -1) {
			game_boy.key_pressed(key);
		}
	} else if(event.type == SDL_KEYUP) {
		switch(event.key.keysym.sym) {
			case SDLK_z:
				key = GameBoy::KEY_B;
				break;
			case SDLK_x:
				key = GameBoy::KEY_A;
				break;
			case SDLK_RETURN:
				key = GameBoy::KEY_START;
				break;
			case SDLK_SPACE:
				key = GameBoy::KEY_SELECT;
				break;
			case SDLK_RIGHT:
				key = GameBoy::KEY_RIGHT;
				break;
			case SDLK_LEFT:
				key = GameBoy::KEY_LEFT;
				break;
			case SDLK_UP:
				key = GameBoy::KEY_UP;
				break;
			case SDLK_DOWN:
				key = GameBoy::KEY_DOWN;
				break;
		}
		if(key != -1) {
			game_boy.key_released(key);
		}
	}
}

void check_colorscheme_change(const SDL_Event event) {
	int scheme = -1;
	if(event.type == SDL_KEYDOWN) {
		if(event.key.keysym.sym == SDLK_0) {
			scheme = 0;
		} else if(event.key.keysym.sym == SDLK_1) {
			scheme = 1;
		} else if(event.key.keysym.sym == SDLK_2) {
			scheme = 2;
		} else if(event.key.keysym.sym == SDLK_3) {
			scheme = 3;
		} else if(event.key.keysym.sym == SDLK_4) {
			scheme = 4;
		} else if(event.key.keysym.sym == SDLK_5) {
			scheme = 5;
		} else if(event.key.keysym.sym == SDLK_6) {
			scheme = 6;
		} else if(event.key.keysym.sym == SDLK_7) {
			scheme = 7;
		} else if(event.key.keysym.sym == SDLK_8) {
			scheme = 8;
		} else if(event.key.keysym.sym == SDLK_9) {
			scheme = 9;
		}
	}
	if(scheme != -1) {
		game_boy.use_color_scheme(scheme);	
	}
}

void emulation_loop() {
	SDL_Event event;
	bool running = true;

	const float MILI_FPS = (1000.0 / GameBoy::FPS);
	const unsigned int FRAME_ADJUST = 40;
	unsigned int second = 0;
	float time_to_sleep;
	unsigned int before;
	unsigned int after;
	char buffer[30];

	while(running) {
		before = SDL_GetTicks();
		while(SDL_PollEvent(&event)) {
			check_input(event);
			check_colorscheme_change(event);

			if(event.type == SDL_QUIT) {
				running = false;
			} else if(event.type == SDL_KEYDOWN) {
				if(event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
					running = false;
				}
			}
		}
		game_boy.frame(); // do a frame
		update_screen(); // update based on the last frame
		after = SDL_GetTicks();
		
		// Adjust 60 fps
		// Bacause 1000/60 = 16.6666
		// I need to adjust to render 60 frames in one second
		// so I do (40 * 17) + (20 * 16) = 1000
		if(fps <= FRAME_ADJUST) {
			time_to_sleep = round(MILI_FPS) - (after - before);
		} else {
			time_to_sleep = static_cast<unsigned int>(MILI_FPS) - (after - before);
		}
		
		if(time_to_sleep > 0 && time_to_sleep < MILI_FPS) {
			SDL_Delay(time_to_sleep);
		}

		if(second <= 1000) {
			second += (SDL_GetTicks() - before);
			fps++;
		} else {
			sprintf(buffer, "%d fps (%.1lf%%)", fps,
				(100 * fps) / static_cast<float>(GameBoy::FPS));
			SDL_WM_SetCaption(buffer, NULL);
			second = 0;
			fps = 0;
		}
	}
}

inline void show_copyright() {
	std::cout << "GBPP - Copyright (C) 2010, 2011 Claudemiro Feitosa <dimiro1@gmail.com>" << std::endl;
}

inline void show_usage() {
	std::cerr << "usage: " << stream_name << " [options] romfile.gb." << std::endl
		<< "Available options are:" << std::endl
		<< "  -k [skip-bios] \t\tSkip bios" << std::endl
		<< "  -s [color-scheme] scheme \tselect color scheme (0-8)" << std::endl
		<< "  -v [version] \t\t\tprint version" << std::endl
		<< "  -m [magnification] s \t\tscreen magnification (0-4)" << std::endl
		<< "  -h [help]\t\t\tshow this help" << std::endl
		<< "  -c [copyright]\t\tcopyright information" << std::endl << std::endl
		<< "Cross Platform Nintendo(R) GameBoy(R) (DMG, MGB, MGL) emulator written in C++ with SDL/OpenGL." << std::endl;
	show_copyright();
}

inline void show_version() {
    std::cout << "GBPP v" << gbpp::VERSION_MAJOR
		<< "." << gbpp::VERSION_MINOR
		<< "." << gbpp::VERSION_PATH
#ifdef DEBUG
	std::cout << " DEBUG"
#endif
 	<< std::endl;
}
