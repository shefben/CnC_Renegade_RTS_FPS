#ifndef TT_INCLUDE__VECTOR_H
#define TT_INCLUDE__VECTOR_H



template<typename T> class VectorAllocator
{
};



template<typename T> class Vector//Cloning this here since the real std::vector may not always match what renegade expects
{

public:

	VectorAllocator<T> allocator; // 0000
	T* _Myfirst; // 0004  0000
	T* _Mylast; // 0008  0004
	T* _Myend; // 000C  0008


	Vector()
	{
		_Myfirst = 0;
		_Mylast = 0;
		_Myend = 0;
	}
	~Vector()
	{
		if (_Myfirst)
		{
			delete[] _Myfirst;
		}
	}
	T* Find(const T &value)
	{
		if (!_Myfirst)
		{
			return _Mylast;
		}
		T *ptr;
		for (ptr = _Myfirst;ptr != _Mylast;ptr++)
		{
			if (*ptr == value)
			{
				break;
			}
		}
		return ptr;
	}
	bool push_back(const T value)
	{
		T *temp;
		int size = 0;
		if (_Myfirst)
		{
			if (Find(value) != _Mylast)
			{
				return false;
			}
			if (_Mylast == _Myend)
			{
				temp = _Myfirst;
				size = _Mylast - temp;
				_Myfirst = new T[size+8];
				_Mylast = _Myfirst + size;
				_Myend = _Mylast + 8;
				memcpy(_Myfirst,temp,size*sizeof(T));
			}
		}
		else
		{
			_Myfirst = new T[8];
			_Mylast = _Myfirst;
			_Myend = _Myfirst + 8;
		}
		memcpy(_Mylast++,&value,sizeof(T));
		return true;
	}
	bool erase(const T& value)
	{
		T *temp = Find(value);
		if (temp == _Mylast)
			return false;
		
		if (temp != _Mylast-1)
			memmove(temp,temp+1,(_Mylast-1-temp)*sizeof(T));
		
		_Mylast--;
		return true;
	}
	
	uint size() const
	{
		return _Mylast - _Myfirst;
	}

	const T& operator[](const uint index) const
	{
		return *(_Myfirst + index);
	}

	T& operator[](const uint index)
	{
		return *(_Myfirst + index);
	}

	bool empty() const
	{
		return _Myfirst == _Mylast;
	}

	const T& back() const
	{
		return *(_Mylast - 1);
	}

	T& back()
	{
		return *(_Mylast - 1);
	}

	T* begin() { return _Myfirst; }
	const T* begin() const { return _Myfirst; }
	T* end() { return _Mylast; }
	const T* end() const { return _Mylast; }

}; // 0010  000C



#endif