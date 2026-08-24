/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/bridgesection.cpp                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *   BridgeDefinitionClass::BridgeDefinitionClass -- sizes that make a bridge a bridge          *
 *   BridgeDefinitionClass::Set_Section_Length -- how long one piece is                         *
 *   BridgeDefinitionClass::Peek_Rule -- one kind in one state                                  *
 *   BridgeDefinitionClass::Names_Any_Model -- whether any art has been chosen yet              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "bridgesection.h"

BridgeStateRuleClass	BridgeDefinitionClass::_EmptyRule;


const char * Bridge_Section_Kind_Name(int kind)
{
	switch (kind) {
		case BRIDGE_SECTION_START_CAP:	return "START_CAP";
		case BRIDGE_SECTION_SPAN:			return "SPAN";
		case BRIDGE_SECTION_END_CAP:		return "END_CAP";
		case BRIDGE_SECTION_SUPPORT:		return "SUPPORT";
		case BRIDGE_SECTION_DECORATION:	return "OPTIONAL_DECORATION";
		default:									return "?";
	}
}


const char * Bridge_Damage_State_Name(int state)
{
	switch (state) {
		case BRIDGE_STATE_PRISTINE:	return "PRISTINE";
		case BRIDGE_STATE_DAMAGED:		return "DAMAGED";
		case BRIDGE_STATE_CRITICAL:	return "CRITICAL";
		case BRIDGE_STATE_BROKEN:		return "BROKEN";
		default:								return "?";
	}
}


