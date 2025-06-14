#include "isleapp.h"

#include "3dmanager/lego3dmanager.h"
#include "decomp.h"
#include "isledebug.h"
#include "legoanimationmanager.h"
#include "legobuildingmanager.h"
#include "legogamestate.h"
#include "legoinputmanager.h"
#include "legomain.h"
#include "legomodelpresenter.h"
#include "legopartpresenter.h"
#include "legoutils.h"
#include "legovideomanager.h"
#include "legoworldpresenter.h"
#include "misc.h"
#include "mxbackgroundaudiomanager.h"
#include "mxdirectx/mxdirect3d.h"
#include "mxdsaction.h"
#include "mxmisc.h"
#include "mxomnicreateflags.h"
#include "mxomnicreateparam.h"
#include "mxstreamer.h"
#include "mxticklemanager.h"
#include "mxtimer.h"
#include "mxtransitionmanager.h"
#include "mxutilities.h"
#include "mxvariabletable.h"
#include "res/isle_bmp.h"
#include "res/resource.h"
#include "roi/legoroi.h"
#include "viewmanager/viewmanager.h"

#define SDL_MAIN_USE_CALLBACKS
#include "SDL_properties.h"
#include "SDL_compat.h"
#include "SDL_iostream_compat.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <errno.h>
#include <iniparser.h>
#include <stdlib.h>
#include <time.h>
#include <emscripten.h>

DECOMP_SIZE_ASSERT(IsleApp, 0x8c)

// GLOBAL: ISLE 0x410030
IsleApp* g_isle = NULL;

// GLOBAL: ISLE 0x410034
MxU8 g_mousedown = FALSE;

// GLOBAL: ISLE 0x410038
MxU8 g_mousemoved = FALSE;

// GLOBAL: ISLE 0x41003c
MxS32 g_closed = FALSE;

// GLOBAL: ISLE 0x410050
MxS32 g_rmDisabled = FALSE;

// GLOBAL: ISLE 0x410054
MxS32 g_waitingForTargetDepth = TRUE;

// GLOBAL: ISLE 0x410058
MxS32 g_targetWidth = 640;

// GLOBAL: ISLE 0x41005c
MxS32 g_targetHeight = 480;

// GLOBAL: ISLE 0x410060
MxS32 g_targetDepth = 16;

// GLOBAL: ISLE 0x410064
MxS32 g_reqEnableRMDevice = FALSE;

// STRING: ISLE 0x4101dc
#define WINDOW_TITLE "LEGO®"

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

bool running = true;

// FUNCTION: ISLE 0x401000
IsleApp::IsleApp()
{
	m_hdPath = NULL;
	m_cdPath = NULL;
	m_deviceId = NULL;
	m_savePath = NULL;
	m_fullScreen = FALSE;
	m_flipSurfaces = FALSE;
	m_backBuffersInVram = TRUE;
	m_using8bit = FALSE;
	m_using16bit = TRUE;
	m_hasLightSupport = FALSE;
	m_drawCursor = FALSE;
	m_use3dSound = TRUE;
	m_useMusic = TRUE;
	m_useJoystick = FALSE;
	m_joystickIndex = 0;
	m_wideViewAngle = TRUE;
	m_islandQuality = 1;
	m_islandTexture = 1;
	m_gameStarted = FALSE;
	m_frameDelta = 10;
	m_windowActive = TRUE;

#ifdef COMPAT_MODE
	{
		MxRect32 r(0, 0, 639, 479);
		MxVideoParamFlags flags;
		m_videoParam = MxVideoParam(r, NULL, 1, flags);
	}
#else
	m_videoParam = MxVideoParam(MxRect32(0, 0, 639, 479), NULL, 1, MxVideoParamFlags());
#endif
	m_videoParam.Flags().Set16Bit(MxDirectDraw::GetPrimaryBitDepth() == 16);

	m_windowHandle = NULL;
	m_cursorArrow = NULL;
	m_cursorBusy = NULL;
	m_cursorNo = NULL;
	m_cursorCurrent = NULL;

	LegoOmni::CreateInstance();

	m_iniPath = NULL;
}

