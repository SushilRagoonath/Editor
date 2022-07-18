#include <blah.h>

#include "blah_imgui.h"
#include "../imgui/imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

using namespace Blah;

namespace
{
	MeshRef mesh;
	MaterialRef material;
	ShaderRef shader;
	TextureRef tex;
	Batch batcher;

	const ShaderData opengl_shader_data = {
		"#version 330\n"
		"uniform mat4 u_matrix;"
		"layout(location=0) in vec2 a_position;\n"
		"layout(location=1) in vec2 a_tex;\n"
		"layout(location=2) in vec4 a_color;\n"
		"out vec2 v_tex;\n"
		"out vec4 v_color;\n"
		"void main() {\n"
		"	gl_Position = u_matrix * vec4(a_position.xy, 0, 1);\n"
		"	v_tex = a_tex;"
		"	v_color = a_color;\n"
		"}\n",

		"#version 330\n"
		"uniform sampler2D u_texture;\n"
		"in vec2 v_tex;\n"
		"in vec4 v_color;\n"
		"out vec4 frag_color;\n"
		"void main() {\n"
		"	frag_color = texture(u_texture, v_tex.st) * v_color;\n"
		"}\n"
	};

	const char* d3d11_shader = ""
		"cbuffer constants : register(b0)\n"
		"{\n"
		"	row_major float4x4 u_matrix;\n"
		"}\n"

		"struct vs_in\n"
		"{\n"
		"	float2 position : POS;\n"
		"	float2 texcoord : TEX;\n"
		"	float4 color : COL;\n"
		"};\n"

		"struct vs_out\n"
		"{\n"
		"	float4 position : SV_POSITION;\n"
		"	float2 texcoord : TEX;\n"
		"	float4 color : COL;\n"
		"};\n"

		"Texture2D    u_texture : register(t0);\n"
		"SamplerState u_sampler : register(s0);\n"

		"vs_out vs_main(vs_in input)\n"
		"{\n"
		"	vs_out output;\n"

		"	output.position = mul(float4(input.position, 0.0f, 1.0f), u_matrix);\n"
		"	output.texcoord = input.texcoord;\n"
		"	output.color = input.color;\n"

		"	return output;\n"
		"}\n"

		"float4 ps_main(vs_out input) : SV_TARGET\n"
		"{\n"
		"	return u_texture.Sample(u_sampler, input.texcoord) * input.color;\n"
		"}\n";

	const ShaderData d3d11_shader_data = {
		d3d11_shader,
		d3d11_shader,
		{
			{ "POS", 0 },
			{ "TEX", 0 },
			{ "COL", 0 },
		}
	};

