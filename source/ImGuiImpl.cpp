#include "ImGuiImpl.h"


#include <UnigineApp.h>
#include <UnigineTextures.h>
#include <UnigineMeshDynamic.h>
#include <UnigineMaterials.h>
#include <UnigineRender.h>

using namespace Unigine;


static TexturePtr font_texture;
static MeshDynamicPtr imgui_mesh;
static MaterialPtr imgui_material;
static ImDrawData *frame_draw_data;

static int on_key_pressed(unsigned int key)
{
	auto &io = ImGui::GetIO();
	io.KeysDown[key] = true;
	return io.WantCaptureKeyboard ? 1 : 0;
}

static int on_key_released(unsigned int key)
{
	auto &io = ImGui::GetIO();
	io.KeysDown[key] = false;
	return 0;
}

static int on_button_pressed(int button)
{
	auto &io = ImGui::GetIO();

	switch (button)
	{
	case App::BUTTON_LEFT:
		io.MouseDown[0] = true;
		break;
	case App::BUTTON_RIGHT:
		io.MouseDown[1] = true;
		break;
	case App::BUTTON_MIDDLE:
		io.MouseDown[2] = true;
		break;
	}

	return 0;
}

static int on_button_released(int button)
{
	auto &io = ImGui::GetIO();

	switch (button)
	{
	case App::BUTTON_LEFT:
		io.MouseDown[0] = false;
		break;
	case App::BUTTON_RIGHT:
		io.MouseDown[1] = false;
		break;
	case App::BUTTON_MIDDLE:
		io.MouseDown[2] = false;
		break;
	}

	return 0;
}

static int on_unicode_key_pressed(unsigned int key)
{
	auto &io = ImGui::GetIO();

	if(key < App::KEY_ESC || key >= App::NUM_KEYS)
		io.AddInputCharacter(key);

	return 0;
}

static void set_clipboard_text(void *, const char *text)
{
	App::setClipboard(text);
}

static char const *get_clipboard_text(void *)
{
	return App::getClipboard();
}

static void create_font_texture()
{
	auto &io = ImGui::GetIO();
	unsigned char *pixels = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	font_texture = Texture::create();
	font_texture->create2D(width, height, Texture::FORMAT_RGBA8, Texture::DEFAULT_FLAGS);
	
	auto blob = Blob::create();
	blob->setData(pixels, width * height * 32);
	font_texture->setBlob(blob);
	blob->setData(nullptr,0);

	io.Fonts->TexID = font_texture.get();
}

static void create_imgui_mesh()
{
	imgui_mesh = MeshDynamic::create(MeshDynamic::DYNAMIC_ALL);

	MeshDynamic::Attribute attributes[3]{};
	attributes[0].offset = 0;
	attributes[0].size = 2;
	attributes[0].type = MeshDynamic::TYPE_FLOAT;
	attributes[1].offset = 8;
	attributes[1].size = 2;
	attributes[1].type = MeshDynamic::TYPE_FLOAT;
	attributes[2].offset = 16;
	attributes[2].size = 4;
	attributes[2].type = MeshDynamic::TYPE_UCHAR;
	imgui_mesh->setVertexFormat(attributes, 3);

	assert(imgui_mesh->getVertexSize() == sizeof(ImDrawVert) && "Vertex size of MeshDynamic is not equal to size of ImDrawVert");
}

static void create_imgui_material()
{
	imgui_material = Materials::findMaterial("imgui")->inherit();
}

static void draw_callback()
{
	if (frame_draw_data == nullptr)
		return;

	auto draw_data = frame_draw_data;
	frame_draw_data = nullptr;

	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;


	// Write vertex and index data into dynamic mesh
	imgui_mesh->clearVertex();
	imgui_mesh->clearIndices();
	imgui_mesh->allocateVertex(draw_data->TotalVtxCount);
	imgui_mesh->allocateIndices(draw_data->TotalIdxCount);
	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[i];
		
		imgui_mesh->addVertexArray(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size);
		imgui_mesh->addIndicesArray(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size);
	}
	imgui_mesh->flushVertex();
	imgui_mesh->flushIndices();


	auto render_target = Render::getTemporaryRenderTarget();
	render_target->bindColorTexture(0, Renderer::getTextureColor());

	// Render state
	RenderState::saveState();
	RenderState::clearStates();
	RenderState::setBlendFunc(RenderState::BLEND_SRC_ALPHA, RenderState::BLEND_ONE_MINUS_SRC_ALPHA, RenderState::BLEND_OP_ADD);
	RenderState::setPolygonCull(RenderState::CULL_NONE);
	RenderState::setDepthFunc(RenderState::DEPTH_NONE);
	RenderState::setViewport(static_cast<int>(draw_data->DisplayPos.x), static_cast<int>(draw_data->DisplayPos.y),
		static_cast<int>(draw_data->DisplaySize.x),static_cast<int>(draw_data->DisplaySize.y));

	// Orthographic projection matrix
	float left = draw_data->DisplayPos.x;
	float right = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float top = draw_data->DisplayPos.y;
	float bottom = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

	Math::Mat4 proj;
	proj.m00 = 2.0f / (right - left);
	proj.m03 = (right + left) / (left - right);
	proj.m11 = 2.0f / (top - bottom);
	proj.m13 = (top + bottom) / (bottom - top);
	proj.m22 = 0.5f;
	proj.m23 = 0.5f;
	proj.m33 = 1.0f;

	Renderer::setProjection(proj);
	auto shader = imgui_material->fetchShader("imgui");
	auto pass = imgui_material->getRenderPass("imgui");
	Renderer::setShaderParameters(pass, shader, imgui_material, false);
	

	imgui_mesh->bind();
	render_target->enable();
	{
		int global_idx_offset = 0;
		int global_vtx_offset = 0;
		ImVec2 clip_off = draw_data->DisplayPos;
		// Draw command lists
		for (int i = 0; i < draw_data->CmdListsCount; ++i)
		{
			const ImDrawList *cmd_list = draw_data->CmdLists[i];
			for (int j = 0; j < cmd_list->CmdBuffer.Size; ++j)
			{
				const ImDrawCmd *cmd = &cmd_list->CmdBuffer[j];

				if (cmd->UserCallback != nullptr)
				{
					if (cmd->UserCallback != ImDrawCallback_ResetRenderState)
						cmd->UserCallback(cmd_list, cmd);
				}
				else
				{
					float width = (cmd->ClipRect.z - cmd->ClipRect.x) / draw_data->DisplaySize.x;
					float height = (cmd->ClipRect.w - cmd->ClipRect.y) / draw_data->DisplaySize.y;
					float x = (cmd->ClipRect.x - clip_off.x) / draw_data->DisplaySize.x;
					float y = 1.0f - height - (cmd->ClipRect.y - clip_off.y) / draw_data->DisplaySize.y;

					RenderState::setScissorTest(x, y, width, height);
					RenderState::flushStates();

					auto texture = TexturePtr(static_cast<Texture *>(cmd->TextureId));
					imgui_material->setTexture("imgui_texture", texture);

					imgui_mesh->renderInstancedSurface(MeshDynamic::MODE_TRIANGLES,
						cmd->VtxOffset + global_vtx_offset,
						cmd->IdxOffset + global_idx_offset,
						cmd->IdxOffset + global_idx_offset + cmd->ElemCount, 1);
				}
			}
			global_vtx_offset += cmd_list->VtxBuffer.Size;
			global_idx_offset += cmd_list->IdxBuffer.Size;

		}
	}
	render_target->disable();
	imgui_mesh->unbind();

	RenderState::restoreState();

	render_target->unbindColorTexture(0);
	Render::releaseTemporaryRenderTarget(render_target);
}

