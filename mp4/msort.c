/** @file msort.c */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

FILE *fr;

typedef struct _qsort_args
{
	int* numarr;
	int size;
} qsort_args;

typedef struct _merge_args
{
	int* arr1, * arr2;
	int* size1,* size2;
	int** numarr;
	int pos;
} merge_args;

//compares two inputs and returns the smaller one
int cmp(const void *a, const void *b)
{
	if(*(int*)a < *(int*)b )
		return -1;
	else if(*(int*)a == *(int*)b )
		return 0;
	else
		return 1;
}

void* merge(void* ptr)
{

	int* a = ((merge_args*)ptr)->arr1;
	int* b = ((merge_args*)ptr)->arr2;
	int* sizea = ((merge_args*)ptr)->size1;
	int* sizeb = ((merge_args*)ptr)->size2;
	int** numarr = ((merge_args*)ptr)->numarr;
	int pos = ((merge_args*)ptr)->pos;

	int* newarr = malloc(( *sizea + *sizeb)*sizeof(int));

	int count = 0, matchingct = 0;
	int act = 0, bct = 0;
	while(*sizea > act && *sizeb > bct)
	{
		if(a[act] < b[bct])
		{
			newarr[count] = a[act];
			act++;
		}
		else if(a[act] == b[bct])
		{
			newarr[count] = a[act];
			act++;
			matchingct++;
		}
		else
		{
			newarr[count] = b[bct];
			bct++;
		}
		count++;
	}
	while(*sizea > act)
	{
		newarr[count] = a[act];
		act++;
		count++;
	}
	while(*sizeb > bct)
	{
		newarr[count] = b[bct];
		bct++;
		count++;
	}

	free(a);
	free(b);
	numarr[pos] = newarr;
	numarr[pos+1] = NULL;
	fprintf(stderr, "Merged %d and %d elements with %d duplicates.\n", *sizea, *sizeb, matchingct);
	return NULL;
}

void* qsort_wrapper(void* ptr)
{
	qsort( ((qsort_args*)ptr)->numarr, ((qsort_args*)ptr)->size, sizeof(int), cmp);
	fprintf(stderr, "Sorted %d elements.\n", ((qsort_args*)ptr)->size);
	return NULL;
}

//launches sorting threads
void launchSortThreads(int threadct, int** numarr, int *mainsize, int *endsize)
{
	pthread_t threads[threadct];
	int i;
	qsort_args* qargs[threadct];
	for(i = 0; i<threadct; i++)
	{
		qargs[i] = malloc(sizeof(qsort_args));
		qargs[i]->numarr = numarr[i];
		if(i == threadct-1 && *endsize > 0)
		{
			qargs[i]->size = *endsize;
		}
		else
		{
			qargs[i]->size = *mainsize;
		}
		pthread_create(&threads[i], NULL, qsort_wrapper, (void*)qargs[i]);
	}
	for(i=0; i<threadct; i++)
	{
		pthread_join(threads[i], NULL);
		free(qargs[i]);
	}
}

int launchMergeThreads(int** numarr, int *mainsize, int *endsize, int seg_ct)
{
	int threadct = seg_ct/2;
	pthread_t threads[threadct];
	merge_args* margs[threadct];

	int i, *leftsize, *rightsize, newsegct = seg_ct, newendsize = *endsize;
	for(i=0; i<threadct; i++)
	{
		leftsize = mainsize;
		if(*endsize > 0 && i == threadct-1 && seg_ct%2 == 0) 
		{
			rightsize = endsize;
			newendsize = *rightsize + *leftsize;
		}
		else {rightsize = mainsize;}
		margs[i] = malloc(sizeof(merge_args));
		margs[i]->arr1 = numarr[i*2];
		margs[i]->arr2 = numarr[(i*2)+1];
		margs[i]->size1 = leftsize;
		margs[i]->size2 = rightsize;
		margs[i]->numarr = numarr;
		margs[i]->pos = i*2;

		pthread_create(&threads[i], NULL, merge, (void*)margs[i]);
		newsegct--;
	}
	
	for(i=0; i<threadct; i++)
	{
		pthread_join(threads[i], NULL);
		free(margs[i]);
	}
	for(i=1; i<(seg_ct/2)+1; i++)
	{
		numarr[i] = numarr[i*2];
	}
	*endsize = newendsize;
	*mainsize = *mainsize * 2;
	return newsegct;
}

