
#include "engine/keen/dreams/dreamsengine.h"

extern dreams::DreamsEngine *gDreamsEngine;

extern "C"
{


#include <string.h>
#include "SDL.h"

#include "be_cross.h"
#include "be_st.h"


//static SDL_Rect g_sdlAspectCorrectionRect, g_sdlAspectCorrectionBorderedRect;

static bool g_sdlDoRefreshGfxOutput;
bool g_sdlForceGfxControlUiRefresh;

void BE_ST_MarkGfxForUpdate(void)
{
	g_sdlDoRefreshGfxOutput = true;
}

const int GFX_TEX_WIDTH  = 320;
const int GFX_TEX_HEIGHT = 200;
#define VGA_TXT_TEX_WIDTH 720
#define VGA_TXT_TEX_HEIGHT 400
//#define EGACGA_TXT_TEX_WIDTH 640
//#define EGACGA_TXT_TEX_HEIGHT 200

// Overscan border dimensions (for each side of the screen)
#define ENGINE_VGA_GFX_OVERSCAN_LEFT_AFTER_DOUBLING 16 // Doubling from 8 for us
#define ENGINE_VGA_GFX_OVERSCAN_RIGHT_AFTER_DOUBLING 16 // Doubling from 8 for us
#define ENGINE_VGA_GFX_OVERSCAN_TOP_AFTER_DOUBLING 7    // 200-line doubling
#define ENGINE_VGA_GFX_OVERSCAN_BOTTOM_AFTER_DOUBLING 7 // 200-line doubling
#define ENGINE_VGA_TXT_OVERSCAN_LEFT 9
#define ENGINE_VGA_TXT_OVERSCAN_RIGHT 9
#define ENGINE_VGA_TXT_OVERSCAN_TOP 7
#define ENGINE_VGA_TXT_OVERSCAN_BOTTOM 7

const int TXT_COLS_NUM = 80;
const int TXT_ROWS_NUM = 25;

#define VGA_TXT_CURSOR_BLINK_VERT_FRAME_RATE 8
#define VGA_TXT_BLINK_VERT_FRAME_RATE 16


extern const uint8_t g_vga_8x16TextFont[256*8*16];
// We can use a union because the memory contents are refreshed on screen mode change
// (well, not on change between modes 0xD and 0xE, both sharing planar A000:0000)
static union {
	uint8_t egaGfx[4][0x10000]; // Contents of A000:0000 (4 planes)
	uint8_t text[TXT_COLS_NUM*TXT_ROWS_NUM*2]; // Textual contents of B800:0000
} g_sdlVidMem;

// Used for simple caching of EGA graphics (due to page flipping and more)
// and similarly CGA graphics (modified only at one place)
static union {
	uint8_t egaGfx[2*GFX_TEX_WIDTH*GFX_TEX_HEIGHT]; // Support 640x200 mode for Catacomb Abyss
	uint8_t cgaGfx[GFX_TEX_WIDTH*GFX_TEX_HEIGHT];
} g_sdlHostScrMem, g_sdlHostScrMemCache;

static uint16_t g_sdlScreenStartAddress = 0;
static int g_sdlScreenMode = 3;
static int g_sdlTexWidth, g_sdlTexHeight;
static uint8_t g_sdlPelPanning = 0;
static uint8_t g_sdlLineWidth = 40;
static int16_t g_sdlSplitScreenLine = -1;
static int g_sdlTxtCursorPosX, g_sdlTxtCursorPosY;
static bool g_sdlTxtCursorEnabled = true;
static int g_sdlTxtColor = 7, g_sdlTxtBackground = 0;


/*** Game controller UI resource definitions ***/

//#include "../rsrc/pad_font_mono.xpm"
//#include "../rsrc/pad_thumb_buttons.xpm"
//#include "../rsrc/pad_dpad.xpm"

#define ALTCONTROLLER_PAD_PIX_WIDTH 48
#define ALTCONTROLLER_PAD_PIX_HEIGHT 48
#define ALTCONTROLLER_CHAR_PIX_WIDTH 6
#define ALTCONTROLLER_CHAR_PIX_HEIGHT 8
#define ALTCONTROLLER_CHAR_TOTAL_PIX_WIDTH 570

#define ALTCONTROLLER_EDGE_PIX_DIST 2
#define ALTCONTROLLER_FACEBUTTONS_SCREEN_DIM_RATIO 5

// Measured in keys
#define ALTCONTROLLER_KEYBOARD_WIDTH 18
#define ALTCONTROLLER_KEYBOARD_HEIGHT 3

#define ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH 22
#define ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT 12

#define ALTCONTROLLER_KEYBOARD_PIX_WIDTH (ALTCONTROLLER_KEYBOARD_WIDTH*ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH)
#define ALTCONTROLLER_KEYBOARD_PIX_HEIGHT (ALTCONTROLLER_KEYBOARD_HEIGHT*ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT)

#define ALTCONTROLLER_KEYBOARD_SCREEN_MINDIM_RATIO 2

// These are given as (x, y) offset pairs within the non-scaled,
// face buttons image, assuming longest texts possible (3 chars long)
static const int g_sdlControllerFaceButtonsTextLocs[] = {15, 34, 28, 21, 2, 21, 15, 8};

//static SDL_Rect g_sdlControllerFaceButtonsRect, g_sdlControllerDpadRect, g_sdlControllerTextInputRect;


static bool g_sdlFaceButtonsAreShown, g_sdlDpadIsShown, g_sdlTextInputUIIsShown;
// With alternative game controllers scheme, all UI is hidden if no controller is connected
bool g_sdlShowControllerUI;

static int g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY;
static bool g_sdlTextInputIsKeyPressed, g_sdlTextInputIsShifted;


// Scancode names for controller face buttons UI and similar
// (but not a whole on-screen keyboard), based on DOS scancodes
// (doesn't include "special" keys for which 0xE0 is sent), and others
/*static const char *g_sdlDOSScanCodeStrs[] = {
	NULL, "Esc",
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\x11",
	"Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Ent",
	"Ctl", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`",
	"\x1E", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/", NULL,
	NULL, "Alt", "[_]", "Cap",
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "Num", "Scl",
	"Hom", "\x18", "PgU", NULL, "\x1B", NULL, "\x1A", NULL, "End", "\x19", "PgD",
	"Ins", "Del",
};*/

// Same, but for onscreen keyboard in non-shifted state
/*static const char *g_sdlDOSScanCodeKeyboardNonShiftedStrs[] = {
	NULL, "Esc",
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\x11",
	NULL, "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Ent",
	NULL, "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`",
	"\x1E", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", NULL,
	NULL, NULL, "[_]", NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, "\x18", NULL, NULL, "\x1B", NULL, "\x1A", "+", NULL, "\x19", NULL,
	NULL, "Del",
};*/

// Same but shifted
/*static const char *g_sdlDOSScanCodeKeyboardShiftedStrs[] = {
	NULL, "Esc",
	"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "\x11",
	NULL, "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "Ent",
	NULL, "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "~",
	"\x1E", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", NULL,
	NULL, NULL, "[_]", NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, "\x18", NULL, NULL, "\x1B", NULL, "\x1A", "+", NULL, "\x19", NULL,
	NULL, "Del",
};*/

// One of the shifted and non-shifted variants
//static const char **g_sdlDOSScanCodeKeyboardStrs_Ptr;

// On-screen keyboard layout definition (probably better we don't use "special" keys e.g., with scancode 0xE0 sent, even though there shouldn't be a difference)

static const BE_ST_ScanCode_T g_sdlDOSScanCodeKeyboardLayout[ALTCONTROLLER_KEYBOARD_HEIGHT][ALTCONTROLLER_KEYBOARD_WIDTH] = {
	{BE_ST_SC_KP_4/*LEFT*/, BE_ST_SC_Q, BE_ST_SC_W, BE_ST_SC_E, BE_ST_SC_R, BE_ST_SC_T, BE_ST_SC_Y, BE_ST_SC_U, BE_ST_SC_I, BE_ST_SC_O, BE_ST_SC_P, BE_ST_SC_LBRACKET, BE_ST_SC_RBRACKET, BE_ST_SC_7, BE_ST_SC_8, BE_ST_SC_9, BE_ST_SC_0, BE_ST_SC_MINUS},

	{BE_ST_SC_KP_6/*RIGHT*/, BE_ST_SC_A, BE_ST_SC_S, BE_ST_SC_D, BE_ST_SC_F, BE_ST_SC_G, BE_ST_SC_H, BE_ST_SC_J, BE_ST_SC_K, BE_ST_SC_L, BE_ST_SC_SEMICOLON, BE_ST_SC_QUOTE, BE_ST_SC_BACKSLASH, BE_ST_SC_4, BE_ST_SC_5, BE_ST_SC_6, BE_ST_SC_SPACE, BE_ST_SC_EQUALS},

	{BE_ST_SC_LSHIFT, BE_ST_SC_Z, BE_ST_SC_X, BE_ST_SC_C, BE_ST_SC_V, BE_ST_SC_B, BE_ST_SC_N, BE_ST_SC_M, BE_ST_SC_COMMA, BE_ST_SC_PERIOD, BE_ST_SC_SLASH, BE_ST_SC_GRAVE, BE_ST_SC_ENTER, BE_ST_SC_1, BE_ST_SC_2, BE_ST_SC_3, BE_ST_SC_KP_PERIOD/*DEL*/, BE_ST_SC_BSPACE},
};


// Colors in BGRA format/order (on certain platforms)

static const uint32_t g_sdlCGAGfxBGRAScreenColors[] = {
	0xff000000/*black*/,
	0xff00ffff/*light cyan*/,
	0xffff00ff/*light magenta*/,
	0xffffffff/*white*/
};

// Same but for the EGA/VGA (and colored text modes on CGA/EGA/VGA)

static const uint32_t g_sdlEGABGRAScreenColors[] = {
	0xff000000/*black*/, 0xff0000aa/*blue*/, 0xff00aa00/*green*/, 0xff00aaaa/*cyan*/,
	0xffaa0000/*red*/, 0xffaa00aa/*magenta*/, 0xffaa5500/*brown*/, 0xffaaaaaa/*light gray*/,
	0xff555555/*gray*/, 0xff5555ff/*light blue*/, 0xff55ff55/*light green*/, 0xff55ffff/*light cyan*/,
	0xffff5555/*light red*/, 0xffff55ff/*light magenta*/, 0xffffff55/*yellow*/, 0xffffffff/*white*/
};
/*
static void BEL_ST_CreatePadTextureIfNeeded(SDL_Texture **padTexturePtrPtr)
{
	if (*padTexturePtrPtr)
	{
		return;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	*padTexturePtrPtr = SDL_CreateTexture(g_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, ALTCONTROLLER_PAD_PIX_WIDTH, ALTCONTROLLER_PAD_PIX_HEIGHT);
	if (!(*padTexturePtrPtr))
	{
		BE_Cross_LogMessage(BE_LOG_MSG_ERROR, "Failed to (re)create SDL2 pad texture,\n%s\n", SDL_GetError());
		//Destroy window and renderer?
		exit(0);
	}
	SDL_SetTextureBlendMode(*padTexturePtrPtr, SDL_BLENDMODE_BLEND); // Yes there's some Alpha
}
*/
/*static void BEL_ST_RedrawTextToBuffer(uint32_t *picPtr, int picWidth, const char *text)
{
    for (int currRow = 0, fontXpmIndex = 3; currRow < ALTCONTROLLER_CHAR_PIX_HEIGHT; ++currRow, picPtr += picWidth, ++fontXpmIndex)
	{
		const char *fontRowPtr = pad_font_mono_xpm[fontXpmIndex];
		uint32_t *currPtr = picPtr;
		for (const char *chPtr = text; *chPtr; ++chPtr)
		{
			const char *fontPixPtr = fontRowPtr + ALTCONTROLLER_CHAR_PIX_WIDTH * (*chPtr);
			for (int currCharCol = 0; currCharCol < ALTCONTROLLER_CHAR_PIX_WIDTH; ++currCharCol, ++currPtr, ++fontPixPtr)
			{
				if (*fontPixPtr == '+')
				{
					*currPtr = g_sdlCGAGfxBGRAScreenColors[0];
				}
			}
		}
    }
}*/
/*
static const char * BEL_ST_PrepareToShowOnePad(const char *scanCodes, const char **padXpm, SDL_Texture **padTexturePtrPtr, bool *areButtonsShownPtr)
{
	BEL_ST_CreatePadTextureIfNeeded(padTexturePtrPtr);

	uint32_t pixels[ALTCONTROLLER_PAD_PIX_WIDTH*ALTCONTROLLER_PAD_PIX_HEIGHT];
	uint32_t *currPtr = pixels;
	for (int currRow = 0, xpmIndex = 5; currRow < ALTCONTROLLER_PAD_PIX_HEIGHT; ++currRow, ++xpmIndex)
	{
		const char *xpmRowPtr = padXpm[xpmIndex];
		for (int currCol = 0; currCol < ALTCONTROLLER_PAD_PIX_WIDTH; ++currCol, ++currPtr, ++xpmRowPtr)
		{
			switch (*xpmRowPtr)
			{
			case ' ':
				*currPtr = 0x00000000; // HACK (BGRA, working with any order) because we don't have it defined elsewhere
				break;
			case '.':
				*currPtr = g_sdlEGABGRAScreenColors[8]; // Gray
				break;
			case '+':
				*currPtr = g_sdlEGABGRAScreenColors[7]; // Light gray
				break;
			case '@':
				*currPtr = g_sdlEGABGRAScreenColors[15]; // White
				break;
			}
		}
	}
	// FIXME Rather than drawing each scancode as the ASCII code, we want something else...
	for (int counter = 0; (*scanCodes) && (counter < 4); ++scanCodes, ++counter)
	{
		const char *str = g_sdlDOSScanCodeStrs[(unsigned char)(*scanCodes)];
		BEL_ST_RedrawTextToBuffer(pixels + g_sdlControllerFaceButtonsTextLocs[2*counter] + g_sdlControllerFaceButtonsTextLocs[2*counter+1]*ALTCONTROLLER_PAD_PIX_WIDTH + (3-strlen(str))*(ALTCONTROLLER_CHAR_PIX_WIDTH/2), ALTCONTROLLER_PAD_PIX_WIDTH, str);
	}
	// Add some alpha channel
	currPtr = pixels;
	for (int pixCounter = 0; pixCounter < ALTCONTROLLER_PAD_PIX_WIDTH*ALTCONTROLLER_PAD_PIX_HEIGHT; ++pixCounter, ++currPtr)
	{
		*currPtr &= 0xBFFFFFFF; // BGRA
	}
	SDL_UpdateTexture(*padTexturePtrPtr, NULL, pixels, 4*ALTCONTROLLER_PAD_PIX_WIDTH);
	*areButtonsShownPtr = true;

	g_sdlForceGfxControlUiRefresh = true;

	return scanCodes; // Check if there's something left
}
*/
/*static*/ void BEL_ST_PrepareToShowFaceButtonsAndDpad(const char *scanCodes)
{
/*	scanCodes =  BEL_ST_PrepareToShowOnePad(scanCodes, pad_thumb_buttons_xpm, &g_sdlFaceButtonsTexture, &g_sdlFaceButtonsAreShown);
	if (*scanCodes)
        BEL_ST_PrepareToShowOnePad(scanCodes, pad_dpad_xpm, &g_sdlDpadTexture, &g_sdlDpadIsShown);*/
}
/*
static void BEL_ST_CreateTextInputTextureIfNeeded(void)
{
	if (g_sdlTextInputTexture)
	{
		return;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	g_sdlTextInputTexture = SDL_CreateTexture(g_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, ALTCONTROLLER_KEYBOARD_PIX_WIDTH, ALTCONTROLLER_KEYBOARD_PIX_HEIGHT);
	if (!g_sdlTextInputTexture)
	{
		BE_Cross_LogMessage(BE_LOG_MSG_ERROR, "Failed to (re)create SDL2 text input texture,\n%s\n", SDL_GetError());
		//Destroy window and renderer?
		exit(0);
	}
	SDL_SetTextureBlendMode(g_sdlTextInputTexture, SDL_BLENDMODE_BLEND); // Yes there's some Alpha
}
*/

//static void BEL_ST_RedrawKeyToBuffer(uint32_t *picPtr, int picWidth, const char *text, bool isSelected)
//{
//#if 0
//	// This can happen for space that should be skipped
//	if (!text)
//		text = "";
//#endif

//	/// Draw frame
//	uint32_t frameColor = isSelected ? g_sdlEGABGRAScreenColors[15] /*White*/ : g_sdlEGABGRAScreenColors[8] /*Gray*/;
//	uint32_t *currPtr = picPtr;
//	// Frame top
//	for (int currCol = 0; currCol < ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH; ++currCol, ++currPtr)
//	{
//		*currPtr = frameColor;
//	}
//	// Frame left + key + Frame right
//	currPtr += picWidth-ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH;
//	for (int currRow = 1; currRow < ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT-1; ++currRow)
//	{
//		// Frame left pixel
//		*(currPtr++) = frameColor;
//		// Line between frame sides
//		for (int currCol = 1; currCol < ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH-1; ++currCol, ++currPtr)
//		{
//			*currPtr = g_sdlEGABGRAScreenColors[7]/*Light gray*/;
//		}
//		// Frame right pixel
//		*currPtr = frameColor;

//		currPtr += picWidth-ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH+1;
//	}
//	// Frame bottom
//	for (int currCol = 0; currCol < ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH; ++currCol, ++currPtr)
//	{
//		*currPtr = frameColor;
//	}
//	/*** Draw text ***/
//	BEL_ST_RedrawTextToBuffer(picPtr + (ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH-ALTCONTROLLER_CHAR_PIX_WIDTH*strlen(text))/2 + picWidth*(ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT-ALTCONTROLLER_CHAR_PIX_HEIGHT)/2, picWidth, text);
//	// Add some alpha channel (shouldn't be a lot)
//	currPtr = picPtr;
//	for (int currRow = 0; currRow < ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT; ++currRow, currPtr += picWidth-ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH)
//	{
//		for (int currCol = 0; currCol < ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH; ++currCol, ++currPtr)
//		{
//		*currPtr &= 0xDFFFFFFF; // BGRA
//		}
//	}
//}
/*
static void BEL_ST_RedrawWholeTextInputUI(void)
{
	uint32_t pixels[ALTCONTROLLER_KEYBOARD_PIX_WIDTH*ALTCONTROLLER_KEYBOARD_PIX_HEIGHT];
	uint32_t *currPtr = pixels;

	for (int currKeyRow = 0; currKeyRow < ALTCONTROLLER_KEYBOARD_HEIGHT; ++currKeyRow, currPtr += ALTCONTROLLER_KEYBOARD_PIX_WIDTH*(ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT-1))
	{
		for (int currKeyCol = 0; currKeyCol < ALTCONTROLLER_KEYBOARD_WIDTH; ++currKeyCol, currPtr += ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH)
		{
			BEL_ST_RedrawKeyToBuffer(currPtr, ALTCONTROLLER_KEYBOARD_PIX_WIDTH, g_sdlDOSScanCodeKeyboardStrs_Ptr[(int)g_sdlDOSScanCodeKeyboardLayout[currKeyRow][currKeyCol]], false);
		}
	}
	// Simpler to do so outside the loop
	BEL_ST_RedrawKeyToBuffer(pixels + (ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH*g_sdlTextInputSelectedKeyX) + ALTCONTROLLER_KEYBOARD_PIX_WIDTH*(ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT*g_sdlTextInputSelectedKeyY), ALTCONTROLLER_KEYBOARD_PIX_WIDTH, g_sdlDOSScanCodeKeyboardStrs_Ptr[(int)g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX]], true);

	SDL_UpdateTexture(g_sdlTextInputTexture, NULL, pixels, 4*ALTCONTROLLER_KEYBOARD_PIX_WIDTH);
}
*/
/*static*//* void BEL_ST_PrepareToShowTextInputUI(void)
{
	BEL_ST_CreateTextInputTextureIfNeeded();

	g_sdlTextInputSelectedKeyX = 0;
	g_sdlTextInputSelectedKeyY = 0;
	g_sdlTextInputIsKeyPressed = false;
	g_sdlTextInputIsShifted = false;
	g_sdlDOSScanCodeKeyboardStrs_Ptr = g_sdlDOSScanCodeKeyboardNonShiftedStrs;

	BEL_ST_RedrawWholeTextInputUI();
	g_sdlTextInputUIIsShown = true;

	g_sdlForceGfxControlUiRefresh = true;
}

static void BEL_ST_ToggleTextInputUIKey(int x, int y, bool toggle)
{
	uint32_t pixels[ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH*ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT];

	BEL_ST_RedrawKeyToBuffer(pixels, ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH, g_sdlDOSScanCodeKeyboardStrs_Ptr[(int)g_sdlDOSScanCodeKeyboardLayout[y][x]], toggle);

	SDL_Rect outRect = {x*ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH, y*ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT, ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH, ALTCONTROLLER_KEYBOARD_KEY_PIXHEIGHT};

	SDL_UpdateTexture(g_sdlTextInputTexture, &outRect, pixels, 4*ALTCONTROLLER_KEYBOARD_KEY_PIXWIDTH);
}
*/
/*
int BEL_ST_MoveUpInTextInputUI(void)
{
	int origScanCode = g_sdlTextInputIsKeyPressed ? (int)g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX] : 0;
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, false);
	g_sdlTextInputIsKeyPressed = false;

	--g_sdlTextInputSelectedKeyY;
	if (g_sdlTextInputSelectedKeyY < 0)
	{
		g_sdlTextInputSelectedKeyY = ALTCONTROLLER_KEYBOARD_HEIGHT-1;
	}
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, true);
	return origScanCode;
}

int BEL_ST_MoveDownInTextInputUI(void)
{
	int origScanCode = g_sdlTextInputIsKeyPressed ? (int)g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX] : 0;
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, false);
	g_sdlTextInputIsKeyPressed = false;

	++g_sdlTextInputSelectedKeyY;
	if (g_sdlTextInputSelectedKeyY >= ALTCONTROLLER_KEYBOARD_HEIGHT)
	{
		g_sdlTextInputSelectedKeyY = 0;
	}
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, true);
	return origScanCode;
}

int BEL_ST_MoveLeftInTextInputUI(void)
{
	int origScanCode = g_sdlTextInputIsKeyPressed ? (int)g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX] : 0;
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, false);
	g_sdlTextInputIsKeyPressed = false;

	--g_sdlTextInputSelectedKeyX;
	if (g_sdlTextInputSelectedKeyX < 0)
	{
		g_sdlTextInputSelectedKeyX = ALTCONTROLLER_KEYBOARD_WIDTH-1;
	}
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, true);
	return origScanCode;
}

int BEL_ST_MoveRightInTextInputUI(void)
{
	int origScanCode = g_sdlTextInputIsKeyPressed ? (int)g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX] : 0;
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, false);
	g_sdlTextInputIsKeyPressed = false;

	++g_sdlTextInputSelectedKeyX;
	if (g_sdlTextInputSelectedKeyX >= ALTCONTROLLER_KEYBOARD_WIDTH)
	{
		g_sdlTextInputSelectedKeyX = 0;
	}
	BEL_ST_ToggleTextInputUIKey(g_sdlTextInputSelectedKeyX, g_sdlTextInputSelectedKeyY, true);
	return origScanCode;
}

int BEL_ST_ToggleShiftStateInTextInputUI(bool *pToggle)
{
	if (!(*pToggle))
		return 0;
	g_sdlTextInputIsShifted = !g_sdlTextInputIsShifted;
	*pToggle = g_sdlTextInputIsShifted;
	g_sdlDOSScanCodeKeyboardStrs_Ptr = g_sdlTextInputIsShifted ? g_sdlDOSScanCodeKeyboardShiftedStrs : g_sdlDOSScanCodeKeyboardNonShiftedStrs;

	BEL_ST_RedrawWholeTextInputUI();

	return BE_ST_SC_LSHIFT;
}

int BEL_ST_ToggleKeyPressInTextInputUI(bool *pToggle)
{
	if (g_sdlTextInputIsKeyPressed == *pToggle)
		return 0;
	g_sdlTextInputIsKeyPressed = *pToggle;
	// Shift key is a special case
	if ((g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX] == BE_ST_SC_LSHIFT))
		return BEL_ST_ToggleShiftStateInTextInputUI(pToggle);

	return (int)g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX];
}
*/
// Note: IGNORES shift status
int BEL_ST_GetPressedKeyScanCodeFromTextInputUI(void)
{
	return g_sdlTextInputIsKeyPressed ? (int)g_sdlDOSScanCodeKeyboardLayout[g_sdlTextInputSelectedKeyY][g_sdlTextInputSelectedKeyX] : 0;
}

// May be additionally required
bool BEL_ST_IsTextInputUIShifted(void)
{
	return g_sdlTextInputIsShifted;
}

/*static*/ void BEL_ST_HideAltInputUI(void)
{
	g_sdlFaceButtonsAreShown = false;
	g_sdlDpadIsShown = false;
	g_sdlTextInputUIIsShown = false;

	g_sdlForceGfxControlUiRefresh = true;
}
/*
void BE_ST_SetGfxOutputRects(void)
{
	int srcWidth = g_sdlTexWidth;
	int srcHeight = g_sdlTexHeight;
	if (g_sdlScreenMode != 3)
	{
		// On the VGA, line doubling is in effect for graphical 200 lines modes
		if (g_sdlScreenMode != 0xe) // Is source 320x200 (not 640x200)?
			srcWidth *= 2;
		srcHeight *= 2;
	}
	int srcBorderLeft, srcBorderRight, srcBorderTop, srcBorderBottom;
	if (g_sdlScreenMode == 3)
	{
		srcBorderLeft = ENGINE_VGA_TXT_OVERSCAN_LEFT;
		srcBorderRight = ENGINE_VGA_TXT_OVERSCAN_RIGHT;
		srcBorderTop = ENGINE_VGA_TXT_OVERSCAN_TOP;
		srcBorderBottom = ENGINE_VGA_TXT_OVERSCAN_BOTTOM;
	}
	else
	{
		srcBorderLeft = ENGINE_VGA_GFX_OVERSCAN_LEFT_AFTER_DOUBLING;
		srcBorderRight = ENGINE_VGA_GFX_OVERSCAN_RIGHT_AFTER_DOUBLING;
		srcBorderTop = ENGINE_VGA_GFX_OVERSCAN_TOP_AFTER_DOUBLING;
		srcBorderBottom = ENGINE_VGA_GFX_OVERSCAN_BOTTOM_AFTER_DOUBLING;
	}
	int srcBorderedWidth = srcBorderLeft+srcWidth+srcBorderRight;
	int srcBorderedHeight = srcBorderTop+srcHeight+srcBorderBottom;
	int winWidth, winHeight;
	SDL_GetWindowSize(g_sdlWindow, &winWidth, &winHeight);
	if (g_refKeenCfg.scaleType == SCALE_FILL)
	{
		g_sdlAspectCorrectionBorderedRect.w = winWidth;
		g_sdlAspectCorrectionBorderedRect.h = winHeight;
		g_sdlAspectCorrectionBorderedRect.x = 0;
		g_sdlAspectCorrectionBorderedRect.y = 0;
	}
	else
	{
		// Aspect correct - Includes overscan borders, should multiply
		// the height by 1.2 first (or the width by 5 and height 6),
		// if in 320x200 gfx mode, cause 1.2 == (320*3)/(4*200).
		// More generally, (srcwidth*3)/(4*srcheight).
		// So multiply width by 4*srcheight and height by 3*srcwidth;
		int scaledSrcBorderedWidth = (4*srcHeight)*srcBorderedWidth, scaledSrcBorderedHeight = (3*srcWidth)*srcBorderedHeight;
		if (scaledSrcBorderedHeight*winWidth < scaledSrcBorderedWidth*winHeight) // Thinner than scaledSrcBorderedWidth:scaledSrcBorderedHeight
		{
			g_sdlAspectCorrectionBorderedRect.w = winWidth;
			g_sdlAspectCorrectionBorderedRect.h = winWidth*scaledSrcBorderedHeight/scaledSrcBorderedWidth;
			g_sdlAspectCorrectionBorderedRect.x = 0;
			g_sdlAspectCorrectionBorderedRect.y = (winHeight-g_sdlAspectCorrectionBorderedRect.h)/2;
		}
		else // As wide as scaledSrcBorderedWidth:scaledSrcBorderedHeight at the least
		{
			g_sdlAspectCorrectionBorderedRect.w = winHeight*scaledSrcBorderedWidth/scaledSrcBorderedHeight;
			g_sdlAspectCorrectionBorderedRect.h = winHeight;
			g_sdlAspectCorrectionBorderedRect.x = (winWidth-g_sdlAspectCorrectionBorderedRect.w)/2;
			g_sdlAspectCorrectionBorderedRect.y = 0;
		}
	}
	// Finish with internal (non-bordered) rect
	g_sdlAspectCorrectionRect.x = g_sdlAspectCorrectionBorderedRect.x + g_sdlAspectCorrectionBorderedRect.w*srcBorderLeft/srcBorderedWidth;
	g_sdlAspectCorrectionRect.y = g_sdlAspectCorrectionBorderedRect.y + g_sdlAspectCorrectionBorderedRect.h*srcBorderTop/srcBorderedHeight;
	g_sdlAspectCorrectionRect.w = g_sdlAspectCorrectionBorderedRect.w*srcWidth/srcBorderedWidth;
	g_sdlAspectCorrectionRect.h = g_sdlAspectCorrectionBorderedRect.h*srcHeight/srcBorderedHeight;
	// As a little addition have this too - but:
	// 1. Use same dimensions independently of scaling.
	// 2. The dimensions of the controller UI are picked relatively to the host window's internal contents (without borders), not directly related to the client window size.
	// 3. Also taking the whole window into account (this doesn't depend on "screen mode", borders and more).
	int offset;
	int minWinDim = (winWidth >= winHeight) ? winHeight : winWidth;
	g_sdlControllerFaceButtonsRect.w = g_sdlControllerFaceButtonsRect.h = minWinDim/ALTCONTROLLER_FACEBUTTONS_SCREEN_DIM_RATIO;
	offset = minWinDim/(16*ALTCONTROLLER_FACEBUTTONS_SCREEN_DIM_RATIO);
	g_sdlControllerFaceButtonsRect.x = winWidth-g_sdlControllerFaceButtonsRect.w-offset;
	g_sdlControllerFaceButtonsRect.y = winHeight-g_sdlControllerFaceButtonsRect.h-offset;
	// Repeat for D-pad (same dimensions as the face buttons, other side)
	g_sdlControllerDpadRect.w = g_sdlControllerDpadRect.h = g_sdlControllerFaceButtonsRect.w;
	g_sdlControllerDpadRect.x = offset;
	g_sdlControllerDpadRect.y = g_sdlControllerFaceButtonsRect.y;
	// Also this - on-screen keyboard (somewhat different because the keyboard is rectangular, but not square-shaped)
	g_sdlControllerTextInputRect.w = minWinDim;
	g_sdlControllerTextInputRect.h = g_sdlControllerTextInputRect.w * ALTCONTROLLER_KEYBOARD_HEIGHT / ALTCONTROLLER_KEYBOARD_WIDTH;
	g_sdlControllerTextInputRect.x = (winWidth-g_sdlControllerTextInputRect.w)/2;
	g_sdlControllerTextInputRect.y = winHeight-g_sdlControllerTextInputRect.h;
}
*/
void BE_ST_SetScreenStartAddress(uint16_t crtc)
{
	g_sdlScreenStartAddress = crtc;
	g_sdlDoRefreshGfxOutput = true;
}

uint8_t *BE_ST_GetTextModeMemoryPtr(void)
{
	return g_sdlVidMem.text;
}




static uint32_t g_sdlEGACurrBGRAPaletteAndBorder[17], g_sdlEGACurrBGRAPaletteAndBorderCache[17];

/* Gets a value represeting 6 EGA signals determining a color number and
 * returns it in a "Blue Green Red Intensity" 4-bit format.
 * Usually, the 6 signals represented by the given input mean:
 * "Blue Green Red Secondary-Blue Secondary-Green Secondary-Red". However, for
 * the historical reason of compatibility with CGA monitors, on the 200-lines
 * modes used by Keen the Secondary-Green signal is treated as an Intensity
 * one and the two other intensity signals are ignored.
 */
static int BEL_ST_ConvertEGASignalToEGAEntry(int color)
{
	return (color & 7) | ((color & 16) >> 1);
}


void BE_ST_SetBorderColor(uint8_t color)
{
	g_sdlEGACurrBGRAPaletteAndBorder[16] = g_sdlEGABGRAScreenColors[BEL_ST_ConvertEGASignalToEGAEntry(color)];
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGASetPaletteAndBorder(const uint8_t *palette)
{
	for (int entry = 0; entry < 16; ++entry)
	{
		g_sdlEGACurrBGRAPaletteAndBorder[entry] =  g_sdlEGABGRAScreenColors[BEL_ST_ConvertEGASignalToEGAEntry(palette[entry])];
	}
	g_sdlEGACurrBGRAPaletteAndBorder[16] = g_sdlEGABGRAScreenColors[BEL_ST_ConvertEGASignalToEGAEntry(palette[16])];
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGASetPelPanning(uint8_t panning)
{
	g_sdlPelPanning = panning;
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGASetLineWidth(uint8_t widthInBytes)
{
	g_sdlLineWidth = widthInBytes;
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGASetSplitScreen(int16_t linenum)
{
	// VGA only for now (200-lines graphics modes)
	if (g_sdlTexHeight == GFX_TEX_HEIGHT)
	{
		// Because 200-lines modes are really double-scanned to 400,
		// a linenum of x was originally replaced with 2x-1 in id_vw.c.
		// In practice it should've probably been 2x+1, and this is how
		// we "correct" it here (one less black line in Catacomb Abyss
		// before gameplay begins in a map, above the status bar).
		g_sdlSplitScreenLine = linenum/2;
	}
	else
		g_sdlSplitScreenLine = linenum;
}

void BE_ST_EGAUpdateGFXByte(uint16_t destOff, uint8_t srcVal, uint16_t planeMask)
{
	if (planeMask & 1)
		g_sdlVidMem.egaGfx[0][destOff] = srcVal;
	if (planeMask & 2)
		g_sdlVidMem.egaGfx[1][destOff] = srcVal;
	if (planeMask & 4)
		g_sdlVidMem.egaGfx[2][destOff] = srcVal;
	if (planeMask & 8)
		g_sdlVidMem.egaGfx[3][destOff] = srcVal;
	g_sdlDoRefreshGfxOutput = true;
}

// Same as BE_ST_EGAUpdateGFXByte but picking specific bits out of each byte, and WITHOUT plane mask
void BE_ST_EGAUpdateGFXBits(uint16_t destOff, uint8_t srcVal, uint8_t bitsMask)
{
	g_sdlVidMem.egaGfx[0][destOff] = (g_sdlVidMem.egaGfx[0][destOff] & ~bitsMask) | (srcVal & bitsMask); 
	g_sdlVidMem.egaGfx[1][destOff] = (g_sdlVidMem.egaGfx[1][destOff] & ~bitsMask) | (srcVal & bitsMask); 
	g_sdlVidMem.egaGfx[2][destOff] = (g_sdlVidMem.egaGfx[2][destOff] & ~bitsMask) | (srcVal & bitsMask); 
	g_sdlVidMem.egaGfx[3][destOff] = (g_sdlVidMem.egaGfx[3][destOff] & ~bitsMask) | (srcVal & bitsMask); 
	g_sdlDoRefreshGfxOutput = true;
}

// Based on BE_Cross_LinearToWrapped_MemCopy
static void BEL_ST_LinearToEGAPlane_MemCopy(uint8_t *planeDstPtr, uint16_t planeDstOff, const uint8_t *linearSrc, uint16_t num)
{
	uint16_t bytesToEnd = 0x10000-planeDstOff;
	if (num <= bytesToEnd)
	{
		memcpy(planeDstPtr+planeDstOff, linearSrc, num);
	}
	else
	{
		memcpy(planeDstPtr+planeDstOff, linearSrc, bytesToEnd);
		memcpy(planeDstPtr, linearSrc+bytesToEnd, num-bytesToEnd);
	}
	g_sdlDoRefreshGfxOutput = true;
}

// Based on BE_Cross_WrappedToLinear_MemCopy
static void BEL_ST_EGAPlaneToLinear_MemCopy(uint8_t *linearDst, const uint8_t *planeSrcPtr, uint16_t planeSrcOff, uint16_t num)
{
	uint16_t bytesToEnd = 0x10000-planeSrcOff;
	if (num <= bytesToEnd)
	{
		memcpy(linearDst, planeSrcPtr+planeSrcOff, num);
	}
	else
	{
		memcpy(linearDst, planeSrcPtr+planeSrcOff, bytesToEnd);
		memcpy(linearDst+bytesToEnd, planeSrcPtr, num-bytesToEnd);
	}
	//No need to since we just read screen data
	//g_sdlDoRefreshGfxOutput = true;
}

// Based on BE_Cross_WrappedToWrapped_MemCopy
static void BEL_ST_EGAPlaneToEGAPlane_MemCopy(uint8_t *planeCommonPtr, uint16_t planeDstOff, uint16_t planeSrcOff, uint16_t num)
{
	uint16_t srcBytesToEnd = 0x10000-planeSrcOff;
	uint16_t dstBytesToEnd = 0x10000-planeDstOff;
	if (num <= srcBytesToEnd)
	{
		// Source is linear: Same as BE_Cross_LinearToWrapped_MemCopy here
		if (num <= dstBytesToEnd)
		{
			memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, num);
		}
		else
		{
			memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, dstBytesToEnd);
			memcpy(planeCommonPtr, planeCommonPtr+planeSrcOff+dstBytesToEnd, num-dstBytesToEnd);
		}
	}
	// Otherwise, check if at least the destination is linear
	else if (num <= dstBytesToEnd)
	{
		// Destination is linear: Same as BE_Cross_WrappedToLinear_MemCopy, non-linear source
		memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, srcBytesToEnd);
		memcpy(planeCommonPtr+planeDstOff+srcBytesToEnd, planeCommonPtr, num-srcBytesToEnd);
	}
	// BOTH buffers have wrapping. We don't check separately if
	// srcBytesToEnd==dstBytesToEnd (in such a case planeDstOff==planeSrcOff...)
	else if (srcBytesToEnd <= dstBytesToEnd)
	{
		memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, srcBytesToEnd);
		memcpy(planeCommonPtr+planeDstOff+srcBytesToEnd, planeCommonPtr, dstBytesToEnd-srcBytesToEnd);
		memcpy(planeCommonPtr, planeCommonPtr+(dstBytesToEnd-srcBytesToEnd), num-dstBytesToEnd);
	}
	else // srcBytesToEnd > dstBytesToEnd
	{
		memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, dstBytesToEnd);
		memcpy(planeCommonPtr, planeCommonPtr+planeSrcOff+dstBytesToEnd, srcBytesToEnd-dstBytesToEnd);
		memcpy(planeCommonPtr+(srcBytesToEnd-dstBytesToEnd), planeCommonPtr, num-srcBytesToEnd);
	}
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGAUpdateGFXBuffer(uint16_t destOff, const uint8_t *srcPtr, uint16_t num, uint16_t planeMask)
{
	if (planeMask & 1)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[0], destOff, srcPtr, num);
	if (planeMask & 2)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[1], destOff, srcPtr, num);
	if (planeMask & 4)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[2], destOff, srcPtr, num);
	if (planeMask & 8)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[3], destOff, srcPtr, num);
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGAUpdateGFXByteScrToScr(uint16_t destOff, uint16_t srcOff)
{
	g_sdlVidMem.egaGfx[0][destOff] = g_sdlVidMem.egaGfx[0][srcOff];
	g_sdlVidMem.egaGfx[1][destOff] = g_sdlVidMem.egaGfx[1][srcOff];
	g_sdlVidMem.egaGfx[2][destOff] = g_sdlVidMem.egaGfx[2][srcOff];
	g_sdlVidMem.egaGfx[3][destOff] = g_sdlVidMem.egaGfx[3][srcOff];
	g_sdlDoRefreshGfxOutput = true;
}

