#pragma once
#include "..\Gui.h"
#include <vector>
#include <string>
namespace default_ui {
	inline std::vector<std::string> list_elements;
	inline int selectedModule = -1;
	void on_draw(gui& gui);

	void refresh_list();
}
