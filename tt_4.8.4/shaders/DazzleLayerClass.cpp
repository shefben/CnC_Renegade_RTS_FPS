#include "General.h"
#include "DazzleLayerClass.h"



RENEGADE_FUNCTION
void DazzleLayerClass::Render(CameraClass* camera)
AT2(0x00541800, 0x005410A0);

REF_DEF1(type_count, unsigned int, 0x008306C0);
#pragma warning(disable: 6386) //Buffer overrun: accessing 'visible_lists', not an actual issue
DazzleLayerClass::DazzleLayerClass()
{
	visible_lists = 0;
	if (type_count)
	{
		visible_lists = new DazzleRenderObjClass*[type_count];
	}
	for (unsigned int i = 0;i < type_count;i++)
	{
		visible_lists[i] = 0;
	}
}
#pragma warning(default: 6386)

DazzleLayerClass::~DazzleLayerClass()
{
	for (unsigned int i = 0;i < type_count;i++)
	{
		Clear_Visible_List(i);
	}
	if (visible_lists)
	{
		delete[] visible_lists;
	}
}

RENEGADE_FUNCTION
int DazzleLayerClass::Get_Visible_Item_Count(unsigned int type)
AT1(0x005418A0);

RENEGADE_FUNCTION
void DazzleLayerClass::Clear_Visible_List(unsigned int type)
AT1(0x005418D0);