// FUNCTION: ISLE 0x4011a0
IsleApp::~IsleApp()
{
	if (LegoOmni::GetInstance()) {
		Close();
		MxOmni::DestroyInstance();
	}

	if (m_hdPath) {
		delete[] m_hdPath;
	}

	if (m_cdPath) {
		delete[] m_cdPath;
	}

	if (m_deviceId) {
		delete[] m_deviceId;
	}

	if (m_savePath) {
		delete[] m_savePath;
	}

	if (m_mediaPath) {
		delete[] m_mediaPath;
	}
}

// FUNCTION: ISLE 0x401260
void IsleApp::Close()
{
	MxDSAction ds;
	ds.SetUnknown24(-2);

	if (Lego()) {
		GameState()->Save(0);
		if (InputManager()) {
			InputManager()->QueueEvent(c_notificationKeyPress, 0, 0, 0, SDLK_SPACE);
		}

		VideoManager()->Get3DManager()->GetLego3DView()->GetViewManager()->RemoveAll(NULL);

		Lego()->RemoveWorld(ds.GetAtomId(), ds.GetObjectId());
		Lego()->DeleteObject(ds);
		TransitionManager()->SetWaitIndicator(NULL);
		Lego()->Resume();

		while (Streamer()->Close(NULL) == SUCCESS) {
		}

		while (Lego() && !Lego()->DoesEntityExist(ds)) {
			Timer()->GetRealTime();
			TickleManager()->Tickle();
		}
	}
}

// FUNCTION: ISLE 0x4013b0
MxS32 IsleApp::SetupLegoOmni()
{
	MxS32 result = FALSE;

#ifdef COMPAT_MODE
	MxS32 failure;
	{
		MxOmniCreateParam param(m_mediaPath, m_windowHandle, m_videoParam, MxOmniCreateFlags());
		failure = Lego()->Create(param) == FAILURE;
	}
#else
	MxS32 failure =
		Lego()->Create(MxOmniCreateParam(m_mediaPath, m_windowHandle, m_videoParam, MxOmniCreateFlags())) == FAILURE;
#endif

	if (!failure) {
		VariableTable()->SetVariable("ACTOR_01", "");
		TickleManager()->SetClientTickleInterval(VideoManager(), 10);
		result = TRUE;
	}

	return result;
}

