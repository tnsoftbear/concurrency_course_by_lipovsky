# Twist-Course

add_library(course-test STATIC sources/env.cpp)

target_include_directories(course-test PUBLIC include)
target_link_libraries(course-test twist)
