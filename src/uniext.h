/*
 * This file is part of Partty
 *
 * Copyright (C) 2007-2008 FURUHASHI Sadayuki
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UNIO_H__
#define UNIO_H__

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

namespace Partty {


inline size_t read_all(int fd, void *buf, size_t count)
{
	char *p = (char*)buf;
	char * const endp = p + count;

	do {
		const ssize_t num_bytes = read(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return count - (endp - p);
			}
		} else if( num_bytes == 0 ) {
			return count - (endp - p);
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	return count;
}

inline size_t write_all(int fd, const void *buf, size_t count)
{
	const char *p = (char*)buf;
	const char * const endp = p + count;

	do {
		const ssize_t num_bytes = write(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return count - (endp - p);
			}
		} else if( num_bytes == 0 ) {
			return count - (endp - p);
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	return count;
}

inline size_t continued_blocking_read_all(int fd, void* buf, size_t count)
{
	ssize_t num_bytes;

	num_bytes = read(fd, buf, count);
	if( num_bytes >= count ) {
		return num_bytes;
	} else if( num_bytes <= 0 ) {
		if( errno != EINTR && errno != EAGAIN ) { return 0; }
		else { num_bytes = 0; }
	}

	fcntl(fd, F_SETFL, 0);

	char *p = (char*)buf + num_bytes;
	char * const endp = (char*)buf + count;

	do {
		num_bytes = read(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return count - (endp - p);
			}
		} else if( num_bytes == 0 ) {
			return count - (endp - p);
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	fcntl(fd, F_SETFL, O_NONBLOCK);

	return count;
}

inline size_t continued_blocking_write_all(int fd, const void* buf, size_t count)
{
	ssize_t num_bytes;

	num_bytes = write(fd, buf, count);
	if( num_bytes >= count ) {
		return num_bytes;
	} else if( num_bytes <= 0 ) {
		if( errno != EINTR && errno != EAGAIN ) { return 0; }
		else { num_bytes = 0; }
	}

	fcntl(fd, F_SETFL, 0);

	const char *p = (char*)buf + num_bytes;
	const char * const endp = (char*)buf + count;

	do {
		num_bytes = write(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return count - (endp - p);
			}
		} else if( num_bytes == 0 ) {
			return count - (endp - p);
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	fcntl(fd, F_SETFL, O_NONBLOCK);

	return count;
}

void initprocname(int argc, char**& argv, char**& envp);

void setprocname(const char *fmt, ...);

}  // namespace Partty

#endif /* unio.h */
