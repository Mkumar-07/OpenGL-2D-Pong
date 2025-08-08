FILE_NAME = pong
CPP_FILES = ./src/*.cpp
GLAD_SRC = ./src/*.c

FILE_DEFINES:=
FILE_INCLUDES:= -I./include -framework Cocoa -framework OpenGL -framework IOKit
FILE_LINKERS:= -L./lib/ -lglfw3

build:
	g++ -std=c++20 $(CPP_FILES) $(GLAD_SRC) $(FILE_INCLUDES) $(FILE_LINKERS) -o $(FILE_NAME)