// FUNCTION: ISLE 0x401560
void IsleApp::SetupVideoFlags(
	MxS32 fullScreen,
	MxS32 flipSurfaces,
	MxS32 backBuffers,
	MxS32 using8bit,
	MxS32 using16bit,
	MxS32 hasLightSupport,
	MxS32 param_7,
	MxS32 wideViewAngle,
	char* deviceId
)
{
	m_videoParam.Flags().SetFullScreen(fullScreen);
	m_videoParam.Flags().SetFlipSurfaces(flipSurfaces);
	m_videoParam.Flags().SetBackBuffers(!backBuffers);
	m_videoParam.Flags().SetLacksLightSupport(!hasLightSupport);
	m_videoParam.Flags().SetF1bit7(param_7);
	m_videoParam.Flags().SetWideViewAngle(wideViewAngle);
	m_videoParam.Flags().SetF2bit1(1);
	m_videoParam.SetDeviceName(deviceId);
	if (using8bit) {
		m_videoParam.Flags().Set16Bit(0);
	}
	if (using16bit) {
		m_videoParam.Flags().Set16Bit(1);
	}
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	*appstate = NULL;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK)) {
		char buffer[256];
		SDL_snprintf(
			buffer,
			sizeof(buffer),
			"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again.\nSDL error: %s",
			SDL_GetError()
		);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "LEGO® Island Error", buffer, NULL);
		return SDL_APP_FAILURE;
	}

	// [library:window]
	// Original game checks for an existing instance here.
	// We don't really need that.

	// Create global app instance
	g_isle = new IsleApp();

	if (g_isle->ParseArguments(argc, argv) != SUCCESS) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"LEGO® Island Error",
			"\"LEGO® Island\" failed to start.  Invalid CLI arguments.",
			window
		);
		return SDL_APP_FAILURE;
	}

	// Create window
	if (g_isle->SetupWindow() != SUCCESS) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"LEGO® Island Error",
			"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again.",
			window
		);
		return SDL_APP_FAILURE;
	}

	// Get reference to window
	*appstate = g_isle->GetWindowHandle();
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	if (g_closed) {
		return SDL_APP_SUCCESS;
	}

	if (!g_isle->Tick()) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"LEGO® Island Error",
			"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again."
			"\nFailed to initialize; see logs for details",
			NULL
		);
		return SDL_APP_FAILURE;
	}

	if (!g_closed) {
		IsleDebug_Render();

		if (g_reqEnableRMDevice) {
			g_reqEnableRMDevice = FALSE;
			VideoManager()->EnableRMDevice();
			g_rmDisabled = FALSE;
			Lego()->Resume();
		}

		if (g_closed) {
			return SDL_APP_SUCCESS;
		}

		if (g_mousedown && g_mousemoved && g_isle) {
			if (!g_isle->Tick()) {
				SDL_ShowSimpleMessageBox(
					SDL_MESSAGEBOX_ERROR,
					"LEGO® Island Error",
					"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again."
					"\nFailed to initialize; see logs for details",
					NULL
				);
				return SDL_APP_FAILURE;
			}
		}

		if (g_mousemoved) {
			g_mousemoved = FALSE;
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	if (!g_isle) {
		return SDL_APP_CONTINUE;
	}

	if (IsleDebug_Event(event)) {
		return SDL_APP_CONTINUE;
	}

	// [library:window]
	// Remaining functionality to be implemented:
	// Full screen - crashes when minimizing/maximizing, but this will probably be fixed once DirectDraw is replaced
	// WM_TIMER - use SDL_Timer functionality instead

	switch (event->type) {
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		if (!IsleDebug_Enabled()) {
			g_isle->SetWindowActive(TRUE);
			Lego()->Resume();
		}
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (!IsleDebug_Enabled()) {
			g_isle->SetWindowActive(FALSE);
			Lego()->Pause();
		}
		break;
	case SDL_WINDOWEVENT_CLOSE:
		if (!g_closed) {
			delete g_isle;
			g_isle = NULL;
			g_closed = TRUE;
		}
		break;
	case SDL_KEYDOWN: {
		if (event->key.repeat) {
			break;
		}

		SDL_Keycode keyCode = event->key.keysym.sym;
		if (InputManager()) {
			InputManager()->QueueEvent(c_notificationKeyPress, keyCode, 0, 0, keyCode);
		}
		break;
	}
	case SDL_MOUSEMOTION:
		g_mousemoved = TRUE;

		if (InputManager()) {
			InputManager()->QueueEvent(
				c_notificationMouseMove,
				IsleApp::MapMouseButtonFlagsToModifier(event->motion.state),
				event->motion.x,
				event->motion.y,
				0
			);
		}

		if (g_isle->GetDrawCursor()) {
			VideoManager()->MoveCursor(Min((MxS32) event->motion.x, 639), Min((MxS32) event->motion.y, 479));
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		g_mousedown = TRUE;

		if (InputManager()) {
			InputManager()->QueueEvent(
				c_notificationButtonDown,
				IsleApp::MapMouseButtonFlagsToModifier(SDL_GetMouseState(NULL, NULL)),
				event->button.x,
				event->button.y,
				0
			);
		}
		break;
	case SDL_MOUSEBUTTONUP:
		g_mousedown = FALSE;

		if (InputManager()) {
			InputManager()->QueueEvent(
				c_notificationButtonUp,
				IsleApp::MapMouseButtonFlagsToModifier(SDL_GetMouseState(NULL, NULL)),
				event->button.x,
				event->button.y,
				0
			);
		}
		break;
	case SDL_QUIT:
		return SDL_APP_SUCCESS;
		break;
	}

	if (event->user.type == g_legoSdlEvents.m_windowsMessage) {
		switch (event->user.code) {
		case WM_ISLE_SETCURSOR:
			g_isle->SetupCursor((Cursor) (uintptr_t) event->user.data1);
			break;
		case WM_TIMER:
			if (InputManager()) {
				InputManager()->QueueEvent(c_notificationTimer, (MxU8) (uintptr_t) event->user.data1, 0, 0, 0);
			}
			break;
		default:
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown SDL Windows message: 0x%" SDL_PRIx32, event->user.code);
			break;
		}
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	if (appstate != NULL) {
		SDL_DestroyWindow((SDL_Window*) appstate);
	}

	SDL_Quit();
}

MxU8 IsleApp::MapMouseButtonFlagsToModifier(Uint32 p_flags)
{
	// [library:window]
	// Map button states to Windows button states (LegoEventNotificationParam)
	// Not mapping mod keys SHIFT and CTRL since they are not used by the game.

	MxU8 modifier = 0;
	if (p_flags & SDL_BUTTON_LMASK) {
		modifier |= LegoEventNotificationParam::c_lButtonState;
	}
	if (p_flags & SDL_BUTTON_RMASK) {
		modifier |= LegoEventNotificationParam::c_rButtonState;
	}

	return modifier;
}

// FUNCTION: ISLE 0x4023e0
MxResult IsleApp::SetupWindow()
{
    if (!LoadConfig()) {
        return FAILURE;
    }

    SetupVideoFlags(
        m_fullScreen,
        m_flipSurfaces,
        m_backBuffersInVram,
        m_using8bit,
        m_using16bit,
        m_hasLightSupport,
        FALSE,
        m_wideViewAngle,
        m_deviceId
    );

    MxOmni::SetSound3D(m_use3dSound);

    srand(time(NULL));

    // Setup cursors
    m_cursorCurrent = m_cursorArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    m_cursorBusy = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    m_cursorNo = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
    SDL_SetCursor(m_cursorCurrent);

    // SDL2 window creation
    Uint32 windowFlags = SDL_WINDOW_SHOWN;
    if (m_fullScreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }

    window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        g_targetWidth,
        g_targetHeight,
        windowFlags
    );

    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create SDL window: %s", SDL_GetError());
        return FAILURE;
    }

#ifdef MINIWIN
    m_windowHandle = reinterpret_cast<HWND>(window);
#else
    // For SDL2, getting native window handle is platform-dependent.
    // You can retrieve it using SDL_GetWindowWMInfo (if needed), or set to nullptr for now.
    m_windowHandle = nullptr;
#endif

    if (!m_windowHandle) {
        // For SDL2, often you don't need native handle or it's nullptr by default.
        // You can skip this failure return or remove the check entirely if you don't rely on it.
        // Otherwise, just comment this out:
        // return FAILURE;
    }

    SDL_RWops* icon_stream = SDL_RWFromMem(isle_bmp, isle_bmp_len);

    if (icon_stream) {
        SDL_Surface* icon = SDL_LoadBMP_RW(icon_stream, 1);
        if (icon) {
            SDL_SetWindowIcon(window, icon);
            SDL_FreeSurface(icon);
        }
        else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load icon: %s", SDL_GetError());
        }
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open SDL_RWops for icon: %s", SDL_GetError());
    }

    if (!SetupLegoOmni()) {
        return FAILURE;
    }

    GameState()->SetSavePath(m_savePath);
    GameState()->SerializePlayersInfo(LegoStorage::c_read);
    GameState()->SerializeScoreHistory(LegoStorage::c_read);

    MxS32 iVar10;
    switch (m_islandQuality) {
        case 0:
            iVar10 = 1;
            break;
        case 1:
            iVar10 = 2;
            break;
        default:
            iVar10 = 100;
            break;
    }

    MxS32 uVar1 = (m_islandTexture == 0);
    LegoModelPresenter::configureLegoModelPresenter(uVar1);
    LegoPartPresenter::configureLegoPartPresenter(uVar1, iVar10);
    LegoWorldPresenter::configureLegoWorldPresenter(m_islandQuality);
    LegoBuildingManager::configureLegoBuildingManager(m_islandQuality);
    LegoROI::configureLegoROI(iVar10);
    LegoAnimationManager::configureLegoAnimationManager(m_islandQuality);

    if (LegoOmni::GetInstance()) {
        if (LegoOmni::GetInstance()->GetInputManager()) {
            LegoOmni::GetInstance()->GetInputManager()->SetUseJoystick(m_useJoystick);
            LegoOmni::GetInstance()->GetInputManager()->SetJoystickIndex(m_joystickIndex);
        }
    }

    IsleDebug_Init();

	if (!window) {
    	SDL_Log("Failed to create SDL window: %s", SDL_GetError());
    	return FAILURE;
	}
    return SUCCESS;
}

