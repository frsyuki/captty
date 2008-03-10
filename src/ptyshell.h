/*
 * This file is part of Partty
 *
 * Copyright (C) 2008-2009 FURUHASHI Sadayuki
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

#ifndef PARTTY_PTYSHELL_H__
#define PARTTY_PTYSHELL_H__ 1

#include <unistd.h>

namespace Partty {


class ptyshell {
public:
	ptyshell(int parentwin) :
		m_parentwin(parentwin),
		m_master(-1),
		m_pid(-1) {}
	~ptyshell() {}
public:
	pid_t fork(char* cmd[]);
	int masterfd(void) const { return m_master; }
	pid_t getpid(void) const { return m_pid; }
private:
	int execute_shell(char* cmd[]);
private:
	int m_parentwin;
	int m_master;
	pid_t m_pid;
};


}  // namespace Partty

#endif /* ptyshell.h */

