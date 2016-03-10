// BddDisplay.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BddDisplay.h"
extern "C" {
	#include "bdd.h"
	#include "bdd_apply.h"
	#include "bdd_sifting.h"
	#include "bdd_tools.h"
	#include "blif_common.h"
}

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TDisplayState g_displayState;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void PrepareDiagramsToDisplay(const char *inputFileName);
void OnContinueCalculations(HWND hWnd);
void OnChangeDisplayedDiagram(HWND hWnd, int diagram_index_delta);
void DrawCurrentDiagram(HWND hWnd);
void OnHVScroll(HWND hwnd, int bar, UINT code);

TDisplayState::TDisplayState()
	: curDiagramIndex(0),
		curDiagramByLevels(NULL),
		lastDiagramSource(NULL),
		cloningInProgress(false)
{
	clientAreaSizeDelta.cx = 1;
}

TDisplayState::~TDisplayState()
{
	cloningInProgress = true;
	for (TDiagramArray::const_iterator it = diagrams.begin(); it != diagrams.end(); it++)
		bdd_free_diagram(*it);
}

void TDisplayState::AddDiagram(bdd *diagram)
{
	diagrams.push_back(bdd_clone_diagram(diagram));
	lastDiagramSource = diagram;
	cloningInProgress = false;
}

void TDisplayState::AddUpdatedDiagram()
{
	if (lastDiagramSource != NULL && !cloningInProgress)
	{
		cloningInProgress = true;
		diagrams.push_back(bdd_clone_diagram(lastDiagramSource));
		cloningInProgress = false;
	}
}

bool TDisplayState::ChangeCurDiagram(int delta)
{
	int oldValue = curDiagramIndex;

	curDiagramIndex += delta;
	if (curDiagramIndex < 0)
		curDiagramIndex = 0;
	else  if (curDiagramIndex >= (int)diagrams.size())
		curDiagramIndex = (int)diagrams.size() - 1;

	if (curDiagramIndex != oldValue && curDiagramByLevels != NULL)
	{
		free_level_infos(curDiagramByLevels, diagrams[oldValue]->var_count);
		curDiagramByLevels = NULL;
	}
	return oldValue != curDiagramIndex;
}

//bdd_root_node *TDisplayState::GetCurDiagram()
//{
//	return diagrams[curDiagramIndexs];
//}

bdd_level_info *TDisplayState::GetCurDiagramByLevels()
{
	if (diagrams.empty())
		return NULL;

	if (curDiagramByLevels == NULL)
	{
		bdd *diagram = diagrams[curDiagramIndex];

		curDiagramByLevels = create_level_infos(diagram->var_count);
		bdd_divide_nodes_by_level(curDiagramByLevels, diagram->var_count, diagram->root_node);
	}
	return curDiagramByLevels;
}

int TDisplayState::GetCurDiagramLevelCount()
{
	return diagrams[curDiagramIndex]->var_count;
}


void TDisplayState::SetCurWindowSize(HWND hWnd)
{
	RECT windowRect, clientRect;

	GetWindowRect(hWnd, &windowRect);
	windowSize.cx = windowRect.right - windowRect.left;
	windowSize.cy = windowRect.bottom - windowRect.top;
	if (clientAreaSizeDelta.cx >= 0)
	{
		ShowScrollBar(hWnd, SB_BOTH, FALSE);
		if (clientAreaSizeDelta.cx >= 0)     
		{
			GetClientRect(hWnd, &clientRect);
			clientAreaSizeDelta.cx = clientRect.right - clientRect.left - windowSize.cx;
			clientAreaSizeDelta.cy = clientRect.bottom - clientRect.top - windowSize.cy;
			assert(clientAreaSizeDelta.cx <= 0 && clientAreaSizeDelta.cy <= 0);

			lastScroll.x = 0;
			lastScroll.y = 0;
			prevShowScrollBars = false;
		}     // Else this method was just called one more time recursively and 
		      // already calculated clientAreaSizeDelta
	}
}

void TDisplayState::SetCurDiagramScrollArea(const RECT &scrollArea_)
{
	scrollArea = scrollArea_;
	if (!prevShowScrollBars && scrollArea.left < lastScroll.x)
		lastScroll.x = scrollArea.left;
}

