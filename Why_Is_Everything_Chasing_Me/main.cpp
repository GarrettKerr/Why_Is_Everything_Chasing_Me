#define _CRT_SECURE_NO_DEPRECATE
#include <SDL.h>
#include <glew.h>
#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include <vector>
#include <cmath>
#include <math.h>
#include <fmod.hpp>
#include <fmod.h>
#include <fmod_errors.h>
#undef main

char shouldExit = 0;
float _speed = 230.0f;
//float _camX = 1300.0f;
//float _camY = 30.0f;
float _camX = 213.0f;
float _camY = 213.0f;
float _playerX = 1500.0f;
float _playerY = 90.0f;
float healthX = 0.0f;
float healthY = 0.0f;
int _counter = 0;
float _cartX = 0;
float _cartY = 0;
float _isoX = 0;
float _isoY = 0;
float _newX = 0;
float _newY = 0;
float _newCamX = 0;
float _newCamY = 0;
float minionSpeed = 150.0f;
bool winCondition = false;
int gameState = 0; //0-Main Menu, 1-Gameplay, 2-Victory, 3-Loss
unsigned char kbPrevState[SDL_NUM_SCANCODES] = { 0 };
const unsigned char* kbState = NULL;



struct AnimFrameDef {
	// combined with the AnimDef's name to make
	// the actual texture name
	int frameNum;
	float frameTime;
};

struct AnimDef
{
	const char* name;
	AnimFrameDef frames[20];
	int numFrames;
};

// Runtime state for an animation
struct AnimData {
	AnimDef* def;
	int curFrame;
	float timeToNextFrame;
	bool isPlaying;
};

typedef struct Player
{
	AnimData anim;
	float _posX = 1253.0f;
	float _posY = 596.0f;
	int health = 5;
	int movementState = 0;
	int x = 1280.0f;
	int y = -28.0f;
	int w = 60;
	int h = 60;

}Player;

typedef struct Camera
{
	float _camX = 962.0f;
	float _camY = 404.0f;

}Camera;

void animSet(AnimData* anim, AnimDef* toPlay)
{
	anim->def = toPlay;
	anim->curFrame = 0;
	anim->timeToNextFrame
		= toPlay->frames[0].frameTime;
	anim->isPlaying = true;
}

void animReset(AnimData* anim)
{
	animSet(anim, anim->def);
}

void animTick(AnimData* data, float dt)
{
	if (!data->isPlaying)
	{
		return;
	}
	int numFrames = data->def->numFrames;
	data->timeToNextFrame -= dt;
	if (data->timeToNextFrame < 0)
	{
		++data->curFrame;
		if (data->curFrame >= numFrames)
		{
			// end of the animation, stop it
			data->curFrame = numFrames - 1;
			data->timeToNextFrame = 0;
			data->isPlaying = false;
		}
		else {
			AnimFrameDef* curFrame
				= &data->def->frames[data->curFrame];
			data->timeToNextFrame
				+= curFrame->frameTime;
		}
	}
}


typedef struct Coin
{
	AnimData anim;
	float _posX;
	float _posY;
	bool isDrawn;

}Coin;

struct AABB {
	int x, y, w, h;
};

bool AABBIntersect(int x, int y, int w, int h, int x2, int y2, int w2, int h2)
{
	// box1 to the right
	if (x > x2 + w2) {
		return false;
	}
	// box1 to the left
	if (x + w < x2) {
		return false;
	}
	// box1 below
	if (y > y2 + h2) {
		return false;
	}
	// box1 above
	if (y + h < y2) {
		return false;
	}
	return true;
}

void playerUpdate(Player* p, float dt)
{
	float tempX = p->_posX;
	float tempY = p->_posY;
	//add movement
	tempX += _newX * dt;
	tempY += _newY * dt;
	if (tempX < -10.0f)
	{
		tempX = -10.0f;
	}
	if (tempX > 2505.0f)
	{
		tempX = 2505.0f;
	}
	if (tempY < -40.0f)
	{
		tempY = -40.0f;
	}
	if (tempY > 1213.0f)
	{
		tempY = 1213.0f;
	}
	if (_newX == 0 && _newY == 0 && p->movementState == 1)
	{
		p->movementState = 0; //idle left
	}
	else if (_newX == 0 && _newY == 0 && p->movementState == 2)
	{
		p->movementState = 3; //idle right
	}
	else if (_newX < 0)
	{
		p->movementState = 1; //animate walk left
	}
	else if (_newX > 0)
	{
		p->movementState = 2; //animate walk right
	}
	//set new position
	p->_posX = tempX;
	p->_posY = tempY;
	//reset the new movement value
	_newX = 0;
	_newY = 0;
}

void cameraUpdate(Camera* c, float dt)
{
	float tempX = c->_camX;
	float tempY = c->_camY;
	//add movement
	tempX += _newCamX * dt;
	tempY += _newCamY * dt;
	if (tempX < -3300.0f)
	{
		tempX = -3300.0f;
	}
	if (tempX > 2220.0f)
	{
		tempX = 2220.0f;
	}
	if (tempY < -250.0f)
	{
		tempY = -250.0f;
	}
	if (tempY > 1030.0f)
	{
		tempY = 1030.0f;
	}
	//set new position
	c->_camX = tempX;
	c->_camY = tempY;
	//reset the new movement value
	_newCamX = 0;
	_newCamY = 0;
}

