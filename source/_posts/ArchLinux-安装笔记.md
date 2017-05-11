---
title: 'ArchLinux 安装笔记'
date: 2016-07-17 16:01:18
tags: [Linux]
---

# 前提说明 #
建议优先选择官方文档为参考，内容随时更新且非常详细。这里记录是包含一些自己遇到的坑。且只针对自己安装需求的情况。

# 引用参考 #
> [Beginners'guide (简体中文)](https://wiki.archlinux.org/index.php/Beginners%27_guide_%28%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87%29)
> [Installation guide (简体中文)](https://wiki.archlinux.org/index.php/Installation_guide_%28%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87%29)
> [给妹子看的 Arch Linux 桌面日常安装](https://bigeagle.me/2014/06/archlinux-install-for-beginners/)
> [ArchLinux 安装笔记](https://blog.ikke.moe/posts/archlinux-installation-notes/)
> [寒假折腾Archlinux的一些经验（新手向）--桌面配置篇](http://blog.csdn.net/u011152627/article/details/18925121)
> [ArchLinux使用中常见问题集锦](http://blog.csdn.net/u011152627/article/details/38145125)

# 环境 #
机器: DELL
BOOT: UEFI
SSD: 256G
内存: 8G
CPU: i7-6500U
安装需求：本机安装单系统
ArchLinux: Release 2016.06.01
<!--more-->
# LiveUSB #
参考 [USB flash installation media (简体中文)](https://wiki.archlinux.org/index.php/USB_flash_installation_media_%28%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87%29)
推荐使用里面的手动方法，这样制作的LiveUSB可以使用剩余空间来存储其他东西。

# 安装准备 #
镜像中不包含软件包，安装的软件是通过服务器上的源下载，所以安装的时候必须要有网络连接。

* 联网
有线：
1.	用`ip addr` 查看网卡接口型号，比如 enp2s0
2.	启用网卡DHCP功能，`systemctl enable dhcpcd@enp2s0.service`
无线：
1.	`wifi-menu`
2.	选择自己的 wifi 并输入密码连接网络

最后 `ping` 一下，确认网络无误

* 更新系统时间
`timedatectl set-ntp true`

* 准备磁盘
1.	lsblk 查看自己的硬盘所在，比如我的就是 /dev/sda
2.	使用parted 分区
注意：我是要全盘安装的，所以重新建立分区表了。
a. `parted /dev/sda`
b. `(parted) mktable gpt` 重建 GPT 分区表
c. `(parted) mkpart ESP fat32 1M 513M` 分配 ESP 分区，前1M是分区表，ESP大小为512M
d. `(parted) set 1 boot on` 设置为ESP分区
e. `(parted) mkpart primary linux-swap 513M 8705M` 分配swap分区，这里使用了与我内存同样大小的8G
f. `(parted) mkpart primary ext4 8705M 100%` 分配root分区，使用剩余所有空间
3. 格式化分区
a. `mkfs.vfat –F32 /dev/sda1` ESP分区需要格式化成fat32，否则无法启动
b. `mkswap /dev/sda2 & swapon /dev/sda2` 格式化交换分区，并设置
c. `mkfs.ext4 –b 4096 /dev/sda3` 格式化root分区，并4K对齐
4. 挂载分区
a. `mount –t ext4 –o discard,noatime /dev/sda3 /mnt`
b. `mkdir –p /mnt/boot/EFI`
c. `mount /dev/sda1 /mnt/boot/EFI`

# 安装 #
* 配置安装源
默认镜像是美国的，在中国速度慢，所以全改中国了..
`sed -i '/Score/{/China/!{n;s/^/#/}}' /etc/pacman.d/mirrorlist`

* 安装基本系统
安装之前先确认是否连网
`pacstrap /mnt base base-devel vim`

* 生成 fstab
`genfstab –U –p /mnt >> /mnt/etc/fstab`

* chroot
`arch-chroot /mnt /bin/bash`

* Locale
`vim /etc/locale.gen`
取消下面这些注释
en_US.UTF-8 UTF-8
zh_CN.UTF-8 UTF-8
zh_TW.UTF-8 UTF-8

生成locale信息
`locale-gen`
`echo LANG=en_US.UTF-8 > /etc/locale.conf`

* 时间
选择时区（Shanghai）
`tzselect`

将 `/etc/localtime` 软连接到 `/usr/share/zoneinfo/Zone/SubZone`
`ln –s /usr/share/zoneinfo/Asia/Shanghai /etc/localtime`

设置时间标准为 UTC 并调整时间偏移
`hwclock –systohc –utc`

* 创建初始 ramdisk 环境
`mkinitcpio –p linux`

* 设置 root 密码
`passwd`

* 安装 grub
先df命令确认一下有木有挂载ESP分区
应该是这样的...

|File system |Mounted On  |
|:-----------|:-----------|
|/dev/sda3   |/           |
|/dev/sda1   |/boot/EFI   |
|...         |...         |

`pacman –S grub efibootmgr`
`grub-install –target=x86_64-efi –efi-directory=/boot/EFI –bootloader-id=arch_grub –recheck`
`grub-mkconfig –o /boot/grub/grub.cfg`
**注意:**有些BIOS需要自己设置EFI文件位置才能找到efi文件。比如我的DELL

* 配置网络
`echo myhostname > /etc/hostname`
并在 /etc/hosts 添加同样主机名
```bash
#<ip-address>    <hostname.domain.org>        <hostname>
127.0.0.1        localhost.localdomain        localhost        myhostname
::1              localhost.localdomain        localhost        myhostname
```
有线网络
Interface 是您的网络接口名，见连网
`systemctl enable dhcpcd@interface.service`

无线网络
`pacman –S iw wpa_supplicant dialog`

* 卸载分区并重启系统
`exit`
`umount -R /mnt`
`reboot`

# 折腾新大陆 #

重启之后就阔以以root进入到archlinux系统了，首先我们要进行[联网](#安装准备)。

* 添加用户
`useradd –m –g users –G wheel –s /bin/bash username`
`passwd username`

* sudo
`pacman –S sudo`
`vim /etc/sudoers`
找到 root ALL=(ALL) ALL
照着这个，在下面添加一个 username ALL=(ALL) ALL

* 安装 yaourt
`vim /etc/pacman.conf`
加入下面的内容:
```bash
[archlinuxcn]
# The Chinese Arch Linux communities packages.
SigLevel = Optional TrustAll
Server = http://mirrors.163.com/archlinux-cn/$arch
```
更新并安装yaourt
```bash
pacman –Syu
pacman –S yaourt
pacman –S archlinuxcn-keyring
```

* 安装 SSH、GIT、wget
`pacman –S git openssh wget`

* 安装 zsh
```bash
pacman –S zsh
chsh /bin/zsh
sh –c “$(curl –fsSL https://raw.github.com/robbyrussell/oh-my-zsh/master/tools/install.sh)”
```

* 安装 screenfetch
`pacman –S screenfetch`

* NTFS 读写
`pacman –S ntfs-3g`

* 安装解压缩软件
`pacman –S file-roller unrar unzip p7zip`

* Shadowsocks-qt5
`pacman –S shadowsocks-qt5`

* ProxyChains
`pacman –S proxychains`

* RP-PPPOE
拨号的，按需安装
`pacman –S rp-pppoe`
`nm-connection-editor`

* 安装 xorg 桌面管理器
`pacman –S xorg-xinit xorg-server xorg-twm xterm`

* 安装 gnome 桌面环境
按需，个人安装的gnome，觉得新版3.20挺好看的
`pacman –S gnome`
`pacman –S gnome-tweak-tool`

* VPN 扩展
```bash
pacman –S networkmanager-pptp
yaourt networkmanager-l2tp
systemctl restart NetworkManager
```

* 启动服务
显示管理器gnome默认是用的GDM
`systemctl enable gdm.service`

网络管理
`systemctl enable NetworkManager.service`

更新
`pacman –Syu`

* 安装 chromium
`pacman –S chromium`

* 安装输入法
依赖
`pacman –S fcitx-im fcitx-configtool fcitx-gtk3 fcitx-gtk2 fcitx-qt4 fcitx-qt5`

自行选择安装的拼音，我选择的sun
`pacman –S sunpinyin`

配置.xprofile文件
`vim ~/.xprofile`

添加如下内容
```bash
export LC_CTYPE=zh_CN.UTF-8
export XIM=fcitx
export XIM_PROGRAM=fcitx
export GTK_IM_MODULE=fcitx
export QT_IM_MODULE=fcitx
export XMODIFIERS="@im=fcitx"
eval `dbus-launch --sh-syntax --exit-with-session`
exec fcitx &
```
注意，即使这样，你会发现还是调用不出输入法…
等下重启之后告诉你如何解决~

* 安装网易云音乐
`pacman –S netease-cloud-music`

* 重启
`reboot`

# 来到新的世界 #
重启你会发现有了界面~

* 配置输入法
前面说即使安装完那些东西也调不出来，是有个地方需要配置一下
左下角可以有个后台程序栏。右键输入法，选择配置。发现输入法里面并没有拼音，我们添加进安装的 sunpinyin 就好了。
注意:切换输入法与 gnome 显示的不一致。
默认切换输入法是 `ctrl+space`

* 中文化
安装中文字体，推荐[思源黑体](https://github.com/adobe-fonts/source-han-sans)，安装方法见 Github 上的安装过程。
安装等宽字体，推荐 [Source Code Pro](https://github.com/adobe-fonts/source-code-pro/)
打开 Gnome Tweak Tool，切换到字体栏，
将窗口、界面、文档的字体改为 Source Han Sans Normal
将等宽字体设置为 Source Code Pro

* VMWare
我这里环境：
VMWare: VMware-Workstation-Full-12.1.1-3770994.x86_64.bundle
Linux: Linux 4.6.2-1-ARCH

首先从VMWare官网下载个 VMWare二进制包
安装部分详见 VMware_(简体中文)

安装依赖
`mkdir /etc/init.d`

添加VMWare服务配置文件
`yaourt vmware-systemd-services`
`systemctl enable vmware.service`
`systemctl start vmware.service`

有一个地方我要说明一下。
在启动提示有个服务跟新的时候，更新会失败，导致不能启动VMWare
注意：下面的方法不一定在你的版本适用，请注意备份。
解决方法是：

1.	进入 `/usr/lib/vmware/modules/source`
2.	解包 `vmnet.tar vmmon.tar`
3.	`Replace function "get_user_pages()" with "get_user_pages_remote()" in vmmon-only/linux/hostif.c and vmnet-only/userif.c files.`
4.	重新打包回去

具体如下：
`cd /usr/lib/vmware/modules/source`

解包
`sudo tar –xvf vmnet.tar`
`sudo tar –xvf vmmon.tar`

把下面两个文件里面的 `get_user_pages` 函数替换成 `get_user_pages_remote`
`sudo vim vmnet-only/driver.c`
`sudo vim vmmon-only/linux/hostif.c`

打包
`sudo tar -uvf vmnet.tar vmnet-only`
`sudo tar -uvf vmmon.tar vmmon-only`

然后删除那解包的文件夹
`sudo rm -r vmnet-only`
`sudo rm -r vmmon-only`

# 结束 #