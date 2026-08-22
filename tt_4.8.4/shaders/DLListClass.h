#ifndef TT_INCLUDE__DLLISTCLASS_H
#define TT_INCLUDE__DLLISTCLASS_H
template <class T> class DLListClass;
template <class T> class DLNodeClass {
	friend class DLListClass<T>;
private:
	DLNodeClass<T>* succ; // 0000
	DLNodeClass<T>* pred; // 0004
	DLListClass<T>* list; // 0008
public:
	DLNodeClass()
	{
		succ = NULL;
		pred = NULL;
		list = NULL;
	}
	T *Succ()
	{
		return (T *)succ;
	}
	T *Pred()
	{
		return (T *)pred;
	}
	DLListClass<T> *List()
	{
		return list;
	}
	~DLNodeClass()
	{
		this->Remove();
	}
	void Insert_Before(DLNodeClass<T>* n)
	{
		this->list = n->list;
		this->succ = n;
		this->pred = n->pred;
		if (n->pred)
		{
			n->pred->succ = this;
		}
		n->pred = this;
		if (this->list->head == n)
		{
			this->list->head = this;
		}
	}
	void Insert_After(DLNodeClass<T>* n)
	{
		this->list = n->list;
		this->pred = n;
		this->succ = n->succ;
		if (n->succ)
		{
			n->succ->pred = this;
		}
		n->succ = this;
		if (this->list->tail == n)
		{
			this->list->tail = this;
		}
	}
	void Remove()
	{
		if (this->list)
		{
			DLListClass<T> *list = this->list;
			this->list = 0;

			if (list->head == this)
				list->Remove_Head();

			else if (list->tail == this)
				list->Remove_Tail();

			else
			{
				if (this->succ)
					this->succ->pred = this->pred;

				if (this->pred)
					this->pred->succ = this->succ;
			}
		}
	}
}; // 000C
template <class T> class DLListClass {
	friend class DLNodeClass<T>;
private:
	DLNodeClass<T>* head;
	DLNodeClass<T>* tail;
public:
	DLListClass()
	{
		head = NULL;
		tail = NULL;
	}
	T *Head()
	{
		return (T *)head;
	}
	T *Tail()
	{
		return (T *)tail;
	}
	virtual ~DLListClass()
	{
	}
	void Add_Head(DLNodeClass<T>* n)
	{
		n->list = this;
		if (this->head != 0)
		{
			n->Insert_Before(this->head);
			this->head = n;
		}
		else
		{
			this->head = n;
			this->tail = n;
			n->succ = 0;
			n->pred = 0;
		}
	}
	void Remove_Head()
	{
		if (this->head)
		{
			DLNodeClass<T> *node = this->head;
			this->head = node->succ;
			if (!node->succ)
			{
				this->tail = 0;
			}
			else
			{
				this->head->pred = 0;
			}
			node->Remove();
		}
	}
	T * __Pop_Head()
	{
		if (this->head)
		{
			DLNodeClass<T> *node = this->head;
			this->head = node->succ;
			if (!node->succ)
			{
				this->tail = 0;
			}
			else
			{
				this->head->pred = 0;
			}
			node->Remove();
			return (T*) node;
		}
		return NULL;
	}
	void Add_Tail(DLNodeClass<T>* n)
	{
		n->list = this;
		if (this->tail != 0)
		{
			n->Insert_After(this->tail);
			this->tail = n;
		}
		else
		{
			this->head = n;
			this->tail = n;
			n->succ = 0;
			n->pred = 0;
		}
	}
	void Remove_Tail()
	{
		if (this->tail)
		{
			DLNodeClass<T> *node = this->tail;
			this->tail = node->pred;
			if (!node->pred)
			{
				this->head = 0;
			}
			else
			{
				this->tail->succ = 0;
			}
			node->Remove();
		}
	}

	uint count() const
	{
		uint result = 0;
		
		for (DLNodeClass<T>* node = head; node; node = node->succ)
			result++;

		return result;
	}
};
#endif
