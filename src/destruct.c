/*
 * OpenTyrian Classic: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* File notes:
 * Two players duke it out in a Scorched Earth style game.
 * Most of the variables referring to the players are global as
 * they are often edited and that's how the original was written.
 *
 * Right now almost all of the left/right code duplications are gone.  The
 * parts that aren't look noticeably messy but must wait until teams are added.
 * Most of the functions have been examined and tightened up, none of the enums
 * start with '1', and the various large functions have been divided into
 * smaller chunks.
 *
 * Things I have yet to do:
 * Add in extra 'configuration' variables.  They will be constants in the
 *  classic destruct but will be editable in the enhanced version.
 * Add in LEFT/RIGHT teams instead of players.  Added bonus: 4 man fights.
 * Rework the initial startup functions.
 * Strictly define unit sizes.  Having random +12s in the code is confusing.
 * Reorganize all these functions!
 *
 * Things I wanted to do but can't: Remove references to VGAScreen.  For
 * a multitude of reasons this just isn't feasable.  It would have been nice
 * to increase the playing field though...
 */

/*** Headers ***/
#include "opentyr.h"
#include "destruct.h"

#include "config.h"
#include "fonthand.h"
#include "helptext.h"
#include "keyboard.h"
#include "loudness.h"
#include "mtrand.h"
#include "newshape.h"
#include "nortsong.h"
#include "palette.h"
#include "picload.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"

#include <assert.h>

/*** Defines ***/
#define MAX_SHOTS 40
#define MAX_WALLS 20
#define MAX_EXPLO 40
#define MAX_KEY_OPTIONS 4
#define MAX_INSTALLATIONS 20

/*** Enums ***/
enum de_state_t { STATE_INIT, STATE_RELOAD, STATE_CONTINUE };
enum de_player_t { PLAYER_LEFT = 0, PLAYER_RIGHT = 1, MAX_PLAYERS = 2 };
enum de_team_t { TEAM_LEFT = 0, TEAM_RIGHT = 1, MAX_TEAMS = 2 };
enum de_mode_t { MODE_5CARDWAR = 0, MODE_TRADITIONAL, MODE_HELIASSAULT,
                 MODE_HELIDEFENSE, MODE_OUTGUNNED,
                 MODE_FIRST = MODE_5CARDWAR, MODE_LAST = MODE_OUTGUNNED,
                 MAX_MODES = 5, MODE_NONE = -1 };
enum de_unit_t { UNIT_TANK = 0, UNIT_NUKE, UNIT_DIRT, UNIT_SATELLITE,
                 UNIT_MAGNET, UNIT_LASER, UNIT_JUMPER, UNIT_HELI,
                 UNIT_FIRST = UNIT_TANK, UNIT_LAST = UNIT_HELI,
                 MAX_UNITS = 8 };
enum de_shot_t { SHOT_TRACER = 0, SHOT_SMALL, SHOT_LARGE, SHOT_MICRO,
                 SHOT_SUPER, SHOT_DEMO, SHOT_SMALLNUKE, SHOT_LARGENUKE,
                 SHOT_SMALLDIRT, SHOT_LARGEDIRT, SHOT_MAGNET, SHOT_MINILASER,
                 SHOT_MEGALASER, SHOT_LASERTRACER, SHOT_MEGABLAST, SHOT_MINI,
                 SHOT_BOMB,
                 SHOT_FIRST = SHOT_TRACER, SHOT_LAST = SHOT_BOMB,
                 MAX_SHOT_TYPES = 17, SHOT_INVALID = -1 };
enum de_expl_t { EXPL_NONE, EXPL_MAGNET, EXPL_DIRT, EXPL_NORMAL }; /* this needs a better name */
enum de_trails_t { TRAILS_NONE, TRAILS_NORMAL, TRAILS_FULL };
enum de_pixel_t { PIXEL_BLACK = 0, PIXEL_DIRT = 25 };
enum de_mapflags_t { MAP_NORMAL = 0x00, MAP_WALLS = 0x01, MAP_RINGS = 0x02,
					 MAP_HOLES = 0x04, MAP_FUZZY = 0x08, MAP_TALL = 0x10 };

/* keys and moves should line up. */
enum de_keys_t {  KEY_LEFT = 0,  KEY_RIGHT,  KEY_UP,  KEY_DOWN,  KEY_CHANGE,  KEY_FIRE,  KEY_CYUP,  KEY_CYDN,  MAX_KEY = 8};
enum de_move_t { MOVE_LEFT = 0, MOVE_RIGHT, MOVE_UP, MOVE_DOWN, MOVE_CHANGE, MOVE_FIRE, MOVE_CYUP, MOVE_CYDN, MAX_MOVE = 8};

/* The tracerlaser is dummied out.  It works but (probably due to the low
 * MAX_SHOTS) is not assigned to anything.  The bomb does not work.
 */


/*** Structs ***/
struct destruct_unit_s {

	/* Positioning/movement */
	unsigned int unitX; /* yep, one's an int and the other is a real */
	double       unitY;
	double       unitYMov;
	bool         isYInAir;

	/* What it is and what it fires */
	enum de_unit_t unitType;
	enum de_shot_t shotType;

	/* What it's pointed */
	double angle;
	double power;

	/* Misc */
	int lastMove;
	unsigned int ani_frame;
	int health;
};
struct destruct_shot_s {

	bool isAvailable;

	double x;
	double y;
	double xmov;
	double ymov;
	bool gravity;
	unsigned int shottype;
	//int shotdur; /* This looks to be unused */
	unsigned int trailx[4], traily[4], trailc[4];
};
struct destruct_explo_s {

	bool isAvailable;

	unsigned int x, y;
	unsigned int explowidth;
	unsigned int explomax;
	unsigned int explofill;
	enum de_expl_t exploType;
};
struct destruct_moves_s {
	bool actions[MAX_MOVE];
};
struct destruct_keys_s {
	SDLKey Config[MAX_KEY][MAX_KEY_OPTIONS];
};
struct destruct_ai_s {

	int c_Angle, c_Power, c_Fire;
	unsigned int c_noDown;
};
struct destruct_player_s {

	bool is_cpu;
	struct destruct_ai_s aiMemory;

	struct destruct_unit_s unit[MAX_INSTALLATIONS];
	struct destruct_moves_s moves;
	struct destruct_keys_s  keys;

	enum de_team_t team;
	unsigned int unitsRemaining;
	unsigned int unitSelected;
	unsigned int shotDelay;
	unsigned int score;
};
struct destruct_wall_s {

	bool wallExist;
	unsigned int wallX, wallY;
};
struct destruct_world_s {

	/* Map data & screen pointer */
	unsigned int baseMap[320];
	SDL_Surface * VGAScreen;
	struct destruct_wall_s mapWalls[MAX_WALLS];

	/* Map configuration */
	enum de_mode_t destructMode;
	unsigned int mapFlags;
};

/*** Function decs ***/
//Prep functions
void JE_destructMain( void );
void JE_introScreen( void );
enum de_mode_t JE_modeSelect( void );
void JE_helpScreen( void );
void JE_pauseScreen( void );

//level generating functions
void JE_generateTerrain( void );
void DE_generateBaseTerrain( unsigned int, unsigned int *);
void DE_drawBaseTerrain( unsigned int * );
void DE_generateUnits( unsigned int * );
void DE_generateWalls( struct destruct_world_s * );
void DE_generateRings(SDL_Surface *, Uint8 );
void DE_ResetLevel( void );
unsigned int JE_placementPosition( unsigned int, unsigned int, unsigned int * );

//drawing functions
void JE_aliasDirt( SDL_Surface * );
void DE_RunTickDrawCrosshairs( void );
void DE_RunTickDrawHUD( void );
void DE_GravityDrawUnit( enum de_player_t, struct destruct_unit_s * );
void DE_RunTickAnimate( void );
void DE_RunTickDrawWalls( void );
void DE_DrawTrails( struct destruct_shot_s *, unsigned int, unsigned int, unsigned int );
void JE_tempScreenChecking( void );
void JE_superPixel( unsigned int, unsigned int );
void JE_pixCool( unsigned int, unsigned int, Uint8 );

//player functions
void DE_RunTickGetInput( void );
void DE_ProcessInput( void );
void DE_ResetPlayers( void );
void DE_ResetAI( void );
void DE_ResetActions( void );
void DE_RunTickAI( void );

//unit functions
void DE_RaiseAngle( struct destruct_unit_s * );
void DE_LowerAngle( struct destruct_unit_s * );
void DE_RaisePower( struct destruct_unit_s * );
void DE_LowerPower( struct destruct_unit_s * );
void DE_CycleWeaponUp( struct destruct_unit_s * );
void DE_CycleWeaponDown( struct destruct_unit_s * );
void DE_RunMagnet( enum de_player_t, struct destruct_unit_s * );
void DE_GravityFlyUnit( struct destruct_unit_s * );
void DE_GravityLowerUnit( struct destruct_unit_s * );
void DE_DestroyUnit( enum de_player_t, struct destruct_unit_s * );
void DE_ResetUnits( void );
static inline bool DE_isValidUnit( struct destruct_unit_s *);

//weapon functions
void DE_ResetWeapons( void );
void DE_RunTickShots( void );
void DE_RunTickExplosions( void );
void DE_TestExplosionCollision( unsigned int, unsigned int);
void JE_makeExplosion( unsigned int, unsigned int, enum de_shot_t );
void DE_MakeShot( enum de_player_t, const struct destruct_unit_s *, int );

//gameplay functions
enum de_state_t DE_RunTick( void );
void DE_RunTickCycleDeadUnits( void );
void DE_RunTickGravity( void );
bool DE_RunTickCheckEndgame( void );
bool JE_stabilityCheck( unsigned int, unsigned int );

//sound
void DE_RunTickPlaySounds( void );
void JE_eSound( unsigned int );



/*** Weapon configurations ***/

