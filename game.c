#include "game.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *k_player_names[PLAYER_COUNT] = {
	"SOUTH",
	"EAST",
	"NORTH",
	"WEST"
};

static void score_round(GameState *game);
static int current_trick_winner(const GameState *game);
static void lock_bid(GameState *game, int bid_value);

static void set_status(GameState *game, const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vsnprintf(game->status, sizeof(game->status), fmt, args);
	va_end(args);
}

static int suit_sort_value(Suit suit) {
	switch (suit) {
	case SUIT_SPADES: return 0;
	case SUIT_HEARTS: return 1;
	case SUIT_DIAMONDS: return 2;
	case SUIT_CLUBS: return 3;
	default: return 4;
	}
}

static void sort_hand(PlayerState *player) {
	for (int i = 0; i < player->card_count - 1; ++i) {
		for (int j = i + 1; j < player->card_count; ++j) {
			int left = suit_sort_value(player->cards[i].suit) * 100 + player->cards[i].rank;
			int right = suit_sort_value(player->cards[j].suit) * 100 + player->cards[j].rank;
			if (left > right) {
				Card tmp = player->cards[i];
				player->cards[i] = player->cards[j];
				player->cards[j] = tmp;
			}
		}
	}
}

static void remove_card_at(PlayerState *player, int index) {
	if (index < 0 || index >= player->card_count) {
		return;
	}

	for (int i = index; i < player->card_count - 1; ++i) {
		player->cards[i] = player->cards[i + 1];
	}

	player->card_count--;
	if (player->card_count <= 0) {
		player->selected = 0;
	} else if (player->selected >= player->card_count) {
		player->selected = player->card_count - 1;
	}
}

static void shuffle_deck(Card *deck, int count) {
	for (int i = count - 1; i > 0; --i) {
		int j = rand() % (i + 1);
		Card tmp = deck[i];
		deck[i] = deck[j];
		deck[j] = tmp;
	}
}

static bool player_has_suit(const PlayerState *player, Suit suit) {
	for (int i = 0; i < player->card_count; ++i) {
		if (player->cards[i].suit == suit) {
			return true;
		}
	}
	return false;
}

static bool player_has_non_trump(const PlayerState *player, Suit trump) {
	for (int i = 0; i < player->card_count; ++i) {
		if (player->cards[i].suit != trump) {
			return true;
		}
	}
	return false;
}

static int highest_rank_of_suit(const PlayerState *player, Suit suit) {
	int highest = 0;
	for (int i = 0; i < player->card_count; ++i) {
		if (player->cards[i].suit == suit && player->cards[i].rank > highest) {
			highest = player->cards[i].rank;
		}
	}
	return highest;
}

static bool card_beats(Card challenger, Card current_winner, Suit lead_suit, Suit trump) {
	bool challenger_trump = challenger.suit == trump;
	bool winner_trump = current_winner.suit == trump;

	if (challenger_trump && !winner_trump) {
		return true;
	}
	if (!challenger_trump && winner_trump) {
		return false;
	}
	if (challenger.suit == current_winner.suit) {
		return challenger.rank > current_winner.rank;
	}
	if (challenger.suit == lead_suit && current_winner.suit != lead_suit) {
		return true;
	}
	return false;
}

static bool trick_has_cards(const GameState *game) {
	for (int i = 0; i < PLAYER_COUNT; ++i) {
		if (game->trick_filled[i]) {
			return true;
		}
	}
	return false;
}

static int current_trick_winner(const GameState *game) {
	int winner = -1;
	Card winning_card = { NO_CARD_RANK, SUIT_CLUBS };
	Suit lead_suit = SUIT_COUNT;

	for (int offset = 0; offset < PLAYER_COUNT; ++offset) {
		int player = (game->lead_player + offset) % PLAYER_COUNT;
		if (!game->trick_filled[player]) {
			continue;
		}
		if (winner < 0) {
			winner = player;
			winning_card = game->trick_cards[player];
			lead_suit = winning_card.suit;
			continue;
		}
		if (card_beats(game->trick_cards[player], winning_card, lead_suit, game->trump)) {
			winner = player;
			winning_card = game->trick_cards[player];
		}
	}

	return winner;
}

