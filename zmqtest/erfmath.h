

#include <stdlib.h>
#include <stdio.h>
#include <math.h>



#define	ITMAX 100
#define	EPS 3.0e-7
#define	FPMIN -1.0e-30

class erfmath 
{
public:

	erfmath();

	float gammln(float xx);
	void gser(float *gamser, float a, float x, float *gln);
	void gcf(float *gammcf, float a, float x, float *gln);
		

	float gammp (float a, float x);
	
	float erff(float x);
	float 	randf(void);
	float maxf(float a, float b);
	float randg(float sd, float mu);
	float randgt(float sd, float mu);
protected:

enum {randlen=1000000};
float *randtable;
int rcnt;

};


	
