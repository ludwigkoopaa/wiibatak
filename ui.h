#ifndef WIIBATAK_UI_H
#define WIIBATAK_UI_H

#include <SDL2/SDL.h>

#include "game.h"

void ui_render(SDL_Renderer *renderer, const GameState *game, bool paused, bool show_help);

#endif