// Same as BE_ST_EGAUpdateGFXByteScrToScr but with plane mask (added for Catacomb Abyss vanilla bug reproduction/workaround)
void BE_ST_EGAUpdateGFXByteWithPlaneMaskScrToScr(uint16_t destOff, uint16_t srcOff, uint16_t planeMask)
{
	if (planeMask & 1)
		g_sdlVidMem.egaGfx[0][destOff] = g_sdlVidMem.egaGfx[0][srcOff];
	if (planeMask & 2)
		g_sdlVidMem.egaGfx[1][destOff] = g_sdlVidMem.egaGfx[1][srcOff];
	if (planeMask & 4)
		g_sdlVidMem.egaGfx[2][destOff] = g_sdlVidMem.egaGfx[2][srcOff];
	if (planeMask & 8)
		g_sdlVidMem.egaGfx[3][destOff] = g_sdlVidMem.egaGfx[3][srcOff];
	g_sdlDoRefreshGfxOutput = true;
}

// Same as BE_ST_EGAUpdateGFXByteScrToScr but picking specific bits out of each byte
void BE_ST_EGAUpdateGFXBitsScrToScr(uint16_t destOff, uint16_t srcOff, uint8_t bitsMask)
{
	g_sdlVidMem.egaGfx[0][destOff] = (g_sdlVidMem.egaGfx[0][destOff] & ~bitsMask) | (g_sdlVidMem.egaGfx[0][srcOff] & bitsMask); 
	g_sdlVidMem.egaGfx[1][destOff] = (g_sdlVidMem.egaGfx[1][destOff] & ~bitsMask) | (g_sdlVidMem.egaGfx[1][srcOff] & bitsMask); 
	g_sdlVidMem.egaGfx[2][destOff] = (g_sdlVidMem.egaGfx[2][destOff] & ~bitsMask) | (g_sdlVidMem.egaGfx[2][srcOff] & bitsMask); 
	g_sdlVidMem.egaGfx[3][destOff] = (g_sdlVidMem.egaGfx[3][destOff] & ~bitsMask) | (g_sdlVidMem.egaGfx[3][srcOff] & bitsMask); 
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGAUpdateGFXBufferScrToScr(uint16_t destOff, uint16_t srcOff, uint16_t num)
{
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[0], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[1], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[2], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem.egaGfx[3], destOff, srcOff, num);
	g_sdlDoRefreshGfxOutput = true;
}

