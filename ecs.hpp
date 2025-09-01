/*
      ___  ___ ___ 
     / _ \/ __/ __|    Copyright (c) 2024 Nikita Martynau
    |  __/ (__\__ \    https://opensource.org/license/mit
     \___|\___|___/    https://github.com/nikitawew/ecs  

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

/*! \cond Doxygen_Suppress */
// Config section 

#include <cassert>

#ifndef ECS_PROFILE
#define ECS_PROFILE()
#endif

#ifndef ECS_ASSERT
#define ECS_ASSERT(x, msg) assert((x) && ((msg) || true))
#endif

#ifndef ECS_THROW
#define ECS_THROW(x) (throw (x))
#endif

#ifndef ECS_INLINE
#define ECS_INLINE inline
#endif

#define ECS_IMPLEMENTATION

/*! \endcond */

// ============
// Declaration 
// ============

namespace ecs
{
    /** \brief Entity ID. */
    using entity = std::uint32_t;
    /** \brief Component ID. Used with signature. */
    using component_id = std::uint8_t;

    /** \brief Controls the maximum number of registered components allowed to exist simultaneously. */
    constexpr component_id MAX_COMPONENTS = 128;

    /**
     * \brief Used to track which components entity has. 
     * As an example, if Transform has type 0, RigidBody has type 1, and Gravity has type 2, an entity that “has” those three components would have a signature of 0b111 (bits 0, 1, and 2 are set).
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
        /** \brief The dense list iterator */
        using iterator_type = typename std::vector<dense_type>::iterator;
        /** \copydoc iterator_type */
        using const_iterator_type = typename std::vector<dense_type>::const_iterator;

        /** \brief The sparse pointer that represents the empty index. */
        static constexpr std::size_t null = std::numeric_limits<std::size_t>::max();
    private:
        std::vector<dense_type> m_dense;
        std::vector<sparse_type> m_denseToSparse;
        std::vector<size_t> m_sparse;

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
         * \brief Inserts an element to a sparse index.
         * \param sparse A sparse index.
         * \param element A lvalue reference to the element to add.
         * \throws std::invalid_argument If the sparse index already contains an element.
         */
        void insert(sparse_type const &sparse, dense_type const &element);

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
        void remove(sparse_type const &sparse);

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
        std::vector<size_t> const &sparseData() const;

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

        /** \brief The cbegin of the dense list. */
        const_iterator_type begin() const;
        /** \brief The cend of the dense list. */
        const_iterator_type end() const;
        /** \brief The begin of the dense list. */
        iterator_type begin();
        /** \brief The end of the dense list. */
        iterator_type end();
    };

    /**
     * \brief Manages entities (create, destroy) and their signatures (set, get).
     * Any entity supplied to the manager must be created by the same manager object.
     */
    class entity_manager
    {
    private:
        std::queue<entity> m_availableEntityIDs;
        ecs::sparse_set<entity> m_entities;
        std::uint32_t m_livingEntitiesCount = 0;
        ecs::sparse_set<signature> m_signatures;
        ecs::entity m_nextID = 1;
    public:
        explicit entity_manager(ecs::entity numEntities = 100000);
        ~entity_manager() = default;
        /**
         * \brief Creates entity with an optional signature.
         * \param signature A signature representing components the entity has (optional).
         * \return Unique entity id managed by entity_manager.
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
         * \return A copy of a signature, describing the components an entity has.
         */
        ecs::signature getSignature(entity const &entity) const;
        /**
         * \brief Gets the signature of a valid entity.
         * \param entity A valid entity identifier.
         * \return A signature, describing the components an entity has.
         */
        ecs::signature &getSignature(entity const &entity);

        /**
         * \brief Get entities of this manager.
         * \return A set of valid entities created by this manager
         */
        std::vector<entity> const &getEntities() const;

        /**
         * \brief Checks if an identifier refers to a valid entity.
         * \param entity An identifier, either valid or not.
         * \return True if the identifier is valid, false otherwise.
         */
        bool valid(entity const &entity) const;

        /**
         * \return True if next createEntity call will produce a valid entity, false otherwise,
         */
        bool nextEntityValid() const;
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
    };

    /**
     * \brief Stores components of entities of a specific type as a sparse set.
     * @tparam component_t The type of stored components.
     */
    template <typename component_t>
    class component_array : public icomponent_array, public sparse_set<component_t>
    {
    public:
        /**
         * \brief Notify the array that the entity is destroyed.
         * \param entity A destroyed entity identifier.
         */
        void onEntityDestroyed(entity const &entity) override;
    };

    /**
     * \brief Manages components and their arrays. All components are destroyed automatically.
     */
    class component_manager
    {
    private:
        ecs::sparse_set<std::unique_ptr<icomponent_array>> m_componentArrays{};
        static component_id m_nextID;
    public:
        component_manager() = default;
        ~component_manager() = default;

        /**
         * \brief Registers component.
         * \tparam component_t The component type.
         * This should be called for every component used. Multiple calls for the same component_t will do nothing.
         */
        template <typename component_t> 
        void registerComponent();

        /**
         * \brief Get unique component ID used to index the signature bitset.
         * \tparam component_t The component type.
         */
        template <typename component_t> 
        component_id getComponentID() const;

        /**
         * \brief Adds component to an entity.
         * \param entity A valid entity identifier.
         * \param component An optional rvalue reference to the component to move.
         * \tparam component_t A component type.
         */
        template <typename component_t> 
        void add(entity const &entity, component_t const &component);

        /**
         * \brief Emplaces a component to a valid entity.
         * \param entity A valid entity identifier.
         * \param args Arguments forwarded to construct the new component.
         */
        template <typename component_t, class... Args>
        void emplace(entity const &entity, Args&&... args);

        /**
         * \brief Removes component from an entity.
         * \param entity A valid entity identifier.
         * \tparam component_t A component type.
         */
        template <typename component_t> 
        void remove(entity const &entity);

        /**
         * \brief Gets a component of an entity.
         * Not synchronized!
         * \param entity A valid entity identifier.
         * \tparam component_t The component type.
         * \return A component lvalue reference.
         */
        template <typename component_t> 
        component_t &get(entity const &entity);

        /** \copydoc get */
        template <typename component_t> 
        component_t const &get(entity const &entity) const;

        /**
         * \brief Notify component arrays that the entity is destroyed.
         * \param entity A deleted entity identifier.
         */
        void entityDestroyed(entity const &entity) const;
    private:
        /*! \cond Doxygen_Suppress */
        template <typename component_t> 
        component_array<component_t> *getComponentArray();
        template <typename component_t>
        ecs::component_array<component_t> const *getComponentArray() const;
        /*! \endcond */
    };

    /**
     * \brief A class to use to push around lists of types.
     * \tparam Type Types provided by the type list.
     */
    template<typename... Type>
    struct type_list {
        /*! \brief Type list type. */
        using type = type_list;
        /*! \brief Compile-time number of elements in the type list. */
        static constexpr auto size = sizeof...(Type);
    };
    /**
     * \brief Alias for exclusion lists.
     * \tparam Type List of types.
     */
    template<typename... Type>
    struct exclude_t final : type_list<Type...> {
        /*! \brief Default constructor. */
        explicit constexpr exclude_t() = default;
    };
    /**
     * \brief Alias for inclusion lists.
     * \tparam Type List of types.
     */
    template<typename... Type>
    struct include_t final : type_list<Type...> {
        /*! \brief Default constructor. */
        explicit constexpr include_t() = default;
    };

    /**
     * \brief An ECS interface.
     * This class should be enough to use the ecs.
     */
    class registry
    {
    private:
        ecs::entity_manager m_entityManager;
        // ugly fix for lazy component registration.
        mutable ecs::component_manager m_componentManager;
    public:
        /** \copydoc ecs::entity_manager::valid */
        bool valid(entity const &entity) const;

        /**
         * \brief Construct a signature from the component type list.
         * \tparam Components_t The variadic component type list.
         * \return The signature with all the required components set.
         */
        template<typename... Components_t>
        constexpr ecs::signature makeSignature() const;

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
         * \brief Adds a component to a valid entity.
         * \param entity A valid entity identifier.
         * \param component An optional component value to move.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::invalid_argument if the component is already added.
         * \tparam component_t The component type.
         */
        template <typename component_t> 
        void add(entity const &entity, component_t const &component = {});
        
        /**
         * \copydoc ecs::component_manager::emplace
         * \throws std::invalid_argument if the entity is not a valid identifier.
         * \throws std::invalid_argument if the component is already added.
         */
        template <typename component_t, class... Args>
        void emplace(entity const &entity, Args&&... args);

        /**
         * \brief Creates entity with optional components.
         * \throws std::length_error If the next entity is invalid. Perhaps too many entities?
         * \tparam Components_t Components (optional).
         * \return Unique valid entity id managed by entity_manager.
         */
        template <typename... Components_t> 
        ecs::entity create();

        /**
         * \copydoc create
         * \param components The components to move in.
         */
        template <typename... Components_t> 
        ecs::entity create(Components_t&&... components);

        /**
         * \brief Destroys an entity and its components.
         * \param entity A valid entity identifier.
         * \throws std::invalid_argument if the entity is not a valid identifier.
         */
        void destroy(ecs::entity const &entity);

        /**
         * \brief Returns a view for the given elements.
         * \tparam Type Type of element used to construct the view.
         * \tparam Other Other types of elements used to construct the view.
         * \tparam Exclude Types of elements used to filter the view.
         * \param toExclude The type list used to deduce Exclude variadic template argument.
         * \return A view on entities that are valid at the time of calling this member function and contain given included components and do not contain excluded ones at the time of calling this member function.
         */
        template<typename Type, typename... Other, typename... Exclude>
        std::vector<ecs::entity> view(exclude_t<Exclude...> toExclude = exclude_t{}) const;

        /**
         * \brief Get entities of this registry.
         * \return A set of valid entities created by this registry.
         */
        std::vector<entity> getEntities() const;

        /**
         * \copydoc ecs::entity_manager::getSignature
         * \throws std::invalid_argument if the entity is not a valid identifier.
         */
        ecs::signature getSignature(entity const &entity) const;

        /** \copydoc ecs::component_manager::getComponentID */
        template <typename component_t> 
        component_id getComponentID();
    };
} // namespace ecs

// ===============
// Implementation
// ===============

#ifdef ECS_IMPLEMENTATION
/*! \cond Doxygen_Suppress */

template <typename dense_t>
ECS_INLINE void ecs::sparse_set<dense_t>::setDenseIndex(sparse_type const &sparse, std::size_t index)
{
    ECS_PROFILE();
    if(sparse < 0)
        return;

    if(sparse >= m_sparse.size())
        m_sparse.resize(sparse + 1, null);

    m_sparse[sparse] = index;
}
template <typename dense_t>
ECS_INLINE std::size_t ecs::sparse_set<dense_t>::getDenseIndex(sparse_type const &sparse) const
{
    ECS_PROFILE();
    if(sparse < 0 || sparse >= m_sparse.size())
        return null;

    return m_sparse[sparse];
}
template <typename dense_t>
ECS_INLINE ecs::sparse_set<dense_t>::sparse_set(std::size_t capacity)
{
    ECS_PROFILE();
    reserve(capacity);
}
template <typename dense_t>
template <class... Args>
ECS_INLINE void ecs::sparse_set<dense_t>::emplace(sparse_type const &sparse, Args &&...args)
{
    ECS_PROFILE();
    if(contains(sparse))
        ECS_THROW(std::invalid_argument{"element added to the same sparse index more than once!"});

    if constexpr(std::is_aggregate_v<dense_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<dense_type>)) 
        m_dense.emplace_back(dense_type{std::forward<Args>(args)...});
    else 
        m_dense.emplace_back(std::forward<Args>(args)...);

    std::size_t index = m_dense.size() - 1;
    setDenseIndex(sparse, index);
    m_denseToSparse.emplace_back(sparse);
}
template <typename dense_t>
ECS_INLINE void ecs::sparse_set<dense_t>::insert(sparse_type const &sparse, dense_type const &element)
{
    ECS_PROFILE();
    emplace(sparse, element);
}
template <typename dense_t>
ECS_INLINE void ecs::sparse_set<dense_t>::remove(sparse_type const &sparse)
{
    ECS_PROFILE();
    if(!contains(sparse))
        ECS_THROW(std::out_of_range{"removing a non-existing element from a sparse index!"});

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
ECS_INLINE bool ecs::sparse_set<dense_t>::contains(sparse_type const &sparse) const
{
    ECS_PROFILE();
    return getDenseIndex(sparse) != null;
}
template <typename dense_t>
ECS_INLINE void ecs::sparse_set<dense_t>::reserve(std::size_t newCapacity)
{
    ECS_PROFILE();
    m_dense.reserve(newCapacity);
    m_denseToSparse.reserve(newCapacity);
    m_sparse.reserve(newCapacity);
}
template <typename dense_t>
ECS_INLINE void ecs::sparse_set<dense_t>::shrink_to_fit()
{
    ECS_PROFILE();
    m_dense.shrink_to_fit();
    m_denseToSparse.shrink_to_fit();
    m_sparse.shrink_to_fit();
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::dense_type const &ecs::sparse_set<dense_t>::get(sparse_type const &sparse) const
{
    ECS_PROFILE();
    if(!contains(sparse))
        ECS_THROW(std::out_of_range{"getting a non-existing element from a sparse index!"});

    return m_dense[getDenseIndex(sparse)];
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::dense_type &ecs::sparse_set<dense_t>::get(sparse_type const &sparse)
{
    ECS_PROFILE();
    if(!contains(sparse))
        ECS_THROW(std::out_of_range{"getting a non-existing element from a sparse index!"});

    return m_dense[getDenseIndex(sparse)];
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::dense_type &ecs::sparse_set<dense_t>::operator[](sparse_type const &sparse)
{
    ECS_PROFILE();
    return get(sparse);
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::dense_type const &ecs::sparse_set<dense_t>::operator[](sparse_type const &sparse) const
{
    ECS_PROFILE();
    return get(sparse);
}
template <typename dense_t>
ECS_INLINE std::vector<typename ecs::sparse_set<dense_t>::dense_type> const &ecs::sparse_set<dense_t>::data() const
{
    ECS_PROFILE();
    return m_dense;
}
template <typename dense_t>
ECS_INLINE std::vector<typename ecs::sparse_set<dense_t>::sparse_type> const &ecs::sparse_set<dense_t>::getDenseToSparse() const
{
    ECS_PROFILE();
    return m_denseToSparse;
}
template <typename dense_t>
ECS_INLINE std::vector<size_t> const &ecs::sparse_set<dense_t>::sparseData() const
{
    ECS_PROFILE();
    return m_sparse;
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::const_iterator_type ecs::sparse_set<dense_t>::begin() const
{
    ECS_PROFILE();
    return m_dense.begin();
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::const_iterator_type ecs::sparse_set<dense_t>::end() const
{
    ECS_PROFILE();
    return m_dense.end();
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::iterator_type ecs::sparse_set<dense_t>::begin()
{
    ECS_PROFILE();
    return m_dense.begin();
}
template <typename dense_t>
ECS_INLINE typename ecs::sparse_set<dense_t>::iterator_type ecs::sparse_set<dense_t>::end()
{
    ECS_PROFILE();
    return m_dense.end();
}

ECS_INLINE ecs::entity_manager::entity_manager(ecs::entity numEntities)
{
    ECS_PROFILE();
    for(; m_nextID <= numEntities; ++m_nextID) {
        m_availableEntityIDs.push(m_nextID);
    }
    m_signatures.reserve(numEntities);
}
ECS_INLINE ecs::entity ecs::entity_manager::createEntity(signature signature)
{
    ECS_PROFILE();
    ECS_ASSERT(nextEntityValid(), "too many entities! (should not happen?)");

    entity entity = m_availableEntityIDs.front();
    m_availableEntityIDs.pop();
    ++m_livingEntitiesCount;
    m_entities.emplace(entity, entity);

    if(m_availableEntityIDs.empty())
        m_availableEntityIDs.push(m_nextID++);

    m_signatures.emplace(entity, signature);

    return entity;
}
ECS_INLINE void ecs::entity_manager::destroyEntity(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    --m_livingEntitiesCount;
    m_availableEntityIDs.push(entity);
    m_entities.remove(entity);
    m_signatures.remove(entity);
}
ECS_INLINE void ecs::entity_manager::setSignature(entity const &entity, signature signature)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    m_signatures[entity] = signature;
}
ECS_INLINE ecs::signature ecs::entity_manager::getSignature(entity const &entity) const
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");
    
    return m_signatures[entity];
}
ECS_INLINE ecs::signature &ecs::entity_manager::getSignature(entity const &entity)
{
    ECS_PROFILE();
    ECS_ASSERT(valid(entity), "invalid entity identifier!");

    return m_signatures[entity];
}
ECS_INLINE bool ecs::entity_manager::valid(entity const &entity) const
{
    ECS_PROFILE();
    return 1 <= entity && entity < m_nextID && m_entities.contains(entity);
}
ECS_INLINE bool ecs::entity_manager::nextEntityValid() const
{
    ECS_PROFILE();
    return !m_availableEntityIDs.empty();
}
ECS_INLINE std::vector<ecs::entity> const &ecs::entity_manager::getEntities() const
{
    ECS_PROFILE();
    return m_entities.data();
} 

template <typename component_t>
ECS_INLINE void ecs::component_array<component_t>::onEntityDestroyed(entity const &entity)
{
ECS_PROFILE();
    if(this->contains(entity))
        this->remove(entity);
}

template <typename component_t>
ECS_INLINE void ecs::component_manager::registerComponent()
{
ECS_PROFILE();
    auto id = getComponentID<component_t>();
    if(!m_componentArrays.contains(id))
        m_componentArrays.emplace(id, std::move(std::make_unique<component_array<component_t>>()));
}
template <typename component_t>
ECS_INLINE ecs::component_id ecs::component_manager::getComponentID() const
{
ECS_PROFILE();
    ECS_ASSERT(m_nextID < MAX_COMPONENTS, "too many components registered!");
    static const component_id id = m_nextID++;
    return id;
}
template <typename component_t>
ECS_INLINE void ecs::component_manager::add(entity const &entity, component_t const &component)
{
ECS_PROFILE();
    getComponentArray<component_t>()->insert(entity, component);
}
template <typename component_t>
ECS_INLINE void ecs::component_manager::remove(entity const &entity)
{
ECS_PROFILE();
    getComponentArray<component_t>()->remove(entity);
}
template <typename component_t>
ECS_INLINE component_t &ecs::component_manager::get(entity const &entity)
{
ECS_PROFILE();
    return getComponentArray<component_t>()->get(entity);
}
template <typename component_t>
ECS_INLINE component_t const &ecs::component_manager::get(entity const &entity) const
{
ECS_PROFILE();
    return getComponentArray<component_t>()->get(entity);
}
template <typename component_t>
ECS_INLINE ecs::component_array<component_t> *ecs::component_manager::getComponentArray()
{
ECS_PROFILE();
    auto id = getComponentID<component_t>();
    ECS_ASSERT(m_componentArrays.contains(id), "component not registered before use");
    return static_cast<component_array<component_t> *>(m_componentArrays.get(id).get());
}
template <typename component_t>
ECS_INLINE ecs::component_array<component_t> const *ecs::component_manager::getComponentArray() const
{
ECS_PROFILE();
    auto id = getComponentID<component_t>();
    ECS_ASSERT(m_componentArrays.contains(id), "component not registered before use");
    return static_cast<component_array<component_t> *>(m_componentArrays.get(id).get());
}
ECS_INLINE void ecs::component_manager::entityDestroyed(entity const &entity) const
{
ECS_PROFILE();
    for(auto const &componentArray : m_componentArrays) {
        componentArray->onEntityDestroyed(entity);
    }
}
template <typename component_t, class... Args>
ECS_INLINE void ecs::component_manager::emplace(entity const &entity, Args&&... args)
{
ECS_PROFILE();
    getComponentArray<component_t>()->emplace(entity, std::forward<Args>(args)...);
}
ECS_INLINE ecs::component_id ecs::component_manager::m_nextID = 0;

ECS_PROFILE();
template <typename... Components_t>
constexpr ecs::signature ecs::registry::makeSignature() const
{
    (m_componentManager.registerComponent<Components_t>(), ...);
    ecs::signature signature;
    (signature.set(m_componentManager.getComponentID<Components_t>()), ...);
    return signature;
}
template <typename component_t>
ECS_INLINE bool ecs::registry::has(entity const &entity) const
{ 
ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    
    m_componentManager.registerComponent<component_t>();
    return m_entityManager.getSignature(entity).test(m_componentManager.getComponentID<component_t>()); 
}
template <typename component_t>
ECS_INLINE component_t &ecs::registry::get(entity const &entity) 
{
ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(!has<component_t>(entity)) 
        ECS_THROW(std::out_of_range{"component to get is not added!"});
    
    return m_componentManager.get<component_t>(entity);
}
template <typename component_t>
ECS_INLINE component_t const &ecs::registry::get(entity const &entity) const
{
ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(!has<component_t>(entity)) 
        ECS_THROW(std::out_of_range{"component to get is not added!"});
    
    return m_componentManager.get<component_t>(entity);
}
template <typename... Components_t>
ECS_INLINE ecs::entity ecs::registry::create()
{
ECS_PROFILE();
    if(!m_entityManager.nextEntityValid())
        throw std::length_error{"Invalid entity creation. (too many entities?)"};
    
    (m_componentManager.registerComponent<Components_t>(), ...);
    ecs::signature signature;
    (signature.set(m_componentManager.getComponentID<Components_t>()), ...);

    entity entity = m_entityManager.createEntity(signature);

    (m_componentManager.add(entity, Components_t{}), ...);

    return entity;
}
template <typename... Components_t>
ECS_INLINE ecs::entity ecs::registry::create(Components_t &&...components)
{
ECS_PROFILE();
    if(!m_entityManager.nextEntityValid())
        throw std::length_error{"Invalid entity creation. (too many entities?)"};
    
    (m_componentManager.registerComponent<Components_t>(), ...);
    ecs::signature signature;
    (signature.set(m_componentManager.getComponentID<Components_t>()), ...);

    entity entity = m_entityManager.createEntity(signature);

    (m_componentManager.add(entity, std::forward<Components_t>(components)), ...);

    return entity;
}
template <typename component_t>
ECS_INLINE void ecs::registry::remove(entity const &entity)
{
ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(!has<component_t>(entity)) 
        ECS_THROW(std::out_of_range{"component to remove is not added!"});
    
    m_entityManager.getSignature(entity).set(m_componentManager.getComponentID<component_t>(), false);
    m_componentManager.remove<component_t>(entity);
}
template <typename component_t, class... Args>
ECS_INLINE void ecs::registry::emplace(entity const &entity, Args&&... args)
{
ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});
    if(has<component_t>(entity)) 
        ECS_THROW(std::invalid_argument{"component to emplace already added!"});

    m_componentManager.emplace<component_t>(entity, std::forward<Args>(args)...);
    m_entityManager.getSignature(entity).set(m_componentManager.getComponentID<component_t>(), true);
}
template <typename component_t>
ECS_INLINE void ecs::registry::add(entity const &entity, component_t const &component)
{
ECS_PROFILE();
    emplace<component_t>(entity, component);
}
ECS_INLINE bool ecs::registry::valid(entity const &entity) const
{
ECS_PROFILE();
    return m_entityManager.valid(entity);
}
ECS_INLINE void ecs::registry::destroy(ecs::entity const &entity)
{
ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});

    m_entityManager.destroyEntity(entity);

    m_componentManager.entityDestroyed(entity);
}
ECS_INLINE std::vector<ecs::entity> ecs::registry::getEntities() const
{
ECS_PROFILE();
    return m_entityManager.getEntities();
}
template<typename Type, typename... Other, typename... Exclude>
ECS_INLINE std::vector<ecs::entity> ecs::registry::view(exclude_t<Exclude...>) const
{
ECS_PROFILE();

    auto entities = m_entityManager.getEntities();

    m_componentManager.registerComponent<Type>();
    (m_componentManager.registerComponent<Other>(), ...);
    (m_componentManager.registerComponent<Exclude>(), ...);
    ecs::signature required;
    required.set(m_componentManager.getComponentID<Type>());
    (required.set(m_componentManager.getComponentID<Other>()), ...);
    ecs::signature excluded;
    (excluded.set(m_componentManager.getComponentID<Exclude>()), ...);

    std::vector<ecs::entity> result;
    result.reserve(10);

    for(auto const &entity : entities)
    {
        auto signature = m_entityManager.getSignature(entity);
        if((signature & required) == required && (signature & excluded).none())
            result.emplace_back(entity);
    }

    return result;
}
ECS_INLINE ecs::signature ecs::registry::getSignature(entity const &entity) const
{
ECS_PROFILE();
    if(!valid(entity)) 
        ECS_THROW(std::invalid_argument{"invalid entity identifier!"});

    return m_entityManager.getSignature(entity);
}
template <typename component_t>
ECS_INLINE ecs::component_id ecs::registry::getComponentID()
{
ECS_PROFILE();
    m_componentManager.registerComponent<component_t>();
    return m_componentManager.getComponentID<component_t>();
}

/*! \endcond */
#endif // ifdef ECS_IMPLEMENTATION
