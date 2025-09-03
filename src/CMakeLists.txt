# src/CMakeLists.txt

set(subproj ${proj})

target_include_directories(${subproj} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${subproj} PRIVATE
    main.cc
)

add_subdirectory(platform)
