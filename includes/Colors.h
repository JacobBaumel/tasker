#ifndef TASKER_COLORS_H
#define TASKER_COLORS_H
#include "imgui.h"

namespace tasker {
    namespace Colors {
        static constexpr ImVec4 green = ImVec4(0, 0.839, 0, 1); 
        static constexpr ImVec4 text = ImVec4(0.859, 0.839, 0.82, 1);
        static constexpr ImVec4 error_text = ImVec4(1, 0, 0, 1); 
        static constexpr ImVec4 background = ImVec4(0.29, 0.29, 0.29, 1);
        static constexpr ImVec4 hovered = ImVec4(0.35, 0.35, 0.35, 1);
        static constexpr ImVec4 active = ImVec4(0.4, 0.4, 0.4, 1);
    }
}

#endif