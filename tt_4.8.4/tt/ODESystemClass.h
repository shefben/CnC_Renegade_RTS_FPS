#ifndef TT_INCLUDE__ODESYSTEMCLASS_H
#define TT_INCLUDE__ODESYSTEMCLASS_H



class StateVectorClass;



class ODESystemClass
{

public:

   virtual void   Get_State           (StateVectorClass& target) = 0;
   virtual int    Set_State           (const StateVectorClass& states, int index) = 0;
   virtual int    Compute_Derivatives (float, StateVectorClass* states, StateVectorClass* newStates, int index) = 0;

};


#endif
