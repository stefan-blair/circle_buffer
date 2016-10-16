/*

add a feature to promot scalability (if the size of the buffer must increase, this should be supported)
add a feature for writing data that is the size of multiple chunks, like if a chunk is too small to store the data.

*/


#include "circle_buffer.h"
#include <stdio.h>				// not neccessary for purposes besides debugging
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

typedef struct circle_buffer_header_t
{
	int _unread;
	int _writing;
	int _copying;
} circle_buffer_header_t;

const size_t circle_buffer_header_size = sizeof(circle_buffer_header_t);

void circle_buffer_header_init(circle_buffer_header_t* header_)
{
	header_->_unread = 0;
	header_->_writing = 0;
	header_->_copying = 0;
}

char* circle_buffer_header_extract_data(circle_buffer_header_t* header_)
{
	return (char*)header_ + circle_buffer_header_size;
}

size_t circle_mem_dist(circle_buffer_t* c_buf_);

void circle_buffer_init(circle_buffer_t* c_buf_, size_t size_, size_t ch_size_)
{
	char* data;
	size_t len = size_;

	// ch_size needs extra space to hold the header for each piece of data
	ch_size_ += circle_buffer_header_size;
	// total allocated memory size is chunk size times chunk count
	size_ *= ch_size_;
	// allocate the data and clear it
	data = (char*) malloc(size_);
	bzero(data, size_);

	c_buf_->_start = data;
	c_buf_->_end = data + size_;
	c_buf_->_length = len;
	c_buf_->_w_pos = 0;
	c_buf_->_r_pos = 0;
	c_buf_->_size = size_;
	c_buf_->_ch_size = ch_size_;
	c_buf_->_dist = circle_mem_dist(c_buf_);
	c_buf_->_atomic = 0;
}

void circle_buffer_atomize(circle_buffer_t* c_buf_)
{
	// set the atomic flag and initialize the lock
	c_buf_->_atomic = 1;
	pthread_mutex_init(&c_buf_->_lock, NULL);
}

/*
 * given an integer offset and a circle buffer, returns a pointer
 * to the spot in the circle buffer's memory that the offset was referencing
 */
char* circle_position_to_pointer(circle_buffer_t* c_buf_, size_t pos_)
{
	return c_buf_->_start + (c_buf_->_ch_size * pos_);
}

/*
 * locks the circle buffer if it is atomic
 */
void circle_buffer_lock(circle_buffer_t* c_buf_)
{
	if (c_buf_->_atomic)
	{
		pthread_mutex_lock(&c_buf_->_lock);
	}
}

/*
 * unlocks the circle buffer if it is atomic
 */
void circle_buffer_unlock(circle_buffer_t* c_buf_)
{
	if (c_buf_->_atomic)
	{
		pthread_mutex_unlock(&c_buf_->_lock);
	}
}

/*
 * calculate the distance between the circle buffer's read and write positions
 */
size_t circle_mem_dist(circle_buffer_t* c_buf_)
{
	size_t w_pos = c_buf_->_w_pos;
	size_t r_pos = c_buf_->_r_pos;
	size_t len = c_buf_->_length;
	circle_buffer_header_t* header;
	char* pos;
	size_t dist;

	if (w_pos > r_pos)
	{
		dist = (len - w_pos) + r_pos;
	}
	else if (w_pos < r_pos)
	{
		dist = r_pos - w_pos;
	}
	else
	{
		pos = circle_position_to_pointer(c_buf_, w_pos);
		header = (circle_buffer_header_t*) pos;
		if (header->_unread)
		{
			dist = 0;
		}
		else
		{
			dist = len;
		}
	}

	return dist;
}

/*
int circle_readin( circle_buffer_t* c_buf, int fd, size_t flags = 0 ){
	int ch_size = c_buf->ch_size;
	int at = flags & CBUF_ATOMIC;
	int wr_fd = flags & CBUF_SAVE_DESCRIPTOR;
	char* w_pos = c_buf->start + (ch_size * (c_buf->w_pos - 1));		// the write position writes data to the chunk BEFORE it, and the read position reads from the chunk AFTER it.
	if ( c_buf->dist > 1 ){							// keeps a record of the last recorded distance.
		if ( wr_fd ){
			*( (int*)(w_pos + META_SIZE) ) = fd;			// then, it saves the file descriptor there.
		}
		read( fd, w_pos + SETTING_SIZE, ch_size - SETTING_SIZE );	// read in data from the file descriptor directly to the buffer.  Add four to avoid overwriting unread flag.  Subtract setting size for message size.
		*w_pos = 1;							// set the first byte at the write position to 1, marked unread.
		c_buf->dist--;							// decrement the distance from the write position to the last known read position.
		if ( c_buf->w_pos == c_buf->length ){
			c_buf->w_pos = 1;					// if the write position reaches the end of the buffer, loop it back to the beginning.
		} else {
			c_buf->w_pos++;						// increase write position.
		}
	} else {								// if the write position has reached the last recorded read position, then find the new one to avoid overwriting data.
		if ( at ){							// check if the atomic flag is set.  If it is, fetch the lock.  Else, dont bother.
			pthread_mutex_lock( c_buf->r_lock );			// Lock the reader and check current distance.
			c_buf->dist = circle_mem_dist( c_buf->w_pos, c_buf->r_pos, c_buf->length);
			pthread_mutex_unlock( c_buf->r_lock );
		} else {
			c_buf->dist = circle_mem_dist( c_buf->w_pos, c_buf->r_pos, c_buf->length);
		}
		if ( c_buf->dist == 1 ){					// if there is actually no more free space, return an error.
			printf( "No More Free Space\n" );
			return -1;
		} else {
			printf( "---------------[%i]   ( reader @ position %i, writer @ position %i )\n", c_buf->dist, c_buf->r_pos, c_buf->w_pos );
			circle_readin( c_buf, fd, at );				// if there is, start the whole process again.
		}
	}
	return 0;
}
*/

