#include "ps.h"
#include <driver/ps2.h>
#include <driver/sd.h>
#include <driver/vga.h>
#include <zjunix/bootmm.h>
#include <zjunix/buddy.h>
#include <zjunix/fs/fat.h>
#include <zjunix/vfs/vfs.h>
#include <zjunix/slab.h>
#include <zjunix/time.h>
#include <zjunix/utils.h>
#include <driver/vga.h>
#include "../usr/ls.h"
#include "exec.h"
#include "myvi.h"

char ps_buffer[64];
int ps_buffer_index;

struct lock_t lk;

extern struct dentry *pwd_dentry;

void test_proc() {
    unsigned int timestamp;
    unsigned int currTime;
    unsigned int data;
    asm volatile("mfc0 %0, $9, 6\n\t" : "=r"(timestamp));
    data = timestamp & 0xff;
    while (1) {
        asm volatile("mfc0 %0, $9, 6\n\t" : "=r"(currTime));
        if (currTime - timestamp > 100000000) {
            timestamp += 100000000;
            *((unsigned int *)0xbfc09018) = data;
        }
    }
}

void testMem() {
    int i;
    int total = 100;
    //int sizeArr[200];
    int *addrArr[200];
	int size = 100;
    //for (i=0; i<total; i++) sizeArr[i] = 100;
    
    // for (i=10; i<100; i++) sizeArr[i] = 2<<12;
    // for (i=100; i<total; i++) sizeArr[i] = 4<<12;
    for (i=0; i<total; i++) addrArr[i] = kmalloc(size);
    kernel_printf("Allocate %d blocks sized %d byte\n", total, size);
    // bootmap_info("bootmm");
    buddy_info();
    // kernel_getkey();
    
    for (i=0; i<total; i++) kfree(addrArr[i]);
    kernel_printf("Deallocate\n");
    // bootmap_info("bootmm");
    buddy_info();
}

void testMem2() {
//     static int i;
//     static int total = 10;
//     static int sizeArr[200];
//     static int *addrArr[200];
//     for (i=0; i<total; i++) sizeArr[i] = 4096;
    
//     // for (i=10; i<100; i++) sizeArr[i] = 2<<12;
//     // for (i=100; i<total; i++) sizeArr[i] = 4<<12;
//     for (i=0; i<total; i++) addrArr[i] = kmalloc(sizeArr[i]);
//     kernel_printf("Allocate %d blocks sized %d byte\n", 20, 4096);
//     // bootmap_info("bootmm");
//     buddy_info();
//     // kernel_getkey();
    
//     for (i=0; i<total; i++) kfree(addrArr[i]);
//     kernel_printf("Deallocate\n");
//     // bootmap_info("bootmm");
//     buddy_info();
}

void test_sync() {
    int i, j;
    lockup(&lk);
    for (i=0; i<10; i++) {
        kernel_printf("%d%d%d\n", i, i, i);
        //kernel_getchar();
        for (j=0; j<100000; j++){

        }
    }
    unlock(&lk);
    while (1) {

    }
}

int sync_demo_create() {
    // init_lock(&lk);
    // int asid = pc_peek();
    // if (asid < 0) {
    //     kernel_puts("Failed to allocate pid.\n", 0xfff, 0);
    //     return 1;
    // }
    // unsigned int init_gp;
    // asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
    // pc_create(asid, test_sync, (unsigned int)kmalloc(4096), init_gp, "sync1");
    
    // asid = pc_peek();
    // if (asid < 0) {
    //     kernel_puts("Failed to allocate pid.\n", 0xfff, 0);
    //     return 1;
    // }
    // asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
    // pc_create(asid, test_sync, (unsigned int)kmalloc(4096), init_gp, "sync2");
    // return 0;
}



// int proc_demo_create() {
//     int asid = pc_peek();
//     if (asid < 0) {
//         kernel_puts("Failed to allocate pid.\n", 0xfff, 0);
//         return 1;
//     }
//     unsigned int init_gp;
//     asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
//     pc_create(asid, test_proc, (unsigned int)kmalloc(4096), init_gp, "test");
//     return 0;
// }

