CC := gcc
CFLAGS = -Wall 
ALL_CFLAGS := -std=c99 -pthread $(CFLAGS)

BUILD_DIR := build
SRC_DIR := src

# In this section, you list the files that are part of the project.
# If you add/change names of source files, here is where you
# edit the Makefile.
SOURCES := main.c server.c client.c tty_utils.c network.c
OBJECTS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(SOURCES)))
TARGET := dchat


# The first target defined in the makefile is the one
# used when make is invoked with no argument. Given the definitions
# above, this Makefile file will build the one named TARGET and
# assume that it depends on all the named OBJECTS files.

$(TARGET): $(OBJECTS)	
	$(CC) $(ALL_CFLAGS) -o $(BUILD_DIR)/$@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@

install:
	mkdir -p $(DESTDIR)/usr/bin
	install -m 0755 $(BUILD_DIR)/$(TARGET) $(DESTDIR)/usr/bin/$(TARGET)

clean:
	@rm -f $(BUILD_DIR)/$(TARGET) $(OBJECTS) core