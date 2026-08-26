#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdio.h>

const int screen_width = 640;
const int screen_height = 480;


const char level[] = 
  "................................................................"
  "................................................................"
  "................................................................"
  "................................................................"
  ".......#.............................#..#......................."
  "...................#########...................................."
  "..................##.................#..#......................."
  ".................##............................................."
  "................##.............................................."
  "#########################################.######...#############"
  "........................................#.#......##............."
  "............................#############.#.....##.............."
  "............................#.............#....##..............."
  "............................#..############...##................"
  "............................#................##................."
  "............................##################..................";
const int level_width = 64;
const int level_height = 16;

float camera_pos_x = 0.0;
float camera_pos_y = 0.0;

float player_pos_x = 0.0;
float player_pos_y = 0.0;
float player_vel_x = 0.0;
float player_vel_y = 0.0;

char get_tile_at(int x, int y) {
  if (x >= 0 && x < level_width && y >= 0 && y < level_height) {
    return level[y * level_width + x];
  } else {
    return ' ';
  }
}

int main(int argc, char* argv[]) {

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize\n");
    return 1;
  }

  SDL_Surface* screen = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_SWSURFACE);
  if (screen == NULL) {
    printf("window could not be created\n");
    return  1;
  }

  int update = 1;

  SDL_Event event;
  int running = 1;

  const int fps = 30;
  const int framedelay = 1000 / fps;
  Uint32 framestart;
  int frametime;

  while (running) {

    SDL_FillRect(screen, &screen->clip_rect, SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00));

    framestart = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	running = 0;
      }
      if (event.type == SDL_KEYDOWN) {
	if (event.key.keysym.sym == SDLK_ESCAPE) {
	  running = 0;
	} else if (event.key.keysym.sym == SDLK_UP) {
	  player_vel_y = -6.0;
	} else if (event.key.keysym.sym == SDLK_DOWN) {
	  player_vel_y = 6.0;
	} else if (event.key.keysym.sym == SDLK_LEFT) {
	  player_vel_x = -6.0;
	} else if (event.key.keysym.sym == SDLK_RIGHT) {
	  player_vel_x = 6.0;
	}
      }
      if (event.type == SDL_KEYUP) {
	if (event.key.keysym.sym == SDLK_UP) {
	  player_vel_y = 0.0;
	} else if (event.key.keysym.sym == SDLK_DOWN) {
	  player_vel_y = 0.0;
	} else if (event.key.keysym.sym == SDLK_LEFT) {
	  player_vel_x = 0.0;
	} else if (event.key.keysym.sym == SDLK_RIGHT) {
	  player_vel_x = 0.0;
	}
      }
    }

    player_pos_x += player_vel_x * ((float)framedelay / 1000.0);
    player_pos_y += player_vel_y * ((float)framedelay / 1000.0);

    if (player_pos_x < 0.0) player_pos_x = 0.0;
    if (player_pos_y < 0.0) player_pos_y = 0.0;

    if (player_pos_x > (float)(level_width - 1)) player_pos_x = (float)(level_width - 1);
    if (player_pos_y > (float)(level_height - 1)) player_pos_y = (float)(level_height - 1);

    // update camera based on player
    camera_pos_x = player_pos_x;
    camera_pos_y = player_pos_y;

    // draw current frame
    int tile_width = 16;
    int tile_height = 16;
    int visible_tiles_x = screen_width / tile_width;
    int visible_tiles_y = screen_height / tile_height;

    if (camera_pos_x > level_width - visible_tiles_x / 2) camera_pos_x = level_width - visible_tiles_x / 2;
    if (camera_pos_y > level_height - visible_tiles_y / 2) camera_pos_y = level_height - visible_tiles_y / 2;

    // calculate top left most visible tile
    float offset_x = camera_pos_x - (float)visible_tiles_x / 2.0;
    float offset_y = camera_pos_y - (float)visible_tiles_y / 2.0;

    if (offset_x < 0) offset_x = 0;
    if (offset_y < 0) offset_y = 0;

    SDL_Rect drect = {x:0, y:0, w:tile_width, h:tile_height};

    for (int x=0; x < visible_tiles_x; x++) {
      for (int y=0; y < visible_tiles_y; y++) {
	drect.x = x * tile_width;
	drect.y = y * tile_height;
	char tile_id = get_tile_at(offset_x + x, offset_y + y);
	switch (tile_id) {
	case '.':
	  SDL_FillRect(screen, &drect, SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00));
	  break;
	case '#':
	  SDL_FillRect(screen, &drect, SDL_MapRGB(screen->format, 0x00, 0x00, 0xFF));
	  break;
	default:
	  break;
	}
      }
    }

    // draw player
    drect.x = (player_pos_x - offset_x) * tile_width;
    drect.y = (player_pos_y - offset_y) * tile_height;
    SDL_FillRect(screen, &drect, SDL_MapRGB(screen->format, 0x00, 0xFF, 0xFF));

    // delay to limit fps
    frametime = SDL_GetTicks() - framestart;
    if (framedelay > frametime) {
      SDL_Delay(framedelay - frametime);
    }

    SDL_Flip(screen);
  }

  SDL_Quit();

  return 0;
  
}
