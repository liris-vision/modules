#ifndef _TRIPLET_H
#define _TRIPLET_H

#include <math.h>

// With gcc, function abs() is defined in stdlib.h
#if defined(__GNUC__) && defined(__unix__)
#include <stdlib.h>
#endif

template <class T> class CTriplet
{
  public:
	T x,y,z;

  public:
	inline CTriplet() {}
	inline CTriplet(const T &a) {x = y = z = a;}
	inline CTriplet(const T &a, const T &b, const T &c) {x=a; y=b; z=c;}
	template <class S> inline CTriplet(const CTriplet<S> &tr) {x=(T)tr.x; y=(T)tr.y; z=(T)tr.z;}
	inline CTriplet &operator =(const T &t) {x=y=z=t; return *this;}
	inline void Set(const T &a, const T &b, const T &c) {x=a; y=b; z=c;}

	bool operator ==(const CTriplet<T> &c) const
	{
		return (x==c.x && y==c.y && z==c.z);
	}

	bool operator !=(const CTriplet<T> &c) const
	{
		return (x!=c.x || y!=c.y || z!=c.z);
	}

	bool operator >(const CTriplet<T> &c) const
	{
		return (x>c.x || (x==c.x && y>c.y) || (x==c.x && y==c.y && z>c.z));
	}

	bool operator <(const CTriplet<T> &c) const
	{
		return (x<c.x || (x==c.x && y<c.y) || (x==c.x && y==c.y && z<c.z));
	}

	bool operator >=(const CTriplet<T> &c) const
	{
		return (x>=c.x || (x==c.x && y>=c.y) || (x==c.x && y==c.y && z>=c.z));
	}

	bool operator <=(const CTriplet<T> &c) const
	{
		return (x<=c.x || (x==c.x && y<=c.y) || (x==c.x && y==c.y && z<=c.z));
	}

	bool IsInRange(const CTriplet<T> &pmin, const CTriplet<T> &pmax) const
	{
		return (x>=pmin.x && x<=pmax.x &&
			y>=pmin.y && y<=pmax.y &&
			z>=pmin.z && z<=pmax.z);
	}

	void LimitWithinRange(const CTriplet<T> &pmin, const CTriplet<T> &pmax)
	{
		if (x<pmin.x) x = pmin.x;
		else if (x>pmax.x) x = pmax.x;
		if (y<pmin.y) y = pmin.y;
		else if (y>pmax.y) y = pmax.y;
		if (z<pmin.z) z = pmin.z;
		else if (z>pmax.z) z = pmax.z;
	}

	inline CTriplet operator +(const CTriplet<T> &c) const
	{
		CTriplet<T> r;
		r.x = x+c.x;
		r.y = y+c.y;
		r.z = z+c.z;
		return r;
	}

	inline CTriplet operator -(const CTriplet<T> &c) const
	{
		CTriplet<T> r;
		r.x = x-c.x;
		r.y = y-c.y;
		r.z = z-c.z;
		return r;
	}

	inline CTriplet<T> operator -() const
	{
		CTriplet<T> r;
		r.x = -x;
		r.y = -y;
		r.z = -z;
		return r;
	}

	friend CTriplet<T> operator *(const T &t, const CTriplet<T> &c) {return c*t;}
	inline CTriplet<T> operator *(const T &t) const
	{
		CTriplet<T> r;
		r.x = x*t;
		r.y = y*t;
		r.z = z*t;
		return r;
	}

	inline CTriplet<T> operator /(const T &t) const
	{
		CTriplet<T> r;
		r.x = x/t;
		r.y = y/t;
		r.z = z/t;
		return r;
	}

	inline CTriplet<T> &operator +=(const CTriplet<T> &c)
	{
		x += c.x;
		y += c.y;
		z += c.z;
		return *this;
	}

	inline CTriplet<T> &operator -=(const CTriplet<T> &c)
	{
		x -= c.x;
		y -= c.y;
		z -= c.z;
		return *this;
	}

	inline CTriplet<T> operator *=(const T &t)
	{
		x*=t;
		y*=t;
		z*=t;
		return *this;
	}

	inline CTriplet<T> operator /=(const T &t)
	{
		x/=t;
		y/=t;
		z/=t;
		return *this;
	}

	inline friend CTriplet<T> tripletMin(const CTriplet<T> &a, const CTriplet<T> &b)
	{
		CTriplet<T> r;
		r.x = (a.x<b.x)?a.x:b.x;
		r.y = (a.y<b.y)?a.y:b.y;
		r.z = (a.z<b.z)?a.z:b.z;
		return r;
	}

	inline friend CTriplet<T> tripletMax(const CTriplet<T> &a, const CTriplet<T> &b)
	{
		CTriplet<T> r;
		r.x = (a.x>b.x)?a.x:b.x;
		r.y = (a.y>b.y)?a.y:b.y;
		r.z = (a.z>b.z)?a.z:b.z;
		return r;
	}

	inline friend CTriplet<T> fabs(const CTriplet<T> &c)
	{
		CTriplet<T> r;
		r.x = fabs(c.x);
		r.y = fabs(c.y);
		r.z = fabs(c.z);
		return r;
	}

	inline T L2Norm() const
	{
		return (T)sqrt(x*x+y*y+z*z);
	}

	inline T L2Norm2() const
	{
		return x*x+y*y+z*z;
	}

	CTriplet<T> Normalized()
	{
		T nor = L2Norm();
		if (nor!=(T)0)
			return *this/nor;
		else return *this;
	}

	inline friend T dotProduct(const CTriplet &a, const CTriplet &b)
	{
		return (a.x*b.x+a.y*b.y+a.z*b.z);
	}

	friend CTriplet<T> crossProduct(const CTriplet<T> &a, const CTriplet<T> &b)
	{
		CTriplet<T> r;
		r.x = a.y*b.z-a.z*b.y;
		r.y = a.z*b.x-a.x*b.z;
		r.z = a.x*b.y-a.y*b.x;
		return r;
	}

	friend T angle(const CTriplet<T> &a, const CTriplet<T> &b)
	{
		float nora=a.L2Norm(), norb=b.L2Norm();
		if (nora!=0 && norb!=0)
			return acos(prodScal(a,b)/(nora*norb));
		else return 0;
	}
};

#endif
