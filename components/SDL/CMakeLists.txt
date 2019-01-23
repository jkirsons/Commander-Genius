#set(COMPONENT_SRCS "SDL_system.c")
set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES fatfs)

register_component()

component_compile_options(
    -Wno-error=unused-value
)