#Can either be DEBUG or PRODUCTION, applying debug symbols or optimizations, respectively
MODE = DEBUG

#Output file to be generated
EXE = test.out

#What folder to output .o files. Folder must exist prior to running makefile
BUILD_DIR = build

#Root directory of the Dear Imgui git repository. Found at https://github.com/ocornut/imgui
IMGUI_DIR = ../imgui

#Root directory for small json library used to keep track of workspaces, found at https://github.com/davidmoreno/json11
JSON11_DIR = ../json11

CXX = g++

#List of source files and directories relative to project root
SOURCES = main.cpp json/jsonstuff.cpp display_windows.cpp mysqlstuff/db_functions.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
SOURCES += $(JSON11_DIR)/json11.cpp

#Strips all source files of preceding folder names and extensions, converting it to ${BUILD_DIR}/FILE_NAME.o
COMPILE_OBJECTS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

#List of includes for Dear Imgui, Json11, and if necessary, the mysql connector, Opengl3 and GLFW3
INCLUDES = -I $(IMGUI_DIR) -I $(IMGUI_DIR)/backends -I ./includes -I $(JSON11_DIR)

#Dynamic library files for the mysql connector, opengl, and glfw
LIBS = -lmysqlcppconn -lGL -lglfw

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
%.o: $$(notdir $$(basename $$@)).cpp dir
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: json/$$(notdir $$(basename $$@)).cpp dir
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: mysqlstuff/$$(notdir $$(basename $$@)).cpp dir
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(IMGUI_DIR)/$$(notdir $$(basename $$@)).cpp dir
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(IMGUI_DIR)/backends/$$(notdir $$(basename $$@)).cpp dir
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(JSON11_DIR)/$$(notdir $$(basename $$@)).cpp dir
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

all: $(EXE) dir
	@echo Build Complete! && ./$(EXE)

$(EXE): $(COMPILE_OBJECTS) dir
	$(CXX) $(COMPILE_OBJECTS) $(LIBS) $(EXTRA_FLAGS) -o $(EXE)


#Deletes all compiled objects, and target executable
clean:
	-rm $(BUILD_DIR)/* -f ; rm $(EXE) -f

fullclean: clean
	rm -r $(BUILD_DIR)

dir:
	@if [ ! -d "$(BUILD_DIR)" ]; then mkdir $(BUILD_DIR); fi