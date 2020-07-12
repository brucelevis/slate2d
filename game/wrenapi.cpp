#include "wrenapi.h"
#include "../src/slate2d.h"
#include "game.h"
#include <cstring>
#include "wren/wren.hpp"
extern "C" {
#include "wren/wren_debug.h"
}
#include "map.h"
#include <string>
#include <map>
#include <imgui.h>

static unsigned int mapId;

static const char *wrenTypeStrings[] =
{
	"WREN_TYPE_BOOL",
	"WREN_TYPE_NUM",
	"WREN_TYPE_FOREIGN",
	"WREN_TYPE_LIST",
	"WREN_TYPE_NULL",
	"WREN_TYPE_STRING",
	"WREN_TYPE_MAP",
	"WREN_TYPE_UNKNOWN"
};
static const int wrenTypeCount = sizeof(wrenTypeStrings) / sizeof(wrenTypeStrings[0]);
static const char* wrenGetTypeString(WrenType i) {
	return i < 0 || i >= wrenTypeCount ? "WREN_TYPE_UNKNOWN" : wrenTypeStrings[i];
}

bool wrenCheckArgs(WrenVM *vm, int count, ...) {
	va_list args;
	va_start(args, count);
	for (int i = 1; i <= count; ++i) {
		WrenType result = va_arg(args, WrenType);
		WrenType slot = wrenGetSlotType(vm, i);
		if (result == WREN_TYPE_UNKNOWN) {
			continue;
		}
		else if (slot != result) {
			SLT_Print("Expected %s in paramter %i, got %s.", wrenGetTypeString(result), i, wrenGetTypeString(slot));
			wrenDebugPrintStackTrace(vm);
			va_end(args);
			return false;
		}
	}
	va_end(args);
	return true;
}

#define CHECK_ARGS(count, ...) if (wrenCheckArgs(vm, count, __VA_ARGS__) == false) { return; }

#pragma region Trap Module
void wren_trap_print(WrenVM *vm) {
	const char *str = wrenGetSlotString(vm, 1);
	SLT_Print("%s", str);
}

void wren_trap_dbgwin(WrenVM *vm) {
	CHECK_ARGS(3, WREN_TYPE_STRING, WREN_TYPE_STRING, WREN_TYPE_STRING);

	const char *title = wrenGetSlotString(vm, 1);
	const char *key = wrenGetSlotString(vm, 2);
	const char *value = wrenGetSlotString(vm, 3);

	ImGui::SetNextWindowSize({ 250, 500 }, ImGuiCond_FirstUseEver);
	ImGui::Begin(title, nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing);

	float width = ImGui::GetWindowContentRegionWidth();
	float keyWidth = ImGui::CalcTextSize(key).x;
	float valWidth = ImGui::CalcTextSize(value).x;

	ImGui::Text("%s", key);
	if (keyWidth + valWidth + 20 < width) {
		ImGui::SameLine();
	}
	int x = (int)(width - valWidth);
	x = x < 5 ? 5 : x;
	ImGui::SetCursorPosX((float)x);
	ImGui::Text("%s", value);
	ImGui::Separator();
	ImGui::End();
}

void wren_trap_console(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_STRING);

	const char *str = wrenGetSlotString(vm, 1);
	SLT_SendConsoleCommand(str);
}

void wren_trap_error(WrenVM *vm) {
	CHECK_ARGS(2, WREN_TYPE_NUM, WREN_TYPE_STRING);

	int err = (int) wrenGetSlotDouble(vm, 1);
	const char *str = wrenGetSlotString(vm, 2);
	SLT_Error(err, gtempstr("wren script: %s",str));
}

void wren_trap_snd_play(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_BOOL);

	AssetHandle assetHandle = (AssetHandle)wrenGetSlotDouble(vm, 1);
	float volume = (float)wrenGetSlotDouble(vm, 2);
	float pan = (float)wrenGetSlotDouble(vm, 3);
	bool loop = wrenGetSlotBool(vm, 4);

	unsigned int hnd = SLT_Snd_Play(assetHandle, volume, pan, loop);
	wrenSetSlotDouble(vm, 0, hnd);
}

void wren_trap_snd_stop(WrenVM *vm) {
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NULL) {
		CHECK_ARGS(1, WREN_TYPE_NUM);

		unsigned int handle = (unsigned int)wrenGetSlotDouble(vm, 1);
		SLT_Snd_Stop(handle);
	}
}

void wren_trap_snd_pause_resume(WrenVM *vm) {
	CHECK_ARGS(2, WREN_TYPE_NUM, WREN_TYPE_BOOL);

	unsigned int handle = (unsigned int)wrenGetSlotDouble(vm, 1);
	bool pause = wrenGetSlotBool(vm, 2);
	SLT_Snd_PauseResume(handle, pause);
}

void wren_trap_in_register_button(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_LIST);

	int count = wrenGetListCount(vm, 1);
	const char **names = (const char**) malloc(count * sizeof(char *));

	wrenEnsureSlots(vm, count + 2);

	for (int i = 0; i < count; i++) {
		wrenGetListElement(vm, 1, i, i + 2);
		names[i] = wrenGetSlotString(vm, i + 2);
	}
	
	SLT_In_AllocateButtons(names, count);

	free(names);
}

void wren_trap_in_button_pressed(WrenVM *vm) {
	CHECK_ARGS(3, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	int button = (int)wrenGetSlotDouble(vm, 1);
	int delay = (int)wrenGetSlotDouble(vm, 2);
	int repeat = (int)wrenGetSlotDouble(vm, 3);

	wrenSetSlotBool(vm, 0, SLT_In_ButtonPressed(button, delay, repeat));
}

void wren_trap_mouse_position(WrenVM *vm) {
	MousePosition mousePos = SLT_In_MousePosition();

	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);

	wrenSetSlotDouble(vm, 1, mousePos.x);
	wrenSetSlotDouble(vm, 2, mousePos.y);

	wrenInsertInList(vm, 0, -1, 1);
	wrenInsertInList(vm, 0, -1, 2);
}

