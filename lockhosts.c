#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/timekeeping.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Billington and GPT-4");
MODULE_DESCRIPTION("Prevent modification to /etc/hosts");
MODULE_VERSION("1.0");

static struct task_struct *monitor_thread;
static char *original_hosts_content = NULL;
static ssize_t original_hosts_size = 0;

static ssize_t read_file(const char *filename, char **content) {
    struct file *filp;
    ssize_t ret, nbytes;
    loff_t pos = 0;
    char *buf;

    filp = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        return PTR_ERR(filp);
    }

    buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!buf) {
        ret = -ENOMEM;
        goto out;
    }

    do {
        nbytes = kernel_read(filp, buf + pos, PAGE_SIZE - 1, &pos);
        if (nbytes < 0) {
            ret = nbytes;
            goto free_buf;
        }

        pos += nbytes;
        buf = krealloc(buf, pos + PAGE_SIZE, GFP_KERNEL);
        if (!buf) {
            ret = -ENOMEM;
            goto out;
        }
    } while (nbytes > 0);

    buf[pos] = '\0';
    *content = buf;
    ret = pos;

    goto out;

free_buf:
    kfree(buf);
out:
    filp_close(filp, NULL);
    return ret;
}

static int write_file(const char *filename, const char *content, ssize_t size) {
    struct file *filp;
    loff_t pos = 0;
    int ret;

    filp = filp_open(filename, O_WRONLY | O_TRUNC, 0);
    if (IS_ERR(filp)) {
        return PTR_ERR(filp);
    }

    ret = kernel_write(filp, content, size, &pos);
    filp_close(filp, NULL);

    return ret;
}

static int monitor_function(void *data) {
    int err;
    char *current_hosts_content = NULL;
    ssize_t current_hosts_size;

    while (!kthread_should_stop()) {
        current_hosts_size = read_file("/etc/hosts", &current_hosts_content);
        if (current_hosts_size < 0) {
            printk(KERN_WARNING "Failed to read /etc/hosts content: %ld\n", current_hosts_size);
        } else if (current_hosts_size != original_hosts_size || memcmp(current_hosts_content, original_hosts_content, current_hosts_size) != 0) {
            printk(KERN_INFO "/etc/hosts modified! Restoring original content.\n");
            err = write_file("/etc/hosts", original_hosts_content, strlen(original_hosts_content));
            if (err < 0) {
                printk(KERN_WARNING "Failed to restore /etc/hosts content: %d\n", err);
            } else {
                printk(KERN_INFO "Successfully restored /etc/hosts content.\n");
            }
        }
        if (current_hosts_content) {
            kfree(current_hosts_content);
        }
        msleep(1000);
    }

    return 0;
}



static int __init lockhosts_init(void) {
    printk(KERN_INFO "Loading lockhosts module\n");

    original_hosts_size = read_file("/etc/hosts", &original_hosts_content);
    if (original_hosts_size < 0) {
        printk(KERN_WARNING "Failed to read /etc/hosts content: %ld\n", original_hosts_size);
        return original_hosts_size;
    }

    monitor_thread = kthread_run(monitor_function, NULL, "lockhosts");
    if (IS_ERR(monitor_thread)) {
        printk(KERN_WARNING "Failed to create monitor thread: %ld\n", PTR_ERR(monitor_thread));
        kfree(original_hosts_content);
        return PTR_ERR(monitor_thread);
    }

    return 0;
}


// static void __exit lockhosts_exit(void) {
//     int ret;

//     ret = kthread_stop(monitor_thread);
//     if (ret < 0) {
//         printk(KERN_WARNING "Failed to stop monitor thread: %d\n", ret);
//     } else {
//         printk(KERN_INFO "Unloaded lockhosts module.\n");
//     }
//     kfree(original_hosts_content);
// }

module_init(lockhosts_init);
// module_exit(lockhosts_exit);

