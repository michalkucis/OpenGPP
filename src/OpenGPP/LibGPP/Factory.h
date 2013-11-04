#pragma once

#include "Set.h"
#include "Ptr.h"
#include <boost/shared_ptr.hpp>
#include <assert.h>


//template <typename T>
//class Factory;
//
//template <typename T>
//class PtrProduct: public Ptr<T>
//{
//	Factory<T>* m_factory;
//public:
//	PtrProduct ()
//	{
//		m_factory=NULL;
//	}
//	PtrProduct (const PtrProduct<T>& ptr): Ptr<T>(ptr)
//	{
//		m_factory=ptr._internalGetFactory();
//	}
//	PtrProduct (T* ptr): Ptr<T>(ptr)
//	{
//		m_factory=NULL;
//	}
//	virtual ~PtrProduct()
//	{
//	}
//	T& operator* ()
//	{
//		if (isNull())
//			assert(! isNull());
//		return boost::shared_ptr<T>::operator*();
//	}
//	T* operator-> ()
//	{
//		if (isNull())
//			assert(! isNull());
//		return boost::shared_ptr<T>::operator->();
//	}
//	T operator= (PtrProduct<T> ptr)
//	{
//		boost::shared_ptr<T>::operator=(ptr);
//		m_factory = ptr._internalGetFactory();
//		return this->operator*();
//	}
//	bool isNull ()
//	{
//		return boost::shared_ptr<T>::get() == NULL;
//	}
//	void _internalSetFactory (Factory<T>* factory)
//	{
//		m_factory = factory;
//	}
//	void _internalUnsetFactory ()
//	{
//		m_factory = NULL;
//	}
//	Factory<T>* _internalGetFactory () const
//	{
//		return m_factory;
//	}
//};
//
//template<typename T>
//inline bool operator< (const PtrProduct<T>& lhs, const PtrProduct<T>& rhs)
//{
//	const boost::shared_ptr<T>* p1 = reinterpret_cast<const boost::shared_ptr<T>*>(&lhs);
//	const boost::shared_ptr<T>* p2 = reinterpret_cast<const boost::shared_ptr<T>*>(&rhs);
//	assert(p1);
//	assert(p2);
//	return *p1 < *p2;
//}
//
//template <typename T>
//class Visitor1UnsetFactories: public Visitor1ReadWrite<PtrProduct<T>>
//{
//public:
//	void visitReadWrite (int, PtrProduct<T>& value)
//	{
//		value._internalUnsetFactory();
//	}
//};
//
//template <typename T>
//class Factory
//{
//private:
//	Set<PtrProduct<T>> m_used;
//	Set<PtrProduct<T>> m_allocNotUsed;
//
//public:
//	Factory<T> ()
//	{}
//	PtrProduct<T> createProduct ()
//	{
//		if (m_allocNotUsed.isEmpty())
//		{
//			PtrProduct<T> ptr = allocProduct();
//			ptr._internalSetFactory(this);
//			m_allocNotUsed.insert(ptr);
//		}
//
//		PtrProduct<T> ptr = m_allocNotUsed.getAndPopFirst();
//		startUsingProduct(ptr);
//		m_used.insert(ptr);
//		return ptr;
//	}
//	void _internalStopUsing (PtrProduct<T> ptr)
//	{
//		if (m_used.exist(ptr))
//			error0 (ERR_STRUCT, "Product is not used");
//		m_used.erase(ptr);
//		stopUsingProduct(ptr);
//		m_allocNotUsed.insert(ptr);
//	}
//	~Factory<T>()
//	{
//		Visitor1UnsetFactories<T> visit;
//		m_allocNotUsed.visit (&visit);
//		m_used.visit (&visit);
//	}
//protected:
//	virtual PtrProduct<T> allocProduct () = 0;
//	virtual void startUsingProduct (PtrProduct<T> ptr)
//	{}
//	virtual void stopUsingProduct (PtrProduct<T> ptr)
//	{}
//};

template <typename T>
class Factory
{
public:
	Factory<T> ()
	{}
	Ptr<T> createProduct ()
	{
		return allocProduct ();
	}
protected:
	virtual Ptr<T> allocProduct () = 0;
};