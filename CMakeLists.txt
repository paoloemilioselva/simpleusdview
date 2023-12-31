cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET "simpleusdview")
add_definitions(-DNOMINMAX)
add_definitions(-DTBB_USE_DEBUG)

project(
    ${TARGET}
    DESCRIPTION "Simple GLFW usdview"
    LANGUAGES CXX C
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON) # fPIC

# Add OpenGL
find_package(OpenGL REQUIRED)

# Add GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory( "../glfw" "../glfw" )

# check target config
if(NOT TARGET_CONFIG)
    set( TARGET_CONFIG "usd-23.11")
endif()

string(REPLACE "-" ";" _t_config ${TARGET_CONFIG})
list(LENGTH _t_config _t_config_length)
if(_t_config_length LESS 2)
    MESSAGE(FATAL_ERROR "Wrong TARGET_CONFIG value " ${TARGET_CONFIG} )
endif()

list(GET _t_config 0 _target_name )

set( USD_LIBS "" )
set( USD_EMBEDDED FALSE )

if( _target_name MATCHES "houdini" )
    list(GET _t_config 1 HOUDINI_MAJ_MIN )
    list(GET _t_config 2 HOUDINI_PATCH )
    list(GET _t_config 3 HOUDINI_PYTHON_VERSION )
    set( HOUDINI_VERSION "Houdini ${HOUDINI_MAJ_MIN}.${HOUDINI_PATCH}" )
    set( HOUDINI_ROOT "C:\\Program Files\\Side Effects Software\\${HOUDINI_VERSION}" )

elseif( _target_name MATCHES "usd" )
    list(GET _t_config 1 USD_VERSION )
    if(_t_config_length EQUAL 3)
        set( USD_EMBEDDED TRUE )
    endif()
    set(USD_ROOT "C:\\dev\\usd-${USD_VERSION}" )
    set(USD_ROOT_FOR_BATCH ${USD_ROOT})
    if( USD_EMBEDDED )
        set(USD_ROOT_FOR_BATCH "%CURRENT_FOLDER%usd-${USD_VERSION}")
    endif()

else()
    MESSAGE(FATAL_ERROR "Invalid target " ${_target_name})
endif()

include(cmake/FindUsd.cmake)

include(cmake/FindTBB.cmake)
#find_package(TBB REQUIRED)

add_executable( 
    ${TARGET}
    main.cpp
    font.h 
)

target_compile_options(
        ${TARGET}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4273 /Zi /experimental:external /external:W0 /D> 
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/Ob0 /Od>
        )

target_link_options(
        ${TARGET}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/ignore:4217 /ignore:4049> 
        )

target_link_directories(
    ${TARGET}
    PRIVATE
    ${USD_LIBRARY_DIR}
)

target_link_libraries(
    ${TARGET}
    PUBLIC
    OpenGL::GL
    glfw
    ${USD_LIBS}
)

if( USD_EMBEDDED )
    message(STATUS "USD EMBEDDED") 
    install(DIRECTORY ${USD_ROOT}/bin DESTINATION bin/usd-${USD_VERSION})
    install(DIRECTORY ${USD_ROOT}/include DESTINATION bin/usd-${USD_VERSION})
    install(DIRECTORY ${USD_ROOT}/lib DESTINATION bin/usd-${USD_VERSION})
    install(DIRECTORY ${USD_ROOT}/plugin DESTINATION bin/usd-${USD_VERSION})
    install(DIRECTORY ${USD_ROOT}/share DESTINATION bin/usd-${USD_VERSION})
endif()

install(TARGETS ${TARGET} RUNTIME DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/meadow_2_2k.exr DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/plugInfo.json DESTINATION bin/usd-extra/plugin/usd)

# Install from binary folder so that the bat file has all env-vars replaced
# after configure_file call
configure_file( run_simpleusdview.bat run_simpleusdview.bat )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/run_simpleusdview.bat DESTINATION bin)
