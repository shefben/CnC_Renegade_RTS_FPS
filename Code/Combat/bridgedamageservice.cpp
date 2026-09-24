/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/bridgedamageservice.cpp               $*
 *                                                                                             *
 *	P15.  See bridgedamageservice.h.                                                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "bridgedamageservice.h"

#include "bridgesystem.h"
#include "combat.h"
#include "generatedworldplacementrules.h"
#include "logisticsgraphservice.h"
#include "navmeshgenerator.h"
#include "vector.h"
#include "worldsynctransport.h"
#include "wwdebug.h"


const float	BridgeDamageService::SPAN_HIT_POINTS = 1000.0f;

struct SpanDamage
{
	SpanDamage(void) : BridgeId(-1),Section(-1),Damage(0.0f) { }

	bool	operator == (const SpanDamage &) const	{ return false; }
	bool	operator != (const SpanDamage &) const	{ return true; }

	int	BridgeId;
	int	Section;
	float	Damage;
};

static DynamicVectorClass<SpanDamage>	_Damage;
static int										_BreakCount = 0;


static SpanDamage *	Find_Span(int bridge_id,int section,bool create)
{
	for (int i = 0; i < _Damage.Count (); i++) {
		if ((_Damage [i].BridgeId == bridge_id) && (_Damage [i].Section == section)) return &_Damage [i];
	}
	if (!create) return nullptr;

	SpanDamage entry;
	entry.BridgeId	= bridge_id;
	entry.Section	= section;
	_Damage.Add (entry);
	return &_Damage [_Damage.Count () - 1];
}


void BridgeDamageService::Init(void)	{ Reset (); }

void BridgeDamageService::Reset(void)
{
	_Damage.Delete_All ();
	_BreakCount = 0;
}


int BridgeDamageService::State_For_Damage(float damage)
{
	if (damage >= SPAN_HIT_POINTS)					return BRIDGE_STATE_BROKEN;
	if (damage >= SPAN_HIT_POINTS * (2.0f / 3.0f))	return BRIDGE_STATE_CRITICAL;
	if (damage >= SPAN_HIT_POINTS * (1.0f / 3.0f))	return BRIDGE_STATE_DAMAGED;
	return BRIDGE_STATE_PRISTINE;
}


//	After a span falls: everything that reads "can this be crossed" is asked again.
static void	On_Span_Broken(int bridge_id)
{
	_BreakCount++;

	if (NavMeshGenerator::Has_Navigation ()) {
		NavMeshParams params;
		StringClass error ("");
		if (NavMeshGenerator::Build (params,&error)) {
			StringClass choke_error ("");
			GeneratedWorldPlacementRules::Build_Chokepoints (&choke_error);
		} else {
			WWDEBUG_SAY (("BridgeDamageService: navigation not rebuilt after bridge %d fell: %s\r\n",
							  bridge_id,error.Peek_Buffer ()));
		}
	}

	LogisticsGraphService::Update_Route_Status ();
}


int BridgeDamageService::Apply_Explosion(const Vector3 & position,float radius,float strength)
{
	if (!CombatManager::I_Am_Server ()) return 0;
	if ((radius <= 0.0f) || (strength <= 0.0f)) return 0;
	if (BridgeSystem::Get_Bridge_Count () <= 0) return 0;

	int changed = 0;

	for (int b = 0; b < BridgeSystem::Get_Bridge_Count (); b++) {
		BridgeClass * bridge = BridgeSystem::Peek_Bridge (b);			// a bridge's id is its index
		if (bridge == nullptr) continue;
		int bridge_id = bridge->Get_ID ();

		for (int s = 0; s < bridge->Get_Section_Count (); s++) {
			const BridgeSectionClass & section = bridge->Get_Section (s);
			if (section.Kind != BRIDGE_SECTION_SPAN) continue;
			if (section.State == BRIDGE_STATE_BROKEN) continue;

			//	Distance to the span's deck line, so a blast at either end of a long span still
			//	counts, falling off linearly to nothing at the blast's radius.
			Vector3 along = section.End - section.Start;
			float length2 = along.Length2 ();
			float t = (length2 > 0.0001f) ? Vector3::Dot_Product (position - section.Start,along) / length2 : 0.0f;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
			float distance = ((section.Start + along * t) - position).Length ();
			if (distance >= radius) continue;

			SpanDamage * span = Find_Span (bridge_id,s,true);
			span->Damage += strength * (1.0f - (distance / radius));

			int wanted = State_For_Damage (span->Damage);
			if (wanted <= section.State) continue;		// states only ever advance

			if (BridgeSystem::Set_Section_State (bridge_id,s,wanted)) {
				changed++;
				if (wanted == BRIDGE_STATE_BROKEN) {
					WWDEBUG_SAY (("BridgeDamageService: bridge %d span %d fell (%.0f damage)\r\n",
									  bridge_id,s,span->Damage));
					On_Span_Broken (bridge_id);
				}
			}
		}
	}

	if (changed > 0) {
		WorldSyncTransport::Broadcast_Bridge_State ();
	}
	return changed;
}


float BridgeDamageService::Get_Span_Damage(int bridge_id,int section_index)
{
	SpanDamage * span = Find_Span (bridge_id,section_index,false);
	return (span != nullptr) ? span->Damage : 0.0f;
}


int BridgeDamageService::Get_Break_Count(void)	{ return _BreakCount; }