extern void wren_trap_inspect(WrenVM *vm);

void wren_trap_get_resolution(WrenVM *vm) {
	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);

	int width, height;
	SLT_GetResolution(&width, &height);

	wrenSetSlotDouble(vm, 1, width);
	wrenSetSlotDouble(vm, 2, height);

	wrenInsertInList(vm, 0, -1, 1);
	wrenInsertInList(vm, 0, -1, 2);
}

void wren_trap_set_window_title(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_STRING);

	const char *title = wrenGetSlotString(vm, 1);

	if (title == nullptr || strlen(title) == 0) {
		return;
	}

	SLT_SetWindowTitle(title);
}

void wren_trap_get_platform(WrenVM *vm) {
	wrenEnsureSlots(vm, 1);

#if defined(_WIN32)
	static const char *platform = "windows";
#elif defined(__EMSCRIPTEN__)
	static const char *platform = "emscripten";
#else
	static const char *platform = "unknown";
#endif

	wrenSetSlotString(vm, 0, platform);
}

#pragma endregion

#pragma region CVar Module

void wren_cvar_bool(WrenVM *vm) {
	conVar_t** var = (conVar_t**)wrenGetSlotForeign(vm, 0);
	if (*var == nullptr) {
		return;
	}

	wrenSetSlotBool(vm, 0, (*var)->value > 0 || (*var)->string[0] != '0' || strlen((*var)->string) > 1);
}

void wren_cvar_number(WrenVM *vm) {
	conVar_t** var = (conVar_t**)wrenGetSlotForeign(vm, 0);
	if (*var == nullptr) {
		return;
	}

	wrenSetSlotDouble(vm, 0, (*var)->value);
}

void wren_cvar_string(WrenVM *vm) {
	conVar_t** var = (conVar_t**)wrenGetSlotForeign(vm, 0);
	if (*var == nullptr) {
		return;
	}

	wrenSetSlotString(vm, 0, (*var)->string);
}

void wren_cvar_set(WrenVM *vm) {
	conVar_t** var = (conVar_t**)wrenGetSlotForeign(vm, 0);
	if (*var == nullptr) {
		return;
	}

	auto valType = wrenGetSlotType(vm, 1);

	if (valType == WREN_TYPE_NUM) {
		double value = wrenGetSlotDouble(vm, 1);
		char cvarStr[1024];
		snprintf(cvarStr, 1024, "%f", value);
		SLT_Con_SetVar((*var)->name, cvarStr);
	}
	else if (valType == WREN_TYPE_BOOL) {
		bool value = wrenGetSlotBool(vm, 1);
		SLT_Con_SetVar((*var)->name, value ? "1" : "0");
	}
	else if (valType == WREN_TYPE_STRING) {
		const char *value = wrenGetSlotString(vm, 1);
		SLT_Con_SetVar((*var)->name, value);
	}
	else {
		SLT_Con_SetVar((*var)->name, "0");
	}
}

#pragma endregion

#pragma region Asset Module
void wren_asset_create(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_STRING, WREN_TYPE_STRING, WREN_TYPE_NUM);

	AssetType_t assetType = (AssetType_t)(int)wrenGetSlotDouble(vm, 1);
	const char *name = wrenGetSlotString(vm, 2);
	const char *path = wrenGetSlotString(vm, 3);
	int flags = (int)wrenGetSlotDouble(vm, 4);

	wrenSetSlotDouble(vm, 0, SLT_Asset_Create(assetType, name, path, flags));
}

void wren_asset_find(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_STRING);

	const char *name = wrenGetSlotString(vm, 1);

	int id = SLT_Asset_Find(name);
	
	if (id < 0) {
		SLT_Error(ERR_FATAL, "can't find asset %s", name);
		return;
	}

	wrenSetSlotDouble(vm, 0, id);
}

void wren_asset_load(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_NUM);

	AssetHandle assetHandle = (AssetHandle)wrenGetSlotDouble(vm, 1);
	SLT_Asset_Load(assetHandle);
}

void wren_asset_loadall(WrenVM *vm) {
	NOTUSED(vm);
	SLT_Asset_LoadAll();
}

void wren_asset_clearall(WrenVM *vm) {
	NOTUSED(vm);
	SLT_Asset_ClearAll();
}

void wren_asset_loadini(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_STRING);
	NOTUSED(vm);

	const char *name = wrenGetSlotString(vm, 1);
	SLT_Asset_LoadINI(name);
}