/* Part of me wants to leave these as bytes to save space. */
const bool     demolish[MAX_SHOT_TYPES] = {false, false, false, false, false, true, true, true, false, false, false, false, true, false, true, false, true};
//const int        shotGr[MAX_SHOT_TYPES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 101};
const int     shotTrail[MAX_SHOT_TYPES] = {TRAILS_NONE, TRAILS_NONE, TRAILS_NONE, TRAILS_NORMAL, TRAILS_NORMAL, TRAILS_NORMAL, TRAILS_FULL, TRAILS_FULL, TRAILS_NONE, TRAILS_NONE, TRAILS_NONE, TRAILS_NORMAL, TRAILS_FULL, TRAILS_NORMAL, TRAILS_FULL, TRAILS_NORMAL, TRAILS_NONE};
//const int      shotFuse[MAX_SHOT_TYPES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
const int     shotDelay[MAX_SHOT_TYPES] = {10, 30, 80, 20, 60, 100, 140, 200, 20, 60, 5, 15, 50, 5, 80, 16, 0};
const int     shotSound[MAX_SHOT_TYPES] = {S_SELECT, S_WEAPON_2, S_WEAPON_1, S_WEAPON_7, S_WEAPON_7, S_EXPLOSION_9, S_EXPLOSION_22, S_EXPLOSION_22, S_WEAPON_5, S_WEAPON_13, S_WEAPON_10, S_WEAPON_15, S_WEAPON_15, S_WEAPON_26, S_WEAPON_14, S_WEAPON_7, S_WEAPON_7};
const int     exploSize[MAX_SHOT_TYPES] = {4, 20, 30, 14, 22, 16, 40, 60, 10, 30, 0, 5, 10, 3, 15, 7, 0};
const bool   shotBounce[MAX_SHOT_TYPES] = {false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false, true};
const int  exploDensity[MAX_SHOT_TYPES] = {  2,  5, 10, 15, 20, 15, 25, 30, 40, 80, 0, 30, 30,  4, 30, 5, 0};
const int      shotDirt[MAX_SHOT_TYPES] = {EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_DIRT, EXPL_DIRT, EXPL_MAGNET, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NORMAL, EXPL_NONE};
const int     shotColor[MAX_SHOT_TYPES] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 10, 10, 10, 10, 16, 0};

const int     defaultWeapon[MAX_UNITS] = {SHOT_SMALL, SHOT_MICRO,     SHOT_SMALLDIRT, SHOT_INVALID, SHOT_MAGNET, SHOT_MINILASER, SHOT_MICRO, SHOT_MINI};
const int  defaultCpuWeapon[MAX_UNITS] = {SHOT_SMALL, SHOT_MICRO,     SHOT_DEMO,      SHOT_INVALID, SHOT_MAGNET, SHOT_MINILASER, SHOT_MICRO, SHOT_MINI};
const int defaultCpuWeaponB[MAX_UNITS] = {SHOT_DEMO,  SHOT_SMALLNUKE, SHOT_DEMO,      SHOT_INVALID, SHOT_MAGNET, SHOT_MEGALASER, SHOT_MICRO, SHOT_MINI};
const int       systemAngle[MAX_UNITS] = {true, true, true, false, false, true, false, false};
const int        baseDamage[MAX_UNITS] = {200, 120, 400, 300, 80, 150, 600, 40};
const int         systemAni[MAX_UNITS] = {false, false, false, true, false, false, false, true};

const bool weaponSystems[MAX_UNITS][MAX_SHOT_TYPES] =
{
	{1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // normal
	{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // nuke
	{0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0}, // dirt
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // worthless
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}, // magnet
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0}, // laser
	{1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}, // jumper
	{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0}  // helicopter
};

/* More constant configuration settings. */
/* Music that destruct will play.  You can check out musmast.c to see what is what. */
const JE_byte goodsel[14] /*[1..14]*/ = {1, 2, 6, 12, 13, 14, 17, 23, 24, 26, 28, 29, 32, 33};

/* Unit creation.  Need to move this later: Doesn't belong here */
const JE_byte basetypes[8][11] /*[1..8, 1..11]*/ = /* [0] is amount of units*/
{
	{5, UNIT_TANK, UNIT_TANK, UNIT_NUKE, UNIT_DIRT,      UNIT_DIRT,   UNIT_SATELLITE, UNIT_MAGNET, UNIT_LASER,  UNIT_JUMPER, UNIT_HELI},   /*Normal*/
	{1, UNIT_TANK, UNIT_TANK, UNIT_TANK, UNIT_TANK,      UNIT_TANK,   UNIT_TANK,      UNIT_TANK,   UNIT_TANK,   UNIT_TANK,   UNIT_TANK},   /*Traditional*/
	{4, UNIT_HELI, UNIT_HELI, UNIT_HELI, UNIT_HELI,      UNIT_HELI,   UNIT_HELI,      UNIT_HELI,   UNIT_HELI,   UNIT_HELI,   UNIT_HELI},   /*Weak   Heli attack fleet*/
	{8, UNIT_TANK, UNIT_TANK, UNIT_TANK, UNIT_NUKE,      UNIT_NUKE,   UNIT_NUKE,      UNIT_DIRT,   UNIT_MAGNET, UNIT_LASER,  UNIT_JUMPER},   /*Strong Heli defense fleet*/
	{8, UNIT_HELI, UNIT_HELI, UNIT_HELI, UNIT_HELI,      UNIT_HELI,   UNIT_HELI,      UNIT_HELI,   UNIT_HELI,   UNIT_HELI,   UNIT_HELI},   /*Strong Heli attack fleet*/
	{4, UNIT_TANK, UNIT_TANK, UNIT_TANK, UNIT_TANK,      UNIT_NUKE,   UNIT_NUKE,      UNIT_DIRT,   UNIT_MAGNET, UNIT_JUMPER, UNIT_JUMPER},   /*Weak   Heli defense fleet*/
	{8, UNIT_TANK, UNIT_NUKE, UNIT_DIRT, UNIT_SATELLITE, UNIT_MAGNET, UNIT_LASER,     UNIT_JUMPER, UNIT_HELI,   UNIT_TANK,   UNIT_NUKE},   /*Overpowering fleet*/
	{4, UNIT_TANK, UNIT_TANK, UNIT_NUKE, UNIT_DIRT,      UNIT_TANK,   UNIT_LASER,     UNIT_JUMPER, UNIT_HELI,   UNIT_NUKE,   UNIT_JUMPER}    /*Weak fleet*/
};
const unsigned int baseLookup[MAX_PLAYERS][DESTRUCT_MODES] =
{
	{0, 1, 3, 4, 6},
	{0, 1, 2, 5, 7}
};


const JE_byte GraphicBase[MAX_PLAYERS][MAX_UNITS] =
{
	{  1,   6,  11,  58,  63,  68,  96, 153},
	{ 20,  25,  30,  77,  82,  87, 115, 172}
};

const JE_byte ModeScore[MAX_PLAYERS][DESTRUCT_MODES] =
{
	{1, 0, 0, 5, 0},
	{1, 0, 5, 0, 1}
};

const SDLKey defaultKeyConfig[MAX_PLAYERS][MAX_KEY][MAX_KEY_OPTIONS] =
{
	{	{SDLK_c},
		{SDLK_v},
		{SDLK_a},
		{SDLK_z},
		{SDLK_LALT},
		{SDLK_x, SDLK_LSHIFT},
		{SDLK_LCTRL},
		{SDLK_SPACE}
	},
	{	{SDLK_LEFT, SDLK_KP4},
		{SDLK_RIGHT, SDLK_KP6},
		{SDLK_UP, SDLK_KP8},
		{SDLK_DOWN, SDLK_KP2},
		{SDLK_BACKSLASH, SDLK_KP5},
		{SDLK_INSERT, SDLK_RETURN, SDLK_KP0, SDLK_KP_ENTER},
		{SDLK_PAGEUP, SDLK_KP9},
		{SDLK_PAGEDOWN, SDLK_KP3}
	}
};


/*** Globals ***/
SDL_Surface *destructTempScreen;
JE_boolean destructFirstTime;

struct destruct_player_s player[MAX_PLAYERS];
struct destruct_world_s  world;
struct destruct_shot_s   shotRec[MAX_SHOTS];
struct destruct_explo_s  exploRec[MAX_EXPLO];


void JE_destructGame( void )
{
	/* This is the entry function.  Any one-time actions we need to
	 * perform can go in here. */
	JE_clr256();
	JE_showVGA();

	destructTempScreen = game_screen;
	world.VGAScreen = VGAScreen;

	JE_loadCompShapes(&eShapes1, &eShapes1Size, '~');
	fade_black(1);

	JE_destructMain();
}

void JE_destructMain( void )
{
	enum de_state_t curState;


	JE_loadPic(11, false);
	JE_introScreen();

	DE_ResetPlayers();

	player[PLAYER_LEFT].is_cpu = true;
	//player[PLAYER_RIGHT].is_cpu = true; // this is fun :)

	while(1)
	{
		world.destructMode = JE_modeSelect();

		if(world.destructMode == MODE_NONE) {
			break; /* User is quitting */
		}

		do
		{

			destructFirstTime = true;
			JE_loadPic(11, false);

			DE_ResetUnits();
			DE_ResetLevel();
			do {
				curState = DE_RunTick();
			} while(curState == STATE_CONTINUE);

			fade_black(25);
		}
		while (curState == STATE_RELOAD);
	}
}

void JE_introScreen( void )
{
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->h * VGAScreen2->pitch);
	JE_outText(JE_fontCenter(specialName[7], TINY_FONT), 90, specialName[7], 12, 5);
	JE_outText(JE_fontCenter(miscText[64], TINY_FONT), 180, miscText[64], 15, 2);
	JE_outText(JE_fontCenter(miscText[65], TINY_FONT), 190, miscText[65], 15, 2);
	JE_showVGA();
	fade_palette(colors, 15, 0, 255);

	newkey = false;
	while (!newkey)
	{
		service_SDL_events(false);
		SDL_Delay(16);
	}

	fade_black(15);
	memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->h * VGAScreen->pitch);
	JE_showVGA();
}

/* JE_modeSelect
 *
 * This function prints the DESTRUCT mode selection menu.
 * The return value is the selected mode, or -1 (MODE_NONE)
 * if the user quits.
 */
enum de_mode_t JE_modeSelect( void )
{
	enum de_mode_t mode;
	unsigned int i;


	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->h * VGAScreen2->pitch);
	mode = MODE_5CARDWAR;

	/* Draw the menu once and fade us in. */
	for (i = 0; i < DESTRUCT_MODES; i++)
	{   /* What a large function call. */
		JE_textShade(JE_fontCenter(destructModeName[i], TINY_FONT), 82 + i * 12, destructModeName[i], 12, (i == mode) * 4, FULL_SHADE);
	}
	JE_showVGA();
	fade_palette(colors, 15, 0, 255);

	/* Get input in a loop. */
	while(1)
	{
		/* Re-draw the menu every iteration */
		for (i = 0; i < DESTRUCT_MODES; i++)
		{
			JE_textShade(JE_fontCenter(destructModeName[i], TINY_FONT), 82 + i * 12, destructModeName[i], 12, (i == mode) * 4, FULL_SHADE);
		}
		JE_showVGA();

		/* Grab keys */
		newkey = false;
		do {
			service_SDL_events(false);
			SDL_Delay(16);
		} while(!newkey);

		/* See what was pressed */
		if (keysactive[SDLK_ESCAPE])
		{
			mode = MODE_NONE; /* User is quitting, return failure */
			break;
		}
		if (keysactive[SDLK_RETURN])
		{
			break; /* User has selected, return choice */
		}
		if (keysactive[SDLK_UP])
		{
			if(mode == MODE_FIRST)
			{
				mode = MODE_LAST;
			} else {
				mode--;
			}
		}
		if (keysactive[SDLK_DOWN])
		{
			if(mode == MODE_LAST)
			{
				mode = MODE_FIRST;
			} else {
				mode++;
			}
		}
	}

	fade_black(15);
	memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->h * VGAScreen->pitch);
	JE_showVGA();
	return(mode);
}

