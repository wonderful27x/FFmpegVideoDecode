
cmake_minimum_required(VERSION 3.4.1)

#引入头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

##指定需要编译的源文件
file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/*.cpp)

add_library( # Sets the name of the library.
        wonderful
        SHARED
        ${SOURCE})

find_library( # Sets the name of the path variable.
        log-lib
        log)

#设置查找路径
set(lib_search_path "${CMAKE_SOURCE_DIR}/../../../libs/${ANDROID_ABI}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${lib_search_path}")

target_link_libraries( # Specifies the target library.
        wonderful
        avfilter
        avformat
        avcodec
        avutil
        swresample
        swscale
        android
        z
        OpenSLES
        ${log-lib})

#############################坑坑坑##################################
#在连接库时一定要加入android和z库，否则编译编译无法通过