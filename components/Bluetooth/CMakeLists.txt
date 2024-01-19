#
# SPDX-License-Identifier: MIT
#

set(SRCS
    Bluetooth.cpp
    bt_svc_app.cpp
    bt_svc_batt.cpp
    bt_svc_dis.cpp
)

set(INCLUDEDIRS include)

set(REQUIRES
   )

set(PRIV_REQUIRES
    bt
    console
   )


idf_component_register(
    SRCS          "${SRCS}"
    INCLUDE_DIRS  "${INCLUDEDIRS}"
    REQUIRES      "${REQUIRES}"
    PRIV_REQUIRES "${PRIV_REQUIRES}"
)