void TDisplayState::GetCurDiagramScrollValues(POINT &scroll, POINT &minValues, POINT &maxValues, 
	                                            SIZE &pageSizes)
{
	SIZE clientAreaSize = { windowSize.cx + clientAreaSizeDelta.cx,
		                      windowSize.cy + clientAreaSizeDelta.cy };

	minValues.x = scrollArea.left;
	maxValues.x = scrollArea.right - scrollArea.left; 
	minValues.y = scrollArea.top;
	maxValues.y = scrollArea.bottom; 
	if (maxValues.x < minValues.x)
		maxValues.x = minValues.x;
	if (maxValues.y < minValues.y)
		maxValues.y = minValues.y;
	pageSizes = clientAreaSize;

	bool showScrollBars = minValues.x + pageSizes.cx < maxValues.x || 
		                    minValues.y + pageSizes.cy < maxValues.y;

	if (prevShowScrollBars && !showScrollBars)
	{
		lastScroll.x = scrollArea.left;
		lastScroll.y = 0;
	}
	if (showScrollBars)
	{
		pageSizes.cx -= 50;
		if (pageSizes.cx < 20)
			pageSizes.cx = 20;
		pageSizes.cy -= 50;
		if (pageSizes.cy < 20)
			pageSizes.cy = 20;
	}
	
	scroll = lastScroll;
	prevShowScrollBars = showScrollBars;
}