void JE_generateTerrain( void )
{
	/* The unique modifiers:
	    Altered generation (really tall)
	    Fuzzy hills
	    Rings of dirt

	   The non-unique ones;:
	    Rings of not dirt (holes)
	    Walls
	*/

	world.mapFlags = MAP_NORMAL;

	if(mt_rand() % 2 == 0)
	{
		world.mapFlags |= MAP_WALLS;
	}
	if(mt_rand() % 4 == 0)
	{
		world.mapFlags |= MAP_HOLES;
	}
	switch(mt_rand() % 4)
	{
	case 0:
		world.mapFlags |= MAP_FUZZY;
		break;

	case 1:
		world.mapFlags |= MAP_TALL;
		break;

	case 2:
		world.mapFlags |= MAP_RINGS;
		break;
	}

	play_song(goodsel[mt_rand() % 14] - 1);

	DE_generateBaseTerrain(world.mapFlags, world.baseMap);
	DE_generateUnits(world.baseMap);
	DE_generateWalls(&world);
	DE_drawBaseTerrain(world.baseMap);

	if (world.mapFlags & MAP_RINGS)
	{
		DE_generateRings(world.VGAScreen, PIXEL_DIRT);
	}
	if (world.mapFlags & MAP_HOLES)
	{
		DE_generateRings(world.VGAScreen, PIXEL_BLACK);
	}

	JE_aliasDirt(world.VGAScreen);
	JE_showVGA();

	memcpy(destructTempScreen->pixels, VGAScreen->pixels, destructTempScreen->pitch * destructTempScreen->h);
}
void DE_generateBaseTerrain( unsigned int mapFlags, unsigned int * baseWorld)
{
	unsigned int i;
	unsigned int newheight, HeightMul;
	double sinewave, sinewave2, cosinewave, cosinewave2;


	/* The 'terrain' is actually the video buffer :).  If it's brown, flu... er,
	 * brown pixels are what we check for collisions with. */

	/* The ranges here are between .01 and roughly 0.07283...*/
	sinewave    = (mt_rand_lt1()) * M_PI / 50.0 + 0.01;
	sinewave2   = (mt_rand_lt1()) * M_PI / 50.0 + 0.01;
	cosinewave  = (mt_rand_lt1()) * M_PI / 50.0 + 0.01;
	cosinewave2 = (mt_rand_lt1()) * M_PI / 50.0 + 0.01;
	HeightMul = 20;

	/* This block just exists to mix things up. */
	if(mapFlags & MAP_FUZZY)
	{
		sinewave  = M_PI - mt_rand_lt1() * 0.3f;
		sinewave2 = M_PI - mt_rand_lt1() * 0.3f;
	}
	if(mapFlags & MAP_TALL)
	{
		HeightMul = 100;
	}

	/* Now compute a height for each of our lines. */
	for (i = 1; i <= 318; i++)
	{
		newheight = round(sin(sinewave   * i) * HeightMul + sin(sinewave2   * i) * 15.0 +
		                  cos(cosinewave * i) * 10.0      + sin(cosinewave2 * i) * 15.0) + 130;

		/* Bind it; we have mins and maxs */
		if (newheight < 40)
		{
			newheight = 40;
		}
		else if (newheight > 195) {
			newheight = 195;
		}
		baseWorld[i] = newheight;
	}
	/* The base world has been created. */
}
void DE_drawBaseTerrain( unsigned int * baseWorld)
{
	unsigned int i;


	for (i = 1; i <= 318; i++)
	{
		JE_rectangle(i, baseWorld[i], i, 199, PIXEL_DIRT);
	}
}

void DE_generateUnits( unsigned int * baseWorld )
{
	unsigned int i, j, numSatellites;


	for (i = 0; i < MAX_PLAYERS; i++)
	{
		numSatellites = 0;
		player[i].unitsRemaining = 0;

		for (j = 0; j < basetypes[baseLookup[i][world.destructMode]][0]; j++)
		{
			/* Not everything is the same between players */
			if(i == PLAYER_LEFT)
			{
				player[i].unit[j].unitX = (mt_rand() % 120) + 10;
			}
			else
			{
				player[i].unit[j].unitX = 320 - ((mt_rand() % 120) + 22);
			}

			player[i].unit[j].unitY = JE_placementPosition(player[i].unit[j].unitX - 1, 14, baseWorld);
			player[i].unit[j].unitType = basetypes[baseLookup[i][world.destructMode]][(mt_rand() % 10) + 1];

			/* Sats are special cases since they are useless.  They don't count
			 * as active units and we can't have a team of all sats */
			if (player[i].unit[j].unitType == UNIT_SATELLITE)
			{
				if (numSatellites == basetypes[baseLookup[i][world.destructMode]][0])
				{
					player[i].unit[j].unitType = UNIT_TANK;
					player[i].unitsRemaining++;
				} else {
					/* Place the satellite. Note: Earlier we cleared
					 * space with JE_placementPosition.  Now we are randomly
					 * placing the sat's Y.  It can be generated in hills
					 * and there is a clearing underneath it.  This CAN
					 * be fixed but won't be for classic.
					 */
					player[i].unit[j].unitY = 30 + (mt_rand() % 40);
					numSatellites++;
				}
			}
			else
			{
				player[i].unitsRemaining++;
			}

			/* Now just fill in the rest of the unit's values. */
			player[i].unit[j].lastMove = 0;
			player[i].unit[j].unitYMov = 0;
			player[i].unit[j].isYInAir = false;
			player[i].unit[j].angle = 0;
			player[i].unit[j].power = (player[i].unit[j].unitType == UNIT_LASER) ? 6 : 3;
			player[i].unit[j].shotType = defaultWeapon[player[i].unit[j].unitType];
			player[i].unit[j].health = baseDamage[player[i].unit[j].unitType];
			player[i].unit[j].ani_frame = 0;
		}
	}
}
void DE_generateWalls( struct destruct_world_s * gameWorld )
{
	unsigned int i, j, wallX;
	unsigned int wallHeight, remainWalls;
	unsigned int tries;
	bool isGood;


	if ((world.mapFlags & MAP_WALLS) == false)
	{
		/* Just clear them out */
		for (i = 0; i < MAX_WALLS; i++)
		{
			gameWorld->mapWalls[i].wallExist = false;
		}
		return;
	}

	remainWalls = MAX_WALLS;

	do {

		/* Create a wall.  Decide how tall the wall will be */
		wallHeight = (mt_rand() % 5) + 1;
		if(wallHeight > remainWalls)
		{
			wallHeight = remainWalls;
		}

		/* Now find a good place to put the wall. */
		tries = 0;
		do {

			isGood = true;
			wallX = (mt_rand() % 300) + 10;

			/* Is this X already occupied?  In the original Tyrian we only
			 * checked to make sure four units on each side were unobscured.
			 * That's not very scalable; instead I will check every unit,
			 * but I'll only try plotting an unobstructed X four times.
			 * After that we'll cover up what may; having a few units
			 * stuck behind walls makes things mildly interesting.
			 */
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				for (j = 0; j < MAX_INSTALLATIONS; j++)
				{
					if ((wallX > player[i].unit[j].unitX - 12)
					 && (wallX < player[i].unit[j].unitX + 13))
					{
						isGood = false;
						goto label_outer_break; /* I do feel that outer breaking is a legitimate goto use. */
					}
				}
			}

label_outer_break:
			tries++;

		} while(isGood == false && tries < 5);


		/* We now have a valid X.  Create the wall. */
		for (i = 1; i <= wallHeight; i++)
		{
			gameWorld->mapWalls[remainWalls - i].wallExist = true;
			gameWorld->mapWalls[remainWalls - i].wallX = wallX;
			gameWorld->mapWalls[remainWalls - i].wallY = JE_placementPosition(wallX, 12, gameWorld->baseMap) - 14 * i;
		}

		remainWalls -= wallHeight;

	} while (remainWalls != 0);
}

void DE_generateRings( SDL_Surface * screen, Uint8 pixel )
{
	unsigned int i, j, tempSize, rings;
	int tempPosX1, tempPosY1, tempPosX2, tempPosY2;
	double tempRadian;


	rings = mt_rand() % 6 + 1;
	for (i = 1; i <= rings; i++)
	{
		tempPosX1 = (mt_rand() % 320);
		tempPosY1 = (mt_rand() % 160) + 20;
		tempSize = (mt_rand() % 40) + 10;  /*Size*/

		for (j = 1; j <= tempSize * tempSize * 2; j++)
		{
			tempRadian = mt_rand_lt1() * (M_PI * 2);
			tempPosY2 = tempPosY1 + round(cos(tempRadian) * (mt_rand_lt1() * 0.1f + 0.9f) * tempSize);
			tempPosX2 = tempPosX1 + round(sin(tempRadian) * (mt_rand_lt1() * 0.1f + 0.9f) * tempSize);
			if ((tempPosY2 > 12) && (tempPosY2 < 200)
			 && (tempPosX2 > 0) && (tempPosX2 < 319))
			{
				((Uint8 *)screen->pixels)[tempPosX2 + tempPosY2 * screen->pitch] = pixel;
			}
		}
	}
}

void JE_aliasDirt( SDL_Surface * screen )
{
	/* This complicated looking function goes through the whole screen
	 * looking for brown pixels which just happen to be next to non-brown
	 * pixels.  It's an aliaser, just like it says. */
	unsigned int x, y, newColor;


	/* This is a pointer to a screen.  If you don't like pointer arithmetic,
	 * you won't like this function. */
	Uint8 *s = screen->pixels;
	s += 12 * screen->pitch;

	for (y = 12; y < screen->h; y++)
	{
		for (x = 0; x < screen->pitch; x++)
		{
			if (*s == PIXEL_BLACK)
			{
				newColor = PIXEL_BLACK;
				if (*(s - screen->pitch) == PIXEL_DIRT) /* look up */
				{
					newColor += 1;
				}
				if (y < screen->h - 1)
				{
					if (*(s + screen->pitch) == PIXEL_DIRT) /* look down */
					{
						newColor += 3;
					}
				}
				if (x > 0)
				{
					if (*(s - 1) == PIXEL_DIRT) /* look left */
					{
						newColor += 2;
					}
				}
				if (x < screen->pitch - 1)
				{
					if (*(s + 1) == PIXEL_DIRT) /* look right */
					{
						newColor += 2;
					}
				}
				if (newColor)
				{
					*s = newColor + 16; /* 16 must be the start of the brown pixels. */
				}
			}

			s++;
		}
	}
}

