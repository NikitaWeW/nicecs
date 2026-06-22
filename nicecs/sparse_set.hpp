/*
      ___  ___ ___ 
     / _ \/ __/ __|        Copyright (c) 2024 Nikita Martynau 
    |  __/ (__\__ \        https://opensource.org/license/mit 
     \___|\___|___/ v1.5.8 https://github.com/nikitawew/nicecs


Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <algorithm>
#include <limits>
#include <vector>
#include <cstdint>

#ifndef ECS_PROFILE
#define ECS_PROFILE
#endif
#ifndef ECS_ASSERT
#define ECS_ASSERT(x, msg) assert((x) && msg)
#endif

namespace ecs
{

    /// @brief A sparse set implementation.
    /// @tparam dense_t The type of densely stored data.
    template<typename dense_t>
    class sparse_set
    {
    private:
        // dry
        template<typename owner_t, typename reference_second_t>
        class basic_iterator;
    public:
        /// @brief The type of the sparse index.
        using sparse_type = std::size_t;
        /// @brief The type of densely stored data.
        using dense_type = dense_t;
        /// @brief The type used to index the densely stored data.
        using index_type = std::uint32_t;
        
        /// @copydoc basic_iterator
        using iterator = basic_iterator<sparse_set, dense_type>;
        /// @copydoc basic_iterator
        using const_iterator = basic_iterator<sparse_set const, dense_type const>;

        /// @brief The sparse pointer that represents the empty index.
        static constexpr index_type null = std::numeric_limits<index_type>::max();
    private:
        std::vector<dense_type> mDense;
        std::vector<sparse_type> mDenseToSparse;
        std::vector<std::vector<index_type>> mSparse;
        std::uint32_t mPageSize = 256;

        void setDenseIndex(sparse_type const &sparse, index_type index);
    public:
        /// @param capacity The optional capacity to reserve.
        /// @param pageSize Number of indices in one sparse page. Bigger page size reduces fragmentation but increases potential memory waste.
        sparse_set(std::size_t capacity = 10, std::uint32_t pageSize = 256);
        ~sparse_set() = default;
        sparse_set(sparse_set const &other);
        sparse_set(sparse_set &&other) noexcept;
        sparse_set &operator=(sparse_set const &other);
        sparse_set &operator=(sparse_set &&other) noexcept;

        /// @brief The container is extended by inserting a new element at sparse position. This new element is constructed in place using args as the arguments for its construction.
        /// @param sparse A sparse index.
        /// @param args Arguments forwarded to construct the new element.
        /// @throws std::invalid_argument If the sparse index already contains an element.
        template <class... Args>
        void emplace(sparse_type const &sparse, Args&&... args);

        /// @brief Removes an element from a sparse index.
        /// @param sparse A sparse index.
        /// @throws std::out_of_range If a sparse index doesent contain the element.
        void erase(sparse_type const &sparse);

        /// @brief Gets an element at a sparse index.
        /// @param sparse A sparse index.
        /// @return An element lvalue reference.
        /// @throws std::out_of_range If a sparse index doesent contain the element.
        dense_type const &get(sparse_type const &sparse) const;

        /// @copydoc get
        dense_type &get(sparse_type const &sparse);

        /// @brief Get an element. Just like std::map::operator[]
        /// If the container doesent contain @p sparse index, default construct it. 
        dense_type &operator[](sparse_type const &sparse);

        /// @brief Gets the dense list.
        /// @return A std::vector with the elements.
        std::vector<dense_type> const &dense() const;

        /// @brief Get dense to sparse mapping. All non-null sparse indices.
        /// @return 1 to 1 with the dense data vector with the dense to sparse mapping.
        std::vector<sparse_type> const &sparse() const;

        /// @brief Get an index of the element at sparse index.
        /// @param sparse The sparse index.
        /// @return The index of the element, null if container doesent contain @p sparse.
        index_type getDenseIndex(sparse_type const &sparse) const;

        /// @brief Check whether the sparse set contains an element at a given sparse index.
        /// @param sparse A sparse index.
        /// @return True if found, false otherwise.
        bool contains(sparse_type const &sparse) const;

        /// @brief Increase the capacity of the dense list.
        void reserve(std::size_t newCapacity);

        /// @brief Sets the capacity to the size.
        void shrink_to_fit();

        /// @return True if the container is empty, false otherwise.
        bool empty() const;

        /// Get the size of the container.
        /// @return Dense list size.
        std::size_t size() const;

        /// @brief Clear the sparse set.
        void clear();

        /// @brief Get the pointer pointing to the beginning of the dense list. 
        /// Useful for making changes to the entire set.
        dense_type *denseData();
        /// @copydoc denseData
        dense_type const *denseData() const;

        /// @brief The cbegin of the sparse set.
        const_iterator begin() const;
        /// @brief The cend of the sparse set.
        const_iterator end() const;
        /// @brief The begin of the sparse set.
        iterator begin();
        /// @brief The end of the sparse set.
        iterator end();
    private:
        /// @brief The [sparse; dense] pair iterator.
        template<typename owner_t, typename reference_second_t>
        class basic_iterator
        {
            owner_t *mOwner;
            std::size_t mIndex;
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::pair<sparse_type, reference_second_t&>;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = value_type;

            inline basic_iterator() : mOwner(nullptr), mIndex(0) {}
            inline basic_iterator(owner_t *owner, std::size_t index) : mOwner(owner), mIndex(index) {}

            inline reference operator*() const { return {mOwner->mDenseToSparse[mIndex], mOwner->mDense[mIndex]}; }

            inline basic_iterator &operator++() { ++mIndex; return *this; }
            inline basic_iterator operator++(int) { basic_iterator tmp = *this; ++mIndex; return tmp; }

            inline basic_iterator &operator--() { --mIndex; return *this; }
            inline basic_iterator operator--(int) { basic_iterator tmp = *this; --mIndex; return tmp; }

            inline basic_iterator &operator+=(difference_type n) { mIndex += n; return *this; }
            inline basic_iterator operator+(difference_type n) const { return basic_iterator(mOwner, mIndex + n); }
            inline basic_iterator &operator-=(difference_type n) { mIndex -= n; return *this; }
            inline basic_iterator operator-(difference_type n) const { return basic_iterator(mOwner, mIndex - n); }

            inline difference_type operator-(basic_iterator const &other) const { return static_cast<difference_type>(mIndex) - static_cast<difference_type>(other.mIndex); }

            inline bool operator==(basic_iterator const &o) const { return mOwner == o.mOwner && mIndex == o.mIndex; }
            inline bool operator!=(basic_iterator const &o) const { return !(*this == o); }

            inline reference operator[](difference_type n) const { return *(*this + n); }
        };
    };

    /// @brief An alias for sparse_set::null.
    constexpr auto SPARSE_SET_NULL = sparse_set<int>::null;
} // namespace ecs


template <typename dense_t>
inline void ecs::sparse_set<dense_t>::setDenseIndex(sparse_type const &sparse, ecs::sparse_set<dense_t>::index_type index)
{
    ECS_PROFILE;
    auto pageIndex = sparse / mPageSize;
    if(pageIndex >= mSparse.size())
        mSparse.resize(pageIndex + 1);

    auto &page = mSparse[pageIndex];
    if(page.empty())
        page = std::vector<index_type>(mPageSize, null);

    page[sparse % mPageSize] = index;
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::index_type ecs::sparse_set<dense_t>::getDenseIndex(sparse_type const &sparse) const
{
    ECS_PROFILE;
    auto pageIndex = sparse / mPageSize;
    if(pageIndex >= mSparse.size()) 
        return null;

    auto const &page = mSparse[pageIndex];
    if(page.empty())
        return null;

    return page[sparse % mPageSize];
}
template <typename dense_t>
inline ecs::sparse_set<dense_t>::sparse_set(std::size_t capacity, std::uint32_t pageSize) : mPageSize(pageSize)
{
    ECS_PROFILE;
    reserve(capacity);
}
template <typename dense_t>
inline ecs::sparse_set<dense_t>::sparse_set(sparse_set const &other)
{
    ECS_PROFILE;
    *this = other;
}
template <typename dense_t>
inline ecs::sparse_set<dense_t>::sparse_set(sparse_set &&other) noexcept
{
    *this = std::move(other);
}
template <typename dense_t>
inline ecs::sparse_set<dense_t> &ecs::sparse_set<dense_t>::operator=(sparse_set const &other)
{
    ECS_PROFILE;
    mDense = other.mDense;
    mDenseToSparse = other.mDenseToSparse;
    
    mSparse.clear();
    for(auto const &otherPage : other.mSparse)
    {
        if(!otherPage.empty())
            mSparse.emplace_back(otherPage);
        else
            mSparse.emplace_back();
    }

    return *this;
}
template <typename dense_t>
inline ecs::sparse_set<dense_t> &ecs::sparse_set<dense_t>::operator=(sparse_set &&other) noexcept
{
    ECS_PROFILE;
    std::swap(mDense, other.mDense);
    std::swap(mDenseToSparse, other.mDenseToSparse);
    std::swap(mSparse, other.mSparse);
    
    return *this;
}
template <typename dense_t>
template <class... Args>
inline void ecs::sparse_set<dense_t>::emplace(sparse_type const &sparse, Args &&...args)
{
    ECS_PROFILE;
    ECS_ASSERT(getDenseIndex(sparse) == null, "Element added to the same sparse index more than once");

    if constexpr(std::is_aggregate_v<dense_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<dense_type>)) 
        mDense.emplace_back(dense_type{std::forward<Args>(args)...});
    else 
        mDense.emplace_back(std::forward<Args>(args)...);

    std::size_t index = mDense.size() - 1;
    setDenseIndex(sparse, index);
    mDenseToSparse.emplace_back(sparse);
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::erase(sparse_type const &sparse)
{
    ECS_PROFILE;
    ECS_ASSERT(getDenseIndex(sparse) != null, "Removing a non-existing element from a sparse index");

    std::size_t removedDenseIndex = getDenseIndex(sparse);
    std::size_t lastDenseIndex = mDense.size() - 1;

    if(removedDenseIndex != lastDenseIndex)
    {
        sparse_type lastSparseIndex = mDenseToSparse[lastDenseIndex];
        setDenseIndex(lastSparseIndex, removedDenseIndex);

        mDenseToSparse[removedDenseIndex] = lastSparseIndex;
        mDense[removedDenseIndex] = std::move(mDense[lastDenseIndex]);
    }

    setDenseIndex(sparse, null);

    mDense.pop_back();
    mDenseToSparse.pop_back();
}
template <typename dense_t>
inline bool ecs::sparse_set<dense_t>::contains(sparse_type const &sparse) const
{
    ECS_PROFILE;
    return getDenseIndex(sparse) != null;
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::reserve(std::size_t newCapacity)
{
    ECS_PROFILE;
    mDense.reserve(newCapacity);
    mDenseToSparse.reserve(newCapacity);
    mSparse.reserve((newCapacity + mPageSize - 1) / mPageSize);
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::shrink_to_fit()
{
    ECS_PROFILE;
    mDense.shrink_to_fit();
    mDenseToSparse.shrink_to_fit();

    auto maxIterator = std::max_element(mDenseToSparse.begin(), mDenseToSparse.end());
    sparse_type maxSparse = maxIterator != mDenseToSparse.end() ? *maxIterator + 1 : 0;
    mSparse.resize((maxSparse + mPageSize - 1) / mPageSize);
    mSparse.shrink_to_fit();
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::dense_type const &ecs::sparse_set<dense_t>::get(sparse_type const &sparse) const
{
    ECS_PROFILE;
    ECS_ASSERT(getDenseIndex(sparse) != null, "Getting a non-existing element from a sparse index");

    return mDense[getDenseIndex(sparse)];
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::dense_type &ecs::sparse_set<dense_t>::get(sparse_type const &sparse)
{
    ECS_PROFILE;
    ECS_ASSERT(getDenseIndex(sparse) != null, "Getting a non-existing element from a sparse index");

    return mDense[getDenseIndex(sparse)];
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::dense_type &ecs::sparse_set<dense_t>::operator[](sparse_type const &sparse)
{
    if(!contains(sparse))
        emplace(sparse);
    return get(sparse);
}
template <typename dense_t>
inline std::vector<typename ecs::sparse_set<dense_t>::dense_type> const &ecs::sparse_set<dense_t>::dense() const
{
    return mDense;
}
template <typename dense_t>
inline std::vector<typename ecs::sparse_set<dense_t>::sparse_type> const &ecs::sparse_set<dense_t>::sparse() const
{
    return mDenseToSparse;
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::clear()
{
    ECS_PROFILE;
    mDense.clear();
    mSparse.clear();
    mDenseToSparse.clear();
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::const_iterator ecs::sparse_set<dense_t>::begin() const
{
    return {this, 0};
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::const_iterator ecs::sparse_set<dense_t>::end() const
{
    return {this, mDense.size()};
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::iterator ecs::sparse_set<dense_t>::begin()
{
    return {this, 0};
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::iterator ecs::sparse_set<dense_t>::end()
{
    return {this, mDense.size()};
}
template <typename dense_t>
inline bool ecs::sparse_set<dense_t>::empty() const
{
    return mDense.empty();
}
template <typename dense_t>
inline std::size_t ecs::sparse_set<dense_t>::size() const
{
    return mDense.size();
}
template <typename dense_t>
inline dense_t *ecs::sparse_set<dense_t>::denseData()
{
    return mDense.data();
}
template <typename dense_t>
inline dense_t const *ecs::sparse_set<dense_t>::denseData() const
{
    return mDense.data();
}