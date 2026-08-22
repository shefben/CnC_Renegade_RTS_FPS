#ifndef _VECTORANDLISTTEMPLATECLASSES_H_
#define _VECTORANDLISTTEMPLATECLASSES_H_
#pragma once
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

template<typename T> class VectorClass {
public:
	T *m_pVector;
	int m_nItems;
    bool m_Safe_For_Thread_Access;
	bool m_Is_Locally_Allocated;


	VectorClass(int nItems, T const *pItems){
		m_nItems = nItems;
		m_Is_Locally_Allocated = false;
		m_Safe_For_Thread_Access = true;
		if (nItems){
			if (pItems)
				m_pVector = (T *)pItems;
			else
				m_pVector = new T[nItems]();
		}
	}
	virtual ~VectorClass(){
		this->Clear();
	}
	virtual bool __eq(VectorClass<T> const &comp){
		if (m_nItems != comp.m_nItems || (!m_pVector || !comp.m_pVector)) return false;
		if (m_nItems < 1 ) return true;
		for (int i = 0; i < m_nItems; i++){
			if (m_pVector[i] != comp.m_pVector[i])
				return false;
		}
		return true;
	}
	virtual bool Resize(int nItems, T const *pItems){
		T *v_pVector = NULL;
		register int v_nItems;
		if (!nItems){
			Clear();
			return true;
		} else {
			m_Safe_For_Thread_Access = false;
			if (!pItems)
				v_pVector = new T[nItems]();
			else
				v_pVector = (T*)pItems;
			m_Safe_For_Thread_Access = true;
			if (!v_pVector)
				return false;
			if (m_pVector){
				v_nItems = (m_nItems <= nItems ? m_nItems : nItems);
				if (v_nItems > 1){
					for (int i = 0; i < v_nItems; i++)
						v_pVector[i] = m_pVector[i];
					if (m_Is_Locally_Allocated && m_pVector)
						delete[]m_pVector;
				}
			}
			m_pVector = v_pVector;
			m_nItems = nItems;
			if (pItems)
				m_Is_Locally_Allocated = false;
			else
				m_Is_Locally_Allocated = true;
			return true;

		}
	}
	virtual void Clear(){
		if (m_pVector && m_Is_Locally_Allocated){
			delete[] m_pVector;
			m_pVector = NULL;
		}
		m_Is_Locally_Allocated = NULL;
		m_nItems = NULL;
	}
	virtual int ID(T const &instance){
		if (!m_Safe_For_Thread_Access) return 0;
		if (m_nItems < 1){
			return -1;
		}
		for (int i = 0; i < m_nItems; i++){
			if (memcmp(&m_pVector[i], &instance, sizeof(T)) == 0)
				return i;
		}
		return -1;
	}
	virtual int ID_(T const *ptr){
		if (!m_Safe_For_Thread_Access) return 0;
		if (m_nItems < 1){
			return -1;
		}
		return (int)((char*)ptr - (char*)m_pVector) >> 2;
	}
	inline T& operator[](int index){
		if (index < 0 || index >= m_nItems){
			//debug shit here
		}
		return m_pVector[index];
	}
};

template<typename T> class DynamicVectorClass : public VectorClass<T> {
public:
	int m_nItemCount;
    int m_nResizePadding;

	DynamicVectorClass(int nItems = 0, T const *pItems = 0 ) : VectorClass<T>(nItems, pItems){		
		m_nItemCount = 0;
		m_nResizePadding = 10;
	}
	virtual ~DynamicVectorClass(){
		this->Clear();
	}
	virtual bool __eq(VectorClass<T> const &comp){
		if (m_nItems != comp.m_nItems || (!m_pVector || !comp.m_pVector)) return false;
		if (m_nItems < 1 ) return true;
		for (int i = 0; i < m_nItems; i++){
			if (memcmp(&m_pVector[i], &comp.m_pVector[i], sizeof(T)))
				return false;
		}
		return true;
	}
	virtual bool Resize(int nItems, T const *pItems){
		if (__super::Resize(nItems, pItems)){
			if (m_nItemCount > m_nItems)
				m_nItemCount = m_nItems;
			return true;
		} else return false;
	}
	virtual void Clear(){
		m_nItemCount = 0;
		__super::Clear();
	}
	virtual int ID(T const &instance){
		if (!m_Safe_For_Thread_Access) return 0;
		if (m_nItemCount < 1){
			return -1;
		}
		for (int i = 0; i < m_nItemCount; i++){
			if (memcmp(&m_pVector[i], &instance, sizeof(T)) == 0)
				return i;
		}
		return -1;
	}
	virtual int ID_(T const *ptr){
		if (!m_Safe_For_Thread_Access) return 0;
		if (m_nItems < 1){
			return -1;
		}
		return (int)((char*)ptr - (char*)m_pVector) >> 2;
	}
	inline T& operator[](int index){
		if (index < 0 || index >= m_nItems){
			//debug shit here
		}
		return m_pVector[index];
	}
	bool Add(T const &instance){
		if (m_nItemCount >= m_nItems){
			if ((m_Is_Locally_Allocated == false && m_nItems > 0) || m_nResizePadding <= 0)
				return false;
			if (this->Resize(m_nItems + m_nResizePadding, NULL) == false)
				return false;
		}
		m_pVector[m_nItemCount++] = instance;
		return true;
	}
	bool Delete(int index){
		if (index < 0 || index >= m_nItemCount)
			return false;
		m_nItemCount--;
		for (int i = index; i < m_nItemCount; i++)		
			m_pVector[i] = m_pVector[i+1];
		return true;
	}
	bool Insert(int index, T const &instance){
		if (/*index == 0 ||*/ index >= m_nItemCount)
			return false;
		if (m_nItemCount >= m_nItems){
			if ((!m_Safe_For_Thread_Access && m_nItems != 0) || m_nResizePadding <= 0) 
				return false;
			if (!this->Resize(m_nItems + m_nResizePadding, NULL)) 
				return false;

		}
		if (index < m_nItemCount){
			memmove(&m_pVector[index+1], &m_pVector[index], (m_nItemCount-index)*sizeof(T));
		}
		m_pVector[index] = instance;
		m_nItemCount++;
		return true;
	}
	DynamicVectorClass<T> &operator = (DynamicVectorClass &src){
		int count;
		if (!Resize(count = src.m_nItems, NULL)) return *this;
		memcpy(m_pVector, src.m_pVector, sizeof(T)*count);
		m_nItemCount = src.m_nItemCount;
		m_nResizePadding = src.m_nResizePadding;
		return *this;
	}
};

template<typename T> class SimpleVecClass {
public:
	T *m_pVector;
	int m_nItems;

	SimpleVecClass(){
		m_nItems = 0;
		m_pVector = NULL;
	}
	virtual ~SimpleVecClass(){
		if (m_pVector)
			delete [] m_pVector;
	}
	virtual bool Resize(int nItems){
		T *v_pVector = NULL;
		register int v_nItems;
		if (nItems == m_nItems) return true;
		if (!nItems){
			if (m_pVector)
				delete[] m_pVector;
			m_pVector = NULL;
			m_nItems = 0;
			return true;
		} else {
			v_pVector = new T[nItems]();
			if (!v_pVector)
				return false;
			if (m_pVector){
				v_nItems = (m_nItems <= nItems ? m_nItems : nItems);
				if (v_nItems > 1){
					for (int i = 0; i < v_nItems; i++)
						v_pVector[i] = m_pVector[i];
					if (m_pVector)
						delete[]m_pVector;
				}
			}
			m_pVector = v_pVector;
			m_nItems = nItems;
			return true;
		}
	}
	virtual bool Uninitialised_Grow(int nItems){
		if (nItems <= m_nItems) return true;
		if (m_pVector)
			delete [] m_pVector;
		m_pVector = new T[nItems];
		m_nItems = nItems;
		return true;
	}
	inline T& operator[](int index){
		if (index < 0 || index >= m_nItems){
			//debug shit here
		}
		return m_pVector[index];
	}
};

template<typename T> class SimpleDynVecClass : public SimpleVecClass<T> {
public:
	int m_nItemCount;

	SimpleDynVecClass(){
		m_nItemCount = 0;
	}
	virtual ~SimpleDynVecClass(){
	}
	virtual bool Resize(int nItems){
		T *v_pVector = NULL;
		register int v_nItems;
		if (nItems != m_nItems){
			if (!nItems){
				if (m_pVector)
					delete[] m_pVector;
				m_pVector = NULL;
				m_nItems = 0;
			} else {
				v_pVector = new T[nItems]();
				if (m_pVector && v_pVector){
					v_nItems = (m_nItems <= nItems ? m_nItems : nItems);
					if (v_nItems > 1){
						for (int i = 0; i < v_nItems; i++)
							v_pVector[i] = m_pVector[i];
						if (m_pVector)
							delete[]m_pVector;
					}
				}
				m_pVector = v_pVector;
				m_nItems = nItems;
			}
		}
		if (m_nItems < m_nItemCount)
			m_nItemCount = m_nItems;
		return true;
	}
	virtual bool Uninitialised_Grow(int nItems){
		if (nItems <= m_nItems) return true;
		if (m_pVector)
			delete [] m_pVector;
		m_pVector = new T[nItems];
		m_nItems = nItems;
		return true;
	}
	inline T& operator[](int index){
		if (index < 0 || index >= m_nItems){
			//debug shit here
		}
		return m_pVector[index];
	}
};

#endif //_VECTORANDLISTTEMPLATECLASSES_H_