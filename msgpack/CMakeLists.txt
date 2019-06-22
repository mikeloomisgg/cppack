cmake_minimum_required(VERSION 3.9)
project(Msgpack LANGUAGES CXX VERSION 1.0)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

add_library(Msgpack INTERFACE)

target_include_directories(Msgpack
                           INTERFACE
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                           )

install(TARGETS Msgpack
        EXPORT MsgpackTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )

set(MSGPACK_CMAKE_CONFIG_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Msgpack")

install(EXPORT MsgpackTargets
        DESTINATION ${MSGPACK_CMAKE_CONFIG_DESTINATION})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MsgpackConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/MsgpackConfigVersion.cmake"
        DESTINATION ${MSGPACK_CMAKE_CONFIG_DESTINATION})

add_subdirectory(tests)