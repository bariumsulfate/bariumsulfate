CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (BARIUMSULFATE)

ADD_DEFINITIONS(-std=c++11 -Wall -Wextra)

SET (Boost_USE_MULTITHREADED  ON)
FIND_PACKAGE (Boost COMPONENTS thread system REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

FILE (GLOB_RECURSE SOURCE_FILES src/ *.cpp)
FILE (GLOB_RECURSE HEADER_FILES src/ *.hpp)

INCLUDE_DIRECTORIES (src/)
INCLUDE_DIRECTORIES (${Boost_INCLUDE_DIRS})
ADD_EXECUTABLE (bariumsulfate ${SOURCE_FILES} ${HEADER_FILES})
TARGET_LINK_LIBRARIES(bariumsulfate
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
