#pragma once

#include <array>
#include <vector>



/// Statically sized view of buffer (no ownership)
template<typename T, int Size=-1>
class buffer_view
{
public:
	buffer_view(T* ptr):
		m_ptr(ptr)
	{
	}

	/// Return a dynamic view
	buffer_view<T,-1> DynamicView()
	{
		return buffer_view<T,-1>(m_ptr, Size);
	}

private:
	T* m_ptr;
};

/// Dynamically sized view of buffer (no ownership)
template<typename T>
class buffer_view<T,-1>
{
public:
	buffer_view(T* ptr, std::ptrdiff_t size):
		m_ptr(ptr), m_size(size)
	{
	}

	template<int N>
	buffer_view(std::array<T, N>& a):
		m_ptr(a.data()), m_size(static_cast<std::ptrdiff_t>(a.size()))
	{
	}

	explicit buffer_view(std::vector<T>& v):
		m_ptr(v.data()), m_size(static_cast<std::ptrdiff_t>(v.size()))
	{
	}

	/// Return a static view
	template<int Size>
	buffer_view<T,Size> StaticView()
	{
		if(Size != m_size)
			throw std::runtime_error("Invalid buffer size");
		return buffer_view<T,-1>(m_ptr);
	}

	T* data()
	{
		return m_ptr;
	}

	std::ptrdiff_t size() const
	{
		return m_size;
	}

	T* begin()
	{
		return m_ptr;
	}

	T* end()
	{
		return m_ptr + m_size;
	}

	template<typename TT = T>
	typename std::enable_if<!std::is_same<TT, void>::value, T>::type&
	operator[](int index)
	{
		return m_ptr[index];
	}

private:
	T* m_ptr;
	std::ptrdiff_t m_size;
};