void wren_asset_bmpfnt_set(WrenVM *vm) {
	CHECK_ARGS(6, WREN_TYPE_NUM, WREN_TYPE_STRING, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	AssetHandle assetHandle = (AssetHandle)wrenGetSlotDouble(vm, 1);
	const char *glyphs = wrenGetSlotString(vm, 2);
	int glyphWidth = (int)wrenGetSlotDouble(vm, 3);
	int charSpacing = (int)wrenGetSlotDouble(vm, 4);
	int intWidth = (int)wrenGetSlotDouble(vm, 5);
	int lineHeight = (int)wrenGetSlotDouble(vm, 6);

	SLT_Asset_BMPFNT_Set(assetHandle, glyphs, glyphWidth, charSpacing, intWidth, lineHeight);
}

void wren_asset_textwidth(WrenVM *vm) {
	CHECK_ARGS(3, WREN_TYPE_NUM, WREN_TYPE_STRING, WREN_TYPE_NUM);

	AssetHandle fntId = (AssetHandle)wrenGetSlotDouble(vm, 1);
	const char *text = wrenGetSlotString(vm, 2);
	float scale = (float)wrenGetSlotDouble(vm, 3);

	double width = SLT_Asset_TextWidth(fntId, text, scale);
	wrenSetSlotDouble(vm, 0, width);
}

void wren_asset_breakstring(WrenVM *vm) {
	CHECK_ARGS(2, WREN_TYPE_NUM, WREN_TYPE_STRING);

	int width = (int)wrenGetSlotDouble(vm, 1);
	const char *text = wrenGetSlotString(vm, 2);

	const char *out = SLT_Asset_BreakString(width, text);

	wrenSetSlotString(vm, 0, out);
}

void wren_asset_sprite_set(WrenVM *vm) {
	CHECK_ARGS(5, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	AssetHandle assetHandle = (AssetHandle)wrenGetSlotDouble(vm, 1);
	int width = (int)wrenGetSlotDouble(vm, 2);
	int height = (int)wrenGetSlotDouble(vm, 3);
	int marginX = (int)wrenGetSlotDouble(vm, 4);
	int marginY = (int)wrenGetSlotDouble(vm, 5);

	SLT_Asset_Sprite_Set(assetHandle, width, height, marginX, marginY);
}

void wren_asset_canvas_set(WrenVM *vm) {
	CHECK_ARGS(3, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	AssetHandle assetHandle = (AssetHandle)wrenGetSlotDouble(vm, 1);
	int width = (int)wrenGetSlotDouble(vm, 2);
	int height = (int)wrenGetSlotDouble(vm, 3);

	SLT_Asset_Canvas_Set(assetHandle, width, height);
}

void wren_asset_shader_set(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_BOOL, WREN_TYPE_STRING, WREN_TYPE_STRING);

	AssetHandle assetHandle = (AssetHandle)wrenGetSlotDouble(vm, 1);
	bool isFile = wrenGetSlotBool(vm, 2);
	char *vs = (char*) wrenGetSlotString(vm, 3);
	char *fs = (char*) wrenGetSlotString(vm, 4);

	SLT_Asset_Shader_Set(assetHandle, isFile, vs, fs);
}

void wren_asset_image_size(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_NUM);

	AssetHandle assetHandle = (AssetHandle)wrenGetSlotDouble(vm, 1);

	const Image *img = SLT_Get_Img(assetHandle);

	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);

	wrenSetSlotDouble(vm, 1, img->w);
	wrenSetSlotDouble(vm, 2, img->h);

	wrenInsertInList(vm, 0, -1, 1);
	wrenInsertInList(vm, 0, -1, 2);
}
#pragma endregion

#pragma region Draw Module
void wren_dc_setcolor(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	uint8_t r = (uint8_t) wrenGetSlotDouble(vm, 1);
	uint8_t g = (uint8_t) wrenGetSlotDouble(vm, 2);
	uint8_t b = (uint8_t) wrenGetSlotDouble(vm, 3);
	uint8_t a = (uint8_t) wrenGetSlotDouble(vm, 4);
	DC_SetColor(r, g, b, a);
}

void wren_dc_reset_transform(WrenVM *vm) {
	NOTUSED(vm);
	DC_ResetTransform();
}

void wren_dc_scale(WrenVM *vm) {
	CHECK_ARGS(2, WREN_TYPE_NUM, WREN_TYPE_NUM);

	float x = (float) wrenGetSlotDouble(vm, 1);
	float y = (float) wrenGetSlotDouble(vm, 2);

	DC_Scale(x, y);
}

void wren_dc_rotate(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_NUM);

	float angle = (float) wrenGetSlotDouble(vm, 1);
	DC_Rotate(angle);
}

void wren_dc_translate(WrenVM *vm) {
	CHECK_ARGS(2, WREN_TYPE_NUM, WREN_TYPE_NUM);

	float x = (float) wrenGetSlotDouble(vm, 1);
	float y = (float) wrenGetSlotDouble(vm, 2);
	DC_Translate(x, y);
}

void wren_dc_setscissor(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	int x = (int) wrenGetSlotDouble(vm, 1);
	int y = (int) wrenGetSlotDouble(vm, 2);
	int w = (int) wrenGetSlotDouble(vm, 3);
	int h = (int) wrenGetSlotDouble(vm, 4);

	DC_SetScissor(x, y, w, h);
}

void wren_dc_resetscissor(WrenVM *vm) {
	NOTUSED(vm);
	DC_ResetScissor();
}

void wren_dc_usecanvas(WrenVM *vm) {
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL) {
		DC_ResetCanvas();
	}
	else {
		CHECK_ARGS(1, WREN_TYPE_NUM);

		AssetHandle canvasId = (AssetHandle)wrenGetSlotDouble(vm, 1);

		DC_UseCanvas(canvasId);
	}
}

void wren_dc_useshader(WrenVM *vm) {
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL) {
		DC_ResetShader();
	}
	else {
		CHECK_ARGS(1, WREN_TYPE_NUM);

		AssetHandle shaderId = (AssetHandle)wrenGetSlotDouble(vm, 1);

		DC_UseShader(shaderId);
	}
}

void wren_dc_settextstyle(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	AssetHandle fntId = (AssetHandle)wrenGetSlotDouble(vm, 1);
	float size = (float)wrenGetSlotDouble(vm, 2);
	float lineHeight = (float)wrenGetSlotDouble(vm, 3);
	int align = (int)wrenGetSlotDouble(vm, 4);

	DC_SetTextStyle(fntId, size, lineHeight, align);
}

void wren_dc_drawrect(WrenVM *vm) {
	CHECK_ARGS(5, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_BOOL);

	float x = (float) wrenGetSlotDouble(vm, 1);
	float y = (float) wrenGetSlotDouble(vm, 2);
	float w = (float) wrenGetSlotDouble(vm, 3);
	float h = (float) wrenGetSlotDouble(vm, 4);
	bool outline = wrenGetSlotBool(vm, 5);

	DC_DrawRect(x, y, w, h, outline);
}

