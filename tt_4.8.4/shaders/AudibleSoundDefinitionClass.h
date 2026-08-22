#ifndef TT_INCLUDE__AUDIBLESOUNDDEFINITIONCLASS_H
#define TT_INCLUDE__AUDIBLESOUNDDEFINITIONCLASS_H



#include "definition.h"



class LogicalSoundClass;
class AudibleSoundClass;



class AudibleSoundDefinitionClass :
	public DefinitionClass
{

	UNK unk001C; // 001C
	UNK unk0020; // 0020
	UNK unk0024; // 0024
	UNK unk0028; // 0028
	UNK unk002C; // 002C
	UNK unk0030; // 0030
	UNK unk0034; // 0034
	UNK unk0038; // 0038
	UNK unk003C; // 003C
	UNK unk0040; // 0040
	UNK unk0044; // 0044
	StringClass displayText; // 0048
	// ...

public:

	virtual ~AudibleSoundDefinitionClass();
	virtual const PersistFactoryClass& Get_Factory() const;
	virtual bool Save(ChunkSaveClass&);
	virtual bool Load(ChunkLoadClass&);
	virtual uint32 Get_Class_ID() const;
	virtual PersistClass* Create() const;
	virtual UNK Create_Sound(int);
	virtual UNK Initialize_From_Sound(AudibleSoundClass*);
	virtual UNK Get_Filename();
	virtual const StringClass& Get_Display_Text() { return displayText; }
	virtual UNK Get_Max_Vol_Radius();
	virtual UNK Get_DropOff_Radius();
	virtual UNK Get_Sphere_Color();
	virtual UNK Get_Volume();
	virtual UNK Get_Volume_Randomizer();
	virtual UNK Get_Start_Offset();
	virtual UNK Get_Pitch_Factor();
	virtual UNK Get_Pitch_Factor_Randomizer();
	virtual UNK Get_Virtual_Channel();
	virtual UNK Set_Volume(float);
	virtual UNK Set_Volume_Randomizer(float);
	virtual UNK Set_Max_Vol_Radius(float);
	virtual UNK Set_DropOff_Radius(float);
	virtual UNK Set_Start_Offset(float);
	virtual UNK Set_Pitch_Factor(float);
	virtual UNK Set_Pitch_Factor_Randomizer(float);
	virtual UNK Set_Virtual_Channel(int);
	virtual LogicalSoundClass* Create_Logical();

	AudibleSoundDefinitionClass();
	bool Save_Variables(ChunkSaveClass&);
	bool Load_Variables(ChunkLoadClass&);

};



#endif