#ifndef TT_INCLUDE__PARAMETERIMPL_H
#define TT_INCLUDE__PARAMETERIMPL_H
#include "OBBoxClass.h"
#include "Vector2.h"
#include "Rect.h"
const int	DEF_CLASSID_START			= 0x00001000;
const int	DEF_CLASSID_RANGE			= 0x00001000;
#define		NEXT_SUPER_CLASSID(n)	DEF_CLASSID_START + (n * DEF_CLASSID_RANGE)
enum
{
	CLASSID_TERRAIN			= NEXT_SUPER_CLASSID(0),
	CLASSID_TILE				= NEXT_SUPER_CLASSID(1),
	CLASSID_GAME_OBJECTS		= NEXT_SUPER_CLASSID(2),
	CLASSID_LIGHT				= NEXT_SUPER_CLASSID(3),
	CLASSID_SOUND				= NEXT_SUPER_CLASSID(4),
	CLASSID_WAYPATH			= NEXT_SUPER_CLASSID(5),
	CLASSID_ZONE				= NEXT_SUPER_CLASSID(6),
	CLASSID_TRANSITION		= NEXT_SUPER_CLASSID(7),
	CLASSID_PHYSICS			= NEXT_SUPER_CLASSID(8),
	CLASSID_EDITOR_OBJECTS	= NEXT_SUPER_CLASSID(9),
	CLASSID_MUNITIONS			= NEXT_SUPER_CLASSID(10),
	CLASSID_DUMMY_OBJECTS	= NEXT_SUPER_CLASSID(11),
	CLASSID_BUILDINGS			= NEXT_SUPER_CLASSID(12),
	CLASSID_TWIDDLERS			= NEXT_SUPER_CLASSID(13),
	CLASSID_GLOBAL_SETTINGS	= NEXT_SUPER_CLASSID(14),
};
class StringParameterClass : public ParameterClass
{
public:	
	StringParameterClass (StringClass *string);
	StringParameterClass (const StringParameterClass &src);
	virtual ~StringParameterClass (void) {}
	const StringParameterClass &	operator= (const StringParameterClass &src);
	bool									operator== (const StringParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_STRING; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_STRING) || ParameterClass::Is_Type (type); }
	virtual const char *		Get_String (void) const;
	virtual void				Set_String (const char *string);
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	StringClass *	m_String;
};
class FilenameParameterClass : public StringParameterClass
{
public:	
	FilenameParameterClass (StringClass *string);
	FilenameParameterClass (const FilenameParameterClass &src);
	virtual ~FilenameParameterClass (void) {}
	const FilenameParameterClass &	operator= (const FilenameParameterClass &src);
	bool										operator== (const FilenameParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type			Get_Type (void) const { return TYPE_FILENAME; }
	virtual bool			Is_Type (Type type) const { return (type == TYPE_FILENAME) || StringParameterClass::Is_Type (type); }
	virtual void			Copy_Value (const ParameterClass &src);
	virtual void			Set_Extension (const char *extension)	{ m_Extension = extension; }
	virtual const char *	Get_Extension (void) const					{ return m_Extension; }
	virtual void			Set_Description (const char *desc)		{ m_Description = desc; }
	virtual const char *	Get_Description (void) const				{ return m_Description; }
protected:
	StringClass				m_Extension;
	StringClass				m_Description;
};
class SoundFilenameParameterClass : public FilenameParameterClass
{
public:	
	SoundFilenameParameterClass (StringClass *string);
	SoundFilenameParameterClass (const SoundFilenameParameterClass &src);
	virtual ~SoundFilenameParameterClass (void) {}
	const SoundFilenameParameterClass &	operator= (const SoundFilenameParameterClass &src);
	bool											operator== (const SoundFilenameParameterClass &src);
	virtual Type			Get_Type (void) const { return TYPE_SOUND_FILENAME; }
	virtual bool			Is_Type (Type type) const { return (type == TYPE_SOUND_FILENAME) || FilenameParameterClass::Is_Type (type); }
};
class EnumParameterClass : public ParameterClass
{
public:	
	EnumParameterClass (int *value);
	EnumParameterClass (const EnumParameterClass &src);
	virtual ~EnumParameterClass (void) {}
	const EnumParameterClass &	operator= (const EnumParameterClass &src);
	bool								operator== (const EnumParameterClass &src);
	bool								operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_ENUM; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_ENUM) || ParameterClass::Is_Type (type); }
	virtual void __cdecl		Add_Values (const char *first_name, int first_value, ...);
	virtual void				Add_Value (const char *display_name, int value);
	virtual int					Get_Count (void) const					{ return m_List.Count (); }
	virtual const char *		Get_Entry_Name (int index) const		{ return m_List[index].name; }
	virtual int					Get_Entry_Value (int index) const	{ return m_List[index].value; }
	
	virtual void				Set_Selected_Value (int value)	{ (*m_Value) = value; Set_Modified (); }
	virtual int					Get_Selected_Value (void) const	{ return (*m_Value); }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	struct ENUM_VALUE
	{
		StringClass		name;
		int				value;
		ENUM_VALUE (const char *_name=NULL, int _value=0) : name (_name), value (_value) {}
		bool operator== (const ENUM_VALUE &) { return false; }
		bool operator!= (const ENUM_VALUE &) { return true; }
	};
	DynamicVectorClass<ENUM_VALUE>		m_List;
	int *											m_Value;
};
class PhysDefParameterClass : public ParameterClass
{
public:	
	PhysDefParameterClass (int *id);
	PhysDefParameterClass (const PhysDefParameterClass &src);
	virtual ~PhysDefParameterClass (void) {}
	const PhysDefParameterClass &	operator= (const PhysDefParameterClass &src);
	bool									operator== (const PhysDefParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_PHYSDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_PHYSDEFINITIONID) || ParameterClass::Is_Type (type); }
	virtual void				Set_Value (int id)						{ (*m_Value) = id; Set_Modified (); }
	virtual int					Get_Value (void) const					{ return (*m_Value); }
	virtual void				Set_Base_Class (const char *name)	{ m_BaseClass = name; }
	virtual const char *		Get_Base_Class (void) const			{ return m_BaseClass; }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	int *							m_Value;
	StringClass					m_BaseClass;
};
class ModelDefParameterClass : public ParameterClass
{
public:	
	ModelDefParameterClass (int *id);
	ModelDefParameterClass (const ModelDefParameterClass &src);
	virtual ~ModelDefParameterClass (void) {}
	const ModelDefParameterClass &	operator= (const ModelDefParameterClass &src);
	bool									operator== (const ModelDefParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_MODELDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_MODELDEFINITIONID) || ParameterClass::Is_Type (type); }
	virtual void				Set_Value (int id)						{ (*m_Value) = id; Set_Modified (); }
	virtual int					Get_Value (void) const					{ return (*m_Value); }
	virtual void				Set_Base_Class (const char *name)	{ m_BaseClass = name; }
	virtual const char *		Get_Base_Class (void) const			{ return m_BaseClass; }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	int *							m_Value;
	StringClass					m_BaseClass;
};
class DefParameterClass : public ParameterClass
{
public:	
	DefParameterClass (int *id);
	DefParameterClass (const DefParameterClass &src);
	virtual ~DefParameterClass (void) {}
	const DefParameterClass &	operator= (const DefParameterClass &src);
	bool								operator== (const DefParameterClass &src);
	bool								operator== (const ParameterClass &src);
	virtual DefParameterClass *	As_DefParameterClass (void)	{ return this; }
	virtual void				Set_Value (int id)						{ (*m_Value) = id; Set_Modified (); }
	virtual int					Get_Value (void) const					{ return (*m_Value); }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	int *							m_Value;
};
class GenericDefParameterClass : public DefParameterClass
{
public:	
	GenericDefParameterClass (int *id);
	GenericDefParameterClass (const GenericDefParameterClass &src);
	virtual ~GenericDefParameterClass (void) {}
	const GenericDefParameterClass &	operator= (const GenericDefParameterClass &src);
	bool										operator== (const GenericDefParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_GENERICDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_GENERICDEFINITIONID) || ParameterClass::Is_Type (type); }
	virtual void				Set_Class_ID (int class_id)			{ m_ClassID = class_id; Set_Modified (); }
	virtual int					Get_Class_ID (void) const				{ return m_ClassID; }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	int							m_ClassID;
};
class GameObjDefParameterClass : public DefParameterClass
{
public:	
	GameObjDefParameterClass (int *id);
	GameObjDefParameterClass (const GameObjDefParameterClass &src);
	virtual ~GameObjDefParameterClass (void) {}
	const GameObjDefParameterClass &	operator= (const GameObjDefParameterClass &src);
	bool										operator== (const GameObjDefParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_GAMEOBJDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_GAMEOBJDEFINITIONID) || ParameterClass::Is_Type (type); }
	virtual void				Set_Base_Class (const char *name)	{ m_BaseClass = name; Set_Modified (); }
	virtual const char *		Get_Base_Class (void) const			{ return m_BaseClass; }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	StringClass					m_BaseClass;
};
class AmmoObjDefParameterClass : public GameObjDefParameterClass
{
public:	
	AmmoObjDefParameterClass (int *id);
	AmmoObjDefParameterClass (const AmmoObjDefParameterClass &src);
	virtual ~AmmoObjDefParameterClass (void) {}
	const AmmoObjDefParameterClass &	operator= (const AmmoObjDefParameterClass &src);
	bool										operator== (const AmmoObjDefParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_AMMOOBJDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_AMMOOBJDEFINITIONID) || GameObjDefParameterClass::Is_Type (type); }
	virtual void				Copy_Value (const ParameterClass &src);
};
class WeaponObjDefParameterClass : public GameObjDefParameterClass
{
public:	
	WeaponObjDefParameterClass (int *id);
	WeaponObjDefParameterClass (const WeaponObjDefParameterClass &src);
	virtual ~WeaponObjDefParameterClass (void) {}
	const WeaponObjDefParameterClass &	operator= (const WeaponObjDefParameterClass &src);
	bool										operator== (const WeaponObjDefParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_WEAPONOBJDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_WEAPONOBJDEFINITIONID) || GameObjDefParameterClass::Is_Type (type); }
	virtual void				Copy_Value (const ParameterClass &src);
};
class ExplosionObjDefParameterClass : public GameObjDefParameterClass
{
public:	
	ExplosionObjDefParameterClass (int *id);
	ExplosionObjDefParameterClass (const ExplosionObjDefParameterClass &src);
	virtual ~ExplosionObjDefParameterClass (void) {}
	const ExplosionObjDefParameterClass &	operator= (const ExplosionObjDefParameterClass &src);
	bool										operator== (const ExplosionObjDefParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_EXPLOSIONDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_EXPLOSIONDEFINITIONID) || GameObjDefParameterClass::Is_Type (type); }
	virtual void				Copy_Value (const ParameterClass &src);
};
class SoundDefParameterClass : public DefParameterClass
{
public:	
	SoundDefParameterClass (int *id);
	SoundDefParameterClass (const SoundDefParameterClass &src);
	virtual ~SoundDefParameterClass (void) {}
	const SoundDefParameterClass &	operator= (const SoundDefParameterClass &src);
	bool										operator== (const SoundDefParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const { return TYPE_SOUNDDEFINITIONID; }
	virtual bool				Is_Type (Type type) const { return (type == TYPE_SOUNDDEFINITIONID) || ParameterClass::Is_Type (type); }
};
class ScriptParameterClass : public ParameterClass
{
public:	
	ScriptParameterClass (StringClass *name, StringClass *params);
	ScriptParameterClass (const ScriptParameterClass &src);
	virtual ~ScriptParameterClass (void) {}
	const ScriptParameterClass &	operator= (const ScriptParameterClass &src);
	bool									operator== (const ScriptParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const					{ return TYPE_SCRIPT; }
	virtual bool				Is_Type (Type type) const				{ return (type == TYPE_SCRIPT) || ParameterClass::Is_Type (type); }
	virtual void				Set_Script_Name (const char *name)	{ (*m_ScriptName) = name; Set_Modified (); }
	virtual const char *		Get_Script_Name (void) const			{ return (*m_ScriptName); }
	virtual void				Set_Params (const char *params)		{ (*m_ScriptParams) = params; Set_Modified (); }
	virtual const char *		Get_Params (void) const					{ return (*m_ScriptParams); }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	StringClass *				m_ScriptName;
	StringClass *				m_ScriptParams;
};
class DefIDListParameterClass : public ParameterClass
{
public:	
	DefIDListParameterClass (DynamicVectorClass<int> *list);
	DefIDListParameterClass (const DefIDListParameterClass &src);
	virtual ~DefIDListParameterClass (void) {}
	const DefIDListParameterClass &	operator= (const DefIDListParameterClass &src);
	bool									operator== (const DefIDListParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const				{ return TYPE_DEFINITIONIDLIST; }
	virtual bool				Is_Type (Type type) const			{ return (type == TYPE_DEFINITIONIDLIST) || ParameterClass::Is_Type (type); }
	virtual void				Set_Selected_Class_ID (uint32 *id)	{ m_SelectedClassID = id; }
	virtual uint32 *			Get_Selected_Class_ID (void) const	{ return m_SelectedClassID; }
	virtual void				Set_Class_ID (uint32 id)				{ m_ClassID = id; }
	virtual uint32 			Get_Class_ID (void) const				{ return m_ClassID; }
	virtual DynamicVectorClass<int> &Get_List (void) const	{ return (*m_IDList); }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	DynamicVectorClass<int> *	m_IDList;
	uint32							m_ClassID;
	uint32 *							m_SelectedClassID;
};
class ZoneParameterClass : public ParameterClass
{
public:	
	ZoneParameterClass (OBBoxClass *box);
	ZoneParameterClass (const ZoneParameterClass &src);
	virtual ~ZoneParameterClass (void) {}
	const ZoneParameterClass &		operator= (const ZoneParameterClass &src);
	bool									operator== (const ZoneParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const				{ return TYPE_ZONE; }
	virtual bool				Is_Type (Type type) const			{ return (type == TYPE_ZONE) || ParameterClass::Is_Type (type); }
	virtual void					Set_Zone (const OBBoxClass &box)	{ (*m_OBBox) = box; Set_Modified (); }
	virtual const OBBoxClass &	Get_Zone (void) const				{ return (*m_OBBox); }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	OBBoxClass		*m_OBBox;
};
class FilenameListParameterClass : public ParameterClass
{
public:	
	FilenameListParameterClass (DynamicVectorClass<StringClass> *list);
	FilenameListParameterClass (const FilenameListParameterClass &src);
	virtual ~FilenameListParameterClass (void) {}
	const FilenameListParameterClass &	operator= (const FilenameListParameterClass &src);
	bool									operator== (const FilenameListParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const				{ return TYPE_FILENAMELIST; }
	virtual bool				Is_Type (Type type) const			{ return (type == TYPE_FILENAMELIST) || ParameterClass::Is_Type (type); }
	virtual DynamicVectorClass<StringClass> &Get_List (void) const	{ return (*m_FilenameList); }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	DynamicVectorClass<StringClass> *	m_FilenameList;
};
class ScriptListParameterClass : public ParameterClass
{
public:	
	ScriptListParameterClass (DynamicVectorClass<StringClass> *name_list, DynamicVectorClass<StringClass> *param_list);
	ScriptListParameterClass (const ScriptListParameterClass &src);
	virtual ~ScriptListParameterClass (void) {}
	const ScriptListParameterClass &	operator= (const ScriptListParameterClass &src);
	bool										operator== (const ScriptListParameterClass &src);
	bool										operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const				{ return TYPE_SCRIPTLIST; }
	virtual bool				Is_Type (Type type) const			{ return (type == TYPE_SCRIPTLIST) || ParameterClass::Is_Type (type); }
	virtual DynamicVectorClass<StringClass> &Get_Name_List (void) const	{ return (*m_NameList); }
	virtual DynamicVectorClass<StringClass> &Get_Param_List (void) const	{ return (*m_ParamList); }
	virtual void				Copy_Value (const ParameterClass &src);
protected:
	bool							Are_Lists_Identical (DynamicVectorClass<StringClass> &list1, DynamicVectorClass<StringClass> &list2);
	DynamicVectorClass<StringClass> *	m_NameList;
	DynamicVectorClass<StringClass> *	m_ParamList;
};
class SeparatorParameterClass : public ParameterClass
{
public:	
	SeparatorParameterClass (void) {}
	SeparatorParameterClass (const SeparatorParameterClass &src);
	virtual ~SeparatorParameterClass (void) {}
	const SeparatorParameterClass &	operator= (const SeparatorParameterClass &src);
	bool									operator== (const SeparatorParameterClass &src);
	bool									operator== (const ParameterClass &src);
	virtual Type				Get_Type (void) const				{ return TYPE_SEPARATOR; }
	virtual bool				Is_Type (Type type) const			{ return (type == TYPE_SEPARATOR) || ParameterClass::Is_Type (type); }
	virtual void				Copy_Value (const ParameterClass &src);
};
template <class T, ParameterClass::Type type>
class SimpleParameterClass : public ParameterClass
{
public:
	SimpleParameterClass (void *data, const char *name);
	bool				operator== (const ParameterClass &src);
	const T &		Get_Value (void) const;
	void				Set_Value (const T &new_value);
	ParameterClass::Type	Get_Type (void) const;
	void						Copy_Value (const ParameterClass &src);
private:
	T *			m_Data;
	T				m_Min;
	T				m_Max;
};
template <class T, ParameterClass::Type type> inline
SimpleParameterClass<T, type>::SimpleParameterClass (void *data, const char *name)
{
	Set_Name (name);
	m_Data = (T *)data;
	return ;
}
template <class T, ParameterClass::Type type> inline bool
SimpleParameterClass<T, type>::operator== (const ParameterClass &src)
{
	bool retval = false;
	if (src.Get_Type () == Get_Type ()) {
		retval = ((*m_Data) == *(((const SimpleParameterClass &)src).m_Data));
	}
	return retval;
}
template <class T, ParameterClass::Type type> inline const T &
SimpleParameterClass<T, type>::Get_Value (void) const
{
	return (*m_Data);
}
template <class T, ParameterClass::Type type> inline  void
SimpleParameterClass<T, type>::Set_Value (const T &new_value)
{
	(*m_Data) = new_value;
	Set_Modified ();
	return ;
}
template <class T, ParameterClass::Type type> inline ParameterClass::Type
SimpleParameterClass<T, type>::Get_Type (void) const
{	
	return type;
}
template <class T, ParameterClass::Type type> inline void
SimpleParameterClass<T, type>::Copy_Value (const ParameterClass &src)
{
	if (Get_Type () == src.Get_Type ()) {
		(*m_Data) = ((SimpleParameterClass<T, type> &)src).Get_Value ();
	}
	ParameterClass::Copy_Value (src);
	return ;
}
typedef SimpleParameterClass<bool,		ParameterClass::TYPE_BOOL>			BoolParameterClass;
typedef SimpleParameterClass<Vector2,	ParameterClass::TYPE_VECTOR2>		Vector2ParameterClass;
typedef SimpleParameterClass<Vector3,	ParameterClass::TYPE_VECTOR3>		Vector3ParameterClass;
typedef SimpleParameterClass<Matrix3D, ParameterClass::TYPE_MATRIX3D>	Matrix3DParameterClass;
typedef SimpleParameterClass<RectClass,ParameterClass::TYPE_RECT>			RectParameterClass;
typedef SimpleParameterClass<Vector3,	ParameterClass::TYPE_COLOR	>		ColorParameterClass;
typedef SimpleParameterClass<int,		ParameterClass::TYPE_STRINGSDB_ID>	StringsDBEntryParameterClass;
template <class T, ParameterClass::Type type>
class RangedParameterClass : public SimpleParameterClass<T, type>
{
public:
	RangedParameterClass (void *data, const char *name)
		:	SimpleParameterClass<T, type> (data, name) { }
	void				Set_Range (const T &min, const T &max)	{ m_Min = min; m_Max = max; }
	const T &		Get_Min (void) const							{ return m_Min; }
	const T &		Get_Max (void) const							{ return m_Max; }
private:
	T				m_Min;
	T				m_Max;
};
class IntParameterClass : public RangedParameterClass<int, ParameterClass::TYPE_INT>
{
public:
	IntParameterClass (void *data, const char *name)
		:	RangedParameterClass<int, ParameterClass::TYPE_INT> (data, name)
			{ Set_Range (-1000000000L, 1000000000L); }
};
class FloatParameterClass  : public RangedParameterClass<float, ParameterClass::TYPE_FLOAT>
{
public:
	FloatParameterClass  (void *data, const char *name)
		:	RangedParameterClass<float, ParameterClass::TYPE_FLOAT> (data, name)
			{ Set_Range (-100000.0F, 100000.0F); }
};
class AngleParameterClass   : public RangedParameterClass<float, ParameterClass::TYPE_ANGLE>
{
public:
	AngleParameterClass   (void *data, const char *name)
		:	RangedParameterClass<float, ParameterClass::TYPE_ANGLE> (data, name)
			{ Set_Range (0.0F,  6.283185307F); }
};
#ifdef	PARAM_EDITING_ON
	#define EDITABLE_PARAM(_class, type, data)						plist_##_class.Add (&(data), #data, type);
	#define NAMED_EDITABLE_PARAM(_class, type, data, name)		plist_##_class.Add (&(data), name, type);
	