typedef struct movePoint
{
	float x;
	float y;
};


/* Load a file into an OpenGL texture, and return that texture. */

GLuint glTexImageTGAFile(const char* filename, int* outWidth, int* outHeight){
	const int BPP = 4;

	/* open the file */
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "File: %s -- Could not open for reading.\n", filename);
		return 0;
	}

	/* skip first two bytes of data we don't need */
	fseek(file, 2, SEEK_CUR);

	/* read in the image type.  For our purposes the image type should
	* be either a 2 or a 3. */
	unsigned char imageTypeCode;
	fread(&imageTypeCode, 1, 1, file);
	if (imageTypeCode != 2 && imageTypeCode != 3) {
		fclose(file);
		fprintf(stderr, "File: %s -- Unsupported TGA type: %d\n", filename, imageTypeCode);
		return 0;
	}

	/* skip 9 bytes of data we don't need */
	fseek(file, 9, SEEK_CUR);

	/* read image dimensions */
	int imageWidth = 0;
	int imageHeight = 0;
	int bitCount = 0;
	fread(&imageWidth, sizeof(short), 1, file);
	fread(&imageHeight, sizeof(short), 1, file);
	fread(&bitCount, sizeof(unsigned char), 1, file);
	fseek(file, 1, SEEK_CUR);

	/* allocate memory for image data and read it in */
	unsigned char* bytes = (unsigned char*)calloc(imageWidth * imageHeight * BPP, 1);

	/* read in data */
	if (bitCount == 32) {
		int it;
		for (it = 0; it != imageWidth * imageHeight; ++it) {
			bytes[it * BPP + 0] = fgetc(file);
			bytes[it * BPP + 1] = fgetc(file);
			bytes[it * BPP + 2] = fgetc(file);
			bytes[it * BPP + 3] = fgetc(file);
		}
	}
	else {
		int it;
		for (it = 0; it != imageWidth * imageHeight; ++it) {
			bytes[it * BPP + 0] = fgetc(file);
			bytes[it * BPP + 1] = fgetc(file);
			bytes[it * BPP + 2] = fgetc(file);
			bytes[it * BPP + 3] = 255;
		}
	}

	fclose(file);

	/* load into OpenGL */
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0,
		GL_BGRA, GL_UNSIGNED_BYTE, bytes);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	free(bytes);

	if (outWidth) {
		*outWidth = imageWidth;
	}
	if (outHeight) {
		*outHeight = imageHeight;
	}
	return tex;
}



/* Draw the sprite */
void glDrawSprite(GLuint tex, int x, int y, int w, int h){
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	{
		glColor3ub(255, 255, 255);
		glTexCoord2f(0, 1);
		glVertex2i(x, y);
		glTexCoord2f(1, 1);
		glVertex2i(x + w, y);
		glTexCoord2f(1, 0);
		glVertex2i(x + w, y + h);
		glTexCoord2f(0, 0);
		glVertex2i(x, y + h);
	}
	glEnd();
}


GLuint coinTextures[20];
GLuint playerTextures[20];
GLuint playerTextures2[20];
GLuint tiles[2];
GLuint health[7];
GLuint play[4];
GLuint numbers[10];
Camera camera;
movePoint mp1
{
	1253.0f,
	30.0f
};
movePoint mp2
{
	1253.0f,
	1175.0f
};

void writeHealth(Player* p)
{
	for (int i = 0; i < 6; i++)
	{
		glDrawSprite(health[i], healthX + (i*16), healthY, 16, 26);
	}
	glDrawSprite(health[6], healthX + (6 * 16), healthY, 10, 26);
	glDrawSprite(numbers[p->health], healthX + (7 * 17), healthY, 16, 26);
}

typedef struct Boss
{
	float posX;
	float posY;
	int health = 10;
	bool needGuard = true;
	float spawnTimer = 2.0f;
}Boss;

typedef struct Minion
{
	float posX;
	float posY;
	bool needANewDecision = false;
	int decisionType = 0;
	float decisionTimer = 5.0f;
	Boss guardTarget;
}Minion;

typedef struct Projectile
{
	float posX;
	float posY;
	float speed = 400.0f;
	float velocityX;
	float velocityY;

}Projectile;

bool withinReasonableDistance(float x1, float y1, float x2, float y2)
{
	if ((std::abs(x2 - x1) + std::abs(y2 - y1)) < 90.0f)
	{
		return true;
	}
	return false;
}

int getDistance(int x1, int y1, int x2, int y2)
{
	return (std::abs(x2 - x1) + std::abs(y2 - y1));
}

std::vector<Boss> drawnBosses;
std::vector<Boss> deadBosses;
std::vector<Minion> drawnMinions;
std::vector<Minion> deadMinions;
std::vector<Projectile> projectiles;
std::vector<Projectile> thrownProjectiles;

