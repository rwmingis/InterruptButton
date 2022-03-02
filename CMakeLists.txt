cmake_minimum_required(VERSION 3.5)

FILE(GLOB_RECURSE app_sources "*.cpp")

# Build InterruptButton as an ESP-IDF component
if(ESP_PLATFORM)
    idf_component_register(
        SRCS ${app_sources}
        INCLUDE_DIRS "."
        #REQUIRES ${depends}
        #PRIV_REQUIRES
    )
return()
endif()

project(InterruptButton VERSION 1.0.0)
#target_compile_options(${COMPONENT_TARGET} PRIVATE -fno-rtti)