void wren_dc_drawtext(WrenVM *vm) {
	CHECK_ARGS(5, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_STRING, WREN_TYPE_NUM);

	float x = (float)wrenGetSlotDouble(vm, 1);
	float y = (float)wrenGetSlotDouble(vm, 2);
	float w = (float)wrenGetSlotDouble(vm, 3);
	const char *text = wrenGetSlotString(vm, 4);
	int len = (int)wrenGetSlotDouble(vm, 5);

	DC_DrawText(x, y, w, text, len);
}

void wren_dc_drawimage(WrenVM *vm) {
	CHECK_ARGS(9, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM,
		WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	AssetHandle imgId = (AssetHandle)wrenGetSlotDouble(vm, 1);
	float x = (float)wrenGetSlotDouble(vm, 2);
	float y = (float)wrenGetSlotDouble(vm, 3);
	float w = (float)wrenGetSlotDouble(vm, 4);
	float h = (float)wrenGetSlotDouble(vm, 5);
	float scale = (float)wrenGetSlotDouble(vm, 6);
	uint8_t flipBits = (uint8_t)wrenGetSlotDouble(vm, 7);
	float ox = (float)wrenGetSlotDouble(vm, 8);
	float oy = (float)wrenGetSlotDouble(vm, 9);

	DC_DrawImage(imgId, x, y, w, h, scale, flipBits, ox, oy);
}

void wren_dc_drawline(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	float x1 = (float)wrenGetSlotDouble(vm, 1);
	float y1 = (float)wrenGetSlotDouble(vm, 2);
	float x2 = (float)wrenGetSlotDouble(vm, 3);
	float y2 = (float)wrenGetSlotDouble(vm, 4);

	DC_DrawLine(x1, y1, x2, y2);
}

void wren_dc_drawcircle(WrenVM *vm) {
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_BOOL);

	float x = (float)wrenGetSlotDouble(vm, 1);
	float y = (float)wrenGetSlotDouble(vm, 2);
	float radius = (float)wrenGetSlotDouble(vm, 3);
	bool outline = wrenGetSlotBool(vm, 4);

	DC_DrawCircle(x, y, radius, outline);
}

void wren_dc_drawtri(WrenVM *vm) {
	CHECK_ARGS(7, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_BOOL);

	float x1 = (float)wrenGetSlotDouble(vm, 1);
	float y1 = (float)wrenGetSlotDouble(vm, 2);
	float x2 = (float)wrenGetSlotDouble(vm, 3);
	float y2 = (float)wrenGetSlotDouble(vm, 4);
	float x3 = (float)wrenGetSlotDouble(vm, 5);
	float y3 = (float)wrenGetSlotDouble(vm, 6);
	bool outline = wrenGetSlotBool(vm, 7);

	DC_DrawTri(x1, y1, x2, y2, x3, y3, outline);
}

void wren_dc_drawmaplayer(WrenVM *vm) {
	CHECK_ARGS(7, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	int layer = (int)wrenGetSlotDouble(vm, 1);
	float x = (float)wrenGetSlotDouble(vm, 2);
	float y = (float)wrenGetSlotDouble(vm, 3);
	unsigned int cellX = (unsigned int)wrenGetSlotDouble(vm, 4);
	unsigned int cellY = (unsigned int)wrenGetSlotDouble(vm, 5);
	unsigned int cellW = (unsigned int)wrenGetSlotDouble(vm, 6);
	unsigned int cellH = (unsigned int)wrenGetSlotDouble(vm, 7);

	DC_DrawMapLayer(mapId, layer, x, y, cellX, cellY, cellW, cellH);
}

void wren_dc_drawsprite(WrenVM *vm) {
	CHECK_ARGS(8, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM,
		WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, 
		WREN_TYPE_NUM, WREN_TYPE_NUM);

	AssetHandle sprId = (AssetHandle)wrenGetSlotDouble(vm, 1);
	int id = (int)wrenGetSlotDouble(vm, 2);
	float x = (float)wrenGetSlotDouble(vm, 3);
	float y = (float)wrenGetSlotDouble(vm, 4);
	float scale = (float)wrenGetSlotDouble(vm, 5);
	uint8_t flipBits = (uint8_t)wrenGetSlotDouble(vm, 6);
	int w = (int)wrenGetSlotDouble(vm, 7);
	int h = (int)wrenGetSlotDouble(vm, 8);

	DC_DrawSprite(sprId, id, x, y, scale, flipBits, w, h);
}

void wren_dc_submit(WrenVM *vm) {
	NOTUSED(vm);
	DC_Submit();
}

void wren_dc_clear(WrenVM *vm) {
	NOTUSED(vm);
	CHECK_ARGS(4, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	uint8_t r = (uint8_t)wrenGetSlotDouble(vm, 1);
	uint8_t g = (uint8_t)wrenGetSlotDouble(vm, 2);
	uint8_t b = (uint8_t)wrenGetSlotDouble(vm, 3);
	uint8_t a = (uint8_t)wrenGetSlotDouble(vm, 4);
	DC_Clear(r, g, b, a);
}
#pragma endregion

#pragma region Map Module

void parseProperties(WrenVM *vm, int propSlot, int *totalSlotsIn, int *sIn, void *properties) {
	int &s = *sIn;
	int &totalSlots = *totalSlotsIn;

	// loop through properties
	if (properties != nullptr) {
		auto &props = *(std::map<std::string, void *>*) properties;

		for (auto item : props) {
			// add 2 slots for key, value
			totalSlots += 2;
			wrenEnsureSlots(vm, totalSlots);

			auto prop = (tmx_property*)item.second;
			wrenSetSlotString(vm, s++, prop->name);
			switch (prop->type) {
			case PT_INT:
				wrenSetSlotDouble(vm, s++, prop->value.integer);
				break;
			case PT_FLOAT:
				wrenSetSlotDouble(vm, s++, prop->value.decimal);
				break;
			case PT_BOOL:
				wrenSetSlotBool(vm, s++, prop->value.boolean);
				break;
			case PT_COLOR:
				wrenSetSlotDouble(vm, s++, prop->value.color);
				break;
			case PT_NONE:
			case PT_STRING:
			case PT_FILE:
			default:
				wrenSetSlotString(vm, s++, prop->value.string);
				break;
			}
			wrenInsertInMap(vm, propSlot, s - 2, s - 1);
		}
	}
}

void wren_map_setcurrent(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_NUM);

	unsigned int id = (unsigned int) wrenGetSlotDouble(vm, 1);
	mapId = id;
}

void wren_map_getlayerbyname(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_STRING);

	const char *name = wrenGetSlotString(vm, 1);

	const tmx_map *map = SLT_Get_TMX(mapId);
	int id = Map_GetLayerByName(map, name);

	wrenSetSlotDouble(vm, 0, id);
}