void Report_Error(const char *err)
{
	OutputDebugString(err);
	OutputDebugString(_T("\n"));
}   


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BDDDISPLAY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BDDDISPLAY));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BDDDISPLAY));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BDDDISPLAY);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

	 if (__argc <= 1)
	 {
		 MessageBox(NULL, "Usage: BddDisplay.exe <.blif file name>", "Error", MB_OK);
		 return FALSE;
	 }

   hInst = hInstance; // Store instance handle in our global variable
	 bdd_set_error_callback(&Report_Error);
	 PrepareDiagramsToDisplay(__argv[1]);

   hWnd = CreateWindow(szWindowClass, szTitle, 
				WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_CONTINUE_CALCULATIONS:
			OnContinueCalculations(hWnd);
			break;
		case IDM_PREV_DIAGRAM:
			OnChangeDisplayedDiagram(hWnd, -1);
			break;
		case IDM_NEXT_DIAGRAM:
			OnChangeDisplayedDiagram(hWnd, 1);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		DrawCurrentDiagram(hWnd);
		break;
	case WM_HSCROLL:
		OnHVScroll(hWnd, SB_HORZ, LOWORD(wParam));
		break; 
	case WM_VSCROLL:
		OnHVScroll(hWnd, SB_VERT, LOWORD(wParam));
		break; 
	case WM_SIZE:
		g_displayState.SetCurWindowSize(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void AddUpdatedDiagram()
{
	g_displayState.AddUpdatedDiagram();
}

bdd *bdd_display_create_diagram_from_blif(t_blif_cubical_function *function)
{
	bdd *diagram = bdd_create_diagram(function->input_count);
	int cube_index;

	g_displayState.AddDiagram(diagram);
	for (cube_index = 0; cube_index < function->cube_count; cube_index++)
		bdd_add_cube_path(diagram, diagram->root_node, 
						          function->set_of_cubes[cube_index], 0, cube_index + 1);
	return diagram;   // This is actually a tree
}

void PrepareDiagramsToDisplay(const char *inputFileName)
{
	t_blif_logic_circuit *circuit = ReadBLIFCircuit((char *)inputFileName);

	if (circuit != NULL)
	{
		int index;

		//for (index = 0; index < circuit->function_count; index++)
		//{
		//	t_blif_cubical_function *function = circuit->list_of_functions[index];

		//	simplify_function(function);
		//}

		bdd_set_diagram_changed_callback(&AddUpdatedDiagram, 5);

		bdd *saved_diagrams[] = { NULL, NULL };

		for (index = 0; index < circuit->function_count; index++)
		{
			t_blif_cubical_function *function = circuit->list_of_functions[index];
			int var_count = function->input_count;
			bdd *diagram = bdd_display_create_diagram_from_blif(function);
			bdd_level_info *levels = create_level_infos(var_count);

			bdd_divide_nodes_by_level(levels, var_count, diagram->root_node);
			bdd_label_nodes_by_levels(levels, var_count);
			free_level_infos(levels, var_count);

			g_displayState.AddUpdatedDiagram();

			bdd_reduce(diagram);

			bdd_do_sifting(diagram);

			if (index < 2)
				saved_diagrams[index] = diagram;
			else
				bdd_free_diagram(diagram);
		}

		if (circuit->function_count >= 2)
		{
			// Both diagrams must contain variables in the same order.
			// But one diagram may contain less variables
			int left_diagram_num = saved_diagrams[0]->var_count <= saved_diagrams[1]->var_count;
			int var_count = saved_diagrams[left_diagram_num]->var_count;
			bdd *diagram = bdd_create_diagram(var_count);
			
			g_displayState.AddDiagram(diagram);
			strcpy(diagram->root_node->label, "Apply");
			bdd_add_apply_result(diagram, 
				                   saved_diagrams[left_diagram_num], apply_operation_and,
						               saved_diagrams[1 - left_diagram_num], 1, var_count);
			g_displayState.AddUpdatedDiagram();
			
			bdd_reduce(diagram);
			bdd_free_diagram(diagram);
		}

		for (int i = 0; i < sizeof(saved_diagrams) / sizeof(saved_diagrams[0]); i++)
			if (saved_diagrams[i] != NULL)
				bdd_free_diagram(saved_diagrams[i]);

		DeleteBLIFCircuit(blif_circuit);
	}
	else
	{
		OutputDebugString(_T("Error reading BLIF file. Terminating.\n"));
	}
}


void OnChangeDisplayedDiagram(HWND hWnd, int diagram_index_delta)
{
	if (g_displayState.ChangeCurDiagram(diagram_index_delta))
	{
		InvalidateRect (hWnd, NULL, TRUE);
		UpdateWindow (hWnd);
	}
}

void OnContinueCalculations(HWND hWnd)
{
  // ...

	InvalidateRect (hWnd, NULL, TRUE);
  UpdateWindow (hWnd);
}

int GetNodeIndexAtLevel(bdd_level_info *levels, bdd_node *node)
{
	bdd_level_info *level = &(levels[node->var_index]);
	bdd_node_info *info;
	int index = 0;

	for (info = level->info_head; info != NULL; info = info->next)
	{
		if (info->node == node)
			return index;
		index++;
	}
	Report_Error("Node not found");
	return -1;
}

int GetLevelNodeCount(bdd_level_info *level)
{
	int index;
	bdd_node_info *info;

  index = 0;
	for (info = level->info_head; info != NULL; info = info->next)
		index++;
	return index;
}

int c_margin = 5,
    g_junctionNodeSize = 50,
    c_terminalNodeSize = 50,
		g_stepX = 90,
		g_stepY = 80,
		c_childArcShiftsX[2] = { -3, 3 };
int g_terminalNodesX[2], g_terminalNodesY;

void DrawArcToChildNode(HDC hdc, bdd_level_info *levels, HPEN hPens[],
	                      int parentX, int parentY, const POINT &scroll, 
												int to_high, bdd_node *node)
{
	int targetX, targetY;

	if (bdd_is_node_terminal(node))
	{
		targetX = g_terminalNodesX[bdd_is_terminal_node_1(node)] - scroll.x + 
					c_terminalNodeSize / 2 + c_childArcShiftsX[to_high];
		targetY = g_terminalNodesY - scroll.y;
		SelectObject(hdc, hPens[to_high + ((targetY - parentY <= g_stepY * 2) ? 0 : 2)]);
	}
	else  if (node == NULL)
	{
		targetX = parentX + g_junctionNodeSize / 2 + c_childArcShiftsX[to_high] * 5;
		targetY = parentY + int(g_junctionNodeSize * 1.5);
		SelectObject(hdc, hPens[4]);
	} 
	else
	{
		int index_at_level = GetNodeIndexAtLevel(levels, node);

		targetX = levels[node->var_index].leftX - scroll.x + index_at_level * g_stepX +
					g_junctionNodeSize / 2 + c_childArcShiftsX[to_high];
		targetY = c_margin - scroll.y + node->var_index * g_stepY;
		SelectObject(hdc, hPens[to_high + ((targetY - parentY <= g_stepY) ? 0 : 2)]);
		int err = GetLastError();
	}

	MoveToEx(hdc, parentX + g_junctionNodeSize / 2 + c_childArcShiftsX[to_high], 
			    parentY + g_junctionNodeSize, NULL);
	LineTo(hdc, targetX, targetY);
}

void DrawTerminalNodes(HDC hdc, const POINT &scroll)
{
	char st[10];

	for (int i = 0; i < 2; i++)
	{
		RECT rect = { g_terminalNodesX[i] - scroll.x, g_terminalNodesY - scroll.y, 
			            g_terminalNodesX[i] + c_terminalNodeSize - scroll.x, 
									g_terminalNodesY + c_terminalNodeSize - scroll.y };

		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		sprintf(st, "%d", i);
		DrawText(hdc, st, -1, &rect, 
				       DT_SINGLELINE | DT_NOCLIP | DT_CENTER | DT_VCENTER);
	}
}


int GetScrollPos(HWND hwnd, int bar, UINT code)
{
  SCROLLINFO si = {};
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
  GetScrollInfo(hwnd, bar, &si);

  const int minPos = si.nMin;
  const int maxPos = si.nMax - (si.nPage - 1);

  int result = -1;

  switch(code)
  {
  case SB_LINEUP /*SB_LINELEFT*/:
      result = max(si.nPos - 1, minPos);
      break;

  case SB_LINEDOWN /*SB_LINERIGHT*/:
      result = min(si.nPos + 1, maxPos);
      break;

  case SB_PAGEUP /*SB_PAGELEFT*/:
      result = max(si.nPos - (int)si.nPage, minPos);
      break;

  case SB_PAGEDOWN /*SB_PAGERIGHT*/:
      result = min(si.nPos + (int)si.nPage, maxPos);
      break;

  case SB_THUMBPOSITION:
      // do nothing
      break;

  case SB_THUMBTRACK:
      result = si.nTrackPos;
      break;

  case SB_TOP /*SB_LEFT*/:
      result = minPos;
      break;

  case SB_BOTTOM /*SB_RIGHT*/:
      result = maxPos;
      break;

  case SB_ENDSCROLL:
      // do nothing
      break;
  }

  return result;
}

void OnHVScroll(HWND hWnd, int bar, UINT code)
{
	const int scrollPos = GetScrollPos(hWnd, bar, code);

	if(scrollPos == -1)
		return;
	SetScrollPos(hWnd, bar, scrollPos, TRUE);

	InvalidateRect (hWnd, NULL, TRUE);
	DrawCurrentDiagram(hWnd);
}


void UpdateScrollBars(POINT &scroll, HWND hWnd, const RECT &scrollArea)
{
	POINT minValues, maxValues;
	SIZE pageSizes;
	SCROLLINFO info;
	
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = SIF_RANGE | SIF_PAGE; 

	g_displayState.GetCurDiagramScrollValues(scroll, minValues, maxValues, pageSizes);
	if ((GetKeyState(VK_SCROLL) & 1))
		return;
	
	if (minValues.x + pageSizes.cx < maxValues.x)
	{
		ShowScrollBar(hWnd, SB_HORZ, TRUE);
		info.nMin = minValues.x;
		info.nMax = maxValues.x;
		info.nPage = pageSizes.cx;
		SetScrollInfo(hWnd, SB_HORZ, &info, TRUE);

		scroll.x = GetScrollPos(hWnd, SB_HORZ);
	}
	else
		ShowScrollBar(hWnd, SB_HORZ, FALSE);

	if (minValues.y + pageSizes.cy < maxValues.y)
	{
		scroll.y = GetScrollPos(hWnd, SB_VERT);
		ShowScrollBar(hWnd, SB_VERT, TRUE);
		info.nMin = minValues.y;
		info.nMax = maxValues.y;
		info.nPage = pageSizes.cy;
		SetScrollInfo(hWnd, SB_VERT, &info, TRUE);

		scroll.y = GetScrollPos(hWnd, SB_VERT);
	}
	else
		ShowScrollBar(hWnd, SB_VERT, FALSE);
}

void FillLevelsInfo(int &max_level_node_count, int &terminalNodesY ,RECT &scrollArea, 
	                  bdd_level_info *levels, int var_count)
{
	int level_index, level_index2;

	max_level_node_count = 0;
	terminalNodesY = c_margin + var_count * g_stepY + 20;
	scrollArea.left = c_margin; 
	scrollArea.top = 0;
	scrollArea.right = c_margin + g_stepX + c_terminalNodeSize;
	scrollArea.bottom = terminalNodesY + c_terminalNodeSize;

	for (level_index = 0; level_index < var_count; level_index++)
	{
		int node_count = GetLevelNodeCount(&(levels[level_index]));

		if (max_level_node_count < node_count)
		{
			for (level_index2 = 0; level_index2 < level_index; level_index2++)
				levels[level_index2].leftX += (node_count - max_level_node_count) * g_stepX / 2; 
			max_level_node_count = node_count;
		}
		levels[level_index].leftX = c_margin + 
			(max_level_node_count - node_count) * g_stepX / 2;

		int rightX = levels[level_index].leftX + g_stepX * (node_count - 1) + g_junctionNodeSize;

		if (scrollArea.left > levels[level_index].leftX)
			scrollArea.left = levels[level_index].leftX;
		if (scrollArea.right < rightX)
			scrollArea.right = rightX;
	}
}

void DrawCurrentDiagram(HWND hWnd)
{
	bdd_level_info *levels = g_displayState.GetCurDiagramByLevels();

	if (levels == NULL)
		return;

	int var_count = g_displayState.GetCurDiagramLevelCount();
	int max_level_node_count;
	RECT scrollArea;
	POINT scroll;
	int centerX;

	//if (var_count > 10)
	//{
	//	g_junctionNodeSize = 40;
	//	g_stepX = 45;
	//	g_stepY = 45;
	//}
	FillLevelsInfo(max_level_node_count, g_terminalNodesY, scrollArea, 
		             levels, var_count);

	centerX = c_margin + ((max_level_node_count - 1) * g_stepX + 
				g_junctionNodeSize) / 2;
	g_terminalNodesX[0] = centerX - 90;
	g_terminalNodesX[1] = centerX + 90 - c_terminalNodeSize;
	if (scrollArea.left > g_terminalNodesX[0] )
		scrollArea.left = g_terminalNodesX[0] ;
	if (scrollArea.right < g_terminalNodesX[1] + c_terminalNodeSize)
		scrollArea.right = g_terminalNodesX[1] + c_terminalNodeSize;
	scrollArea.left -= c_margin;

	g_displayState.SetCurDiagramScrollArea(scrollArea);
	UpdateScrollBars(scroll, hWnd, scrollArea);
	
	int level_index;
	bdd_node_info *info;
	int x, y;
	PAINTSTRUCT ps;
	HDC hdc;
	HPEN hPens[] = { CreatePen(PS_DOT, 1, RGB(0, 0, 0)),
				           CreatePen(PS_SOLID, 1, RGB(0, 0, 0)),
				           CreatePen(PS_DOT, 1, RGB(200, 200, 200)),
				           CreatePen(PS_SOLID, 1, RGB(200, 200, 200)),
				           CreatePen(PS_SOLID, 1, RGB(255, 0, 0)) };

	hdc = BeginPaint(hWnd, &ps);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));
	SetTextColor(hdc, RGB(50, 50, 50));
	SetBkMode(hdc,TRANSPARENT);

	DrawTerminalNodes(hdc, scroll);

	y = c_margin - scroll.y;
	for (level_index = 0; level_index < var_count; level_index++)
	{
		x = levels[level_index].leftX - scroll.x;
		for (info = levels[level_index].info_head; info != NULL; info = info->next)
		{
			bdd_node *node = info->node;
			
			SelectObject(hdc, hPens[1]);
			Ellipse(hdc, x, y, x + g_junctionNodeSize, y + g_junctionNodeSize);
			DrawArcToChildNode(hdc, levels, hPens, x, y, scroll, 0, node->low);
			DrawArcToChildNode(hdc, levels, hPens, x, y, scroll, 1, node->high);

			RECT rect = { x, y, x + g_junctionNodeSize, y + g_junctionNodeSize };
			char st[BDD_MAX_NODE_LABEL_LEN + 10];

			sprintf(st, "%s %d", node->label, node->table_id);
			DrawText(hdc, st, -1, &rect, 
			//DrawText(hdc, node->label, -1, &rect, 
				       DT_SINGLELINE | DT_NOCLIP | DT_CENTER | DT_VCENTER);

			x += g_stepX;
		}
		y += g_stepY;
	}

	for (int i = 0; i < sizeof(hPens) / sizeof(hPens[0]); i++)
		DeleteObject(hPens[i]);
	EndPaint(hWnd, &ps);
}