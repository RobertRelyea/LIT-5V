include_directories(${lit5v_SOURCE_DIR}/include)

# Set up lit5v GPIO
add_library(lit5v_gpio lit5v_gpio.c)
target_link_libraries(lit5v_gpio pico_stdlib hardware_pwm hardware_adc)

# Set up lit5v main
add_executable(lit5v lit5v.c)
target_link_libraries(lit5v pico_stdlib lit5v_gpio)

# create map/bin/hex file etc.
pico_add_extra_outputs(lit5v)

# add url via pico_set_program_url
#example_auto_set_url(lit5v)
