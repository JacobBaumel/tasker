
#Can either be DEBUG or PRODUCTION, applying debug symbols or optimizations, respectively
MODE = DEBUG

#Output file to be generated
EXE = test.out

#What folder to output .o files. Folder must exist prior to running makefile
BUILD_DIR = build
BUILD_DIR_CMD = @mkdir -p $(BUILD_DIR)

#Root directory of the Dear Imgui git repository. Found at https://github.com/ocornut/imgui
IMGUI_DIR = ../imgui

#Root directory for small json library used to keep track of workspaces, found at https://github.com/davidmoreno/json11
JSON11_DIR = ../json11

#Optional variable if the mysql connector is in a nonstandard location
MSQLC_DIR = ../mysqlcppconn

CXX = g++

#List of source files and directories relative to project root
SOURCES = main.cpp jsonstuff.cpp display_windows.cpp server_requests.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
SOURCES += $(JSON11_DIR)/json11.cpp

#Strips all source files of preceding folder names and extensions, converting it to ${BUILD_DIR}/FILE_NAME.o
COMPILE_OBJECTS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

#List of includes for Dear Imgui, Json11, and if necessary, the mysql connector, Opengl3 and GLFW3
INCLUDES = -I $(IMGUI_DIR) -I $(IMGUI_DIR)/backends -I ./includes -I $(JSON11_DIR)

#Dynamic library files for the mysql connector, opengl, and glfw
LIBS = -lmysqlcppconn -lGL -lglfw

#Add mysqlcppconn include directory and lib directory if MSQLC_DIR is set
ifneq ($(strip $(MSQLC_DIR)),)
INCLUDES += -I $(MSQLC_DIR)/include/jdbc
LIBS += -L $(MSQLC_DIR)/lib64
endif

#Explicitly defines c++11
EXTRA_FLAGS = -std=c++11

#Appends extra debug symbols if mode = DEBUG
ifeq ($(MODE), DEBUG)
EXTRA_FLAGS += -g
endif

#Strips debug symbols, and applies level 3 optimizations if mode = PRODUCTION
ifeq ($(MODE), PRODUCTION)
EXTRA_FLAGS += -O3 -DNDEBUG -s
endif

.PHONY: all
#Second expansion necessary to use compile objects for targets, and corresponding sources for prereqs. Probably not the best way to go about it, but it works
.SECONDEXPANSION:
%.o: $$(notdir $$(basename $$@)).cpp
	$(BUILD_DIR_CMD)
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(IMGUI_DIR)/$$(notdir $$(basename $$@)).cpp
	$(BUILD_DIR_CMD)
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(IMGUI_DIR)/backends/$$(notdir $$(basename $$@)).cpp
	$(BUILD_DIR_CMD)
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(JSON11_DIR)/$$(notdir $$(basename $$@)).cpp
	$(BUILD_DIR_CMD)
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

all: $(EXE)
	@echo Build Complete!

$(EXE): $(COMPILE_OBJECTS)
	$(CXX) $(COMPILE_OBJECTS) $(LIBS) $(EXTRA_FLAGS) -o $(EXE)


#Deletes all compiled objects, and target executable
clean:
	-rm $(BUILD_DIR)/* -f ; rm $(EXE) -f

# config.json is a file used by tasker during runtime. Clean this too
fullclean: clean
	rm -r $(BUILD_DIR) && rm config.json