void wren_map_getobjectsinlayer(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_NUM);

	int id = (int) wrenGetSlotDouble(vm, 1);
	int totalSlots = 1; // total num of slots for wrenEnsureSlots
	int s = 1; // current slot we're on

	static const char *keys[] = { "name", "type", "x", "y", "width", "height", "visible", "rotation", "properties" };
	static const int keySz = sizeof(keys) / sizeof(*keys);

	// reserve keys
	totalSlots += keySz;
	wrenEnsureSlots(vm, totalSlots);

	// return value is a list of map objects
	wrenSetSlotNewList(vm, 0);

	// store the keys starting from slot 1 since they can be reused
	for (int i = 0; i < keySz; i++) {
		wrenSetSlotString(vm, s++, keys[i]);
	}

	const tmx_map *map = SLT_Get_TMX(mapId);
	tmx_object *obj = Map_LayerObjects(map, id, nullptr);
	while (obj != nullptr) {
		// ensure enough slots for map object + map values
		totalSlots += keySz + 1;
		wrenEnsureSlots(vm, totalSlots);

		// make a new map, push it to the end of the return value list
		int mapSlot = s++;
		wrenSetSlotNewMap(vm, mapSlot);
		wrenInsertInList(vm, 0, -1, mapSlot);

		// slots for property values, same order as keys[]
		wrenSetSlotString(vm, s++, obj->name == nullptr ? "" : obj->name);
		const char *defaultedType = Map_GetObjectType(map, obj);
		wrenSetSlotString(vm, s++, defaultedType == nullptr ? "" : defaultedType);
		wrenSetSlotDouble(vm, s++, obj->x);
		wrenSetSlotDouble(vm, s++, obj->y);
		wrenSetSlotDouble(vm, s++, obj->width);
		wrenSetSlotDouble(vm, s++, obj->height);
		wrenSetSlotBool(vm, s++, obj->visible);
		wrenSetSlotDouble(vm, s++, obj->rotation);

		// properties is a nested map, we'll populate it after we deal with the object
		int propSlot = s++;
		wrenSetSlotNewMap(vm, propSlot);

		// apply the default properties first if it is a tile
		if (obj->obj_type == OT_TILE) {
			tmx_tile *baseTile = Map_GetTileInfo(map, obj->content.gid);
			if (baseTile != nullptr) {
				parseProperties(vm, propSlot, &totalSlots, &s, baseTile->properties);
			}
		}
		// add the object
		parseProperties(vm, propSlot, &totalSlots, &s, obj->properties);

		for (int i = 0; i < keySz; i++) {
			// i + 1 because 0 is return value. keys are at the top, values are placed after the map
			wrenInsertInMap(vm, mapSlot, i + 1, mapSlot + 1 + i);
		}

		obj = Map_LayerObjects(map, id, obj);
	}
}

void wren_map_getmapproperties(WrenVM *vm) {
	static const char *keys[] = { "width", "height", "tileWidth", "tileHeight", "backgroundColor", "properties" };
	static const int keySz = sizeof(keys) / sizeof(*keys);

	const tmx_map *map = SLT_Get_TMX(mapId);

	int totalSlots = 1; // total num of slots for wrenEnsureSlots
	int s = 1; // current slot we're on

	// reserve keys
	totalSlots += keySz * 2;
	wrenEnsureSlots(vm, totalSlots);

	// return value is a map
	wrenSetSlotNewMap(vm, 0);

	// store the keys starting from slot 1
	for (int i = 0; i < keySz; i++) {
		wrenSetSlotString(vm, s++, keys[i]);
	}

	// values
	wrenSetSlotDouble(vm, s++, map->width);
	wrenSetSlotDouble(vm, s++, map->height);
	wrenSetSlotDouble(vm, s++, map->tile_width);
	wrenSetSlotDouble(vm, s++, map->tile_height);
	wrenSetSlotDouble(vm, s++, map->backgroundcolor);

	int propSlot = s++;
	wrenSetSlotNewMap(vm, propSlot);

	parseProperties(vm, propSlot, &totalSlots, &s, map->properties);

	for (int i = 0; i < keySz; i++) {
		wrenInsertInMap(vm, 0, 1 + i, 1 + keySz + i);
	}
}

void wren_map_getlayerproperties(WrenVM *vm) {
	CHECK_ARGS(1, WREN_TYPE_NUM);

	int id = (int)wrenGetSlotDouble(vm, 1);

	const tmx_map *map = SLT_Get_TMX(mapId);
	tmx_layer *layer = Map_GetLayer(map, id);

	if (layer == nullptr) {
		return;
	}

	static const char *keys[] = { "name", "visible", "opacity", "offsetX", "offsetY", "properties" };
	static const int keySz = sizeof(keys) / sizeof(*keys);

	int totalSlots = 1; // total num of slots for wrenEnsureSlots
	int s = 1; // current slot we're on

	// reserve keys
	totalSlots += keySz * 2;
	wrenEnsureSlots(vm, totalSlots);

	// return value is a map
	wrenSetSlotNewMap(vm, 0);

	// store the keys starting from slot 1
	for (int i = 0; i < keySz; i++) {
		wrenSetSlotString(vm, s++, keys[i]);
	}

	wrenSetSlotString(vm, s++, layer->name == nullptr ? "" : layer->name);
	wrenSetSlotBool(vm, s++, layer->visible);
	wrenSetSlotDouble(vm, s++, layer->opacity);
	wrenSetSlotDouble(vm, s++, layer->offsetx);
	wrenSetSlotDouble(vm, s++, layer->offsety);
	int propSlot = s++;
	wrenSetSlotNewMap(vm, propSlot);

	parseProperties(vm, propSlot, &totalSlots, &s, layer->properties);

	for (int i = 0; i < keySz; i++) {
		wrenInsertInMap(vm, 0, 1 + i, 1 + keySz + i);
	}
}

