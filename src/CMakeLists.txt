SET(COMPILE_DEFINITIONS -Werror)
add_definitions(-std=c++11 -O2)

find_package( OpenCV REQUIRED )

add_executable(../picam Main.cpp Control.cpp LowPass.cpp cRIO.cpp ImageProc.cpp PID.cpp)

target_link_libraries(../picam ${OpenCV_LIBS})