static void *draw_callback_handle;

void ImGuiImpl::init()
{
	App::setKeyPressFunc(on_key_pressed);
	App::setKeyReleaseFunc(on_key_released);
	App::setButtonPressFunc(on_button_pressed);
	App::setButtonReleaseFunc(on_button_released);
	App::setKeyPressUnicodeFunc(on_unicode_key_pressed);

	draw_callback_handle = Render::addCallback(Render::CALLBACK_END_SCREEN, MakeCallback(draw_callback));
	ImGuiIO &io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	io.BackendPlatformName = "imgui_impl_unigine";
	io.BackendRendererName = "imgui_impl_unigine";

	io.KeyMap[ImGuiKey_Tab] = App::KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = App::KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = App::KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = App::KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = App::KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = App::KEY_PGUP;
	io.KeyMap[ImGuiKey_PageDown] = App::KEY_PGDOWN;
	io.KeyMap[ImGuiKey_Home] = App::KEY_HOME;
	io.KeyMap[ImGuiKey_End] = App::KEY_END;
	io.KeyMap[ImGuiKey_Insert] = App::KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = App::KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = App::KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = ' ';
	io.KeyMap[ImGuiKey_Enter] = App::KEY_RETURN;
	io.KeyMap[ImGuiKey_Escape] = App::KEY_ESC;
	io.KeyMap[ImGuiKey_KeyPadEnter] = App::KEY_RETURN;
	io.KeyMap[ImGuiKey_A] = 'a';
	io.KeyMap[ImGuiKey_C] = 'c';
	io.KeyMap[ImGuiKey_V] = 'v';
	io.KeyMap[ImGuiKey_X] = 'x';
	io.KeyMap[ImGuiKey_Y] = 'y';
	io.KeyMap[ImGuiKey_Z] = 'z';

	io.SetClipboardTextFn = set_clipboard_text;
	io.GetClipboardTextFn = get_clipboard_text;
	io.ClipboardUserData = nullptr;

	create_font_texture();
	create_imgui_mesh();
	create_imgui_material();
}

void ImGuiImpl::newFrame()
{
	auto &io = ImGui::GetIO();

	io.DisplaySize = ImVec2(static_cast<float>(App::getWidth()), static_cast<float>(App::getHeight()));
	io.DeltaTime = App::getIFps();

	io.KeyCtrl = App::getKeyState(App::KEY_CTRL) != 0;
	io.KeyShift = App::getKeyState(App::KEY_SHIFT) != 0;
	io.KeyAlt = App::getKeyState(App::KEY_ALT) != 0;
	io.KeySuper = App::getKeyState(App::KEY_CMD) != 0;

	if (io.WantSetMousePos)
		App::setMouse(static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y));

	io.MousePos = ImVec2(static_cast<float>(App::getMouseX()), static_cast<float>(App::getMouseY()));
	io.MouseWheel += static_cast<float>(App::getMouseAxis(App::AXIS_Y));
	io.MouseWheelH += static_cast<float>(App::getMouseAxis(App::AXIS_X));
}

void ImGuiImpl::renderDrawData(ImDrawData *draw_data)
{
	frame_draw_data = draw_data;
}

void ImGuiImpl::shutdown()
{
	imgui_material->deleteLater();
	imgui_mesh->deleteLater();
	font_texture->deleteLater();

	Render::removeCallback(Render::CALLBACK_END_SCREEN, draw_callback_handle);

	App::setKeyPressFunc(nullptr);
	App::setKeyReleaseFunc(nullptr);
	App::setButtonPressFunc(nullptr);
	App::setButtonReleaseFunc(nullptr);
	App::setKeyPressUnicodeFunc(nullptr);
}