// FUNCTION: ISLE 0x4028d0
bool IsleApp::LoadConfig()
{
	char* prefPath = SDL_GetPrefPath("isledecomp", "isle");
	char* iniConfig;
	if (m_iniPath) {
		iniConfig = new char[strlen(m_iniPath) + 1];
		strcpy(iniConfig, m_iniPath);
	}
	else if (prefPath) {
		iniConfig = new char[strlen(prefPath) + strlen("isle.ini") + 1]();
		strcat(iniConfig, prefPath);
		strcat(iniConfig, "isle.ini");
	}
	else {
		iniConfig = new char[strlen("isle.ini") + 1];
		strcpy(iniConfig, "isle.ini");
	}
	SDL_Log("Reading configuration from \"%s\"", iniConfig);

	dictionary* dict = iniparser_load(iniConfig);

	// [library:config]
	// Load sane defaults if dictionary failed to load
	if (!dict) {
		if (m_iniPath) {
			SDL_Log("Invalid config path '%s'", m_iniPath);
			return false;
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading sane defaults");
		FILE* iniFP = fopen(iniConfig, "wb");

		if (!iniFP) {
			SDL_LogError(
				SDL_LOG_CATEGORY_APPLICATION,
				"Failed to write config at '%s': %s",
				iniConfig,
				strerror(errno)
			);
			return false;
		}

		dict = iniparser_load(iniConfig);
		iniparser_set(dict, "isle", NULL);

		iniparser_set(dict, "isle:diskpath", SDL_GetBasePath());
		iniparser_set(dict, "isle:cdpath", MxOmni::GetCD());
		iniparser_set(dict, "isle:mediapath", SDL_GetBasePath());
		iniparser_set(dict, "isle:savepath", prefPath);

		iniparser_set(dict, "isle:Flip Surfaces", m_flipSurfaces ? "true" : "false");
		iniparser_set(dict, "isle:Full Screen", m_fullScreen ? "true" : "false");
		iniparser_set(dict, "isle:Wide View Angle", m_wideViewAngle ? "true" : "false");

		iniparser_set(dict, "isle:3DSound", m_use3dSound ? "true" : "false");
		iniparser_set(dict, "isle:Music", m_useMusic ? "true" : "false");

		iniparser_set(dict, "isle:UseJoystick", m_useJoystick ? "true" : "false");
		iniparser_set(dict, "isle:JoystickIndex", m_joystickIndex ? "true" : "false");
		iniparser_set(dict, "isle:Draw Cursor", m_drawCursor ? "true" : "false");

		iniparser_set(dict, "isle:Back Buffers in Video RAM", "-1");

		iniparser_set(dict, "isle:Island Quality", "1");
		iniparser_set(dict, "isle:Island Texture", "1");

		iniparser_dump_ini(dict, iniFP);
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "New config written at '%s'", iniConfig);
		fclose(iniFP);
	}

	const char* hdPath = iniparser_getstring(dict, "isle:diskpath", SDL_GetBasePath());
	m_hdPath = new char[strlen(hdPath) + 1];
	strcpy(m_hdPath, hdPath);
	MxOmni::SetHD(m_hdPath);

	const char* cdPath = iniparser_getstring(dict, "isle:cdpath", MxOmni::GetCD());
	m_cdPath = new char[strlen(cdPath) + 1];
	strcpy(m_cdPath, cdPath);
	MxOmni::SetCD(m_cdPath);

	const char* mediaPath = iniparser_getstring(dict, "isle:mediapath", hdPath);
	m_mediaPath = new char[strlen(mediaPath) + 1];
	strcpy(m_mediaPath, mediaPath);

	m_flipSurfaces = iniparser_getboolean(dict, "isle:Flip Surfaces", m_flipSurfaces);
	m_fullScreen = iniparser_getboolean(dict, "isle:Full Screen", m_fullScreen);
	m_wideViewAngle = iniparser_getboolean(dict, "isle:Wide View Angle", m_wideViewAngle);
	m_use3dSound = iniparser_getboolean(dict, "isle:3DSound", m_use3dSound);
	m_useMusic = iniparser_getboolean(dict, "isle:Music", m_useMusic);
	m_useJoystick = iniparser_getboolean(dict, "isle:UseJoystick", m_useJoystick);
	m_joystickIndex = iniparser_getint(dict, "isle:JoystickIndex", m_joystickIndex);
	m_drawCursor = iniparser_getboolean(dict, "isle:Draw Cursor", m_drawCursor);

	MxS32 backBuffersInVRAM = iniparser_getboolean(dict, "isle:Back Buffers in Video RAM", -1);
	if (backBuffersInVRAM != -1) {
		m_backBuffersInVram = !backBuffersInVRAM;
	}

	MxS32 bitDepth = iniparser_getint(dict, "isle:Display Bit Depth", -1);
	if (bitDepth != -1) {
		if (bitDepth == 8) {
			m_using8bit = TRUE;
		}
		else if (bitDepth == 16) {
			m_using16bit = TRUE;
		}
	}

	m_islandQuality = iniparser_getint(dict, "isle:Island Quality", 1);
	m_islandTexture = iniparser_getint(dict, "isle:Island Texture", 1);

	const char* deviceId = iniparser_getstring(dict, "isle:3D Device ID", NULL);
	if (deviceId != NULL) {
		m_deviceId = new char[strlen(deviceId) + 1];
		strcpy(m_deviceId, deviceId);
	}

	// [library:config]
	// The original game does not save any data if no savepath is given.
	// Instead, we use SDLs prefPath as a default fallback and always save data.
	const char* savePath = iniparser_getstring(dict, "isle:savepath", prefPath);
	m_savePath = new char[strlen(savePath) + 1];
	strcpy(m_savePath, savePath);

	iniparser_freedict(dict);
	delete[] iniConfig;
	SDL_free(prefPath);

	return true;
}

