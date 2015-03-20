#ifndef _COUPLE_H
#define _COUPLE_H

#include <math.h>

template <class T> class CCouple
{
  public:
	  T x, y;

  public:
	inline CCouple() {}
	inline CCouple(const T &a) {x = y = a;}
	inline CCouple(const T &a, const T &b) {x=a; y=b;}
	template <class S> inline CCouple(const CCouple<S> &c) {x=(T)c.x; y=(T)c.y;}
	inline void Set(const T &a, const T &b) {x=a; y=b;}

	inline CCouple<T> &operator =(const T &t)
	{
		x=y=t;
		return *this;
	}
	bool operator ==(const CCouple<T> &c) const
	{
		return (x==c.x && y==c.y);
	}

	bool operator !=(const CCouple<T> &c) const
	{
		return (x!=c.x || y!=c.y);
	}

	bool operator >(const CCouple<T> &c) const
	{
		return (x>c.x || (x==c.x && y>c.y));
	}

	bool operator <(const CCouple<T> &c) const
	{
		return (x<c.x || (x==c.x && y<c.y));
	}

	bool operator >=(const CCouple<T> &c) const
	{
		return (x>=c.x || (x==c.x && y>=c.y));
	}

	bool operator <=(const CCouple<T> &c) const
	{
		return (x<=c.x || (x==c.x && y<=c.y));
	}

	bool IsInRange(const CCouple<T> &pmin, const CCouple<T> &pmax) const
	{
		return (x>=pmin.x && x<=pmax.x &&
			y>=pmin.y && y<=pmax.y);
	}

	void LimitWithinRange(const CCouple<T> &pmin, const CCouple<T> &pmax)
	{
		if (x<pmin.x) x = pmin.x;
		else if (x>pmax.x) x = pmax.x;
		if (y<pmin.y) y = pmin.y;
		else if (y>pmax.y) y = pmax.y;
	}

	CCouple<T> operator +(const CCouple<T> &c) const
	{
		CCouple<T> r;
		r.x = x+c.x;
		r.y = y+c.y;
		return r;
	}

	CCouple<T> operator -() const
	{
		CCouple<T> r;
		r.x = -x;
		r.y = -y;
		return r;
	}

	CCouple<T> operator -(const CCouple<T> &c) const
	{
		CCouple<T> r;
		r.x = x-c.x;
		r.y = y-c.y;
		return r;
	}

	CCouple<T> operator *(const T &t) const
	{
		CCouple<T> r;
		r.x = x*t;
		r.y = y*t;
		return r;
	}

	friend CCouple<T> operator *(const T &t, const CCouple<T> &c) {return c*t;}

	CCouple<T> operator /(const T &t) const
	{
		CCouple<T> r;
		r.x = x/t;
		r.y = y/t;
		return r;
	}

	CCouple<T> &operator +=(const CCouple<T> &c)
	{
		x += c.x;
		y += c.y;
		return *this;
	}

	CCouple<T> &operator -=(const CCouple<T> &c)
	{
		x -= c.x;
		y -= c.y;
		return *this;
	}

	CCouple<T> &operator *=(const T &t)
	{
		x*=t;
		y*=t;
		return *this;
	}

	CCouple<T> &operator /=(const T &t)
	{
		x/=t;
		y/=t;
		return *this;
	}

	T L2Norm() const
	{
		return (T)sqrt(x*x+y*y);
	}

	T L2Norm2() const
	{
		return x*x+y*y;
	}

	CCouple<T> Normalized()
	{
		T nor = L2Norm();
		if (nor!=(T)0)
			return *this/nor;
		else return *this;
	}

	friend T dotProduct(const CCouple<T> &a, const CCouple<T> &b)
	{
		return (a.x*b.x+a.y*b.y);
	}

	friend T crossProduct(const CCouple<T> &a, const CCouple<T> &b)
	{
		return (a.x*b.y-a.y*b.x);
	}

	friend T angle(const CCouple<T> &a, const CCouple<T> &b)
	{
		T nora=a.L2Norm(), norb=b.L2Norm();
		if (nora!=(T)0 && norb!=(T)0)
			return (T)acos(prodScal(a,b)/(nora*norb));
		else return (T)0;
	}

	inline friend CCouple<T> coupleMin(const CCouple<T> &a, const CCouple<T> &b)
	{
		CCouple<T> r;
		r.x = (a.x<b.x)?a.x:b.x;
		r.y = (a.y<b.y)?a.y:b.y;
		return r;
	}

	inline friend CCouple<T> coupleMax(const CCouple<T> &a, const CCouple<T> &b)
	{
		CCouple<T> r;
		r.x = (a.x>b.x)?a.x:b.x;
		r.y = (a.y>b.y)?a.y:b.y;
		return r;
	}
};

#endif
