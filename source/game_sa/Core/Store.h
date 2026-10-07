/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

template <typename ObjectType, size_t Capacity> class CStore {
public:
    size_t     m_nCount;
    ObjectType m_aObjects[Capacity];

public:
    // (Only if the objects can be constructed too: the body can't be compiled otherwise, and asking whether a store
    // can be default constructed has to give the right answer, see `StaticData.h`)
    CStore() requires std::is_default_constructible_v<ObjectType> {
        m_nCount = 0;
    }

    ObjectType& GetItemAtIndex(size_t index) {
        return m_aObjects[index];
    }

    inline ObjectType& AddItem() {
        auto& obj = m_aObjects[m_nCount];
        ++m_nCount;
        return obj;
    }
};
