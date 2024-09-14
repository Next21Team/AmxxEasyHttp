cmake_minimum_required(VERSION 3.20)
project(parson VERSION 1.2.4)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(${PROJECT_NAME} STATIC
        src/parson.cpp
)
add_library(parson::parson ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
        include
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
            -fno-rtti  # disable rtti to reduce the library size
            -Werror    # treat all warnings as error
    )
elseif (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            /GR-  # disable rtti to reduce the library size
            /WX   # treat all warnings as error
            /EHsc # NEVER catch SEH exceptions by catch(...); c functions cannot throw exceptions
    )
endif ()