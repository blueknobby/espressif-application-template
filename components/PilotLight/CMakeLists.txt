#
# SPDX-License-Identifier: MIT
#

set(SRCS
    PilotLight.cpp
   )

set(INCLUDEDIRS include)

set(REQUIRES
    priorities
    console
    freertos-addons
    driver
    led_strip
)

idf_component_register(
    SRCS         "${SRCS}"
    INCLUDE_DIRS "${INCLUDEDIRS}"
    REQUIRES     "${REQUIRES}"
)