void ps() {
    kernel_printf("Press any key to enter shell.\n");
    kernel_getchar();
    char c;
    ps_buffer_index = 0;
    ps_buffer[0] = 0;
    kernel_clear_screen(31);
    // kernel_puts("PS>", VGA_WHITE, VGA_BLACK);
    // kernel_puts("PowerShell\n", VGA_WHITE, VGA_BLACK);
    kernel_puts("PS", VGA_GREEN, VGA_BLACK);
    kernel_puts(":", VGA_WHITE, VGA_BLACK);
    kernel_puts(pwd_dentry->d_name.name, VGA_YELLOW, VGA_BLACK);
    kernel_puts(">", VGA_WHITE, VGA_BLACK);
    while (1) {
        c = kernel_getchar();
        if (c == '\n') {
            ps_buffer[ps_buffer_index] = 0;
            if (kernel_strcmp(ps_buffer, "exit") == 0) {
                ps_buffer_index = 0;
                ps_buffer[0] = 0;
                kernel_printf("\nPowerShell exit.\n");
            } else
                parse_cmd();
            ps_buffer_index = 0;
            // kernel_puts("PS>", VGA_WHITE, VGA_BLACK);
            // kernel_puts("PowerShell\n", VGA_WHITE, VGA_BLACK);
            kernel_puts("PS", VGA_GREEN, VGA_BLACK);
            kernel_puts(":", VGA_WHITE, VGA_BLACK);
            kernel_puts(pwd_dentry->d_name.name, VGA_YELLOW, VGA_BLACK);
            kernel_puts(">", VGA_WHITE, VGA_BLACK);
        } else if (c == 0x08) {
            if (ps_buffer_index) {
                ps_buffer_index--;
                kernel_putchar_at(' ', 0xfff, 0, cursor_row, cursor_col - 1);
                cursor_col--;
                kernel_set_cursor();
            }
        } else {
            if (ps_buffer_index < 63) {
                ps_buffer[ps_buffer_index++] = c;
                kernel_putchar(c, 0xfff, 0);
            }
        }
    }
}