uint8_t BE_ST_EGAFetchGFXByte(uint16_t destOff, uint16_t planenum)
{
	return g_sdlVidMem.egaGfx[planenum][destOff];
}

void BE_ST_EGAFetchGFXBuffer(uint8_t *destPtr, uint16_t srcOff, uint16_t num, uint16_t planenum)
{
	BEL_ST_EGAPlaneToLinear_MemCopy(destPtr, g_sdlVidMem.egaGfx[planenum], srcOff, num);
}

void BE_ST_EGAUpdateGFXPixel4bpp(uint16_t destOff, uint8_t color, uint8_t bitsMask)
{
	for (int currBitNum = 0, currBitMask = 1; currBitNum < 8; ++currBitNum, currBitMask <<= 1)
	{
		if (bitsMask & currBitMask)
		{
			g_sdlVidMem.egaGfx[0][destOff] &= ~currBitMask;
			g_sdlVidMem.egaGfx[0][destOff] |= ((color & 1) << currBitNum);
			g_sdlVidMem.egaGfx[1][destOff] &= ~currBitMask;
			g_sdlVidMem.egaGfx[1][destOff] |= (((color & 2) >> 1) << currBitNum);
			g_sdlVidMem.egaGfx[2][destOff] &= ~currBitMask;
			g_sdlVidMem.egaGfx[2][destOff] |= (((color & 4) >> 2) << currBitNum);
			g_sdlVidMem.egaGfx[3][destOff] &= ~currBitMask;
			g_sdlVidMem.egaGfx[3][destOff] |= (((color & 8) >> 3) << currBitNum);
		}
	}
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_EGAUpdateGFXPixel4bppRepeatedly(uint16_t destOff, uint8_t color, uint16_t count, uint8_t bitsMask)
{
	for (uint16_t loopVar = 0; loopVar < count; ++loopVar, ++destOff)
	{
		BE_ST_EGAUpdateGFXPixel4bpp(destOff, color, bitsMask);
	}
}

void BE_ST_EGAXorGFXByte(uint16_t destOff, uint8_t srcVal, uint16_t planeMask)
{
	if (planeMask & 1)
		g_sdlVidMem.egaGfx[0][destOff] ^= srcVal;
	if (planeMask & 2)
		g_sdlVidMem.egaGfx[1][destOff] ^= srcVal;
	if (planeMask & 4)
		g_sdlVidMem.egaGfx[2][destOff] ^= srcVal;
	if (planeMask & 8)
		g_sdlVidMem.egaGfx[3][destOff] ^= srcVal;
	g_sdlDoRefreshGfxOutput = true;
}

// Like BE_ST_EGAXorGFXByte, but:
// - OR instead of XOR.
// - All planes are updated.
// - Only specific bits are updated in each plane's byte.
void BE_ST_EGAOrGFXBits(uint16_t destOff, uint8_t srcVal, uint8_t bitsMask)
{
	g_sdlVidMem.egaGfx[0][destOff] |= (srcVal & bitsMask); 
	g_sdlVidMem.egaGfx[1][destOff] |= (srcVal & bitsMask); 
	g_sdlVidMem.egaGfx[2][destOff] |= (srcVal & bitsMask); 
	g_sdlVidMem.egaGfx[3][destOff] |= (srcVal & bitsMask); 
	g_sdlDoRefreshGfxOutput = true;
}



void BE_ST_CGAFullUpdateFromWrappedMem(const uint8_t *segPtr, const uint8_t *offInSegPtr, uint16_t byteLineWidth)
{
	const uint8_t *endSegPtr = segPtr + 0x10000;
	uint8_t *cgaHostPtr = g_sdlHostScrMem.cgaGfx, *cgaHostCachePtr = g_sdlHostScrMemCache.cgaGfx;
	// Remember this is a wrapped copy, offInSegPtr points inside a 65536-bytes long segment beginning at segPtr;
	// But we still want to skip some bytes, assuming byteLineWidth sources bytes per line (picking 80 out of these)

	int lineBytesLeft = byteLineWidth - GFX_TEX_WIDTH/4;
	for (int line = 0, byteInLine; line < GFX_TEX_HEIGHT; ++line)
	{
		for (byteInLine = 0; byteInLine < GFX_TEX_WIDTH/4; ++byteInLine, ++offInSegPtr, offInSegPtr = (offInSegPtr == endSegPtr) ? segPtr : offInSegPtr)
		{
			*cgaHostPtr = ((*offInSegPtr) & 0xC0) >> 6;
			g_sdlDoRefreshGfxOutput |= (*cgaHostPtr != *cgaHostCachePtr);
			*(cgaHostCachePtr++) = *(cgaHostPtr++);
			*cgaHostPtr = ((*offInSegPtr) & 0x30) >> 4;
			g_sdlDoRefreshGfxOutput |= (*cgaHostPtr != *cgaHostCachePtr);
			*(cgaHostCachePtr++) = *(cgaHostPtr++);
			*cgaHostPtr = ((*offInSegPtr) & 0x0C) >> 2;
			g_sdlDoRefreshGfxOutput |= (*cgaHostPtr != *cgaHostCachePtr);
			*(cgaHostCachePtr++) = *(cgaHostPtr++);
			*cgaHostPtr = ((*offInSegPtr) & 0x03);
			g_sdlDoRefreshGfxOutput |= (*cgaHostPtr != *cgaHostCachePtr);
			*(cgaHostCachePtr++) = *(cgaHostPtr++);
		}
		offInSegPtr += lineBytesLeft;
		if (offInSegPtr >= endSegPtr)
		{
			offInSegPtr = segPtr + (uint16_t)(offInSegPtr-segPtr);
		}
	}
}


void BE_ST_SetScreenMode(int mode)
{
    // Lock Rendering
    //SDL_SemWait( gpRenderLock );

    g_sdlDoRefreshGfxOutput = true;
    switch (mode)
    {
    case 3:
        g_sdlTexWidth = VGA_TXT_TEX_WIDTH;
        g_sdlTexHeight = VGA_TXT_TEX_HEIGHT;
        g_sdlTxtColor = 7;
        g_sdlTxtBackground = 0;
        g_sdlTxtCursorPosX = g_sdlTxtCursorPosY = 0;
        BE_ST_clrscr();
        g_sdlEGACurrBGRAPaletteAndBorder[16] = g_sdlEGABGRAScreenColors[0];
        break;
    case 4:
        g_sdlTexWidth = GFX_TEX_WIDTH;
        g_sdlTexHeight = GFX_TEX_HEIGHT;
        memset(g_sdlHostScrMem.cgaGfx, 0, sizeof(g_sdlHostScrMem.cgaGfx));
        g_sdlEGACurrBGRAPaletteAndBorder[16] = g_sdlEGABGRAScreenColors[0];
        g_sdlHostScrMemCache.cgaGfx[0] = g_sdlHostScrMem.cgaGfx[0]^0xFF; // Force refresh
        break;
    case 0xD:
        g_sdlTexWidth = GFX_TEX_WIDTH;
        g_sdlTexHeight = GFX_TEX_HEIGHT;
        memcpy(g_sdlEGACurrBGRAPaletteAndBorder, g_sdlEGABGRAScreenColors, sizeof(g_sdlEGABGRAScreenColors));
        g_sdlEGACurrBGRAPaletteAndBorder[16] = g_sdlEGABGRAScreenColors[0];
        g_sdlPelPanning = 0;
        g_sdlLineWidth = 40;
        g_sdlSplitScreenLine = -1;
        // HACK: Looks like this shouldn't be done if changing gfx->gfx
        if (g_sdlScreenMode != 0xE)
        {
            memset(g_sdlVidMem.egaGfx, 0, sizeof(g_sdlVidMem.egaGfx));
        }
        memset(g_sdlHostScrMem.egaGfx, 0, sizeof(g_sdlHostScrMem.egaGfx));
        g_sdlHostScrMemCache.egaGfx[0] = g_sdlHostScrMem.egaGfx[0]^0xFF; // Force refresh
        break;
    case 0xE:
        g_sdlTexWidth = 2*GFX_TEX_WIDTH;
        g_sdlTexHeight = GFX_TEX_HEIGHT;
        memcpy(g_sdlEGACurrBGRAPaletteAndBorder, g_sdlEGABGRAScreenColors, sizeof(g_sdlEGABGRAScreenColors));
        g_sdlEGACurrBGRAPaletteAndBorder[16] = g_sdlEGABGRAScreenColors[0];
        g_sdlPelPanning = 0;
        g_sdlLineWidth = 80;
        g_sdlSplitScreenLine = -1;
        // HACK: Looks like this shouldn't be done if changing gfx->gfx
        if (g_sdlScreenMode != 0xD)
        {
            memset(g_sdlVidMem.egaGfx,  0, sizeof(g_sdlVidMem.egaGfx));
        }
        memset(g_sdlHostScrMem.egaGfx,  0, sizeof(g_sdlHostScrMem.egaGfx));
        g_sdlHostScrMemCache.egaGfx[0] = g_sdlHostScrMem.egaGfx[0]^0xFF; // Force refresh
        break;
    }
    g_sdlScreenMode = mode;

    gDreamsEngine->setScreenMode(mode);

    // Unlock
    //SDL_SemPost( gpRenderLock );
}

void BE_ST_textcolor(int color)
{
	g_sdlTxtColor = color;
}

void BE_ST_textbackground(int color)
{
	g_sdlTxtBackground = color;
}

void BE_ST_clrscr(void)
{
	uint8_t *currMemByte = g_sdlVidMem.text;
	for (int i = 0; i < 2*TXT_COLS_NUM*TXT_ROWS_NUM; ++i)
	{
		*(currMemByte++) = ' ';
		*(currMemByte++) = g_sdlTxtColor | (g_sdlTxtBackground << 4);
	}
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_MoveTextCursorTo(int x, int y)
{
	g_sdlTxtCursorPosX = x;
	g_sdlTxtCursorPosY = y;
	g_sdlDoRefreshGfxOutput = true;
}

void BE_ST_ToggleTextCursor(bool isEnabled)
{
	g_sdlTxtCursorEnabled = isEnabled;
}

static uint8_t *BEL_ST_printchar(uint8_t *currMemByte, char ch, bool iscolored, bool requirecrchar)
{
	if (ch == '\t')
	{
		int nextCursorPosX = (g_sdlTxtCursorPosX & ~7) + 8;
		for (; g_sdlTxtCursorPosX != nextCursorPosX; ++g_sdlTxtCursorPosX)
		{
			*(currMemByte++) = ' ';
			*currMemByte = iscolored ? (g_sdlTxtColor | (g_sdlTxtBackground << 4)) : *currMemByte;
			++currMemByte;
		}
	}
	else if (ch == '\r')
	{
		if (!requirecrchar)
			return currMemByte; // Do nothing

		g_sdlTxtCursorPosX = 0; // Carriage return
		currMemByte = g_sdlVidMem.text + 2*TXT_COLS_NUM*g_sdlTxtCursorPosY;
	}
	else if (ch == '\n')
	{
		if (!requirecrchar)
		{
			g_sdlTxtCursorPosX = 0; // Carriage return
		}
		++g_sdlTxtCursorPosY;
		currMemByte = g_sdlVidMem.text + 2*(g_sdlTxtCursorPosX+TXT_COLS_NUM*g_sdlTxtCursorPosY);
	}
	else
	{
		*(currMemByte++) = ch;
		*currMemByte = iscolored ? (g_sdlTxtColor | (g_sdlTxtBackground << 4)) : *currMemByte;
		++currMemByte;
		++g_sdlTxtCursorPosX;
	}

	// Go to next line
	if (g_sdlTxtCursorPosX == TXT_COLS_NUM)
	{
		g_sdlTxtCursorPosX = 0; // Carriage return
		++g_sdlTxtCursorPosY; // Line feed
		currMemByte = g_sdlVidMem.text + 2*TXT_COLS_NUM*g_sdlTxtCursorPosY;
	}
	// Shift screen by one line
	if (g_sdlTxtCursorPosY == TXT_ROWS_NUM)
	{
		--g_sdlTxtCursorPosY;
		// Scroll one line down
		uint8_t lastAttr = g_sdlVidMem.text[sizeof(g_sdlVidMem.text)-1];
		memmove(g_sdlVidMem.text, g_sdlVidMem.text+2*TXT_COLS_NUM, sizeof(g_sdlVidMem.text)-2*TXT_COLS_NUM);
		currMemByte = g_sdlVidMem.text+sizeof(g_sdlVidMem.text)-2*TXT_COLS_NUM;
		// New empty line
		for (int i = 0; i < TXT_COLS_NUM; ++i)
		{
			*(currMemByte++) = ' ';
			*(currMemByte++) = lastAttr;
		}
		currMemByte -= 2*TXT_COLS_NUM; // Go back to beginning of line
	}

	return currMemByte;
}

void BE_ST_puts(const char *str)
{
	uint8_t *currMemByte = g_sdlVidMem.text + 2*(g_sdlTxtCursorPosX+TXT_COLS_NUM*g_sdlTxtCursorPosY);
	for (; *str; ++str)
	{
		currMemByte = BEL_ST_printchar(currMemByte, *str, true, false);
	}
	BEL_ST_printchar(currMemByte, '\n', true, false);

	g_sdlDoRefreshGfxOutput = true;
}

static void BEL_ST_vprintf_impl(const char *format, va_list args, bool iscolored, bool requirecrchar);

void BE_ST_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	BEL_ST_vprintf_impl(format, args, false, false);
	va_end(args);
}

void BE_ST_cprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	BEL_ST_vprintf_impl(format, args, true, true);
	va_end(args);
}

