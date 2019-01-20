set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_PRIV_REQUIRES SDL fatfs)

register_component()

component_compile_options(
    -Wno-error=unused-label
    -Wno-unused-label
    -Wno-unused-but-set-variable
    -Wno-unused-function
)