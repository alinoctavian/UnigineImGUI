/* Copyright (C) 2005-2020, UNIGINE. All rights reserved.
 *
 * This file is a part of the UNIGINE 2 SDK.
 *
 * Your use and / or redistribution of this software in source and / or
 * binary form, with or without modification, is subject to: (i) your
 * ongoing acceptance of and compliance with the terms and conditions of
 * the UNIGINE License Agreement; and (ii) your inclusion of this notice
 * in any version of this software that you use or redistribute.
 * A copy of the UNIGINE License Agreement is available by contacting
 * UNIGINE. at http://unigine.com/
 */


#include "AppSystemLogic.h"
#include "UnigineApp.h"

#include "ImGuiImpl.h"

#include <UnigineInput.h>

using namespace Unigine;

AppSystemLogic::AppSystemLogic()
{
}

AppSystemLogic::~AppSystemLogic()
{

}

int AppSystemLogic::init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiImpl::init();
	ImGui::StyleColorsDark();

	return 1;
}

int AppSystemLogic::update()
{
	ImGuiImpl::newFrame();
	ImGui::NewFrame();


	ImGui::ShowDemoWindow();

	ImGui::Render();
	ImGuiImpl::renderDrawData(ImGui::GetDrawData());

	return 1;
}


int AppSystemLogic::shutdown()
{
	ImGuiImpl::shutdown();
	ImGui::DestroyContext();
	return 1;
}
