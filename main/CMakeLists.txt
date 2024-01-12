#
# SPDX-License-Identifier: GPL-2.0-only
#


set(APP_SRCS
    main.cpp
    cmds.cpp
)

idf_component_register(
    SRCS "${APP_SRCS}"
    INCLUDE_DIRS "."
)
