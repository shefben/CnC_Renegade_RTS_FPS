#include "General.h"
#include "dx8texman.h"
#include "DX8Wrapper.h"
MultiListClass<DX8TextureTrackerClass> DX8TextureManagerClass::Managed_Textures;
void DX8TextureManagerClass::Add(DX8TextureTrackerClass* track)
{
	Managed_Textures.Add(track);
}
void DX8TextureManagerClass::Remove(TextureClass* tex)
{
	MultiListIterator<DX8TextureTrackerClass> it(&Managed_Textures);
	for (it.First();!it.Is_Done();it.Next())
	{
		DX8TextureTrackerClass *ttc = it.Peek_Obj();
		if (ttc->Texture == tex)
		{
			it.Remove_Current_Object();
			delete ttc;
		}
	}
}
void DX8TextureManagerClass::Release_Textures()
{
	MultiListIterator<DX8TextureTrackerClass> it(&Managed_Textures);
	for (it.First();!it.Is_Done();it.Next())
	{
		DX8TextureTrackerClass *ttc = it.Peek_Obj();
		SafeRelease(ttc->Texture->D3DTexture);
	}
}
void DX8TextureManagerClass::Recreate_Textures()
{
	MultiListIterator<DX8TextureTrackerClass> it(&Managed_Textures);
	for (it.First();!it.Is_Done();it.Next())
	{
		DX8TextureTrackerClass *ttc = it.Peek_Obj();
		ttc->Texture->D3DTexture = DX8Wrapper::Create_DX8_Texture(ttc->Width,ttc->Height,ttc->Format,ttc->Mip_level_count,D3DPOOL_DEFAULT,ttc->RenderTarget);
		ttc->Texture->Dirty = true;
	}
}

void DX8TextureManagerClass::Shutdown()
{
	while (!Managed_Textures.Is_Empty())
		delete Managed_Textures.Remove_Head();
}