void wren_map_gettileproperties(WrenVM *vm) {
	const tmx_map *map = SLT_Get_TMX(mapId);

	int totalSlots = 2; // total num of slots for wrenEnsureSlots
	int s = 1; // current slot we're on

	// return value is a list of map objects
	wrenSetSlotNewList(vm, 0);

	wrenEnsureSlots(vm, totalSlots);
	wrenSetSlotString(vm, s++, "type");

	for (unsigned int gid = 0; gid < map->tilecount; gid++) {
		tmx_tile *tile = Map_GetTileInfo(map, gid);
		if (tile == nullptr) {
			continue;
		}

		totalSlots += 3;
		wrenEnsureSlots(vm, totalSlots);

		int propSlot = s++;
		wrenSetSlotNewMap(vm, propSlot);
		wrenInsertInList(vm, 0, -1, propSlot);

		wrenSetSlotString(vm, s++, tile->type == nullptr ? "" : tile->type);
		wrenInsertInMap(vm, propSlot, 1, s - 1);

		parseProperties(vm, propSlot, &totalSlots, &s, tile->properties);
	}
}

void wren_map_gettile(WrenVM *vm) {
	CHECK_ARGS(3, WREN_TYPE_NUM, WREN_TYPE_NUM, WREN_TYPE_NUM);

	int layer = (int)wrenGetSlotDouble(vm, 1);
	unsigned int x = (unsigned int)wrenGetSlotDouble(vm, 2);
	unsigned int y = (unsigned int)wrenGetSlotDouble(vm, 3);
	
	const tmx_map *map = SLT_Get_TMX(mapId);
	int gid = Map_GetTile(map, layer, x, y);

	wrenSetSlotDouble(vm, 0, gid);
}

void wren_map_getlayernames(WrenVM *vm) {
	int i = 0;

	wrenSetSlotNewList(vm, 0);

	const tmx_map *map = SLT_Get_TMX(mapId);
	tmx_layer *layer = Map_GetLayer(map, i);
	while (layer != nullptr) {
		wrenEnsureSlots(vm, i + 2);
		wrenSetSlotString(vm, i + 1, layer->name);
		wrenInsertInList(vm, 0, -1, i + 1);
		i++;
		layer = layer->next;
	}
}
#pragma endregion

#pragma region Wren config callbacks
static bool clearNextError = true;
static void wren_error(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message) {
	NOTUSED(vm);

	if (type == WREN_ERROR_STACK_COMPLETE) {
		clearNextError = true;
		return;
	}

	if (clearNextError) {
		SLT_Con_SetVar("engine.lastErrorStack", "");
		clearNextError = false;
	}

	const conVar_t *stack = SLT_Con_GetVarDefault("engine.lastErrorStack", "", 0);
	if (line == -1) {
		SLT_Con_SetVar("engine.lastErrorStack", gtempstr("%s\n%s", stack->string, message));
		SLT_Print("%s\n", message);
	}
	else {
		SLT_Con_SetVar("engine.lastErrorStack", gtempstr("%s\n(%s:%i) %s", stack->string, module, line, message));
		SLT_Print("(%s:%i) %s\n", module, line, message);
	}
}

char* wren_loadModuleFn(WrenVM* vm, const char* name) {
	NOTUSED(vm);

	char *script = nullptr;
	const char *path = gtempstr("scripts/%s.wren", name);

	int sz = SLT_FS_ReadFile(path, (void**)&script);
	if (sz <= 0) {
		return nullptr;
	}
	else {
		// wren will free this (see allocatedSource in wrenImportModule)
		return script;
	}
}

typedef struct {
	const char *module;
	const char *className;
	bool isStatic;
	const char *signature;
	WrenForeignMethodFn fn;
} wrenMethodDef;