unsigned int JE_placementPosition( unsigned int passed_x, unsigned int width, unsigned int * world )
{
	unsigned int i, new_y;


	/* This is the function responsible for carving out chunks of land.
	 * There's a bug here, but it's a pretty major gameplay altering one:
	 * areas can be carved out for units that are aerial or in mountains.
	 * This can result in huge caverns.  Ergo, it's a feature :) */
	new_y = 0;
	for (i = passed_x; i <= passed_x + width - 1; i++)
	{
		if (new_y < world[i])
		{
			new_y = world[i];
		}
	}

	for (i = passed_x; i <= passed_x + width - 1; i++)
	{
		world[i] = new_y;
	}

	return new_y;
}

bool JE_stabilityCheck( unsigned int x, unsigned int y )
{
	unsigned int i, numDirtPixels;
	Uint8 * s;


	numDirtPixels = 0;
	s = destructTempScreen->pixels;
	s += x + (y * destructTempScreen->pitch) - 1;

	/* Check the 12 pixels on the bottom border of our object */
	for (i = 0; i < 12; i++)
	{
		if (*s == PIXEL_DIRT)
		{
			numDirtPixels++;
		}
		s++;
	}

	/* If there are fewer than 10 brown pixels we don't consider it a solid base */
	return (numDirtPixels < 10);
}

void JE_tempScreenChecking( void ) /*and copy to vgascreen*/
{
	Uint8 *s = VGAScreen->pixels;
	s += 12 * VGAScreen->pitch;

	Uint8 *temps = destructTempScreen->pixels;
	temps += 12 * destructTempScreen->pitch;

	for (int y = 12; y < VGAScreen->h; y++)
	{
		for (int x = 0; x < VGAScreen->pitch; x++)
		{
			/* This block is what fades out explosions. The palette from 241
			 * to 255 fades from a very dark red to a very bright yellow. */
			if (*temps >= 241)
			{
				if (*temps == 241)
				{
					*temps = PIXEL_BLACK;
				}
				else
				{
					(*temps)--;
				}
			}

			/* This is copying from our temp screen to VGAScreen */
			*s = *temps;

			s++;
			temps++;
		}
	}
}

void JE_makeExplosion( unsigned int tempPosX, unsigned int tempPosY, enum de_shot_t shottype )
{
	unsigned int i, tempExploSize;


	/* First find an open explosion. If we can't find one, return.*/
	for (i = 0; i < MAX_EXPLO; i++)
	{
		if (exploRec[i].isAvailable == true)
		{
			break;
		}
	}
	if(i == MAX_EXPLO) /* No empty slots */
	{
		return;
	}


	exploRec[i].isAvailable = false;
	exploRec[i].x = tempPosX;
	exploRec[i].y = tempPosY;
	exploRec[i].explowidth = 2;

	if(shottype != SHOT_INVALID)
	{
		tempExploSize = exploSize[shottype];
		if (tempExploSize < 5)
		{
			JE_eSound(3);
		}
		else if (tempExploSize < 15)
		{
			JE_eSound(4);
		}
		else if (tempExploSize < 20)
		{
			JE_eSound(12);
		}
		else if (tempExploSize < 40)
		{
			JE_eSound(11);
		}
		else {
			JE_eSound(12);
			JE_eSound(11);
		}

		exploRec[i].explomax  = tempExploSize;
		exploRec[i].explofill = exploDensity[shottype];
		exploRec[i].exploType = shotDirt[shottype];
	}
	else
	{
		JE_eSound(4);
		exploRec[i].explomax  = (mt_rand() % 40) + 10;
		exploRec[i].explofill = (mt_rand() % 60) + 20;
		exploRec[i].exploType = EXPL_NORMAL;
	}
}

void JE_eSound( unsigned int sound )
{
	static int exploSoundChannel = 0;

	if (++exploSoundChannel > 5)
	{
		exploSoundChannel = 1;
	}

	soundQueue[exploSoundChannel] = sound;
}

void JE_superPixel( unsigned int tempPosX, unsigned int tempPosY )
{
	const unsigned int starPattern[5][5] = {
		{   0,   0, 246,   0,   0 },
		{   0, 247, 249, 247,   0 },
		{ 246, 249, 252, 249, 246 },
		{   0, 247, 249, 247,   0 },
		{   0,   0, 246,   0,   0 }
	};
	const unsigned int starIntensity[5][5] = {
		{   0,   0,   1,   0,   0 },
		{   0,   1,   2,   1,   0 },
		{   1,   2,   4,   2,   1 },
		{   0,   1,   2,   1,   0 },
		{   0,   0,   1,   0,   0 }
	};

	int x, y, maxX, maxY;
	unsigned int rowLen;
	Uint8 *s;


	maxX = destructTempScreen->pitch;
	maxY = destructTempScreen->h;

	rowLen = destructTempScreen->pitch;
	s = destructTempScreen->pixels;
	s += (rowLen * (tempPosY - 2)) + (tempPosX - 2);

	for (y = 0; y < 5; y++, s += rowLen - 5)
	{
		if ((signed)tempPosY + y - 2 < 0     /* would be out of bounds */
		||  (signed)tempPosY + y - 2 >= maxY) { continue; }

		for (x = 0; x < 5; x++, s++)
		{
			if ((signed)tempPosX + x - 2 < 0
			 || (signed)tempPosX + x - 2 >= maxX) { continue; }

			if (starPattern[y][x] == 0) { continue; } /* this is just to speed it up */

			/* at this point *s is our pixel.  Our constant arrays tell us what
			 * to do with it. */
			if (*s < starPattern[y][x])
			{
				*s = starPattern[y][x];
			}
			else if (*s + starIntensity[y][x] > 255)
			{
				*s = 255;
			}
			else
			{
				*s += starIntensity[y][x];
			}
		}
	}
}

void JE_helpScreen( void )
{
	unsigned int i, j;


	//JE_getVGA();  didn't do anything anyway?
	fade_black(15);
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->h * VGAScreen2->pitch);
	JE_clr256();

	for(i = 0; i < 2; i++)
	{
		JE_outText(100,  5 + i * 90, destructHelp[i * 12 + 0], 2, 4);
		JE_outText(100, 15 + i * 90, destructHelp[i * 12 + 1], 2, 1);
		for (j = 3; j <= 12; j++)
		{
			JE_outText(((j - 1) % 2) * 160 + 10, 15 + ((j - 1) / 2) * 12 + i * 90, destructHelp[i * 12 + j-1], 1, 3);
		}
	}
	JE_outText(30, 190, destructHelp[24], 3, 4);
	JE_showVGA();
	fade_palette(colors, 15, 0, 255);

	do {
		service_SDL_events(true);
		SDL_Delay(16);
	} while (!newkey);

	fade_black(15);
	memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->h * VGAScreen->pitch);
	JE_showVGA();
	fade_palette(colors, 15, 0, 255);
}


void JE_pauseScreen( void )
{
	set_volume(tyrMusicVolume / 2, fxVolume);

	/* Save our current screen/game world.  We don't want to screw it up while paused. */
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->h * VGAScreen2->pitch);
	JE_outText(JE_fontCenter(miscText[22], TINY_FONT), 90, miscText[22], 12, 5);
	JE_showVGA();

	do { /* just wait until the user hits a key */
		service_SDL_events(true);
		SDL_Delay(16);
	} while (!newkey);

	/* Restore current screen & volume*/
	memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->h * VGAScreen->pitch);
	JE_showVGA();

	set_volume(tyrMusicVolume, fxVolume);
}

/* DE_ResetX
 *
 * The reset functions clear the state of whatefer they are assigned to.
 */
void DE_ResetUnits( void )
{
	unsigned int p, u;


	for (p = 0; p < MAX_PLAYERS; ++p)
	{
		for (u = 0; u < MAX_INSTALLATIONS; ++u)
		{
			player[p].unit[u].health = 0;
		}
	}
}
void DE_ResetPlayers( void )
{
	unsigned int i;


	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		player[i].is_cpu = false;
		player[i].unitSelected = 0;
		player[i].shotDelay = 0;
		player[i].score = 0;
		player[i].aiMemory.c_Angle = 0;
		player[i].aiMemory.c_Power = 0;
		player[i].aiMemory.c_Fire = 0;
		player[i].aiMemory.c_noDown = 0;
		memcpy(player[i].keys.Config, defaultKeyConfig[i], sizeof(player[i].keys.Config));
	}
}
void DE_ResetWeapons( void )
{
	unsigned int i;


	for (i = 0; i < MAX_SHOTS; i++)
	{
		shotRec[i].isAvailable = true;
	}
	for (i = 0; i < MAX_EXPLO; i++)
	{
		exploRec[i].isAvailable = true;
	}
}
void DE_ResetLevel( void )
{
	/* Okay, let's prep the arena */

	DE_ResetWeapons();

	JE_generateTerrain();
	DE_ResetAI();
}
void DE_ResetAI( void )
{
	unsigned int i, j;
	struct destruct_unit_s * ptr;


	for (i = PLAYER_LEFT; i < MAX_PLAYERS; i++)
	{
		if (player[i].is_cpu == false) { continue; }
		ptr = player[i].unit;

		for( j = 0; j < MAX_INSTALLATIONS; j++, ptr++)
		{
			if(DE_isValidUnit(ptr) == false) { continue; }

			if (systemAngle[ptr->unitType] || ptr->unitType == UNIT_HELI)
			{
				ptr->angle = M_PI / 4.0;
			} else {
				ptr->angle = 0;
			}
			ptr->power = (ptr->unitType == UNIT_LASER) ? 6 : 4;

			if (world.mapFlags & MAP_WALLS)
			{
				ptr->shotType = defaultCpuWeaponB[ptr->unitType];
			} else {
				ptr->shotType = defaultCpuWeapon[ptr->unitType];
			}
		}
	}
}
void DE_ResetActions( void )
{
	unsigned int i;


	for(i = 0; i < MAX_PLAYERS; i++)
	{	/* Zero it all.  A memset would do the trick */
		memset(&(player[i].moves), 0, sizeof(player[i].moves));
	}
}
/* DE_RunTick
 *
 * Runs one tick.  One tick involves handling physics, drawing crap,
 * moving projectiles and explosions, and getting input.
 * Returns true while the game is running or false if the game is
 * to be terminated.
 */
