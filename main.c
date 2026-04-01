#include <SDL2/SDL.h>
#include <gccore.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ogc/pad.h>
#include <wiiuse/wpad.h>

#include "game.h"
#include "ui.h"

static bool handle_input(GameState *game, u32 pressed_down, u32 pressed_held, u16 gc_down, u16 gc_held,
	bool keyboard_confirm, bool keyboard_pass, bool key_left, bool key_right, bool key_up, bool key_down,
	bool key_bid_down, bool key_bid_up) {
	bool confirm = (pressed_down & WPAD_BUTTON_A) || (pressed_held & WPAD_BUTTON_A) || (gc_down & PAD_BUTTON_A) || (gc_held & PAD_BUTTON_A) || keyboard_confirm;
	bool pass = (pressed_down & WPAD_BUTTON_B) || (gc_down & PAD_BUTTON_B) || keyboard_pass;

	if ((pressed_down & WPAD_BUTTON_HOME) || (pressed_held & WPAD_BUTTON_HOME)) {
		return false;
	}
	if ((gc_down & PAD_BUTTON_START) || (gc_held & PAD_BUTTON_START)) {
		return false;
	}
	if (game->phase == PHASE_MENU && (confirm || pressed_down != 0 || gc_down != 0)) {
		game_confirm_action(game);
		return true;
	}
	if (pressed_down & WPAD_BUTTON_MINUS) {
		game_reset_round(game);
	}
	if (!game_is_human_turn(game)) {
		return true;
	}
	if ((pressed_down & WPAD_BUTTON_LEFT) || (gc_down & PAD_BUTTON_LEFT) || key_left) {
		if (game->phase == PHASE_TRUMP_SELECT) {
			game_change_trump(game, -1);
		} else {
			game_move_selection(game, -1);
		}
	}
	if ((pressed_down & WPAD_BUTTON_RIGHT) || (gc_down & PAD_BUTTON_RIGHT) || key_right) {
		if (game->phase == PHASE_TRUMP_SELECT) {
			game_change_trump(game, 1);
		} else {
			game_move_selection(game, 1);
		}
	}
	if ((pressed_down & WPAD_BUTTON_UP) || (gc_down & PAD_BUTTON_UP) || key_up) {
		if (game->phase == PHASE_TRUMP_SELECT) {
			game_change_trump(game, -1);
		}
	}
	if ((pressed_down & WPAD_BUTTON_DOWN) || (gc_down & PAD_BUTTON_DOWN) || key_down) {
		if (game->phase == PHASE_TRUMP_SELECT) {
			game_change_trump(game, 1);
		}
	}
	if ((pressed_down & WPAD_BUTTON_1) || (gc_down & PAD_TRIGGER_L) || key_bid_down) {
		if (game->phase == PHASE_TRUMP_SELECT) {
			game_change_trump(game, -1);
		} else {
			game_adjust_bid(game, -1);
		}
	}
	if ((pressed_down & WPAD_BUTTON_2) || (gc_down & PAD_TRIGGER_R) || key_bid_up) {
		if (game->phase == PHASE_TRUMP_SELECT) {
			game_change_trump(game, 1);
		} else {
			game_adjust_bid(game, 1);
		}
	}
	if (pass) {
		game_pass_bid(game);
	}
	if (confirm) {
		game_confirm_action(game);
	}

	return true;
}

int main(int argc, char **argv) {
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	GameState game;
	bool running = true;
	bool paused = false;
	bool show_help = false;
	bool keyboard_confirm = false;
	bool keyboard_pass = false;
	bool key_left = false;
	bool key_right = false;
	bool key_up = false;
	bool key_down = false;
	bool key_bid_down = false;
	bool key_bid_up = false;

	VIDEO_Init();
	PAD_Init();
	WPAD_Init();
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		exit(1);
	}

	window = SDL_CreateWindow("Wii Batak", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_FULLSCREEN);
	if (!window) {
		SDL_Quit();
		exit(1);
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(1);
	}

	game_init(&game);

	while (running && SYS_MainLoop()) {
		SDL_Event event;
		u32 pressed_down;
		u32 pressed_held;
		u16 gc_down;
		u16 gc_held;

		keyboard_confirm = false;
		keyboard_pass = false;
		key_left = false;
		key_right = false;
		key_up = false;
		key_down = false;
		key_bid_down = false;
		key_bid_up = false;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = false;
			}
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
					keyboard_confirm = true;
				}
				if (event.key.keysym.sym == SDLK_BACKSPACE || event.key.keysym.sym == SDLK_ESCAPE) {
					keyboard_pass = true;
				}
				if (event.key.keysym.sym == SDLK_LEFT) {
					key_left = true;
				}
				if (event.key.keysym.sym == SDLK_a) {
					key_left = true;
				}
				if (event.key.keysym.sym == SDLK_RIGHT) {
					key_right = true;
				}
				if (event.key.keysym.sym == SDLK_d) {
					key_right = true;
				}
				if (event.key.keysym.sym == SDLK_UP) {
					key_up = true;
				}
				if (event.key.keysym.sym == SDLK_w) {
					key_up = true;
				}
				if (event.key.keysym.sym == SDLK_DOWN) {
					key_down = true;
				}
				if (event.key.keysym.sym == SDLK_s) {
					key_down = true;
				}
				if (event.key.keysym.sym == SDLK_1 || event.key.keysym.sym == SDLK_KP_1) {
					key_bid_down = true;
				}
				if (event.key.keysym.sym == SDLK_2 || event.key.keysym.sym == SDLK_KP_2) {
					key_bid_up = true;
				}
			}
			if (event.type == SDL_MOUSEMOTION) {
				game_select_card_from_pointer(&game, event.motion.x, event.motion.y);
			}
			if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
				int selected_before = game.players[HUMAN_PLAYER].selected;
				bool hit_card = game_select_card_from_pointer(&game, event.button.x, event.button.y);
				if (hit_card && game.players[HUMAN_PLAYER].selected == selected_before) {
					keyboard_confirm = true;
				}
			}
			if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
				keyboard_pass = true;
			}
		}

		WPAD_ScanPads();
		PAD_ScanPads();
		pressed_down = WPAD_ButtonsDown(0);
		pressed_held = WPAD_ButtonsHeld(0);
		gc_down = PAD_ButtonsDown(0);
		gc_held = PAD_ButtonsHeld(0);

		if ((pressed_down & WPAD_BUTTON_PLUS) || (gc_down & PAD_BUTTON_X)) {
			paused = !paused;
			show_help = paused;
		}

		if (!paused) {
			running = handle_input(&game, pressed_down, pressed_held, gc_down, gc_held,
				keyboard_confirm, keyboard_pass, key_left, key_right, key_up, key_down, key_bid_down, key_bid_up);
			game_tick(&game, SDL_GetTicks());
		} else {
			if ((pressed_down & WPAD_BUTTON_HOME) || (gc_down & PAD_BUTTON_START)) {
				running = false;
			}
			if ((pressed_down & WPAD_BUTTON_A) || (gc_down & PAD_BUTTON_A) || keyboard_confirm) {
				paused = false;
				show_help = false;
			}
			if ((pressed_down & WPAD_BUTTON_B) || (gc_down & PAD_BUTTON_B) || keyboard_pass) {
				show_help = !show_help;
			}
		}

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderClear(renderer);
		ui_render(renderer, &game, paused, show_help);
		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
