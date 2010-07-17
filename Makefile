# BUILD SETTINGS ###########################################

PLATFORM := UNIX

TARGET := opentyrian

############################################################

STRIP := strip
SDL_CONFIG := sdl-config

ifneq ($(PLATFORM), UNIX)
    include crosscompile.mk
endif

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=obj/%.o)

# FLAGS ####################################################

ifneq ($(MAKECMDGOALS), release)
    EXTRA_CFLAGS += -g3 -O0 -Werror
else
    EXTRA_CFLAGS += -g0 -O2 -DNDEBUG
endif
EXTRA_CFLAGS += -MMD -pedantic -Wall -Wextra -Wno-missing-field-initializers

HG_REV := $(shell hg id -ib && touch src/hg_revision.h)
ifneq ($(HG_REV), )
    EXTRA_CFLAGS += '-DHG_REV="$(HG_REV)"'
endif

EXTRA_LDLIBS += -lm

SDL_CFLAGS := $(shell $(SDL_CONFIG) --cflags)
SDL_LDLIBS := $(shell $(SDL_CONFIG) --libs) -lSDL_net

ALL_CFLAGS += -std=c99 -I./src -DTARGET_$(PLATFORM) $(EXTRA_CFLAGS) $(SDL_CFLAGS) $(CFLAGS)
ALL_LDFLAGS += $(LDFLAGS)
LDLIBS += $(EXTRA_LDLIBS) $(SDL_LDLIBS)

# RULES ####################################################

.PHONY : all release clean

all : $(TARGET)

release : all
	$(STRIP) $(TARGET)

clean :
	rm -rf obj/*
	rm -f $(TARGET)

ifneq ($(MAKECMDGOALS), clean)
    -include $(OBJS:.o=.d)
endif

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(ALL_LDFLAGS) $^ $(LDLIBS)

obj/%.o : src/%.c
	@mkdir -p "$(dir $@)"
	$(CC) -c -o $@ $(ALL_CFLAGS) $<

