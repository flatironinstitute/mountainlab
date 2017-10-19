#ifndef MLITERATOR_H
#define MLITERATOR_H

#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>

namespace ML {

/*!
 * \class counting_iterator
 * \brief counting_itertor is a template class which iterates over "numbers" (or other integral-like types)
 *
 * This class is useful for generating sequences of numbers using an iterator.
 *
 * Template type T has to implement the following semantics:
 * - ++x
 * - x++
 * - --x
 * - x--
 * - x+=n
 * - x-=n
 * - x+y
 * - x-y
 * - x<y
 * - x>y
 * - x<=y
 * - x>=y
 * - x==y
 * - x!=y
 *
 * Example use: create a vector containing a sequence of numbers [0-100]
 *
 * \code
 * std::vector<int> v(counting_iterator<int>(0), counting_iterator<int>(101);
 * \endcode
 *
 *
 */
template <typename T>
class counting_iterator {
public:
    typedef T value_type;
    typedef const T& reference;
    typedef const T* pointer;
    typedef std::random_access_iterator_tag iterator_category;
    typedef int difference_type;

    counting_iterator() {}
    counting_iterator(T val)
        : m_value(val)
    {
    }
    counting_iterator(const counting_iterator& other)
        : m_value(other.m_value)
    {
    }

    bool operator==(const counting_iterator& other) const { return m_value == other.m_value; }
    bool operator!=(const counting_iterator& other) const { return m_value != other.m_value; }
    reference operator*() { return m_value; }
    pointer operator->() { return &m_value; }
    value_type const& base() const { return m_value; }
    counting_iterator operator++(int) { return counting_iterator(m_value++); }
    counting_iterator operator--(int) { return counting_iterator(m_value--); }
    counting_iterator& operator++()
    {
        ++m_value;
        return *this;
    }
    counting_iterator& operator--()
    {
        --m_value;
        return *this;
    }
    counting_iterator& operator+=(difference_type dist)
    {
        m_value += dist;
        return *this;
    }
    counting_iterator& operator-=(difference_type dist)
    {
        m_value -= dist;
        return *this;
    }
    counting_iterator operator+(difference_type dist) { return counting_iterator(m_value + dist); }
    counting_iterator operator-(difference_type dist) { return counting_iterator(m_value - dist); }
    difference_type operator-(const counting_iterator& other) { return base() - other.base(); }
    reference operator[](difference_type n) { return *(*this + n); }
    bool operator<(const counting_iterator& other) const { return base() < other.base(); }
    bool operator>(const counting_iterator& other) const { return base() > other.base(); }
    bool operator>=(const counting_iterator& other) const { return base() >= other.base(); }
    bool operator<=(const counting_iterator& other) const { return base() <= other.base(); }

private:
    T m_value;
};
}

#endif // MLITERATOR_H
