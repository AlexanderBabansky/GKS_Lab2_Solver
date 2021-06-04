#include "GUI.h"
#include <map>
#include <functional>
#include <string>
#include <set>

#define MAX_LOADSTRING 50
#define BORDER_MARGIN 50
#define BORDER_MARGIN_LEFT 150
#define BORDER_MARGIN_BOTTOM 100
#define BORDER_WIDTH 3
#define MARK_AXIS_SIZE 10
#define MARK_AXIS_TEXT_WIDTH 50

WCHAR szTitle[MAX_LOADSTRING] = L"GKS";
WCHAR szWindowClass[MAX_LOADSTRING] = L"MainWindowClass";
static const WCHAR msc_fontName[] = L"Verdana";
static const FLOAT msc_fontSize = 20;

LRESULT CALLBACK WndProcG(HWND, UINT, WPARAM, LPARAM);
std::map<HWND, GUIForm*> wnd_proc_list;

GUIForm::GUIForm(Timetable* t) :
	m_Timetable(t)
{
	_ASSERT(t);
}

bool GUIForm::InitD2D(HWND hwnd)
{
	HRESULT hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		pD2DFactory.GetAddressOf()
	);
	if (FAILED(hr)) { return false; }

	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(m_pDWriteFactory),
		reinterpret_cast<IUnknown**>(m_pDWriteFactory.GetAddressOf())
	);
	if (FAILED(hr)) { return false; }

	hr = m_pDWriteFactory->CreateTextFormat(
		msc_fontName,
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		msc_fontSize,
		L"", //locale
		&m_pTextFormat
	);
	if (FAILED(hr)) { return false; }
	m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	return InitD2DHWResources(hwnd);
}

bool GUIForm::InitD2DHWResources(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	HRESULT hr = pD2DFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(
			hwnd,
			D2D1::SizeU(
				rc.right - rc.left,
				rc.bottom - rc.top)
		),
		pRT.GetAddressOf()
	);
	if (FAILED(hr)) { return false; }
	pRT->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Black),
		&pBrushBorder
	);

	D2D1_COLOR_F colors[5]{
		D2D1::ColorF(D2D1::ColorF::Orange),
		D2D1::ColorF(D2D1::ColorF::Red),
		D2D1::ColorF(D2D1::ColorF::Green),
		D2D1::ColorF(D2D1::ColorF::Blue),
		D2D1::ColorF(D2D1::ColorF::Green)
	};

	for (int a = 0; a < 5; a++) {
		pRT->CreateSolidColorBrush(
			colors[a],
			pBrushDetail[a].GetAddressOf()
		);
	}
	return true;
}

