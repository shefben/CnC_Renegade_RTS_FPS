#ifndef TT_INCLUDE__TRANSITIONINSTANCECLASS_H
#define TT_INCLUDE__TRANSITIONINSTANCECLASS_H
class TransitionCompletionDataStruct;
class SoldierGameObj;
class TransitionInstanceClass
{
public:
	static void End(SoldierGameObj *,TransitionCompletionDataStruct *);
};

#endif