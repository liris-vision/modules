/****************************************************************
 * Histogram
 *
 * Objects of this class store general histograms. The class is 
 * implemented as template, so the type can be choosen.
 *
 * IMPORTANT: the parameter 'discrete' of each object, given at
 * the constructor, tells the object if it stores discrete values
 * like a graylevel histogram, or continuous values, like the
 * real values of the range [0-1]. This makes a difference in the
 * stepsize:
 *
 * discrete:   stepsize = (max - min  +  1 ) / bincount
 *                  e.g.  (255 -  0   +  1 ) / 256       = 1
 * continuous: stepsize = (max - min ) / bincount
 *                  e.g.  (1.0 - 0.0 ) / 4               = 0.25
 *
 * A graylevel histogram which uses (wrongly) the continous form,
 * would result in a stepsize of 255/256, which is obviously
 * wrong.
 *
 * --------------------------------------------------------------
 *
 * Christian Wolf
 * wolf@rfv.insa-lyon.fr
 ****************************************************************/

#ifndef _WOLF_HISTOGRAM_H_
#define _WOLF_HISTOGRAM_H_

#include <iostream>

using namespace std;

template <class T>
class Histogram {

	public:
	
		// The datatype used in the histogram
		typedef T datatype;
	
		// Constructors
		Histogram (int s, T min, T max, bool isdiscret);

		// Destructor
		~Histogram () { if (bins!=NULL) delete bins; }
							
		
		// Access to the bins
		int value2index(T val);
		T & operator [] (int index) { return bins[index]; }		
		T & operator () (int index) { return bins[index]; }				
		
		// Add and remove a value - i.e. increase the histogram by one
		void add (T val);
		void remove (T val);
		
		// standard operations
		void clear();		
				
		inline int getThresholdValueFisher (int kmin, int kmax) 
		{
			return getThresholdValueFisher (kmin, kmax, NULL, NULL);
		}
		int getThresholdValueFisher (int kmin, int kmax, float *out_peakleft, float *out_peakright);
			
	private:

	public:
		int _size;
		T min, max;
		double step;
		bool discret;
		
		T *bins;			
};			

// *************************************************************
// Constructor - empty Histogram
// *************************************************************

template <class T>
inline Histogram<T>::Histogram (int s, T mi, T ma, bool d) {
   	_size = s;
   	min = (T) mi;
	max = (T) ma;
	discret = d;
	
	// The discret case
	if (discret)
		step = ((double)((double)max-(double)min+1.0))/ (double) _size;
	else
		step = ((double)((double)max-(double)min))/ (double) _size;		
		
   	bins = new T[s];   	
   	clear();
}



// *********************************************************************
// Clear the Histogram
// *********************************************************************

template <class T>
inline void Histogram<T>::clear () 
{
	for	(int i=0; i<_size; ++i)
		bins[i]=0;
}

// *********************************************************************
// Get the histogram bin index from the value
// *********************************************************************

template <class T>
int Histogram<T>::value2index (T val) 
{
	int index = (int) ((double) (val - min) / step) / 1;
	if (index>=_size) index = _size-1;
	if (index<0) index = 0;
	return index;
}

// *********************************************************************
// Add a value
// *********************************************************************

template <class T>
inline void Histogram<T>::add (T val) 
{	
	++bins[value2index(val)];
}


// *************************************************************
// Compute the optimal threshold value with the fisher method.
// The Implementation gathered from Jean-Michel Jolion
// The last two arguments are filled with mode peaks, if they
// are not null (the arguments).
// *************************************************************

template <class T>
int Histogram<T>::getThresholdValueFisher (int kmin, int kmax,
	float *out_peakleft, float *out_peakright) {

	int	k, kopt ;
	float W, m0, m1, M, S, Smax ;	
	float m1opt, m0opt;
	T *H = bins;
	
	if (kmin<0) kmin = 0;
	if (kmax>255) kmax = 255;
		
	// Search the borders
	kopt = 0 ;
	m1 = W = 0.	;
	if (kmin ==	0)
		while (H[kmin] == 0)	
			kmin++ ;
	if (kmax >=	_size-1)
		while (H[kmax]	== 0)
			kmax-- ;
	else {
	 	if	(kmax == -1) {
	 		kmax = kmin ;
	 		Smax =	H[kmin]	;
	   		
	   		for (k = kmin+1 ; k < _size-1 ; k++) {
			
				if (H[k] > Smax) {
					Smax	= H[k] ;
					kmax =	k ;	
				}
			}
		}
	}			
	while (H[kmin] == 0)
		kmin++	;
	while (H[kmax] == 0)
		kmax--	;
			
	// Masse -> M
	M =	0 ;
	for	(k = kmin ;	k <= kmax ;	k++)
		M += H[k] ;
		
	m0 = m0opt = kmin ;
	for	(k = kmin+1	; k	<= kmax	; k++) {
		m1 +=	k*H[k] ;
	  	W	+= H[k]	;
	}
	
	m1 = m1opt = m1/W ;
	kopt = kmin	;
	Smax = (W/M)*(1.-W/M)*(m1 -	m0)*(m1	- m0) ;
	
	for	(k = kmin+1	; k	< kmax ; k++) {
		m0 = m0*(M - W) +	k*H[k] ;
	  	m1 = m1 *	W -	k*H[k] ;
	  	W	-= H[k]	;
	  	m1 /=	W ;
	  	m0 /=	(M-W) ;
	  	S	= (W/M)*(1.-W/M)*(m1 - m0)*(m1 - m0) ;
	  	
	  	if (S	> Smax)	{
	  		Smax = S ;
	  		kopt =	k ;
	  		m0opt = m0;
	  		m1opt = m1;	  		
	  	}
	}
	
#ifdef DEBUG
		/*
		cerr << "threshold by Fisher: " << kopt << endl;
		cerr << "m1 = " << m1opt << ", m0 = " << m0opt << endl;
		*/
#endif
	
	if (out_peakleft != NULL) *out_peakleft = m0;
	if (out_peakright != NULL) *out_peakright = m1;
	return kopt;
}

#endif

