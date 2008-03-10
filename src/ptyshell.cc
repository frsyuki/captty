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

#include "ptyshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UTIL_H
#include <util.h>
#endif
#ifdef HAVE_PTY_H
#include <pty.h>
#endif

namespace Partty {


pid_t ptyshell::fork(char* cmd[])
{
	// termios, winsizeの引き継ぎ
	struct termios tios;
	// XXX: エラー処理？m_parentwinが端末でなかったら？
	tcgetattr(m_parentwin, &tios);
	struct winsize ws;
	if( ioctl(m_parentwin, TIOCGWINSZ, &ws) < 0 ) {
		return -1;
	}

	int slave;
	if( openpty(&m_master, &slave, NULL, &tios, &ws) == -1 ) {
		return -1;
	}

	// fork
	m_pid = ::fork();
	if( m_pid < 0 ) {
		close(m_master);
		close(slave);
		return m_pid;
	} else if( m_pid == 0 ) {
		// child
		::close(m_master);
		setsid();
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		::close(slave);
		m_master = -1;
		slave = -1;
		exit( execute_shell(cmd) );
	}
	// parent
	close(slave);
	return m_pid;
}


int ptyshell::execute_shell(char* cmd[])
{
	if( cmd == NULL ) {
		char* shell = getenv("SHELL");
		char fallback_shell[] = "/bin/sh";
		if( shell == NULL ) { shell = fallback_shell; }
		char* name = strrchr(shell, '/');
		if( name == NULL ) { name = shell; } else { name += 1; }
		execl(shell, name, "-i", NULL);
		perror(shell);
		return 127;
	} else {
		execvp(cmd[0], cmd);
		perror(cmd[0]);
		return 127;
	}
}


}  // namespace Partty

