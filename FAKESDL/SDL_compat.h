#ifndef SDL_COMPAT_H
#define SDL_COMPAT_H

#include <SDL.h>

typedef SDL_RWops SDL_IOStream;

typedef enum {
    SDL_APP_SUCCESS = 0,
    SDL_APP_FAILURE = 1,
    SDL_APP_CONTINUE = 2
} SDL_AppResult;

#define SDL_WINDOWEVENT_FOCUS_GAINED SDL_WINDOWEVENT_FOCUS_GAINED

#endif // SDL_COMPAT_H