static int determine_trick_winner(const GameState *game) {
	return current_trick_winner(game);
}

static Suit trick_lead_suit(const GameState *game) {
	if (!trick_has_cards(game)) {
		return SUIT_COUNT;
	}
	return game->trick_cards[game->lead_player].suit;
}

static bool card_is_legal(const GameState *game, int player_index, int card_index) {
	const PlayerState *player = &game->players[player_index];
	Card selected;
	Suit lead_suit;
	int winner;
	Card winning_card;
	int highest_trump;

	if (card_index < 0 || card_index >= player->card_count) {
		return false;
	}

	selected = player->cards[card_index];
	lead_suit = trick_lead_suit(game);
	if (lead_suit == SUIT_COUNT) {
		if (!game->trump_broken && selected.suit == game->trump && player_has_non_trump(player, game->trump)) {
			return false;
		}
		return true;
	}

	if (selected.suit == lead_suit) {
		return true;
	}

	if (player_has_suit(player, lead_suit)) {
		return false;
	}

	if (!player_has_suit(player, game->trump)) {
		return true;
	}

	if (selected.suit != game->trump) {
		return false;
	}

	winner = current_trick_winner(game);
	if (winner < 0) {
		return true;
	}

	winning_card = game->trick_cards[winner];
	if (winning_card.suit != game->trump) {
		return true;
	}

	highest_trump = highest_rank_of_suit(player, game->trump);
	if (highest_trump > winning_card.rank) {
		return selected.rank == highest_trump || selected.rank > winning_card.rank;
	}

	return true;
}

static int choose_ai_bid(const GameState *game, int player_index) {
	const PlayerState *player = &game->players[player_index];
	int suit_counts[SUIT_COUNT] = { 0, 0, 0, 0 };
	int strength = 0;
	int target;

	for (int i = 0; i < player->card_count; ++i) {
		Card card = player->cards[i];
		suit_counts[card.suit]++;
		if (card.rank >= 14) {
			strength += 3;
		} else if (card.rank >= 13) {
			strength += 2;
		} else if (card.rank >= 11) {
			strength += 1;
		}
	}

	for (int suit = 0; suit < SUIT_COUNT; ++suit) {
		if (suit_counts[suit] >= 4) {
			strength += suit_counts[suit] - 2;
		}
	}

	target = strength / 3;
	if (target < 4) {
		return 0;
	}
	if (target > HAND_SIZE) {
		target = HAND_SIZE;
	}
	if (target <= game->highest_bid) {
		return 0;
	}
	return target;
}

static Suit choose_ai_trump(const PlayerState *player) {
	int best_score = -1;
	Suit best_suit = SUIT_SPADES;

	for (int suit = 0; suit < SUIT_COUNT; ++suit) {
		int score = 0;
		for (int i = 0; i < player->card_count; ++i) {
			Card card = player->cards[i];
			if (card.suit != suit) {
				continue;
			}
			score += 3;
			if (card.rank >= 14) {
				score += 4;
			} else if (card.rank >= 13) {
				score += 3;
			} else if (card.rank >= 11) {
				score += 2;
			}
		}
		if (score > best_score) {
			best_score = score;
			best_suit = (Suit)suit;
		}
	}

	return best_suit;
}

