CC := gcc
CFLAGS = -Wall 
ALL_CFLAGS := -std=c99 -pthread $(CFLAGS)

BUILD_DIR := build
SRC_DIR := src

SOURCES := main.c server.c client.c tty_utils.c network.c
OBJECTS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(SOURCES)))
TARGET := dchat

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
