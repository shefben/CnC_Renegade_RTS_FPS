#include "general.h"

#include "GameModeClass.h"



GameModeClass::GameModeClass()
   : state(1)
{
}



GameModeClass::~GameModeClass()
{
}



void GameModeClass::Activate()
{
   if (this->state == 1)
   {
      this->Init();
      this->state = 0;
   }
   else if (this->state == 2)
      this->state = 0;
}



void GameModeClass::Deactivate()
{
   if (!this->Is_Inactive())
      this->state = 2;
}



bool GameModeClass::Is_Active()
{
   return (!this->state);
}



bool GameModeClass::Is_Inactive()
{
   return (this->state - 1 <= 1);
}



bool GameModeClass::Is_Suspended()
{
   return (this->state == 3);
}



void GameModeClass::Resume()
{
   if (this->state == 3)
      this->state = 0;
}



void GameModeClass::Safely_Deactivate()
{
   if (this->state == 2)
   {
      this->Shutdown();
      this->state = 1;
   }
}



void GameModeClass::Suspend()
{
   if (!this->state)
      this->state = 3;
}
