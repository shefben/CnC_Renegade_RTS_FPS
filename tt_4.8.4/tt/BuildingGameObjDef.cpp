#include "general.h"

#include "BuildingGameObjDef.h"



int BuildingGameObjDef::Get_Destroy_Report
   (int teamId) const
{
   if (teamId == 0)
      return this->nodDestroyReportStringId;
   else if (teamId == 1)
      return this->gdiDestroyReportStringId;

   return 0;
}