void BE_ST_vprintf(const char *format, va_list args)
{
	BEL_ST_vprintf_impl(format, args, false, false);
}

// There's no colored version of vprintf in the original codebase

static void BEL_ST_vprintf_impl(const char *format, va_list args, bool iscolored, bool requirecrchar)
{
	uint8_t *currMemByte = g_sdlVidMem.text + 2*(g_sdlTxtCursorPosX+TXT_COLS_NUM*g_sdlTxtCursorPosY);
	while (*format)
	{
		if (*format == '%')
		{
			switch (*(++format))
			{
			case '%':
				currMemByte = BEL_ST_printchar(currMemByte, '%', iscolored, requirecrchar);
				break;
			case 's':
			{
				for (const char *str = va_arg(args, char *); *str; ++str)
				{
					currMemByte = BEL_ST_printchar(currMemByte, *str, iscolored, requirecrchar);
				}
				break;
			}
			default:
			{
				// Do NOT constify this cause of hack...
				char errorMsg[] = "BEL_ST_vprintf_impl: Unsupported format specifier flag: X\n";
				errorMsg[strlen(errorMsg)-2] = *format; // Hack
				BE_ST_ExitWithErrorMsg(errorMsg);
			}
			}
		}
		else
		{
			currMemByte = BEL_ST_printchar(currMemByte, *format, iscolored, requirecrchar);
		}
		++format;
	}
}