static int pick_ai_card(const GameState *game, int player_index) {
	const PlayerState *player = &game->players[player_index];
	int best_win = -1;
	int best_low = -1;
	int current_winner = current_trick_winner(game);
	Card winning_card = { NO_CARD_RANK, SUIT_CLUBS };
	Suit lead_suit = trick_lead_suit(game);
	bool wants_trick = player->tricks_won < player->bid;

	if (current_winner >= 0) {
		winning_card = game->trick_cards[current_winner];
	}

	for (int i = 0; i < player->card_count; ++i) {
		Card card = player->cards[i];
		if (!card_is_legal(game, player_index, i)) {
			continue;
		}

		if (best_low < 0 || card.rank < player->cards[best_low].rank) {
			best_low = i;
		}

		if (lead_suit == SUIT_COUNT || current_winner < 0 || card_beats(card, winning_card, lead_suit, game->trump)) {
			if (best_win < 0 || card.rank < player->cards[best_win].rank) {
				best_win = i;
			}
		}
	}

	if (!wants_trick && best_low >= 0) {
		return best_low;
	}
	if (wants_trick && best_win >= 0) {
		return best_win;
	}
	if (best_win >= 0) {
		return best_win;
	}
	return best_low;
}

static void finish_trick(GameState *game) {
	int winner = determine_trick_winner(game);

	game->players[winner].tricks_won++;
	game->last_trick_winner = winner;
	game->lead_player = winner;
	game->active_player = winner;
	game->trick_count++;
	game->trick_ready = true;
	set_status(game, "%s TOOK THE TRICK.", game->players[winner].name);

	if (game->trick_count >= HAND_SIZE) {
		game->phase = PHASE_SCORING;
		score_round(game);
	}
}

static void score_round(GameState *game) {
	int best_score = -999999;
	int best_player = 0;

	for (int i = 0; i < PLAYER_COUNT; ++i) {
		PlayerState *player = &game->players[i];
		int delta;

		if (i == game->highest_bidder) {
			if (player->tricks_won >= player->bid) {
				delta = player->bid * 10 + (player->tricks_won - player->bid);
			} else {
				delta = -player->bid * 10;
			}
		} else {
			delta = player->tricks_won;
		}

		player->score += delta;
		if (player->score > best_score) {
			best_score = player->score;
			best_player = i;
		}
	}

	game->round_winner = best_player;
	if (best_score >= TARGET_SCORE) {
		game->phase = PHASE_GAME_OVER;
		set_status(game, "%s WON THE MATCH. PRESS A FOR A NEW MATCH.", game->players[best_player].name);
		return;
	}

	set_status(game, "ROUND OVER. SCORES UPDATED, PRESS A FOR NEXT ROUND.");
}

void game_init(GameState *game) {
	memset(game, 0, sizeof(*game));
	srand(0xBA7A4);

	for (int i = 0; i < PLAYER_COUNT; ++i) {
		game->players[i].name = k_player_names[i];
	}

	game_reset_match(game);
}

void game_reset_match(GameState *game) {
	game->phase = PHASE_MENU;
	game->round_number = 1;
	game->highest_bidder = -1;
	game->last_trick_winner = -1;
	game->round_winner = -1;
	for (int i = 0; i < PLAYER_COUNT; ++i) {
		game->players[i].score = 0;
	}
	game_reset_round(game);
}

void game_reset_round(GameState *game) {
	Card deck[PLAYER_COUNT * HAND_SIZE];
	int deck_index = 0;

	for (int i = 0; i < PLAYER_COUNT; ++i) {
		PlayerState *player = &game->players[i];
		player->card_count = 0;
		player->selected = 0;
		player->bid = 0;
		player->tricks_won = 0;
		player->passed = false;
	}

	for (int suit = 0; suit < SUIT_COUNT; ++suit) {
		for (int rank = 2; rank <= 14; ++rank) {
			deck[deck_index].rank = rank;
			deck[deck_index].suit = (Suit)suit;
			deck_index++;
		}
	}

	shuffle_deck(deck, deck_index);

	deck_index = 0;
	for (int card = 0; card < HAND_SIZE; ++card) {
		for (int player = 0; player < PLAYER_COUNT; ++player) {
			game->players[player].cards[card] = deck[deck_index++];
			game->players[player].card_count++;
		}
	}

	for (int i = 0; i < PLAYER_COUNT; ++i) {
		sort_hand(&game->players[i]);
	}

	for (int i = 0; i < PLAYER_COUNT; ++i) {
		game->trick_filled[i] = false;
		game->trick_cards[i].rank = NO_CARD_RANK;
		game->trick_cards[i].suit = SUIT_CLUBS;
	}

	game->phase = PHASE_MENU;
	game->trump = SUIT_SPADES;
	game->active_player = (game->dealer + 1) % PLAYER_COUNT;
	game->lead_player = game->active_player;
	game->bids_locked = 0;
	game->highest_bid = 0;
	game->highest_bidder = -1;
	game->current_bid = 4;
	game->trick_count = 0;
	game->last_trick_winner = -1;
	game->trick_ready = false;
	game->trump_broken = false;
	game->ai_next_action_ms = 0;
	game->round_winner = -1;
	set_status(game, "PRESS A TO START BIDDING.");
}

