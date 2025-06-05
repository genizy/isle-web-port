#ifndef ISLEDEBUG_H
#define ISLEDEBUG_H

#include <stdbool.h> // for bool

typedef union SDL_Event SDL_Event;

#ifdef ISLE_DEBUG

extern bool IsleDebug_Enabled();

extern void IsleDebug_SetEnabled(bool);

extern void IsleDebug_Init();

extern bool IsleDebug_Event(SDL_Event* event);

extern void IsleDebug_Render();

extern void IsleDebug_SetPaused(bool v);

extern bool IsleDebug_Paused();

extern bool IsleDebug_StepModeEnabled();

extern void IsleDebug_ResetStepMode();

#else

// Provide empty inline implementations when ISLE_DEBUG is not defined

static inline bool IsleDebug_Enabled(void) { return false; }
static inline void IsleDebug_SetEnabled(bool v) { (void)v; }
static inline void IsleDebug_Init(void) { }
static inline bool IsleDebug_Event(SDL_Event* event) { (void)event; return false; }
static inline void IsleDebug_Render(void) { }
static inline void IsleDebug_SetPaused(bool v) { (void)v; }
static inline bool IsleDebug_Paused(void) { return false; }
static inline bool IsleDebug_StepModeEnabled(void) { return false; }
static inline void IsleDebug_ResetStepMode(void) { }

#endif

#endif // ISLEDEBUG_H