void updateProjectile(Projectile* p, float dt)
{
	p->posX += p->velocityX * dt;
	p->posY += p->velocityY * dt;
}

void MinionUdpdate(Minion* min, float dt, std::vector<Boss>* bosses, Player* p)
{
	float deltaX = 0.0f;
	float deltaY = 0.0f;
	if (min->decisionType == 1)
	{
		if (withinReasonableDistance(min->posX, min->posY, min->guardTarget.posX, min->guardTarget.posY) == false)
		{
			if (min->posX < min->guardTarget.posX)
			{
				deltaX = minionSpeed;
			}
			else
			{
				deltaX = -minionSpeed;
			}
			if (min->posY < min->guardTarget.posY)
			{
				deltaY = minionSpeed;
			}
			else
			{
				deltaY = -minionSpeed;
			}
		}
	}
	else if (min->decisionType == 0 || min->decisionType == 2 || min->decisionType == 3)
	{
		if (min->posX < p->_posX)
		{
			deltaX = minionSpeed;
		}
		else
		{
			deltaX = -minionSpeed;
		}
		if (min->decisionType == 0)
		{
			if (min->posY < p->_posY)
			{
				deltaY = minionSpeed;
			}
			else
			{
				deltaY = -minionSpeed;
			}
		}
		else if (min->decisionType == 2)
		{
			if (min->posY < mp1.y)
			{
				deltaY = minionSpeed;
			}
			else
			{
				deltaY = -minionSpeed;
			}
		}
		else if (min->decisionType == 3)
		{
			if (min->posY < mp2.y)
			{
				deltaY = minionSpeed;
			}
			else
			{
				deltaY = -minionSpeed;
			}
		}
		for (int i = 0; i < bosses->size(); i++)
		{
			if (bosses->at(i).needGuard == true && (min->decisionType == 0 || min->decisionType > 1))
			{
				min->decisionType = 1;
				bosses->at(i).needGuard = false;
				min->guardTarget = bosses->at(i);
			}
		}
	}
	min->posX += deltaX * dt;
	min->posY += deltaY * dt;
	if ((double)rand() / RAND_MAX < 0.1 && getDistance(min->posX, min->posY, p->_posX, p->_posY) > 1000.0f && min->decisionType == 0)
	{
		if (((double)rand() / RAND_MAX) < 0.5)
		{
			min->decisionType = 2;
		}
		else
		{
			min->decisionType = 3;
		}
	}
	if (getDistance(min->posX, 0, p->_posX, 0) < 300 && (min->decisionType == 2 || min->decisionType == 3))
	{
		min->decisionType = 0;
	}
	if (getDistance(0, min->posX, 0, mp2.x) < 70 && (min->decisionType == 2 || min->decisionType == 3))
	{
		min->decisionType = 0;
	}

}

void spawnMinion(Boss* b, std::vector<Minion>* minionPool, std::vector<Minion>* aliveMinions)
{
	Minion newMinion = minionPool->back();
	minionPool->pop_back();
	newMinion.posX = b->posX + 60;
	newMinion.posY = b->posY + 60;
	aliveMinions->push_back(newMinion);
}

void animDraw(AnimData* anim, GLuint textures[], int x, int y,
	int w, int h)
{
	AnimDef* def = anim->def;
	int curFrameNum = anim->def->frames[anim->curFrame].frameNum;
	GLuint tex = textures[curFrameNum];
	glDrawSprite(tex, x, y, w, h);
}

void coinDraw(Coin* coin){
	animDraw(&coin->anim, coinTextures, coin->_posX - camera._camX, coin->_posY - camera._camY, 20, 20);
	if (coin->anim.isPlaying == false) animReset(&coin->anim);
}

void playerDraw(Player* player){
	animDraw(&player->anim, playerTextures2, player->_posX - camera._camX, player->_posY - camera._camY, 60, 60);
	if (player->anim.isPlaying == false) animReset(&player->anim);
}

