#ifndef TT_INCLUDE_HUMANPHYSCLAS_H
#define TT_INCLUDE_HUMANPHYSCLAS_H
#include "Phys3Class.h"
class HumanPhysClass : public Phys3Class
{
private:
	bool b1;
	bool b2;
public:
	virtual ~HumanPhysClass();
	virtual void            Timestep                     (float);
	virtual void                       Render                       (RenderInfoClass&);
	virtual HumanPhysClass*            As_HumanPhysClass            () {return this;}
	virtual bool                       Save                         (ChunkSaveClass& oSave);
	virtual bool                       Load                         (ChunkLoadClass& oLoad);
	virtual bool                       Is_Simulation_Disabled       ();
	virtual bool                       Is_Rendering_Disabled        ();
	const PersistFactoryClass &Get_Factory() const;
	virtual void            Check_Ground                 (const AABoxClass&, GroundStateStruct*, float);
	virtual bool            Ballistic_Move               (float);
	virtual bool            Slide_Move                   (const GroundStateStruct&, float);
	virtual bool            Normal_Move                  (const GroundStateStruct&, float);
};

#endif
