/**
 * GsApp.h
 *
 *  Created on: 01.05.2009
 *      Author: gerstrong
 */

#ifndef GSAPP_H_
#define GSAPP_H_

#include <base/GsEvent.h>
#include <base/GsEngine.h>
#include <base/Singleton.h>

#include <memory>


#define gApp GsApp::get()

// Forward declaration
class GsApp;

// App State has an event sink that is registered thought the constructor and teared down through the deconstrcutor
class GsAppEventSink : public GsEventSink
{

public:
    GsAppEventSink(GsApp* pApp) :
        mpApp(pApp) {}

    void pumpEvent(const CEvent *ev) override;

private:
    GsApp* mpApp;

};

class GsApp : public GsSingleton<GsApp>
{
public:
    GsApp();
    ~GsApp();
	
	bool init(int argc, char *argv[]);
    bool loadDrivers();

    void setEngine(GsEngine *engPtr);
	
    void runMainCycle();
	void cleanup();

    void pollEvents();

    void ponder(const float deltaT);

    void render();

    bool mustShutdown(){ return (mpCurEngine==nullptr); }

    void pumpEvent(const CEvent *evPtr);


private:

    std::unique_ptr<GsEngine> mpCurEngine;
    GsAppEventSink mSink;
};

// It's a simple quit event which will force CG to close the App
struct GMQuit : CEvent {};

#endif /* GSAPP_H */
