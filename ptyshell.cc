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
		ioctl(slave, TIOCSCTTY, 0);
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

