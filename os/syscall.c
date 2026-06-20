#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"

uint64 sys_write(int fd, char *str, uint len)
{
	debugf("sys_write fd = %d str = %x, len = %d", fd, str, len);
	if (fd != STDOUT)
		return -1;
	for (int i = 0; i < len; ++i) {
		console_putchar(str[i]);
	}
	return len;
}

__attribute__((noreturn)) void sys_exit(int code)
{
	exit(code);
	__builtin_unreachable();
}

uint64 sys_sched_yield()
{
	yield();
	return 0;
}

uint64 sys_gettimeofday(TimeVal *val, int _tz)
{
	uint64 cycle = get_cycle();
	val->sec = cycle / CPU_FREQ;
	val->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	return 0;
}

/*
* LAB1: you may need to define sys_trace here
*/
uint64 sys_trace(int trace_request, unsigned long id, uint8 data)
{
    struct proc *p = curr_proc();
    
    if (trace_request == 0) {
        // 读取 id 地址处的一个字节
        uint8 *ptr = (uint8 *)id;
        return *ptr;
    } else if (trace_request == 1) {
        // 写入 data 到 id 地址处
        uint8 *ptr = (uint8 *)id;
        *ptr = data;
        return 0;
    } else if (trace_request == 2) {
        // 查询系统调用次数
        if (id < MAX_SYSCALL_NUM) {
            return p->syscall_times[id];
        }
        return -1;
    }
    
    return -1;
}

extern char trap_page[];

void syscall()
{
	struct trapframe *trapframe = curr_proc()->trapframe;
	int id = trapframe->a7, ret;
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	tracef("syscall %d args = [%x, %x, %x, %x, %x, %x]", id, args[0],
	       args[1], args[2], args[3], args[4], args[5]);
	/*
	* LAB1: you may need to update syscall counter here
	*/
	if (id >= 0 && id < MAX_SYSCALL_NUM) {
        curr_proc()->syscall_times[id]++;
    }
	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], (char *)args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	/*
	* LAB1: you may need to add SYS_trace case here
	*/
	case SYS_trace:
        ret = sys_trace((int)args[0], (unsigned long)args[1], (uint8)args[2]);
        break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}
