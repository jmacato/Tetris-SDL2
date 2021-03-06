/*
 * Playfield.cpp
 *
 *  Created on: 8 Aug 2015
 *      Author: eva
 */

#include "Playfield.h"

Playfield::Playfield(SDL_Renderer* renderer) :
		mGameState(GAME_STATE_PAUSED), mRenderer(renderer), screenWidth(0), screenHeight(
				0), mLevel(1), mCurrentPosX(0), mCurrentPosY(0), mFont(NULL), mHoldLock(
				false), mLockDelay(500), mLeft(false), mRight(false) {

	// Colour Array
	mColourArray[COLOUR_INDEX_CYAN] = {0x00, 0xBF, 0xFF};
	mColourArray[COLOUR_INDEX_YELLOW] = {0xFF, 0xD7, 0x00};
	mColourArray[COLOUR_INDEX_PURPLE] = {0x80, 0x00, 0x80};
	mColourArray[COLOUR_INDEX_GREEN] = {0x00, 0xFF, 0x00};
	mColourArray[COLOUR_INDEX_RED] = {0xFF, 0x00, 0x00};
	mColourArray[COLOUR_INDEX_BLUE] = {0x00, 0x00, 0xFF};
	mColourArray[COLOUR_INDEX_ORANGE] = {0xFF, 0x8C, 0x00};
	mColourArray[COLOUR_INDEX_WHITE] = {0xFF, 0xFF, 0xFF};
	mColourArray[COLOUR_INDEX_GREY] = {0x80, 0x80, 0x80};

	// Load Text font
	mFont = TTF_OpenFont("assets/FFFFORWA.TTF", PF_BLOCKSIZE);
	if (mFont == NULL) {
		printf("Failed to load font for playfield! TTF_Error: %s. \n",
				TTF_GetError());
	}
	mNext.setRenderer(mRenderer);
	mNext.loadFromRenderedText(mFont, "NEXT:", mColourArray[COLOUR_INDEX_WHITE]);
	mHold.setRenderer(mRenderer);
	mHold.loadFromRenderedText(mFont, "HOLD: ", mColourArray[COLOUR_INDEX_WHITE]);

	// Random Seed
	srand(time(NULL));
}

void Playfield::start() {
	// Empty PlayField
	for (int x = 0; x < PF_WIDTH; x++) {
		for (int y = 0; y < PF_HEIGHT; y++) {
			// Fill every element to be -1
			mPlayfield[x][y] = -1;
		}
	}
	// Reset variables
	mLevel = 1;
	mCurrentPosX = 0;
	mCurrentPosY = 0;
	mHoldLock = false;
	mLockDelay = 500;
	mLeft = false;
	mRight = false;
	mQueue.clear();
	mHoldTetromino.clear();
	mCurrentTetromino.clear();

	mGameState = GAME_STATE_INGAME;
	// Start Timer
	mGlobalTimer.start();
	mElapsedTimer.start();
	// Generate new Queues
	checkQueue();
	newTetromino();

}

void Playfield::setScreenSize(int width, int height) {
	screenWidth = width;
	screenHeight = height;
	// playfield position on screen
	mPlayArea.x = PF_BLOCKSIZE;
	mPlayArea.y = screenHeight - (PF_HEIGHT - 1) * PF_BLOCKSIZE;
	mPlayArea.w = PF_WIDTH * PF_BLOCKSIZE;
	mPlayArea.h = (PF_HEIGHT - 2) * PF_BLOCKSIZE;
}

void Playfield::tick() {

	int elapsed = mElapsedTimer.getTicks();

	if (mGameState == GAME_STATE_INGAME) {
		checkQueue();
		// Update y position
		double speed = 0.2 * mLevel / 1000.0; // grid per ms
		mCurrentPosY += (speed * elapsed);

		// Can't move down anymore
		if (!isLegal(mCurrentPosX, mCurrentPosY)) {
			// Reverse position & change to lowest possible position
			mCurrentPosY -= (speed * elapsed);
			mCurrentPosY = project();

			// lock delay
			if (mLockDelayTimer.isStarted()) {
				if (mLockDelayTimer.getTicks() >= mLockDelay) {
					lock();
				}
			} else {
				mLockDelayTimer.start();
			}
		}
	}

	mElapsedTimer.reset();
}