/***********************************************************************************************
 * BridgeDefinitionClass::BridgeDefinitionClass -- sizes that make a bridge a bridge            *
 *                                                                                             *
 *	The defaults are a road-width deck on eight metre spans with a pier every twenty-four, and  *
 *	the only state that changes anything is BROKEN, which stops carrying and stops colliding.   *
 *	A definition made and used without being told anything else is therefore a plain bridge     *
 *	that can be destroyed, which is what Section 20 is about; everything else is dressing.      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BridgeDefinitionClass::BridgeDefinitionClass(void) :
	Width(8.0f),
	DeckThickness(0.6f),
	SupportSpacing(24.0f),
	SupportClearance(2.0f),
	DecorationSpacing(0.0f),
	DecorationInset(0.9f)
{
	SectionLength[BRIDGE_SECTION_START_CAP]		= 6.0f;
	SectionLength[BRIDGE_SECTION_SPAN]			= 8.0f;
	SectionLength[BRIDGE_SECTION_END_CAP]			= 6.0f;
	SectionLength[BRIDGE_SECTION_SUPPORT]			= 2.0f;
	SectionLength[BRIDGE_SECTION_DECORATION]		= 0.5f;

	for (int kind = 0; kind < BRIDGE_SECTION_KIND_COUNT; kind++) {
		for (int state = 0; state < BRIDGE_STATE_COUNT; state++) {
			Rules[kind][state].Collides		= (state != BRIDGE_STATE_BROKEN);
			Rules[kind][state].Traversable	= (state != BRIDGE_STATE_BROKEN);
			Rules[kind][state].DebrisCount	= 0;
		}
	}

	//	Decoration is never what holds a bridge up, and a lamp post is not a floor.  Whatever
	//	state it is in, nothing stands on it and nothing crosses on it.
	for (int state = 0; state < BRIDGE_STATE_COUNT; state++) {
		Rules[BRIDGE_SECTION_DECORATION][state].Collides		= false;
		Rules[BRIDGE_SECTION_DECORATION][state].Traversable	= false;
	}

	//	A pier is a thing in the water you can hit, not a thing you drive along.
	for (int state = 0; state < BRIDGE_STATE_COUNT; state++) {
		Rules[BRIDGE_SECTION_SUPPORT][state].Traversable = false;
	}
}


bool BridgeDefinitionClass::operator == (const BridgeDefinitionClass & that) const
{
	if ((Name != that.Name) || (Width != that.Width) || (DeckThickness != that.DeckThickness) ||
		 (SupportSpacing != that.SupportSpacing) || (SupportClearance != that.SupportClearance) ||
		 (DecorationSpacing != that.DecorationSpacing) || (DecorationInset != that.DecorationInset)) {
		return false;
	}
	for (int kind = 0; kind < BRIDGE_SECTION_KIND_COUNT; kind++) {
		if (SectionLength[kind] != that.SectionLength[kind]) { return false; }
		for (int state = 0; state < BRIDGE_STATE_COUNT; state++) {
			if (Rules[kind][state] != that.Rules[kind][state]) { return false; }
		}
	}
	return true;
}


bool BridgeDefinitionClass::Is_Valid(int kind,int state)
{
	return (kind >= 0) && (kind < BRIDGE_SECTION_KIND_COUNT) &&
			 (state >= 0) && (state < BRIDGE_STATE_COUNT);
}


/***********************************************************************************************
 * BridgeDefinitionClass::Set_Section_Length -- how long one piece is                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void BridgeDefinitionClass::Set_Section_Length(int kind,float length)
{
	if ((kind < 0) || (kind >= BRIDGE_SECTION_KIND_COUNT)) { return; }
	if (length < 0.0f) { length = 0.0f; }
	SectionLength[kind] = length;
}


float BridgeDefinitionClass::Get_Section_Length(int kind) const
{
	if ((kind < 0) || (kind >= BRIDGE_SECTION_KIND_COUNT)) { return 0.0f; }
	return SectionLength[kind];
}


/***********************************************************************************************
 * BridgeDefinitionClass::Peek_Rule -- one kind in one state                                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BridgeStateRuleClass & BridgeDefinitionClass::Peek_Rule(int kind,int state)
{
	if (!Is_Valid(kind,state)) { return _EmptyRule; }
	return Rules[kind][state];
}


const BridgeStateRuleClass & BridgeDefinitionClass::Get_Rule(int kind,int state) const
{
	if (!Is_Valid(kind,state)) { return _EmptyRule; }
	return Rules[kind][state];
}


void BridgeDefinitionClass::Set_Model(int kind,int state,const char * name)
{
	if (!Is_Valid(kind,state)) { return; }
	Rules[kind][state].Model = (name != nullptr) ? name : "";
}


const char * BridgeDefinitionClass::Get_Model(int kind,int state) const
{
	if (!Is_Valid(kind,state)) { return ""; }
	return Rules[kind][state].Model.Peek_Buffer();
}


void BridgeDefinitionClass::Set_Debris(int kind,int state,const char * name,int count)
{
	if (!Is_Valid(kind,state)) { return; }
	Rules[kind][state].Debris = (name != nullptr) ? name : "";
	Rules[kind][state].DebrisCount = (count > 0) ? count : 0;
}


void BridgeDefinitionClass::Set_Collides(int kind,int state,bool onoff)
{
	if (!Is_Valid(kind,state)) { return; }
	Rules[kind][state].Collides = onoff;
}


void BridgeDefinitionClass::Set_Traversable(int kind,int state,bool onoff)
{
	if (!Is_Valid(kind,state)) { return; }
	Rules[kind][state].Traversable = onoff;
}


bool BridgeDefinitionClass::Is_Traversable(int kind,int state) const
{
	if (!Is_Valid(kind,state)) { return false; }
	return Rules[kind][state].Traversable;
}


bool BridgeDefinitionClass::Does_Collide(int kind,int state) const
{
	if (!Is_Valid(kind,state)) { return false; }
	return Rules[kind][state].Collides;
}


/***********************************************************************************************
 * BridgeDefinitionClass::Names_Any_Model -- whether any art has been chosen yet                *
 *                                                                                             *
 *	False for every definition in the default table, which is the honest state of the art and   *
 *	the reason the slab path exists.  A caller that wants to say so in a log asks this rather   *
 *	than guessing from one empty string.                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeDefinitionClass::Names_Any_Model(void) const
{
	for (int kind = 0; kind < BRIDGE_SECTION_KIND_COUNT; kind++) {
		for (int state = 0; state < BRIDGE_STATE_COUNT; state++) {
			if (Rules[kind][state].Model.Get_Length() > 0) { return true; }
		}
	}
	return false;
}
