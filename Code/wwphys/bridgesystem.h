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

	int				StartRoadID;
	int				EndRoadID;

	DynamicVectorClass<BridgeSectionClass>	Sections;
};


/*
**	One thing that came off a bridge when it changed state, waiting for somebody to draw it.
**
**	Section 20 lists debris emission among the things a damage state may change, and there is
**	no effect system to hand it to yet.  Rather than drop it, the system records what came off
**	and where; the consumer arrives with Section 35's decal and effect work and reads this.
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
