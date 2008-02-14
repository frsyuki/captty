#ifndef SPARCE_ARRAY_H__
#define SPARCE_ARRAY_H__ 1

#include <vector>
#include <stdexcept>


template <typename Data>
class sparse_array {
public:
	typedef Data data_t;
	typedef size_t size_type;
	sparse_array();
	~sparse_array();
	inline void set(size_type index, data_t* data);
	inline void reset(size_type index);
	inline data_t& data(size_type index);
	inline const data_t& data(size_type index) const;
	inline bool test(size_type index) const;
private:
	struct item_t {
		item_t() : data(NULL) {}
		data_t* data;
	};
	typedef std::vector<item_t> set_t;
	set_t m_set;
};



template <typename Data>
sparse_array<Data>::sparse_array() {}

template <typename Data>
sparse_array<Data>::~sparse_array()
{
	for(typename set_t::iterator it(m_set.begin()), it_end(m_set.end());
			it != it_end;
			++it ) {
		delete it->data;
	}
}

template <typename Data>
void sparse_array<Data>::set(size_type index, data_t* data)
{
	if( m_set.size() <= index ) {
		m_set.resize(index + 1);
		m_set.resize(m_set.capacity());
		m_set[index].data = data;
		return;
	}
	if( m_set[index].data ) {
		delete m_set[index].data;
	}
	m_set[index].data = data;
}

template <typename Data>
void sparse_array<Data>::reset(size_type index)
{
	if( m_set[index].data ) {
		delete m_set[index].data;
		m_set[index].data = NULL;
	}
}

template <typename Data>
bool sparse_array<Data>::test(size_type index) const
{
	return m_set.size() > index && m_set[index].data;
}


template <typename Data>
Data& sparse_array<Data>::data(size_type index)
{
	return *m_set[index].data;
}

template <typename Data>
const Data& sparse_array<Data>::data(size_type index) const
{
	return *m_set[index].data;
}


#endif /* sparse_array.h */