LRESULT GUIForm::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		DrawGantasDiagram();
	}
	break;
	case WM_SIZE:
		InitD2DHWResources(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

int TimeToPixels(int t, RECT rc, int last_time) {
	float koef = t;
	koef /= last_time;
	koef *= (rc.right - BORDER_MARGIN - BORDER_MARGIN_LEFT);
	return koef;
}

void GUIForm::DrawGantasDiagram()
{
	_ASSERT(pRT);
	RECT rc{ 0 };
	GetClientRect(hWnd, &rc);
	pRT->BeginDraw();
	pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));
	pRT->DrawLine(D2D1::Point2F(BORDER_MARGIN_LEFT - BORDER_WIDTH / 2, BORDER_MARGIN), D2D1::Point2F(BORDER_MARGIN_LEFT - BORDER_WIDTH / 2, rc.bottom - BORDER_MARGIN_BOTTOM), pBrushBorder.Get(), BORDER_WIDTH);
	pRT->DrawLine(D2D1::Point2F(BORDER_MARGIN_LEFT, rc.bottom - BORDER_MARGIN_BOTTOM + BORDER_WIDTH / 2), D2D1::Point2F(rc.right - BORDER_MARGIN, rc.bottom - BORDER_MARGIN_BOTTOM + BORDER_WIDTH / 2), pBrushBorder.Get(), BORDER_WIDTH);

	auto tt = m_Timetable->GetTimetable();
	for (int a = 0; a < tt.size(); a++) {
		std::wstring row_cap = L"Верстат ";
		wchar_t row_cap_id[2]{ 0 };
		_itow_s(a + 1, row_cap_id, 10);
		row_cap += row_cap_id;
		pRT->DrawTextW(
			row_cap.c_str(),
			wcslen(row_cap.c_str()),
			m_pTextFormat.Get(),
			D2D1::RectF(0, (rc.bottom - BORDER_MARGIN_BOTTOM - BORDER_MARGIN) / 3 * a + BORDER_MARGIN, BORDER_MARGIN_LEFT, (rc.bottom - BORDER_MARGIN_BOTTOM - BORDER_MARGIN) / 3 * (a + 1) + BORDER_MARGIN),//(rc.bottom - BORDER_MARGIN_BOTTOM - BORDER_MARGIN) / 3
			pBrushBorder.Get()
		);
	}

	int oper = 0;
	std::set<int> existing_times;
	for (auto& i : tt) {
		for (auto& ps : i) {
			if (ps.detail != -1) {
				float x_1 = TimeToPixels(ps.start_time, rc, m_Timetable->GetLastTime()), x_2 = TimeToPixels(ps.start_time + ps.duration, rc, m_Timetable->GetLastTime());
				pRT->FillRectangle(D2D1::RectF(
					BORDER_MARGIN_LEFT + x_1,
					(rc.bottom - BORDER_MARGIN_BOTTOM - BORDER_MARGIN) / (tt.size()) * oper + BORDER_MARGIN,
					BORDER_MARGIN_LEFT + x_2,
					(rc.bottom - BORDER_MARGIN_BOTTOM - BORDER_MARGIN) / (tt.size()) * (oper + 1) + BORDER_MARGIN)
					, pBrushDetail[ps.detail].Get());

				wchar_t det_w[3]{ 0 };
				_itow_s(ps.detail + 1, det_w, 10);
				pRT->DrawTextW(det_w, wcslen(det_w), m_pTextFormat.Get(), D2D1::RectF(
					BORDER_MARGIN_LEFT + x_1,
					(rc.bottom - BORDER_MARGIN_BOTTOM - BORDER_MARGIN) / 3 * oper + BORDER_MARGIN,
					BORDER_MARGIN_LEFT + x_2,
					(rc.bottom - BORDER_MARGIN_BOTTOM - BORDER_MARGIN) / 3 * (oper + 1) + BORDER_MARGIN)
					, pBrushBorder.Get());

				if (existing_times.find(ps.start_time) == existing_times.end()) {
					existing_times.insert(ps.start_time);
					int markX = TimeToPixels(ps.start_time, rc, m_Timetable->GetLastTime());
					wchar_t start_time_w[4]{ 0 };
					_itow_s(ps.start_time, start_time_w, 10);
					pRT->DrawLine(D2D1::Point2F(BORDER_MARGIN_LEFT + markX, rc.bottom - BORDER_MARGIN_BOTTOM), D2D1::Point2F(BORDER_MARGIN_LEFT + markX, rc.bottom - BORDER_MARGIN_BOTTOM + MARK_AXIS_SIZE), pBrushBorder.Get(), BORDER_WIDTH);
					pRT->DrawTextW(start_time_w, wcslen(start_time_w), m_pTextFormat.Get(), D2D1::RectF(
						BORDER_MARGIN_LEFT + markX - MARK_AXIS_TEXT_WIDTH,
						rc.bottom - BORDER_MARGIN_BOTTOM + MARK_AXIS_SIZE,
						BORDER_MARGIN_LEFT + markX + MARK_AXIS_TEXT_WIDTH,
						rc.bottom
					), pBrushBorder.Get());
				}
			}
		}
		oper++;
	}


	pRT->EndDraw();
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex{ 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProcG;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = szWindowClass;
	return RegisterClassExW(&wcex);
}

BOOL GUIForm::InitWindow() {
	hInstance = GetModuleHandle(NULL);
	MyRegisterClass(hInstance);
	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	wnd_proc_list[hWnd] = this;
	InitD2D(hWnd);
	ShowWindow(hWnd, SW_NORMAL);
	UpdateWindow(hWnd);
	return true;
}

void GUIForm::Show()
{
	InitWindow();

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT CALLBACK WndProcG(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return wnd_proc_list[hWnd]->WndProc(hWnd, message, wParam, lParam);
}