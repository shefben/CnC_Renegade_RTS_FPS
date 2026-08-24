/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/bridgesection.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	What a bridge is made of, and what each piece of it looks like in each of the states it     *
 *	can be in.                                                                                  *
 *                                                                                             *
 *	Roadmap Section 20 names the five kinds of section -- a cap at each end, a span repeated    *
 *	as many times as the gap needs, a support under the joints, and decoration that is          *
 *	optional -- and the four states each of them can be in.  It also says what a state may      *
 *	change: the model, the collision, whether anything can cross, whether the path still        *
 *	connects, and what debris comes off.  That list is exactly the contents of                  *
 *	BridgeStateRuleClass, and a definition is a table of those, one per kind per state.         *
 *                                                                                             *
 *	The donor, Zero Hour's W3DBridge, has left mesh, section mesh and right mesh, a width       *
 *	scale, and one damage state for the whole bridge chosen from four models.  The three        *
 *	meshes are the caps and the span, and the reason this generalises them into a table is      *
 *	Section 20's phrase "broken spans": a bridge that can be broken in the middle and still     *
 *	stand at both ends needs damage per section, not per bridge, because the question anybody   *
 *	asks about a damaged bridge is whether you can still get across it.                         *
 *                                                                                             *
 *	No bridge art exists.  A definition whose model names are empty, or whose models are not    *
 *	in the mix files, still lays out, still collides and can still be driven over: the system   *
 *	builds the section as a slab of its declared dimensions.  See docs/assets/BridgeSections.md *
 *	-- a bridge nobody can stand on would fail the acceptance of the section that builds it,    *
 *	so missing art degrades to plain geometry rather than to a hole in the world.               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BRIDGESECTION_H
#define BRIDGESECTION_H

#include "always.h"
#include "matrix3d.h"
#include "vector3.h"
#include "wwstring.h"

class StaticPhysClass;

enum BridgeSectionKindType
{
	BRIDGE_SECTION_START_CAP	= 0,	// the abutment the road arrives at
	BRIDGE_SECTION_SPAN,					// repeated as many times as the gap needs
	BRIDGE_SECTION_END_CAP,				// the abutment the road leaves from
	BRIDGE_SECTION_SUPPORT,				// a pier under a joint, down to the ground
	BRIDGE_SECTION_DECORATION,			// lamps, railings, cables -- omitted when unnamed
	BRIDGE_SECTION_KIND_COUNT
};

enum BridgeDamageStateType
{
	BRIDGE_STATE_PRISTINE		= 0,	// as built
	BRIDGE_STATE_DAMAGED,				// visibly hurt, still carries everything
	BRIDGE_STATE_CRITICAL,				// about to go; still crossable
	BRIDGE_STATE_BROKEN,					// down.  Nothing crosses here
	BRIDGE_STATE_COUNT
};

const char *	Bridge_Section_Kind_Name(int kind);
const char *	Bridge_Damage_State_Name(int state);


/*
**	One kind of section, in one state.  Everything Section 20 says a state may change.
*/
class BridgeStateRuleClass
{
public:

	BridgeStateRuleClass(void) :
		Collides(true), Traversable(true), DebrisCount(0) { }

	bool operator == (const BridgeStateRuleClass & that) const
	{
		return (Model == that.Model) && (Collides == that.Collides) &&
				 (Traversable == that.Traversable) && (Debris == that.Debris) &&
				 (DebrisCount == that.DebrisCount);
	}
	bool operator != (const BridgeStateRuleClass & that) const	{ return !(*this == that); }

	StringClass		Model;			// what to draw.  Empty means a slab of the declared size
	bool				Collides;		// whether anything can stand on it
	bool				Traversable;	// whether the path still runs through it
	StringClass		Debris;			// emitted once, on arriving in this state
	int				DebrisCount;
};


/*
**	A kind of bridge: the sizes its sections are built at and the table of state rules.
*/
class BridgeDefinitionClass
{
public:

	BridgeDefinitionClass(void);

	bool	operator == (const BridgeDefinitionClass & that) const;
	bool	operator != (const BridgeDefinitionClass & that) const	{ return !(*this == that); }

	void			Set_Name(const char * name)		{ Name = (name != nullptr) ? name : ""; }
	const char *	Get_Name(void) const					{ return Name.Peek_Buffer(); }

