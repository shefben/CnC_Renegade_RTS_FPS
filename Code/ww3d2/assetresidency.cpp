/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando / G 3D Library                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/assetresidency.cpp                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   AssetResidencyManagerClass::Register_Asset -- claim an asset for a scope                  *
 *   AssetResidencyManagerClass::Add_Dependency -- one asset cannot be kept without another    *
 *   AssetResidencyManagerClass::Capture_Loaded_Assets -- claim whatever is loaded now         *
 *   AssetResidencyManagerClass::Build_Retained_List -- what survives releasing a scope        *
 *   AssetResidencyManagerClass::Release_Scope -- release a scope and all shorter-lived ones   *
 *   AssetResidencyManagerClass::Get_Dangling_Reference_Count -- records with nothing behind   *
 *   AssetResidencyManagerClass::Log_Report -- the whole picture, per scope and per kind       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "assetresidency.h"
#include "assetmgr.h"
#include "texture.h"
#include "wwdebug.h"

#include <string.h>


AssetResidencyManagerClass * AssetResidencyManagerClass::TheInstance = nullptr;


AssetResidencyManagerClass::AssetResidencyManagerClass(void)
{
	if (TheInstance == nullptr) {
		TheInstance = this;
	}
}


AssetResidencyManagerClass::~AssetResidencyManagerClass(void)
{
	if (TheInstance == this) {
		TheInstance = nullptr;
	}
	Reset();
}


AssetResidencyManagerClass & AssetResidencyManagerClass::Get_Instance(void)
{
	//	The engine's one instance is built on first use rather than at static init time,
	//	so nothing here depends on the order the translation units initialise in.
	static AssetResidencyManagerClass _TheResidencyManager;
	if (TheInstance == nullptr) {
		TheInstance = &_TheResidencyManager;
	}
	return *TheInstance;
}


StringClass AssetResidencyManagerClass::Normalize_Name(const char * name)
{
	StringClass normalized(name != nullptr ? name : "",true);
	if (normalized.Get_Length() > 0) {
		_strlwr(normalized.Peek_Buffer());
	}
	return normalized;
}


int AssetResidencyManagerClass::Find_Record(const StringClass & normalized) const
{
	int index = -1;
	if (NameToIndex.Get(normalized,index)) {
		return index;
	}
	return -1;
}


