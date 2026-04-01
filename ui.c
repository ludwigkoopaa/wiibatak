#include "ui.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum {
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 480,
	CARD_W = 34,
	CARD_H = 48
};

static SDL_Color suit_color(Suit suit) {
	if (suit == SUIT_DIAMONDS || suit == SUIT_HEARTS) {
		return (SDL_Color){ 176, 38, 38, 255 };
	}
	return (SDL_Color){ 30, 35, 38, 255 };
}

static const unsigned char *glyph_for(char c) {
	switch (c) {
	case 'A': { static const unsigned char g[7] = {14,17,17,31,17,17,17}; return g; }
	case 'B': { static const unsigned char g[7] = {30,17,17,30,17,17,30}; return g; }
	case 'C': { static const unsigned char g[7] = {14,17,16,16,16,17,14}; return g; }
	case 'D': { static const unsigned char g[7] = {30,17,17,17,17,17,30}; return g; }
	case 'E': { static const unsigned char g[7] = {31,16,16,30,16,16,31}; return g; }
	case 'F': { static const unsigned char g[7] = {31,16,16,30,16,16,16}; return g; }
	case 'G': { static const unsigned char g[7] = {14,17,16,23,17,17,14}; return g; }
	case 'H': { static const unsigned char g[7] = {17,17,17,31,17,17,17}; return g; }
	case 'I': { static const unsigned char g[7] = {31,4,4,4,4,4,31}; return g; }
	case 'J': { static const unsigned char g[7] = {7,2,2,2,18,18,12}; return g; }
	case 'K': { static const unsigned char g[7] = {17,18,20,24,20,18,17}; return g; }
	case 'L': { static const unsigned char g[7] = {16,16,16,16,16,16,31}; return g; }
	case 'M': { static const unsigned char g[7] = {17,27,21,21,17,17,17}; return g; }
	case 'N': { static const unsigned char g[7] = {17,25,21,19,17,17,17}; return g; }
	case 'O': { static const unsigned char g[7] = {14,17,17,17,17,17,14}; return g; }
	case 'P': { static const unsigned char g[7] = {30,17,17,30,16,16,16}; return g; }
	case 'Q': { static const unsigned char g[7] = {14,17,17,17,21,18,13}; return g; }
	case 'R': { static const unsigned char g[7] = {30,17,17,30,20,18,17}; return g; }
	case 'S': { static const unsigned char g[7] = {15,16,16,14,1,1,30}; return g; }
	case 'T': { static const unsigned char g[7] = {31,4,4,4,4,4,4}; return g; }
	case 'U': { static const unsigned char g[7] = {17,17,17,17,17,17,14}; return g; }
	case 'V': { static const unsigned char g[7] = {17,17,17,17,17,10,4}; return g; }
	case 'W': { static const unsigned char g[7] = {17,17,17,21,21,21,10}; return g; }
	case 'X': { static const unsigned char g[7] = {17,17,10,4,10,17,17}; return g; }
	case 'Y': { static const unsigned char g[7] = {17,17,10,4,4,4,4}; return g; }
	case 'Z': { static const unsigned char g[7] = {31,1,2,4,8,16,31}; return g; }
	case '0': { static const unsigned char g[7] = {14,17,19,21,25,17,14}; return g; }
	case '1': { static const unsigned char g[7] = {4,12,4,4,4,4,14}; return g; }
	case '2': { static const unsigned char g[7] = {14,17,1,2,4,8,31}; return g; }
	case '3': { static const unsigned char g[7] = {30,1,1,14,1,1,30}; return g; }
	case '4': { static const unsigned char g[7] = {2,6,10,18,31,2,2}; return g; }
	case '5': { static const unsigned char g[7] = {31,16,16,30,1,1,30}; return g; }
	case '6': { static const unsigned char g[7] = {14,16,16,30,17,17,14}; return g; }
	case '7': { static const unsigned char g[7] = {31,1,2,4,8,8,8}; return g; }
	case '8': { static const unsigned char g[7] = {14,17,17,14,17,17,14}; return g; }
	case '9': { static const unsigned char g[7] = {14,17,17,15,1,1,14}; return g; }
	case ':': { static const unsigned char g[7] = {0,4,0,0,4,0,0}; return g; }
	case '-': { static const unsigned char g[7] = {0,0,0,31,0,0,0}; return g; }
	case '+': { static const unsigned char g[7] = {0,4,4,31,4,4,0}; return g; }
	case '/': { static const unsigned char g[7] = {1,2,2,4,8,8,16}; return g; }
	case '.': { static const unsigned char g[7] = {0,0,0,0,0,12,12}; return g; }
	case ' ': { static const unsigned char g[7] = {0,0,0,0,0,0,0}; return g; }
	default: { static const unsigned char g[7] = {31,1,2,4,8,0,8}; return g; }
	}
}