enum de_state_t DE_RunTick( void )
{
	static unsigned int endDelay;


	setjasondelay(1);

	memset(soundQueue, 0, sizeof(soundQueue));
	JE_tempScreenChecking();

	DE_ResetActions();
	DE_RunTickCycleDeadUnits();


	DE_RunTickGravity();
	DE_RunTickAnimate();
	DE_RunTickDrawWalls();
	DE_RunTickExplosions();
	DE_RunTickShots();
	DE_RunTickAI();
	DE_RunTickDrawCrosshairs();
	DE_RunTickDrawHUD();
	JE_showVGA();

	if (destructFirstTime)
	{
		fade_palette(colors, 25, 0, 255);
		destructFirstTime = false;
		endDelay = 0;
	}

	DE_RunTickGetInput();
	DE_ProcessInput();

	if (endDelay > 0)
	{
		if(--endDelay == 0)
		{
			return(STATE_RELOAD);
		}
	}
	else if ( DE_RunTickCheckEndgame() == true)
	{
		endDelay = 80;
	}

	DE_RunTickPlaySounds();

	/* The rest of this cruft needs to be put in appropriate sections */
	if (keysactive[SDLK_F10])
	{
		player[PLAYER_LEFT].is_cpu = !player[PLAYER_LEFT].is_cpu;
		keysactive[SDLK_F10] = false;
	}

	if (keysactive[SDLK_p])
	{
		JE_pauseScreen();
		keysactive[lastkey_sym] = false;
	}

	if (keysactive[SDLK_F1])
	{
		JE_helpScreen();
		keysactive[lastkey_sym] = false;
	}

	wait_delay();

	if (keysactive[SDLK_ESCAPE])
	{
		keysactive[SDLK_ESCAPE] = false;
		return(STATE_INIT); /* STATE_INIT drops us to the mode select */
	}

	if (keysactive[SDLK_BACKSPACE])
	{
		keysactive[SDLK_BACKSPACE] = false;
		return(STATE_RELOAD); /* STATE_RELOAD creates a new map */
	}

	return(STATE_CONTINUE);
}

/* DE_RunTickX
 *
 * Handles something that we do once per tick, such as
 * track ammo and move asplosions.
 */
void DE_RunTickCycleDeadUnits( void )
{
	unsigned int i;
	struct destruct_unit_s * unit;


	/* This code automatically switches the active unit if it is destroyed
	 * and skips over the useless satellite */
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (player[i].unitsRemaining == 0) { continue; }

		unit = &(player[i].unit[player[i].unitSelected]);
		while(DE_isValidUnit(unit) == false
		   || unit->shotType == SHOT_INVALID)
		{
			player[i].unitSelected++;
			unit++;
			if (player[i].unitSelected >= MAX_INSTALLATIONS)
			{
				player[i].unitSelected = 0;
				unit = player[i].unit;
			}
		}
	}
}
void DE_RunTickGravity( void )
{
	unsigned int i, j;
	struct destruct_unit_s * unit;


	for (i = 0; i < MAX_PLAYERS; i++)
	{

		unit = player[i].unit;
		for (j = 0; j < MAX_INSTALLATIONS; j++, unit++)
		{
			if (DE_isValidUnit(unit) == false) { continue; } /* invalid unit */

			switch(unit->unitType)
			{
				case UNIT_SATELLITE: /* satellites don't fall down */
					break;

				case UNIT_HELI:
				case UNIT_JUMPER:
					if (unit->isYInAir == true) /* unit is falling down, at least in theory */
					{
						DE_GravityFlyUnit(unit);
						break;
					}
					/* else fall through and treat as a normal unit */

				default:
					DE_GravityLowerUnit(unit);
			}

		/* Draw the unit. */
		DE_GravityDrawUnit(i, unit);
		}
	}
}
void DE_GravityDrawUnit( enum de_player_t team, struct destruct_unit_s * unit )
{
	unsigned int anim_index;


	anim_index = GraphicBase[team][unit->unitType] + unit->ani_frame;
	if (unit->unitType == UNIT_HELI)
	{
		/* Adjust animation index if we are travelling right or left. */
		if (unit->lastMove < -2)
		{
			anim_index += 5;
		} else if (unit->lastMove > 2) {
			anim_index += 10;
		}
	} else { /* This handles our cannons and the like */
		anim_index += floor(unit->angle * 9.99 / M_PI);
	}

	JE_drawShape2(unit->unitX, round(unit->unitY) - 13, anim_index, eShapes1);
}
void DE_GravityLowerUnit( struct destruct_unit_s * unit )
{
	/* units fall at a constant speed.  The heli is an odd case though;
	 * we simply give it a downward velocity, but due to a buggy implementation
	 * th chopper didn't lower until you tried to fly it up.  Tyrian 2000 fixes
	 * this by not making the chopper a special case.  I've decided to actually
	 * mix both; the chopper is given a slight downward acceleration (simulating
	 * a 'rocky' takeoff), and it is lowered like a regular unit, but not as
	 * quickly.
	 */
	if(unit->unitY < 199) { /* checking takes time, don't check if it's at the bottom */
		if (JE_stabilityCheck(unit->unitX, round(unit->unitY)))
		{
			switch(unit->unitType)
			{
				case UNIT_HELI:
					unit->unitYMov = 1.5;
					unit->unitY += 0.2;
					break;

				default:
					unit->unitY += 1;
			}

			if (unit->unitY > 199) /* could be possible */
			{
				unit->unitY = 199;
			}
		}
	}
}
void DE_GravityFlyUnit( struct destruct_unit_s * unit )
{
	if (unit->unitY + unit->unitYMov > 199) /* would hit bottom of screen */
	{
		unit->unitY = 199;
		unit->unitYMov = 0;
		unit->isYInAir = false;
		return;
	}

	/* move the unit and alter acceleration */
	unit->unitY += unit->unitYMov;
	if (unit->unitY < 24) /* This stops units from going above the screen */
	{
		unit->unitYMov = 0;
		unit->unitY = 24;
	}
	if (unit->unitType == UNIT_HELI) /* helicopters fall more slowly */
	{
		unit->unitYMov += 0.0001;
	} else {
		unit->unitYMov += 0.03;
	}
	if (!JE_stabilityCheck(unit->unitX, round(unit->unitY)))
	{
		unit->unitYMov = 0;
		unit->isYInAir = false;
	}
}
void DE_RunTickAnimate( void )
{
	unsigned int p, u;
	struct destruct_unit_s * ptr;


	for (p = 0; p < MAX_PLAYERS; ++p)
	{
		ptr = player[p].unit;
		for (u = 0; u < MAX_INSTALLATIONS; ++u,  ++ptr)
		{
			/* Don't mess with any unit that is unallocated
			 * or doesn't animate and is set to frame 0 */
			if(DE_isValidUnit(ptr) == false) { continue; }
			if(systemAni[ptr->unitType] == false && ptr->ani_frame == 0) { continue; }

			if (++(ptr->ani_frame) > 3)
			{
				ptr->ani_frame = 0;
			}
		}
	}
}
void DE_RunTickDrawWalls( void )
{
	unsigned int i;


	for (i = 0; i < MAX_WALLS; i++)
	{
		if (world.mapWalls[i].wallExist)
		{
			JE_drawShape2(world.mapWalls[i].wallX, world.mapWalls[i].wallY, 42, eShapes1);
		}
	}
}
void DE_RunTickExplosions( void )
{
	unsigned int i, j;
	int tempPosX, tempPosY;
	double tempRadian;


	/* Run through all open explosions.  They are not sorted in any way */
	for (i = 0; i < MAX_EXPLO; i++)
	{
		if (exploRec[i].isAvailable == true) { continue; } /* Nothing to do */

		for (j = 0; j < exploRec[i].explofill; j++)
		{
			/* An explosion is comprised of multiple 'flares' that fan out.
			   Calculate where this 'flare' will end up */
			tempRadian = mt_rand_lt1() * M_PI * 2;
			tempPosY = exploRec[i].y + round(cos(tempRadian) * mt_rand_lt1() * exploRec[i].explowidth);
			tempPosX = exploRec[i].x + round(sin(tempRadian) * mt_rand_lt1() * exploRec[i].explowidth);

			/* Our game allows explosions to wrap around.  This looks to have
			 * originally been a bug that was left in as being fun, but we are
			 * going to replicate it w/o risking out of bound arrays. */

			while(tempPosX < 0)   { tempPosX += 320; }
			while(tempPosX > 320) { tempPosX -= 320; }

			/* We don't draw our explosion if it's out of bounds vertically */
			if (tempPosY >= 200 || tempPosY <= 15) { continue; }

			/* And now the drawing.  There are only two types of explosions
			 * right now; dirt and flares.  Dirt simply draws a brown pixel;
			 * flares explode and have a star formation. */
			switch(exploRec[i].exploType)
			{
				case EXPL_DIRT:
					((Uint8 *)destructTempScreen->pixels)[tempPosX + tempPosY * destructTempScreen->pitch] = PIXEL_DIRT;
					break;

				case EXPL_NORMAL:
					JE_superPixel(tempPosX, tempPosY);
					DE_TestExplosionCollision(tempPosX, tempPosY);
					break;

				default:
					assert(false);
					break;
			}
		}

		/* Widen the explosion and delete it if necessary. */
		exploRec[i].explowidth++;
		if (exploRec[i].explowidth == exploRec[i].explomax)
		{
			exploRec[i].isAvailable = true;
		}
	}
}
void DE_TestExplosionCollision( unsigned int PosX, unsigned int PosY)
{
	unsigned int i, j;
	struct destruct_unit_s * unit;


	for (i = PLAYER_LEFT; i < MAX_PLAYERS; i++)
	{
		unit = player[i].unit;
		for (j = 0; j < MAX_INSTALLATIONS; j++, unit++)
		{
			if (DE_isValidUnit(unit) == true
			 && PosX > unit->unitX && PosX < unit->unitX + 11
		 	 && PosY < unit->unitY && PosY > unit->unitY - 11)
			{
				unit->health--;
				if (unit->health <= 0)
				{
					DE_DestroyUnit(i, unit);
				}
			}
		}
	}
}
void DE_DestroyUnit( enum de_player_t playerID, struct destruct_unit_s * unit )
{
	/* This function call was an evil evil piece of brilliance before.  Go on.
	 * Look at the older revisions.  It passed the result of a comparison.
	 * MULTIPLIED.  This is at least a little clearer... */
	JE_makeExplosion(unit->unitX + 5, round(unit->unitY) - 5, (unit->unitType == UNIT_HELI) ? SHOT_SMALL : SHOT_INVALID); /* Helicopters explode like small shots do.  Invalids are their own special case. */

	if (unit->unitType != UNIT_SATELLITE) /* increment score */
	{ /* todo: change when teams are created. Hacky kludge for now.*/
		player[playerID].unitsRemaining--;
		player[((playerID == PLAYER_LEFT) ? PLAYER_RIGHT : PLAYER_LEFT)].score++;
	}
}

