#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdio.h>
#include <math.h>

// size of screen
const int screen_width = 320;
const int screen_height = 240;
#define SCREEN_MAG 2

// size of tile
const int tile_width = 16;
const int tile_height = 16;

// how many tiles we can see
const int visible_tiles_x = screen_width / tile_width;
const int visible_tiles_y = screen_height / tile_height;

// level map
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
  "............................##################.................."
  "................................................................"
  "................................................................";

// map size
const int level_width = 64;
const int level_height = 18;

// store camera pos in map
float camera_pos_x = 0.0;
float camera_pos_y = 0.0;

// store player pos and vel
float player_pos_x = 0.0;
float player_pos_y = 0.0;
float player_vel_x = 0.0;
float player_vel_y = 0.0;
float player_looking_left = 0;

int player_on_ground = 0;

char get_tile_at(int x, int y) {
  if (x >= 0 && x < level_width && y >= 0 && y < level_height) {
    return level[y * level_width + x];
  } else {
    return ' ';
  }
}

SDL_Surface* flipSurfaceHorizontal(SDL_Surface* source) {
    if (!source) return NULL;

    // 1. Create a blank surface with the exact same dimensions and pixel format
    SDL_Surface* flipped = SDL_CreateRGBSurface(
        source->flags, 
        source->w, source->h, 
        source->format->BitsPerPixel,
        source->format->Rmask, source->format->Gmask, 
        source->format->Bmask, source->format->Amask
    );

    if (!flipped) return NULL;

    // 2. Lock surfaces if required by the hardware memory
    if (SDL_MUSTLOCK(source)) SDL_LockSurface(source);
    if (SDL_MUSTLOCK(flipped)) SDL_LockSurface(flipped);

    // 3. Perform the row-by-row pixel mirroring
    // We treat the pixels as 32-bit integers (Uint32) since you are using PNGs with alpha
    Uint32* srcPixels = (Uint32*)source->pixels;
    Uint32* destPixels = (Uint32*)flipped->pixels;

    int width = source->w;
    int height = source->h;
    
    // SDL_Surface 'pitch' is the length of a row in bytes. 
    // Dividing by 4 gives us the row length measured in 32-bit integers (Uint32)
    int srcPitch = source->pitch / 4;
    int destPitch = flipped->pitch / 4;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Read from left-to-right on the source row
            Uint32 pixel = srcPixels[y * srcPitch + x];
            
            // Write from right-to-left on the destination row
            destPixels[y * destPitch + (width - 1 - x)] = pixel;
        }
    }

    // 4. Unlock memory surfaces
    if (SDL_MUSTLOCK(flipped)) SDL_UnlockSurface(flipped);
    if (SDL_MUSTLOCK(source)) SDL_UnlockSurface(source);

    // 5. Explicitly pass over transparency flags to the new surface
    if (source->flags & SDL_SRCALPHA) {
        SDL_SetAlpha(flipped, SDL_SRCALPHA, source->format->alpha);
    }

    return flipped;
}


