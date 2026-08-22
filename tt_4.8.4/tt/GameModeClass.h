#ifndef TT_INCLUDE__GAMEMODECLASS_H
#define TT_INCLUDE__GAMEMODECLASS_H


class GameModeClass
{

private:

                 // *  RH7
   uint32 state; // 4    0


public:

            GameModeClass();
   virtual ~GameModeClass();

   virtual void        Activate          ();
   virtual void        Deactivate        ();
   virtual void        Safely_Deactivate ();
   virtual void        Suspend           ();
   virtual void        Resume            ();
   virtual bool        Is_Inactive       ();
   virtual bool        Is_Suspended      ();
   virtual bool        Is_Active         ();
   virtual const char* Name              () const = 0;
   virtual void        Init              () = 0;
   virtual void        Shutdown          () = 0;
   virtual void        Render            () = 0;
   virtual void        Think             () = 0;

   uint32 getState() const
   {
	   return state;
   }

}; // 8


#endif
