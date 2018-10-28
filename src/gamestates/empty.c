/*! \file empty.c
 *  \brief Empty gamestate.
 */
/*
 * Copyright (c) Sebastian Krzyszkowiak <dos@dosowisko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common.h"
#include <allegro5/allegro_color.h>
#include <libsuperderpy.h>

static int NUM = 100;

struct GamestateResources {
	// This struct is for every resource allocated and used by your gamestate.
	// It gets created on load and then gets passed around to all other function calls.
	struct Character* character;
	ALLEGRO_FONT* font;
	ALLEGRO_AUDIO_STREAM *music, *sz;
	struct {
		float a, b;
		float pos;
		float angle;
		bool reverse;
		bool an;
		int dx, dy, dcolor;
		float speed;
		ALLEGRO_COLOR color;
	} states[100];
	int counter;
	bool click;
	ALLEGRO_BITMAP *spray1, *spray2;
};

int Gamestate_ProgressCount = 1; // number of loading steps as reported by Gamestate_Load; 0 when missing

void Gamestate_Logic(struct Game* game, struct GamestateResources* data, double delta) {
	// Here you should do all your game logic as if <delta> seconds have passed.
	AnimateCharacter(game, data->character, delta, 1.0);
	for (int i = 0; i < NUM; i++) {
		if (data->states[i].reverse) {
			data->states[i].pos -= delta / 4.0 * data->states[i].speed;
		} else {
			data->states[i].pos += delta / 4.0 * data->states[i].speed;
		}
		data->states[i].angle += (data->states[i].an ? 1 : -1) * (rand() / (double)RAND_MAX) * delta * data->states[i].speed;
	}
}

void Gamestate_Tick(struct Game* game, struct GamestateResources* data) {
	data->counter++;
	if (data->counter == 60) {
		data->counter = 0;

		for (int i = 0; i < NUM; i++) {
			data->states[i].a = rand() / (double)RAND_MAX * 2 - 1.0;
			data->states[i].b = rand() / (double)RAND_MAX / 5.0;
			data->states[i].dcolor = rand();
			data->states[i].angle = rand() / (double)RAND_MAX * ALLEGRO_PI;
			data->states[i].dx = (rand() / (double)RAND_MAX) * 500 - 250;
			data->states[i].dy = (rand() / (double)RAND_MAX) * 500 - 250;
			data->states[i].reverse = rand() % 2;
			data->states[i].an = rand() % 2;
			data->states[i].pos = rand() / (double)RAND_MAX - 0.5;

			data->states[i].speed = 0.5 + rand() / (double)RAND_MAX;
		}
	}
}

void Gamestate_Draw(struct Game* game, struct GamestateResources* data) {
	// Draw everything to the screen here.
	al_hold_bitmap_drawing(true);
	SetCharacterPositionF(game, data->character, 0.5, 0.5, 0);
	data->character->tint = al_map_rgb(255, 255, 255);
	data->character->scaleX = 1.0;
	data->character->scaleY = 1.0;
	DrawCharacter(game, data->character);

	data->character->scaleX = 0.25;
	data->character->scaleY = 0.25;

	for (int i = 0; i < NUM; i++) {
		data->character->tint = al_color_hsl(data->states[i].pos * 4 * 360, 1.0, 0.75);
		SetCharacterPositionF(game, data->character, data->states[i].pos + 0.5 + data->states[i].dx / 1280.0, data->states[i].a * data->states[i].pos + data->states[i].b + 0.5 + data->states[i].dy / 720.0, data->states[i].angle);
		DrawCharacter(game, data->character);
	}
	al_hold_bitmap_drawing(false);

	al_draw_bitmap(data->click ? data->spray2 : data->spray1, game->data->mouseX * 1280, game->data->mouseY * 720, 0);

	if (NUM == 0) {
		DrawTextWithShadow(data->font, al_map_rgb(255, 255, 255), 1280 * 0.5, 720 * 0.5 - 100, ALLEGRO_ALIGN_CENTRE, "YOU PARTIED FOR 0 HOURS");
		DrawTextWithShadow(data->font, al_map_rgb(255, 255, 255), 1280 * 0.5, 720 * 0.5 + 100, ALLEGRO_ALIGN_LEFT, "yOU WIN");
	}
}

void Gamestate_ProcessEvent(struct Game* game, struct GamestateResources* data, ALLEGRO_EVENT* ev) {
	// Called for each event in Allegro event queue.
	// Here you can handle user input, expiring timers etc.
	if ((ev->type == ALLEGRO_EVENT_KEY_DOWN) && (ev->keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
		UnloadCurrentGamestate(game); // mark this gamestate to be stopped and unloaded
		// When there are no active gamestates, the engine will quit.
	}

	if (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
		data->click = true;
		al_set_audio_stream_playing(data->sz, true);
		NUM--;
		if (NUM < 0) {
			NUM = 0;
		}
	}
	if (ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
		data->click = false;
		al_set_audio_stream_playing(data->sz, false);
	}
}

void* Gamestate_Load(struct Game* game, void (*progress)(struct Game*)) {
	// Called once, when the gamestate library is being loaded.
	// Good place for allocating memory, loading bitmaps etc.
	//
	// NOTE: There's no OpenGL context available here. If you want to prerender something,
	// create VBOs, etc. do it in Gamestate_PostLoad.

	struct GamestateResources* data = calloc(1, sizeof(struct GamestateResources));

	data->character = CreateCharacter(game, "dos");
	RegisterSpritesheet(game, data->character, "dos");
	LoadSpritesheets(game, data->character, progress);

	data->font = al_load_font(GetDataFilePath(game, "fonts/PerfectDOSVGA437.ttf"), 64, 0);

	data->music = al_load_audio_stream(GetDataFilePath(game, "audiodump.ogg"), 4, 2048);
	al_attach_audio_stream_to_mixer(data->music, game->audio.music);

	data->sz = al_load_audio_stream(GetDataFilePath(game, "spray.flac"), 4, 2048);
	al_attach_audio_stream_to_mixer(data->sz, game->audio.fx);
	al_set_audio_stream_playmode(data->sz, ALLEGRO_PLAYMODE_LOOP);
	al_set_audio_stream_playing(data->sz, false);
	al_set_audio_stream_gain(data->sz, 2.0);

	data->spray1 = al_load_bitmap(GetDataFilePath(game, "spray.png"));
	data->spray2 = al_load_bitmap(GetDataFilePath(game, "spray2.png"));

	return data;
}

void Gamestate_Unload(struct Game* game, struct GamestateResources* data) {
	// Called when the gamestate library is being unloaded.
	// Good place for freeing all allocated memory and resources.
	al_destroy_audio_stream(data->music);
	al_destroy_audio_stream(data->sz);
	free(data);
}

void Gamestate_Start(struct Game* game, struct GamestateResources* data) {
	// Called when this gamestate gets control. Good place for initializing state,
	// playing music etc.
	SetCharacterPosition(game, data->character, 0, 0, 0);

	for (int i = 0; i < NUM; i++) {
		data->states[i].a = rand() / (double)RAND_MAX;
		data->states[i].b = rand() / (double)RAND_MAX / 5.0;
		data->states[i].dcolor = rand();
		data->states[i].angle = rand() / (double)RAND_MAX * ALLEGRO_PI;
		data->states[i].dx = (rand() / (double)RAND_MAX) * 500 - 250;
		data->states[i].dy = (rand() / (double)RAND_MAX) * 500 - 250;
		data->states[i].reverse = rand() % 2;
		data->states[i].an = rand() % 2;
		data->states[i].pos = rand() / (double)RAND_MAX - 0.5;
	}
	data->character->scaleX = 0.25;
	data->character->scaleY = 0.25;
}

void Gamestate_Stop(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets stopped. Stop timers, music etc. here.
}

// Optional endpoints:

void Gamestate_PostLoad(struct Game* game, struct GamestateResources* data) {
	// This is called in the main thread after Gamestate_Load has ended.
	// Use it to prerender bitmaps, create VBOs, etc.
}

void Gamestate_Pause(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets paused (so only Draw is being called, no Logic nor ProcessEvent)
	// Pause your timers and/or sounds here.
}

void Gamestate_Resume(struct Game* game, struct GamestateResources* data) {
	// Called when gamestate gets resumed. Resume your timers and/or sounds here.
}

void Gamestate_Reload(struct Game* game, struct GamestateResources* data) {
	// Called when the display gets lost and not preserved bitmaps need to be recreated.
	// Unless you want to support mobile platforms, you should be able to ignore it.
}
