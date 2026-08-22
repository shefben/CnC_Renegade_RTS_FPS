#include "General.h"
#include "IngameSendMessageDialog.h"



#include "engine_vector.h"
#include "string_ids.h"
#include "cPlayer.h"
#include "TranslateDBClass.h"
#include "SList.h"
#include "EditCtrl.h"
#include "resource.h"
#include "cPlayerManager.h"
#include "cNetwork.h"
#include "cCsTextObj.h"
#include "cScTextObj.h"



RENEGADE_FUNCTION
bool IngameSendMessageDialog::sub_494980(const wchar_t* string)
AT1(0x00494980);



void IngameSendMessageDialog::sendMessage()
{
	WideStringClass message = Get_Dlg_Item_Text(IDC_MESSAGE_EDIT);

	message.trim();
	if (!message.Is_Empty())
	{
		if (!sub_494980(message))
		{
			TextMessageEnum messageType = unk0078;
			int receiverId = -1;
			
			if (!sendTargetName.Is_Empty())
			{
				messageType = PrivateMessage;

				cPlayer* player = cPlayerManager::Find_Player(sendTargetName);
				if (player)
					receiverId = player->PlayerId;
				else if (_wcsicmp(sendTargetName, TRANSLATE(IDS_MP_HOST)) != 0)
					return;
			}
			
			if (cNetwork::I_Am_Client())
				(new cCsTextObj())->Init(message, messageType, cNetwork::Get_My_Id(), receiverId);
			else
				(new cScTextObj())->Init(message, messageType, false, -1, receiverId);
		}
		Set_Dlg_Item_Text(IDC_MESSAGE_EDIT, L"");
	}
}



bool IngameSendMessageDialog::getCompletedName(const wchar_t* nameStart, WideStringClass& fullName) const
{
	int nameStartLength = wcslen(nameStart);
	if (nameStartLength != 1 || (nameStart[0] != L'r' && nameStart[0] != L'R'))
	{
		for (SLNode<cPlayer>* iter = PlayerList.Head(); iter; iter = iter->Next())
		{
			cPlayer* player = iter->Data();
			
			if (player->IsActive &&
				player->PlayerName &&
				_wcsnicmp(player->PlayerName, nameStart, nameStartLength) == 0)
			{
				fullName = player->PlayerName;
				return true;
			}
		}
	}

	if (_wcsnicmp(nameStart, TRANSLATE(IDS_MP_HOST), nameStartLength) == 0)
	{
		fullName = TRANSLATE(IDS_MP_HOST);
		return true;
	}

	return false;
}



bool IngameSendMessageDialog::autoComplete(EditCtrlClass* editControl, bool correctable)
{
	WideStringClass message = editControl->Get_Text();
	
	if (message[0] == L'/')
	{
		int caretPos = editControl->Get_Caret_Pos();
		if (caretPos >= 1)
		{
			const wchar_t* spacePtr = wcschr(message, L' ');
			if (!spacePtr || spacePtr - message > caretPos)
			{
				int selStart;
				int selEnd;
				editControl->Get_Sel(selStart, selEnd);
				if (selStart != -1 && selStart < selEnd)
				{
					message.removeSubstring(selStart, selEnd - selStart);
					
					caretPos = selStart;
					spacePtr = wcschr(message, L' ');
				}
				
				WideStringClass pattern = message.substring(1, caretPos - 1);
				if (getCompletedName(pattern, autoCompletingPlayerName))
				{
					if (correctable)
					{
						message.replaceSubstring(caretPos, 0, autoCompletingPlayerName + caretPos - 1);

						editControl->Set_Text(message);
						editControl->Set_Caret_Pos(caretPos);
						
						int endSelectionPos = 1 + autoCompletingPlayerName.Get_Length();
						
						if (caretPos == endSelectionPos)
							editControl->Set_Sel(-1, -1);
						
						else
						{
							editControl->Set_Sel(caretPos, endSelectionPos);
							editControl->Set_Hilight_Anchor(endSelectionPos);
						}
					}
					else
					{
						message.removeSubstring(0, caretPos);

						editControl->Set_Text(message);
						editControl->Set_Caret_Pos(0);

						setTarget(autoCompletingPlayerName);
					}

					return true;
				}
			}
		}
	}

	return false;
}



void IngameSendMessageDialog::setTarget(const wchar_t* _sendTargetName)
{
	sendTargetName = _sendTargetName;
	
	WideStringClass sendTargetLabelText = sendTargetName;
	sendTargetLabelText += L":";
	Get_Dlg_Item(IDC_TYPE_STATIC)->Set_Text(sendTargetLabelText);
}



void IngameSendMessageDialog::On_EditCtrl_Change(EditCtrlClass* editControl, int)
{
	if (canAutoComplete)
		autoComplete(editControl, true);
}



bool IngameSendMessageDialog::On_EditCtrl_Key_Down(EditCtrlClass* editControl, uint32 character, uint32 keyData)
{
	canAutoComplete = character != '\b';
	if (character == ' ' && autoComplete(editControl, false))
		return true;
	
	return false;
}
