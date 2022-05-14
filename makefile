
# ----------------------------
# Makefile Options
# ----------------------------

NAME ?= TERMINAL
ICON ?= icon.png
DESCRIPTION ?= "VT100 Terminal Emulator"
COMPRESSED ?= YES
ARCHIVED ?= YES

CFLAGS ?= -Wall -Wextra -Oz
CXXFLAGS ?= -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

fonts:
	$(MAKE) -C fonts

.PHONY: fonts