void Playfield::draw() {

	// Drawing the play Area
	SDL_SetRenderDrawColor(mRenderer, mColourArray[COLOUR_INDEX_GREY].r,
			mColourArray[COLOUR_INDEX_GREY].g,
			mColourArray[COLOUR_INDEX_GREY].b, 0xFF);
	SDL_RenderFillRect(mRenderer, &mPlayArea);
	SDL_SetRenderDrawColor(mRenderer, mColourArray[COLOUR_INDEX_WHITE].r,
			mColourArray[COLOUR_INDEX_WHITE].g,
			mColourArray[COLOUR_INDEX_WHITE].b, 0xFF);
	SDL_RenderDrawRect(mRenderer, &mPlayArea);

	// Drawing the Current Tetromino and its ghost pieces
	double roundedY = roundY(mCurrentPosY);
	// Centre of the Tetromino on display
	// -2 on y to correct for hidden rows
	SDL_Point currentCentreOnDisplay;
	currentCentreOnDisplay.x =
			(int) (mPlayArea.x + mCurrentPosX * PF_BLOCKSIZE);
	currentCentreOnDisplay.y = (int) (mPlayArea.y
			+ (roundedY - 2) * PF_BLOCKSIZE);

	SDL_Point ghostCenterOnDisplay;
	ghostCenterOnDisplay.x = currentCentreOnDisplay.x;
	ghostCenterOnDisplay.y = mPlayArea.y + (project() - 2) * PF_BLOCKSIZE;

	drawTetromino(ghostCenterOnDisplay, mCurrentTetromino, true);
	drawTetromino(currentCentreOnDisplay, mCurrentTetromino);

// Drawing elements of the board
	for (int row = 0; row < PF_WIDTH; row++) {
		// col init at 2, the first 2 rows are not visible
		for (int col = 2; col < PF_HEIGHT; col++) {
			// Block position
			SDL_Point blockTopLeft;
			blockTopLeft.x = mPlayArea.x + PF_BLOCKSIZE * row;
			blockTopLeft.y = mPlayArea.y + PF_BLOCKSIZE * (col - 2);

			// Get the colour of the block
			int element = mPlayfield[row][col];
			// if the element is valid
			if (element >= 0 && element < 7) {
				drawBlock(blockTopLeft, element);
			}
		}
	}
	if (mGameState != GAME_STATE_PAUSED) {

		// Drawing text
		mNext.draw(mPlayArea.x + mPlayArea.w + PF_BLOCKSIZE,
				mPlayArea.y + PF_BLOCKSIZE);
		// Draw upcoming tetromino

		SDL_Point currentNext;
		currentNext.x = mPlayArea.x + mPlayArea.w + PF_BLOCKSIZE;
		currentNext.y = mPlayArea.y + 3 * PF_BLOCKSIZE;
		for (int i = 1; i < 5; i++) {
			Tetromino upcoming;
			upcoming.generate(mQueue.at(mQueue.size() - i));
			int thickness = 1;

			if (upcoming.getTType() != TTYPE_I) {
				thickness++;
			}

			SDL_Point center = topLeftToCenter(currentNext,
					upcoming.getTType());
			drawTetromino(center, upcoming);
			currentNext.y += (thickness + 1) * PF_BLOCKSIZE;
		}

		// Draw hold text and tetromino
		SDL_Point holdTopLeft = { mPlayArea.x + mPlayArea.w + PF_BLOCKSIZE,
				mPlayArea.y + 16 * PF_BLOCKSIZE };
		mHold.draw(holdTopLeft.x, holdTopLeft.y);
		holdTopLeft.y += 2 * PF_BLOCKSIZE;	// Spacing
		SDL_Point holdCenter = topLeftToCenter(holdTopLeft,
				mHoldTetromino.getTType());
		drawTetromino(holdCenter, mHoldTetromino);
	}

	// Draw level info
	LTexture levelText;
	levelText.setRenderer(mRenderer);
	stringstream levelSS;
	levelSS << "Lvl: ";
	levelSS << mLevel;
	levelText.loadFromRenderedText(mFont, levelSS.str().c_str(),
			mColourArray[COLOUR_INDEX_WHITE]);
	levelText.draw(mPlayArea.x, mPlayArea.y - 2 * PF_BLOCKSIZE);

	// Draw time info
	LTexture gameTime;
	gameTime.setRenderer(mRenderer);
	stringstream timeSS;
	timeSS << "Time: ";
	timeSS << (int) (mGlobalTimer.getTicks() / 1000);
	gameTime.loadFromRenderedText(mFont, timeSS.str().c_str(),
			mColourArray[COLOUR_INDEX_WHITE]);
	gameTime.draw(mPlayArea.x + 6 * PF_BLOCKSIZE,
			mPlayArea.y - 2 * PF_BLOCKSIZE);
}

