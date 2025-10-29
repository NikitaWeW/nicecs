/*
      ___  ___ ___ 
     / _ \/ __/ __|        Copyright (c) 2024 Nikita Martynau 
    |  __/ (__\__ \        https://opensource.org/license/mit 
     \___|\___|___/ v1.5.1 https://github.com/nikitawew/nicecs

Thanks to this article: https://austinmorlan.com/posts/entity_component_system.
Took a bit of inspiration from https://github.com/skypjack/entt.
*/

/*
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <cstdint>
#include <bitset>
#include <queue>
#include <memory>
#include <vector>
#include <type_traits>
#include <stdexcept>
#include <limits>
#include <unordered_map>
#include <algorithm>

/*! \cond Doxygen_Suppress */
// Config section 

#include <cassert>

#ifndef ECS_PROFILE
#define ECS_PROFILE()
#endif

#ifndef ECS_ASSERT
#define ECS_ASSERT(x, msg) assert((x) && msg)
#endif

#ifndef ECS_THROW
#define ECS_THROW(x) (throw (x))
#endif

#ifndef ECS_SPARSE_SET_THROW
/**
 * ecs::sparse_set should not throw (error checking handled by ecs::registry).
 * Define before including the library or change in file when using ecs::sparse_set separately.
 */
#define ECS_SPARSE_SET_THROW(x) ECS_ASSERT(false, #x)
#endif

/*! \endcond */

// ============
// Declaration 
// ============

namespace ecs
{
    /** \brief Entity ID. Entity 0 is invalid. */
    using entity = std::uint32_t;
    /** \brief Component ID. Used with signature. */
    using component_id = std::uint16_t;

    /** \brief Controls the maximum number of registered components allowed to be registered. */
    constexpr component_id MAX_COMPONENTS = 1024;

    /**
     * \brief Used to track which components entity has. 
     * As an example, if Transform has type 0, RigidBody has type 1, and Gravity has type 2, an entity that “has” those three components would have a signature of 0b111 (bits 0, 1, and 2 are set).
     * Important: signatures are uniform across registries.
     * TODO: If MAX_COMPONENTS becomes an issue, use std::vector<bool>
     */
    using signature = std::bitset<MAX_COMPONENTS>;

    /**
     * \brief A sparse set implementation.
     * \tparam dense_t The type of densely stored data.
     */
    template<typename dense_t>
    class sparse_set
    {
    public:
        /** \brief The type of the sparse index. */
        using sparse_type = std::size_t;
        /** \brief The type of densely stored data. */
        using dense_type = dense_t;

        class iterator;
        class const_iterator;

        /** \brief The sparse pointer that represents the empty index. */
        static constexpr sparse_type null = std::numeric_limits<std::size_t>::max();
    private:
        std::vector<dense_type> m_dense;
        std::vector<sparse_type> m_denseToSparse;
        std::vector<std::size_t> m_sparse;

        void setDenseIndex(sparse_type const &sparse, std::size_t index);
        std::size_t getDenseIndex(sparse_type const &sparse) const;
    public:
        /** 
         * \brief The constructor.
         * \param capacity The optional capacity to reserve.
         * */
        sparse_set(std::size_t capacity = 1000);
        /** \brief The destructor */
        ~sparse_set() = default;
        /** \brief The copy constructor */
        sparse_set(sparse_set const &) = default;
        /** \brief The move constructor */
        sparse_set(sparse_set &&) noexcept = default;
        /** \brief The copy assignment operator */
        sparse_set &operator=(sparse_set const &) = default;
        /** \brief The move assignment operator */
        sparse_set &operator=(sparse_set &&) noexcept = default;

        /**
         * \brief The container is extended by inserting a new element at sparse position. This new element is constructed in place using args as the arguments for its construction.
         * \param sparse A sparse index.
         * \param args Arguments forwarded to construct the new element.
         * \throws std::invalid_argument If the sparse index already contains an element.
         */
        template <class... Args>
        void emplace(sparse_type const &sparse, Args&&... args);

        /**
         * \brief Removes an element from a sparse index.
         * \param sparse A sparse index.
         * \throws std::out_of_range If a sparse index doesent contain the element.
         */
        void erase(sparse_type const &sparse);

        /**
         * \brief Gets an element at a sparse index.
         * \param sparse A sparse index.
         * \return An element lvalue reference.
         * \throws std::out_of_range If a sparse index doesent contain the element.
         */
        dense_type const &get(sparse_type const &sparse) const;
        /** \copydoc get */
        dense_type &get(sparse_type const &sparse);

        /** \copydoc get */
        dense_type &operator[](sparse_type const &sparse);
        /** \copydoc get */
        dense_type const &operator[](sparse_type const &sparse) const;
        /**
         * \brief Gets the dense list.
         * \return A std::vector with the elements.
         */
        std::vector<dense_type> const &data() const;

