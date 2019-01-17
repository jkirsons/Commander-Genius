/*
 OpenLieroX
 
 various utilities
 
 code under LGPL
 created 01-05-2007
 by Albert Zeyer and Dark Charlie
 */

#ifndef __OLX__REF_H__
#define __OLX__REF_H__

/*
 intended to hold an object of an abstract class (interface)
 
 Ref< Iterator<int> > is a typical example, because you normally
 don't want to know about the specific implementation of Iterator
 */
template < typename _Obj >
class Ref {
private:
	_Obj* m_obj;
	void clear() { if(m_obj) delete m_obj; m_obj = NULL; }
	
public:
	Ref(_Obj* obj = NULL) : m_obj(obj) {}
	Ref(const Ref& ref) { m_obj = ref->copy(); }
	~Ref() { clear(); }
	
	Ref& operator=(_Obj* obj) { if(obj != m_obj) { clear(); m_obj = obj; } return *this; }
	Ref& operator=(const Ref& ref) { if(ref.m_obj != m_obj) { clear(); m_obj = ref->copy(); } return *this; }
	
	_Obj* operator->() { return m_obj; }
	const _Obj* operator->() const { return m_obj; }
	_Obj& get() { return *m_obj; }
	const _Obj& get() const { return *m_obj; }
	bool isSet() const { return m_obj != NULL; }
	_Obj* overtake() { _Obj* r = m_obj; m_obj = NULL; return r; } // this resets the Ref and returns the old pointer without deleting it
};

#endif

