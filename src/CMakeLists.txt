add_executable(lit5v lit5v.c)

# pull in common dependencies and additional pwm hardware support
target_link_libraries(lit5v pico_stdlib hardware_pwm hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(lit5v)

# add url via pico_set_program_url
#example_auto_set_url(lit5v)
