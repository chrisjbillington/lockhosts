lockhosts kernel module: prevent modification to /etc/hosts without a reboot
----------------------------------------------------------------------------

This is a Linux kernel module that prevents changes to `/etc/hosts`. The module cannot
be unloaded, so after loading the module, one must reboot before changes to `/etc/hosts`
can be made. This can be useful for e.g. blocking social media or other websites, and
creating friction to unblock them.

Requirements
------------

Your distro's `linux-headers` package, e.g
```bash
sudo apt install linux-headers-generic # Ubuntu
sudo pacman -S linux-headers # Arch Linux
```

Building
--------
To build the module, run:
```bash
make
```
And to delete all build and temporary files, run
```bash
make clean
````

Using
-----
To load the module, run:
```bash
sudo insmod lockhosts.ko
```
To unload it, reboot your computer.

Note
----
The module will need to be recompiled after each kernel upgrade in order to work. I
prefer to just rebuild it every time by putting a function like this in my bashrc:
```bash
blocksocial ()
{
    # Uncomment lines in /etc/hosts containing "twitter" or "reddit"
    sudo sed -i '/twitter/s/^#//g' /etc/hosts;
    sudo sed -i '/reddit/s/^#//g' /etc/hosts;
    # Rebuild and load lockhosts module:
    cd /path/to/lockhosts/folder;
    make clean;
    make;
    sudo insmod lockhosts.ko;
    cd -
}
```
