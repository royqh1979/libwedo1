set(TESTWEDO_SRC testwedo.c)
add_executable(testwedo ${TESTWEDO_SRC})
target_link_libraries(testwedo usb-1.0)