/***********************************************************************************************
 * AssetResidencyManagerClass::Register_Asset -- claim an asset for a scope                    *
 *                                                                                             *
 * Registering a name twice keeps the longer-lived of the two scopes.  Something the HUD wants *
 * permanently does not stop being permanent because a level also wants it.                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void AssetResidencyManagerClass::Register_Asset
(
	const char *		name,
	AssetKindType		kind,
	AssetScopeType		scope,
	unsigned				bytes
)
{
	if ((name == nullptr) || (*name == 0)) {
		return;
	}
	WWASSERT(scope >= 0 && scope < ASSET_SCOPE_COUNT);
	WWASSERT(kind >= 0 && kind < ASSET_KIND_COUNT);

	StringClass normalized = Normalize_Name(name);

	int index = Find_Record(normalized);
	if (index >= 0) {
		AssetRecordStruct & record = Assets[index];
		if (scope < record.Scope) {
			record.Scope = scope;
		}
		if (bytes > record.Bytes) {
			record.Bytes = bytes;
		}
		return;
	}

	AssetRecordStruct record;
	record.Name				= normalized;
	record.OriginalName	= name;
	record.Kind				= kind;
	record.Scope			= scope;
	record.Bytes			= bytes;

	Assets.Add(record);
	NameToIndex.Insert(normalized,Assets.Count() - 1);
}


void AssetResidencyManagerClass::Register_Texture(TextureClass * texture, AssetScopeType scope)
{
	if (texture == nullptr) {
		return;
	}
	Register_Asset(texture->Get_Full_Path(),ASSET_KIND_TEXTURE,scope,texture->Get_Texture_Memory_Usage());
}


bool AssetResidencyManagerClass::Is_Registered(const char * name) const
{
	return Find_Record(Normalize_Name(name)) >= 0;
}


AssetScopeType AssetResidencyManagerClass::Get_Asset_Scope(const char * name) const
{
	int index = Find_Record(Normalize_Name(name));
	if (index < 0) {
		return ASSET_SCOPE_COUNT;
	}
	return Assets[index].Scope;
}


/***********************************************************************************************
 * AssetResidencyManagerClass::Add_Dependency -- one asset cannot be kept without another      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void AssetResidencyManagerClass::Add_Dependency(const char * owner, const char * dependent)
{
	if ((owner == nullptr) || (dependent == nullptr)) {
		return;
	}

	DependencyStruct edge;
	edge.Owner		= Normalize_Name(owner);
	edge.Dependent	= Normalize_Name(dependent);

	if (edge.Owner == edge.Dependent) {
		return;
	}

	for (int i=0; i<Dependencies.Count(); i++) {
		if ((Dependencies[i].Owner == edge.Owner) && (Dependencies[i].Dependent == edge.Dependent)) {
			return;
		}
	}

	Dependencies.Add(edge);
}


/***********************************************************************************************
 * AssetResidencyManagerClass::Capture_Loaded_Assets -- claim whatever is loaded now           *
 *                                                                                             *
 * Everything the asset manager currently holds that nobody has claimed becomes this scope's.  *
 * Anything already claimed keeps the scope it has, so calling this after loading a level gives *
 * the level's assets to WORLD without disturbing what is permanent.                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int AssetResidencyManagerClass::Capture_Loaded_Assets(AssetScopeType scope)
{
	WW3DAssetManager * assets = WW3DAssetManager::Get_Instance();
	if (assets == nullptr) {
		return 0;
	}

	DynamicVectorClass<StringClass> loaded;
	assets->Create_Asset_List(loaded);

	int added = 0;
	for (int i=0; i<loaded.Count(); i++) {
		if (!Is_Registered(loaded[i])) {

			//	The list holds both the files prototypes came out of and the files
			//	animations came out of, and they are not the same set: an animation
			//	file name is not a render object.  Asking the manager which it is
			//	keeps the dangling-reference check honest, since that check can only
			//	verify a prototype.
			AssetKindType kind = assets->Render_Obj_Exists(loaded[i])
					? ASSET_KIND_PROTOTYPE : ASSET_KIND_ANIMATION;

			Register_Asset(loaded[i],kind,scope);
			added++;
		}
	}

	return added;
}


/***********************************************************************************************
 * AssetResidencyManagerClass::Build_Retained_List -- what survives releasing a scope          *
 *                                                                                             *
 * Everything belonging to a longer-lived scope survives, and so does everything those assets  *
 * depend on, however short-lived its own scope says it is.  The closure is iterated to a fixed *
 * point rather than recursed, so a dependency cycle terminates instead of overflowing.        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void AssetResidencyManagerClass::Build_Retained_List
(
	AssetScopeType							scope,
	DynamicVectorClass<StringClass> &	names
) const
{
	WWASSERT(scope >= 0 && scope < ASSET_SCOPE_COUNT);

	HashTemplateClass<StringClass,int> retained;

	int i=0;
	for (; i<Assets.Count(); i++) {
		if (Assets[i].Scope < scope) {
			if (!retained.Exists(Assets[i].Name)) {
				retained.Insert(Assets[i].Name,1);
				names.Add(Assets[i].OriginalName);
			}
		}
	}

	//	Follow dependencies out of the retained set until nothing new is reached.
	bool grew = true;
	while (grew) {
		grew = false;
		for (i=0; i<Dependencies.Count(); i++) {

			const DependencyStruct & edge = Dependencies[i];
			if (retained.Exists(edge.Owner) && !retained.Exists(edge.Dependent)) {

				retained.Insert(edge.Dependent,1);

				//	Report the name the way it was registered where we know it, so the
				//	exclusion list is matching against something a human can recognise.
				int index = Find_Record(edge.Dependent);
				names.Add(index >= 0 ? Assets[index].OriginalName : edge.Dependent);
				grew = true;
			}
		}
	}
}


/***********************************************************************************************
 * AssetResidencyManagerClass::Release_Scope -- release a scope and all shorter-lived ones     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void AssetResidencyManagerClass::Release_Scope(AssetScopeType scope)
{
	WWASSERT(scope >= 0 && scope < ASSET_SCOPE_COUNT);

	if (scope == ASSET_SCOPE_PERMANENT) {
		//	Releasing the permanent scope means releasing everything, which the asset
		//	manager already knows how to do and does more thoroughly than an empty
		//	exclusion list would.
		WW3DAssetManager * assets = WW3DAssetManager::Get_Instance();
		if (assets != nullptr) {
			assets->Free_Assets();
		}
		Reset();
		return;
	}

	DynamicVectorClass<StringClass> retained;
	Build_Retained_List(scope,retained);

	WW3DAssetManager * assets = WW3DAssetManager::Get_Instance();
	if (assets != nullptr) {
		if (retained.Count() == 0) {
			//	Nothing is being kept, so this is the release the engine has always
			//	done, and Free_Assets does it more thoroughly than an empty exclusion
			//	list would: it also drops the fonts and every texture reference, not
			//	just the unreferenced ones.
			assets->Free_Assets();
		} else {
			assets->Free_Assets_With_Exclusion_List(retained);
		}
	}

	//	Drop the records for everything that was just released.  The vector is rebuilt
	//	rather than deleted through, because the name index stores positions in it.
	DynamicVectorClass<AssetRecordStruct> survivors;
	int i=0;
	for (; i<Assets.Count(); i++) {
		if (Assets[i].Scope < scope) {
			survivors.Add(Assets[i]);
		}
	}

	Assets.Delete_All();
	NameToIndex.Remove_All();
	for (i=0; i<survivors.Count(); i++) {
		Assets.Add(survivors[i]);
		NameToIndex.Insert(survivors[i].Name,i);
	}

	//	And the dependency edges that no longer have an owner.
	DynamicVectorClass<DependencyStruct> live_edges;
	for (i=0; i<Dependencies.Count(); i++) {
		if (Find_Record(Dependencies[i].Owner) >= 0) {
			live_edges.Add(Dependencies[i]);
		}
	}

	Dependencies.Delete_All();
	for (i=0; i<live_edges.Count(); i++) {
		Dependencies.Add(live_edges[i]);
	}
}


void AssetResidencyManagerClass::Reset(void)
{
	Assets.Delete_All();
	Dependencies.Delete_All();
	NameToIndex.Remove_All();
}


int AssetResidencyManagerClass::Get_Scope_Asset_Count(AssetScopeType scope) const
{
	int count = 0;
	for (int i=0; i<Assets.Count(); i++) {
		if (Assets[i].Scope == scope) {
			count++;
		}
	}
	return count;
}


int AssetResidencyManagerClass::Get_Kind_Asset_Count(AssetKindType kind) const
{
	int count = 0;
	for (int i=0; i<Assets.Count(); i++) {
		if (Assets[i].Kind == kind) {
			count++;
		}
	}
	return count;
}


unsigned AssetResidencyManagerClass::Get_Scope_Memory(AssetScopeType scope) const
{
	unsigned bytes = 0;
	for (int i=0; i<Assets.Count(); i++) {
		if (Assets[i].Scope == scope) {
			bytes += Assets[i].Bytes;
		}
	}
	return bytes;
}


unsigned AssetResidencyManagerClass::Get_Total_Memory(void) const
{
	unsigned bytes = 0;
	for (int i=0; i<Assets.Count(); i++) {
		bytes += Assets[i].Bytes;
	}
	return bytes;
}


int AssetResidencyManagerClass::Get_Live_Prototype_Count(void) const
{
	WW3DAssetManager * assets = WW3DAssetManager::Get_Instance();
	if (assets == nullptr) {
		return 0;
	}
	return assets->Prototype_Count();
}


/***********************************************************************************************
 * AssetResidencyManagerClass::Get_Dangling_Reference_Count -- records with nothing behind them*
 *                                                                                             *
 * A record naming a prototype the asset manager no longer holds means something released an   *
 * asset out from under the scope that claimed it.  Only prototypes can be checked this way;    *
 * the other kinds are counted but not verified, and are not reported as dangling.             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int AssetResidencyManagerClass::Get_Dangling_Reference_Count(void) const
{
	WW3DAssetManager * assets = WW3DAssetManager::Get_Instance();
	if (assets == nullptr) {
		return 0;
	}

	int dangling = 0;
	for (int i=0; i<Assets.Count(); i++) {
		if (Assets[i].Kind == ASSET_KIND_PROTOTYPE) {
			if (!assets->Render_Obj_Exists(Assets[i].OriginalName)) {
				dangling++;
			}
		}
	}
	return dangling;
}


const char * AssetResidencyManagerClass::Scope_Name(AssetScopeType scope)
{
	switch (scope) {
		case ASSET_SCOPE_PERMANENT:	return "PERMANENT";
		case ASSET_SCOPE_GAME_MODE:	return "GAME_MODE";
		case ASSET_SCOPE_WORLD:			return "WORLD";
		case ASSET_SCOPE_SECTOR:		return "SECTOR";
		case ASSET_SCOPE_TRANSIENT:	return "TRANSIENT";
		default:								return "?";
	}
}


const char * AssetResidencyManagerClass::Kind_Name(AssetKindType kind)
{
	switch (kind) {
		case ASSET_KIND_PROTOTYPE:			return "prototype";
		case ASSET_KIND_HIERARCHY_TREE:	return "hierarchy tree";
		case ASSET_KIND_ANIMATION:			return "animation";
		case ASSET_KIND_TEXTURE:			return "texture";
		case ASSET_KIND_MATERIAL:			return "material";
		case ASSET_KIND_WORLD_BUFFER:		return "world buffer";
		default:									return "?";
	}
}


/***********************************************************************************************
 * AssetResidencyManagerClass::Log_Report -- the whole picture, per scope and per kind         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void AssetResidencyManagerClass::Log_Report(void) const
{
	WWDEBUG_SAY(("Asset residency: %d tracked assets, %d dependencies, %d live prototypes\n",
		Assets.Count(),Dependencies.Count(),Get_Live_Prototype_Count()));

	for (int scope=0; scope<ASSET_SCOPE_COUNT; scope++) {
		AssetScopeType which = static_cast<AssetScopeType>(scope);
		WWDEBUG_SAY(("  %-10s %5d assets %10u bytes\n",
			Scope_Name(which),Get_Scope_Asset_Count(which),Get_Scope_Memory(which)));
	}

	for (int kind=0; kind<ASSET_KIND_COUNT; kind++) {
		AssetKindType which = static_cast<AssetKindType>(kind);
		int count = Get_Kind_Asset_Count(which);
		if (count > 0) {
			WWDEBUG_SAY(("  %-16s %5d\n",Kind_Name(which),count));
		}
	}

	int dangling = Get_Dangling_Reference_Count();
	if (dangling > 0) {
		WWDEBUG_SAY(("  %d tracked prototypes are no longer loaded\n",dangling));
	}
}
