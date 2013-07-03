//
//  main.c
//  GetLuckyNum
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

char *glmaparray;
int glreturnnum[4];

void initmap(char *mapArray, int maxnum)
{
	mapArray[0] = 0;
	for(int i=3; i <= maxnum; i+=2)
	{
		if(mapArray[(i-1)>>1] == 1)
		{
			for (int j = i+i; j <= maxnum; j+=i)
			{
				if ((j&1) != 0)
					mapArray[(j-1)>>1] = 0;
			}
		}
		//printf("%d:%d\n", i, mapArray[(i-1)/2]);
	}
}

void* modulefunc(void *arg)
{
    int *ary = (int *)arg;
    int primenum = 0;
    for (int i=ary[0]; i<=ary[1]; ++i) {
        int sumone = 0, sumtwo = 0;
		int tt = i;
		while(tt > 0)
		{
			int tmp = tt%10;
			sumone += tmp;
			sumtwo += tmp*tmp;
			tt = tt/10;
		}
		if ((sumone&1) != 0 && (sumtwo&1) != 0)
		{
			if (glmaparray[(sumone-1)>>1] == 1 && glmaparray[(sumtwo-1)>>1] == 1)
			{
				++primenum;
				//printf("sumone:%d sumtwo:%d luckynum:%d\n", sumone, sumtwo, i);
				continue;
			}
		}
		primenum = (i == 11 ? primenum+1 : primenum);
    }
    glreturnnum[ary[2]] = primenum;
    pthread_exit(NULL);
}

int luckynum(int maxnum, int lnum, int rnum)
{
	int tmp = maxnum/2;
	int arysize = (((maxnum&1) == 0) ? tmp : (tmp + 1));
	char *mapArray = calloc(arysize, sizeof(char));
	memset(mapArray, 1, arysize);
	initmap(mapArray, maxnum);
    glmaparray = mapArray;
    
    pthread_t pthreads[4] = {0};
    int step = (rnum-lnum)/4;
    for (int i=0 ; i<4; ++i) {
        int nums[3];
        nums[0] = i*step + lnum;
        nums[1] = nums[0] + step;
        nums[2] = i;
        pthread_create(&pthreads[i], NULL, modulefunc, nums);
    }
    
    pthread_join(pthreads[0], NULL);
    pthread_join(pthreads[1], NULL);
    pthread_join(pthreads[2], NULL);
    pthread_join(pthreads[3], NULL);
    
    free(mapArray);
	glmaparray = mapArray = NULL;
	return (glreturnnum[0]+glreturnnum[1]+glreturnnum[2]+glreturnnum[3]);
}

int main(int argc, char const *argv[])
{
	time_t start = time(NULL);
	printf("primenum:%d\n", luckynum(729, 1, 1000000000));
	time_t end = time(NULL);
	printf("十亿 using seconds:%ld\n", end-start);
    
	// start = time(NULL);
	// max = 100000000;
	// printf("primenum:%d\n", luckynum(max));
	// end = time(NULL);
	// printf("一亿 using seconds:%ld\n", end-start);
    
	// start = time(NULL);
	// max = 10000000;
	// printf("primenum:%d\n", luckynum(max));
	// end = time(NULL);
	// printf("一千万 using seconds:%ld\n", end-start);
	
	// start = time(NULL);
	// max = 1000000;
	// printf("primenum:%d\n", luckynum(max));
	// end = time(NULL);
	// printf("一百万 using seconds:%ld\n", end-start);
    
	return 0;
}