/*
static void BEL_ST_FinishHostDisplayUpdate(void)
{
	g_sdlForceGfxControlUiRefresh = false;

	if (g_sdlShowControllerUI)
	{
		if (g_sdlFaceButtonsAreShown)
		{
			SDL_RenderCopy(g_sdlRenderer, g_sdlFaceButtonsTexture, NULL, &g_sdlControllerFaceButtonsRect);
		}
		if (g_sdlDpadIsShown)
		{
			SDL_RenderCopy(g_sdlRenderer, g_sdlDpadTexture, NULL, &g_sdlControllerDpadRect);
		}
		if (g_sdlTextInputUIIsShown)
		{
			SDL_RenderCopy(g_sdlRenderer, g_sdlTextInputTexture, NULL, &g_sdlControllerTextInputRect);
		}
	}

        //SDL_RenderPresent(g_sdlRenderer);
}
*/

/*
    if(SDL_MUSTLOCK(sfc)) SDL_LockSurface(sfc);

    // This makes the white pixel transparent
    Uint8 *pixel = (Uint8*)sfc->pixels;

    for( Uint16 y=0 ; y<sfc->h ; y++ )
    {
        for( Uint16 x=0 ; x<sfc->w ; x++ )
        {
            memcpy( &color, pixel, sfc->format->BytesPerPixel );

            SDL_GetRGBA( color, sfc->format, &r, &g, &b, &a );

            if( a>0 )
            {
                memcpy( pixel, &fgColor, sfc->format->BytesPerPixel );
            }

            pixel += sfc->format->BytesPerPixel;
        }
    }
    if(SDL_MUSTLOCK(sfc)) SDL_LockSurface(sfc);
 */