void Playfield::handleEvent(SDL_Event& event) {
	if (mGameState == GAME_STATE_INGAME) {

		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
			case SDLK_LEFT:
				if (isLegal(mCurrentPosX - 1, mCurrentPosY)) {
					mCurrentPosX--;
					mLockDelayTimer.reset();
				}
				mLeft = true;
				break;
			case SDLK_RIGHT:
				if (isLegal(mCurrentPosX + 1, mCurrentPosY)) {
					mCurrentPosX++;
					mLockDelayTimer.reset();
				}
				mRight = true;
				break;
			case SDLK_UP:
				mCurrentPosY = project();
				break;
			case SDLK_DOWN:
				if (isLegal(mCurrentPosX, mCurrentPosY + 1)) {
					mCurrentPosY++;
				}
				break;
			case SDLK_SPACE:
				mCurrentPosY = project();
				lock();
				break;
			case SDLK_z:
				mCurrentTetromino.antiClock();
				if (!isLegal(mCurrentPosX, mCurrentPosY)) {
					if (!kick()) {
						mCurrentTetromino.clock();
					} else {
						mLockDelayTimer.reset();
					}
				} else {
					mLockDelayTimer.reset();
				}
				break;
			case SDLK_x:
				mCurrentTetromino.clock();
				if (!isLegal(mCurrentPosX, mCurrentPosY)) {
					if (!kick()) {
						mCurrentTetromino.antiClock();
					} else {
						mLockDelayTimer.reset();
					}
				} else {
					mLockDelayTimer.reset();
				}
				break;
			case SDLK_c:
				hold();
				break;
			}
			// Need to check if the move is legal or out of bound
		} else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_LEFT:
				mLeft = false;
				break;
			case SDLK_RIGHT:
				mRight = false;
				break;
			}

		}
	}
}

vector<int> Playfield::randomiser() {
	// Generate nature array
	vector<int> random;
	for (int i = 0; i < 7; i++) {
		random.push_back(i);
	}
	// Shuffle and return
	random_shuffle(random.begin(), random.end());
	return random;
}

void Playfield::checkQueue() {
	while (mQueue.size() < 7) {
		vector<int> newQueue = randomiser();
		newQueue.insert(newQueue.end(), mQueue.begin(), mQueue.end());
		mQueue = newQueue;
	}
}

void Playfield::newTetromino(int tType) {
	// Generate new Piece
	if (tType == -1) {
		tType = mQueue.back();
		// Remove generate piece from the queue
		mQueue.pop_back();
	}
	mCurrentTetromino.generate(tType);

	// Update new positions
	switch (tType) {
	case TTYPE_I:
		mCurrentPosX = 5;
		mCurrentPosY = 2;
		break;
	case TTYPE_O:
		mCurrentPosX = 5;
		mCurrentPosY = 1;
		break;
	case TTYPE_T:
	case TTYPE_S:
	case TTYPE_Z:
	case TTYPE_J:
	case TTYPE_L:
		mCurrentPosX = 4.5;
		mCurrentPosY = 1.5;
		break;
	}

	// Check if game is over
	if (!isLegal(mCurrentPosX, mCurrentPosY)) {
		mGameState = GAME_STATE_LOST;
		mGlobalTimer.pause();
	}

}

SDL_Point Playfield::topLeftToCenter(SDL_Point& topLeft, int tType) {
// This only works for unrotaed pieces

	SDL_Point center;

// Get piece centre
	double dx, dy;
	dx = 2; // Horizontally centred
	switch (tType) {
	case TTYPE_I:
		dy = 1;
		break;
	case TTYPE_O:
		dy = 1;
		break;
	default:
		dy = 1.5;
		break;
	}

	center.x = topLeft.x + PF_BLOCKSIZE * dx;
	center.y = topLeft.y + PF_BLOCKSIZE * dy;

	return center;

}

