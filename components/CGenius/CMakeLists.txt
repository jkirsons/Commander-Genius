set(COMPONENT_SRCDIRS   
    "." 
    "fileio"
    "fileio/compression"
    "dialog"
    "sdl/sys"
    "sdl/audio"
    "sdl/audio/music"
    "sdl/audio/sound"
    "sdl/audio/base"
    "graphics/effects"
    "engine"
    "engine/refkeen"
    "engine/refkeen/kdreams"
    "engine/refkeen/kdreams/lscr"
    "engine/core"
    "engine/core/mode"
    "engine/core/VGamepads"
    "engine/core/menu"
    "engine/core/menu/widgets"
    "engine/unzip"
    "engine/keen"
    "engine/keen/galaxy"
    "engine/keen/galaxy/res"
    "engine/keen/galaxy/ep4"
    "engine/keen/galaxy/ep4/ai"
    "engine/keen/galaxy/ep5"
    "engine/keen/galaxy/ep5/ai"
    "engine/keen/galaxy/common"
    "engine/keen/galaxy/common/dialog"
    "engine/keen/galaxy/common/ai"
    "engine/keen/galaxy/common/ai/platform"
    "engine/keen/galaxy/menu"
    "engine/keen/galaxy/ep6"
    "engine/keen/galaxy/ep6/ai"
    "engine/keen/dreams"
    "engine/keen/vorticon"
    "engine/keen/vorticon/dialog"
    "engine/keen/vorticon/ai"
    "engine/keen/vorticon/menu"
    "engine/keen/vorticon/menu/widgets"
    "engine/keen/vorticon/playgame"
    "engine/keen/vorticon/finale"
    )
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES SDL SDL_image-1.2.12 SDL_mixer-1.2.12 Utils)



register_component()

component_compile_options(
    -Wno-unused-variable 
    -Wno-error=maybe-uninitialized 
    -Wno-missing-field-initializers 
    -Wno-maybe-uninitialized 
    -Wno-ignored-qualifiers 
    -Wno-error=switch
    )