void updateTextMode(SDL_Surface *sfc)
{

    const auto VGA_TXT_CHAR_PIX_WIDTH = ((sfc->w)/TXT_COLS_NUM);
    const auto VGA_TXT_CHAR_PIX_HEIGHT = ((sfc->h)/TXT_ROWS_NUM);

    bool areBlinkingCharsShown = (((uint64_t)(70086*SDL_GetTicks()/1000)/(1000*VGA_TXT_BLINK_VERT_FRAME_RATE)) % 2);
    bool isBlinkingCursorShown = g_sdlTxtCursorEnabled && (((uint64_t)(70086*SDL_GetTicks()/1000)/(1000*VGA_TXT_CURSOR_BLINK_VERT_FRAME_RATE)) % 2);
    /*if (!g_sdlDoRefreshGfxOutput && (wereBlinkingCharsShown == areBlinkingCharsShown) && (wasBlinkingCursorShown == isBlinkingCursorShown))
    {
        if (g_sdlForceGfxControlUiRefresh)
            BEL_ST_FinishHostDisplayUpdate();
        return;
    }*/
    /// Do update
//        static bool wereBlinkingCharsShown = areBlinkingCharsShown;
//        static bool wasBlinkingCursorShown = isBlinkingCursorShown;

    void *pixels = sfc->pixels;
    //int pitch;
    //SDL_LockTexture(g_sdlTexture, NULL, &pixels, &pitch);

    uint8_t *screenPixelPtrYOffset = static_cast<uint8_t*>(pixels);
    int bpp = sfc->format->BytesPerPixel;
    // Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    uint8_t currChar;
    const uint8_t *currCharFontPtrVBase;
    uint32_t currBackgroundColor, currCharColor;
    uint8_t *currScrPixelPtrY;
    int txtByteCounter = 0;
    int currCharPixX, currCharPixY;
    for (int currCharY = 0, currCharX; currCharY < TXT_ROWS_NUM; ++currCharY)
    {
        // Draw striped lines
        uint8_t *screenPixelPtrXOffset = screenPixelPtrYOffset;

        for (currCharX = 0; currCharX < TXT_COLS_NUM; ++currCharX)
        {
            currChar = g_sdlVidMem.text[txtByteCounter];
            // Luckily, the width*height product is always divisible by 8...
            // Note that the actual width is always 8,
            // even in VGA mode. We convert to 9 while drawing.
            currCharFontPtrVBase = g_vga_8x16TextFont + currChar*16*8;
            ++txtByteCounter;
            currBackgroundColor = g_sdlEGABGRAScreenColors[(g_sdlVidMem.text[txtByteCounter] >> 4) & 7];
            // Should the character blink?
            if (!(g_sdlVidMem.text[txtByteCounter] & 0x80) || areBlinkingCharsShown)
                currCharColor = g_sdlEGABGRAScreenColors[g_sdlVidMem.text[txtByteCounter] & 15];
            else
                currCharColor = currBackgroundColor;
            ++txtByteCounter;

            currScrPixelPtrY = screenPixelPtrXOffset;

            const uint8_t *currCharFontPtrV;

            const int vgaTxtCharPixH = VGA_TXT_CHAR_PIX_HEIGHT;

            for (currCharPixY = 0 ; currCharPixY < vgaTxtCharPixH ; ++currCharPixY)
            {
                // NOTE: The char width is actually 8
                // in both of the EGA and VGA fonts. On the
                // VGA case, the 9th pixel is determined
                // according to the 8th and char number.
                uint8_t *currScrPixelPtrX = currScrPixelPtrY;

                currCharFontPtrV = currCharFontPtrVBase + ((currCharPixY*16)/vgaTxtCharPixH)*8;

                Uint32 color;

                //for (currCharPixX = 0; currCharPixX < 8; ++currCharPixX, ++currCharFontPtr)
                for (currCharPixX = 0; currCharPixX < VGA_TXT_CHAR_PIX_WIDTH; ++currCharPixX)
                {
                    const uint8_t *currCharFontPtrH = currCharFontPtrV + (currCharPixX*8)/VGA_TXT_CHAR_PIX_WIDTH;

                    color = (*currCharFontPtrH) ? currCharColor : currBackgroundColor;

                    memcpy(currScrPixelPtrX, &color, sizeof(Uint32));

                    currScrPixelPtrX += bpp;
                }

                // Add an extra 9th column on VGA
                color = ((currChar < 192) || (currChar > 223)) ? currBackgroundColor : (color-1);

                memcpy(currScrPixelPtrX, &color, sizeof(Uint32));

                currScrPixelPtrY += sfc->pitch;
            }

            screenPixelPtrXOffset += bpp*VGA_TXT_CHAR_PIX_WIDTH;
        }
        // Go to the character right below current one
        screenPixelPtrYOffset += sfc->pitch*VGA_TXT_CHAR_PIX_HEIGHT;
    }
    // Finish with outputting the cursor if required
    currCharColor = g_sdlEGABGRAScreenColors[g_sdlVidMem.text[1+((TXT_COLS_NUM*g_sdlTxtCursorPosY+g_sdlTxtCursorPosX)<<1)] & 15];

    // Blinking cursor
    if (isBlinkingCursorShown)
    {
        uint32_t *screenPixelPtr = (uint32_t *)pixels+g_sdlTexWidth;
        screenPixelPtr += g_sdlTxtCursorPosY*VGA_TXT_CHAR_PIX_HEIGHT*g_sdlTexWidth;
        screenPixelPtr += g_sdlTxtCursorPosX*VGA_TXT_CHAR_PIX_WIDTH;
        // Out of 3 last scanlines of char, draw to the first 2.
        screenPixelPtr += (VGA_TXT_CHAR_PIX_HEIGHT-3)*g_sdlTexWidth;
        for (currCharPixY = 0; currCharPixY < 2; currCharPixY++)
        {
            for (currCharPixX = 0; currCharPixX < VGA_TXT_CHAR_PIX_WIDTH; currCharPixX++, screenPixelPtr++)
                *screenPixelPtr = currCharColor;
            screenPixelPtr += g_sdlTexWidth - VGA_TXT_CHAR_PIX_WIDTH;
        }
    }

    g_sdlDoRefreshGfxOutput = false;
}