void Playfield::drawBlock(SDL_Point& topLeft, int tType, bool ghost) {
	Uint8 transparency = 0xFF;

	if (ghost) {
		transparency = 0x80;
		SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);
	}

	// Forming block
	SDL_Rect block;

	block.w = PF_BLOCKSIZE;
	block.h = PF_BLOCKSIZE;
	block.x = topLeft.x;
	block.y = topLeft.y;

	// Drawing block
	SDL_Color colour = mColourArray[tType];
	SDL_SetRenderDrawColor(mRenderer, colour.r, colour.g, colour.b,
			transparency);
	SDL_RenderFillRect(mRenderer, &block);

	// Polish effect
	if (!ghost) {
		Uint8 white = 0xFF;
		int shineSize = PF_BLOCKSIZE / 7;
		SDL_Rect shine1, shine2, shine3, shine4;
		//shine1 = {block.x, block.y, shineSize,shineSize};
		shine2 = {block.x + shineSize, block.y + shineSize,shineSize, shineSize};
		shine3 = {block.x + 2*shineSize, block.y + shineSize, shineSize, shineSize};
		shine4 = {block.x + shineSize, block.y + 2*shineSize,shineSize, shineSize};
		SDL_SetRenderDrawColor(mRenderer, white, white, white, white);
		//SDL_RenderFillRect(mRenderer, &shine1);
		SDL_RenderFillRect(mRenderer, &shine2);
		SDL_RenderFillRect(mRenderer, &shine3);
		SDL_RenderFillRect(mRenderer, &shine4);

		SDL_Rect shadow1, shadow2;
		shadow1 = {block.x, block.y + block.h - shineSize, block.w, shineSize};
		shadow2 = {block.x + block.w - shineSize, block.y, shineSize, block.h};
		SDL_SetRenderDrawColor(mRenderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderFillRect(mRenderer, &shadow1);
		SDL_RenderFillRect(mRenderer, &shadow2);
	}

	// Line is too thick with shadows
//	// Drawing outline
//	if (!ghost) {
//		SDL_Color outline = { 0x00, 0x00, 0x00 };
//		SDL_SetRenderDrawColor(mRenderer, outline.r, outline.g, outline.b,
//				transparency);
//		SDL_RenderDrawRect(mRenderer, &block);
//	}
}

void Playfield::drawTetromino(SDL_Point& center, Tetromino& tetromino,
		bool ghost) {

	vector<TetrominoCRS> blockPos = tetromino.getBlockPos();
	for (vector<TetrominoCRS>::iterator it = blockPos.begin();
			it != blockPos.end(); it++) {

		SDL_Point blockTopLeft;
		// - 0.5 correction to caluclate the topleft conor
		blockTopLeft.x = center.x + (it->x - 0.5) * PF_BLOCKSIZE;
		blockTopLeft.y = center.y + (it->y - 0.5) * PF_BLOCKSIZE;
		if (blockTopLeft.y >= screenHeight - 21 * PF_BLOCKSIZE) {
			drawBlock(blockTopLeft, tetromino.getTType(), ghost);
		}
	}
}

bool Playfield::isLegal(double x, double y, int rotation) {

	bool legal = true;
	// Ronud y
	y = roundY(y);

	// Check every single block of current Tetromino

	Tetromino testTetromino = mCurrentTetromino;
	for (int i = 0; i < rotation; i++) {
		testTetromino.clock();
	}
	vector<TetrominoCRS> blockPosArray = testTetromino.getBlockPos();

	for (vector<TetrominoCRS>::iterator it = blockPosArray.begin();
			it != blockPosArray.end(); it++) {

		// Get top left point
		SDL_Point topLeft = { (int) (x + (it->x - 0.5)), (int) (y
				+ (it->y - 0.5)) };

		// Check for bound
		if (topLeft.x < 0 || topLeft.x >= PF_WIDTH) {
			legal = false;
		}
		if (topLeft.y < 0 || topLeft.y >= PF_HEIGHT) {
			legal = false;
		}
		// Check for collision
		if (mPlayfield[topLeft.x][topLeft.y] != -1) {
			legal = false;
		}
	}

	return legal;
}

void Playfield::lock() {

	mHoldLock = false;	// Reset holdlock
	// Ronud y
	double y = roundY(mCurrentPosY);

	// Check every single block of current Tetromino
	vector<TetrominoCRS> blockPosArray = mCurrentTetromino.getBlockPos();
	for (vector<TetrominoCRS>::iterator it = blockPosArray.begin();
			it != blockPosArray.end(); it++) {
		// Get top left point
		int xIndex = (int) (mCurrentPosX + (it->x - 0.5));
		int yIndex = (int) (y + (it->y - 0.5));

		mPlayfield[xIndex][yIndex] = mCurrentTetromino.getTType();
	}

	// Check for complete here and generate new
	lineCheck();	// Check for complete lines
	newTetromino();		// New pieces
	mLockDelayTimer.stop();	// stop lock delay timer
}

void Playfield::hold() {
	if (!mHoldLock) {
		if (mHoldTetromino.getTType() == -1) {
			mHoldTetromino.generate(mCurrentTetromino.getTType());
			newTetromino();
		} else {
			int tempType = mCurrentTetromino.getTType();
			newTetromino(mHoldTetromino.getTType());
			mHoldTetromino.generate(tempType);
		}
	}
	mHoldLock = true;
}

