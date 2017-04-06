 #include "erfmath.h"



  erfmath::erfmath()
  {
  	printf("erfmath::erfmath Init rand table\n");
  	randtable = (float*)malloc(randlen*sizeof(float));
  rcnt=0;
  	for (int k=0;k<randlen;k++)
	{
		randtable[k] = randg(1.0,0.0);
	}
  }


float erfmath::gammln(float xx)
{
    double x,y,tmp,ser;
    	static double cof[6]={76.18009172947146,-86.50532032941677,
        24.01409824083091,-1.231739572450155,
        0.1208650973866179e-2,-0.5395239384953e-5};

    int j;

    y=x=xx;
    tmp=x+5.5;
    tmp -= (x+0.5)*log(tmp);
    ser=1.000000000190015;
    for (j=0;j<=5;j++) ser += cof[j]/++y;
    return -tmp+log(2.5066282746310005*ser/x);
}


void erfmath::gser(float *gamser, float a, float x, float *gln)
{
 	
	int n;
	float sum,del,ap;
	
	
	*gln=gammln(a);
	if (x <= 0.0) 
	{
		if (x,0.0)
			printf("x less than 0 gser\n");
		
		*gamser = 0.0;
		return;
	}
	else
	{
		ap = a;
		sum=1.0/a;
		del = sum;
		
		
		for (n=1; n<=ITMAX;n++)
		{
			++ap;
			del *=x/ap;
			sum+=del;
			if (fabs(del) < fabs(sum)*EPS)
			{
				*gamser=sum*exp(-x+a*log(x)-(*gln));
				return;
			}
		}
		printf("a too large- gser\n");
		return;
	}
}



void erfmath::gcf(float *gammcf, float a, float x, float *gln)
{
	int i;
	float an,b,c,d,del,h;
	
	*gln = gammln(a);
	b = x+1.0-a;
	c = 1.0/FPMIN;
	d = 1.0/b;
	h=d;
	for (i=1; i<=ITMAX;i++)
	{
		an = -i*(i-a);
		b+=2.0;
		d = an*d+b;
		if (fabs(d) < FPMIN) 
			d = FPMIN;
			
		c = b+an/c;
		
		if (fabs(c) < FPMIN) 
			c = FPMIN;
		d = 1.0/d;
		del = d * c;
		h *=del;
		if (fabs(del-1.0) < EPS)
			break;
			
			
		
	}
	if (i> ITMAX)
		printf("a too large- gcf\n");
	
	*gammcf = exp(-x+ a*log(x)-(*gln))*h;
}


float erfmath::gammp (float a, float x)
{
	float gamser, gammcf, gln;
	
	if (x<  0.0 || a <= 0.0)
		printf("bad args gammp\n");
		
	if (x< (a+ 1.0))
	{
		gser(&gamser, a, x, &gln);
		return(gamser);
	}
	else
	{
		gcf(&gammcf, a,x, &gln);
		return(1.0-gammcf);
	}
}

 
float erfmath::erff(float x)
{
	return( x< 0.0 ? -gammp(0.5,x*x) : gammp(0.5, x*x));
}

float erfmath::randf(void)
{
	int a=rand();
	float fa = (float)a;
	fa = fa / (float)RAND_MAX;
	return(fa);
}
	
float erfmath::maxf(float a, float b)
{
	return(a>b ? a : b);
}


float erfmath::randg(float sd, float mu)
{

static int iset=0;
static float gset;
float fac, rsq, v1, v2;


  iset=0;

  if (iset==0)
  {
    do
    {
     v1=2.0*randf() -1.0;
     v2=2.0*randf() -1.0;
     rsq=v1*v1 + v2*v2;
    } while(rsq>=1.0 || rsq==0.0);

  fac = sqrt(-2.0*log(rsq)/rsq);
   gset = v1*fac;
   iset = 1;
   return(mu+ sd*v2*fac);
   }
   else
   {
   	iset = 0;
	return(gset);
   }

}


float erfmath::randgt(float sd, float mu)
{
  
  if (rcnt>=randlen)
  	rcnt=0;
	
  float val=mu+sd*randtable[rcnt];
  rcnt++;
  
  
	return(val);

}
