#ifndef MXSEMAPHORE_H
#define MXSEMAPHORE_H

#include "mxtypes.h"

#include <SDL2/SDL_mutex.h>

// VTABLE: LEGO1 0x100dccf0
// SIZE 0x08
class MxSemaphore {
public:
	MxSemaphore();

	// FUNCTION: LEGO1 0x100c87e0
	~MxSemaphore() { SDL_DestroySemaphore(m_semaphore); }

	virtual MxResult Init(MxU32 p_initialCount, MxU32 p_maxCount);

	void Wait();
	void Release();

private:
	SDL_semaphore* m_semaphore;
};

#endif // MXSEMAPHORE_H