void updateEGAGraphics(SDL_Surface *sfc)
{
    uint32_t ratioW = (sfc->w)/(g_sdlTexWidth);
    uint32_t ratioH = (sfc->h)/(g_sdlTexHeight);

    if(ratioW == 1 && ratioH == 1) // Optimized. One layer less and
                                   // fewer loops to run through
    {
        if(SDL_MUSTLOCK(sfc)) SDL_LockSurface(sfc);

        void *pixels = sfc->pixels;

        uint32_t *currPixPtr = (uint32_t *)pixels;

        uint16_t currLineFirstByte = (g_sdlScreenStartAddress + g_sdlPelPanning/8) % 0x10000;
        uint8_t panningWithinInByte = g_sdlPelPanning%8;
        bool doUpdate = false;
        for (int line = 0, col; line < g_sdlTexHeight; ++line)
        {
            uint8_t currBitNum = 7-panningWithinInByte, currBitMask = 1<<currBitNum;
            uint16_t currByte = currLineFirstByte;

            for (col = 0; col < g_sdlTexWidth; ++col)
            {
                const uint8_t thePixel =
                           ((g_sdlVidMem.egaGfx[0][currByte]&currBitMask)>>currBitNum) |
                          (((g_sdlVidMem.egaGfx[1][currByte]&currBitMask)>>currBitNum)<<1) |
                          (((g_sdlVidMem.egaGfx[2][currByte]&currBitMask)>>currBitNum)<<2) |
                          (((g_sdlVidMem.egaGfx[3][currByte]&currBitMask)>>currBitNum)<<3);

                *currPixPtr = g_sdlEGACurrBGRAPaletteAndBorder[thePixel];

                currPixPtr++;

                if (currBitNum == 0)
                {
                    ++currByte;
                    currByte %= 0x10000;
                    currBitNum = 7;
                    currBitMask = 0x80;
                }
                else
                {
                    --currBitNum;
                    currBitMask >>= 1;
                }
                if (col == 8*g_sdlLineWidth)
                {
                    ++col;
                    break;
                }
            }

            if (g_sdlSplitScreenLine == line)
            {
                currLineFirstByte = 0; // NEXT line begins split screen, NOT g_sdlSplitScreenLine
            }
            else
            {
                currLineFirstByte += g_sdlLineWidth;
                currLineFirstByte %= 0x10000;
            }
        }

        if(SDL_MUSTLOCK(sfc)) SDL_UnlockSurface(sfc);

        if (!doUpdate)
        {
            int paletteAndBorderEntry;
            for (paletteAndBorderEntry = 0; paletteAndBorderEntry < 17; ++paletteAndBorderEntry)
            {
                if (g_sdlEGACurrBGRAPaletteAndBorder[paletteAndBorderEntry] != g_sdlEGACurrBGRAPaletteAndBorderCache[paletteAndBorderEntry])
                {
                    g_sdlEGACurrBGRAPaletteAndBorderCache[paletteAndBorderEntry] = g_sdlEGACurrBGRAPaletteAndBorder[paletteAndBorderEntry];
                    doUpdate = true;
                }
            }
            if (!doUpdate)
            {
                g_sdlDoRefreshGfxOutput = false;
                return;
            }
        }

        g_sdlDoRefreshGfxOutput = false;

    }
    else // unoptimized. This happens when the game resolution does not match the keen dreams resolution
    {

        uint16_t currLineFirstByte = (g_sdlScreenStartAddress + g_sdlPelPanning/8) % 0x10000;
        uint8_t panningWithinInByte = g_sdlPelPanning%8;
        uint8_t *currPalPixPtrBase, *currPalPixCachePtr;
        bool doUpdate = false;
        for (int line = 0, col; line < g_sdlTexHeight; ++line)
        {
            uint8_t currBitNum = 7-panningWithinInByte, currBitMask = 1<<currBitNum;
            uint16_t currByte = currLineFirstByte;
            currPalPixPtrBase = g_sdlHostScrMem.egaGfx + line*g_sdlTexWidth;
            currPalPixCachePtr = g_sdlHostScrMemCache.egaGfx + line*g_sdlTexWidth;
            for (col = 0; col < g_sdlTexWidth; ++col, ++currPalPixPtrBase)
            {
                *currPalPixPtrBase = ((g_sdlVidMem.egaGfx[0][currByte]&currBitMask)>>currBitNum) |
                        (((g_sdlVidMem.egaGfx[1][currByte]&currBitMask)>>currBitNum)<<1) |
                        (((g_sdlVidMem.egaGfx[2][currByte]&currBitMask)>>currBitNum)<<2) |
                        (((g_sdlVidMem.egaGfx[3][currByte]&currBitMask)>>currBitNum)<<3);

                doUpdate |= (*currPalPixPtrBase != *currPalPixCachePtr);
                *currPalPixCachePtr = *currPalPixPtrBase;

                if (currBitNum == 0)
                {
                    ++currByte;
                    currByte %= 0x10000;
                    currBitNum = 7;
                    currBitMask = 0x80;
                }
                else
                {
                    --currBitNum;
                    currBitMask >>= 1;
                }
                if (col == 8*g_sdlLineWidth)
                {
                    ++col;
                    ++currPalPixPtrBase;
                    ++currPalPixCachePtr;
                    break;
                }
            }
            // Just if this makes sense... (FIXME: Check!)
            for (; col < g_sdlTexWidth; ++col, ++currPalPixPtrBase, ++currPalPixCachePtr)
            {
                doUpdate |= (*currPalPixCachePtr);
            }

            memset(currPalPixPtrBase, 0, sizeof(uint8_t));
            memset(currPalPixCachePtr, 0, sizeof(uint8_t));

            if (g_sdlSplitScreenLine == line)
            {
                currLineFirstByte = 0; // NEXT line begins split screen, NOT g_sdlSplitScreenLine
            }
            else
            {
                currLineFirstByte += g_sdlLineWidth;
                currLineFirstByte %= 0x10000;
            }
        }

        if (!doUpdate)
        {
            int paletteAndBorderEntry;
            for (paletteAndBorderEntry = 0; paletteAndBorderEntry < 17; ++paletteAndBorderEntry)
            {
                if (g_sdlEGACurrBGRAPaletteAndBorder[paletteAndBorderEntry] != g_sdlEGACurrBGRAPaletteAndBorderCache[paletteAndBorderEntry])
                {
                    g_sdlEGACurrBGRAPaletteAndBorderCache[paletteAndBorderEntry] = g_sdlEGACurrBGRAPaletteAndBorder[paletteAndBorderEntry];
                    doUpdate = true;
                }
            }
            if (!doUpdate)
            {
                g_sdlDoRefreshGfxOutput = false;
                return;
            }
        }

        if(SDL_MUSTLOCK(sfc)) SDL_LockSurface(sfc);

        void *pixels = sfc->pixels;

        uint32_t *currPixPtrBase = (uint32_t *)pixels;
        currPalPixPtrBase = g_sdlHostScrMem.egaGfx;

        // Can we skip loops by just blitting the surfaces?

        uint8_t *currPalPixPtr = currPalPixPtrBase;

        /*if( ratioW == 1 && ratioH == 1)
    {
        memcpy(currPixPtrBase, );
    }
    else*/
        {
            for(int pixY=0 ; pixY < sfc->h ; pixY++ )
            {
                uint32_t *currPixPtr = currPixPtrBase + pixY*sfc->w;

                const int yStart = (pixY/ratioH)*GFX_TEX_WIDTH;

                for(uint32_t pixX=0 ; pixX < ratioW*GFX_TEX_WIDTH ; pixX++ )
                {
                    currPalPixPtr = currPalPixPtrBase + yStart + pixX/ratioW;

                    *currPixPtr = g_sdlEGACurrBGRAPaletteAndBorder[*currPalPixPtr];

                    currPixPtr++;
                }
            }
        }

        g_sdlDoRefreshGfxOutput = false;

        if(SDL_MUSTLOCK(sfc)) SDL_UnlockSurface(sfc);

    }
}

