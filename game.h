#ifndef WIIBATAK_GAME_H
#define WIIBATAK_GAME_H

#include <stdbool.h>

#define PLAYER_COUNT 4
#define HAND_SIZE 13
#define NO_CARD_RANK 0
#define HUMAN_PLAYER 0
#define TARGET_SCORE 151

typedef enum {
	SUIT_CLUBS = 0,
	SUIT_DIAMONDS,
	SUIT_HEARTS,
	SUIT_SPADES,
	SUIT_COUNT
} Suit;

typedef enum {
	PHASE_MENU = 0,
	PHASE_BIDDING,
	PHASE_TRUMP_SELECT,
	PHASE_PLAYING,
	PHASE_SCORING,
	PHASE_GAME_OVER
} GamePhase;

typedef struct {
	int rank;
	Suit suit;
} Card;

typedef struct {
	Card cards[HAND_SIZE];
	int card_count;
	int selected;
	int bid;
	int tricks_won;
	int score;
	bool passed;
	const char *name;
} PlayerState;

typedef struct {
	PlayerState players[PLAYER_COUNT];
	Card trick_cards[PLAYER_COUNT];
	bool trick_filled[PLAYER_COUNT];
	Suit trump;
	GamePhase phase;
	int dealer;
	int active_player;
	int lead_player;
	int round_number;
	int bids_locked;
	int highest_bid;
	int highest_bidder;
	int current_bid;
	int trick_count;
	int last_trick_winner;
	bool trick_ready;
	bool trump_broken;
	unsigned int ai_next_action_ms;
	int round_winner;
	char status[96];
} GameState;

void game_init(GameState *game);
void game_reset_match(GameState *game);
void game_reset_round(GameState *game);
void game_start_bidding(GameState *game);
void game_adjust_bid(GameState *game, int delta);
void game_pass_bid(GameState *game);
void game_confirm_action(GameState *game);
void game_move_selection(GameState *game, int delta);
void game_change_trump(GameState *game, int delta);
bool game_select_card_from_pointer(GameState *game, int pointer_x, int pointer_y);
void game_advance_round(GameState *game);
void game_tick(GameState *game, unsigned int now_ms);

bool game_can_play_selected(const GameState *game);
bool game_is_round_complete(const GameState *game);
bool game_is_human_turn(const GameState *game);
const char *game_phase_label(GamePhase phase);
const char *game_suit_label(Suit suit);
void game_card_label(Card card, char *buffer, int buffer_size);

#endif