void DE_RunTickShots( void )
{
	unsigned int i, j, k;
	unsigned int tempTrails;
	unsigned int tempPosX, tempPosY;
	struct destruct_unit_s * unit;


	for (i = 0; i < MAX_SHOTS; i++)
	{
		if (shotRec[i].isAvailable == true) { continue; } /* Nothing to do */

		/* Move the shot.  Simple displacement */
		shotRec[i].x += shotRec[i].xmov;
		shotRec[i].y += shotRec[i].ymov;

		/* If the shot can bounce off the map, bounce it */
		if (shotBounce[shotRec[i].shottype])
		{
			if (shotRec[i].y > 199 || shotRec[i].y < 14)
			{
				shotRec[i].y -= shotRec[i].ymov;
				shotRec[i].ymov = -shotRec[i].ymov;
			}
			if (shotRec[i].x < 1 || shotRec[i].x > 318)
			{
				shotRec[i].x -= shotRec[i].xmov;
				shotRec[i].xmov = -shotRec[i].xmov;
			}
		} else { /* If it cannot, apply normal physics */
			shotRec[i].ymov += 0.05f; /* add gravity */

			if (shotRec[i].y > 199) /* We hit the floor */
			{
				shotRec[i].y -= shotRec[i].ymov;
				shotRec[i].ymov = -shotRec[i].ymov * 0.8f; /* bounce at reduced velocity */

				/* Don't allow a bouncing shot to bounce straight up and down */
				if (shotRec[i].xmov == 0)
				{
					shotRec[i].xmov += mt_rand_lt1() - 0.5f;
				}
			}
		}

		/* Shot has gone out of bounds. Eliminate it. */
		if (shotRec[i].x > 318 || shotRec[i].x < 1)
		{
			shotRec[i].isAvailable = true;
			continue;
		}

		/* Now check for collisions. */
		if (shotRec[i].y <= 14)
		{
			/* Don't bother checking for collisions above the map :) */
			continue;
		}

		tempPosX = round(shotRec[i].x);
		tempPosY = round(shotRec[i].y);

		/*Check building hits*/
		for(j = 0; j < MAX_PLAYERS; j++)
		{
			unit = player[j].unit;
			for(k = 0; k < MAX_INSTALLATIONS; k++, unit++)
			{
				if(DE_isValidUnit(unit) == false) { continue; }

				if (tempPosX > unit->unitX && tempPosX < unit->unitX + 11
				 && tempPosY < unit->unitY && tempPosY > unit->unitY - 13)
				{
					shotRec[i].isAvailable = true;
					JE_makeExplosion(tempPosX, tempPosY, shotRec[i].shottype);
				}
			}
		}

		tempTrails = (shotColor[shotRec[i].shottype] << 4) - 3;
		JE_pixCool(tempPosX, tempPosY, tempTrails);

		/*Draw the shot trail (if applicable) */
		switch (shotTrail[shotRec[i].shottype])
		{
		case TRAILS_NONE:
			break;
		case TRAILS_NORMAL:
			DE_DrawTrails( &(shotRec[i]), 2, 4, tempTrails - 3 );
			break;
		case TRAILS_FULL:
			DE_DrawTrails( &(shotRec[i]), 4, 3, tempTrails - 1 );
			break;
		}

		/* Bounce off of or destroy walls */
		for (j = 0; j < MAX_WALLS; j++)
		{
			if (world.mapWalls[j].wallExist == true
			 && tempPosX >= world.mapWalls[j].wallX && tempPosX <= world.mapWalls[j].wallX + 11
			 && tempPosY >= world.mapWalls[j].wallY && tempPosY <= world.mapWalls[j].wallY + 14)
			{
				if (demolish[shotRec[i].shottype])
				{
					/* Blow up the wall and remove the shot. */
					world.mapWalls[j].wallExist = false;
					shotRec[i].isAvailable = true;
					JE_makeExplosion(tempPosX, tempPosY, shotRec[i].shottype);
					continue;
				} else {
					/* Otherwise, bounce. */
					if (shotRec[i].x - shotRec[i].xmov < world.mapWalls[j].wallX
					 || shotRec[i].x - shotRec[i].xmov > world.mapWalls[j].wallX + 11)
					{
						shotRec[i].xmov = -shotRec[i].xmov;
					}
					if (shotRec[i].y - shotRec[i].ymov < world.mapWalls[j].wallY
					 || shotRec[i].y - shotRec[i].ymov > world.mapWalls[j].wallY + 14)
					{
						if (shotRec[i].ymov < 0)
						{
							shotRec[i].ymov = -shotRec[i].ymov;
						}
						else
						{
							shotRec[i].ymov = -shotRec[i].ymov * 0.8f;
						}
					}

					tempPosX = round(shotRec[i].x);
					tempPosY = round(shotRec[i].y);
				}
			}
		}

		/* Our last collision check, at least for now.  We hit dirt. */
		if((((Uint8 *)destructTempScreen->pixels)[tempPosX + tempPosY * destructTempScreen->pitch]) == PIXEL_DIRT)
		{
			shotRec[i].isAvailable = true;
			JE_makeExplosion(tempPosX, tempPosY, shotRec[i].shottype);
			continue;
		}
	}
}
void DE_DrawTrails( struct destruct_shot_s * shot, unsigned int count, unsigned int decay, unsigned int startColor )
{
	int i;


	for (i = count-1; i >= 0; i--) /* going in reverse is important as it affects how we draw */
	{
		if (shot->trailc[i] > 0 && shot->traily[i] > 12) /* If it exists and if it's not out of bounds, draw it. */
		{
			JE_pixCool(shot->trailx[i], shot->traily[i], shot->trailc[i]);
		}

		if (i == 0) /* The first trail we create. */
		{
			shot->trailx[i] = round(shot->x);
			shot->traily[i] = round(shot->y);
			shot->trailc[i] = startColor;
		}
		else /* The newer trails decay into the older trails.*/
		{
			shot->trailx[i] = shot->trailx[i-1];
			shot->traily[i] = shot->traily[i-1];
			if (shot->trailc[i-1] > 0)
			{
				shot->trailc[i] = shot->trailc[i-1] - decay;
			}
		}
	}
}
void DE_RunTickAI( void )
{
	unsigned int i, j;
	struct destruct_player_s * ptrPlayer, * ptrTarget;
	struct destruct_unit_s * ptrUnit, * ptrCurUnit;


	for (i = 0; i < MAX_PLAYERS; i++)
	{
		ptrPlayer = &(player[i]);
		if (ptrPlayer->is_cpu == false)
		{
			continue;
		}


		/* I've been thinking, purely hypothetically, about what it would take
		 * to have multiple computer opponents.  The answer?  A lot of crap
		 * and a 'target' variable in the player struct. */
		j = i + 1;
		if (j >= MAX_PLAYERS)
		{
			j = 0;
		}

		ptrTarget  = &(player[j]);
		ptrCurUnit = &(ptrPlayer->unit[ptrPlayer->unitSelected]);


		/* This is the start of the original AI.  Heh.  AI. */
		if (ptrPlayer->aiMemory.c_noDown > 0)
		{
			ptrPlayer->aiMemory.c_noDown--;
		}

		/* Until all structs are properly divvied up this must only apply to player1 */
		if (mt_rand() % 100 > 80)
		{
			ptrPlayer->aiMemory.c_Angle += (mt_rand() % 3) - 1;
			if (ptrPlayer->aiMemory.c_Angle > 1)
			{
				ptrPlayer->aiMemory.c_Angle = 1;
			}
			if (ptrPlayer->aiMemory.c_Angle < -1)
			{
				ptrPlayer->aiMemory.c_Angle = -1;
			}
		}
		if (mt_rand() % 100 > 90)
		{
			if (ptrPlayer->aiMemory.c_Angle > 0 && ptrCurUnit->angle > (M_PI / 2.0f) - (M_PI / 9.0f))
			{
				ptrPlayer->aiMemory.c_Angle = 0;
			}
			if (ptrPlayer->aiMemory.c_Angle < 0 && ptrCurUnit->angle < M_PI / 8.0f)
			{
				ptrPlayer->aiMemory.c_Angle = 0;
			}
		}

		if (mt_rand() % 100 > 93)
		{
			ptrPlayer->aiMemory.c_Power += (mt_rand() % 3) - 1;
			if (ptrPlayer->aiMemory.c_Power > 1)
			{
				ptrPlayer->aiMemory.c_Power = 1;
			}
			if (ptrPlayer->aiMemory.c_Power < -1)
			{
				ptrPlayer->aiMemory.c_Power = -1;
			}
		}
		if (mt_rand() % 100 > 90)
		{
			if (ptrPlayer->aiMemory.c_Power > 0 && ptrCurUnit->power > 4)
			{
				ptrPlayer->aiMemory.c_Power = 0;
			}
			if (ptrPlayer->aiMemory.c_Power < 0 && ptrCurUnit->power < 3)
			{
				ptrPlayer->aiMemory.c_Power = 0;
			}
			if (ptrCurUnit->power < 2)
			{
				ptrPlayer->aiMemory.c_Power = 1;
			}
		}

		// prefer helicopter
		ptrUnit = ptrPlayer->unit;
		for (j = 0; j < MAX_INSTALLATIONS; j++, ptrUnit++)
		{
			if (DE_isValidUnit(ptrUnit) && ptrUnit->unitType == UNIT_HELI)
			{
				ptrPlayer->unitSelected = j;
				break;
			}
		}

		if (ptrCurUnit->unitType == UNIT_HELI)
		{
			if (ptrCurUnit->isYInAir == false)
			{
				ptrPlayer->aiMemory.c_Power = 1;
			}
			if (mt_rand() % ptrCurUnit->unitX > 100)
			{
				ptrPlayer->aiMemory.c_Power = 1;
			}
			if (mt_rand() % 240 > ptrCurUnit->unitX)
			{
				ptrPlayer->moves.actions[MOVE_RIGHT] = true;
			}
			else if ((mt_rand() % 20) + 300 < ptrCurUnit->unitX)
			{
				ptrPlayer->moves.actions[MOVE_LEFT] = true;
			}
			else if (mt_rand() % 30 == 1)
			{
				ptrPlayer->aiMemory.c_Angle = (mt_rand() % 3) - 1;
			}
			if (ptrCurUnit->unitX > 295 && ptrCurUnit->lastMove > 1)
			{
				ptrPlayer->moves.actions[MOVE_LEFT] = true;
				ptrPlayer->moves.actions[MOVE_RIGHT] = false;
			}
			if (ptrCurUnit->unitType != UNIT_HELI || ptrCurUnit->lastMove > 3 || (ptrCurUnit->unitX > 160 && ptrCurUnit->lastMove > -3))
			{
				if (mt_rand() % (int)round(ptrCurUnit->unitY) < 150 && ptrCurUnit->unitYMov < 0.01f && (ptrCurUnit->unitX < 160 || ptrCurUnit->lastMove < 2))
				{
					ptrPlayer->moves.actions[MOVE_FIRE] = true;
				}
				ptrPlayer->aiMemory.c_noDown = (5 - abs(ptrCurUnit->lastMove)) * (5 - abs(ptrCurUnit->lastMove)) + 3;
				ptrPlayer->aiMemory.c_Power = 1;
			} else
			{
				ptrPlayer->moves.actions[MOVE_FIRE] = false;
			}

			ptrUnit = ptrTarget->unit;
			for (j = 0; j < MAX_INSTALLATIONS; j++, ptrUnit++)
			{
				if (abs(ptrUnit->unitX - ptrCurUnit->unitX) < 8)
				{
					/* I get it.  This makes helicoptors hover over
					 * their enemies. */
					if (ptrUnit->unitType == UNIT_SATELLITE)
					{
						ptrPlayer->moves.actions[MOVE_FIRE] = false;
					}
					else
					{
						ptrPlayer->moves.actions[MOVE_LEFT] = false;
						ptrPlayer->moves.actions[MOVE_RIGHT] = false;
						if (ptrCurUnit->lastMove < -1)
						{
							ptrCurUnit->lastMove++;
						}
						else if (ptrCurUnit->lastMove > 1)
						{
							ptrCurUnit->lastMove--;
						}
					}
				}
			}
		} else {
			ptrPlayer->moves.actions[MOVE_FIRE] = 1;
		}

		if (mt_rand() % 200 > 198)
		{
			ptrPlayer->moves.actions[MOVE_CHANGE] = true;
			ptrPlayer->aiMemory.c_Angle = 0;
			ptrPlayer->aiMemory.c_Power = 0;
			ptrPlayer->aiMemory.c_Fire = 0;
		}

		if (mt_rand() % 100 > 98 || ptrCurUnit->shotType == SHOT_TRACER)
		{   /* Clearly the CPU doesn't like the tracer :) */
			ptrPlayer->moves.actions[MOVE_CYDN] = true;
		}
		if (ptrPlayer->aiMemory.c_Angle > 0)
		{
			ptrPlayer->moves.actions[MOVE_LEFT] = true;
		}
		if (ptrPlayer->aiMemory.c_Angle < 0)
		{
			ptrPlayer->moves.actions[MOVE_RIGHT] = true;
		}
		if (ptrPlayer->aiMemory.c_Power > 0)
		{
			ptrPlayer->moves.actions[MOVE_UP] = true;
		}
		if (ptrPlayer->aiMemory.c_Power < 0 && ptrPlayer->aiMemory.c_noDown == 0)
		{
			ptrPlayer->moves.actions[MOVE_DOWN] = true;
		}
		if (ptrPlayer->aiMemory.c_Fire > 0)
		{
			ptrPlayer->moves.actions[MOVE_FIRE] = true;
		}

		if (ptrCurUnit->unitYMov < -0.1f && ptrCurUnit->unitType == UNIT_HELI)
		{
			ptrPlayer->moves.actions[MOVE_FIRE] = false;
		}

		/* This last hack was down in the processing section.
		 * What exactly it was doing there I do not know */
		if(ptrCurUnit->unitType == UNIT_LASER || ptrCurUnit->isYInAir == true) {
			ptrPlayer->aiMemory.c_Power = 0;
		}
	}
}
void DE_RunTickDrawCrosshairs( void )
{
	unsigned int i;
	int tempPosX, tempPosY;
	int direction;
	struct destruct_unit_s * curUnit;


	/* Draw the crosshairs.  Most vehicles aim left or right.  Helis can aim
	 * either way and this must be accounted for.
	 */
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		direction = (i == PLAYER_LEFT) ? -1 : 1;
		curUnit = &(player[i].unit[player[i].unitSelected]);

		if (curUnit->unitType == UNIT_HELI)
		{
			tempPosX = curUnit->unitX + round(0.1 * curUnit->lastMove * curUnit->lastMove * curUnit->lastMove) + 5;
			tempPosY = round(curUnit->unitY) + 1;
		} else {
			tempPosX = round(curUnit->unitX + 6 - cos(curUnit->angle) * (curUnit->power * 8 + 7) * direction);
			tempPosY = round(curUnit->unitY - 7 - sin(curUnit->angle) * (curUnit->power * 8 + 7));
		}

		/* Draw it.  Clip away from the HUD though. */
		if(tempPosY > 9)
		{
			if(tempPosY > 11)
			{
				if(tempPosY > 13)
				{
					/* Top pixel */
					JE_pix(tempPosX,     tempPosY - 2,  3);
				}
				/* Middle three pixels */
				JE_pix(tempPosX + 3, tempPosY,      3);
				JE_pix(tempPosX,     tempPosY,     14);
				JE_pix(tempPosX - 3, tempPosY,      3);
			}
			/* Bottom pixel */
			JE_pix(tempPosX,     tempPosY + 2,  3);
		}
	}
}
void DE_RunTickDrawHUD( void )
{
	unsigned int i;
	unsigned int startX;
	char tempstr[16]; /* Max size needed: 16 assuming 10 digit int max. */
	struct destruct_unit_s * curUnit;


	for (i = 0; i < MAX_PLAYERS; i++)
	{
		curUnit = &(player[i].unit[player[i].unitSelected]);
		startX = ((i == PLAYER_LEFT) ? 0 : 320 - 150);

		JE_bar       ( startX +  5, 3, startX +  14, 8, 241);
		JE_rectangle ( startX +  4, 2, startX +  15, 9, 242);
		JE_rectangle ( startX +  3, 1, startX +  16, 10, 240);
		JE_bar       ( startX + 18, 3, startX + 140, 8, 241);
		JE_rectangle ( startX + 17, 2, startX + 143, 9, 242);
		JE_rectangle ( startX + 16, 1, startX + 144, 10, 240);

		JE_drawShape2( startX +  4, 0, 191 + curUnit->shotType, eShapes1);

		JE_outText   ( startX + 20, 3, weaponNames[curUnit->shotType], 15, 2);
		sprintf      (tempstr, "dmg~%d~", curUnit->health);
		JE_outText   ( startX + 75, 3, tempstr, 15, 0);
		sprintf      (tempstr, "pts~%d~", player[i].score);
		JE_outText   ( startX + 110, 3, tempstr, 15, 0);
	}
}
void DE_RunTickGetInput( void )
{
	unsigned int player_index, key_index, slot_index;
	SDLKey key;

	/* player.keys holds our key config.  Players will eventually be allowed
	 * to can change their key mappings.  player.moves and player.keys
	 * line up; rather than manually checking left and right we can
	 * just loop through the indexes and set the actions as needed. */
	service_SDL_events(true);

	for(player_index = 0; player_index < MAX_PLAYERS; player_index++)
	{
		for(key_index = 0; key_index < MAX_KEY; key_index++)
		{
			for(slot_index = 0; slot_index < MAX_KEY_OPTIONS; slot_index++)
			{
				key = player[player_index].keys.Config[key_index][slot_index];
				if(key == SDLK_UNKNOWN) { break; }
				if(keysactive[key] == true)
				{
					/* The right key was clearly pressed */
					player[player_index].moves.actions[key_index] = true;

					/* Some keys we want to toggle afterwards */
					if(key_index == KEY_CHANGE ||
					   key_index == KEY_CYUP   ||
					   key_index == KEY_CYDN)
					{
						keysactive[key] = false;
					}
					break;
				}
			}
		}
	}
}
void DE_ProcessInput( void )
{
	int direction;

	unsigned int player_index, player_enemy;
	struct destruct_unit_s * curUnit;


	for (player_index = 0; player_index < MAX_PLAYERS; player_index++)
	{
		if (player[player_index].unitsRemaining <= 0) { continue; }

		player_enemy = player_index + 1;
		if(player_enemy > PLAYER_RIGHT)
		{
			player_enemy = PLAYER_LEFT;
		}

		direction = (player_index == PLAYER_LEFT) ? -1 : 1;
		curUnit = &(player[player_index].unit[player[player_index].unitSelected]);

		if (systemAngle[curUnit->unitType] == true) /* selected unit may change shot angle */
		{
			if (player[player_index].moves.actions[MOVE_LEFT] == true)
			{
				(player_index == PLAYER_LEFT) ? DE_RaiseAngle(curUnit) : DE_LowerAngle(curUnit);
			}
			if (player[player_index].moves.actions[MOVE_RIGHT] == true)
			{
				(player_index == PLAYER_LEFT) ? DE_LowerAngle(curUnit) : DE_RaiseAngle(curUnit);

			}
		} else if (curUnit->unitType == UNIT_HELI) {
			if (player[player_index].moves.actions[MOVE_LEFT] == true && curUnit->unitX > 5)
				if (JE_stabilityCheck(curUnit->unitX - 5, round(curUnit->unitY)))
				{
					if (curUnit->lastMove > -5)
					{
						curUnit->lastMove--;
					}
					curUnit->unitX--;
					if (JE_stabilityCheck(curUnit->unitX, round(curUnit->unitY)))
					{
						curUnit->isYInAir = true;
					}
				}
			if (player[player_index].moves.actions[MOVE_RIGHT] == true && curUnit->unitX < 305)
			{
				if (JE_stabilityCheck(curUnit->unitX + 5, round(curUnit->unitY)))
				{
					if (curUnit->lastMove < 5)
					{
						curUnit->lastMove++;
					}
					curUnit->unitX++;
					if (JE_stabilityCheck(curUnit->unitX, round(curUnit->unitY)))
					{
						curUnit->isYInAir = true;
					}
				}
			}
		}

		if (curUnit->unitType != UNIT_LASER)

		{	/*increasepower*/
			if (player[player_index].moves.actions[MOVE_UP] == true)
			{
				if (curUnit->unitType == UNIT_HELI)
				{
					curUnit->isYInAir = true;
					curUnit->unitYMov -= 0.1;
				}
				else if (curUnit->unitType == UNIT_JUMPER
				      && curUnit->isYInAir == false) {
					curUnit->unitYMov = -3;
					curUnit->isYInAir = true;
				}
				else {
					DE_RaisePower(curUnit);
				}
			}
			/*decreasepower*/
			if (player[player_index].moves.actions[MOVE_DOWN] == true)
			{
				if (curUnit->unitType == UNIT_HELI && curUnit->isYInAir == true)
				{
					curUnit->unitYMov += 0.1;
				} else {
					DE_LowerPower(curUnit);
				}
			}
		}

		/*up/down weapon.  These just cycle until a valid weapon is found */
		if (player[player_index].moves.actions[MOVE_CYUP] == true)
		{
			DE_CycleWeaponUp(curUnit);
		}
		if (player[player_index].moves.actions[MOVE_CYDN] == true)
		{
			DE_CycleWeaponDown(curUnit);
		}

		/* Change.  Since change would change out curUnit pointer, let's just do it last.
		 * Validity checking is performed at the beginning of the tick. */
		if (player[player_index].moves.actions[MOVE_CHANGE] == true)
		{
			player[player_index].unitSelected++;
			if (player[player_index].unitSelected >= MAX_INSTALLATIONS)
			{
				player[player_index].unitSelected = 0;
			}
		}

		/*Newshot*/
		if (player[player_index].shotDelay > 0)
		{
			player[player_index].shotDelay--;
		}
		if (player[player_index].moves.actions[MOVE_FIRE] == true
		&& (player[player_index].shotDelay == 0))
		{
			player[player_index].shotDelay = shotDelay[curUnit->shotType];

			switch(shotDirt[curUnit->shotType])
			{
				case EXPL_NONE:
					break;

				case EXPL_MAGNET:
					DE_RunMagnet(player_index, curUnit);
					break;

				case EXPL_DIRT:
				case EXPL_NORMAL:
					DE_MakeShot(player_index, curUnit, direction);
					break;

				default:
					assert(false);
			}
		}
	}
}

