//Albert Su
//Space Invaders
//Press space to start
//Can't get the ships to drop consistently
//player and enemies get one bullet

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include "ShaderProgram.h"
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "vector"
#include "SheetSprite.h"
using namespace std;
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *image_path){
	SDL_Surface *surface = IMG_Load(image_path);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_FreeSurface(surface);
	return textureID;
}

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
		float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glUseProgram(program->programID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}


class Entity {
public:
	Entity(float x, float y, float width, float height, float speed, SheetSprite sprite) :x(x), y(y), speed(speed), sprite(sprite), width(width), height(height){}
	float x;
	float y;
	float width;
	float height;
	float speed;
	SheetSprite sprite;

	bool alive = 1;

	bool checkCol(Entity e)
	{
		if (y - height >= e.y + e.height || y + height <= e.y - e.height || x - width >= e.x + e.width || x + width <= e.x - e.width)
			return 0;
		else
			return 1;
	}
	void Draw()
	{
		sprite.Draw();
	}
};

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 500, 600, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	SDL_Event event;
	bool done = false;


	glViewport(0, 0, 500, 600);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	GLuint font= LoadTexture("font1.png");
	GLuint sprites = LoadTexture("sheet.png");

	SheetSprite playerSprite(&program, sprites, 112.0f / 1024.0f, 716.0f / 1024.0f, 112.0f / 1024.0f, 75.0f /1024.0f, 0.2);
	SheetSprite enemySprite(&program, sprites, 423.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f /1024.0f, 0.25);

	SheetSprite bluelaser(&program, sprites, 860.0f / 1024.0f, 475.0f / 1024.0f, 9.0f / 1024.0f, 54.0f /1024.0f, 0.2);
	SheetSprite greenlaser(&program, sprites, 858.0f / 1024.0f, 0.0f / 1024.0f, 9.0f / 1024.0f, 54.0f /1024.0f, 0.2);

	float lastFrameTicks = 0.0f;
	float elapsed;
	float ticks;

	bool shoot = 1;
	bool eShoot = 1;

	Entity player(0.0, -1.8f, 112.0f / 1024.0f, 75.0f / 1024.0f, 2.0, playerSprite);
	Entity playerLaser(0, 0, 9.0f / 1024.0f, 54.0f / 1024.0f, 2.7, bluelaser);
	Entity eLaser(0, 0, 9.0f / 1024.0f, 54.0f / 1024.0f, 2.7, greenlaser);

	int numShips = 12;

	vector<Entity> enemies;
	for (int i = 0; i < numShips; i++)
	{
		Entity enemy(-3.2 + float(i % 6), 1.8 - (i / 6) / 2.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.7, enemySprite);
		enemies.push_back(enemy);
	}

	enum{ START, GAME};
	int state = START;

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}


		}
		glClear(GL_COLOR_BUFFER_BIT);
		
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		switch (state){
		case START:
			//main menu
			modelMatrix.identity();
			modelMatrix.Translate(-2.0, 1, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Space Invaders", .4f, -.1f);
			
			if (keys[SDL_SCANCODE_SPACE])
			{
				//gets the starting tick
				lastFrameTicks = (float)SDL_GetTicks() / 1000.0f;
				state = GAME;
			}
			break;
		case GAME:
			ticks = (float)SDL_GetTicks() / 1000.0f;
			elapsed = ticks - lastFrameTicks;
			lastFrameTicks = ticks;
			//input, left, right, space to shoot
			if (keys[SDL_SCANCODE_LEFT])
			{
				if (player.x > -3.4)
					player.x -= elapsed*player.speed;
			}
			if (keys[SDL_SCANCODE_RIGHT])
			{
				if (player.x < 3.4)
					player.x += elapsed*player.speed;
			}
			if (keys[SDL_SCANCODE_SPACE])
			{
				if (shoot)
				{
					shoot = 0;
					playerLaser.x = player.x;
					playerLaser.y = player.y;
				}

			}
			//draw player
			modelMatrix.identity();
			modelMatrix.Translate(player.x, player.y, 0);
			program.setModelMatrix(modelMatrix);
			player.Draw();

			//picks a random ship to shoot
			int shooter = rand() % 12;
			//draw enemies
			for (int j = 0; j < enemies.size(); j++)
			{
				if (enemies[j].alive)
				{
					modelMatrix.identity();
					enemies[j].x += elapsed * enemies[j].speed;
					//if the enemies get to an edge they drop and reverse
					if (enemies[j].x > 3.4)
					{
						for (int k = 0; k < enemies.size(); k++)
						{
							enemies[k].y -= elapsed*2;
							enemies[k].speed *= -1;
						}
					}
					else if (enemies[j].x < -3.4)
					{
						for (int k = 0; k < enemies.size(); k++)
						{
							enemies[k].y -= elapsed*2;
							enemies[k].speed *= -1;
						}
					}
					//if enemies get under screen player loses
					if (enemies[j].y < -1.8)
						done=true;
					if (eShoot && j==shooter)
					{
						eShoot = 0;
						eLaser.x = enemies[j].x;
						eLaser.y = enemies[j].y;
					}
					modelMatrix.Translate(enemies[j].x, enemies[j].y, 0);
					program.setModelMatrix(modelMatrix);
					enemies[j].Draw();
				}
			}

			eLaser.y -= eLaser.speed* elapsed;
			if (eLaser.y < -2)
			{
				eLaser.y = 3;
				eShoot = 1;
			}
			if (eLaser.checkCol(player))
			{
				done = true;
			}

			modelMatrix.identity();
			modelMatrix.Translate(eLaser.x, eLaser.y, 0);
			program.setModelMatrix(modelMatrix);
			eLaser.Draw();
			//player bullet updates
			playerLaser.y += playerLaser.speed*elapsed;
			if (playerLaser.y > 2)
			{
				shoot = 1;
			}
			//checking collision with enemies
			for (int j = 0; j < enemies.size(); j++)
			{
				if (enemies[j].alive && playerLaser.checkCol(enemies[j]))
				{
					playerLaser.x = 0;
					playerLaser.y = 3;
					enemies[j].alive = 0;
					shoot = 1;
					numShips--;
				}
			}
			//checking collision with other laser
			if (playerLaser.checkCol(eLaser))
			{
				playerLaser.x = 0;
				playerLaser.y = 3;
				eLaser.x = 0;
				eLaser.y = 0;
				shoot = true;
				eShoot = true;
			}

			modelMatrix.identity();
			modelMatrix.Translate(playerLaser.x, playerLaser.y, 0);
			program.setModelMatrix(modelMatrix);
			playerLaser.Draw();

			//win when all enemies are dead
			if (numShips == 0)
			{
				done = true;
			}
			break;
		}
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
