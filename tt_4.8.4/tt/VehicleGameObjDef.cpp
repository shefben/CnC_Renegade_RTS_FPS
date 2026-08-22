#include "general.h"

#include "scripts.h"
#include "engine_common.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "engine_vector.h"
#include "engine_io.h"
#include "VehicleGameObjDef.h"
#include "VehicleGameObj.h"
#include "TransitionGameObj.h"
#include "TransitionGameObjDef.h"


void VehicleGameObjDef::Free_Transition_List()
{
   for (sint32 u = 0; u < this->Transitions.Count(); ++u)
      delete this->Transitions[u];

   this->Transitions.Clear();
}