// FUNCTION: ISLE 0x402c20
inline bool IsleApp::Tick()
{
	// GLOBAL: ISLE 0x4101c0
	static MxLong g_lastFrameTime = 0;

	// GLOBAL: ISLE 0x4101bc
	static MxS32 g_startupDelay = 200;

	if (IsleDebug_Paused() && IsleDebug_StepModeEnabled()) {
		IsleDebug_SetPaused(false);
	}

	if (!m_windowActive) {
		SDL_Delay(1);
		return true;
	}

	if (!Lego()) {
		return true;
	}
	if (!TickleManager()) {
		return true;
	}
	if (!Timer()) {
		return true;
	}

	MxLong currentTime = Timer()->GetRealTime();
	if (currentTime < g_lastFrameTime) {
		g_lastFrameTime = -m_frameDelta;
	}

	if (m_frameDelta + g_lastFrameTime >= currentTime) {
		SDL_Delay(1);
		return true;
	}

	if (!Lego()->IsPaused()) {
		TickleManager()->Tickle();
	}
	g_lastFrameTime = currentTime;

	if (IsleDebug_StepModeEnabled()) {
		IsleDebug_SetPaused(true);
		IsleDebug_ResetStepMode();
	}

	if (g_startupDelay == 0) {
		return true;
	}

	g_startupDelay--;
	if (g_startupDelay != 0) {
		return true;
	}

	LegoOmni::GetInstance()->CreateBackgroundAudio();
	BackgroundAudioManager()->Enable(m_useMusic);

	MxStreamController* stream = Streamer()->Open("\\lego\\scripts\\isle\\isle", MxStreamer::e_diskStream);
	MxDSAction ds;

	if (!stream) {
		stream = Streamer()->Open("\\lego\\scripts\\nocd", MxStreamer::e_diskStream);
		if (!stream) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open NOCD.si: Streamer failed to load");
			return false;
		}

		ds.SetAtomId(stream->GetAtom());
		ds.SetUnknown24(-1);
		ds.SetObjectId(0);
		VideoManager()->EnableFullScreenMovie(TRUE, TRUE);

		if (Start(&ds) != SUCCESS) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open NOCD.si: Failed to start initial action");
			return false;
		}
	}
	else {
		ds.SetAtomId(stream->GetAtom());
		ds.SetUnknown24(-1);
		ds.SetObjectId(0);
		if (Start(&ds) != SUCCESS) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open ISLE.si: Failed to start initial action");
			return false;
		}
		m_gameStarted = TRUE;
	}

	return true;
}

