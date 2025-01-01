cmake_minimum_required(VERSION 3.5.0)
project(test_log VERSION 0.1.0 LANGUAGES C CXX)
include_directories(${PROJECT_SOURCE_DIR}/include)

link_directories(${PROJECT_SOURCE_DIR}/lib)


## .h+.cpp
# set(SOURCES
#     main.cpp
#     src/MY_LOG.cpp
# )

# add_executable(MY_LOG ${SOURCES})


## .h + 静态库
add_executable(test_log src/main.cpp)
target_link_libraries(test_log MY_LOG)




# #编译静态库方法：
# add_library(MY_LOG src/MY_LOG.cpp)