static void set_color(SDL_Renderer *renderer, SDL_Color color) {
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

static void fill_rect(SDL_Renderer *renderer, int x, int y, int w, int h, SDL_Color color) {
	SDL_Rect rect = { x, y, w, h };
	set_color(renderer, color);
	SDL_RenderFillRect(renderer, &rect);
}

static void draw_rect(SDL_Renderer *renderer, int x, int y, int w, int h, SDL_Color color) {
	SDL_Rect rect = { x, y, w, h };
	set_color(renderer, color);
	SDL_RenderDrawRect(renderer, &rect);
}

static void draw_text(SDL_Renderer *renderer, int x, int y, int scale, SDL_Color color, const char *text) {
	int cursor_x = x;

	set_color(renderer, color);
	for (size_t i = 0; i < strlen(text); ++i) {
		const unsigned char *glyph = glyph_for(text[i]);
		for (int row = 0; row < 7; ++row) {
			for (int col = 0; col < 5; ++col) {
				if (glyph[row] & (1 << (4 - col))) {
					SDL_Rect pixel = {
						cursor_x + col * scale,
						y + row * scale,
						scale,
						scale
					};
					SDL_RenderFillRect(renderer, &pixel);
				}
			}
		}
		cursor_x += 6 * scale;
	}
}

static void draw_suit_symbol(SDL_Renderer *renderer, Suit suit, int x, int y, int scale, SDL_Color color) {
	set_color(renderer, color);

	if (suit == SUIT_DIAMONDS) {
		for (int row = 0; row < 7 * scale; ++row) {
			int dist = row < (7 * scale) / 2 ? row : (7 * scale - 1 - row);
			int width = dist * 2 + scale;
			SDL_Rect rect = { x + 3 * scale - width / 2, y + row, width, 1 };
			SDL_RenderFillRect(renderer, &rect);
		}
		return;
	}

	if (suit == SUIT_HEARTS) {
		fill_rect(renderer, x + scale, y + scale, 2 * scale, 2 * scale, color);
		fill_rect(renderer, x + 3 * scale, y + scale, 2 * scale, 2 * scale, color);
		for (int row = 0; row < 5 * scale; ++row) {
			int width = (5 * scale) - row;
			SDL_Rect rect = { x + scale + row / 2, y + 2 * scale + row, width, 1 };
			SDL_RenderFillRect(renderer, &rect);
		}
		return;
	}

	if (suit == SUIT_SPADES) {
		for (int row = 0; row < 5 * scale; ++row) {
			int width = row + 1;
			SDL_Rect left = { x + 2 * scale - width, y + row, width, 1 };
			SDL_Rect right = { x + 3 * scale, y + row, width, 1 };
			SDL_RenderFillRect(renderer, &left);
			SDL_RenderFillRect(renderer, &right);
		}
		fill_rect(renderer, x + 2 * scale, y + 2 * scale, 2 * scale, 3 * scale, color);
		fill_rect(renderer, x + 2 * scale + scale / 2, y + 5 * scale, scale, 2 * scale, color);
		return;
	}

	fill_rect(renderer, x + 2 * scale, y + scale, 2 * scale, 2 * scale, color);
	fill_rect(renderer, x + scale, y + 2 * scale, 2 * scale, 2 * scale, color);
	fill_rect(renderer, x + 3 * scale, y + 2 * scale, 2 * scale, 2 * scale, color);
	fill_rect(renderer, x + 2 * scale + scale / 2, y + 4 * scale, scale, 3 * scale, color);
}

static void draw_card(SDL_Renderer *renderer, int x, int y, Card card, bool selected, bool legal, bool face_up) {
	SDL_Color body = face_up ? (SDL_Color){ 238, 226, 195, 255 } : (SDL_Color){ 41, 72, 124, 255 };
	SDL_Color border = (selected && !legal)
		? (SDL_Color){ 208, 54, 54, 255 }
		: (selected ? (SDL_Color){ 255, 214, 10, 255 } : (SDL_Color){ 25, 29, 35, 255 });
	SDL_Color text = suit_color(card.suit);
	char label[4];

	fill_rect(renderer, x, y, CARD_W, CARD_H, body);
	draw_rect(renderer, x, y, CARD_W, CARD_H, border);
	draw_rect(renderer, x + 1, y + 1, CARD_W - 2, CARD_H - 2, border);

	if (!face_up) {
		fill_rect(renderer, x + 5, y + 5, CARD_W - 10, CARD_H - 10, (SDL_Color){ 19, 44, 77, 255 });
		draw_text(renderer, x + 8, y + 18, 1, (SDL_Color){ 230, 232, 235, 255 }, "BAT");
		return;
	}

	game_card_label(card, label, sizeof(label));
	if (strlen(label) > 0) {
		label[strlen(label) - 1] = '\0';
	}
	draw_text(renderer, x + 4, y + 4, 1, text, label);
	draw_suit_symbol(renderer, card.suit, x + 10, y + 24, 2, text);
}

static void draw_player_panel(SDL_Renderer *renderer, int x, int y, int w, int h, const PlayerState *player, bool active) {
	SDL_Color panel = active ? (SDL_Color){ 43, 72, 53, 255 } : (SDL_Color){ 31, 45, 35, 255 };
	SDL_Color border = active ? (SDL_Color){ 255, 214, 10, 255 } : (SDL_Color){ 68, 92, 76, 255 };
	char line[48];

	fill_rect(renderer, x, y, w, h, panel);
	draw_rect(renderer, x, y, w, h, border);

	snprintf(line, sizeof(line), "%s", player->name);
	draw_text(renderer, x + 8, y + 8, 2, (SDL_Color){ 248, 245, 231, 255 }, line);

	snprintf(line, sizeof(line), "EL:%d", player->card_count);
	draw_text(renderer, x + 8, y + 32, 1, (SDL_Color){ 229, 237, 223, 255 }, line);

	snprintf(line, sizeof(line), "IHALE:%d", player->bid);
	draw_text(renderer, x + 8, y + 46, 1, (SDL_Color){ 229, 237, 223, 255 }, line);

	snprintf(line, sizeof(line), "ALDI:%d", player->tricks_won);
	draw_text(renderer, x + 8, y + 58, 1, (SDL_Color){ 229, 237, 223, 255 }, line);

	snprintf(line, sizeof(line), "SKOR:%d", player->score);
	draw_text(renderer, x + 8, y + 70, 1, (SDL_Color){ 229, 237, 223, 255 }, line);
}

static void draw_hand_horizontal(SDL_Renderer *renderer, const GameState *game, int player_index, int start_x, int y, const PlayerState *player, bool active, bool face_up) {
	int selected_index = active ? player->selected : -1;

	for (int i = 0; i < player->card_count; ++i) {
		if (i == selected_index) {
			continue;
		}
		int x = start_x + i * 18;
		int lift = 0;
		bool selected = false;
		bool legal = !selected || player_index != HUMAN_PLAYER || game_can_play_selected(game);
		draw_card(renderer, x, y + lift, player->cards[i], selected, legal, face_up);
	}

	if (selected_index >= 0 && selected_index < player->card_count) {
		int x = start_x + selected_index * 18;
		bool legal = player_index != HUMAN_PLAYER || game_can_play_selected(game);
		draw_card(renderer, x, y - 14, player->cards[selected_index], true, legal, face_up);
		fill_rect(renderer, x + 4, y + 38, CARD_W - 8, 3, (SDL_Color){ 255, 214, 10, 255 });
		fill_rect(renderer, x + 11, y + 42, CARD_W - 22, 3, (SDL_Color){ 255, 214, 10, 255 });
	}
}

static void draw_hand_vertical(SDL_Renderer *renderer, int x, int start_y, const PlayerState *player, bool active, bool face_up) {
	int selected_index = active ? player->selected : -1;

	for (int i = 0; i < player->card_count; ++i) {
		if (i == selected_index) {
			continue;
		}
		int y = start_y + i * 12;
		draw_card(renderer, x, y, player->cards[i], false, true, face_up);
	}

	if (selected_index >= 0 && selected_index < player->card_count) {
		int y = start_y + selected_index * 12;
		draw_card(renderer, x + 10, y, player->cards[selected_index], true, true, face_up);
	}
}

static void draw_trick_area(SDL_Renderer *renderer, const GameState *game) {
	static const SDL_Point positions[PLAYER_COUNT] = {
		{ 303, 295 },
		{ 374, 212 },
		{ 303, 125 },
		{ 230, 212 }
	};
	SDL_Color center = { 26, 72, 48, 255 };

	fill_rect(renderer, 210, 145, 220, 170, center);
	draw_rect(renderer, 210, 145, 220, 170, (SDL_Color){ 194, 162, 94, 255 });
	draw_text(renderer, 255, 156, 2, (SDL_Color){ 245, 234, 201, 255 }, "ORTA");
	if (game->last_trick_winner >= 0 && !game->trick_ready) {
		char winner[32];
		snprintf(winner, sizeof(winner), "ALAN:%s", game->players[game->last_trick_winner].name);
		draw_text(renderer, 246, 178, 1, (SDL_Color){ 241, 228, 189, 255 }, winner);
	}

	for (int i = 0; i < PLAYER_COUNT; ++i) {
		if (game->trick_filled[i]) {
			draw_card(renderer, positions[i].x, positions[i].y, game->trick_cards[i], false, true, true);
		}
	}
}

void ui_render(SDL_Renderer *renderer, const GameState *game, bool paused, bool show_help) {
	char line[64];
	const PlayerState *bottom = &game->players[0];
	const PlayerState *right = &game->players[1];
	const PlayerState *top = &game->players[2];
	const PlayerState *left = &game->players[3];

	fill_rect(renderer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (SDL_Color){ 18, 88, 58, 255 });
	fill_rect(renderer, 10, 10, SCREEN_WIDTH - 20, 84, (SDL_Color){ 28, 54, 61, 255 });
	draw_rect(renderer, 10, 10, SCREEN_WIDTH - 20, 84, (SDL_Color){ 182, 159, 104, 255 });

	draw_text(renderer, 24, 22, 2, (SDL_Color){ 245, 241, 222, 255 }, "WII BATAK");
	snprintf(line, sizeof(line), "FAZ:%s", game_phase_label(game->phase));
	draw_text(renderer, 24, 50, 1, (SDL_Color){ 221, 229, 216, 255 }, line);
	snprintf(line, sizeof(line), "KOZ:%s", game_suit_label(game->trump));
	draw_text(renderer, 142, 50, 1, (SDL_Color){ 221, 229, 216, 255 }, line);
	if (game->highest_bidder >= 0) {
		snprintf(line, sizeof(line), "MAX:%d %s", game->highest_bid, game->players[game->highest_bidder].name);
		draw_text(renderer, 142, 64, 1, (SDL_Color){ 255, 214, 10, 255 }, line);
	}
	snprintf(line, sizeof(line), "EL:%d", game->round_number);
	draw_text(renderer, 248, 50, 1, (SDL_Color){ 221, 229, 216, 255 }, line);
	snprintf(line, sizeof(line), "HEDEF:%d", TARGET_SCORE);
	draw_text(renderer, 248, 64, 1, (SDL_Color){ 221, 229, 216, 255 }, line);
	snprintf(line, sizeof(line), "SIRA:%s", game->players[game->active_player].name);
	draw_text(renderer, 312, 50, 1, (SDL_Color){ 255, 214, 10, 255 }, line);

	draw_text(renderer, 380, 22, 1, (SDL_Color){ 224, 229, 219, 255 }, "HOME CIKIS");
	draw_text(renderer, 380, 36, 1, (SDL_Color){ 224, 229, 219, 255 }, "PLUS DURAKLAT");
	draw_text(renderer, 380, 50, 1, (SDL_Color){ 224, 229, 219, 255 }, "MINUS YENI EL");
	draw_text(renderer, 380, 64, 1, (SDL_Color){ 224, 229, 219, 255 }, "A ONAY B PAS");

	draw_player_panel(renderer, 228, 94, 184, 86, top, game->active_player == 2);
	draw_player_panel(renderer, 438, 168, 184, 86, right, game->active_player == 1);
	draw_player_panel(renderer, 18, 168, 184, 86, left, game->active_player == 3);
	draw_player_panel(renderer, 228, 360, 184, 86, bottom, game->active_player == 0);

	draw_hand_horizontal(renderer, game, 2, 206, 183, top, game->active_player == 2, false);
	draw_hand_vertical(renderer, 188, 158, left, game->active_player == 3, false);
	draw_hand_vertical(renderer, 418, 158, right, game->active_player == 1, false);
	draw_hand_horizontal(renderer, game, 0, 206, 404, bottom, game->active_player == 0, true);

	draw_trick_area(renderer, game);
	draw_text(renderer, 24, 96, 1, (SDL_Color){ 248, 245, 231, 255 }, game->status);

	if (game->phase == PHASE_MENU) {
		fill_rect(renderer, 148, 208, 344, 62, (SDL_Color){ 31, 33, 48, 220 });
		draw_rect(renderer, 148, 208, 344, 62, (SDL_Color){ 255, 214, 10, 255 });
		draw_text(renderer, 170, 222, 2, (SDL_Color){ 248, 245, 231, 255 }, "A ILE BASLA");
	} else if (game->phase == PHASE_BIDDING) {
		snprintf(line, sizeof(line), "TEKLIF:%d", game->current_bid);
		fill_rect(renderer, 175, 208, 290, 62, (SDL_Color){ 31, 33, 48, 220 });
		draw_rect(renderer, 175, 208, 290, 62, (SDL_Color){ 255, 214, 10, 255 });
		if (game->active_player == HUMAN_PLAYER) {
			draw_text(renderer, 200, 222, 2, (SDL_Color){ 248, 245, 231, 255 }, line);
			draw_text(renderer, 168, 326, 1, (SDL_Color){ 248, 245, 231, 255 }, "1/2 DEGIS A ONAY B PAS");
		} else {
			snprintf(line, sizeof(line), "%s DUSUNUYOR", game->players[game->active_player].name);
			draw_text(renderer, 176, 222, 2, (SDL_Color){ 248, 245, 231, 255 }, line);
		}
	} else if (game->phase == PHASE_TRUMP_SELECT) {
		fill_rect(renderer, 162, 208, 316, 62, (SDL_Color){ 31, 33, 48, 220 });
		draw_rect(renderer, 162, 208, 316, 62, (SDL_Color){ 255, 214, 10, 255 });
		snprintf(line, sizeof(line), "KOZ:%s", game_suit_label(game->trump));
		draw_text(renderer, 188, 222, 2, (SDL_Color){ 248, 245, 231, 255 }, line);
		draw_text(renderer, 126, 326, 1, (SDL_Color){ 248, 245, 231, 255 }, game->highest_bidder == HUMAN_PLAYER ? "SOL/SAG KOZ SEC A BASLAT" : "AI KOZU OTOMATIK SECIYOR");
	} else if (game->phase == PHASE_PLAYING) {
		if (game->trick_ready) {
			draw_text(renderer, 194, 326, 1, (SDL_Color){ 248, 245, 231, 255 }, "EL KAPANIYOR");
		} else if (game->active_player == HUMAN_PLAYER) {
			draw_text(renderer, 198, 326, 1, (SDL_Color){ 248, 245, 231, 255 }, "SOL SAG KART SEC A OYNA");
		} else {
			draw_text(renderer, 194, 326, 1, (SDL_Color){ 248, 245, 231, 255 }, "RAKIP KART OYNUYOR");
		}
	} else if (game->phase == PHASE_SCORING) {
		fill_rect(renderer, 126, 196, 388, 96, (SDL_Color){ 31, 33, 48, 220 });
		draw_rect(renderer, 126, 196, 388, 96, (SDL_Color){ 255, 214, 10, 255 });
		snprintf(line, sizeof(line), "KONTRAT:%s %d", game->players[game->highest_bidder].name, game->highest_bid);
		draw_text(renderer, 148, 212, 1, (SDL_Color){ 248, 245, 231, 255 }, line);
		snprintf(line, sizeof(line), "EL ALAN EN IYI:%s", game->players[game->round_winner].name);
		draw_text(renderer, 148, 232, 1, (SDL_Color){ 248, 245, 231, 255 }, line);
		snprintf(line, sizeof(line), "IH.%d ALDI:%d SKOR:%d", game->players[game->highest_bidder].bid, game->players[game->highest_bidder].tricks_won, game->players[game->highest_bidder].score);
		draw_text(renderer, 148, 252, 1, (SDL_Color){ 248, 245, 231, 255 }, line);
		draw_text(renderer, 158, 326, 1, (SDL_Color){ 248, 245, 231, 255 }, "A ILE OZETI KAPAT VE YENI EL BASLAT");
	} else if (game->phase == PHASE_GAME_OVER) {
		fill_rect(renderer, 108, 186, 424, 118, (SDL_Color){ 31, 33, 48, 228 });
		draw_rect(renderer, 108, 186, 424, 118, (SDL_Color){ 255, 214, 10, 255 });
		snprintf(line, sizeof(line), "%s KAZANDI", game->players[game->round_winner].name);
		draw_text(renderer, 190, 206, 2, (SDL_Color){ 248, 245, 231, 255 }, line);
		snprintf(line, sizeof(line), "SKOR:%d", game->players[game->round_winner].score);
		draw_text(renderer, 160, 238, 1, (SDL_Color){ 248, 245, 231, 255 }, line);
		draw_text(renderer, 150, 326, 1, (SDL_Color){ 248, 245, 231, 255 }, "A ILE YENI MAC BASLAT");
	}

	if (paused) {
		fill_rect(renderer, 92, 122, 456, 236, (SDL_Color){ 16, 20, 26, 220 });
		draw_rect(renderer, 92, 122, 456, 236, (SDL_Color){ 255, 214, 10, 255 });
		draw_text(renderer, 232, 142, 2, (SDL_Color){ 248, 245, 231, 255 }, "DURAKLADI");
		if (show_help) {
			draw_text(renderer, 122, 184, 1, (SDL_Color){ 248, 245, 231, 255 }, "A DEVAM ET");
			draw_text(renderer, 122, 202, 1, (SDL_Color){ 248, 245, 231, 255 }, "B YARDIM GIZLE");
			draw_text(renderer, 122, 220, 1, (SDL_Color){ 248, 245, 231, 255 }, "HOME/START CIK");
			draw_text(renderer, 122, 248, 1, (SDL_Color){ 248, 245, 231, 255 }, "IHALE: 1/2 DEGISTIR A ONAY B PAS");
			draw_text(renderer, 122, 266, 1, (SDL_Color){ 248, 245, 231, 255 }, "KOZ: SOL/SAG SEC A BASLAT");
			draw_text(renderer, 122, 284, 1, (SDL_Color){ 248, 245, 231, 255 }, "OYUN: SOL/SAG KART SEC A OYNA");
			draw_text(renderer, 122, 302, 1, (SDL_Color){ 248, 245, 231, 255 }, "KIRMIZI CERCEVE: GECERSIZ SECIM");
			draw_text(renderer, 122, 320, 1, (SDL_Color){ 248, 245, 231, 255 }, "ENTER/SPACE VE SOL TIK = ONAY");
		} else {
			draw_text(renderer, 166, 236, 1, (SDL_Color){ 248, 245, 231, 255 }, "A DEVAM ET  B YARDIM GOSTER");
		}
	}
}
