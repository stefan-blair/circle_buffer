# circle_buffer

## synopsis
this project was an exercies to create a functional, atomic circle buffer data structure in C

## example
```c
// create a new circle_buffer_t object
circle_buffer_t circle_buffer;
// initialize the circle buffer.  it can hold a max of 10 ints
circle_buffer_init(&circle_buffer, 10, sizeof(int));

int i;
// write 20 times.  it will only accept the first 10.  the next 10 will fail with nonzero exit code
for (i = 0; i < 20; ++i)
{
	if (circle_buffer_write(&circle_buffer, &i) == 0)
	{
		printf("wrote %i\n", i);
	}
	else
	{
		printf("failed to write %i\n", i);
	}
 }

// read 20 times.  it will only read the first 10
for (i = 0; i < 20; ++i)
{
	int b;
	if (circle_buffer_read(&circle_buffer, &b) == 0)
	{
		printf("read %i\n", b);
	}
	else
	{
		printf("failed to read at index %i\n", i);
	}
}
```
