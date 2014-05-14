/** @file libmapreduce.c */
/* 
 * CS 241
 * The University of Illinois
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <poll.h>

#include "libmapreduce.h"
#include "libds/libds.h"


static const int BUFFER_SIZE = 2048;  /**< Size of the buffer used by read_from_fd(). */


/**
 * Adds the key-value pair to the mapreduce data structure.  This may
 * require a reduce() operation.
 *
 * @param key
 *    The key of the key-value pair.  The key has been malloc()'d by
 *    read_from_fd() and must be free()'d by you at some point.
 * @param value
 *    The value of the key-value pair.  The value has been malloc()'d
 *    by read_from_fd() and must be free()'d by you at some point.
 * @param mr
 *    The pass-through mapreduce data structure (from read_from_fd()).
 */
static void process_key_value(const char *key, const char *value, mapreduce_t *mr)
{
	//printf("%s\n", value);
	datastore_t* ds = mr->ds;
	unsigned long revision = datastore_put(ds, key, value);
	if(revision == 0)
	{
		const char* old_val = datastore_get(ds, key, &revision);
		const char* reduced = mr->myreduce(value, old_val);
		revision = datastore_update(ds, key, reduced, revision);
		free((char*)old_val);
		free((char*)reduced);
	}
	free((char*)value);
	free((char*)key);
}


/**
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a
 * buffer and calls process_key_value() when for each and every key-value
 * pair that is read from the file descriptor.
 *
 * Each key-value must be in a "Key: Value" format, identical to MP1, and
 * each pair must be terminated by a newline ('\n').
 *
 * Each unique file descriptor must have a unique buffer and the buffer
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two
 * unique file descriptors, you must have two buffers that each have
 * been malloc()'d to size (BUFFER_SIZE + 1).
 *
 * Note that read_from_fd() makes a read() call and will block if the
 * fd does not have data ready to be read.  This function is complete
 * and does not need to be modified as part of this MP.
 *
 * @param fd
 *    File descriptor to read from.
 * @param buffer
 *    A unique buffer associated with the fd.  This buffer may have
 *    a partial key-value pair between calls to read_from_fd() and
 *    must not be modified outside the context of read_from_fd().
 * @param mr
 *    Pass-through mapreduce_t structure (to process_key_value()).
 *
 * @retval 1
 *    Data was available and was read successfully.
 * @retval 0
 *    The file descriptor fd has been closed, no more data to read.
 * @retval -1
 *    The call to read() produced an error.
 */
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr)
{
	/* Find the end of the string. */
	int offset = strlen(buffer);

	/* Read bytes from the underlying stream. */
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset);
	//printf("--%d\n", bytes_read);
	if (bytes_read == 0)
		return 0;
	else if(bytes_read < 0)
	{
		fprintf(stderr, "error in read.\n");
		return -1;
	}

	buffer[offset + bytes_read] = '\0';

	/* Loop through each "key: value\n" line from the fd. */
	char *line;
	while ((line = strstr(buffer, "\n")) != NULL)
	{
		*line = '\0';

		/* Find the key/value split. */
		char *split = strstr(buffer, ": ");
		if (split == NULL)
			continue;

		/* Allocate and assign memory */
		char *key = malloc((split - buffer + 1) * sizeof(char));
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char));

		strncpy(key, buffer, split - buffer);
		key[split - buffer] = '\0';

		strcpy(value, split + 2);

		/* Process the key/value. */
		//printf("%s %s\n", key, value);
		process_key_value(key, value, mr);

		/* Shift the contents of the buffer to remove the space used by the processed line. */
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer));
		buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0';
	}

	return 1;
}


/**
 * Initialize the mapreduce data structure, given a map and a reduce
 * function pointer.
 */
void mapreduce_init(mapreduce_t *mr, 
                    void (*mymap)(int, const char *), 
                    const char *(*myreduce)(const char *, const char *))
{
	mr->mymap = mymap;
	mr->myreduce = myreduce;
	mr->ds = malloc(sizeof(datastore_t));
	datastore_init(mr->ds);
	mr->worker = malloc(sizeof(pthread_t));
}

int count_values(const char **values)
{
	int i=0;
	while(values[i])
		i++;
	return i;
}

void* do_work(void* mr)
{
	mapreduce_t* dat = (mapreduce_t*)mr;
	struct epoll_event ev;
	char* buffers[128];
	int i;
	for ( i = 0; i < 128; ++i)
	{
		buffers[i] = malloc(BUFFER_SIZE+1);
		buffers[i][0] = '\0';
	}
	while(dat->pipe_ct)
	{
		epoll_wait(dat->epoll_fd, &ev, 1, -1);
		//printf("%d\n", ev.data.fd);
		int done = read_from_fd(ev.data.fd, buffers[ev.data.fd], dat);
		if(done == 0)
		{
			epoll_ctl(dat->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, NULL);
			dat->pipe_ct--;
		}
	}
	for ( i = 0; i < 128; ++i)
	{
		free(buffers[i]);
	}
	return NULL;
}


/**
 * Starts the map() processes for each value in the values array.
 * (See the MP description for full details.)
 */
void mapreduce_map_all(mapreduce_t *mr, const char **values)
{
	//Create epoll
	int epoll_fd = epoll_create(1);
	//Find number of value arrays in values array
	int val_ct = count_values(values);
	//Create 2d array of file descriptors
	mr->fds = malloc(val_ct * sizeof(int *));
	//Create the appropriate number of epoll_events
	struct epoll_event event[val_ct];
	memset(event, 0, sizeof(struct epoll_event)*val_ct);
	//For each values[i]
	int i = 0;
	while(values[i])
	{
		//Create pipe and set fds
		mr->fds[i] = malloc(2 * sizeof(int));
		pipe(mr->fds[i]);
		int read_fd = mr->fds[i][0];
		int write_fd = mr->fds[i][1];
		//printf("%d\n", write_fd);

		//Create child processes to run mymap
		pid_t pid = fork();
		if(pid == 0) //child
		{
			close(read_fd);
			dup2(write_fd, read_fd);
			write_fd = read_fd;
			mr->mymap(write_fd, values[i]);
			close(write_fd);
			exit(0);
		}
		else
		{
			close(write_fd);
		}

		//Set up epoll_event
		event[i].events = EPOLLIN;
		event[i].data.fd = read_fd;
		epoll_ctl( epoll_fd, EPOLL_CTL_ADD, read_fd, &event[i]);
		free(mr->fds[i]);
		i++;
	}
	free(mr->fds);
	//Launch worker thread
	mr->epoll_fd = epoll_fd;
	mr->pipe_ct = i;
	pthread_create(mr->worker, NULL, &do_work, (void*)mr);
}


/**
 * Blocks until all the reduce() operations have been completed.
 * (See the MP description for full details.)
 */
void mapreduce_reduce_all(mapreduce_t *mr)
{
	pthread_join(*(mr->worker), NULL);
}


/**
 * Gets the current value for a key.
 * (See the MP description for full details.)
 */
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key)
{
	unsigned long rev = 0;
	const char* val = datastore_get(mr->ds, result_key, &rev);
	return val;
}


/**
 * Destroys the mapreduce data structure.
 */
void mapreduce_destroy(mapreduce_t *mr)
{

	free(mr->worker);
	datastore_destroy(mr->ds);
	free(mr->ds);
}
