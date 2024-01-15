#
# SPDX-License-Identifier: MIT
#

set(SRCS
    Logger.cpp
)

set(INCLUDEDIRS include)

set(REQUIRES
    esp_event
)

set(PRIV_REQUIRES
    freertos-addons

    AppEvents
    priorities
)

idf_component_register(
    SRCS          "${SRCS}"
    INCLUDE_DIRS  "${INCLUDEDIRS}"
    REQUIRES      "${REQUIRES}"
    PRIV_REQUIRES "${PRIV_REQUIRES}"
)
