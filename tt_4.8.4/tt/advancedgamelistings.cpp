#include "General.h"
#include "advancedgamelistings.h"
#include "render2d.h"
#include "listctrl.h"
#include "dialogmgr.h"
#include "scripts.h"
#include "resource.h"
void QuickMatchDisable()
{
	MenuDialogClass* dialog;
	__asm mov dialog, ecx
	
	Vector2 position;

	DialogControlClass* quickmatchControl = dialog->Get_Dlg_Item(IDC_MP_WOL_QUICK_MATCH_BUTTON);
	DialogControlClass* advancedGameListingsControl = dialog->Get_Dlg_Item(IDC_MP_WOL_ADVANCED_LISTINGS_BUTTON);
	DialogControlClass* chatControl = dialog->Get_Dlg_Item(11048);

	position = advancedGameListingsControl->Get_Window_Pos();
	position.Y = Render2DClass::Get_Screen_Resolution().Bottom * 81 / 300; // Position of "Westwood Online"
	advancedGameListingsControl->Set_Window_Pos(position);

	position = chatControl->Get_Window_Pos();
	position.Y = Render2DClass::Get_Screen_Resolution().Bottom * 116 / 300; // Position of "Gamespy"
	chatControl->Set_Window_Pos(position);

	dialog->Remove_Control(quickmatchControl);
	delete quickmatchControl;

	DialogMgrClass::Set_Focus(advancedGameListingsControl);
	dialog->Clear_Last_Focus();

	__asm
	{
		mov ecx, dialog
		mov eax, 0x004A4A20
		call eax
	}
}



AdvancedGameListingsMenu::AdvancedGameListingsMenu(int resourceid) :
	MenuDialogClass(resourceid)
{
}


SimpleDynVecClass<const char *> FavoriteServers;
bool AdvancedGameListingsMenu::isFavourite(ListCtrlClass* list, int index)
{
	for (int i = 0;i < FavoriteServers.Count();i++)
	{
		const char *x = WideCharToChar(list->Get_Entry_Text(index, 2));
		const char *y = FavoriteServers[i];
		if (!strcmp(x,y))
		{
			delete[] x;
			return true;
		}
		delete[] x;
	}
	return false;
}


int __stdcall AdvancedGameListingsMenu::iconSortCallback(ListCtrlClass* list, int left, int right, uint32 userData)
{
	AdvancedGameListingsMenu* _this = (AdvancedGameListingsMenu*)userData;

	uint leftData = list->Get_Entry_Data(left, 0);
	uint rightData = list->Get_Entry_Data(right, 0);
	
	int result = 0;

	if (_this->sortFlags == 1)
	{
		// Hijack (the already broken) dedicated server sort to sort by favourites

		// Sort favourite servers on top
		if (!result)
			result = (int)isFavourite(list, right) - (int)isFavourite(list, left);

		// Sort grayed out servers on bottom
		if (!result)
			result = -(int)((leftData & 1) - (rightData & 1));

		// Sort passworded servers on bottom
		if (!result)
			result = (leftData & 4) - (rightData & 4);

		// Sort clan servers on bottom
		if (!result)
			result = (leftData & 8) - (rightData & 8);

		uint leftPlayerCount = list->Get_Entry_Data(left, 4);
		uint leftPlayerLimit = HIWORD(leftPlayerCount);
		leftPlayerCount = LOWORD(leftPlayerCount);

		uint rightPlayerCount = list->Get_Entry_Data(right, 4);
		uint rightPlayerLimit = HIWORD(rightPlayerCount);
		rightPlayerCount = LOWORD(rightPlayerCount);

		// Sort full servers on bottom
		if (!result)
			result = (int)(leftPlayerCount == leftPlayerLimit) - (int)(rightPlayerCount == rightPlayerLimit);
		
		// Sort descending by player count
		if (!result)
			result = -(int)(leftPlayerCount - rightPlayerCount);

	}
	else
	{
		// Sort by selected flag
		if (!result)
			result = (leftData & _this->sortFlags) - (rightData & _this->sortFlags);
	}
	
	// Sort by ping
	if (!result)
		result = list->Get_Entry_Data(left, 5) - list->Get_Entry_Data(right, 5);
	
	// Sort by hostname
	if (!result)
		result = _wcsicmp(list->Get_Entry_Text(left, 1), list->Get_Entry_Text(right, 1));
	
	// Swap ascending/descending if needed
	if (!_this->sortAscending)
		result = -result;

	return result;
}


