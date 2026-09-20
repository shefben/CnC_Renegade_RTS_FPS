/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/bridgesystem.h                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Bridges built out of sections, from the two points somebody said the bridge went between.   *
 *                                                                                             *
 *	Roadmap Section 20's acceptance is that bridges support variable length, damage, broken     *
 *	spans, collision and multiplayer state.  Each of those is a thing this service does:        *
 *                                                                                             *
 *	  variable length   a bridge is a start, an end and a definition.  The number of spans is   *
 *	                    worked out from the gap and the span length, and the spans are then     *
 *	                    stretched slightly so the last one lands on the far abutment.  No       *
 *	                    length is special and no length is modelled.                            *
 *	  damage            every section has its own state, and the states come from the           *
 *	                    definition's table.                                                     *
 *	  broken spans      because damage is per section, a bridge can be standing at both ends    *
 *	                    and impassable in the middle, which is the only interesting thing to    *
 *	                    do to a bridge.  Is_Traversable answers the question that follows.      *
 *	  collision         a bridge deck is a floor.  Unlike a road, which is a picture on ground  *
 *	                    that already collides, a bridge is the only surface over the gap, so    *
 *	                    each section that collides in its state goes into the physics scene as  *
 *	                    a static object.                                                        *
 *	  multiplayer state the server owns the states; Get_State_Block writes them and             *
 *	                    Apply_State_Block reads them.  Nothing about the geometry is sent,      *
 *	                    because a client with the same definition and the same two endpoints    *
 *	                    lays out exactly the same sections.                                     *
 *                                                                                             *
 *	The road side of the join already exists: a road whose end connection is                    *
 *	ROAD_ENDPOINT_BRIDGE records where its surface stopped and which way it pointed, and        *
 *	Connect_Roads binds those ends to the bridge abutment they are standing at.                 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BRIDGESYSTEM_H
#define BRIDGESYSTEM_H

#include "always.h"
#include "bridgesection.h"
#include "vector.h"
#include "vector3.h"
#include "wwstring.h"

class RenderObjClass;
class StaticPhysClass;


/*
**	One bridge, placed.  Two points, a kind, and the sections that came out of laying it out.
*/
class BridgeClass
{
public:

	BridgeClass(void);
	BridgeClass(const BridgeClass & that);
	~BridgeClass(void);

	BridgeClass &	operator = (const BridgeClass & that);
	bool				operator == (const BridgeClass & that) const;
	bool				operator != (const BridgeClass & that) const	{ return !(*this == that); }

	void				Reset(void);

	void				Set_Name(const char * name);
	const char *	Get_Name(void) const					{ return Name.Peek_Buffer(); }
	void				Set_ID(int id)							{ ID = id; }
	int				Get_ID(void) const					{ return ID; }

	void				Set_Definition(const char * name);
	const char *	Get_Definition(void) const			{ return Definition.Peek_Buffer(); }

	void				Set_Endpoints(const Vector3 & start,const Vector3 & end);
	const Vector3 &	Get_Start(void) const			{ return Start; }
	const Vector3 &	Get_End(void) const				{ return End; }

	//	Zero takes the definition's width, which is the usual case.
	void				Set_Width(float width)				{ Width = width; }
	float				Get_Width(void) const				{ return Width; }

	float				Get_Length(void) const;
	Vector3			Get_Direction(void) const;

	int				Get_Section_Count(void) const		{ return Sections.Count(); }
	const BridgeSectionClass &	Get_Section(int index) const;
	BridgeSectionClass *			Peek_Section(int index);

	/*
	**	Structural vocabulary.  Roadmap Section 52B, Phase 64.
	**
	**	Before this, a bridge had two endpoints and a width, and its height above the ground
	**	was whatever the author happened to put the endpoints at.  Clearance under it was an
	**	accident of how deep Apply_River_Cut had cut, which meant a crossing could not state
	**	what it was for, and nothing could check whether the terrain agreed.
	**
	**	DeckHeight is the deck's height above the higher of the two abutments.  Zero keeps
	**	the old behaviour exactly -- the deck follows the endpoints -- so every bridge that
	**	predates this is unchanged.
	**
	**	The abutment heights are filled in by Build_Layout from the terrain at each bank.
	**	They are an output, not an input: the ground is still the heightfield's to answer.
	*/
	void				Set_Deck_Height(float height)		{ DeckHeight = (height > 0.0f) ? height : 0.0f; }
	float				Get_Deck_Height(void) const		{ return DeckHeight; }
	bool				States_Deck_Height(void) const	{ return DeckHeight > 0.0f; }

