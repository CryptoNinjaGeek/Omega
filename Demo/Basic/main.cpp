#include <iostream>

#include "OPoint3.h"
#include "OWindow.h"
#include "OSystem.h"
#include "OCamera.h"

using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;

class MainWindow : public OWindow
{
public:
    MainWindow(){
        auto camera = std::shared_ptr<OCamera>(new OCamera());

        camera->look(OVector (0.0f, 1.75f, 7.0f), OVector(0.0f, 1.75f, 0.0f));

        setCamera( camera );
    }

    void keyEvent(int type, int state, int key, bool repeat)
    {
        if( state != KEY_STATE_DOWN)
            return;

        switch(key)
        {
        case KEY_Q: quit(); break;
        case KEY_F: setFullscreen(not isFullscreen()); break;
        default: OWindow::keyEvent(type,state,key,repeat); break;
        }
    }

    bool render(){
        return OWindow::render();
    }
};


int main()
{
    OSystem::init();

    auto window = new MainWindow();

    while( window->isRuning() ){
        window->process();
        window->render();
    }

    return 0;
}
