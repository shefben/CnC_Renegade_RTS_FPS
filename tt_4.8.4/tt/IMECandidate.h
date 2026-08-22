/******************************************************************************
*
* FILE
*     $Archive: /Commando/Code/wwui/IMECandidate.h $
*
* DESCRIPTION
*
* PROGRAMMER
*     $Author: Denzil_l $
*
* VERSION INFO
*     $Revision: 4 $
*     $Modtime: 1/11/02 6:44p $
*
******************************************************************************/

#ifndef __IMECANDIDATE_H__
#define __IMECANDIDATE_H__

#include "Notify.h"
//#include "win.h"
#include <imm.h>

#pragma warning(disable : 4514)

#if defined(_MSC_VER)
#pragma warning(push, 3)
#endif

#include <vector>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace IME {

class IMECandidate
	{
	public:
		IMECandidate();
		~IMECandidate();

		void Open(int index, HWND hwnd, UINT codepage, bool unicode, bool startFrom1);
		void Read(void);
		void Close(void);

		bool IsValid(void) const;

		int GetIndex(void) const;

		unsigned long GetStyle(void) const;

		// Get the index of the first candidate in the page
		unsigned long GetPageStart(void) const;

		// Set the page to start with the specified candidate index
		void SetPageStart(unsigned long);

		// Get the number of candidates per page
		unsigned long GetPageSize(void) const;

		// Get the total number of candidates in the list.
		unsigned long GetCount(void) const;

		// Get the index of the current candidate selection
		unsigned long GetSelection(void) const;

		// Get the specified candidate string
		const wchar_t* GetCandidate(unsigned long index);

		// Select a candidate from the list.
		void SelectCandidate(unsigned long index);

		// Set the candidate page view
		void SetView(unsigned long topIndex, unsigned long bottomIndex);

		// Check if the candidates should be displayed starting from 1 or 0
		bool IsStartFrom1(void) const;

	private:
		int mIndex;
		HWND mHWND;
		UINT mCodePage;
		bool mUseUnicode;
		bool mStartFrom1;

		unsigned long mCandidateSize;
		CANDIDATELIST* mCandidates;

		// Multibyte -> Unicode string conversion buffer
		wchar_t mTempString[80];
	};


typedef enum
	{
	CANDIDATE_OPEN = 1,
	CANDIDATE_CHANGE,
	CANDIDATE_CLOSE
	} CandidateAction;

typedef TypedActionPtr<CandidateAction, IMECandidate> CandidateEvent;

typedef Vector<IMECandidate> IMECandidateCollection;

} // namespace IME

#endif // __IMECANDIDATE_H__

