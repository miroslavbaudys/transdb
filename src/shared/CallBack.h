/*
 * Game Server
 * Copyright (C) 2010 Miroslav 'Wayland' Kudrnac
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _CALLBACK_H
#define _CALLBACK_H

#include "Defines.h"

class CallbackBase
{
public:
    explicit CallbackBase() {}
    virtual ~CallbackBase() {};
	virtual void execute() = 0;
    
private:
    DISALLOW_COPY_AND_ASSIGN(CallbackBase);
};

template <class Class>
class CallbackP0 : public CallbackBase
{
    typedef void (Class::*Method)();
    
public:
	explicit CallbackP0(Class* _class_instance, Method _method) : m_obj(_class_instance), m_func(_method)
	{
	}
    
	void operator()()   { return (m_obj->*m_func)(); }
	void execute()      { return operator()(); }

private:
	Class*  m_obj;
	Method  m_func;
};

template <class Class, typename P1>
class CallbackP1 : public CallbackBase
{
    typedef void (Class::*Method)(P1);
    
public:
	explicit CallbackP1(Class* _class_instance, Method _method, P1 p1) : m_obj(_class_instance), m_func(_method), m_p1(p1)
	{
	}

	void operator()()   { return (m_obj->*m_func)(m_p1); }
	void execute()      { return operator()(); }

private:
	Class*  m_obj;
	Method  m_func;
	P1 m_p1;
};

template <class Class, typename P1, typename P2>
class CallbackP2 : public CallbackBase
{
    typedef void (Class::*Method)(P1, P2);
    
public:
	explicit CallbackP2(Class* _class_instance, Method _method, P1 p1, P2 p2) : m_obj(_class_instance), m_func(_method), m_p1(p1), m_p2(p2)
	{
	}

	void operator()()   { return (m_obj->*m_func)(m_p1, m_p2); }
	void execute()      { return operator()(); }

private:
	Class*  m_obj;
	Method  m_func;
	P1 m_p1;
	P2 m_p2;
};

template <class Class, typename P1, typename P2, typename P3>
class CallbackP3 : public CallbackBase
{
    typedef void (Class::*Method)(P1, P2, P3);
    
public:
	explicit CallbackP3(Class* _class_instance, Method _method, P1 p1, P2 p2, P3 p3) : m_obj(_class_instance), m_func(_method), m_p1(p1), m_p2(p2), m_p3(p3)
	{
	}

	void operator()()   { return (m_obj->*m_func)(m_p1, m_p2, m_p3); }
	void execute()      { return operator()(); }

private:
	Class*  m_obj;
	Method  m_func;
	P1 m_p1;
	P2 m_p2;
	P3 m_p3;
};

template <class Class, typename P1, typename P2, typename P3, typename P4>
class CallbackP4 : public CallbackBase
{
    typedef void (Class::*Method)(P1, P2, P3, P4);
    
public:
	explicit CallbackP4(Class* _class_instance, Method _method, P1 p1, P2 p2, P3 p3, P4 p4) : m_obj(_class_instance), m_func(_method), m_p1(p1), m_p2(p2), m_p3(p3), m_p4(p4)
	{
	}

	void operator()()   { return (m_obj->*m_func)(m_p1, m_p2, m_p3, m_p4); }
	void execute()      { return operator()(); }

private:
	Class*  m_obj;
	Method  m_func;
	P1 m_p1;
	P2 m_p2;
	P3 m_p3;
	P4 m_p4;
};

#endif
