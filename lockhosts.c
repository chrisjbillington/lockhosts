#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenAI");
MODULE_DESCRIPTION("A kernel module to check the modified time of /etc/hosts");
MODULE_VERSION("0.1");

static struct task_struct *monitor_thread;
static time64_t load_time;

static int monitor_function(void *data) {
    struct path hosts_path;
    struct kstat hosts_stat;
    int err;

    while (!kthread_should_stop()) {
        err = kern_path("/etc/hosts", LOOKUP_FOLLOW, &hosts_path);
        if (err) {
            printk(KERN_WARNING "Failed to find /etc/hosts: %d\n", err);
        } else {
            err = vfs_getattr(&hosts_path, &hosts_stat, STATX_MTIME, AT_STATX_SYNC_AS_STAT);
            if (err) {
                printk(KERN_WARNING "Failed to get /etc/hosts attributes: %d\n", err);
            } else if (hosts_stat.mtime.tv_sec > load_time) {
                printk(KERN_INFO "modified!\n");
            }
            path_put(&hosts_path);
        }

        msleep(1000);
    }

    return 0;
}

static int __init hosts_monitor_init(void) {
    printk(KERN_INFO "Loading hosts monitor module\n");

    load_time = ktime_get_real_seconds();
    monitor_thread = kthread_run(monitor_function, NULL, "hosts_monitor");
    if (IS_ERR(monitor_thread)) {
        printk(KERN_WARNING "Failed to create monitor thread: %ld\n", PTR_ERR(monitor_thread));
        return PTR_ERR(monitor_thread);
    }

    return 0;
}

static void __exit hosts_monitor_exit(void) {
    int ret;

    ret = kthread_stop(monitor_thread);
    if (ret < 0) {
        printk(KERN_WARNING "Failed to stop monitor thread: %d\n", ret);
    } else {
        printk(KERN_INFO "unloaded!\n");
    }
}

module_init(hosts_monitor_init);
module_exit(hosts_monitor_exit);