static const wrenMethodDef methods[] = {
	{ "engine", "Trap", true, "print(_)", wren_trap_print },
	{ "engine", "Trap", true, "printWin_(_,_,_)", wren_trap_dbgwin },
	{ "engine", "Trap", true, "error(_,_)", wren_trap_error },
	{ "engine", "Trap", true, "console(_)", wren_trap_console },
	{ "engine", "Trap", true, "sndPlay(_,_,_,_)", wren_trap_snd_play },
	{ "engine", "Trap", true, "sndStop(_)", wren_trap_snd_stop },
	{ "engine", "Trap", true, "sndPauseResume(_,_)", wren_trap_snd_pause_resume },
	{ "engine", "Trap", true, "registerButtons(_)", wren_trap_in_register_button },
	{ "engine", "Trap", true, "buttonPressed(_,_,_)", wren_trap_in_button_pressed },
	{ "engine", "Trap", true, "mousePosition()", wren_trap_mouse_position },
	{ "engine", "Trap", true, "inspect(_,_)", wren_trap_inspect },
	{ "engine", "Trap", true, "getResolution()", wren_trap_get_resolution },
	{ "engine", "Trap", true, "setWindowTitle(_)", wren_trap_set_window_title },
	{ "engine", "Trap", true, "getPlatform()", wren_trap_get_platform },

	{ "engine", "CVar", false, "bool()", wren_cvar_bool },
	{ "engine", "CVar", false, "number()", wren_cvar_number },
	{ "engine", "CVar", false, "string()", wren_cvar_string },
	{ "engine", "CVar", false, "set(_)", wren_cvar_set },

	{ "engine", "Asset", true, "create(_,_,_,_)", wren_asset_create },
	{ "engine", "Asset", true, "find(_)", wren_asset_find },
	{ "engine", "Asset", true, "load(_)", wren_asset_load },
	{ "engine", "Asset", true, "loadAll()", wren_asset_loadall },
	{ "engine", "Asset", true, "clearAll()", wren_asset_clearall },
	{ "engine", "Asset", true, "loadINI(_)", wren_asset_loadini },
	{ "engine", "Asset", true, "bmpfntSet(_,_,_,_,_,_)", wren_asset_bmpfnt_set },
	{ "engine", "Asset", true, "textWidth(_,_,_)", wren_asset_textwidth },
	{ "engine", "Asset", true, "breakString(_,_)", wren_asset_breakstring },
	{ "engine", "Asset", true, "spriteSet(_,_,_,_,_)", wren_asset_sprite_set },
	{ "engine", "Asset", true, "imageSize(_)", wren_asset_image_size },
	{ "engine", "Asset", true, "canvasSet(_,_,_)", wren_asset_canvas_set },
	{ "engine", "Asset", true, "shaderSet(_,_,_,_)", wren_asset_shader_set },

	{ "engine", "Draw", true, "setColor(_,_,_,_)", wren_dc_setcolor },
	{ "engine", "Draw", true, "resetTransform()", wren_dc_reset_transform },
	{ "engine", "Draw", true, "scale(_,_)", wren_dc_scale },
	{ "engine", "Draw", true, "rotate(_)", wren_dc_rotate },
	{ "engine", "Draw", true, "translate(_,_)", wren_dc_translate },
	{ "engine", "Draw", true, "setScissor(_,_,_,_)", wren_dc_setscissor },
	{ "engine", "Draw", true, "resetScissor()", wren_dc_resetscissor },
	{ "engine", "Draw", true, "useCanvas(_)", wren_dc_usecanvas },
	{ "engine", "Draw", true, "useShader(_)", wren_dc_useshader },
	{ "engine", "Draw", true, "rect(_,_,_,_,_)", wren_dc_drawrect },
	{ "engine", "Draw", true, "setTextStyle(_,_,_,_)", wren_dc_settextstyle },
	{ "engine", "Draw", true, "text(_,_,_,_,_)", wren_dc_drawtext },
	{ "engine", "Draw", true, "image(_,_,_,_,_,_,_,_,_)", wren_dc_drawimage },
	{ "engine", "Draw", true, "line(_,_,_,_)", wren_dc_drawline },
	{ "engine", "Draw", true, "circle(_,_,_,_)", wren_dc_drawcircle },
	{ "engine", "Draw", true, "tri(_,_,_,_,_,_,_)", wren_dc_drawtri },
	{ "engine", "Draw", true, "mapLayer(_,_,_,_,_,_,_)", wren_dc_drawmaplayer },
	{ "engine", "Draw", true, "sprite(_,_,_,_,_,_,_,_)", wren_dc_drawsprite },
	{ "engine", "Draw", true, "submit()", wren_dc_submit },
	{ "engine", "Draw", true, "clear(_,_,_,_)", wren_dc_clear },

	{ "engine", "TileMap", true, "setCurrent(_)", wren_map_setcurrent },
	{ "engine", "TileMap", true, "layerByName(_)", wren_map_getlayerbyname },
	{ "engine", "TileMap", true, "layerNames()", wren_map_getlayernames },
	{ "engine", "TileMap", true, "objectsInLayer(_)", wren_map_getobjectsinlayer },
	{ "engine", "TileMap", true, "getMapProperties()", wren_map_getmapproperties },
	{ "engine", "TileMap", true, "getLayerProperties(_)", wren_map_getlayerproperties },
	{ "engine", "TileMap", true, "getTileProperties()", wren_map_gettileproperties },
	{ "engine", "TileMap", true, "getTile(_,_,_)", wren_map_gettile },
};
static const int methodsCount = sizeof(methods) / sizeof(wrenMethodDef);

WrenForeignMethodFn wren_bindForeignMethodFn(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature) {
	NOTUSED(vm);

	for (int i = 0; i < methodsCount; i++) {
		const wrenMethodDef &m = methods[i];
		if (strcmp(module, m.module) == 0 && strcmp(className, m.className) == 0 && isStatic == m.isStatic && strcmp(signature, m.signature) == 0) {
			return m.fn;
		}
	}

	return nullptr;
}

void cvarAllocate(WrenVM *vm) {
	wrenEnsureSlots(vm, 3);

	const conVar_t** var = (const conVar_t**)wrenSetSlotNewForeign(vm, 0, 0, sizeof(const conVar_t*));
	const char* name = wrenGetSlotString(vm, 1);

	auto valType = wrenGetSlotType(vm, 2);

	if (valType == WREN_TYPE_NUM) {
		double value = wrenGetSlotDouble(vm, 2);
		char cvarStr[1024];
		snprintf(cvarStr, 1024, "%g", value);
		*var = SLT_Con_GetVarDefault(name, cvarStr, 0);
	}
	else if (valType == WREN_TYPE_BOOL) {
		bool value = wrenGetSlotBool(vm, 2);
		*var = SLT_Con_GetVarDefault(name, value ? "1" : "0", 0);
	}
	else if (valType == WREN_TYPE_STRING) {
		const char *value = wrenGetSlotString(vm, 2);
		*var = SLT_Con_GetVarDefault(name, value, 0);
	}
	else {
		*var = SLT_Con_GetVarDefault(name, "0", 0);
	}
}

void cvarFinalize(void *data) {
	NOTUSED(data);

}

