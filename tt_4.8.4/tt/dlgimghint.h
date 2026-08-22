#ifndef TT_DLGIMGHINT_H
#define TT_DLGIMGHINT_H
#include "PopupDialog.h"
class DlgImgHint : public PopupDialogClass
{
public:
	static void DoDialog(int titleID,int textID,int textID2,int textID3,char *texture);
protected:
	DlgImgHint();
	virtual ~DlgImgHint();
	void On_Command(int ctrl, int message, DWORD param);
private:
	DlgImgHint(const DlgImgHint&);
	const DlgImgHint& operator=(const DlgImgHint&);
};

#endif