        /**
         * \brief Get dense to sparse mapping.
         * \return 1 to 1 with the dense data vector with the dense to sparse mapping.
         */
        std::vector<sparse_type> const &getDenseToSparse() const;

        /**
         * \brief Get the sparse pages.
         * \return The paginated vector with the sparse indices. The ones that are equal to null are null pointers.
         */
        std::vector<std::size_t> const &sparseData() const;

        /**
         * \brief Check whether the sparse set contains an element at a given sparse index.
         * \param sparse A sparse index.
         * \return True if found, false otherwise.
         */
        bool contains(sparse_type const &sparse) const;

        /**
         * \brief Increase the capacity of the dense list.
         */
        void reserve(std::size_t newCapacity);

        /**
         * \brief Sets the capacity to the size.
         */
        void shrink_to_fit();

        /**
         * \return True if the container is empty, false otherwise.
         */
        bool empty() const;

        /**
         * \brief Clear the sparse set.
         */
        void clear();

        /** \brief The cbegin of the dense list. */
        const_iterator begin() const;
        /** \brief The cend of the dense list. */
        const_iterator end() const;
        /** \brief The begin of the dense list. */
        iterator begin();
        /** \brief The end of the dense list. */
        iterator end();

    public:
        /** \brief The [sparse; dense] pair iterator */
        class iterator
        {
            sparse_set *m_owner;
            std::size_t m_index;
            struct proxy 
            {
                sparse_type first;
                dense_type *second;
                proxy(sparse_type f, dense_type *s) : first(f), second(s) {}
                std::pair<sparse_type, dense_type&> operator*() const { return {first, *second}; }
            };
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::pair<sparse_type, dense_type&>;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = value_type;

            inline iterator() : m_owner(nullptr), m_index(0) {}
            inline iterator(sparse_set *owner, std::size_t index) : m_owner(owner), m_index(index) {}

            inline reference operator*() const {
                return {m_owner->m_denseToSparse[m_index], m_owner->m_dense[m_index]};
            }

            inline iterator &operator++() { ++m_index; return *this; }
            inline iterator operator++(int) { iterator tmp = *this; ++m_index; return tmp; }

            inline iterator &operator--() { --m_index; return *this; }
            inline iterator operator--(int) { iterator tmp = *this; --m_index; return tmp; }

            inline iterator &operator+=(difference_type n) { m_index += n; return *this; }
            inline iterator operator+(difference_type n) const { return iterator(m_owner, m_index + n); }
            inline iterator &operator-=(difference_type n) { m_index -= n; return *this; }
            inline iterator operator-(difference_type n) const { return iterator(m_owner, m_index - n); }

            inline difference_type operator-(iterator const &other) const { return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index); }

            inline bool operator==(iterator const &o) const { return m_owner == o.m_owner && m_index == o.m_index; }
            inline bool operator!=(iterator const &o) const { return !(*this == o); }

            inline reference operator[](difference_type n) const { return *(*this + n); }
        };
        class const_iterator
        {
            sparse_set const *m_owner;
            std::size_t m_index;
            struct proxy 
            {
                sparse_type first;
                dense_type const *second;
                proxy(sparse_type f, dense_type const *s) : first(f), second(s) {}
                std::pair<sparse_type, dense_type&> operator*() const { return {first, *second}; }
            };
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::pair<sparse_type, dense_type const &>;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = value_type;

            inline const_iterator() : m_owner(nullptr), m_index(0) {}
            inline const_iterator(sparse_set const *owner, std::size_t index) : m_owner(owner), m_index(index) {}

            inline reference operator*() const {
                return {m_owner->m_denseToSparse[m_index], m_owner->m_dense[m_index]};
            }

            inline const_iterator &operator++() { ++m_index; return *this; }
            inline const_iterator operator++(int) { const_iterator tmp = *this; ++m_index; return tmp; }

            inline const_iterator &operator--() { --m_index; return *this; }
            inline const_iterator operator--(int) { const_iterator tmp = *this; --m_index; return tmp; }

            inline const_iterator &operator+=(difference_type n) { m_index += n; return *this; }
            inline const_iterator operator+(difference_type n) const { return const_iterator(m_owner, m_index + n); }
            inline const_iterator &operator-=(difference_type n) { m_index -= n; return *this; }
            inline const_iterator operator-(difference_type n) const { return const_iterator(m_owner, m_index - n); }

            inline difference_type operator-(const_iterator const &other) const { return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index); }

            inline bool operator==(const_iterator const &o) const { return m_owner == o.m_owner && m_index == o.m_index; }
            inline bool operator!=(const_iterator const &o) const { return !(*this == o); }

