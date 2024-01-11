set(INCLUDEDIRS include)
set(SRCS
    CommandLine.cpp
)

set(REQUIRES
    priorities

    freertos-addons

    console
    driver
    fatfs
)


idf_component_register(
    SRCS         "${SRCS}"
    INCLUDE_DIRS "${INCLUDEDIRS}"
    REQUIRES     "${REQUIRES}"
)
