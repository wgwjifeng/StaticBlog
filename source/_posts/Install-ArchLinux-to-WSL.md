---
title: Install ArchLinux to WSL
comments: true
date: 2017-05-12 17:33:52
tags: [WSL]
categories: [Windows]
---

系统: Windows 10 1703

<!--more-->

## 工具准备
首先下载 [alwsl](https://github.com/alwsl/alwsl), alwsl 是一个已经做好 arch-rootfs 的开源项目, 通过作者提供的 alwsl.bat 可以很容易的安装 ArchLinux 到 WSL. 

在 `Update Apr 2017` 的版本, 有一个小问题: archlinux.ico 的下载连接失效. 所以我们要小小修改一下 alwsl.bat.

你可以选一个自己喜欢的 ico 放到 alwsl.bat 的目录. 我是在 [archlinux.ico](https://www.iconfinder.com/icons/386451/arch_linux_archlinux_icon) 下载的.

打开 alwsl.bat, 替换一条命令

```bat
bitsadmin /RAWRETURN /transfer alwsl /download /priority FOREGROUND "https://antiquant.com/alwsl/archlinux.ico" "%localappdata%\lxss\archlinux.ico"

:: 替换为

copy /V "archlinux.ico" "%localappdata%\lxss\archlinux.ico"
```

## 安装

首先你要打开开发者模式并且在 "启用或关闭 Windows 功能" 中勾选 Linux 子系统

然后执行下面命令

```bat
> cd XXXX/alwsl/
> alwsl.bat install
```

然后进入 WSL, 要修正一下证书问题, 以及根据自己需要调整自己的源 (换国内源速度会快很多)

```
pacman -Syuw
rm /etc/ssl/certs/ca-certificates.crt
update-ca-trust
pacman -Su
```

## 创建快照

alwsl 有一个我非常喜欢的功能, 就是可以创建 WSL 的快照. 这样我就再也不用担心被我搞脏 WSL 了.

个人建议在 WSL 配置前拍摄一个快照, 在 WSL 配置完成后拍摄一个快照, 最后可以根据需要选择是否删除第一次拍着的快照.

```bat
> alwsl.bat snapshot create
```

恢复和删除快照等参数, 自己运行下 alwsl 看一下最好.

## 配置过程中遇到的问题

安装完之后, 你就可以像刚刚安装完 ArchLinux 那样开始各种配置 (比如添加用户, 配置 Shell), 我在这里说一下我在配置 ArchLinux 过程中遇到的一些问题.

### pacman

问题

```bash
root@MeeSong-PC:~# pacman -Syu
pacman: error while loading shared libraries: libcrypto.so.1.0.0: cannot open shared object file: No such file or directory
```

解决方案

```bash
ln -s /usr/lib/libcrypto.so /usr/lib/libcrypto.so.1.0.0
```

### locale-gen

问题

```bash
root@MeeSong-PC:~# cat /etc/locale.gen
en_US.UTF-8 UTF-8
zh_CN.UTF-8 UTF-8
zh_TW.UTF-8 UTF-8

root@MeeSong-PC:~# locale-gen
Generating locales...
  en_US.UTF-8...localedef: ../sysdeps/unix/sysv/linux/spawni.c:360: __spawnix: Assertion `ec >= 0' failed.
/usr/sbin/locale-gen: line 41:  2361 Aborted                 (core dumped) localedef -i $input -c -f $charset -A /usr/share/locale/locale.alias $locale
```

解决方案

```bash
root@MeeSong-PC:~# cd /usr/share/i18n/charmaps/
root@MeeSong-PC:/usr/share/i18n/charmaps# gzip -dk UTF-8.gz
root@MeeSong-PC:/usr/share/i18n/charmaps# locale-gen
```

### ca-certificates.crt

问题

```bash
curl: (77) error setting certificate verify locations:
  CAfile: /etc/ssl/certs/ca-certificates.crt
  CApath: /etc/ssl/certs/

# git 有同样的问题
```

解决方案

```bash
# 重新安装一次...
pacman -S ca-certificates-utils
```

### shell

我个人喜欢用zsh作为shell, 但是在 WSL 中更改默认shell用 chsh 是无效的..  
所以, 我们要改一下 .bashrc 让它来调用 zsh

```bash
# 在 ~/.bashrc 开头添加下面命令

# Launch Zsh
if [ -t 1 ]; then
exec zsh
fi
```
