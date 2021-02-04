#pragma once
#include "imgui/imgui.h"


class ImGuiImpl
{
public:
	static void init();
	static void newFrame();
	static void renderDrawData(ImDrawData *draw_data);
	static void shutdown();
};