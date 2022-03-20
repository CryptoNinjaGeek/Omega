#include <iostream>

#include "OPoint2.h"
#include "OPoint3.h"
#include "OMatrix.h"
#include "OWindow.h"
#include "OSystem.h"

#include <iostream>

using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;

class MainWindow : public OWindow
{
public:
    MainWindow(){
        OPoint2<double> pt;
        OPoint3<double> pt3;
        OMatrix<double> m;

        pt.set(10,10);
        pt3.set(10,10,10);
        m.identity();
        m.dumpMatrix("Identity");
    }

    void keyEvent(int type, int state, int key, bool repeat)
    {
        if(repeat || state != KEY_STATE_DOWN)
            return;

        switch(key)
        {
        case KEY_Q:
            quit();
            break;
        case KEY_F:
            setFullscreen(not isFullscreen());
            break;
        }
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
