#include <iostream>

#include "OPoint2.h"
#include "OPoint3.h"
#include "OMatrix.h"

using namespace omega::geometry;

int main() {
    OPoint2<double> pt;
    OPoint3<double> pt3;
    OMatrix<double> m;

    pt.set(10,10);
    pt3.set(10,10,10);
    m.identity();
    m.dumpMatrix("Identity");
    
    return 0;
}
