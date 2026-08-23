/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando / G 3D Library                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/assetresidency.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef ASSETRESIDENCY_H
#define ASSETRESIDENCY_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"
#include "hashtemplate.h"

class TextureClass;

/*
** How long an asset is expected to stay loaded.  Shorter-lived scopes are released first
** and always released together with everything shorter-lived than them, so the ordering of
** this enumeration is the meaning of it and must not be shuffled.
**
**	PERMANENT	loaded once and never released -- the HUD, the cursor, the fonts
**	GAME_MODE	lives as long as one mode of play does
**	WORLD			lives as long as one level does
**	SECTOR		part of a level that can be streamed in and out while play continues
**	TRANSIENT	wanted for the current moment only
*/
enum AssetScopeType
{
	ASSET_SCOPE_PERMANENT = 0,
	ASSET_SCOPE_GAME_MODE,
	ASSET_SCOPE_WORLD,
	ASSET_SCOPE_SECTOR,
	ASSET_SCOPE_TRANSIENT,

	ASSET_SCOPE_COUNT
};

/*
** What kind of thing a residency record stands for.  The kind does not change what gets
** released -- the exclusion list works on names -- it makes the diagnostics readable and
** lets a dangling-reference check know which records it can actually verify.
*/
enum AssetKindType
{
	ASSET_KIND_PROTOTYPE = 0,
	ASSET_KIND_HIERARCHY_TREE,
	ASSET_KIND_ANIMATION,
	ASSET_KIND_TEXTURE,
	ASSET_KIND_MATERIAL,
	ASSET_KIND_WORLD_BUFFER,

	ASSET_KIND_COUNT
};


/*
** AssetResidencyManagerClass
**
** The canonical answer to "may this asset be thrown away yet".
**
** Renegade's asset manager has always been able to free everything or to free nothing that
** somebody still references.  Between those two is the case a level change actually wants:
** free what belonged to the level being left, keep what the next level needs anyway, and
** keep permanently what is permanent.  This tracks which of those an asset is, and hands
** the resulting keep-list to WW3DAssetManager::Free_Assets_With_Exclusion_List.
**
** Nothing here requires an asset to be authored or exported differently.  A stock Renegade
** .w3d that nobody ever registers is simply owned by the scope that was current when it
** loaded, which is what Capture_Loaded_Assets is for.
*/
class AssetResidencyManagerClass
{
public:

	AssetResidencyManagerClass(void);
	~AssetResidencyManagerClass(void);

	/*
	** The engine keeps one of these.  It is not a singleton in the sense of forbidding a
	** second: a tool or a test may build its own and never touch the global one.
	*/
	static AssetResidencyManagerClass &	Get_Instance(void);

	/*
	** Claim an asset for a scope.  Registering a name a second time moves it to the
	** longer-lived of the two scopes: something wanted permanently and also wanted by
	** this level is wanted permanently.
	*/
	void					Register_Asset(const char * name, AssetKindType kind, AssetScopeType scope, unsigned bytes = 0);
	void					Register_Texture(TextureClass * texture, AssetScopeType scope);

	bool					Is_Registered(const char * name) const;
	AssetScopeType		Get_Asset_Scope(const char * name) const;

	/*
	** Say that `owner` cannot be kept without `dependent`.  Dependencies are followed
	** transitively when the keep-list is built, so a hierarchy that names its animations
	** keeps them, and an animation that names its skeleton keeps that.
	*/
	void					Add_Dependency(const char * owner, const char * dependent);
	int					Get_Dependency_Count(void) const { return Dependencies.Count(); }

	/*
	** Give every currently loaded w3d file that nobody has claimed to this scope.
	** Returns how many records that added.  This is what makes the service usable
	** without modifying every loader in the engine.
	*/
	int					Capture_Loaded_Assets(AssetScopeType scope);

	/*
	** The same thing for textures.  Everything the asset manager is holding a texture for
	** and nobody has claimed becomes this scope's.  Run at the end of startup this claims
	** the HUD, the cursor and the font pages without any of them having to say so, which
	** is the point: a stock installation's UI textures are simply the ones resident before
	** the first level loads.
	*/
	int					Capture_Loaded_Textures(AssetScopeType scope);

	/*
	** Build the list of names that must survive releasing `scope`: everything belonging to
	** a longer-lived scope, closed over dependencies.  Exposed because it is the whole of
	** the decision and is worth being able to check on its own.
	*/
	void					Build_Retained_List(AssetScopeType scope, DynamicVectorClass<StringClass> & names) const;

	/*
	** How many records survive releasing `scope`.  This is not the length of the retained
	** list: that list holds only the names the exclusion list can act on, and a scope may
	** be holding nothing but textures, which are retained by reference count rather than
	** by name.  Whether anything is being kept at all is a question about records.
	*/
	int					Get_Retained_Count(AssetScopeType scope) const;

	/*
	** Release `scope` and everything shorter-lived than it.
	*/
	void					Release_Scope(AssetScopeType scope);

	/*
	** Forget everything.  Does not free anything: use it after Free_Assets.
	*/
	void					Reset(void);

	/*
	** Diagnostics.
	*/
	int					Get_Asset_Count(void) const { return Assets.Count(); }
	int					Get_Scope_Asset_Count(AssetScopeType scope) const;
	int					Get_Kind_Asset_Count(AssetKindType kind) const;
	unsigned				Get_Scope_Memory(AssetScopeType scope) const;
	unsigned				Get_Total_Memory(void) const;
	int					Get_Live_Prototype_Count(void) const;
	int					Get_Dangling_Reference_Count(void) const;
	void					Log_Report(void) const;

	static const char *	Scope_Name(AssetScopeType scope);
	static const char *	Kind_Name(AssetKindType kind);

	/*
	** Names are compared without regard to case, the same way the exclusion list compares
	** them.  Public because a caller building its own keep-list has to agree with us.
	*/
	static StringClass	Normalize_Name(const char * name);

private:

	struct AssetRecordStruct
	{
		StringClass			Name;				// normalized
		StringClass			OriginalName;	// as it was registered, for reports
		AssetKindType		Kind;
		AssetScopeType		Scope;
		unsigned				Bytes;

		AssetRecordStruct(void) : Kind(ASSET_KIND_PROTOTYPE), Scope(ASSET_SCOPE_WORLD), Bytes(0) {}

		//	VectorClass compares its elements; a record is identified by its name.
		bool operator == (const AssetRecordStruct & that) const { return Name == that.Name; }
		bool operator != (const AssetRecordStruct & that) const { return !(*this == that); }
	};

	struct DependencyStruct
	{
		StringClass			Owner;			// normalized
		StringClass			Dependent;		// normalized

		bool operator == (const DependencyStruct & that) const
			{ return (Owner == that.Owner) && (Dependent == that.Dependent); }
		bool operator != (const DependencyStruct & that) const { return !(*this == that); }
	};

	int					Find_Record(const StringClass & normalized) const;

	DynamicVectorClass<AssetRecordStruct>			Assets;
	DynamicVectorClass<DependencyStruct>			Dependencies;
	HashTemplateClass<StringClass,int>				NameToIndex;

	static AssetResidencyManagerClass *				TheInstance;
};

#endif