void BEL_ST_UpdateHostDisplay(SDL_Surface *sfc)
{    
    if (g_sdlScreenMode == 3) // Text mode TODO: Broken for some reason, check!
	{        
        if(SDL_MUSTLOCK(sfc)) SDL_LockSurface(sfc);

        updateTextMode(sfc);

        if(SDL_MUSTLOCK(sfc)) SDL_UnlockSurface(sfc);
	}
    else if (g_sdlScreenMode == 4) // CGA graphics TODO: revisit when in CGA Mode
	{
        /*if (!g_sdlDoRefreshGfxOutput)
		{
			if (g_sdlForceGfxControlUiRefresh)
				BEL_ST_FinishHostDisplayUpdate();
			return;
        }*/
		// That's easy now since there isn't a lot that can be done...
        /*void *pixels;
		int pitch;
        SDL_LockTexture(g_sdlTexture, NULL, &pixels, &pitch);
		uint32_t *currPixPtr = (uint32_t *)pixels;
		uint8_t *currPalPixPtr = g_sdlHostScrMem.cgaGfx;
		for (int pixnum = 0; pixnum < GFX_TEX_WIDTH*GFX_TEX_HEIGHT; ++pixnum, ++currPixPtr, ++currPalPixPtr)
		{
			*currPixPtr = g_sdlCGAGfxBGRAScreenColors[*currPalPixPtr];
        }*/
	}
	else // EGA graphics mode 0xD or 0xE
	{
        updateEGAGraphics(sfc);
    }   
}




}