void game_start_bidding(GameState *game) {
	game->phase = PHASE_BIDDING;
	game->active_player = (game->dealer + 1) % PLAYER_COUNT;
	game->current_bid = game->highest_bid > 0 ? game->highest_bid + 1 : 4;
	set_status(game, "%s IS BIDDING.", game->players[game->active_player].name);
}

void game_advance_round(GameState *game) {
	game->dealer = (game->dealer + 1) % PLAYER_COUNT;
	game->round_number++;
	game_reset_round(game);
}

void game_move_selection(GameState *game, int delta) {
	PlayerState *player = &game->players[HUMAN_PLAYER];

	if (!game_is_human_turn(game) || game->phase != PHASE_PLAYING || game->trick_ready || player->card_count <= 0) {
		return;
	}

	player->selected += delta;
	if (player->selected < 0) {
		player->selected = player->card_count - 1;
	}
	if (player->selected >= player->card_count) {
		player->selected = 0;
	}
}

bool game_select_card_from_pointer(GameState *game, int pointer_x, int pointer_y) {
	PlayerState *player = &game->players[HUMAN_PLAYER];
	int start_x = 206;
	int base_y = 404;

	if (!game_is_human_turn(game) || game->phase != PHASE_PLAYING || game->trick_ready || player->card_count <= 0) {
		return false;
	}

	for (int i = player->card_count - 1; i >= 0; --i) {
		int card_x = start_x + i * 18;
		int card_y = (i == player->selected) ? base_y - 14 : base_y;
		if (pointer_x >= card_x && pointer_x < card_x + 34 && pointer_y >= card_y && pointer_y < card_y + 48) {
			bool hit = true;
			player->selected = i;
			return hit;
		}
	}

	return false;
}

void game_adjust_bid(GameState *game, int delta) {
	if (game->phase != PHASE_BIDDING || game->active_player != HUMAN_PLAYER) {
		return;
	}

	game->current_bid += delta;
	if (game->current_bid < 0) {
		game->current_bid = 0;
	}
	if (game->current_bid > HAND_SIZE) {
		game->current_bid = HAND_SIZE;
	}
}

void game_pass_bid(GameState *game) {
	if (game->phase != PHASE_BIDDING || game->active_player != HUMAN_PLAYER) {
		return;
	}

	lock_bid(game, 0);
}

void game_change_trump(GameState *game, int delta) {
	if (game->phase != PHASE_TRUMP_SELECT || game->highest_bidder != HUMAN_PLAYER) {
		return;
	}

	game->trump = (Suit)((game->trump + delta + SUIT_COUNT) % SUIT_COUNT);
	set_status(game, "%s IS CHOOSING %s AS TRUMP.", game->players[game->highest_bidder].name, game_suit_label(game->trump));
}