void DE_CycleWeaponUp( struct destruct_unit_s * unit )
{
	do
	{
		unit->shotType++;
		if (unit->shotType > SHOT_LAST)
		{
			unit->shotType = SHOT_FIRST;
		}
	} while (weaponSystems[unit->unitType][unit->shotType] == 0);
}
void DE_CycleWeaponDown( struct destruct_unit_s * unit )
{
	do
	{
		unit->shotType--;
		if (unit->shotType < SHOT_FIRST)
		{
			unit->shotType = SHOT_LAST;
		}
	} while (weaponSystems[unit->unitType][unit->shotType] == 0);
}


void DE_MakeShot( enum de_player_t curPlayer, const struct destruct_unit_s * curUnit, int direction )
{
	unsigned int i;
	unsigned int shotIndex;


	/* First, find an empty shot struct we can use */
	for (i = 0; ; i++)
	{
		if (i >= MAX_SHOTS) { return; } /* no empty slots.  Do nothing. */

		if (shotRec[i].isAvailable)
		{
			shotIndex = i;
			break;
		}
	}
	if (curUnit->unitType == UNIT_HELI && curUnit->isYInAir == false)
	{ /* Helis can't fire when they are on the ground. */
		return;
	}

	/* Play the firing sound */
	soundQueue[curPlayer] = shotSound[curUnit->shotType];

	/* Create our shot.  Some units have differing logic here */
	switch (curUnit->unitType)
	{
		case UNIT_HELI:

			shotRec[shotIndex].x = curUnit->unitX + curUnit->lastMove * 2 + 5;
			shotRec[shotIndex].xmov = 0.02 * curUnit->lastMove * curUnit->lastMove * curUnit->lastMove;

			/* If we are trying in vain to move up off the screen, act differently.*/
			if (player[curPlayer].moves.actions[MOVE_UP] && curUnit->unitY < 30)
			{
				shotRec[shotIndex].y = curUnit->unitY;
				shotRec[shotIndex].ymov = 0.1;

				if (shotRec[shotIndex].xmov < 0)
				{
					shotRec[shotIndex].xmov += 0.1f;
				}
				else if (shotRec[shotIndex].xmov > 0)
				{
					shotRec[shotIndex].xmov -= 0.1f;
				}
			}
			else
			{
				shotRec[shotIndex].y = curUnit->unitY + 1;
				shotRec[shotIndex].ymov = 0.50 + curUnit->unitYMov * 0.1;
			}
			break;

		case UNIT_JUMPER: /* Jumpers are actually only special for the left hand player.  Bug?  Or feature? */

			if(curPlayer == PLAYER_LEFT)
			{
				/* This is identical to the default case.
				 * I considered letting the switch fall through
				 * but that's more confusing to people who aren't used
				 * to that quirk of switch. */

				shotRec[shotIndex].x    = curUnit->unitX + 6 - cos(curUnit->angle) * 10 * direction;
				shotRec[shotIndex].y    = curUnit->unitY - 7 - sin(curUnit->angle) * 10;
				shotRec[shotIndex].xmov = -cos(curUnit->angle) * curUnit->power * direction;
				shotRec[shotIndex].ymov = -sin(curUnit->angle) * curUnit->power;
			}
			else
			{
				/* This is not identical to the default case. */

				shotRec[shotIndex].x = curUnit->unitX + 2;
				shotRec[shotIndex].xmov = -cos(curUnit->angle) * curUnit->power * direction;

				if (curUnit->isYInAir == true)
				{
					shotRec[shotIndex].ymov = 1;
					shotRec[shotIndex].y = curUnit->unitY + 2;
				} else {
					shotRec[shotIndex].ymov = -2;
					shotRec[shotIndex].y = curUnit->unitY - 12;
				}
			}
			break;

		default:

			shotRec[shotIndex].x    = curUnit->unitX + 6 - cos(curUnit->angle) * 10 * direction;
			shotRec[shotIndex].y    = curUnit->unitY - 7 - sin(curUnit->angle) * 10;
			shotRec[shotIndex].xmov = -cos(curUnit->angle) * curUnit->power * direction;
			shotRec[shotIndex].ymov = -sin(curUnit->angle) * curUnit->power;
			break;
	}

	/* Now set/clear out a few last details. */
	shotRec[shotIndex].isAvailable = false;

	shotRec[shotIndex].shottype = curUnit->shotType;
	//shotRec[shotIndex].shotdur = shotFuse[shotRec[shotIndex].shottype];

	shotRec[shotIndex].trailc[0] = 0;
	shotRec[shotIndex].trailc[1] = 0;
	shotRec[shotIndex].trailc[2] = 0;
	shotRec[shotIndex].trailc[3] = 0;
}
void DE_RunMagnet( enum de_player_t curPlayer, struct destruct_unit_s * magnet )
{
	unsigned int i;
	enum de_player_t curEnemy;
	int direction;
	struct destruct_unit_s * enemyUnit;


	curEnemy = (curPlayer == PLAYER_LEFT) ? PLAYER_RIGHT : PLAYER_LEFT;
	direction = (curPlayer == PLAYER_LEFT) ? -1 : 1;

	/* Push all shots that are in front of the magnet */
	for (i = 0; i < MAX_SHOTS; i++)
	{
		if (shotRec[i].isAvailable == false)
		{
			if ((curPlayer == PLAYER_LEFT  && shotRec[i].x > magnet->unitX)
			 || (curPlayer == PLAYER_RIGHT && shotRec[i].x < magnet->unitX))
			{
				shotRec[i].xmov += magnet->power * 0.1f * -direction;
			}
		}
	}

	enemyUnit = player[curEnemy].unit;
	for (i = 0; i < MAX_INSTALLATIONS; i++, enemyUnit++) /* magnets push coptors */
	{
		if (DE_isValidUnit(enemyUnit)
		 && enemyUnit->unitType == UNIT_HELI
		 && enemyUnit->isYInAir == true)
		{
			if ((curEnemy == PLAYER_RIGHT && player[curEnemy].unit[i].unitX + 11 < 318)
			 || (curEnemy == PLAYER_LEFT  && player[curEnemy].unit[i].unitX > 1))
			{
				enemyUnit->unitX -= 2 * direction;
			}
		}
	}
	magnet->ani_frame = 1;
}
void DE_RaiseAngle( struct destruct_unit_s * unit )
{
	unit->angle += 0.01;
	if (unit->angle > M_PI / 2 - 0.01)
	{
		unit->angle = M_PI / 2 - 0.01;
	}
}
void DE_LowerAngle( struct destruct_unit_s * unit )
{
	unit->angle -= 0.01f;
	if (unit->angle < 0)
	{
		unit->angle = 0;
	}
}
void DE_RaisePower( struct destruct_unit_s * unit )
{
	unit->power += 0.05;
	if (unit->power > 5)
	{
	unit->power = 5;
	}
}
void DE_LowerPower( struct destruct_unit_s * unit )
{
	unit->power -= 0.05;
	if (unit->power < 1)
	{
		unit->power = 1;
	}
}

