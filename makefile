PROJECT  := kat

INCLUDES := $(wildcard ./src/include/*.h)
SOURCES  := $(wildcard ./src/*.c)
OBJECTS  := $(patsubst ./src/%.c,./src/%.o,$(SOURCES))
DEPENDS  := $(patsubst ./src/%.c,./src/%.d,$(SOURCES))

CC       := gcc
CFLAGS   := -std=c11
LDFLAGS  :=
BUILD    ?=

ifeq ($(BUILD), DEBUG)
	CFLAGS += -g -DDEBUG -Wall -Wextra
endif

TARGET := kat

.PHONY: all
all: $(TARGET)
	$(info [$(PROJECT)] build done)

$(TARGET): $(OBJECTS)
	$(info [$(PROJECT)] linking $(notdir $@))
	@$(CC) $(LDFLAGS) $^ -o $@

$(OBJECTS): src/%.o: src/%.c
	$(info [$(PROJECT)] compiling $(notdir $<) => $(notdir $@))
	@$(CC) -MMD -Isrc/include $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(info [$(PROJECT)] $@)
	@$(RM) $(TARGET) $(OBJECTS) $(DEPENDS)

-include $(DEPENDS)