	void			Set_Width(float width)				{ Width = width; }
	float			Get_Width(void) const				{ return Width; }
	void			Set_Deck_Thickness(float t)		{ DeckThickness = t; }
	float			Get_Deck_Thickness(void) const	{ return DeckThickness; }

	//	How long one section of this kind is, along the bridge.  A support's is its footprint.
	void			Set_Section_Length(int kind,float length);
	float			Get_Section_Length(int kind) const;

	//	Metres between piers.  Zero means a bridge that stands on its abutments alone.
	void			Set_Support_Spacing(float spacing)	{ SupportSpacing = spacing; }
	float			Get_Support_Spacing(void) const		{ return SupportSpacing; }
	//	How far the ground has to be below the deck before a pier is worth building.
	void			Set_Support_Clearance(float c)		{ SupportClearance = c; }
	float			Get_Support_Clearance(void) const		{ return SupportClearance; }

	//	Metres between decorations, on both sides.  Zero, or an unnamed model, means none.
	void			Set_Decoration_Spacing(float s)		{ DecorationSpacing = s; }
	float			Get_Decoration_Spacing(void) const	{ return DecorationSpacing; }
	//	How far out from the centre line the decorations sit, as a share of the half width.
	void			Set_Decoration_Inset(float i)			{ DecorationInset = i; }
	float			Get_Decoration_Inset(void) const		{ return DecorationInset; }

	BridgeStateRuleClass &			Peek_Rule(int kind,int state);
	const BridgeStateRuleClass &	Get_Rule(int kind,int state) const;

	void			Set_Model(int kind,int state,const char * name);
	const char *	Get_Model(int kind,int state) const;
	void			Set_Debris(int kind,int state,const char * name,int count);
	void			Set_Collides(int kind,int state,bool onoff);
	void			Set_Traversable(int kind,int state,bool onoff);

	bool			Is_Traversable(int kind,int state) const;
	bool			Does_Collide(int kind,int state) const;

	//	True when this definition names no art at all, which is every definition today.
	bool			Names_Any_Model(void) const;

private:

	static bool	Is_Valid(int kind,int state);

	StringClass				Name;
	float						Width;
	float						DeckThickness;
	float						SectionLength[BRIDGE_SECTION_KIND_COUNT];
	float						SupportSpacing;
	float						SupportClearance;
	float						DecorationSpacing;
	float						DecorationInset;

	BridgeStateRuleClass	Rules[BRIDGE_SECTION_KIND_COUNT][BRIDGE_STATE_COUNT];

	static BridgeStateRuleClass	_EmptyRule;
};


/*
**	One section, placed.  Where it is, how big it is, what state it is in, and the physics
**	object standing there once the geometry has been built.
*/
class BridgeSectionClass
{
public:

	BridgeSectionClass(void) :
		Kind(BRIDGE_SECTION_SPAN), State(BRIDGE_STATE_PRISTINE), Ordinal(0),
		Start(0.0f,0.0f,0.0f), End(0.0f,0.0f,0.0f), Length(0.0f), HalfWidth(0.0f),
		Drop(0.0f), Transform(1), Phys(nullptr), ModelMissing(false) { }

	bool operator == (const BridgeSectionClass & that) const
	{
		return (Kind == that.Kind) && (State == that.State) && (Ordinal == that.Ordinal) &&
				 (Start == that.Start) && (End == that.End) &&
				 (Length == that.Length) && (HalfWidth == that.HalfWidth) && (Drop == that.Drop);
	}
	bool operator != (const BridgeSectionClass & that) const	{ return !(*this == that); }

	Vector3	Get_Center(void) const	{ return (Start + End) * 0.5f; }

	int				Kind;					// BridgeSectionKindType
	int				State;				// BridgeDamageStateType
	int				Ordinal;				// which span, pier or decoration this is, from the start
	Vector3			Start;				// on the deck centre line, at deck height
	Vector3			End;					// likewise.  Equal to Start for a pier or a decoration
	float				Length;				// along the bridge
	float				HalfWidth;			// of the deck here
	float				Drop;					// a pier's height, deck to ground.  Zero for anything else
	Matrix3D			Transform;			// centre of the section; X along, Y left, Z up
	StaticPhysClass *	Phys;				// null until the geometry is built; owned by BridgeSystem
	bool				ModelMissing;		// true when a slab is standing in for art that is not there
};

#endif // BRIDGESECTION_H
