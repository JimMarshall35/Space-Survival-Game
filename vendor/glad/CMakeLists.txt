project(glad)

file(GLOB_RECURSE SOURCES *.cpp *.c)
file(GLOB_RECURSE INCS *.h)

add_library(glad STATIC ${SOURCES} ${INCS})

include_directories(${glad_SOURCE_DIR}/include)