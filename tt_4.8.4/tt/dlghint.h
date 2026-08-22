#ifndef TT_DLGHINT_H
#define TT_DLGHINT_H
#include "PopupDialog.h"
class DlgHint : public PopupDialogClass
{
public:
	static void DoDialog(int titleID,int textID,int textID2,int textID3);
protected:
	DlgHint();
	virtual ~DlgHint();
	void On_Command(int ctrl, int message, DWORD param);
private:
	DlgHint(const DlgHint&);
	const DlgHint& operator=(const DlgHint&);
};

#endif