int main(int argc, char **argv)
{
	if(argc != 2) return 1;
	int seg_ct = strtoimax(argv[1], NULL, 10);
	
	size_t* buffer = malloc(sizeof(size_t));
	*buffer = 100;
	int* input = malloc(*buffer*sizeof(int));
	char* line = malloc(*buffer);
	int linect = 0;
	while(getline(&line, buffer, stdin) != -1)
	{
		if((unsigned int)linect == *buffer-1)
		{
			input = realloc(input, (*buffer*2)*sizeof(int));
			*buffer *= 2;
		}
		input[linect] = strtoimax(line, NULL, 10);
		linect++;
	}
	free(buffer);
	free(line);
	//calculate size of each segment
	//int equalsegs;
	int* numsegarr[seg_ct]; //pointer to the arrays of numbers, each array is a segment
	int *mainsize = malloc(sizeof(int));
	int *endsize = malloc(sizeof(int));
	if(linect%seg_ct == 0)
	{
		*mainsize = linect/seg_ct;
		*endsize = *mainsize;
		//equalsegs = seg_ct;
	}
	else
	{
		*mainsize = (linect/seg_ct) + 1;
		*endsize = linect - ((*mainsize)*(seg_ct-1));
		//equalsegs = seg_ct-1;
	}
	while (*endsize <= 0)
	{
		seg_ct--;
		*endsize = linect - ((*mainsize)*(seg_ct-1));
	}
	if(seg_ct%2 == 0 && *endsize == *mainsize)
	{
		free(endsize);
		endsize = mainsize;
	}
	//printf("seg_ct:%d *mainsize:%d *endsize:%d linect:%d\n", seg_ct, *mainsize, *endsize, linect);
	//segment data 
	int i, j;
	for(i=0; i<seg_ct; i++)
	{
		if(*endsize > 0 && i == seg_ct-1)
		{
			numsegarr[i] = malloc(sizeof(int)*(*endsize));
			for(j=0; j<*endsize; j++)
			{
				numsegarr[i][j] = input[(i*(*endsize))+j];
			}
		}
		else
		{
			numsegarr[i] = malloc(sizeof(int)*(*mainsize));
			for(j=0; j<*mainsize; j++)
			{
				numsegarr[i][j] = input[(i*(*mainsize))+j];
			}
		}
	}

	free(input);
	//printf("%d %d %d\n", *endsize, *mainsize, seg_ct);

	//sort each segment with a thread
	launchSortThreads(seg_ct, numsegarr, mainsize, endsize);
	/*
	//check result of sorting
	for(i=0; i<seg_ct; i++)
	{
		if(i<seg_ct-1)
		{
			for(j=0; j<*mainsize; j++)
				printf("%d\n", numsegarr[i][j]);
		}
		else
		{
			for(j=0; j<*endsize; j++)
				printf("%d\n", numsegarr[i][j]);
		}
		printf("\n");
	}
	*/
	while(seg_ct > 1)
	{
		//printf("%d\n", seg_ct);
		seg_ct = launchMergeThreads(numsegarr, mainsize, endsize, seg_ct);
	}

	for(i=0; i<linect; i++)
	{
		printf("%d\n", numsegarr[0][i]);
	}
	free(numsegarr[0]);
	free(mainsize);
	if(mainsize != endsize)
		free(endsize);
	return 0;

}
//push please go over

/*
int* ret;
pthread.join(id, (void**)&ret); //sets ret to value returned by thread

void* threadfunc(void* ptr); //pass in a pointer to a struct with the data you want to pass in
*/
