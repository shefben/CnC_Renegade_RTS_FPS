#ifndef TT_INCLUDE__INGAMESENDMESSAGEDIALOG_H
#define TT_INCLUDE__INGAMESENDMESSAGEDIALOG_H



#include "dialogbase.h"
#include "engine_ttdef.h"



class IngameSendMessageDialog :
	public DialogBaseClass
{

private:

	uint32 unk0074;
	TextMessageEnum unk0078;
	WideStringClass sendTargetName;
	WideStringClass autoCompletingPlayerName;
	bool canAutoComplete;

	bool sub_494980(const wchar_t* string);
	void sendMessage();

public:

	IngameSendMessageDialog();
	bool getCompletedName(const wchar_t* nameStart, WideStringClass& fullName) const;
	bool autoComplete(EditCtrlClass* editControl, bool correctable);
	void setTarget(const wchar_t* _sendTargetName);
	virtual void On_EditCtrl_Change(EditCtrlClass* editControl, int);
	virtual bool On_EditCtrl_Key_Down(EditCtrlClass* editControl, uint32 character, uint32 keyData);

};



#endif
