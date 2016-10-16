#include <pthread.h>


typedef struct circle_buffer_t {
	// a pointer to the start of the allocated data
	char* _start;
	// a pointer to the end of the allocated data
	char* _end;

	//holds the distance between the read and write position
	size_t _dist;
	// the writer cursor position
	size_t _w_pos;
	// the reader cursor position
	size_t _r_pos;

	// READONLY
	// the total number of elements the circle buffer can store
	size_t _length;
	// the total size of the allocated memory
	size_t _size;
	// the size of an individual chunk of data
	size_t _ch_size;

	// flags that the circle buffer should be treated atomically
	int _atomic;
	// the lock used by the circle buffer to ensure atomicity
	pthread_mutex_t _lock;
} circle_buffer_t;

// make header, remove the write +1 index thingy

// initialize the circle buffer to default values
void circle_buffer_init(circle_buffer_t* c_buf, size_t size_, size_t ch_size_);
// mark/initialize the circle buffer as atomic
void circle_buffer_atomize(circle_buffer_t* c_buf_);

// this function also can store its file descriptor in its settings.  This will be stored 4 bytes before the message
//int circle_readin(circle_buffer_t* c_buf, int fd, size_t flags);
// write data to the circle buffer
int circle_buffer_write(circle_buffer_t* c_buf_, void* data_);
// read data from the circle buffer
int circle_buffer_read(circle_buffer_t* c_buf_, void* data_);