/* DE_isValidUnit
 *
 * Returns true if the unit's health is above 0 and false
 * otherwise.  This mainly exists because the 'health' var
 * serves two roles and that can get confusing.
 */
static inline bool DE_isValidUnit( struct destruct_unit_s * unit )
{
	return(unit->health > 0);
}


bool DE_RunTickCheckEndgame( void )
{
	if (player[PLAYER_LEFT].unitsRemaining == 0)
	{
		player[PLAYER_RIGHT].score += ModeScore[PLAYER_LEFT][world.destructMode];
		soundQueue[7] = V_CLEARED_PLATFORM;
		return(true);
	}
	if (player[PLAYER_RIGHT].unitsRemaining == 0)
	{
		player[PLAYER_LEFT].score += ModeScore[PLAYER_RIGHT][world.destructMode];
		soundQueue[7] = V_CLEARED_PLATFORM;
		return(true);
	}
	return(false);
}
void DE_RunTickPlaySounds( void )
{
	unsigned int i, tempSampleIndex, tempVolume;


	for (i = 0; i < COUNTOF(soundQueue); i++)
	{
		if (soundQueue[i] != S_NONE)
		{
			tempSampleIndex = soundQueue[i];
			if (i == 7)
			{
				tempVolume = fxPlayVol;
			}
			else
			{
				tempVolume = fxPlayVol / 2;
			}

			JE_multiSamplePlay(digiFx[tempSampleIndex-1], fxSize[tempSampleIndex-1], i, tempVolume);
			soundQueue[i] = S_NONE;
		}
	}
}

void JE_pixCool( unsigned int x, unsigned int y, Uint8 c )
{
	JE_pix(x, y, c);
	JE_pix(x - 1, y, c - 2);
	JE_pix(x + 1, y, c - 2);
	JE_pix(x, y - 1, c - 2);
	JE_pix(x, y + 1, c - 2);
}
// kate: tab-width 4; vim: set noet:
