#pragma once
#include <Arduino.h>


class Lockable
{
    public:
        Lockable( SemaphoreHandle_t& mutex ) : m_mutex(mutex) {}
        virtual ~Lockable() = default;

    private:
        SemaphoreHandle_t& m_mutex;
        friend class MutexHolder;
        
};


class MutexHolder
{
    public:
        MutexHolder( Lockable *pLockable)  : m_pLockable(pLockable)
        {
            if (!xSemaphoreTakeRecursive(m_pLockable->m_mutex,portMAX_DELAY))
            {
                assert(false);
            }
        }
        ~MutexHolder()
        {
            xSemaphoreGiveRecursive(m_pLockable->m_mutex);
        }
    private:
        Lockable *m_pLockable;
};
