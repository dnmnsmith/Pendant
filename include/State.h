#pragma once

#include <string>
#include <stdint.h>
#include <functional> 

class State;

struct StateNotifyStruct
{
    void (*fn)(State*,void*);
    State *state;
    void * param;
};


class State
{

    public:
        State( const std::string &name ) : m_name(name) {}
        virtual ~State() = default;
        State( const State &other) 
        {
            m_name=other.m_name;
            m_notifyFn=other.m_notifyFn;
            m_notifyParam=other.m_notifyParam;
        };

        State& operator=( const State& other ) 
        {
            m_name=other.m_name;
            m_notifyFn=other.m_notifyFn;
            m_notifyParam=other.m_notifyParam;

            return *this;
        }

        void SetNotify( void (*fn)(State*,void*), void *param = nullptr )
        {
            m_notifyFn = fn;
            m_notifyParam = param;

            OnChange();
        }

        void OnChange()
        {
            if (m_notifyFn != nullptr)
            {
                //Serial.printf("Notifying change for %s\n",m_name.c_str());
                m_notifyFn( this, m_notifyParam );
            }
            m_dirty = false;
        }

        void NotifyIfDirty()
        {
            if (m_dirty)
                OnChange();
        }

        void setDirty( bool dirty = true ) { m_dirty = dirty; }

        const std::string &name() const {return m_name; }
        void name(const std::string &name) { m_name.assign(name);}

    private:
        std::string m_name;
        bool m_dirty = false;

        void (*m_notifyFn)(State*,void*) = nullptr;
        void * m_notifyParam = nullptr;
};

template <class T> 
class TState : public State
{
   public:
        TState<T>(const std::string &name, T value) : State(name) { m_value = value; }
        TState<T>(const std::string &name) : State(name) {}
        ~TState<T>() = default;
        TState<T>( const TState<T> &other) : State(other), m_value(other.m_value) {}

        TState<T>& operator=( const TState<T>& other )
        {
            State::operator=(other);

            m_value=other.m_value;

            return *this;
        }


        void set( T value, bool notify = true)
        {

            if (value != m_value)
            {
                m_value = value;
                if (notify)
                {
                    OnChange();
                }
                else
                {
                    setDirty();
                }
            }            
        }

        T get() const
        {
            return m_value;
        }

    private:
        T m_value;
};

