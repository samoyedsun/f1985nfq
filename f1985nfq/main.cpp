#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>
#include <ctime>
#include <iostream>
#include <random>
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

void show_example_app_rendering(ExampleAppLog * log)
{
	bool show_flag = true;
	if (!show_flag)
		return;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	if (!ImGui::Begin("Example: Custom rendering", &show_flag, flags))
	{
		ImGui::End();
		return;
	}

	//-------------------------------------------------
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	std::default_random_engine e1;
	std::uniform_int_distribution<int> u1(1, 10);
	e1.seed(time(NULL) + 1000000);
	std::default_random_engine e2;
	std::uniform_int_distribution<int> u2(1, 10);
	e2.seed(time(NULL));
	std::default_random_engine e3;
	std::uniform_int_distribution<int> u3(1, 10);
	e3.seed(time(NULL) - 1000000);

	// log->AddLog("n1:%d, n2:%d, n3:%d\n", u1(e1), u2(e2), u3(e3));

	const ImU32 color = ImColor(ImVec4(u1(e1) * 0.1f, u2(e2) * 0.3f, u1(e3) * 0.1f, 1.0f));
	const ImVec2 p = ImGui::GetCursorScreenPos();

	static float sz = 60.0f;
	static int ngon_sides = 6;
	const double offset = 50.0f;
	const double col_interval = sz*0.5f + sz*0.5f*0.5;
	const double row_interval = sqrt((sz*0.5f * sz*0.5f) - (sz*0.5f*0.5f * sz*0.5f*0.5f)) * 2;
	double x = 0.0f;
	double y = 0.0f;
	for (int i = 0; i < 10; ++i)
	{
		x = p.x + offset + col_interval * (i * 2);
		y = p.y + offset - row_interval * 0.5 * 0;
		for (int i = 0; i < 10; ++i)
		{
			draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, color, ngon_sides); y += row_interval;
		}
		x = p.x + offset + col_interval * (i * 2 + 1);
		y = p.y + offset - row_interval * 0.5 * 1;
		for (int i = 0; i < 10; ++i)
		{
			draw_list->AddNgonFilled(ImVec2(x, y), sz*0.5f, color, ngon_sides); y += row_interval;
		}
	}

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

	GLFWwindow* window = glfwCreateWindow(1000, 600, "f1985nfq", NULL, NULL);
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
		
		static ExampleAppLog my_log;

		show_example_app_rendering(&my_log);
		
		my_log.Draw("DebugLog");

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