#
# SPDX-License-Identifier: MIT
#

set(SRCS
    AppEvents.cpp
)

set(INCLUDEDIRS include)

set(REQUIRES
    esp_event
)


idf_component_register(
    SRCS         "${SRCS}"
    INCLUDE_DIRS "${INCLUDEDIRS}"
    REQUIRES     "${REQUIRES}"
)
