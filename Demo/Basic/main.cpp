#include <iostream>
#include <iso646.h>

#include "OPoint3.h"
#include "OWindow.h"
#include "OSystem.h"
#include "OCamera.h"
#include "OShader.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/string_cast.hpp"

using namespace omega::geometry;
using namespace omega::render;
using namespace omega::system;

class MainWindow : public OWindow
{
public:
    MainWindow() : OWindow()  {
        camera = OCameraPtr(new OCamera());
        shader = OShaderPtr(new OShader(4,2,"shaders/vertexShader.vs","shaders/fragmentShader.vs"));

        camera->setWindowSize(m_width, m_height);
        camera->look(OVector (0.0f, 0.0f, -3.0f), OVector(0.0f, 0.0f, 0.0f));

        setCamera( camera );
    }

    void keyEvent(int type, int state, int key, bool repeat)
    {
        switch(key)
        {
        case KEY_ESCAPE:
            if(state == KEY_STATE_DOWN)
                quit(); 
            break;
        case KEY_F: 
            if (state == KEY_STATE_DOWN)
                setFullscreen(not isFullscreen()); 
            break;
        default: OWindow::keyEvent(type,state,key,repeat); break;
        }
    }

    bool render(){
        OMatrix<float> identity(true);

        shader->setMat4fv(camera->ProjectionMatrix,"u_projectionMat44");
        shader->setMat4fv(camera->ViewMatrix,"u_viewMat44");
        shader->setMat4fv(identity, "u_modelMat44");
        shader->use();

        return OWindow::render();
    }

private:
    OCameraPtr camera;
    OShaderPtr shader;
};


int main()
{
    OSystem::init();

    auto window = new MainWindow();

    while( window->isRuning() ){
        window->process();
        window->clear();
        window->render();
        window->swap();
    }

    return 0;
}
