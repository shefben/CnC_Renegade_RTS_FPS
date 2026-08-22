#include "General.h"
#include "render2d.h"
#include "sniperhud.h"
#include "FileHash.h"

REF_DEF1(_Sniper2DBaseRenderer, Render2DClass *, 0x0085F7F8);
REF_DEF1(_Sniper2DRenderer, Render2DClass *, 0x0085F830);

void SniperHUDClass::Render()
{
	if (_Sniper2DBaseRenderer)
	{
		_Sniper2DBaseRenderer->Render();
	}
	if (_Sniper2DRenderer)
	{
		_Sniper2DRenderer->Render();
	}
}

RENEGADE_FUNCTION
void SniperHUDClass::Update()
AT1(0x0072D8B0);

RENEGADE_FUNCTION
void SniperHUDClass::Build_Base()
AT1(0x0072D070);

void SniperHUDClass::Init()
{
	char *Name = newstr("hud_sniper.tga");
	FileClass *f = Get_Data_File(Name);
	if ((!f) || (!f->Is_Available()))
	{
		int len = strlen(Name);
		Name[len - 3] = 'd';
		Name[len - 2] = 'd';
		Name[len - 1] = 's';
		f = Get_Data_File(Name);
	}
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckLoad(data,size,Name,HashScope);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	_Sniper2DBaseRenderer = new Render2DClass();
	_Sniper2DBaseRenderer->Set_Texture(Name);
	_Sniper2DBaseRenderer->Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	Build_Base();
	_Sniper2DRenderer = new Render2DClass();
	_Sniper2DRenderer->Set_Texture(Name);
	_Sniper2DRenderer->Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	delete[] Name;
}

RENEGADE_FUNCTION
void SniperHUDClass::Shutdown()
AT1(0x0072D030);
