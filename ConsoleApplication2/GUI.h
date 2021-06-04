#pragma once
#include "GKS.h"
#include <Windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl.h>

class GUIForm {
private:
	Timetable* m_Timetable;

	HWND hWnd = nullptr;
	HMODULE hInstance = nullptr;
	Microsoft::WRL::ComPtr<ID2D1Factory> pD2DFactory;
	Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> pRT;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBrushBorder;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBrushDetail[5];
	Microsoft::WRL::ComPtr<IDWriteFactory> m_pDWriteFactory;
	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_pTextFormat;

	BOOL InitWindow();
	bool InitD2D(HWND hwnd);
	bool InitD2DHWResources(HWND hwnd);
public:
	GUIForm(Timetable* t);
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void DrawGantasDiagram();
	void Show();//block
};