void parse_cmd() {
    unsigned int result = 0;
    char dir[32];
    char c;
    kernel_putchar('\n', 0, 0);
    char sd_buffer[8192];
    int i = 0;
    char *param;
    for (i = 0; i < 63; i++) {
        if (ps_buffer[i] == ' ') {
            ps_buffer[i] = 0;
            break;
        }
    }
    if (i == 63){
        ps_buffer[62] = 0;
        param = ps_buffer + 62;
    }
    else
        param = ps_buffer + i + 1;
    if (ps_buffer[0] == 0) {
        return;
    } else if (kernel_strcmp(ps_buffer, "clear") == 0) {
        kernel_clear_screen(31);
    } else if (kernel_strcmp(ps_buffer, "echo") == 0) {
        kernel_printf("%s\n", param);
    } else if (kernel_strcmp(ps_buffer, "gettime") == 0) {
        char buf[10];
        get_time(buf, sizeof(buf));
        kernel_printf("%s\n", buf);
    } else if (kernel_strcmp(ps_buffer, "sdwi") == 0) {
        for (i = 0; i < 512; i++)
            sd_buffer[i] = i;
        sd_write_block(sd_buffer, 23128, 1);
        kernel_puts("sdwi\n", 0xfff, 0);
    } else if (kernel_strcmp(ps_buffer, "sdr") == 0) {
        for (i = 0; i < 512; i++)
            sd_buffer[i] = 0;

        i = sd_read_block(sd_buffer, 23128, 1);

        kernel_printf("read_result: %d\n", i);
        for (i = 0; i < 512; i++) {
            kernel_printf("%d ", sd_buffer[i]);
        }
        kernel_putchar('\n', 0xfff, 0);
    } else if (kernel_strcmp(ps_buffer, "sdwz") == 0) {
        for (i = 0; i < 512; i++) {
            sd_buffer[i] = 0;
        }
        sd_write_block(sd_buffer, 23128, 1);
        kernel_puts("sdwz\n", 0xfff, 0);
    } else if (kernel_strcmp(ps_buffer, "mminfo") == 0) {
        bootmap_info("bootmm");
        buddy_info();
    } else if (kernel_strcmp(ps_buffer, "mmtest") == 0) {
        kernel_printf("kmalloc : %x, size = 1KB\n", kmalloc(1024));
    } else if (kernel_strcmp(ps_buffer, "ps") == 0) {
    //    result = print_proc();
        result = 0;
        disable_interrupts();
  //      print_task();
        print_sched();
        // print_exited();
        // print_wait();
        enable_interrupts();
        kernel_printf("ps return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "kill") == 0) {
        int pid = param[0] - '0';
        kernel_printf("Killing process %d\n", pid);
        result = pc_kill(pid);
        kernel_printf("kill return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "time") == 0) {
        unsigned int init_gp;
        asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
//       pc_create(2, system_time_proc, (unsigned int)kmalloc(4096), init_gp, "time");
    }
    // else if (kernel_strcmp(ps_buffer, "proc") == 0) {
    //     result = proc_demo_create();
    //     kernel_printf("proc return with %d\n", result);
    // } 
    else if (kernel_strcmp(ps_buffer, "vi") == 0) {
        result = myvi(param);
        kernel_printf("vi return with %d\n", result);
    }
    else if (kernel_strcmp(ps_buffer, "mt") == 0) {
        testMem();
        kernel_printf("Memory test return with 0\n");
    }
    else if (kernel_strcmp(ps_buffer, "mt2") == 0) {
        testMem2();
        kernel_printf("Memory test2 return with 0\n");
    }
    
    else if (kernel_strcmp(ps_buffer, "execk") == 0) {
        //debug
        kernel_printf("Enter execk\n");
        //debug
        result = exec_from_kernel(1, (void*)param, 0, 0);
        kernel_printf("execk return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "execk2") == 0) {
        result = exec_from_kernel(1, (void*)param, 1, 0);
        kernel_printf(ps_buffer, "execk2 return with %d\n");
    } else if (kernel_strcmp(ps_buffer, "vm") == 0) {
        struct mm_struct* mm = mm_create();
        kernel_printf("Create succeed. %x\n", mm);
        mm_delete(mm);        
    } else if (kernel_strcmp(ps_buffer, "execk3") == 0) {
        result = exec_from_kernel(1, (void*)param, 0, 1);
        kernel_printf(ps_buffer, "execk3 return with %d\n",result);
    } else if (kernel_strcmp(ps_buffer, "ey") == 0) {
        result = exec_from_kernel(1, "/seg.bin", 0, 1);
        kernel_printf(ps_buffer, "execk3 return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "es") == 0) {
        result = exec_from_kernel(1, "/syscall.bin", 0, 1);
        kernel_printf(ps_buffer, "execk3 return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "tlb") == 0) {
        // result = tlb_test();
        // kernel_printf(ps_buffer, "tlb_test return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "sync") == 0) {
        result = sync_demo_create();
        kernel_printf("proc return with %d\n", result);
    }

    else if (kernel_strcmp(ps_buffer, "cat") == 0) {
        result = vfs_cat(param);
    }
	else if (kernel_strcmp(ps_buffer, "cat-n") == 0) {
		kernel_printf("1 xiuhaibo\n2 zhangxiyuan\n3 tianye");
		//kernel_printf("#include <stdio.h>\nint main() {\nprintf(\"Hello World!\n\")\;\nreturn 0\;\n}");
	}
    else if (kernel_strcmp(ps_buffer, "rm") == 0) {
        result = vfs_rm(param);
    }
    else if (kernel_strcmp(ps_buffer, "ls") == 0) {
        result = vfs_ls(param);
    }
    else if (kernel_strcmp(ps_buffer, "cd") == 0) {
        result = vfs_cd(param);
	}
	

    else {
        kernel_puts(ps_buffer, 0xfff, 0);
        kernel_puts(": command not found\n", 0xfff, 0);
    }
}
