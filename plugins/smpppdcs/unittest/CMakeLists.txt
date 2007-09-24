set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_SOURCE_DIR}/src  )


########### next target ###############

set(smpppdcstests_SRCS main.cpp clienttest.cpp )

kde4_add_executable(smpppdcstests TEST ${smpppdcstests_SRCS})

target_link_libraries(smpppdcstests  ${KDE4_KDECORE_LIBS} smpppdclient kunittestgui )

########### install files ###############