int main(void){
	if (SDL_Init(SDL_INIT_VIDEO) < 0){
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_Window* window = SDL_CreateWindow("Project 2",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		640, 480, SDL_WINDOW_OPENGL);
	if (!window){
		fprintf(stderr, "Could not create window. ErrorCode=%s\n",
			SDL_GetError());
		SDL_Quit();
		return 1;
	}
	SDL_GL_CreateContext(window);

	GLenum glewError = glewInit();
	if (glewError != GLEW_OK){
		fprintf(stderr, "Could not initialize glew. ErrorCode=%s\n",
			glewGetErrorString(glewError));
		SDL_Quit();
		return 1;
	}
	if (!GLEW_VERSION_3_0){
		fprintf(stderr, "OpenGL max supported version is too low.\n");
		SDL_Quit();
		return 1;
	}

	glViewport(0, 0, 640, 480);
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, 640, 480, 0, 0, 100);
	glEnable(GL_TEXTURE_2D);

	kbState = SDL_GetKeyboardState(NULL);



	int bgX = -20;
	int bgY = -500;

	GLuint tile1 = glTexImageTGAFile("Images/dark_plains.tga", NULL, NULL);
	GLuint tile2 = glTexImageTGAFile("Images/plains.tga", NULL, NULL);
	GLuint link = glTexImageTGAFile("Images/link.tga", NULL, NULL);
	GLuint link0 = glTexImageTGAFile("Images/link0.tga", NULL, NULL);
	GLuint link1 = glTexImageTGAFile("Images/link1.tga", NULL, NULL);
	GLuint link2 = glTexImageTGAFile("Images/link2.tga", NULL, NULL);
	GLuint link3 = glTexImageTGAFile("Images/link3.tga", NULL, NULL);
	GLuint link4 = glTexImageTGAFile("Images/link4.tga", NULL, NULL);
	GLuint link5 = glTexImageTGAFile("Images/link5.tga", NULL, NULL);
	GLuint link6 = glTexImageTGAFile("Images/link6.tga", NULL, NULL);
	GLuint link7 = glTexImageTGAFile("Images/link7.tga", NULL, NULL);
	GLuint link8 = glTexImageTGAFile("Images/link8.tga", NULL, NULL);
	GLuint link9 = glTexImageTGAFile("Images/link9.tga", NULL, NULL);
	GLuint coinTex1 = glTexImageTGAFile("Images/coin1.tga", NULL, NULL);
	GLuint coinTex2 = glTexImageTGAFile("Images/coin2.tga", NULL, NULL);
	GLuint coinTex3 = glTexImageTGAFile("Images/coin3.tga", NULL, NULL);
	GLuint coinTex4 = glTexImageTGAFile("Images/coin4.tga", NULL, NULL);
	GLuint coinTex5 = glTexImageTGAFile("Images/coin5.tga", NULL, NULL);
	GLuint coinTex6 = glTexImageTGAFile("Images/coin6.tga", NULL, NULL);
	GLuint coinTex7 = glTexImageTGAFile("Images/coin7.tga", NULL, NULL);
	GLuint coinTex8 = glTexImageTGAFile("Images/coin8.tga", NULL, NULL);
	GLuint victory = glTexImageTGAFile("Images/victory.tga", NULL, NULL);
	GLuint minionLeft = glTexImageTGAFile("Images/MinionLeft.tga", NULL, NULL);
	GLuint minionRight = glTexImageTGAFile("Images/MinionRight.tga", NULL, NULL);
	GLuint bossFront = glTexImageTGAFile("Images/MonsterForward.tga", NULL, NULL);
	GLuint bossBack = glTexImageTGAFile("Images/MonsterBack.tga", NULL, NULL);
	GLuint projectile = glTexImageTGAFile("Images/projectile.tga", NULL, NULL);
	GLuint main_menu = glTexImageTGAFile("Images/main_menu.tga", NULL, NULL);
	GLuint you_lose = glTexImageTGAFile("Images/you_lose.tga", NULL, NULL);

	//new link sprites
	GLuint l1 = glTexImageTGAFile("Images/l1.tga", NULL, NULL);
	GLuint l2 = glTexImageTGAFile("Images/l2.tga", NULL, NULL);
	GLuint l3 = glTexImageTGAFile("Images/l3.tga", NULL, NULL);
	GLuint l4 = glTexImageTGAFile("Images/l4.tga", NULL, NULL);

	GLuint r1 = glTexImageTGAFile("Images/r1.tga", NULL, NULL);
	GLuint r2 = glTexImageTGAFile("Images/r2.tga", NULL, NULL);
	GLuint r3 = glTexImageTGAFile("Images/r3.tga", NULL, NULL);
	GLuint r4 = glTexImageTGAFile("Images/r4.tga", NULL, NULL);

	GLuint idle = glTexImageTGAFile("Images/idle.tga", NULL, NULL);

	//text sprites
	GLuint semicolon = glTexImageTGAFile("Images/semicolon.tga", NULL, NULL);
	GLuint u_a = glTexImageTGAFile("Images/1.tga", NULL, NULL);
	GLuint u_e = glTexImageTGAFile("Images/5.tga", NULL, NULL);
	GLuint u_h = glTexImageTGAFile("Images/8.tga", NULL, NULL);
	GLuint u_l = glTexImageTGAFile("Images/12.tga", NULL, NULL);
	GLuint u_p = glTexImageTGAFile("Images/16.tga", NULL, NULL);
	GLuint u_t = glTexImageTGAFile("Images/20.tga", NULL, NULL);
	GLuint u_y = glTexImageTGAFile("Images/25.tga", NULL, NULL);
	GLuint l_a = glTexImageTGAFile("Images/27.tga", NULL, NULL);
	GLuint l_e = glTexImageTGAFile("Images/31.tga", NULL, NULL);
	GLuint l_h = glTexImageTGAFile("Images/34.tga", NULL, NULL);
	GLuint l_l = glTexImageTGAFile("Images/38.tga", NULL, NULL);
	GLuint l_t = glTexImageTGAFile("Images/46.tga", NULL, NULL);

	//number sprites
	GLuint num1 = glTexImageTGAFile("Images/num1.tga", NULL, NULL);
	GLuint num2 = glTexImageTGAFile("Images/num2.tga", NULL, NULL);
	GLuint num3 = glTexImageTGAFile("Images/num3.tga", NULL, NULL);
	GLuint num4 = glTexImageTGAFile("Images/num4.tga", NULL, NULL);
	GLuint num5 = glTexImageTGAFile("Images/num5.tga", NULL, NULL);

	play[0] = u_p;
	play[1] = u_l;
	play[2] = u_a;
	play[3] = u_y;

	numbers[1] = num1;
	numbers[2] = num2;
	numbers[3] = num3;
	numbers[4] = num4;
	numbers[5] = num5;

	health[0] = u_h;
	health[1] = u_e;
	health[2] = u_a;
	health[3] = u_l;
	health[4] = u_t;
	health[5] = u_h;
	health[6] = semicolon;

	coinTextures[0] = coinTex1;
	coinTextures[1] = coinTex2;
	coinTextures[2] = coinTex3;
	coinTextures[3] = coinTex4;
	coinTextures[4] = coinTex5;
	coinTextures[5] = coinTex6;
	coinTextures[6] = coinTex7;
	coinTextures[7] = coinTex8;

	playerTextures[0] = link0;
	playerTextures[1] = link1;
	playerTextures[2] = link2;
	playerTextures[3] = link3;
	playerTextures[4] = link4;
	playerTextures[5] = link5;
	playerTextures[6] = link6;
	playerTextures[7] = link7;
	playerTextures[8] = link8;
	playerTextures[9] = link9;

	playerTextures2[0] = idle;
	playerTextures2[1] = l1;
	playerTextures2[2] = l2;
	playerTextures2[3] = l3;
	playerTextures2[4] = l4;
	playerTextures2[5] = r1;
	playerTextures2[6] = r2;
	playerTextures2[7] = r3;
	playerTextures2[8] = r4;

	tiles[0] = tile1;
	tiles[1] = tile2;


	int map[40][40] =
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
	};

	Player player;
	Boss b1;
	b1.posX = 530.0f;
	b1.posY = 178.0f;
	Boss b2;
	b2.posX = 1945.0f;
	b2.posY = 178.0f;
	Boss b3;
	b3.posX = 530.0f;
	b3.posY = 988.0f;
	Boss b4;
	b4.posX = 1945.0f;
	b4.posY = 988.0f;
	drawnBosses.push_back(b1);
	drawnBosses.push_back(b2);
	drawnBosses.push_back(b3);
	drawnBosses.push_back(b4);
	Minion iMinion1;
	Minion iMinion2;
	Minion iMinion3;
	Minion iMinion4;
	Minion m1;
	Minion m2;
	Minion m3;
	Minion m4;
	Minion m5;
	Minion m6;
	Minion m7;
	Minion m8;
	Minion m9;
	Minion m10;
	Minion m11;
	Minion m12;
	Minion m13;
	Minion m14;
	Minion m15;
	Minion m16;
	Minion m17;
	Minion m18;
	Minion m19;
	Minion m20;
	Minion m21;
	Minion m22;
	Minion m23;
	Minion m24;
	Minion m25;
	Minion m26;
	Minion m27;
	Minion m28;
	Minion m29;
	Minion m30;
	deadMinions.push_back(m1);
	deadMinions.push_back(m2);
	deadMinions.push_back(m3);
	deadMinions.push_back(m4);
	deadMinions.push_back(m5);
	deadMinions.push_back(m6);
	deadMinions.push_back(m7);
	deadMinions.push_back(m8);
	deadMinions.push_back(m9);
	deadMinions.push_back(m10);
	deadMinions.push_back(m11);
	deadMinions.push_back(m12);
	deadMinions.push_back(m13);
	deadMinions.push_back(m14);
	deadMinions.push_back(m15);
	deadMinions.push_back(m16);
	deadMinions.push_back(m17);
	deadMinions.push_back(m18);
	deadMinions.push_back(m19);
	deadMinions.push_back(m20);
	deadMinions.push_back(m21);
	deadMinions.push_back(m22);
	deadMinions.push_back(m23);
	deadMinions.push_back(m24);
	deadMinions.push_back(m25);
	deadMinions.push_back(m26);
	deadMinions.push_back(m27);
	deadMinions.push_back(m28);
	deadMinions.push_back(m29);
	deadMinions.push_back(m30);

	iMinion1.posX = 1228.0f;
	iMinion1.posY = 30.0f;
	iMinion2.posX = 171.0f;
	iMinion2.posY = 585.0f;
	iMinion3.posX = 1228.0f;
	iMinion3.posY = 1095.0f;
	iMinion4.posX = 2167.0f;
	iMinion4.posY = 578.0f;

	drawnMinions.push_back(iMinion1);
	drawnMinions.push_back(iMinion2);
	drawnMinions.push_back(iMinion3);
	drawnMinions.push_back(iMinion4);

	Projectile p1;
	Projectile p2;
	Projectile p3;
	Projectile p4;
	Projectile p5;
	projectiles.push_back(p1);
	projectiles.push_back(p2);
	projectiles.push_back(p3);
	projectiles.push_back(p4);
	projectiles.push_back(p5);

	AnimDef player_idle_left
	{
		"player_idle_left",
		{ { 1, .1 } },
		10
	};
	AnimDef player_idle_right
	{
		"player_idle_right",
		{ { 5, .1 } },
		10
	};
	AnimDef player_walk_left
	{
		"player_walk_left",
		{ { 1, .1 }, { 3, .1 }, { 2, .1 }, {3, .1} },
		10
	};
	AnimDef player_walk_right
	{
		"player_walk_right",
		{ { 5, .1 }, { 7, .1 }, { 6, .1 }, { 7, .1 } },
		10
	};

	AnimDef player_walking
	{
		"player_walking",
		{ { 0, .1 }, { 1, .1 }, { 2, .1 }, { 3, .1 }, { 4, 0 }, { 5, .1 }, { 6, .1 }, { 7, .1 }, { 8, .1 }, { 9, .1 } },
		10
	};
	player.anim =
	{
		&player_walking,
		1,
		.1,
		true
	};

	AnimDef coin_spinning
	{
		"coin_spinning",
		{ { 0, .1 }, { 1, .1 }, { 2, .1 }, { 3, .1 }, { 4, 0 }, { 5, .1 }, { 6, .1 }, { 7, .1 } },
		8
	};

	AnimData coin1Anim;
	AnimData coin2Anim;
	AnimData coin3Anim;
	Coin coin1 =
	{
		coin1Anim =
		{
			&coin_spinning,
			1,
			.1,
			true
		},
		990, -220, true
	};

	Coin coin2 =
	{
		coin2Anim =
		{
			&coin_spinning,
			1,
			.1,
			true
		},
		1704, -13, true
	};

	Coin coin3 =
	{
		coin3Anim =
		{
			&coin_spinning,
			1,
			.1,
			true
		},
		1020, 122, true
	};
	FMOD_SYSTEM* fmod;
	FMOD_System_Create(&fmod);
	// Initalize FMOD with up to 100 sounds playing at once
	FMOD_System_Init(fmod, 100, FMOD_INIT_NORMAL, 0);
	
	FMOD_SOUND* bgMusic;
	FMOD_CHANNEL* bgChan;
	FMOD_CHANNELGROUP* channelGroup = 0;
	FMOD_System_CreateStream(fmod, "game_music.mp3", FMOD_DEFAULT, 0, &bgMusic);
	FMOD_System_PlaySound(fmod, FMOD_CHANNEL_FREE, bgMusic, false, &bgChan);

	int mouseX, mouseY, mouseDeltaX, mouseDeltaY;
	Uint32 mouseButtons = 0;
	Uint32 prevMouseButtons = 0;
	int lastMouseDownX, lastMouseDownY;
	Uint32 lastMouseDownMs;
	float safeTimer = 0.0f;

	/* Physics runs at 100fps, or 10ms / physics frame */
	float physicsDeltaTime = 1 / 100.0f;
	int physicsDeltaMs = 10;
	Uint32 lastPhysicsFrameMs = 0;
	Uint32 lastFrameMs;
	Uint32 currentFrameMs = SDL_GetTicks();
	while (!shouldExit){
		/* Save last frame's values (was just keyboard update) */
		lastFrameMs = currentFrameMs;
		prevMouseButtons = mouseButtons;
		memcpy(kbPrevState, kbState, sizeof(kbPrevState));
		SDL_Event event;
		while (SDL_PollEvent(&event)){
			switch (event.type){
			case SDL_QUIT:
				shouldExit = 1;
			}
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		FMOD_System_Update(fmod);

		mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
		SDL_GetRelativeMouseState(&mouseDeltaX, &mouseDeltaY);

		currentFrameMs = SDL_GetTicks();
		float deltaTime = (currentFrameMs - lastFrameMs) / 1000.0f;

		glClearColor(0, ((double)rand()/RAND_MAX), 1, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		if (gameState == 0)
		{
			glDrawSprite(main_menu, 0, 0, 640, 480);
			float textX = 280.0f;
			float textY = 300.0f;
			for (int i = 0; i < 4; i++)
			{
				glDrawSprite(play[i], textX + (i*16), textY, 16, 26);
			}
			if (mouseButtons&SDL_BUTTON_LMASK == 1 && !prevMouseButtons&SDL_BUTTON_LMASK == 1)
			{
				if (AABBIntersect(textX, textY, textX + 64, textY + 26, mouseX - 10, mouseY - 10, 20, 20))
				{
					gameState = 1;
				}
			}
		}
		else if (gameState == 1)
		{
			playerUpdate(&player, deltaTime);
			if (player.movementState == 0)
			{
				player.anim.def = &player_idle_left;
			}
			else if (player.movementState == 1)
			{
				player.anim.def = &player_walk_left;
			}
			else if (player.movementState == 2)
			{
				player.anim.def = &player_walk_right;
			}
			else if(player.movementState == 3)
			{
				player.anim.def = &player_idle_right;
			}
			cameraUpdate(&camera, deltaTime);

			//trajectory code
			if (mouseButtons&SDL_BUTTON_LMASK == 1 && !prevMouseButtons&SDL_BUTTON_LMASK == 1)
			{
				mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
				int mouseClickX = mouseX + (int)camera._camX;
				int mouseClickY = mouseY + (int)camera._camY;
				if (projectiles.size() > 0)
				{
					//printf("Fire!\n");
					Projectile p = projectiles.back();
					p.posX = player._posX;
					p.posY = player._posY;
					projectiles.pop_back();
					float deltaX = (mouseClickX)-p.posX;
					float deltaY = (mouseClickY)-p.posY;
					float theta = std::abs(std::atan(deltaY / deltaX));
					p.velocityX = p.speed * std::cos(theta);
					//printf("VelocityX: %f\n", p.velocityX);
					p.velocityY = p.speed * std::sin(theta);
					//printf("VelocityY: %f\n", p.velocityY);
					if (deltaX < 0)
					{
						p.velocityX *= -1;
					}
					if (deltaY < 0)
					{
						p.velocityY *= -1;
					}
					thrownProjectiles.push_back(p);
				}
			}

			
			for (int i = 0; i < drawnBosses.size(); i++)
			{
				drawnBosses[i].spawnTimer -= deltaTime;
				if (drawnBosses[i].spawnTimer < 0 && deadMinions.size() > 0)
				{
					spawnMinion(&drawnBosses[i], &deadMinions, &drawnMinions);
					drawnBosses[i].spawnTimer = 4.0f;
				}
			}
			for (int i = 0; i < drawnMinions.size(); i++)
			{
				MinionUdpdate(&drawnMinions[i], deltaTime, &drawnBosses, &player);
			}
			for (int i = 0; i < thrownProjectiles.size(); i++)
			{
				updateProjectile(&thrownProjectiles[i], deltaTime);
			}

			if (safeTimer > 0)
			{
				safeTimer -= deltaTime;
			}

			/* Physics update */
			do {
				/* 1. Physics movement */
				

				if (kbState[SDL_SCANCODE_A]) {
					_newX -= _speed;
					_newCamX -= _speed;
				}
				if (kbState[SDL_SCANCODE_D]) {
					_newX += _speed;
					_newCamX += _speed;
				}
				if (kbState[SDL_SCANCODE_W]) {
					_newY -= _speed;
					_newCamY -= _speed;
				}
				if (kbState[SDL_SCANCODE_S]) {
					_newY += _speed;
					_newCamY += _speed;
				}
				/* Update positions of camera here */
				if (kbState[SDL_SCANCODE_RIGHT]) {
					_newCamX += _speed;
				}
				if (kbState[SDL_SCANCODE_LEFT]) {
					_newCamX -= _speed;
				}
				if (kbState[SDL_SCANCODE_UP]) {
					_newCamY -= _speed;
				}
				if (kbState[SDL_SCANCODE_DOWN]) {
					_newCamY += _speed;
				}
				/* 2. Physics collision detection */
				if (AABBIntersect(player._posX, player._posY, 60, 60, coin1._posX, coin1._posY, 20, 20))
				{
					coin1.isDrawn = false;
				}
				if (AABBIntersect(player._posX, player._posY, 60, 60, coin2._posX, coin2._posY, 20, 20))
				{
					coin2.isDrawn = false;
				}
				if (AABBIntersect(player._posX, player._posY, 60, 60, coin3._posX, coin3._posY, 20, 20))
				{
					coin3.isDrawn = false;
				}
				for (int i = 0; i < thrownProjectiles.size(); i++)
				{
					if (thrownProjectiles.size() > 0)
					{
						if (!AABBIntersect(thrownProjectiles[i].posX, thrownProjectiles[i].posY, 20, 20, camera._camX, camera._camY, 640, 480))
						{
							if (thrownProjectiles[i].posX < 5000)
							{
								projectiles.push_back(thrownProjectiles[i]);
							}
							thrownProjectiles.erase(thrownProjectiles.begin() + i);
						}
					}
				}
				for (int i = 0; i < thrownProjectiles.size(); i++)
				{
					for (int j = 0; j < drawnMinions.size(); j++)
					{
						if (thrownProjectiles.size() > 0)
						{
							if (AABBIntersect(thrownProjectiles[i].posX, thrownProjectiles[i].posY, 20, 20, drawnMinions[j].posX, drawnMinions[j].posY, 64, 40))
							{
								if (thrownProjectiles.size() > 0)
								{
									projectiles.push_back(thrownProjectiles[i]);
									if (drawnMinions[j].decisionType == 1)
									{
										drawnMinions[j].guardTarget.needGuard = true;
									}
									deadMinions.push_back(drawnMinions[j]);
									drawnMinions.erase(drawnMinions.begin() + j);
									//thrownProjectiles.erase(thrownProjectiles.begin() + i);
									thrownProjectiles[i].posX = 8000;
								}
							}
						}
					}
				}
				for (int i = 0; i < thrownProjectiles.size(); i++)
				{
					for (int j = 0; j < drawnBosses.size(); j++)
					{
						if (thrownProjectiles.size() > 0)
						{
							if (AABBIntersect(thrownProjectiles[i].posX, thrownProjectiles[i].posY, 20, 20, drawnBosses[j].posX, drawnBosses[j].posY, 64, 40))
							{
								if (thrownProjectiles.size() > 0)
								{
									projectiles.push_back(thrownProjectiles[i]);
									drawnBosses[j].health -= 1;
									if (drawnBosses[j].health == 0)
									{
										deadBosses.push_back(drawnBosses[j]);
										drawnBosses.erase(drawnBosses.begin() + j);
									}
									//thrownProjectiles.erase(thrownProjectiles.begin() + i);
									thrownProjectiles[i].posX = 8000;
								}
							}
						}
					}
				}
				for (int i = 0; i < drawnMinions.size(); i++)
				{
					if (AABBIntersect(player._posX + 10, player._posY + 20, 40, 20, drawnMinions[i].posX, drawnMinions[i].posY, 64, 40))
					{
						deadMinions.push_back(drawnMinions[i]);
						drawnMinions.erase(drawnMinions.begin() + i);
						if (safeTimer <= 0)
						{
							player.health -= 1;
							safeTimer = 2.0f;
						}
					}
				}
				for (int i = 0; i < drawnMinions.size(); i++)
				{
					for (int j = 0; j < deadBosses.size(); j++)
					{
						if (drawnMinions[i].guardTarget.posX == deadBosses[j].posX && drawnMinions[i].guardTarget.posY == deadBosses[j].posY)
						{
							drawnMinions[i].decisionType = 0;
						}
					}
				}
				for (int i = 0; i < drawnMinions.size(); i++)
				{
					if (getDistance(player._posX, player._posY, drawnMinions[i].guardTarget.posX, drawnMinions[i].guardTarget.posY) < 250)
					{
						drawnMinions[i].guardTarget.needGuard = true;
						drawnMinions[i].decisionType = 0;
					}
				}
				/* 3. Physics collision resolution */
				lastPhysicsFrameMs += physicsDeltaMs;
			} while (lastPhysicsFrameMs + physicsDeltaMs < currentFrameMs);

			/* Update positions, animations of sprites here */

			animTick(&player.anim, deltaTime);
			animTick(&coin1.anim, deltaTime);
			animTick(&coin2.anim, deltaTime);
			animTick(&coin3.anim, deltaTime);

			if (kbState[SDL_SCANCODE_SPACE] && !kbPrevState[SDL_SCANCODE_SPACE]) {
				printf("Player X: %f\n", player._posX);
				printf("Player Y: %f\n", player._posY);
				printf("Camera X: %f\n", camera._camX);
				printf("Camera Y: %f\n", camera._camY);
				printf("Movement State: %d\n", player.movementState);
			}

			/* Draw backgrounds, handling parallax */
			/* Draw sprites */
			/* Draw foregrounds, handling parallax */
			for (int i = 0; i < 40; i++) //x
			{
				for (int j = 0; j < 40; j++) //y
				{
					int x = i * 64;
					int y = j * 32;
					int tileNum = map[i][j];
					if (AABBIntersect(camera._camX, camera._camY, 640, 480, x, y, 64, 32))
					{
						glDrawSprite(tiles[tileNum], x - camera._camX, y - camera._camY, 64, 32);
					}
				}
			}
			for (int i = 0; i < drawnMinions.size(); i++)
			{
				if (drawnMinions[i].posX >= player._posX)
				{
					glDrawSprite(minionLeft, drawnMinions[i].posX - camera._camX, drawnMinions[i].posY - camera._camY, 64, 40);
				}
				else if (drawnMinions[i].posX < player._posX)
				{
					glDrawSprite(minionRight, drawnMinions[i].posX - camera._camX, drawnMinions[i].posY - camera._camY, 64, 40);
				}
			}
			for (int i = 0; i < drawnBosses.size(); i++)
			{
				glDrawSprite(bossFront, drawnBosses[i].posX - camera._camX, drawnBosses[i].posY - camera._camY, 64, 40);
			}
			for (int i = 0; i < thrownProjectiles.size(); i++)
			{
				glDrawSprite(projectile, thrownProjectiles[i].posX - camera._camX, thrownProjectiles[i].posY - camera._camY, 20, 20);
			}
			playerDraw(&player);
			writeHealth(&player);
			if (drawnBosses.size() == 0)
			{
				gameState = 3;
			}
			if (player.health == 0)
			{
				gameState = 4;
			}
		}
		else if (gameState == 3)
		{
			camera._camX = 0;
			camera._camY = 0;
			glDrawSprite(victory, 0, 0, 640, 480);
		}
		else if (gameState == 4)
		{
			glDrawSprite(you_lose, 0, 0, 640, 480);
		}

		SDL_GL_SwapWindow(window);
	}

	SDL_Quit();

	return 0;
};