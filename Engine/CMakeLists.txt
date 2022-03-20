cmake_minimum_required(VERSION 3.21)
project(oEngine)

set(CMAKE_CXX_STANDARD 14)

add_library(oEngine
        Source/OPoint2.cpp
        Source/OPoint3.cpp
        Source/OPlane.cpp
        Source/OMatrix.cpp
        Header/OPoint2.h
        Header/OPoint3.h
        Header/OMath.h
        Header/OPlane.h
        Header/OMatrix.h
        Header/OPoint4.h Header/OBox.h Header/OSphere.h Header/OBoxBase.h)

include_directories(oEngine PUBLIC Header)
