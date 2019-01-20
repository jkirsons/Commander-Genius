#set(COMPONENT_SRCS "")
set(COMPONENT_SRCDIRS 
    "zlib" 
    "base"
    "base/utils"
    "base/video"
    "base/video/scaler"
    "graphics"
    "widgets"
    "fileio"
    )
set(COMPONENT_ADD_INCLUDEDIRS "." "curl/include" "zlib" )
set(COMPONENT_PRIV_REQUIRES SDL SDL_image-1.2.12)


register_component()
component_compile_options(
    -Wno-error=narrowing
    -Wno-ignored-qualifiers
)