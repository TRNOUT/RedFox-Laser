add_executable(idn_probe main.cpp)
target_link_libraries(idn_probe PRIVATE libera-core)
redfox_copy_bundled_libusb_dll(idn_probe)
