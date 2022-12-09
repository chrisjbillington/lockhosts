#include <linux/module.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/string.h>

static struct timespec64 load_time;
static struct timer_list timer;

static void check_hosts_modified(struct timer_list *t)
{
    struct inode *inode;
    struct path path;
    struct timespec64 mtime;
    int err;

    err = kern_path("/etc/hosts", LOOKUP_FOLLOW, &path);
    // obtain the path to the file
    if (err) {
        printk(KERN_ALERT "err: %d", err);
        // printk(KERN_ALERT "%s", strerror(err));
    }
    else {
        // obtain the inode struct
        inode = path.dentry->d_inode;
        mtime = inode->i_mtime;
        if (timespec64_compare(&mtime, &load_time) > 0) {
            // panic("lockhosts: /etc/hosts was modified");
            printk(KERN_ALERT "lockhosts: would panic, /etc/hosts was modified");
        }
        else {
            printk(KERN_ALERT "lockhosts: all ok");
        }
    }
    mod_timer(&timer, jiffies + HZ);
    
}

static int __init lockhosts_init(void)
{
    ktime_get_real_ts64(&load_time);
    timer_setup(&timer, check_hosts_modified, 0);
    mod_timer(&timer, jiffies + HZ);
    return 0;
}

static void __exit lockhosts_exit(void)
{
    del_timer(&timer);
    // panic("lockhosts module unloaded\n");
    printk(KERN_ALERT "lockhosts: lockhosts module unloaded");
}

MODULE_AUTHOR("ChatGPT and Chris Billington");
MODULE_LICENSE("GPL");

module_init(lockhosts_init);
module_exit(lockhosts_exit);