	float				Get_Start_Abutment_Height(void) const	{ return StartAbutmentHeight; }
	float				Get_End_Abutment_Height(void) const		{ return EndAbutmentHeight; }
	bool				Has_Abutment_Heights(void) const			{ return AbutmentsConformed; }

	//	Which road, if any, arrives at each abutment.  -1 until Connect_Roads has run.
	void				Set_Start_Road(int id)				{ StartRoadID = id; }
	int				Get_Start_Road(void) const			{ return StartRoadID; }
	void				Set_End_Road(int id)					{ EndRoadID = id; }
	int				Get_End_Road(void) const			{ return EndRoadID; }

	//	The worst state any load-bearing section is in.  A convenience over the sections.
	int				Get_Damage_State(void) const;

	friend class BridgeSystem;

private:

	int				ID;
	StringClass		Name;
	StringClass		Definition;

	Vector3			Start;
	Vector3			End;
	float				Width;

	float				DeckHeight;
	float				StartAbutmentHeight;
	float				EndAbutmentHeight;
	bool				AbutmentsConformed;

	int				StartRoadID;
	int				EndRoadID;

	DynamicVectorClass<BridgeSectionClass>	Sections;
};


/*
**	One thing that came off a bridge when it changed state, waiting for somebody to draw it.
**
**	Section 20 lists debris emission among the things a damage state may change, and at the
**	time this queue was written there was no effect system to hand it to yet.  Rather than
**	drop it, the system records what came off and where.  Section 28 (debris rendering,
**	Code/Combat/debrissystem.h) is the consumer: it drains this queue every simulation frame
**	and clears it, so nothing here is read twice.
*/
class BridgeDebrisClass
{
public:
	BridgeDebrisClass(void) : BridgeID(-1), SectionIndex(-1), Count(0), Position(0.0f,0.0f,0.0f) { }

	bool operator == (const BridgeDebrisClass & that) const
	{
		return (BridgeID == that.BridgeID) && (SectionIndex == that.SectionIndex) &&
				 (Count == that.Count) && (Position == that.Position) && (Model == that.Model);
	}
	bool operator != (const BridgeDebrisClass & that) const	{ return !(*this == that); }

	int				BridgeID;
	int				SectionIndex;
	int				Count;
	Vector3			Position;
	StringClass		Model;
};


/*
**	What Check_Clearance found under a span.  Roadmap Section 52B, Phase 64.
**
**	The point of returning a report rather than a bool is that "this ravine is 0.4 metres too
**	shallow, forty metres along" is actionable and "false" is not -- by a level author, by
**	placement validation, and by any later generator deciding whether to cut deeper.
*/
struct BridgeClearanceReportStruct
{
	BridgeClearanceReportStruct(void) :
		Stated(false), Measured(false), Satisfied(false),
		RequiredClearance(0.0f), MinimumClearance(0.0f),
		WorstPosition(0.0f,0.0f,0.0f), WorstDistanceAlong(0.0f), SampleCount(0) { }

	bool		Stated;					// the definition states a required clearance at all
	bool		Measured;				// terrain answered for at least one sample
	bool		Satisfied;				// Measured, Stated, and the terrain is deep enough everywhere

	float		RequiredClearance;	// what the crossing promised
	float		MinimumClearance;		// the tightest headroom found
	Vector3	WorstPosition;			// where that was
	float		WorstDistanceAlong;	// and how far along the span
	int		SampleCount;
};


