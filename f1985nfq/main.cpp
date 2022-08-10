#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>
#include <iostream>
#include <boost/bind.hpp>
#include <thread>
#include "net_server.h"
#include "net_client.h"
#include "app_log.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

double power(double x, int n)
{
	double val = 1.0;
	while (n--)
		val *= x;
	return val;
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static char remote_host_buf[1024] = "";
static char recv_field[512 * 8] = { 0 };

void show_main_windows()
{
	bool show_flag = true;
	if (!show_flag)
		return;
	
	static int corner = 0;
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		//ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoNav;
	if (corner != -1)
	{
		const float PAD = 15.0f;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 work_pos = viewport->WorkPos;
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
		window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
		window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
		window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		window_flags |= ImGuiWindowFlags_NoMove;
	}

	if (ImGui::Begin("MainWindow", &show_flag, window_flags))
	{
		ImGui::InputText("Host", remote_host_buf, sizeof(remote_host_buf));
		static char path_buf[1024] = "";
		ImGui::InputText("Path", path_buf, sizeof(path_buf));
		if (ImGui::Button("Send File", ImVec2(80, 40)))
			fprintf(stdout, "Send File!\n");

		ImGui::InputTextMultiline("recvBox", recv_field, IM_ARRAYSIZE(recv_field), ImVec2(300, ImGui::GetTextLineHeight() * 8), ImGuiInputTextFlags_ReadOnly);

		static char text2[512 * 8] = { 0 };
		ImGui::InputTextMultiline("sendBox", text2, IM_ARRAYSIZE(text2), ImVec2(300, ImGui::GetTextLineHeight() * 8), ImGuiInputTextFlags_AllowTabInput);

		if (ImGui::Button("Send Msg", ImVec2(80, 40)))
		{
			fprintf(stdout, "Send Msg!\n");
			net_client::instance()->set_remote_host(std::string(remote_host_buf));
			net_client::instance()->send_102(text2);
		}
	}
	ImGui::End();
}

void show_example_app_rendering()
{
	bool show_flag = true;
	if (!show_flag)
		return;

	static bool use_work_area = true;
	static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

	// We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
	// Based on your use case you may want one of the other.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
	ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

	if (!ImGui::Begin("Example: Custom rendering", &show_flag, flags))
	{
		ImGui::End();
		return;
	}

	static ImVector<ImVec2> points;
	static ImVec2 scrolling(0.0f, 0.0f);
	static bool opt_enable_grid = true;
	static bool adding_line = false;


	//-------------------------------------------------
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	static float sz = 120.0f;
	static float thickness = 3.0f;
	static int ngon_sides = 6;
	static ImVec4 colf = ImVec4(0.0f, 1.0f, 0.4f, 1.0f);
	const ImU32 col = ImColor(colf);
	static int circle_segments_override_v = 12;
	const int circle_segments = false ? circle_segments_override_v : 0;
	const ImDrawFlags corners_tl_br = ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight;

	const ImVec2 p = ImGui::GetCursorScreenPos();
	float x = p.x + 100.0f;
	float y = p.y + 100.0f;
	for (int i = 0; i <9; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	x = p.x + 100.0f;
	y = p.y + 100.0f + 104.0f;
	for (int i = 0; i < 8; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	x = p.x + 100.0f;
	y = p.y + 100.0f + 104.0f + 104.0f;
	for (int i = 0; i < 6; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}


	x = p.x + 100.0f;
	y = p.y + 100.0f + 104.0f + 104.0f + 104.0f;
	for (int i = 0; i < 4; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	x = p.x + 100.0f;
	y = p.y + 100.0f + 104.0f + 104.0f + 104.0f + 104.0f;
	for (int i = 0; i < 2; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	// ------------------------------∫·œÚ----------------------------------

	x = p.x + 100.0f + sz * power(0.5f, 2) * 3;
	y = p.y + 100.0f - 52.0f;
	for (int i = 0; i < 8; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	x = p.x + 100.0f + sz * power(0.5f, 2) * 9;
	y = p.y + 100.0f - 52.0f;
	for (int i = 0; i < 6; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	x = p.x + 100.0f + sz * power(0.5f, 2) * 15;
	y = p.y + 100.0f - 52.0f;
	for (int i = 0; i < 4; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	x = p.x + 100.0f + sz * power(0.5f, 2) * 21;
	y = p.y + 100.0f - 52.0f;
	for (int i = 0; i < 2; ++i)
	{
		draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, col, ngon_sides); x += sz * 0.5f + sz * power(0.5f, 2); y += sz * 0.5f - sz * power(0.5f, 4);
	}

	/*
	// Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
	ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
	ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
	if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
	if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
	ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

	// Draw border and background color
	ImGuiIO& io = ImGui::GetIO();
	//ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
	draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

	// This will catch our interactions
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	const bool is_hovered = ImGui::IsItemHovered(); // Hovered
	const bool is_active = ImGui::IsItemActive();   // Held
	const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
	const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);


	// Add first and second point
	if (is_hovered && !adding_line && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		points.push_back(mouse_pos_in_canvas);
		points.push_back(mouse_pos_in_canvas);
		adding_line = true;
	}
	if (adding_line)
	{
		points.back() = mouse_pos_in_canvas;
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
			adding_line = false;
	}

	// Draw grid + all lines in the canvas
	draw_list->PushClipRect(canvas_p0, canvas_p1, true);
	if (opt_enable_grid)
	{
		const float GRID_STEP = 64.0f;
		for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
			draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
		for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
			draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
	}
	for (int n = 0; n < points.Size; n += 2)
		draw_list->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
	draw_list->PopClipRect();
	*/

	ImGui::End();
}

int main()
{
	net_server::instance()->init();
	net_server::instance()->set_recv_field_ptr(recv_field);
	net_server::instance()->startup();

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(1000, 700, "f1985nfq", NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
	IM_ASSERT(font != NULL);

	// Our state
	ImVec4 clear_color = ImVec4(1.00f, 1.00f, 1.0f, 1.00f);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		show_example_app_rendering();

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	net_server::instance()->shutdown();
	net_server::instance()->destory();
	net_client::instance()->destory();

	return 0;
}