static void lock_bid(GameState *game, int bid_value) {
	PlayerState *player = &game->players[game->active_player];

	if (bid_value > 0 && bid_value <= game->highest_bid) {
		set_status(game, "BID MUST BE ABOVE %d.", game->highest_bid);
		return;
	}

	player->bid = bid_value;
	player->passed = (bid_value == 0);
	if (player->bid > game->highest_bid) {
		game->highest_bid = player->bid;
		game->highest_bidder = game->active_player;
	}

	game->bids_locked++;
	if (game->bids_locked >= PLAYER_COUNT) {
		if (game->highest_bidder < 0) {
			game->highest_bidder = (game->dealer + 1) % PLAYER_COUNT;
			game->highest_bid = 4;
			game->players[game->highest_bidder].bid = 4;
		}
		game->phase = PHASE_TRUMP_SELECT;
		game->active_player = game->highest_bidder;
		game->trump = SUIT_SPADES;
		set_status(game, "%s IS CHOOSING TRUMP.", game->players[game->highest_bidder].name);
		return;
	}

	game->active_player = (game->active_player + 1) % PLAYER_COUNT;
	game->current_bid = game->highest_bid > 0 ? game->highest_bid + 1 : 4;
	set_status(game, "%s IS BIDDING.", game->players[game->active_player].name);
}

static void clear_trick(GameState *game) {
	for (int i = 0; i < PLAYER_COUNT; ++i) {
		game->trick_filled[i] = false;
		game->trick_cards[i].rank = NO_CARD_RANK;
		game->trick_cards[i].suit = SUIT_CLUBS;
	}
	game->trick_ready = false;
}

bool game_can_play_selected(const GameState *game) {
	if (!game_is_human_turn(game)) {
		return false;
	}
	return card_is_legal(game, HUMAN_PLAYER, game->players[HUMAN_PLAYER].selected);
}

static void play_selected_card(GameState *game, int player_index, int card_index) {
	PlayerState *player = &game->players[player_index];
	Card played;

	if (!card_is_legal(game, player_index, card_index)) {
		Card attempted = player->cards[card_index];
		Suit lead_suit = trick_lead_suit(game);
		if (lead_suit == SUIT_COUNT && !game->trump_broken && attempted.suit == game->trump && player_has_non_trump(player, game->trump)) {
			set_status(game, "YOU CANNOT LEAD TRUMP BEFORE IT IS BROKEN.");
		} else if (lead_suit != SUIT_COUNT && player_has_suit(player, lead_suit)) {
			set_status(game, "YOU MUST FOLLOW SUIT.");
		} else if (lead_suit != SUIT_COUNT && player_has_suit(player, game->trump) && attempted.suit != game->trump) {
			set_status(game, "IF YOU CANNOT FOLLOW, YOU MUST PLAY TRUMP.");
		} else {
			set_status(game, "YOU MUST PLAY A HIGHER TRUMP IF YOU HAVE ONE.");
		}
		return;
	}

	player->selected = card_index;
	played = player->cards[card_index];
	if (played.suit == game->trump) {
		game->trump_broken = true;
	}
	game->trick_cards[player_index] = played;
	game->trick_filled[player_index] = true;
	remove_card_at(player, card_index);

	for (int i = 1; i <= PLAYER_COUNT; ++i) {
		int next = (player_index + i) % PLAYER_COUNT;
		if (!game->trick_filled[next] && game->players[next].card_count > 0) {
			game->active_player = next;
			set_status(game, "%s TO PLAY.", game->players[next].name);
			return;
		}
	}

	finish_trick(game);
}

bool game_is_round_complete(const GameState *game) {
	return game->phase == PHASE_SCORING || game->phase == PHASE_GAME_OVER;
}

bool game_is_human_turn(const GameState *game) {
	switch (game->phase) {
	case PHASE_MENU:
	case PHASE_SCORING:
	case PHASE_GAME_OVER:
		return true;
	case PHASE_BIDDING:
		return game->active_player == HUMAN_PLAYER;
	case PHASE_TRUMP_SELECT:
		return game->highest_bidder == HUMAN_PLAYER;
	case PHASE_PLAYING:
		return game->active_player == HUMAN_PLAYER || game->trick_ready;
	default:
		return false;
	}
}

