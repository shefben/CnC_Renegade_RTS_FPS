#ifndef _LLIST_H_
#define _LLIST_H_

template<typename T> class LListItem{
public:
	LListItem<T> *m_Next;
	LListItem<T> *m_Prev;
	T m_Data;
	__inline LListItem<T>(){
		m_Next = m_Prev = NULL;
	}
};

template <typename T> class LList{
private:
	LListItem<T> *m_Head;
	LListItem<T> *m_Tail;
public:
	LList(){
		m_Head = m_Tail = NULL;
	}
	LListItem<T> *Add_To_Head(T item){
		LListItem<T> *temp = new LListItem<T>;
		if (!temp) return NULL;
		if (m_Head)
			m_Head->m_Prev = temp;
		temp->m_Next = m_Head;
		m_Head = temp;
		temp->m_Data = item;
		if (!m_Tail) m_Tail = temp;
		return temp;
	}
	LListItem<T> *Add_To_Tail(T item){
		LListItem<T> *temp = new LListItem<T>;
		if (!temp) return NULL;
		if (m_Tail)
			m_Tail->m_Next = temp;
		temp->m_Prev = m_Tail;
		m_Tail = temp;
		temp->m_Data = item;
		if (!m_Head) m_Head = temp;
		return temp;
	}
	int Count(){
		LListItem<T> *temp = m_Head;
		int i = 0;
		while (temp){
			i++;
			temp = temp->m_Next;
		}
		return i;
	}
	LListItem<T> *Get(int idx){
		LListItem<T> *temp = m_Head;
		int i = 0;
		while (temp){
			if (i == idx)
				return temp;
			temp = temp->m_Next;
			i++;
		}
		return NULL;
	}
	bool Belongs_To_List(LListItem<T> *item){		
		LListItem<T> *temp = m_Head;
		while (temp){
			if (temp == item)
				return true;
			temp = temp->m_Next;
		}
		return false;
	}
	void Delete(LListItem<T> *item){
		if (!item) return;

		if (m_Head == item)
			m_Head = item->m_Next;

		if (m_Tail == item)
			m_Tail = item->m_Prev;

		if (item->m_Prev)
			item->m_Prev->m_Next = item->m_Next;

		if (item->m_Next)
			item->m_Next->m_Prev = item->m_Prev;

		delete item;
	}
	LListItem<T> *Find(T cmp){		
		LListItem<T> *temp = m_Head;
		while (temp){
			if (temp->m_Data == cmp) return temp;
			temp = temp->m_Next;
		}
		return NULL;
	}
	LListItem<T> *Get_First(){
		return m_Head;
	}
	LListItem<T> *Get_Last(){
		return m_Tail;
	}
	void Clear_All(){
		LListItem<T> *temp = m_Head, *temp2;
		while (temp){
			temp2 = temp->m_Next;
			delete temp;
			temp = temp2;
		}
		m_Head = m_Tail = NULL;
	}
};

#endif //_LLIST_H_