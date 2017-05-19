---
title: Windows 调试环境搭建
comments: true
date: 2017-05-12 14:02:55
tags: ['Windows', 'Debug']
categories: ['Debug']
---

在 Vista 之前, NTLDR 是 Windows 操作系统的加载程序, 它负责将 CPU 从实模式切换为保护模式, 加载内核文件和启动类型的驱动程序, 然后把执行权交给内核文件的入口函数, 即 KiSystemStartup.

从要完成的任务角度来看, NTLDR 内部又分为两个部分, 一部分负责接受执行权, 做模式切换, 硬件检查, 即启动的准备工作, 这部分通常称为 boot; 另一部分负责加载内核文件, 并为内核的运行做必要的准备, 通常称为 OsLoader.

Vista 将以上两个部分分成两个独立的程序文件, 即 BootMgr 和 WinLoad.exe

<!--more-->

![加载流程图](http://advdbg.org/img/inset/boot_flow.gif)

与调试 NTLDR 需要替换 Checked 版本的 NTLDR 不同, 在 BootMgr 和 WinLoad 内部都已经内建了调试引擎, 这与内核的做法是一致的.

在 Vista 之后的版本可调试部分有 4 个：bootmgr 模块、winload 模块、WinResume 模块以及 windows 内核模块 ntoskerl 模块. 可以同时启用这几个调试引擎, 也可以根据需要启用其中的某一个. 前三个调试引擎是根据位于内核中的内核调试引擎 (KD) 克隆出来的，它们使用与 KD 兼容的协议, 对调试器 (WinDBG) 来说, 它并不区分对方是真正的 KD 还是 KD 的克隆. 因此在 BootMgr 中断调试会话时, WinDBG提示的信息和内核退出时的信息一样的.

下面是通过 bcdedit 开启各项调试引擎的方法, 以串口为栗子

```bash
# 1. 开启 bootmgr 调试
bcdedit /set {bootmgr} bootdebug on
bcdedit /set {bootmgr} debugtype serial
bcdedit /set {bootmgr} debugport 1
bcdedit /set {bootmgr} baudrate 115200

# 2. 开启 WinLoad 调试
#    具体的 GUID 可以通过 bcdedit /enum osloader 来查看
bcdedit /set {GUID} bootdebug on

# 3. 开启 ntoskrnl 调试
#    具体的 GUID 与 WinLoad 相同
bcdedit /set {GUID} debug on

# 4. 开启 WinResume 调试
#    具体的 GUID 可以通过 bcdedit /enum resume 来查看
bcdedit /set {GUID} bootdebug on

```

另外, 在注册表中设置 DbgPrint 的默认日志过滤等级, 才能打印被过滤掉的日志

```bash
Windows Registry Editor Version 5.00  
  
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Debug PrintFilter]  
"DEFAULT"=dword:0000000f 
```