class BridgeSystem
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	//	The kinds of bridge that exist.
	static bool			Define_Bridge(const BridgeDefinitionClass & definition);
	static int			Get_Definition_Count(void);
	static BridgeDefinitionClass *	Peek_Definition(int index);
	static BridgeDefinitionClass *	Find_Definition(const char * name);
	static void			Define_Default_Bridges(void);

	//	The bridges on the map.
	static int			Add_Bridge(const BridgeClass & bridge);
	static BridgeClass *	Peek_Bridge(int id);
	static int			Get_Bridge_Count(void);
	static void			Clear_Bridges(void);

	static bool			Build_Layout(void);
	static bool			Is_Layout_Built(void);

	static bool			Build_Geometry(void);
	static void			Destroy_Geometry(void);
	static bool			Has_Geometry(void);
	static int			Get_Instance_Count(void);
	static int			Get_Missing_Model_Count(void);

	//	Damage.  The server calls these; a client gets them through the state block.
	static bool			Set_Damage_State(int bridge_id,int state);
	static bool			Set_Section_State(int bridge_id,int section_index,int state);
	static int			Get_Section_State(int bridge_id,int section_index);
	//	Break the span nearest a point -- what a shell landing on a bridge does.
	static bool			Break_Span_At(int bridge_id,const Vector3 & position);

	//	Traversal and pathing.
	static bool			Is_Traversable(int bridge_id);
	static bool			Find_Break(int bridge_id,int * section_index_out);
	static bool			Get_Traversable_Extent(int bridge_id,float * from_start_out,float * from_end_out);

	//	Where the deck is, for anything that walks, drives or paths over it.  False when the
	//	point is not over a bridge, or is over a part of one that no longer carries anything.
	static bool			Conform_Point(float x,float y,float * height_out,int * bridge_id_out = nullptr);
	static int			Find_Bridge_At(const Vector3 & position,float max_height_gap = 4.0f);

	/*
	**	Does the ground under this crossing actually leave the headroom the crossing says it
	**	needs?  Roadmap Section 52B, Phase 64's acceptance line, made a function.
	**
	**	Samples the terrain under the span at `sample_spacing` metres and compares each one
	**	against the deck above it.  A crossing whose definition states no clearance reports
	**	Stated == false and is not judged; a place the terrain cannot answer for is skipped
	**	rather than counted as infinitely deep.
	*/
	static bool			Check_Clearance(int bridge_id,BridgeClearanceReportStruct * report_out,
												float sample_spacing = 4.0f);

	/*
	**	Read each abutment's height out of the terrain at its bank, for every bridge.  Called
	**	from Build_Layout, and separately available because a caller that has just reshaped
	**	the ground wants the bridges to notice.  Returns how many abutments were answered.
	*/
	static int			Conform_Abutments(void);

	//	The join to Section 19's roads.
	static int			Connect_Roads(float max_distance = 8.0f);

	//	Multiplayer state.  Deterministic reconstruction means only the states travel.
	static int			Get_State_Block_Size(void);
	static bool			Get_State_Block(unsigned char * buffer,int max_bytes,int * used_out);
	static bool			Apply_State_Block(const unsigned char * buffer,int bytes);
	static bool			Is_State_Dirty(void);
	static void			Clear_State_Dirty(void);

	//	Debris that has come off and not yet been drawn.
	static int			Get_Pending_Debris_Count(void);
	static const BridgeDebrisClass *	Peek_Pending_Debris(int index);
	static void			Clear_Pending_Debris(void);

private:

	static bool			Layout_Bridge(BridgeClass & bridge,const BridgeDefinitionClass & definition);
	static void			Place_Section(BridgeClass & bridge,int kind,int ordinal,
											 const Vector3 & start,const Vector3 & end,
											 float half_width,float drop);
	static bool			Build_Section_Geometry(BridgeSectionClass & section,
														  const BridgeDefinitionClass & definition);
	static void			Destroy_Section_Geometry(BridgeSectionClass & section);
	static void			Note_Debris(const BridgeClass & bridge,int section_index,
										  const BridgeSectionClass & section,
										  const BridgeDefinitionClass & definition);

	static DynamicVectorClass<BridgeDefinitionClass>	Definitions;
	static DynamicVectorClass<BridgeClass>					Bridges;
	static DynamicVectorClass<BridgeDebrisClass>			Debris;

	static bool			LayoutBuilt;
	static bool			GeometryBuilt;
	static bool			StateDirty;
	static int			MissingModels;
};

#endif // BRIDGESYSTEM_H
