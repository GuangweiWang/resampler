
PROJ_NAME := downsampler.bin

SRC_PATH := ../src
INC_PATH := ../inc

SRC_FILES := $(shell ls $(SRC_PATH)/*.cpp)

all: $(PROJ_NAME)

$(PROJ_NAME):
	g++ -I$(INC_PATH) $(SRC_FILES) -o $(PROJ_NAME)

clean:
	rm -rf $(PROJ_NAME)