	const VertexFormat format = VertexFormat({
		{ 0, VertexType::Float2, false },
		{ 1, VertexType::Float2, false },
		{ 2, VertexType::UByte4, true }
	});
}
void blah_imgui_dock(bool* p_open)
{
    // If you strip some features of, this demo is pretty much equivalent to calling DockSpaceOverViewport()!
    // In most cases you should be able to just call DockSpaceOverViewport() and ignore all the code below!
    // In this specific demo, we are not using DockSpaceOverViewport() because:
    // - we allow the host window to be floating/moveable instead of filling the viewport (when opt_fullscreen == false)
    // - we allow the host window to have padding (when opt_padding == true)
    // - we have a local menu bar in the host window (vs. you could use BeginMainMenuBar() + DockSpaceOverViewport() in your code!)
    // TL;DR; this demo is more complicated than what you would normally use.
    // If we removed all the options we are showcasing, this demo would become:
    //     void ShowExampleAppDockSpace()
    //     {
    //         ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    //     }

    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags =  ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", p_open, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Options"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows,
            // which we can't undo at the moment without finer window depth/z control.
            ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
            ImGui::MenuItem("Padding", NULL, &opt_padding);
            ImGui::Separator();

            if (ImGui::MenuItem("Flag: NoSplit",                "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
            if (ImGui::MenuItem("Flag: NoResize",               "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
            if (ImGui::MenuItem("Flag: AutoHideTabBar",         "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
            if (ImGui::MenuItem("Flag: PassthruCentralNode",    "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
            ImGui::Separator();

            if (ImGui::MenuItem("Close", NULL, false, p_open != NULL))
                *p_open = false;
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void blah_imgui_startup()
{
	// Initializes ImGui
	ImGui::CreateContext();
	auto draw_size = App::get_size();
	// Create ImGui Config
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.DisplaySize.x = (float)(draw_size.x);
	io.DisplaySize.y = (float)(draw_size.y);
	io.BackendFlags = 0;
	io.ConfigFlags = ImGuiConfigFlags_DockingEnable;
	io.DisplayFramebufferScale = ImVec2(App::content_scale(), App::content_scale());
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingTransparentPayload = true;
    io.ConfigDockingWithShift = false;
	// Key Mapping
	{
		io.KeyMap[ImGuiKey_Tab] = (int)Key::Tab;
		io.KeyMap[ImGuiKey_LeftArrow] = (int)Key::Left;
		io.KeyMap[ImGuiKey_RightArrow] = (int)Key::Right;
		io.KeyMap[ImGuiKey_UpArrow] = (int)Key::Up;
		io.KeyMap[ImGuiKey_DownArrow] = (int)Key::Down;
		io.KeyMap[ImGuiKey_PageUp] = (int)Key::PageUp;
		io.KeyMap[ImGuiKey_PageDown] = (int)Key::PageDown;
		io.KeyMap[ImGuiKey_Home] = (int)Key::Home;
		io.KeyMap[ImGuiKey_End] = (int)Key::End;
		io.KeyMap[ImGuiKey_Insert] = (int)Key::Insert;
		io.KeyMap[ImGuiKey_Delete] = (int)Key::Delete;
		io.KeyMap[ImGuiKey_Backspace] = (int)Key::Backspace;
		io.KeyMap[ImGuiKey_Space] = (int)Key::Space;
		io.KeyMap[ImGuiKey_Enter] = (int)Key::Enter;
		io.KeyMap[ImGuiKey_Escape] = (int)Key::Escape;
		io.KeyMap[ImGuiKey_KeyPadEnter] = (int)Key::KeypadEnter;
		io.KeyMap[ImGuiKey_A] = (int)Key::A;
		io.KeyMap[ImGuiKey_C] = (int)Key::C;
		io.KeyMap[ImGuiKey_V] = (int)Key::V;
		io.KeyMap[ImGuiKey_X] = (int)Key::X;
		io.KeyMap[ImGuiKey_Y] = (int)Key::Y;
		io.KeyMap[ImGuiKey_Z] = (int)Key::Z;
	}


	// Create Font Texture
	{
		io.Fonts->AddFontDefault();

		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		tex = Texture::create(width, height, TextureFormat::RGBA, pixels);
	}
	{
		//ImFont * font = io.Fonts->AddFontFromFileTTF("content\\Cousine-Regular.ttf", 13.0f);

/*		unsigned char* pixels;
		int width, height;
		// ImGui::PushFont(font);
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		tex = Texture::create(width, height, TextureFormat::RGBA, pixels);*/
	}

	// Create Default Resources
	{
		if (App::renderer().type == RendererType::OpenGL)
			shader = Shader::create(opengl_shader_data);
		else if (App::renderer().type == RendererType::D3D11)
			shader = Shader::create(d3d11_shader_data);
		material = Material::create(shader);
		mesh = Mesh::create();
	}
	// ImFont * font = io.Fonts->AddFontFromFileTTF("content/DroidSansMono.ttf", 15.0f,NULL);
	// ImGui::PushFont(font);
    ImGui::GetStyle().FrameRounding = 4.0f;
    ImGui::GetStyle().GrabRounding = 4.0f;
    
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void blah_imgui_update()
{
	// Setup ImGui Frame Data
	{
		auto draw_size = App::get_size();
		auto scale = App::content_scale();
		auto mouse = Input::mouse_draw();

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize.x = (float)(draw_size.x / scale);
		io.DisplaySize.y = (float)(draw_size.y / scale);
		io.MousePos = ImVec2(mouse.x / scale, mouse.y / scale);
		io.MouseDown[0] = Input::down(MouseButton::Left);
		io.MouseDown[1] = Input::down(MouseButton::Right);
		io.MouseDown[2] = Input::down(MouseButton::Middle);
		io.MouseWheel = (float)Input::mouse_wheel().y;
		io.MouseWheelH = (float)Input::mouse_wheel().x;
		io.KeyCtrl = Input::ctrl();
		io.KeyShift = Input::shift();
		io.KeyAlt = Input::alt();

		// toggle held keys
		for (int i = 0; i < 512; i++)
			io.KeysDown[i] = Input::down((Key)i);

		// add text strings
		// io.AddInputCharactersUTF8(); Input::text() doesnt exist
	}

}

void blah_imgui_render()
{
	// render imgui
	ImGui::Render();
	ImDrawData* data = ImGui::GetDrawData();

	// set up render call
	DrawCall pass;
	pass.target = App::backbuffer();
	pass.mesh = mesh;
	pass.material = material;
	pass.has_scissor = true;
	pass.blend = BlendMode(BlendOp::Add, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

	// size
	Vec2 size = App::get_size();

	// apply the ortho matrix
	Mat4x4 mat =
		Mat4x4f::create_scale(data->FramebufferScale.x, data->FramebufferScale.y, 1.0f) *
		Mat4x4f::create_ortho_offcenter(0, size.x, size.y, 0, 0.1f, 1000.0f);
	material->set_value("u_matrix", &mat.m11, 16);

	// draw imgui buffers to the screen
	for (int i = 0; i < data->CmdListsCount; i++)
	{
		ImDrawList* list = data->CmdLists[i];

		mesh->vertex_data(format, list->VtxBuffer.Data, list->VtxBuffer.size());
		mesh->index_data(IndexFormat::UInt16, list->IdxBuffer.Data, list->IdxBuffer.size());

		for (ImDrawCmd* cmd = list->CmdBuffer.begin(); cmd < list->CmdBuffer.end(); cmd++)
		{
			// Todo:
			// use cmd->TextureId to get the current texture we should draw
			TextureRef * t =  cmd->TextureId;
			if(t)
			material->set_texture(0,*t);
			else
			material->set_texture(0, tex);
			material->set_sampler(0, TextureSampler(TextureFilter::Nearest));

			if (cmd->UserCallback != nullptr)
			{
				cmd->UserCallback(list, cmd);
			}
			else
			{
				
				pass.index_start = cmd->IdxOffset;
				pass.index_count = cmd->ElemCount;
				pass.scissor = Rect(
					cmd->ClipRect.x,
					cmd->ClipRect.y,
					(cmd->ClipRect.z - cmd->ClipRect.x),
					(cmd->ClipRect.w - cmd->ClipRect.y))
					.scale(data->FramebufferScale.x, data->FramebufferScale.y);

				pass.perform();
			}
		}
	}
} 