// FUNCTION: ISLE 0x402e80
void IsleApp::SetupCursor(Cursor p_cursor)
{
	switch (p_cursor) {
	case e_cursorArrow:
		m_cursorCurrent = m_cursorArrow;
		break;
	case e_cursorBusy:
		m_cursorCurrent = m_cursorBusy;
		break;
	case e_cursorNo:
		m_cursorCurrent = m_cursorNo;
		break;
	case e_cursorNone:
		m_cursorCurrent = NULL;
	case e_cursorUnused3:
	case e_cursorUnused4:
	case e_cursorUnused5:
	case e_cursorUnused6:
	case e_cursorUnused7:
	case e_cursorUnused8:
	case e_cursorUnused9:
	case e_cursorUnused10:
		break;
	}

	if (m_cursorCurrent != NULL) {
		SDL_SetCursor(m_cursorCurrent);
		SDL_ShowCursor(SDL_ShowCursor(SDL_QUERY) >= 0 ? SDL_DISABLE : SDL_ENABLE);
	}
	else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

MxResult IsleApp::ParseArguments(int argc, char** argv)
{

	for (int i = 1, consumed; i < argc; i += consumed) {
		consumed = -1;

		if (strcmp(argv[i], "--ini") == 0 && i + 1 < argc) {
			m_iniPath = argv[i + 1];
			consumed = 2;
		}
		else if (strcmp(argv[i], "--debug") == 0) {
#ifdef ISLE_DEBUG
			IsleDebug_SetEnabled(true);
#else
			SDL_Log("isle is built without debug support. Ignoring --debug argument.");
#endif
			consumed = 1;
		}
		if (consumed <= 0) {
			SDL_Log("Invalid argument(s): %s", argv[i]);
			return FAILURE;
		}
	}
	return SUCCESS;
}

int main(int argc, char** argv)
{
    void* appstate = nullptr;

    if (SDL_AppInit(&appstate, argc, argv) != SDL_APP_CONTINUE) {
        return 1;
    }

    emscripten_set_main_loop([] {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (SDL_AppEvent(g_isle ? g_isle->GetWindowHandle() : nullptr, &event) == SDL_APP_SUCCESS) {
                emscripten_cancel_main_loop();
                return;
            }
        }

        if (SDL_AppIterate(g_isle ? g_isle->GetWindowHandle() : nullptr) != SDL_APP_CONTINUE) {
            emscripten_cancel_main_loop();
        }
    }, 0, 1);

    return 0;
}
