/*
 * main.c
 *
 *  Created on: Oct 15, 2016
 *      Author: sblair
 */
#include <stdio.h>
#include <pthread.h>

#include "circle_buffer.h"

void handle_write(circle_buffer_t* c_buf_, int i_)
{
	if (circle_buffer_write(c_buf_, &i_) == 0)
	{
		printf("wrote %i\n", i_);
	}
	else
	{
		printf("failed to write %i\n", i_);
	}
}

void handle_read(circle_buffer_t* c_buf_, int i_)
{
	int b;
	if (circle_buffer_read(c_buf_, &b) == 0)
	{
		printf("read %i\n", b);
	}
	else
	{
		printf("failed to read at index %i\n", i_);
	}
}

void* threaded_write(void* c_buf_ptr_)
{
	circle_buffer_t* c_buf_ = (circle_buffer_t*) c_buf_ptr_;
	int i;
	for (i = 0; i < 10; ++i)
	{
		handle_write(c_buf_, i);
	}

	return NULL;
}

void unthreaded_check()
{
	circle_buffer_t circle_buffer;
	circle_buffer_init(&circle_buffer, 10, sizeof(int));
	int i;

	for (i = 0; i < 20; ++i)
	{
		handle_write(&circle_buffer, i);
	}

	for (i = 0; i < 20; ++i)
	{
		handle_read(&circle_buffer, i);
	}

	for (i = 10; i < 15; ++i)
	{
		handle_write(&circle_buffer, i);
	}

	for (i = 10; i < 20; ++i)
	{
		handle_read(&circle_buffer, i);
	}

	for (i = 15; i < 25; ++i)
	{
		handle_write(&circle_buffer, i);
	}

	for (i = 15; i < 20; ++i)
	{
		handle_read(&circle_buffer, i);
	}

	for (i = 25; i < 35; ++i)
	{
		handle_write(&circle_buffer, i);
	}
}

void threaded_check()
{
	circle_buffer_t circle_buffer;
	circle_buffer_init(&circle_buffer, 100, sizeof(int));
	pthread_t threads[10];
	int i;

	for (i = 0; i < 10; ++i)
	{
		pthread_create(&threads[i], NULL, &threaded_write, &circle_buffer);
	}

	for (i = 0; i < 10; ++i)
	{
		pthread_join(threads[i], NULL);
	}

	printf("finished\n");

	for (i = 0; i < 100; ++i)
	{
		handle_read(&circle_buffer, i);
	}
}

int main(void)
{
	threaded_check();

	return 0;
}