int __stdcall AdvancedGameListingsMenu::stringSortCallback(ListCtrlClass* list, int left, int right, uint32 userData)
{
	AdvancedGameListingsMenu* _this = (AdvancedGameListingsMenu*)userData;
	
	const wchar_t* leftText = list->Get_Entry_Text(left, _this->sortColumn);
	const wchar_t* rightText = list->Get_Entry_Text(right, _this->sortColumn);
	int result = _wcsicmp(leftText, rightText);
	
	if (result == 0)
		result = list->Get_Entry_Data(left, 5) - list->Get_Entry_Data(right, 5);
	
	if (!_this->sortAscending)
		result = -result;
	
	return result;
}


int __stdcall AdvancedGameListingsMenu::intSortCallback(ListCtrlClass* list, int left, int right, uint32 userData)
{
	AdvancedGameListingsMenu* _this = (AdvancedGameListingsMenu*)userData;

	int result = list->Get_Entry_Data(left, _this->sortColumn) - list->Get_Entry_Data(right, _this->sortColumn);
	if (result == 0)
	{
		if (_this->sortColumn != 5)
			result = list->Get_Entry_Data(left, 5) - list->Get_Entry_Data(right, 5);
		
		if (result == 0)
		{
			const wchar_t* leftText = list->Get_Entry_Text(left, _this->sortColumn);
			const wchar_t* rightText = list->Get_Entry_Text(right, _this->sortColumn);
			result = _wcsicmp(leftText, rightText);
		}
	}
	
	if (!_this->sortAscending)
		result = -result;
		
	return result;
}

void ReadWolFavorites()
{
	int handle = Commands->Text_File_Open("favorites.txt");
	if (!handle)
	{
		return;
	}
	char data[100];
	while (Commands->Text_File_Get_String(handle,data,100))
	{
		const char *c = strtrim(newstr(data));
		FavoriteServers.Add(c);
	}
	Commands->Text_File_Close(handle);
}
void DestroyFavorites()
{
	for (int i = 0;i < FavoriteServers.Count();i++)
	{
		delete[] FavoriteServers[i];
	}
}

void AdvancedGameListingsMenu::addFavorite(int _sortColumn, bool _sortAscending, int _sortFlags)
{
	ListCtrlClass* listControl = (ListCtrlClass*)Get_Dlg_Item(IDC_GAME_LIST_CTRL);
	if (!listControl || !listControl->As_ListCtrlClass() || (listControl->Get_Curr_Sel() == 0xFFFFFFFF))
	{
		return;
	}
	const char *c = WideCharToChar(listControl->Get_Entry_Text(listControl->Get_Curr_Sel(),2));
	bool found = false;
	for (int i = 0;i < FavoriteServers.Count();i++)
	{
		const char *y = FavoriteServers[i];
		if (!strcmp(c,y))
		{
			found = true;
			FavoriteServers.Delete(i);
		}
	}
	if (!found)
	{
		FavoriteServers.Add(c);
	}
	FILE *f = fopen("data\\favorites.txt","wt");
	for (int i = 0;i < FavoriteServers.Count();i++)
	{
		fprintf(f,"%s\n",FavoriteServers[i]);
	}
	fclose(f);
	sortList(0,1,1);
}

void AdvancedGameListingsMenu::sortList(int _sortColumn, bool _sortAscending, int _sortFlags)
{
	sortColumn = _sortColumn;
	sortAscending = _sortAscending;
	sortFlags = _sortFlags;
		
	ListCtrlClass* listControl = (ListCtrlClass*)Get_Dlg_Item(IDC_GAME_LIST_CTRL);
		
	if (!listControl || !listControl->As_ListCtrlClass())
		return;

	if (sortColumn == 0)
		listControl->Sort(iconSortCallback, (uint)this);
	
	else if (sortColumn != 4 && sortColumn != 5)
		listControl->Sort(stringSortCallback, (uint)this);
	
	else
		listControl->Sort(intSortCallback, (uint)this);
	
	ListCtrlClass::SORT_TYPE sortType = sortAscending ? ListCtrlClass::SORT_ASCENDING : ListCtrlClass::SORT_DESCENDING;
	listControl->Set_Sort_Designator(sortColumn, sortType);
}