            inline reference operator[](difference_type n) const { return *(*this + n); }
        };
    };

    /**
     * \brief Manages entities (create, destroy) and their signatures (set, get).
     * Any entity supplied to the manager must be created by the same manager object.
     */
    class entity_manager
    {
    private:
        std::queue<entity> m_availableEntityIDs;
        std::unordered_map<signature, sparse_set<entity>> m_entityGroups;
        std::uint32_t m_livingEntitiesCount = 0;
        sparse_set<signature> m_signatures;
        entity m_nextID = 1;
    public:
        explicit entity_manager(entity numEntities = 100000);
        ~entity_manager() = default;
        /**
         * \brief Creates entity with an optional signature.
         * \param signature A signature representing components the entity has (optional).
         * \return Unique entity id.
         */
        entity createEntity(signature signature = {});
        /**
         * \brief Destroys entity.
         * \param entity A valid entity identifier.
         */
        void destroyEntity(entity const &entity);
        /**
         * \brief Sets the signature of the entity.
         * \param entity A valid entity identifier.
         * \param signature A new signature,
         */
        void setSignature(entity const &entity, signature signature);

        /**
         * \brief Gets the signature of a valid entity.
         * \param entity A valid entity identifier.
         * \return A const reference to a signature, describing the components an entity has.
         */
        signature const &getSignature(entity const &entity) const;

        /**
         * \brief Get entities of this manager.
         * \return Get a map of the sparse sets of all the valid entities of this registry with their signatures as its key.
         */
        std::unordered_map<signature, sparse_set<entity>> const &getEntityGroups() const;

        /**
         * \brief Checks if an identifier refers to a valid entity.
         * \param entity An identifier, either valid or not.
         * \return True if the identifier is valid, false otherwise.
         */
        bool valid(entity const &entity) const;

        /**
         * \brief Get the number of entities alive in a manager.
         */
        std::size_t size() const;
    };

    /**
     * \brief Every instanced component_array is derived from this polymorphic class.
     */
    class icomponent_array 
    {
    public:
        virtual ~icomponent_array() = default;

        /**
         * \brief Notify the array that the entity is destroyed.
         * \param entity A destroyed entity identifier.
         */
        virtual void onEntityDestroyed(entity const &entity) = 0;

        /**
         * \brief Copy the component of an entity from another component array.
         * \param other The other component array.
         * \param to The entity to copy to.
         * \param from The entity to copy from.
         */
        virtual void copyEntityFrom(icomponent_array const *other, entity const &to, entity const &from) = 0;

        /**
         * \brief Add an entity to the component array.
         * \param entity A valid entity identifier.
         */
        virtual void addEntity(entity const &entity) = 0;

        /**
         * \brief Create an empty copy of the component array.
         */
        virtual std::unique_ptr<icomponent_array> cloneEmpty() const = 0;

        /**
         * \brief Create a copy of the component array.
         */
        virtual std::unique_ptr<icomponent_array> clone() const = 0;
    };

    /**
     * \brief Stores components of entities of a specific type as a sparse set.
     * @tparam component_t The type of stored components.
     */
    template <typename component_t>
    class component_array : public icomponent_array, public sparse_set<component_t>
    {
    public:
        /** \copydoc ecs::icomponent_array::onEntityDestroyed */
        void onEntityDestroyed(entity const &entity) override;
        
        
        /** \copydoc ecs::icomponent_array::copyEntityFrom */
        void copyEntityFrom(icomponent_array const *other, entity const &to, entity const &from) override;

        /** \copydoc ecs::icomponent_array::addEntity */
        void addEntity(entity const &entity) override;

        /** \copydoc ecs::icomponent_array::cloneEmpty */
        std::unique_ptr<icomponent_array> cloneEmpty() const override;

        /** \copydoc ecs::icomponent_array::clone */
        std::unique_ptr<icomponent_array> clone() const override;
    };

    /**
     * \brief Manages components and their arrays. All components are destroyed automatically.
     */
    class component_manager
    {
    private:
        sparse_set<std::unique_ptr<icomponent_array>> m_componentArrays{};
        inline static component_id m_nextID = 0;
    public:
        /**
         * \brief Get unique component ID used to index the signature bitset.
         * \tparam component_t The component type.
         * The id is the same between the managers.
         */
        template <typename component_t> 
        static component_id getComponentID();

        /**
         * \brief Get the next component id.
         */
        static std::size_t getNextID();
    public:
        component_manager() = default;
        component_manager(component_manager const &other);
        component_manager &operator=(component_manager const &other);
        ~component_manager() = default;

        /**
         * \brief Registers component.
         * \tparam component_t The component type.
         * This should be called for every component used. Multiple calls for the same component_t will do nothing.
         */
        template <typename component_t> 
        void registerComponent();

        /**
         * \brief Notify component arrays that the entity is destroyed.
         * \param entity A deleted entity identifier.
         */
        void entityDestroyed(entity const &entity) const;

        /**
         * \brief Get the component array associated with the given component type.
         * \tparam component_t The component type.
         */
        template <typename component_t> 
        component_array<component_t> *getComponentArray();
        /** \copydoc getComponentArray */
        template <typename component_t>
        component_array<component_t> const *getComponentArray() const;

        /**
         * \brief Get the component arrays.
         */
        sparse_set<std::unique_ptr<icomponent_array>> &getComponentArrays();
        /** \copydoc getComponentArrays */
        sparse_set<std::unique_ptr<icomponent_array>> const &getComponentArrays() const;
    };

    /**
     * A class to push around lists of types.
     * \brief Exclusion type list.
     * \tparam Type List of types.
     */
    template<typename... Type>
    struct exclude_t 
    {
        /*! \brief Default constructor. */
        explicit constexpr exclude_t() = default;
    };

    /**
     * \brief An ECS interface.
     * Contains entities and their components.
     */
    class registry
    {
    private:
        entity_manager m_entityManager;
        // ugly fix for lazy component registration.
        mutable component_manager m_componentManager;
    public:
        /** \copydoc entity_manager::valid */
        bool valid(entity const &entity) const;

        /**
         * \brief Checks if a valid entity has a component.
         * \param entity A valid entity identifier.
         * \tparam component_t The component type.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \return True if the entity has the component, false otherwise.
         */
        template <typename component_t> bool has(entity const &entity) const;

        /**
         * \brief Gets a component from a a valid entity.
         * \param entity A valid entity identifier.
         * \tparam component_t The component type.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::out_of_range if the component is not added.
         * \return The component lvalue reference.
         */
        template <typename component_t> 
        component_t &get(entity const &entity);
        /** \copydoc get */
        template <typename component_t> 
        component_t const &get(entity const &entity) const;

        /**
         * \brief Removes a component from a valid entity.
         * \param entity A valid entity identifier.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::out_of_range if the component is not added.
         * \tparam component_t The component type.
         */
        template <typename component_t> 
        void remove(entity const &entity);
        
        /**
         * \copydoc component_manager::emplace
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::invalid_argument if the component is already added.
         */
        template <typename component_t, class... Args>
        void emplace(entity const &entity, Args&&... args);

        /**
         * \brief Create an entity.
         * \tparam Components_t Components (optional).
         * \return Unique valid entity id.
         */
        template <typename... Components_t> 
        entity create();

        /**
         * \brief Create an entity.
         * \tparam Components_t Components (optional).
         * \param components The components to move in.
         * \return Unique valid entity id.
         */
        template <typename... Components_t> 
        entity create(Components_t&&... components);

        /**
         * \brief Destroys an entity and its components.
         * \param entity A valid entity identifier.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         */
        void destroy(entity const &entity);

        /**
         * \brief Check if an entity has no components.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \return True if the entity is empty, false otherwise.
         */
        bool empty(entity const &entity) const;

        /**
         * \brief Get the number of components in an entity.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         */
        std::size_t size(entity const &entity) const;

        /**
         * \brief Get the number of entities in a registry.
         */
        std::size_t size() const;

        /**
         * \brief Returns a view for the given elements.
         * \tparam Include Types of included elements used to construct the view.
         * \tparam Exclude Types of elements used to filter the view.
         * \param toExclude The type list used to deduce Exclude variadic template argument.
         * \return A view on entities that are valid at the time of calling this member function and contain given included components and do not contain excluded ones at the time of calling this member function.
         * view<>() returns all the entities in the registry.
         */
        template<typename... Include, typename... Exclude>
        std::vector<entity> view(exclude_t<Exclude...> toExclude = exclude_t{}) const;

        /**
         * \brief Returns a view for the given elements.
         * \tparam May Types of elements that entity may have used to construct the view.
         * \tparam Exclude Types of elements used to filter the view.
         * \param toExclude The type list used to deduce Exclude variadic template argument.
         * \return A view on entities that are valid at the time of calling this member function and contain given included components and do not contain excluded ones at the time of calling this member function.
         * view_any_of<>() returns none of the entities in the registry.
         */
        template<typename... May, typename... Exclude>
        std::vector<entity> view_any_of(exclude_t<Exclude...> toExclude = exclude_t{}) const;

        /**
         * \brief Get a registry that merges two registries.
         * \param other The second registry.
         * \return Merged registry containing entities and components from both registries.
         */
        registry merged(registry const &other) const;

        /**
         * \brief Merge with the other registry.
         * \param other The registry to merge with.
         * Adds the entities and their components from the other registry to this registry.
         */
        void merge(registry const &other);

        /**
         * \brief Check if the entities from different registries have same components.
         * \param first,second The entities to compare.
         * \param secondRegistry The registry \p second entity belongs to.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \return True if the signature of the first entity equals to the signature of the second, false otherwise.
         */
        bool same(entity const &first, entity const &second, registry const &secondRegistry);

        /**
         * \brief Check if the entities from the same registry have same components.
         * \param first,second The entities to compare.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \return True if the signature of the first entity equals to the signature of the second, false otherwise.
         */
        bool same(entity const &first, entity const &second);
    private:
        entity create(signature signature);
        std::vector<entity> view(signature required, signature excluded) const;
        std::vector<entity> view_any_of(signature mayHave, signature excluded) const;
    };

    /**
     * \brief Construct a signature from the component type list.
     * \tparam Components_t The variadic component type list.
     * \return The signature with all the required components set.
     */
    template <typename... Components_t>
    inline signature make_signature();
} // namespace ecs

// ===============
// Implementation
// ===============

/*! \cond Doxygen_Suppress */

template <typename dense_t>
inline void ecs::sparse_set<dense_t>::setDenseIndex(sparse_type const &sparse, std::size_t index)
{
    ECS_PROFILE();
    if(sparse < 0) // In case i would change the sparse_type to signed.
        return;

    if(sparse >= m_sparse.size())
        m_sparse.resize(sparse + 1, null);

    m_sparse[sparse] = index;
}
template <typename dense_t>
inline std::size_t ecs::sparse_set<dense_t>::getDenseIndex(sparse_type const &sparse) const
{
    ECS_PROFILE();
    // Check sparse < 0 if sparse_type becomes a template parameter again.
    // if(sparse < 0) 
    //     return null;
    if(sparse >= m_sparse.size()) 
        return null;

    return m_sparse[sparse];
}
template <typename dense_t>
inline ecs::sparse_set<dense_t>::sparse_set(std::size_t capacity)
{
    ECS_PROFILE();
    reserve(capacity);
}
template <typename dense_t>
template <class... Args>
inline void ecs::sparse_set<dense_t>::emplace(sparse_type const &sparse, Args &&...args)
{
    ECS_PROFILE();
    if(contains(sparse))
        ECS_SPARSE_SET_THROW(std::invalid_argument{"element added to the same sparse index more than once!"});

    if constexpr(std::is_aggregate_v<dense_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<dense_type>)) 
        m_dense.emplace_back(dense_type{std::forward<Args>(args)...});
    else 
        m_dense.emplace_back(std::forward<Args>(args)...);

    std::size_t index = m_dense.size() - 1;
    setDenseIndex(sparse, index);
    m_denseToSparse.emplace_back(sparse);
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::erase(sparse_type const &sparse)
{
    ECS_PROFILE();
    if(!contains(sparse))
        ECS_SPARSE_SET_THROW(std::out_of_range{"removing a non-existing element from a sparse index!"});

    std::size_t removedDenseIndex = getDenseIndex(sparse);
    std::size_t lastDenseIndex = m_dense.size() - 1;

    if(removedDenseIndex != lastDenseIndex)
    {
        sparse_type lastSparseIndex = m_denseToSparse[lastDenseIndex];
        setDenseIndex(lastSparseIndex, removedDenseIndex);

        m_denseToSparse[removedDenseIndex] = lastSparseIndex;
        
        m_dense[removedDenseIndex] = std::move(m_dense[lastDenseIndex]);
    }

    setDenseIndex(sparse, null);

    m_dense.pop_back();
    m_denseToSparse.pop_back();
}
template <typename dense_t>
inline bool ecs::sparse_set<dense_t>::contains(sparse_type const &sparse) const
{
    ECS_PROFILE();
    return getDenseIndex(sparse) != null;
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::reserve(std::size_t newCapacity)
{
    ECS_PROFILE();
    m_dense.reserve(newCapacity);
    m_denseToSparse.reserve(newCapacity);
    m_sparse.reserve(newCapacity);
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::shrink_to_fit()
{
    ECS_PROFILE();
    m_dense.shrink_to_fit();
    m_denseToSparse.shrink_to_fit();

    auto maxIterator = std::max_element(m_denseToSparse.begin(), m_denseToSparse.end());
    sparse_type maxIndex = maxIterator != m_denseToSparse.end() ? *maxIterator + 1 : 0;
    m_sparse.resize(maxIndex);
    m_sparse.shrink_to_fit();
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::dense_type const &ecs::sparse_set<dense_t>::get(sparse_type const &sparse) const
{
    ECS_PROFILE();
    if(!contains(sparse))
        ECS_SPARSE_SET_THROW(std::out_of_range{"getting a non-existing element from a sparse index!"});

    return m_dense[getDenseIndex(sparse)];
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::dense_type &ecs::sparse_set<dense_t>::get(sparse_type const &sparse)
{
    ECS_PROFILE();
    if(!contains(sparse))
        ECS_SPARSE_SET_THROW(std::out_of_range{"getting a non-existing element from a sparse index!"});

    return m_dense[getDenseIndex(sparse)];
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::dense_type &ecs::sparse_set<dense_t>::operator[](sparse_type const &sparse)
{
    return get(sparse);
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::dense_type const &ecs::sparse_set<dense_t>::operator[](sparse_type const &sparse) const
{
    return get(sparse);
}
template <typename dense_t>
inline std::vector<typename ecs::sparse_set<dense_t>::dense_type> const &ecs::sparse_set<dense_t>::data() const
{
    return m_dense;
}
template <typename dense_t>
inline std::vector<typename ecs::sparse_set<dense_t>::sparse_type> const &ecs::sparse_set<dense_t>::getDenseToSparse() const
{
    return m_denseToSparse;
}
template <typename dense_t>
inline std::vector<std::size_t> const &ecs::sparse_set<dense_t>::sparseData() const
{
    return m_sparse;
}
template <typename dense_t>
inline void ecs::sparse_set<dense_t>::clear()
{
    ECS_PROFILE();
    m_dense.clear();
    m_sparse.clear();
    m_denseToSparse.clear();
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::const_iterator ecs::sparse_set<dense_t>::begin() const
{
    return {this, 0};
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::const_iterator ecs::sparse_set<dense_t>::end() const
{
    return {this, m_dense.size()};
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::iterator ecs::sparse_set<dense_t>::begin()
{
    return {this, 0};
}
template <typename dense_t>
inline typename ecs::sparse_set<dense_t>::iterator ecs::sparse_set<dense_t>::end()
{
    return {this, m_dense.size()};
}
template <typename dense_t>
inline bool ecs::sparse_set<dense_t>::empty() const
{
    return m_dense.empty();
}

inline ecs::entity_manager::entity_manager(ecs::entity numEntities)
{
    ECS_PROFILE();
    for(; m_nextID <= numEntities; ++m_nextID) {
        m_availableEntityIDs.push(m_nextID);
    }
    m_signatures.reserve(numEntities);
}
inline ecs::entity ecs::entity_manager::createEntity(signature signature)
{
    ECS_PROFILE();

    entity entity = m_availableEntityIDs.front();
    m_availableEntityIDs.pop();
    ++m_livingEntitiesCount;
    m_entityGroups[signature].emplace(entity, entity);

    if(m_availableEntityIDs.empty())
        m_availableEntityIDs.push(m_nextID++);

    m_signatures.emplace(entity, signature);

    return entity;
}
inline void ecs::entity_manager::destroyEntity(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    --m_livingEntitiesCount;
    m_availableEntityIDs.push(entity);

    auto &group = m_entityGroups[getSignature(entity)];
    group.erase(entity);
    if(group.empty())
        m_entityGroups.erase(getSignature(entity));

    m_signatures.erase(entity);
}
inline void ecs::entity_manager::setSignature(entity const &entity, signature signature)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");

    auto &group = m_entityGroups[getSignature(entity)];
    group.erase(entity);
    if(group.empty())
        m_entityGroups.erase(getSignature(entity));

    m_signatures[entity] = signature;

    m_entityGroups[signature].emplace(entity, entity);

}
inline ecs::signature const &ecs::entity_manager::getSignature(entity const &entity) const
{
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    
    return m_signatures[entity];
}
inline bool ecs::entity_manager::valid(entity const &entity) const
{
    ECS_PROFILE();
    return 1 <= entity && entity < m_nextID && m_signatures.contains(entity);
}
inline std::size_t ecs::entity_manager::size() const
{
    return m_livingEntitiesCount;
}
inline std::unordered_map<ecs::signature, ecs::sparse_set<ecs::entity>> const &ecs::entity_manager::getEntityGroups() const
{
    return m_entityGroups;
} 

template <typename component_t>
inline void ecs::component_array<component_t>::onEntityDestroyed(entity const &entity)
{
    ECS_PROFILE();
    if(this->contains(entity))
        this->erase(entity);
}
template <typename component_t>
inline void ecs::component_array<component_t>::copyEntityFrom(icomponent_array const *other, entity const &to, entity const &from)
{
    ECS_ASSERT(other, "");
    ecs::component_array<component_t> const *otherArray = static_cast<ecs::component_array<component_t> const *>(other);
    ECS_ASSERT(this->contains(to) && otherArray->contains(from), "");
    this->get(to) = otherArray->get(from);
}
template <typename component_t>
inline void ecs::component_array<component_t>::addEntity(entity const &entity)
{
    ECS_ASSERT(!this->contains(entity), "Component array already has the entity");
    this->emplace(entity);
}
template <typename component_t>
inline std::unique_ptr<ecs::icomponent_array> ecs::component_array<component_t>::cloneEmpty() const
{
    return std::unique_ptr<ecs::icomponent_array>{new ecs::component_array<component_t>{}};
}
template <typename component_t>
inline std::unique_ptr<ecs::icomponent_array> ecs::component_array<component_t>::clone() const
{
    return std::unique_ptr<ecs::icomponent_array>{new ecs::component_array<component_t>{*this}};
}

template <typename component_t>
inline void ecs::component_manager::registerComponent()
{
    ECS_PROFILE();
    auto id = getComponentID<component_t>();
    if(!m_componentArrays.contains(id))
        m_componentArrays.emplace(id, std::make_unique<component_array<component_t>>());
    // else
    //     ECS_ASSERT(false, "component already registered!"); // Silently ignore
}
template <typename component_t>
inline ecs::component_id ecs::component_manager::getComponentID()
{
    ECS_PROFILE();
    ECS_ASSERT(m_nextID < MAX_COMPONENTS, "too many components registered!");
    static const component_id id = m_nextID++;
    return id;
}
template <typename component_t>
inline ecs::component_array<component_t> *ecs::component_manager::getComponentArray()
{
    ECS_PROFILE();
    auto id = getComponentID<component_t>();
    ECS_ASSERT(m_componentArrays.contains(id), "component not registered before use");
    return static_cast<component_array<component_t> *>(m_componentArrays.get(id).get());
}
template <typename component_t>
inline ecs::component_array<component_t> const *ecs::component_manager::getComponentArray() const
{
    ECS_PROFILE();
    auto id = getComponentID<component_t>();
    ECS_ASSERT(m_componentArrays.contains(id), "component not registered before use");
    return static_cast<component_array<component_t> const *>(m_componentArrays.get(id).get());
}
inline std::size_t ecs::component_manager::getNextID()
{
    return m_nextID;
}
inline ecs::component_manager::component_manager(component_manager const &other)
{
    this->operator=(other);
}
inline ecs::component_manager &ecs::component_manager::operator=(component_manager const &other)
{
    for(auto [id, ptr] : other.m_componentArrays)
    {
        m_componentArrays.emplace(id, ptr->clone());
    }

    return *this;
}
inline void ecs::component_manager::entityDestroyed(entity const &entity) const
{
    ECS_PROFILE();
    for(auto &componentArray : m_componentArrays.data()) {
        componentArray->onEntityDestroyed(entity);
    }
}
inline ecs::sparse_set<std::unique_ptr<ecs::icomponent_array>> &ecs::component_manager::getComponentArrays()
{
    return m_componentArrays;
}
inline ecs::sparse_set<std::unique_ptr<ecs::icomponent_array>> const &ecs::component_manager::getComponentArrays() const
{
    return m_componentArrays;
}

template <typename component_t>
inline bool ecs::registry::has(entity const &entity) const
{ 
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    
    m_componentManager.registerComponent<component_t>();
    return m_entityManager.getSignature(entity).test(component_manager::getComponentID<component_t>()); 
}
template <typename component_t>
inline component_t &ecs::registry::get(entity const &entity) 
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(!has<component_t>(entity)) 
        ECS_THROW(std::out_of_range{"component to get is not added!"});
    
    return m_componentManager.getComponentArray<component_t>()->get(entity);
}
template <typename component_t>
inline component_t const &ecs::registry::get(entity const &entity) const
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(!has<component_t>(entity)) 
        ECS_THROW(std::out_of_range{"component to get is not added!"});
    
    return m_componentManager.getComponentArray<component_t>()->get(entity);
}
inline ecs::entity ecs::registry::create(signature signature)
{
    entity entity = m_entityManager.createEntity(signature);
    for(std::size_t id = 0; id < component_manager::getNextID(); ++id)
    {
        if(signature.test(id))
        {
            ECS_ASSERT(m_componentManager.getComponentArrays().contains(id), "unregistered component");
            m_componentManager.getComponentArrays().get(id)->addEntity(entity);
        }
    }

    return entity;
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create()
{
    ECS_PROFILE();
    
    (m_componentManager.registerComponent<Components_t>(), ...);
    signature signature;
    (signature.set(component_manager::getComponentID<Components_t>()), ...);

    entity entity = m_entityManager.createEntity(signature);

    (m_componentManager.getComponentArray<Components_t>()->emplace(entity), ...);

    return entity;
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create(Components_t &&...components)
{
    ECS_PROFILE();
    
    (m_componentManager.registerComponent<Components_t>(), ...);
    signature signature;
    (signature.set(component_manager::getComponentID<Components_t>()), ...);

    entity entity = m_entityManager.createEntity(signature);

    (m_componentManager.getComponentArray<Components_t>()->emplace(entity, std::forward<Components_t>(components)), ...);

    return entity;
}
template <typename component_t>
inline void ecs::registry::remove(entity const &entity)
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(!has<component_t>(entity)) 
        ECS_THROW(std::out_of_range{"component to remove is not added!"});
    
    m_entityManager.setSignature(entity, signature{m_entityManager.getSignature(entity)}.set(component_manager::getComponentID<component_t>(), false));
    m_componentManager.getComponentArray<component_t>()->erase(entity);
}
template <typename component_t, class... Args>
inline void ecs::registry::emplace(entity const &entity, Args&&... args)
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(has<component_t>(entity)) 
        ECS_THROW(std::invalid_argument{"component to emplace already added!"});

    m_componentManager.getComponentArray<component_t>()->emplace(entity, std::forward<Args>(args)...);
    m_entityManager.setSignature(entity, signature{m_entityManager.getSignature(entity)}.set(component_manager::getComponentID<component_t>(), true));
}
inline bool ecs::registry::valid(entity const &entity) const
{
    return m_entityManager.valid(entity);
}
inline void ecs::registry::destroy(ecs::entity const &entity)
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});

    m_entityManager.destroyEntity(entity);
    m_componentManager.entityDestroyed(entity);
}
inline bool ecs::registry::empty(entity const &entity) const
{
    ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    return m_entityManager.getSignature(entity).none();
}
inline std::size_t ecs::registry::size(entity const &entity) const
{
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    return m_entityManager.getSignature(entity).count();
}
inline std::size_t ecs::registry::size() const
{
    return m_entityManager.size();
}
inline ecs::registry ecs::registry::merged(ecs::registry const &other) const
{
    ECS_PROFILE();
    ecs::registry result;
    result.merge(*this);
    result.merge(other);
    return result;
}
inline void ecs::registry::merge(ecs::registry const &other)
{
    ECS_PROFILE();

    for(auto const &[signature, entities] : other.m_entityManager.getEntityGroups())
    {
        for(std::size_t id = 0; id < component_manager::getNextID(); ++id) 
        {
            if(!signature.test(id)) 
                continue;
            if(!m_componentManager.getComponentArrays().contains(id))
                m_componentManager.getComponentArrays().emplace(id, other.m_componentManager.getComponentArrays().get(id)->cloneEmpty());
        }
        for(auto const &other_entity : entities.data())
        {
            entity entity = create(signature);
            for(std::size_t id = 0; id < component_manager::getNextID(); ++id) 
            {
                if(!signature.test(id)) 
                    continue;

                m_componentManager.getComponentArrays().get(id)->copyEntityFrom(other.m_componentManager.getComponentArrays().get(id).get(), entity, other_entity);
            }
        }
    }
}
inline bool ecs::registry::same(entity const &first, entity const &second, registry const &secondRegistry)
{
    if(!valid(first) || !secondRegistry.valid(second)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    return m_entityManager.getSignature(first) == secondRegistry.m_entityManager.getSignature(second);
}
inline bool ecs::registry::same(entity const &first, entity const &second)
{
    if(!valid(first) || !valid(second)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    return m_entityManager.getSignature(first) == m_entityManager.getSignature(second);
}
inline std::vector<ecs::entity> ecs::registry::view(signature required, signature excluded) const
{
    ECS_PROFILE();
    std::vector<ecs::entity> result;
    result.reserve(10);

    for(auto const &[signature, group] : m_entityManager.getEntityGroups())
    {
        if((signature & required) == required && (signature & excluded).none())
            result.insert(result.end(), group.data().begin(), group.data().end());
    }

    return result;
}
inline std::vector<ecs::entity> ecs::registry::view_any_of(signature mayHave, signature excluded) const 
{
    ECS_PROFILE();
    std::vector<ecs::entity> result;
    result.reserve(10);

    for(auto const &[signature, group] : m_entityManager.getEntityGroups())
    {
        if((signature & mayHave).any() && (signature & excluded).none())
            result.insert(result.end(), group.data().begin(), group.data().end());
    }

    return result;
}
template <typename... Include, typename... Exclude>
inline std::vector<ecs::entity> ecs::registry::view(exclude_t<Exclude...>) const
{
    ECS_PROFILE();

    ecs::signature required;
    ecs::signature excluded;
    (required.set(component_manager::getComponentID<Include>()), ...);
    (excluded.set(component_manager::getComponentID<Exclude>()), ...);

    return view(required, excluded);
}
template <typename... May, typename... Exclude>
inline std::vector<ecs::entity> ecs::registry::view_any_of(exclude_t<Exclude...> toExclude) const 
{
    ECS_PROFILE();

    ecs::signature required;
    ecs::signature excluded;
    (required.set(component_manager::getComponentID<May>()), ...);
    (excluded.set(component_manager::getComponentID<Exclude>()), ...);

    return view_any_of(required, excluded);
}

template <typename... Components_t>
inline ecs::signature ecs::make_signature()
{
    signature signature;
    (signature.set(component_manager::getComponentID<Components_t>()), ...);
    return signature;
}

/*! \endcond */
