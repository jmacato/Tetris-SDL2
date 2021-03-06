/*
 * Graphics.cpp
 *
 *  Created on: 26 Jul 2015
 *      Author: eva
 */

#include "Graphics.h"

Graphics::Graphics() :
		mWindow(NULL), mRenderer(NULL), mFont(NULL) {
}

bool Graphics::init() {
	bool success = true;

	// SDL System Initialisation
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		success = false;
		printf("SDL unable to initialise! SDL Error: %s. \n", SDL_GetError());
	}

	// SDL true front
	if (TTF_Init() == -1) {
		success = false;
		printf("SDL_ttf unable to initialise! SDL_ttf Error: %s. \n",
		TTF_GetError());
	}

	// Create and Present a Window
	if (success) {
		mWindow = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (mWindow == NULL) {
			success = false;
			printf("SDL unable to create window! SDL Error: %s. \n",
					SDL_GetError());
		}
	}

	// Create corresponding Renderer from window
	if (success) {
		mRenderer = SDL_CreateRenderer(mWindow, -1,
				SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (mRenderer == NULL) {
			success = false;
			printf("SDL unable to create renderer! SDL Error: %s. \n",
					SDL_GetError());
		}
	}

	mFrameTimer.start();

	// Hide Cursor
	SDL_ShowCursor(0);
	// Trap cursor
	//SDL_SetRelativeMouseMode(SDL_TRUE);

	return success;
}

void Graphics::clear() {
	// Clear the renderer from previous frames
	SDL_SetRenderDrawColor(mRenderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(mRenderer);
}

void Graphics::render() {
	// Present the renderer
	SDL_RenderPresent(mRenderer);
	// Frame Cap for system out VSync
	if (mFrameTimer.getTicks() < TICKS_PER_FRAME_LIMIT) {
		SDL_Delay(TICKS_PER_FRAME_LIMIT - mFrameTimer.getTicks());
	}
	mFrameTimer.start();
}

SDL_Renderer* Graphics::getRenderer() {
	return mRenderer;
}

// Screen size getters
int Graphics::getScreenWidth() {
	return SCREEN_WIDTH;
}

int Graphics::getScreenHeight() {
	return SCREEN_HEIGHT;
}

Graphics::~Graphics() {

	SDL_DestroyRenderer(mRenderer);
	mRenderer = NULL;
	SDL_DestroyWindow(mWindow);
	mWindow = NULL;

	// Quit sub Systems
	TTF_Quit();
	SDL_Quit();
}

