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