WrenForeignClassMethods wren_bindForeignClassFn(WrenVM* vm, const char* module, const char* className) {
	NOTUSED(vm);
	NOTUSED(module);

	WrenForeignClassMethods fnMethods;

	if (strcmp(className, "CVar") == 0) {
		fnMethods.allocate = cvarAllocate;
		fnMethods.finalize = cvarFinalize;
	}
	else {
		fnMethods.allocate = NULL;
		fnMethods.finalize = NULL;
	}

	return fnMethods;
}

#pragma endregion

#pragma region Public functions
WrenVM *Wren_Init(const char *mainScriptName, const char *constructorStr) {
	WrenConfiguration config;
	wrenInitConfiguration(&config);
	config.errorFn = wren_error;
	config.bindForeignMethodFn = wren_bindForeignMethodFn;
	config.loadModuleFn = wren_loadModuleFn;
	config.bindForeignClassFn = wren_bindForeignClassFn;

	WrenVM *vm = wrenNewVM(&config);

	// load passed in script name as our main function
	char *mainStr;
	int mainSz = SLT_FS_ReadFile(mainScriptName, (void**)&mainStr);
	if (mainSz <= 0) {
		SLT_Error(ERR_GAME, "%s: couldn't load %s", __func__, mainScriptName);
		return nullptr;
	}

	if (wrenInterpret(vm, mainStr) != WREN_RESULT_SUCCESS) {
		SLT_Error(ERR_GAME, "%s: can't compile %s", __func__, mainScriptName);
		return nullptr;
	}

	free(mainStr);

	// make sure we can find a new Game class
	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, "main", "Main", 0);
	WrenHandle *gameClass = wrenGetSlotHandle(vm, 0);

	if (gameClass == nullptr) {
		SLT_Error(ERR_GAME, "%s: couldn't find Game class", __func__);
		return nullptr;
	}

	// make a new instance of the Game class and grab handles to update/draw
	WrenHandle *newHnd = wrenMakeCallHandle(vm, "init(_)");

	wrenHandles_t *hnd = new wrenHandles_t();
	hnd->instanceHnd = gameClass;
	hnd->updateHnd = wrenMakeCallHandle(vm, "update(_)");
	hnd->drawHnd = wrenMakeCallHandle(vm, "draw(_,_)");
	hnd->shutdownHnd = wrenMakeCallHandle(vm, "shutdown()");
	hnd->consoleHnd = wrenMakeCallHandle(vm, "console(_)");

	if (hnd->updateHnd == nullptr) {
		SLT_Error(ERR_GAME, "%s: couldn't find static update(_) on Main", __func__);
		Wren_FreeVM(vm);
		return nullptr;
	}

	if (hnd->drawHnd == nullptr) {
		SLT_Error(ERR_GAME, "%s: couldn't find static draw(_,_) on Main", __func__);
		Wren_FreeVM(vm);
		return nullptr;
	}

	// instantiate a new Game
	wrenEnsureSlots(vm, 8192); // FIXME: crash when expanding stack from inside constructor
	wrenSetSlotHandle(vm, 0, gameClass);
	if (constructorStr == nullptr) {
		wrenSetSlotNull(vm, 1);
	}
	else {
		wrenSetSlotString(vm, 1, constructorStr);
	}
	wrenCall(vm, newHnd);
	wrenReleaseHandle(vm, newHnd);
	//wrenReleaseHandle(vm, gameClass);

	if (wrenGetSlotCount(vm) == 0) {
		SLT_Error(ERR_GAME, "%s: couldn't instantiate new Game class", __func__);
		return nullptr;
	}

	wrenSetUserData(vm, hnd);

	return vm;
}

bool Wren_Update(WrenVM *vm, double dt) {
	wrenHandles_t* hnd = (wrenHandles_t*)wrenGetUserData(vm);
	wrenEnsureSlots(vm, 2);
	wrenSetSlotHandle(vm, 0, hnd->instanceHnd);
	wrenSetSlotDouble(vm, 1, dt);
	wrenCall(vm, hnd->updateHnd);
	
	return wrenGetSlotType(vm, 0) == WREN_TYPE_BOOL ? wrenGetSlotBool(vm, 0) : true;
}

void Wren_Draw(WrenVM *vm, int w, int h) {
	wrenHandles_t* hnd = (wrenHandles_t*)wrenGetUserData(vm);
	wrenEnsureSlots(vm, 3);
	wrenSetSlotHandle(vm, 0, hnd->instanceHnd);
	wrenSetSlotDouble(vm, 1, w);
	wrenSetSlotDouble(vm, 2, h);
	wrenCall(vm, hnd->drawHnd);
}

void Wren_Console(WrenVM *vm, const char *str) {
	wrenHandles_t* hnd = (wrenHandles_t*)wrenGetUserData(vm);
	wrenEnsureSlots(vm, 2);
	wrenSetSlotHandle(vm, 0, hnd->instanceHnd);
	wrenSetSlotString(vm, 1, str);
	wrenCall(vm, hnd->consoleHnd);
}

void Wren_Eval(WrenVM *vm, const char *code) {
	wrenInterpret(vm, code);
}

void Wren_Scene_Shutdown(WrenVM *vm) {
	wrenHandles_t* hnd = (wrenHandles_t*)wrenGetUserData(vm);
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, hnd->instanceHnd);
	wrenCall(vm, hnd->shutdownHnd);
}

void Wren_FreeVM(WrenVM *vm) {
	wrenHandles_t* hnd = (wrenHandles_t*)wrenGetUserData(vm);

	if (hnd->drawHnd) wrenReleaseHandle(vm, hnd->drawHnd);
	if (hnd->updateHnd)	wrenReleaseHandle(vm, hnd->updateHnd);
	if (hnd->shutdownHnd) wrenReleaseHandle(vm, hnd->shutdownHnd);
	if (hnd->instanceHnd)	wrenReleaseHandle(vm, hnd->instanceHnd);
	if (hnd->consoleHnd) wrenReleaseHandle(vm, hnd->consoleHnd);

	delete hnd;

	wrenFreeVM(vm);
}
#pragma endregion
