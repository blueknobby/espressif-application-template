set(SRCDIRS
    "freertos-addons/c/Source"
    "freertos-addons/c++/Source"
   )

set(INCLUDEDIRS
    "freertos-addons/c/Source/include"
    "freertos-addons/c++/Source/include"
    "freertos-addons"
   )

set(REQUIRES "freertos")


idf_component_register(
    SRC_DIRS "${SRCDIRS}"
    INCLUDE_DIRS "${INCLUDEDIRS}"
    REQUIRES "${REQUIRES}"
)
