/******************************************************************************
*
* FILE
*	  $Archive: /Commando/Code/wwlib/Notify.h $
*
* DESCRIPTION
*	  These templates provide implementation of the Subject-Observer pattern.
*
* PROGRAMMER
*	  Steve Clinard
*	  $Author: Denzil_l $
*
* VERSION INFO
*	  $Modtime: 11/13/01 10:49a $
*	  $Revision: 8 $
*
******************************************************************************/

#ifndef __NOTIFY_H__
#define __NOTIFY_H__

// Reduce warning level for STL
#if defined(_MSC_VER)
#pragma warning(push, 3)
#endif

#include <Vector.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <assert.h>

template<typename Event> class Notifier;
template<typename Event> class Observer;

template<typename Event> class Observer
{
public:
	typedef Vector< Notifier<Event>* > NotifierColl;

	Observer() :
		mNotifiers()
	{
	}

	virtual ~Observer()
	{
		StopObserving();
	}

	//! Handle event notification
	virtual void HandleNotification(Event&) = 0;

	//! Notifier has ended notification of this event
	virtual void NotificationEnded(Notifier<Event>& notifier) // TODO: Parameter was const, I just couldn't get it to compile properly.
	{
		mNotifiers.erase(&notifier);
	}

	//! Request notification of this event
	virtual void NotifyMe(Notifier<Event>& notifier)
	{
		notifier.AddObserver(*this);
	}

	//! Stop observing event
	void StopObserving()
	{
		while (mNotifiers.size() > 0)
		{
			Notifier<Event>* notifier = mNotifiers.back();
			assert(notifier && "ERROR: NULL pointer in collection.");
			notifier->RemoveObserver(*this);
		}
	}

protected:
	Observer(const Observer<Event>& observer);
	const Observer<Event>& operator=(const Observer<Event>&);

private:
	friend class Notifier<Event>;
	NotifierColl mNotifiers;
}; // 0014


#define DECLARE_OBSERVER(Event) \
	virtual void NotifyMe(Notifier<Event>& observer) \
		{Notifier<Event>::AddObserver(observer);}


template<typename Event> class Notifier
{

public:

	typedef Vector<Observer<Event>*> ObserverColl;

	Notifier()
	{
	}

	virtual ~Notifier()
	{
		for (int index = mObservers.size(); index--;)
		{
			mObservers[index]->NotificationEnded(*this);
		}
	}

	//! Send event notification to all observers of this event.
	virtual void NotifyObservers(Event& event)
	{
		for (unsigned int index = 0; index < mObservers.size(); index++)
		{
			mObservers[index]->HandleNotification(event);
		}
	}

	//! Add an observer of this event
	virtual void AddObserver(Observer<Event>& observer)
	{
		if (mObservers.Find(&observer) != mObservers._Mylast)
			return;
		
		observer.mNotifiers.push_back(this);
		mObservers.push_back(&observer);
	}

	//! Remove an observer of this event
	virtual void RemoveObserver(Observer<Event>& observer)
	{
		if (mObservers.Find(&observer) != mObservers._Mylast)
			return;

		observer.NotificationEnded(*this);
		mObservers.erase(&observer);
	}

	virtual bool HasObservers(void) const
	{
		return !mObservers.empty();
	}

private:

	//! Observer collection
	ObserverColl mObservers; // 0004

}; // 0014  0010


#define DECLARE_NOTIFIER(Event) \
	virtual void NotifyObservers(Event& event) \
		{Notifier<Event>::NotifyObservers(event);} \
	virtual void AddObserver(Observer<Event>& observer) \
		{Notifier<Event>::AddObserver(observer);} \
	virtual void RemoveObserver(Observer<Event>& observer) \
		{Notifier<Event>::RemoveObserver(observer);} 


/*-----------------------------------------------------------------------------
 * The following templates are useful for defining unique types to use as
 * Events from types such as strings or integers.
 *---------------------------------------------------------------------------*/

/* TypedEvent<T. V>
 * 
 * The first type (T) must be a class or other unique type. This need not
 * be a "real" class. It could be a forward declared class, which is enough
 * to make the template class unique.
 *
 * The second type (V) is the event data. "UString" and "int" are obvious
 * choices.
 *
 * Typedef'ing the template class is a good thing to do.
 */
template<typename T, typename V>
class TypedEvent
	{
	public:
		TypedEvent(V& value) :
				mValue(value)
			{}

		inline V& operator()()
			{return mValue;}

		inline V& Subject(void)
			{return mValue;}

		TypedEvent& operator=(TypedEvent& e) //added this to shut up a wierd compiler issue
		{
			mValue = e.mValue;
		}

	protected:
		V& mValue;
	};

template<typename T, typename O>
class TypedEventPtr
	{
	public:
		TypedEventPtr(O* subject) :
				mSubject(subject)
			{}

		inline O* Subject(void)
			{return mSubject;}

		inline O* operator()()
			{return mSubject;}

	protected:
		O* mSubject;
	};

template<typename A, typename O>
class TypedActionPtr :
		public TypedEventPtr<A, O>
	{
	public:
		A GetAction(void) const
			{return mAction;}

		TypedActionPtr(A action, O* data) :
				TypedEventPtr<A, O>(data),
				mAction(action)
			{}

		~TypedActionPtr()
			{}

	protected:
		A mAction;
	};

#endif // __NOTIFY_H__
