#ifndef TT_INCLUDE_FILTEREDSOUNDCLASS_H
#define TT_INCLUDE_FILTEREDSOUNDCLASS_H
#include "SoundPseudo3DClass.h"

class FilteredSoundClass : public SoundPseudo3DClass {
public:
	int unk164;
	virtual ~FilteredSoundClass();
	virtual FilteredSoundClass* As_FilteredSoundClass  ();
	virtual void                Update_Volume           ();
	virtual const PersistFactoryClass &Get_Factory             () const;
	virtual void                Initialize_Miles_Handle ();
	virtual unsigned long       Get_Class_ID            () const;
};

#endif