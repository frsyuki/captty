#ifndef UNIO_H__
#define UNIO_H__

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

namespace Partty {


inline size_t read_all(int fd, void *buf, size_t count) {
	char *p = (char*)buf;
	char * const endp = p + count;

	do {
		const int num_bytes = read(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return endp - p;
			}
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	return count;
}

inline size_t write_all(int fd, const void *buf, size_t count) {
	const char *p = (char*)buf;
	const char * const endp = p + count;

	do {
		const int num_bytes = write(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return endp - p;
			}
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	return count;
}


}  // namespace Partty

#endif /* unio.h */
