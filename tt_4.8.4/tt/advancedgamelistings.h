#include "menudialog.h"

class AdvancedGameListingsMenu :
	public MenuDialogClass
{

private:

	uint unk0074;
	uint unk0078;
	uint unk007C;
	uint unk0080;
	uint unk0084;
	uint unk0088;
	uint unk008C;
	uint unk0090;
	uint unk0094;
	uint unk0098;
	uint unk009C;
	uint unk00A0;
	uint unk00A4;
	uint unk00A8;
	uint unk00AC;
	uint unk00B0;
	uint unk00B4;
	uint unk00B8;
	uint unk00BC;
	uint unk00C0;
	uint unk00C4;
	uint unk00C8;
	uint sortColumn;
	bool sortAscending;
	uint sortFlags;
public:
	AdvancedGameListingsMenu(int resourceid);
	static bool isFavourite(ListCtrlClass* list, int index);
	static int __stdcall iconSortCallback(ListCtrlClass* list, int left, int right, uint32 userData);
	static int __stdcall stringSortCallback(ListCtrlClass* list, int left, int right, uint32 userData);
	static int __stdcall intSortCallback(ListCtrlClass* list, int left, int right, uint32 userData);
	void sortList(int _sortColumn, bool _sortAscending, int _sortFlags);
	void addFavorite(int _sortColumn, bool _sortAscending, int _sortFlags);
};
void ReadWolFavorites();
void DestroyFavorites();
