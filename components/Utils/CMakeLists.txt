set(COMPONENT_SRCS "base/GsApp.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "." "curl/include" "zlib")
set(COMPONENT_PRIV_REQUIRES SDL SDL_image-1.2.12)

register_component()
