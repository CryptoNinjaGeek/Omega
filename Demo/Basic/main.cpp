#include <iostream>

#include "OPoint2.h"
#include "OPoint3.h"
#include "OMatrix.h"
#include "OWindow.h"

using namespace omega::geometry;
using namespace omega::render;

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
};


int main()
{
    OWindow::init();
    OWindow::run();
    
    return 0;
}
