#pragma once

#include "resource.h"
#include <vector>
extern "C" {
	#include "bdd.h"
	#include "bdd_tools.h"
}

typedef bdd *TDiagramInfoPtr;
typedef std::vector<TDiagramInfoPtr> TDiagramArray;

struct TDisplayState
{	
	TDisplayState();
	~TDisplayState();
	void AddDiagram(bdd *diagram);
	void AddUpdatedDiagram();
	            // Adds diagram at the same address as in last call of AddDiagram
	bool ChangeCurDiagram(int delta);
	// bdd_root_node *GetCurDiagram();
	bdd_level_info *GetCurDiagramByLevels();
	int GetCurDiagramLevelCount();

	void SetCurWindowSize(HWND hWnd);
	void SetCurDiagramScrollArea(const RECT &scrollArea_);
	bool NeedToShowScrollBars();
	void GetCurDiagramScrollValues(POINT &scroll, POINT &minValues, POINT &maxValues, 
	                               SIZE &pageSizes);
	SIZE GetCurClientArea();

private:
	TDiagramArray diagrams;
	int curDiagramIndex;
	bdd_level_info *curDiagramByLevels;

	bdd *lastDiagramSource;  
	bool cloningInProgress;
	SIZE windowSize, clientAreaSizeDelta;
	RECT scrollArea;
	POINT lastScroll;
	bool prevShowScrollBars;
};