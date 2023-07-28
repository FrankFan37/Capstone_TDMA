//#include <bpf/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>

/*
Reference:
http://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/01/18/ebpf-in-c
*/

int bpf_prog(void *ctx) {
    char buf[] = "Hello World!\n";
    bpf_trace_printk(buf, sizeof(buf));
    return 0;
    }
    

// gcc test_ebpf.c -o test_ebpf