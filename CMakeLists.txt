project(camera_demo)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g -ggdb -fPIC")
SET(CMAKE_C_FLAGS_RELEASE  "$ENV{CFLAGS} -O3 -Wall -fPIC")

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -fPIC -std=c++17")
SET(CMAKE_CXX_FLAGS_RELEASE  "$ENV{CXXFLAGS} -O3 -Wall -fPIC -std=c++17")

# add_subdirectory(./rtp_netplay)
add_definitions(-D_UNIX)

message(STATUS "--------------build netplay--------------")
include_directories(/media/xtt/hdd/3rdlib/ffmpeg/include)
link_directories(/media/xtt/hdd/3rdlib/ffmpeg/lib)

include_directories(./rtp_netplay)
aux_source_directory(./rtp_netplay netplay_list)
add_library(rtp_netplay SHARED ${netplay_list})
target_link_libraries(rtp_netplay avcodec avfilter avformat avutil swresample swscale)


message(STATUS "--------------build demo--------------")

link_directories(/media/xtt/hdd/3rdlib/opencv4.4/lib)
include_directories(/media/xtt/hdd/3rdlib/opencv4.4/include/opencv4)

set(CMAKE_PREFIX_PATH "/media/xtt/hdd/3rdlib/Qt5.12.9/5.12.9/gcc_64/lib/cmake")

message(STATUS "Looking for Qt...")
find_package(Qt5 REQUIRED Core Widgets Test)
if (${Qt5_FOUND})
    message(STATUS "Found Qt " ${Qt5_VERSION})
else()
    message(STATUS "Couldn't find Qt")
endif()

include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5_INCLUDE_DIRS})
link_libraries(
    Qt5::Core
    Qt5::Test 
    Qt5::Gui 
    Qt5::Widgets
)
qt5_wrap_ui(ui_list camera_show/camera_show.ui)
qt5_wrap_cpp(moc_list camera_show/camera_show.h)
qt5_add_resources(qrc_list camera_show/camera_show.qrc)

include_directories(./camera_show)

aux_source_directory(./camera_show src_list)

link_libraries(pthread)

add_executable(${PROJECT_NAME} ${src_list} ${moc_list} ${ui_list})
target_link_libraries(${PROJECT_NAME} opencv_highgui opencv_imgproc opencv_core opencv_imgcodecs rtp_netplay)




