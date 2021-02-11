
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

ifndef CEDEV
$(error CEDEV environment path variable is not set)
endif

include $(CEDEV)/meta/makefile.mk

fonts:
	$(MAKE) -C fonts

.PHONY: fonts