int main(int argc, char* argv[]) {

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize\n");
    return 1;
  }

  SDL_Surface* screen = SDL_SetVideoMode(screen_width * SCREEN_MAG, screen_height * SCREEN_MAG, 32, SDL_SWSURFACE);
  if (screen == NULL) {
    printf("window could not be created\n");
    return  1;
  }

  SDL_Surface* virtualScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, screen_width, screen_height,
						    32, screen->format->Rmask,
						    screen->format->Gmask,
						    screen->format->Bmask,
						    screen->format->Amask);

  SDL_Surface* loadedImage = IMG_Load("player.png");
  if (!loadedImage) {
    printf("unable to load image\n");
    SDL_Quit();
    return 1;
  }
  SDL_Surface* optimizedImage = SDL_DisplayFormatAlpha(loadedImage);
  SDL_FreeSurface(loadedImage);
  SDL_Surface* flippedImage = flipSurfaceHorizontal(optimizedImage);

  int update = 1;

  SDL_Event event;
  int running = 1;

  const int fps = 30;
  const int framedelay = 1000 / fps;
  Uint32 framestart;
  int frametime;
  int up_pressed=0, down_pressed=0, left_pressed=0, right_pressed = 0;

  while (running) {

    SDL_FillRect(virtualScreen, &virtualScreen->clip_rect, SDL_MapRGB(virtualScreen->format, 0xFF, 0x00, 0x00));

    framestart = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	running = 0;
      }
      if (event.type == SDL_KEYDOWN) {
	if (event.key.keysym.sym == SDLK_ESCAPE) {
	  running = 0;
	} else if (event.key.keysym.sym == SDLK_UP) {
	  up_pressed = 1;
	} else if (event.key.keysym.sym == SDLK_DOWN) {
	  down_pressed = 1;
	} else if (event.key.keysym.sym == SDLK_LEFT) {
	  left_pressed = 1;
	  player_looking_left = 1;
	} else if (event.key.keysym.sym == SDLK_RIGHT) {
	  player_looking_left = 0;
	  right_pressed = 1;
	} else if (event.key.keysym.sym == SDLK_SPACE) { // jump
	  if (player_vel_y == 0) {
	    player_vel_y = -12.0f;
	  }
	}
      }
      if (event.type == SDL_KEYUP) {
	if (event.key.keysym.sym == SDLK_UP) {
	  up_pressed = 0;
	} else if (event.key.keysym.sym == SDLK_DOWN) {
	  down_pressed = 0;
	} else if (event.key.keysym.sym == SDLK_LEFT) {
	  left_pressed = 0;
	} else if (event.key.keysym.sym == SDLK_RIGHT) {
	  right_pressed = 0;
	}
      }
    }

    /*    if (up_pressed) {
      player_vel_y = -6.0;
    }
    if (down_pressed) {
      player_vel_y = 6.0;
      } */
    if (left_pressed) {
      if (player_on_ground) {
	player_vel_x += -1.0;
      } else {
	player_vel_x += -0.5;
      }
    }
    if (right_pressed) {
      if (player_on_ground) {
	player_vel_x += 1.0;
      } else {
	player_vel_x += 0.5;
      }
    }

    // get elapsed time between frames
    float elapsed = (float)framedelay / 1000.0;

    // add gravity
    player_vel_y += 20.0 * elapsed;

    // drag if on ground
    if (player_on_ground) {
      player_vel_x += -3.0 * player_vel_x * elapsed;
      // clamp velocity to zero if close so we can stop
      if (fabs(player_vel_x) < 0.01) player_vel_x = 0.0;
    }

    // clamp velocities
    if (player_vel_x > 10.0) player_vel_x = 10.0;
    if (player_vel_x < -10.0) player_vel_x = -10.0;
    if (player_vel_y > 10.0) player_vel_y = 10.0;
    if (player_vel_y < -10.0) player_vel_y = -10.0;
    
    // new player position
    // add the velocity to the player position
    float player_new_pos_x = player_pos_x + player_vel_x * elapsed;
    float player_new_pos_y = player_pos_y + player_vel_y * elapsed;

    // check for collision in x direction
    if (player_vel_x <= 0) { // player moving left or stopped
      // check top left and bottom left for solid block
      if (get_tile_at(player_new_pos_x, player_pos_y) != '.' ||
	  get_tile_at(player_new_pos_x, player_pos_y + 0.9) != '.' ) {

	player_new_pos_x = (int)player_new_pos_x + 1; // correct for collision
	player_vel_x = 0.0;
	
      }
    } else { // player moving right
      // check top right and bottom right for solid block
      if (get_tile_at(player_new_pos_x + 1.0, player_pos_y) != '.' ||
	  get_tile_at(player_new_pos_x + 1.0, player_pos_y + 0.9) != '.' ) {

	player_new_pos_x = (int)player_new_pos_x; // correct for collision
	player_vel_x = 0.0;
	
      }
    }

    // check for collision in y direction
    player_on_ground = 0;
    if (player_vel_y <= 0) { // player moving up or stopped
      // check top right and top left for solid block
      if (get_tile_at(player_new_pos_x, player_new_pos_y) != '.' ||
	  get_tile_at(player_new_pos_x + 0.9, player_new_pos_y) != '.' ) {

	player_new_pos_y = (int)player_new_pos_y + 1; // correct for collision
	player_vel_y = 0.0;
	
      }
    } else { // player moving down
      // check bottom left and bottom right for solid block
      if (get_tile_at(player_new_pos_x, player_new_pos_y + 1.0) != '.' ||
	  get_tile_at(player_new_pos_x + 0.9, player_new_pos_y + 1.0) != '.' ) {

	player_new_pos_y = (int)player_new_pos_y; // correct for collision
	player_vel_y = 0.0;

	player_on_ground = 1;
      }
    }

    // set player position to new position corrected for collisions
    player_pos_x = player_new_pos_x;
    player_pos_y = player_new_pos_y;

    // player can't go lower than 0
    if (player_pos_x < 0.0) player_pos_x = 0.0;
    if (player_pos_y < 0.0) player_pos_y = 0.0;

    // clamp player pos to map
    if (player_pos_x > (float)(level_width - 1)) player_pos_x = (float)(level_width - 1);
    if (player_pos_y > (float)(level_height - 1)) player_pos_y = (float)(level_height - 1);

    // update camera based on player
    camera_pos_x = player_pos_x;
    camera_pos_y = player_pos_y;

    // clamp camera into map
    if (camera_pos_x > level_width - visible_tiles_x / 2) camera_pos_x = level_width - visible_tiles_x / 2;
    if (camera_pos_y > level_height - visible_tiles_y / 2) camera_pos_y = level_height - visible_tiles_y / 2;

    // calculate top left most visible tile
    float offset_x = camera_pos_x - (float)visible_tiles_x / 2.0;
    float offset_y = camera_pos_y - (float)visible_tiles_y / 2.0;
    
    // clamp to 0
    if (offset_x < 0) offset_x = 0;
    if (offset_y < 0) offset_y = 0;
    // clamp to map
    if (offset_x >= level_width - visible_tiles_x) offset_x = level_width - visible_tiles_x;
    if (offset_y >= level_height - visible_tiles_y) offset_y = level_height - visible_tiles_y;    

    // get offset into tile for smooth movement
    float tile_offset_x = (offset_x - (int)offset_x) * tile_width;
    float tile_offset_y = (offset_y - (int)offset_y) * tile_height;

    int tile_x_is_offset = 0;
    if (tile_offset_x > 0) tile_x_is_offset = 1;
    int tile_y_is_offset = 0;
    if (tile_offset_y > 0) tile_y_is_offset = 1;

    SDL_Rect drect = {x:0, y:0, w:tile_width, h:tile_height};

    for (int x=0; x < visible_tiles_x + tile_x_is_offset; x++) {
      for (int y=0; y < visible_tiles_y + tile_y_is_offset; y++) {

	// start row of tile in h direction. could be negative
	// doesn't work
	int tile_start_x = x * tile_width - tile_offset_x;
	if (tile_start_x < 0) tile_start_x = 0;
	int tile_start_y = y * tile_height - tile_offset_y;
	if (tile_start_y < 0) tile_start_y = 0;
	int tile_end_x = (x+1) * tile_width - tile_offset_x;
	if (tile_end_x > screen_width) tile_end_x = screen_width;
	int tile_end_y = (y+1) * tile_height - tile_offset_y;
	if (tile_end_y > screen_height) tile_end_y = screen_height;
	
	/*	drect.x = x * tile_width;
	drect.y = y * tile_height;
	drect.w = tile_width;
	drect.h = tile_width; */
	
	drect.x = tile_start_x;
	drect.y = tile_start_y;
	drect.w = tile_end_x - tile_start_x;
	drect.h = tile_end_y - tile_start_y;
	
	char tile_id = get_tile_at(offset_x + x, offset_y + y);
	switch (tile_id) {
	case '.':
	  SDL_FillRect(virtualScreen, &drect, SDL_MapRGB(virtualScreen->format, 0x00, 0xFF, 0x00));
	  break;
	case '#':
	  SDL_FillRect(virtualScreen, &drect, SDL_MapRGB(virtualScreen->format, 0x00, 0x00, 0xFF));
	  break;
	default:
	  break;
	}
      }
    }

    // draw player
    drect.x = (player_pos_x - offset_x) * tile_width;
    drect.y = (player_pos_y - offset_y) * tile_height;
    drect.w = tile_width;
    drect.h = tile_height;
    //    SDL_FillRect(screen, &drect, SDL_MapRGB(screen->format, 0x00, 0xFF, 0xFF));
    if (player_looking_left) {
      SDL_BlitSurface(optimizedImage, NULL, virtualScreen, &drect);
    } else {
      SDL_BlitSurface(flippedImage, NULL, virtualScreen, &drect);
    }

    SDL_SoftStretch(virtualScreen, NULL, screen, NULL);

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