bool Playfield::kick() {
	bool kicked = false;
	int kickRange = 0;
	switch (mCurrentTetromino.getTType()) {
	case TTYPE_I:
		kickRange = 2;
		break;
	case TTYPE_O:
		break;
	default:
		kickRange = 1;
		break;
	}
	// Floor Kick
	if (mCurrentPosY >= project()) {
		//(mCurrentTetromino.getRotation() == 1 || mCurrentTetromino.getRotation() == 3) secondary condition
		for (int i = 1; i <= kickRange + 1; i++) {
			if (isLegal(mCurrentPosX, mCurrentPosY - i)) {
				mCurrentPosY -= i;
				kicked = true;
				break;
			}
		}
		// Wall kick
	} else if (mCurrentPosX <= 1 || mCurrentPosX >= 9) {
		// (mCurrentTetromino.getRotation() == 0 || mCurrentTetromino.getRotation() == 2)
		// Left Wall
		if (mCurrentPosX <= 1) {
			for (int i = 1; i <= kickRange; i++) {
				if (isLegal(mCurrentPosX + i, mCurrentPosY)) {
					mCurrentPosX += i;
					kicked = true;
					break;
				}
			}
		}
		// Right wall
		if (mCurrentPosX >= 9) {
			for (int i = 1; i <= kickRange; i++) {
				if (isLegal(mCurrentPosX - i, mCurrentPosY)) {
					mCurrentPosX -= i;
					kicked = true;
					break;
				}
			}
		}
	}

	return kicked;
}

void Playfield::lineCheck() {

	vector<int> completedLine;

	// Check every single row
	for (int row = 0; row < PF_HEIGHT; row++) {
		int elementInRow = 0;
		for (int index = 0; index < PF_WIDTH; index++) {
			if (mPlayfield[index][row] != -1) {
				elementInRow++;
			}
		}

		// if the row is complete
		if (elementInRow == PF_WIDTH) {
			// Record completed rows
			completedLine.push_back(row);

			// empty the row
			for (int i = 0; i < PF_WIDTH; i++) {
				mPlayfield[i][row] = -1;
			}
		}
	}
	// Collide when lines are cleared
	if (completedLine.size() != 0) {
		sort(completedLine.begin(), completedLine.end());// From the top of the playfield
		for (vector<int>::iterator it = completedLine.begin();
				it != completedLine.end(); it++) {

			for (int row = *it - 1; row >= 0; row--) {
				for (int index = 0; index < PF_WIDTH; index++) {
					mPlayfield[index][row + 1] = mPlayfield[index][row];// move the element down by one
					mPlayfield[index][row] = -1;	// Clear original element
				}
			}
		}

		// Change to level
		switch (completedLine.size()) {
		case 1:
			mLevel += 1;
			break;
		case 2:
			mLevel += 2;
			break;
		case 3:
			mLevel += 4;
			break;
		case 4:
			mLevel += 6;
			break;
		}

		// Change to lock delay
		if (mLevel > 500 && mLevel < 750) {
			mLockDelay = 1000 - mLevel;
		}

		if (mLevel > 1000) {
			mGameState = GAME_STATE_WON;
			mGlobalTimer.pause();
		}

	}

}

double Playfield::project() {
	double y = roundY(mCurrentPosY);
	while (isLegal(mCurrentPosX, y + 1)) {
		y++;
	}
	return y;
}

double Playfield::roundY(double y) {
	// Ronud y
	if (mCurrentTetromino.getTType() < 2) {
		// Round to integer
		y = round(y);
	} else {
		// Ronud to half integer
		y = round(y - 0.5) + 0.5;
	}
	return y;
}

int Playfield::getGameState() {
	return mGameState;
}

SDL_Rect Playfield::getPlayArea() {
	return mPlayArea;
}

// AI communications
PlayfieldNode Playfield::getNode() {
	PlayfieldNode node;
	// Generate simplified playfield
	for (int x = 0; x < PF_WIDTH; x++) {
		for (int y = 0; y < PF_HEIGHT; y++) {
			if (mPlayfield[x][y] < 0) {
				node.mSimplePlayfield[x][y] = false;
			} else {
				node.mSimplePlayfield[x][y] = true;
			}
		}
	}

	return node;
}

vector<int> Playfield::getQueue() {
	vector<int> queue;
	queue.push_back(mCurrentTetromino.getTType());
	for (int i = 0; i < 4; i++) {
		queue.push_back(mQueue.at(mQueue.size() - 1 - i));
	}
	return queue;
}

