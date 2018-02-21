
#include "stopWatch.h"


stopWatch::stopWatch()
{
	currenttime=0.0;

}
/**************************************************************************
 *
 * wait x us
 *
 **************************************************************************/
void stopWatch::wait(int us)
{
	int t0,t1;

	t0 = clock();

	t1 = us*CLOCKS_PER_SEC/1000000;

	while ( (clock()-t0) < t1)
	{
	}


}



/**************************************************************************
 *
 * stat stop watch
 *
 **************************************************************************/
void stopWatch::tic()
{

	 currenttime = (double)clock()/ (double)CLOCKS_PER_SEC;

}



/**************************************************************************
 *
 * read stop watch
 *
 **************************************************************************/
double stopWatch::toc()
{
	     	  elapsedtime =((double)clock() / (double)CLOCKS_PER_SEC) - currenttime;
			  return(elapsedtime);


}


bool stopWatch::isElapsed(double sec)
{
	if (toc()>sec)
		return(true);

	return(false);

}