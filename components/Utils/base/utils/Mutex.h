/*
 OpenLieroX
 
 Mutex wrapper
 
 created 10-02-2009 by Karel Petranek
 code under LGPL
 */

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <SDL_mutex.h>

#define INVALID_THREAD_ID (Uint32)-1

class Condition;

// Mutex wrapper class with some extra debugging checks
class Mutex  {
	friend class Condition;
private:
	SDL_mutex *m_mutex;
	
#ifdef DEBUG
	volatile Uint32 m_lockedThread;  // Thread that keeps the lock
#endif
	
public:
	/*#ifdef DEBUG
	 Mutex();
	 ~Mutex();
	 void lock();
	 void unlock();
	 
	 static void test();
	 #else */
	Mutex()			{ m_mutex = SDL_CreateMutex(); }
	~Mutex()		{ SDL_DestroyMutex(m_mutex); }
	void lock()		{ SDL_LockMutex(m_mutex); }
	void unlock()	{ SDL_UnlockMutex(m_mutex); }
	//#endif
	
	struct ScopedLock {
		Mutex& mutex;
		ScopedLock(Mutex& m) : mutex(m) { mutex.lock(); }
		~ScopedLock() { mutex.unlock(); }
	};
};

#endif // __MUTEX_H__
