cmake_minimum_required(VERSION 3.3)
project(agent)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES INIParser.cpp INIParser.h Logging.cpp Logging.h MutexLock.h macro_define.h FileBackup.cpp FileBackup.h MD5.cpp MD5.h Util.cpp Util.h main.h main.cpp main.h)
add_executable(agent ${SOURCE_FILES})