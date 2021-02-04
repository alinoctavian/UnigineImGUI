// Copyright (C), UNIGINE. All rights reserved.

#include <UnigineEngine.h>

#include "AppEditorLogic.h"
#include "AppSystemLogic.h"
#include "AppWorldLogic.h"
#include "ImGuiImpl.h"

using namespace Unigine;

#include <UnigineGui.h>
#include <UnigineApp.h>

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	// UnigineLogic
	AppSystemLogic system_logic;
	AppWorldLogic world_logic;
	AppEditorLogic editor_logic;

	// init engine
	Unigine::EnginePtr engine(UNIGINE_VERSION, argc, argv);

	engine->addSystemLogic(&system_logic);
	engine->addWorldLogic(&world_logic);
	engine->addEditorLogic(&editor_logic);

	int saved_mouse = 0;

	while (engine->isDone() == 0)
	{
		engine->update();

		// Temporary workaround with mouse capture
		auto &io = ImGui::GetIO();
		if (io.WantCaptureMouse)
		{
			saved_mouse = App::getMouseButton();
			App::setMouseButton(0);
			Gui::get()->setMouseButton(0);
		}
		//

		engine->render();


		//
		if (io.WantCaptureMouse)
			App::setMouseButton(saved_mouse);
		//

		engine->swap();
	}
	return 0;
}

#ifdef _WIN32
#include <Windows.h>
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif
