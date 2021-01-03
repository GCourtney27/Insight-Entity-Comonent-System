#pragma once

#include "ECS/World/Actor_Fwd.h"
#include "ECS/Component/Component_Fwd.h"

#include <unordered_map>


namespace ECS
{
	/*
		* The base for a generic component map that an hold any data structure.
		* NOT to be directly instantiated.
	*/
	class ComponentMapBase
	{
	public:
		ComponentMapBase() = default;
		virtual ~ComponentMapBase() = default;

		virtual void DestroyActorRefs(const Actor_t& Actor) = 0;

	};

	/*
		A container for  list of componenet types.
		* Components are garenteed to be linear in memory for a higher cache hit rate upon iteration.
	*/
	template <typename ComponentType>
	class GenericComponentMap : public ComponentMapBase
	{
		friend class World;
	private:
		/*
			An index into the array of raw components.
		*/
		typedef uint32_t	ArrayIndex;

		/*
			Represends a packed valuu for easier insertions and extractions 
			in the component map.
		*/
		struct alignas (sizeof(uint32_t)) PackedKey
		{
			/*
				Owner of the component.
			*/
			Actor_t		Owner;
			/*
				Index of the component in the raw component array.
			*/
			ArrayIndex	Index;
		};
		static_assert(sizeof(PackedKey) == 8, "Packed key not aligned correctly.");

	public:
		/*
			Key value paur for each unique component and its index in the raw component array.
		*/
		std::unordered_map<ComponentUID_t, PackedKey> m_ComponentMap;
		
		/*
			The array of raw component values. All componnets in this map are gaurenteed 
			to be linear in memory for faster iteration.
		*/
		std::vector<ComponentType> m_RawComponents;
		
		/*
			Next available inertion slot for a component in the raw component array.
		*/
		uint32_t m_NextAvailableIndex;

		GenericComponentMap()
			: m_NextAvailableIndex(0u)
		{
			// Verify the template argument is a valid component.
			ValidateComponent<ComponentType>();
		}

		virtual ~GenericComponentMap()
		{
			size_t ComponentsSize = m_RawComponents.size() * sizeof(ComponentType);
			size_t ComponentMapSize = m_ComponentMap.size() * (sizeof(ComponentUID_t) + sizeof(std::pair<Actor_t, ArrayIndex>));
			printf("[WARNING] Generic ComponentMap being destroyed. Raw component memory [%zi] bytes | Component map [%zi] bytes\n", ComponentsSize, ComponentMapSize);
		}

		ComponentType& operator[](uint32_t Index)
		{
			return m_RawComponents[Index];
		}

		/*
			Returns an iterator at the end of the raw component map.
		*/
		typename std::vector<ComponentType>::iterator end()
		{
			return m_RawComponents.end();
		}

		/*
			Returns an iterator at the beginning of the raw component map.
		*/
		typename std::vector<ComponentType>::iterator begin()
		{
			return m_RawComponents.begin();
		}

		/*
			Returns the number of raw components in the map.
		*/
		inline size_t GetNumComponents() const
		{
			return m_RawComponents.size();
		}

		/*
			Returns the size of the underlying component container in bytes.
		*/
		inline size_t GetContainerSize() const
		{
			return m_RawComponents.size() * sizeof(ComponentType);
		}

		/*
			Adds a component to the component map and returns a pointer to it.
		*/
		template <typename ... Args>
		ComponentType* AddComponent(const Actor_t& Owner, Args ... args)
		{
			m_RawComponents.push_back(ComponentType(args...));
			ComponentUID_t id = m_RawComponents[m_NextAvailableIndex].GetID();
			m_ComponentMap[id] = PackedKey{ Owner, m_NextAvailableIndex };
			m_NextAvailableIndex++;

			return &m_RawComponents[m_NextAvailableIndex - 1];
		}

		/*
			Removes a component from the map by its unique id.
		*/
		void RemoveComponentById(ComponentUID_t Id)
		{
			ArrayIndex i = m_ComponentMap[Id].Index;
			std::vector<ComponentType>::iterator Iter = m_RawComponents.begin();
			Iter += i;
			m_RawComponents.erase(Iter);
			m_ComponentMap.erase(Id);
			m_NextAvailableIndex--;
		}

		/*
			Returns a pointer to the component in the map fetched by its unique id.
		*/
		ComponentType* GetComponentById(ComponentUID_t Id)
		{
			auto Iter = m_ComponentMap.find(Id);
			if (Iter != m_ComponentMap.end())
				return &m_RawComponents[Iter->second.Index];
			else
				return nullptr;
		}

		/*
			Destroys all references of an actor and its components in this component map.
		*/
		virtual void DestroyActorRefs(const Actor_t& Actor) override
		{
			std::vector<ComponentType>::iterator Iter;
			std::unordered_map<ComponentUID_t, PackedKey>::iterator MapIter = m_ComponentMap.begin();
			
			for (auto& [CompUID, PackedVal] : m_ComponentMap)
			{
				if (PackedVal.Owner == Actor)
				{
					Iter = m_RawComponents.begin() + PackedVal.Index;
					m_RawComponents.erase(Iter);
					m_ComponentMap.erase(MapIter);
				}
				if (m_ComponentMap.size() > 0)	// We could have deleted the only component in this map if there was only one.
					MapIter++;
				else
					break;
			}
		}
	};
}