/*
 * initializes the write, and returns a pointer to the next safe spot in the
 * circle buffer to write to.  if there is no available space, it returns NULL.
 * it is atomic and makes sure that the w_pos is incremented properly
 */
char* circle_write_init(circle_buffer_t* c_buf_)
{
	char* w_pos = NULL;
	circle_buffer_header_t* header;

	circle_buffer_lock(c_buf_);

	if (c_buf_->_dist == 0)
	{
		// if the last recorded distance is 0, check what the new distance is
		c_buf_->_dist = circle_mem_dist(c_buf_);
	}

	if (c_buf_->_dist > 0)
	{
		// if there is available space, get the current write position pointer
		w_pos = circle_position_to_pointer(c_buf_, c_buf_->_w_pos);
		header = (circle_buffer_header_t*) w_pos;

		if (header->_unread || header->_copying || header->_writing)
		{
			// make sure that this write will not override another write
			w_pos = NULL;
		}
		else
		{
			/*
			 * if a valid write position was found, set the writing flag in the
			 * header and increment the circle_buffer's next write position
			 */
			circle_buffer_header_init(header);
			header->_writing = 1;

			++c_buf_->_w_pos;
			if (c_buf_->_w_pos >= c_buf_->_length)
			{
				c_buf_->_w_pos = 0;
			}
		}
	}

	circle_buffer_unlock(c_buf_);

	return w_pos;
}

int circle_buffer_write(circle_buffer_t* c_buf_, void* data_)
{
	// initialize the write by getting the write position
	char* w_pos = circle_write_init(c_buf_);
	circle_buffer_header_t* header;
	size_t ch_size;
	int error = 0;

	if (w_pos != NULL)
	{
		// if there is a valid write position, copy the data
		ch_size = c_buf_->_ch_size - circle_buffer_header_size;
		header = (circle_buffer_header_t*) w_pos;
		memcpy(circle_buffer_header_extract_data(header), data_, ch_size);

		// after the copy, update the header
		circle_buffer_lock(c_buf_);
		header->_writing = 0;
		header->_unread = 1;
		circle_buffer_unlock(c_buf_);
	}
	else
	{
		error = -1;
	}

	return error;
}

/*
 * initializes the read, and returns a pointer to the next safe spot in the
 * circle buffer to read from.  if there is no available space, it returns NULL.
 * it is atomic and makes sure that the r_pos is incremented properly
 */
char* circle_read_init(circle_buffer_t* c_buf_)
{
	char* r_pos;
	circle_buffer_header_t* header;

	circle_buffer_lock(c_buf_);

	// get a pointer to the current read position
	r_pos = circle_position_to_pointer(c_buf_, c_buf_->_r_pos);
	header = (circle_buffer_header_t*) r_pos;

	if (header->_unread == 0 || header->_copying ||  header->_writing)
	{
		/*
		 * make sure that there is available data ready.
		 * if not, r_pos is invalid
		 */
		r_pos = NULL;
	}
	else
	{
		// if r_pos is valid, update the header and the next r_pos
		header->_copying = 0;
		++c_buf_->_r_pos;
		if (c_buf_->_r_pos == c_buf_->_length)
		{
			c_buf_->_r_pos = 0;
		}
	}

	circle_buffer_unlock(c_buf_);

	return r_pos;
}

int circle_buffer_read(circle_buffer_t* c_buf_, void* buf_)
{
	char* r_pos = circle_read_init(c_buf_);
	circle_buffer_header_t* header = (circle_buffer_header_t*) r_pos;
	size_t ch_size;
	int error = 0;

	if (r_pos != NULL)
	{
		/*
		 * if the read position is valid, copy the data to the buffer, assumed
		 * to have enough space
		 */
		ch_size = c_buf_->_ch_size - circle_buffer_header_size;
		memcpy(buf_, circle_buffer_header_extract_data(header), ch_size);

		// after the read finished, update the header
		circle_buffer_lock(c_buf_);
		header->_unread = 0;
		header->_copying = 0;
		circle_buffer_unlock(c_buf_);
	}
	else
	{
		error = -1;
	}

	return error;
}
