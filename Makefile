MODE = DEBUG
EXE = test.out
BUILD_DIR = build
IMGUI_DIR = ../imgui
JSON11_DIR = ../json11
CXX = g++
SOURCES = main.cpp json/jsonstuff.cpp display_windows.cpp mysqlstuff/db_functions.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
SOURCES += $(JSON11_DIR)/json11.cpp
COMPILE_OBJECTS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
INCLUDES = -I $(IMGUI_DIR) -I $(IMGUI_DIR)/backends -I ./includes -I $(JSON11_DIR)
LIBS = -lmysqlcppconn -lGL -lglfw
EXTRA_FLAGS = -std=c++11

ifeq ($(MODE), DEBUG)
EXTRA_FLAGS += -g
endif

ifeq ($(MODE), PRODUCTION)
EXTRA_FLAGS += -O3 -DNDEBUG -s
endif

.SECONDEXPANSION:
%.o: $$(notdir $$(basename $$@)).cpp
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: json/$$(notdir $$(basename $$@)).cpp
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: mysqlstuff/$$(notdir $$(basename $$@)).cpp
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(IMGUI_DIR)/$$(notdir $$(basename $$@)).cpp
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(IMGUI_DIR)/backends/$$(notdir $$(basename $$@)).cpp
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

%.o: $(JSON11_DIR)/$$(notdir $$(basename $$@)).cpp
	$(CXX) -o $@ $< $(EXTRA_FLAGS) $(INCLUDES) -c

all: $(EXE)
	@echo Build Complete! && ./$(EXE)

$(EXE): $(COMPILE_OBJECTS)
	$(CXX) $(COMPILE_OBJECTS) $(LIBS) $(EXTRA_FLAGS) -o $(EXE)


clean:
	-rm $(BUILD_DIR)/* -f ; rm $(EXE) -f