	#define INT_EDITABLE_PARAM(_class, data, min, max) {						\
		IntParameterClass *param = new IntParameterClass( &data, #data);	\
		param->Set_Range (min, max);													\
		plist_##_class.Add (param); }													\

	#define INT_UNITS_PARAM(_class, data, min, max, unitsname) {	   	\
		IntParameterClass *param = new IntParameterClass( &data, #data);	\
		param->Set_Range (min, max);													\
		param->Set_Units_Name(unitsname);											\
		plist_##_class.Add (param); }													\

	#define NAMED_INT_UNITS_PARAM(_class,data,min,max,unitsname,name) {	\
		IntParameterClass *param = new IntParameterClass( &data, #data);	\
		param->Set_Range (min, max);													\
		param->Set_Units_Name(unitsname);											\
		param->Set_Name(name);															\
		plist_##_class.Add (param); }													\

	#define FLOAT_EDITABLE_PARAM(_class, data, min, max) {						\
		FloatParameterClass *param = new FloatParameterClass( &data, #data);	\
		param->Set_Range (min, max);														\
		plist_##_class.Add (param); }														\

	#define FLOAT_UNITS_PARAM(_class, data, min, max, unitsname) {				\
		FloatParameterClass *param = new FloatParameterClass( &data, #data);	\
		param->Set_Range (min, max);														\
		param->Set_Units_Name(unitsname);												\
		plist_##_class.Add (param); }	

	#define NAMED_FLOAT_UNITS_PARAM(_class, data, min, max, unitsname,name) {	\
		FloatParameterClass *param = new FloatParameterClass( &data, #data);		\
		param->Set_Range (min, max);															\
		param->Set_Units_Name(unitsname);													\
		param->Set_Name(name);																	\
		plist_##_class.Add (param); }	

	#define ANGLE_EDITABLE_PARAM(_class, data, min, max) {						\
		AngleParameterClass *param = new AngleParameterClass( &data, #data);	\
		param->Set_Range (min, max);														\
		param->Set_Units_Name ("degrees");												\
		plist_##_class.Add (param); }														\

	#define NAMED_ANGLE_EDITABLE_PARAM(_class, data, min, max, name) {		\
		AngleParameterClass *param = new AngleParameterClass( &data, #data);	\
		param->Set_Range (min, max);														\
		param->Set_Units_Name ("degrees");												\
		param->Set_Name(name);																\
		plist_##_class.Add (param); }														\

	#define GENERIC_EDITABLE_PARAM(_class, param)	\
		plist_##_class.Add (param);						\

	#define MODEL_DEF_PARAM(_class, data, name) {									\
		ModelDefParameterClass *param = new ModelDefParameterClass (&data);	\
		param->Set_Name (#data);															\
		param->Set_Base_Class (name);														\
		GENERIC_EDITABLE_PARAM(_class, param); }

	#define PHYS_DEF_PARAM(_class, data, name) {										\
		PhysDefParameterClass *param = new PhysDefParameterClass (&data);		\
		param->Set_Name (#data);															\
		param->Set_Base_Class (name);														\
		GENERIC_EDITABLE_PARAM(_class, param); }

	#define SCRIPT_PARAM(_class, name, params) {													\
		ScriptParameterClass *param = new ScriptParameterClass (&name, &params);		\
		param->Set_Name (#name);																		\
		GENERIC_EDITABLE_PARAM(_class, param); }

	#define SCRIPTLIST_PARAM(_class, name, name_list, param_list) {										\
		ScriptListParameterClass *param = new ScriptListParameterClass (&name_list, &param_list);	\
		param->Set_Name (name);																							\
		GENERIC_EDITABLE_PARAM(_class, param); }

	#define ENUM_PARAM(_class, data, params) {							\
		EnumParameterClass *param = new EnumParameterClass (&data);	\
		param->Set_Name (#data);												\
		param->Add_Values params;												\
		plist_##_class.Add (param); }											\

	#define FILENAME_PARAM(_class, data, desc, extension) {						\
		FilenameParameterClass *param = new FilenameParameterClass (&data);	\
		param->Set_Name (#data);															\
		param->Set_Description (desc);													\
		param->Set_Extension (extension);												\
		plist_##_class.Add (param); }														\

	#define DEFIDLIST_PARAM(_class, data, root_class_id) {							\
		DefIDListParameterClass *param = new DefIDListParameterClass (&data);	\
		param->Set_Name (#data);																\
		param->Set_Class_ID (root_class_id);												\
		plist_##_class.Add (param); }															\

	#define CLASSID_DEFIDLIST_PARAM(_class, data, root_class_id, class_id, name) {	\
		DefIDListParameterClass *param = new DefIDListParameterClass (&data);	\
		param->Set_Name (name);																	\
		param->Set_Class_ID (root_class_id);												\
		param->Set_Selected_Class_ID (&class_id);											\
		plist_##_class.Add (param); }

	#define ZONE_PARAM(_class, data, name) {											\
		ZoneParameterClass *param = new ZoneParameterClass (&data);				\
		param->Set_Name (name);																\
		GENERIC_EDITABLE_PARAM(_class, param); }

	#define PARAM_SEPARATOR(_class, name) {											\
		SeparatorParameterClass *param = new SeparatorParameterClass;			\
		param->Set_Name (name);																\
		GENERIC_EDITABLE_PARAM(_class, param); }

	#define GENERIC_DEFID_PARAM(_class, data, root_class_id) {						\
		GenericDefParameterClass *param = new GenericDefParameterClass (&data);	\
		param->Set_Class_ID (root_class_id);												\
		param->Set_Name (#data);																\
		plist_##_class.Add (param); }
#else
	#define EDITABLE_PARAM(_class, type, data)
	#define NAMED_EDITABLE_PARAM(_class, type, data, name)
	#define INT_EDITABLE_PARAM(_class, data, min, max) 						
	#define INT_UNITS_PARAM(_class, data, min, max, unitsname) 	   	
	#define NAMED_INT_UNITS_PARAM(_class,data,min,max,unitsname,name) 	
	#define FLOAT_EDITABLE_PARAM(_class, data, min, max) 						
	#define FLOAT_UNITS_PARAM(_class, data, min, max, unitsname) 				
	#define NAMED_FLOAT_UNITS_PARAM(_class, data, min, max, unitsname,name) 	
	#define ANGLE_EDITABLE_PARAM(_class, data, min, max) 						
	#define NAMED_ANGLE_EDITABLE_PARAM(_class, data, min, max, name) 		
	#define GENERIC_EDITABLE_PARAM(_class, param)	
	#define MODEL_DEF_PARAM(_class, data, name) 									
	#define PHYS_DEF_PARAM(_class, data, name) 										
	#define SCRIPT_PARAM(_class, name, params)
	#define SCRIPTLIST_PARAM(_class, name, name_list, param_list)
	#define ENUM_PARAM(_class, data, params) 							
	#define FILENAME_PARAM(_class, data, desc, extension) 						
	#define DEFIDLIST_PARAM(_class, data, root_class_id) 							
	#define CLASSID_DEFIDLIST_PARAM(_class, data, root_class_id, class_id, name) 	
	#define ZONE_PARAM(_class, data, name) 											
	#define PARAM_SEPARATOR(_class, name) 											
	#define GENERIC_DEFID_PARAM(_class, data, root_class_id) 						
#endif
#endif
