#ifndef TT_INCLUDE_PLAYERTERMINALCLASS_H
#define TT_INCLUDE_PLAYERTERMINALCLASS_H
class SoldierGameObj;
class PlayerTerminalClass
{
public:
   enum TYPE
   {
	   TYPE_NONE = -1,
   };
   static REF_DECL2(_TheInstance,PlayerTerminalClass*);
   static PlayerTerminalClass *Get_Instance() {return _TheInstance;}
   virtual ~PlayerTerminalClass();
   virtual void Display_Terminal(SoldierGameObj* soldier, TYPE type);
   virtual void Display_Default_Terminal_For_Player(SoldierGameObj* soldier);
};
#endif