void game_confirm_action(GameState *game) {
	switch (game->phase) {
	case PHASE_MENU:
		game_start_bidding(game);
		break;
	case PHASE_BIDDING:
		if (game->active_player == HUMAN_PLAYER) {
			lock_bid(game, game->current_bid);
		}
		break;
	case PHASE_TRUMP_SELECT:
		if (game->highest_bidder == HUMAN_PLAYER) {
			game->phase = PHASE_PLAYING;
			game->active_player = game->highest_bidder;
			game->lead_player = game->active_player;
			set_status(game, "TRUMP IS %s. %s LEADS.", game_suit_label(game->trump), game->players[game->active_player].name);
		}
		break;
	case PHASE_PLAYING:
		if (game->trick_ready) {
			clear_trick(game);
			set_status(game, "%s TO PLAY.", game->players[game->active_player].name);
		} else if (game->active_player == HUMAN_PLAYER) {
			play_selected_card(game, HUMAN_PLAYER, game->players[HUMAN_PLAYER].selected);
		}
		break;
	case PHASE_SCORING:
		game_advance_round(game);
		break;
	case PHASE_GAME_OVER:
		game_reset_match(game);
		break;
	default:
		break;
	}
}

void game_tick(GameState *game, unsigned int now_ms) {
	if (now_ms < game->ai_next_action_ms) {
		return;
	}

	if (game->phase == PHASE_GAME_OVER) {
		return;
	}

	if (game->phase == PHASE_BIDDING && game->active_player != HUMAN_PLAYER) {
		int bid = choose_ai_bid(game, game->active_player);
		lock_bid(game, bid);
		game->ai_next_action_ms = now_ms + 450;
		return;
	}

	if (game->phase == PHASE_TRUMP_SELECT && game->highest_bidder != HUMAN_PLAYER) {
		game->trump = choose_ai_trump(&game->players[game->highest_bidder]);
		game->phase = PHASE_PLAYING;
		game->active_player = game->highest_bidder;
		game->lead_player = game->active_player;
		set_status(game, "TRUMP IS %s. %s LEADS.", game_suit_label(game->trump), game->players[game->active_player].name);
		game->ai_next_action_ms = now_ms + 600;
		return;
	}

	if (game->phase == PHASE_PLAYING && game->trick_ready) {
		clear_trick(game);
		set_status(game, "%s TO PLAY.", game->players[game->active_player].name);
		game->ai_next_action_ms = now_ms + 700;
		return;
	}

	if (game->phase == PHASE_PLAYING && game->active_player != HUMAN_PLAYER) {
		int card_index = pick_ai_card(game, game->active_player);
		if (card_index >= 0) {
			play_selected_card(game, game->active_player, card_index);
			game->ai_next_action_ms = now_ms + 700;
		}
	}
}

const char *game_phase_label(GamePhase phase) {
	switch (phase) {
	case PHASE_MENU: return "MENU";
	case PHASE_BIDDING: return "BID";
	case PHASE_TRUMP_SELECT: return "TRUMP";
	case PHASE_PLAYING: return "PLAY";
	case PHASE_SCORING: return "SCORE";
	case PHASE_GAME_OVER: return "END";
	default: return "UNKNOWN";
	}
}

const char *game_suit_label(Suit suit) {
	switch (suit) {
	case SUIT_CLUBS: return "CLUBS";
	case SUIT_DIAMONDS: return "DIAMONDS";
	case SUIT_HEARTS: return "HEARTS";
	case SUIT_SPADES: return "SPADES";
	default: return "?";
	}
}

void game_card_label(Card card, char *buffer, int buffer_size) {
	const char *rank = "?";
	const char *suit = "X";
	char rank_buf[4];

	switch (card.rank) {
	case 11: rank = "J"; break;
	case 12: rank = "Q"; break;
	case 13: rank = "K"; break;
	case 14: rank = "A"; break;
	default:
		snprintf(rank_buf, sizeof(rank_buf), "%d", card.rank);
		rank = rank_buf;
		break;
	}

	switch (card.suit) {
	case SUIT_CLUBS: suit = "C"; break;
	case SUIT_DIAMONDS: suit = "D"; break;
	case SUIT_HEARTS: suit = "H"; break;
	case SUIT_SPADES: suit = "S"; break;
	default: break;
	}

	snprintf(buffer, buffer_size, "%s%s", rank, suit);
}
