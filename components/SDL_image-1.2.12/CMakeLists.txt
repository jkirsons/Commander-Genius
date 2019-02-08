set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_PRIV_REQUIRES SDL)

register_component()

component_compile_options(
    -Wno-error=unused-value
    -Wno-error=maybe-uninitialized
)