/*
 *  ThreadPool.h
 *  OpenLieroX
 *
 *  Created by Albert Zeyer on 08.02.09.
 *  code under LGPL
 *
 */

#ifndef THREADPOOL_H__
#define THREADPOOL_H__

#include <set>
#include <string>

struct SDL_mutex;
struct SDL_cond;
struct SDL_Thread;
class ThreadPool;
typedef int (*ThreadFunc) (void*);
struct CmdLineIntf;

struct Action {
	virtual ~Action() {}
	virtual int handle() = 0;
};

struct ThreadPoolItem {
	ThreadPool* pool;
	SDL_Thread* thread;
	std::string name;
	bool working;
	bool finished;
	bool headless;
	SDL_cond* finishedSignal;
	SDL_cond* readyForNewWork;
	int ret;
};

class ThreadPool {
private:
	SDL_mutex* mutex;
	SDL_cond* awakeThread;
	SDL_cond* threadStartedWork;
	SDL_cond* threadStatusChanged;
	Action* nextAction; bool nextIsHeadless; std::string nextName;
	ThreadPoolItem* nextData;
	bool quitting;
    std::set< ThreadPoolItem* > availableThreads;
    std::set< ThreadPoolItem* > usedThreads;
	void prepareNewThread();
	static int threadWrapper(void* param);
	SDL_mutex* startMutex;
public:
	ThreadPool(unsigned int size = 5);
	~ThreadPool();
	
    ThreadPoolItem* start(ThreadFunc fct, void* param = NULL, const std::string& name = "unknown worker");
	// WARNING: if you set headless, you cannot use wait() and you should not save the returned ThreadPoolItem*
	ThreadPoolItem* start(Action* act, const std::string& name = "unknown worker", bool headless = false); // ThreadPool will own and free the Action
	bool finalizeIfReady(ThreadPoolItem* thread, int* status = NULL);
	bool wait(ThreadPoolItem* thread, int* status = NULL);
	bool waitAll();
	void dumpState(CmdLineIntf& cli) const;
};

extern ThreadPool* threadPool;

void InitThreadPool(unsigned int size = 5);
void UnInitThreadPool();



template<typename _T>
struct _ThreadFuncWrapper {
	typedef int (_T::* FuncPointer)();
	template< FuncPointer _func >
		struct Wrapper {
			static int wrapper(void* obj) {
				return (((_T*)obj) ->* _func)();
			}
			
			static ThreadPoolItem* startThread(_T* const obj, const std::string& name) {
				return threadPool->start((ThreadFunc)&wrapper, (void*)obj, name);
			}
		};
};

#define StartMemberFuncInThread(T, memberfunc, name) \
_ThreadFuncWrapper<T>::Wrapper<&memberfunc>::startThread(this, name)


#endif // THREADPOOL_H__

