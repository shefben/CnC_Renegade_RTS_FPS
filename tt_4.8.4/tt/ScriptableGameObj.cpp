#include "General.h"
#include "ScriptableGameObj.h"
#include "scripts.h"
void ScriptableGameObj::Export_Creation
   (BitStreamClass&)
{
}
void ScriptableGameObj::Import_Creation
   (BitStreamClass&)
{
   this->Remove_All_Observers();
}
void ScriptableGameObj::Remove_All_Observers()
{
   for (int u = 0; u < this->Observers.Count(); ++u)
      this->Observers[u]->Detach(this);

   this->Observers.Delete_All();
}

void Do_Object_Create(GameObject *obj);
void ScriptableGameObj::Start_Observers()
{
   for (int u = 0; u < this->Observers.Count(); ++u)
      this->Observers[u]->Created(this);
   Do_Object_Create(this);
}

void ScriptableGameObj::Post_Re_Init()
{
	Start_Observers();
}

RENEGADE_FUNCTION
void ScriptableGameObj::Think()
   AT2(0x006B6FE0,0x006B6880);
