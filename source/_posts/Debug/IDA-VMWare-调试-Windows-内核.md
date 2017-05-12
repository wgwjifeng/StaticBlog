---
title: IDA + VMWare 调试 Windows 内核
comments: true
date: 2017-05-12 14:05:41
tags: ['Windows', 'Debug']
categories: ['Debug']
---

1. 打开虚拟机 vmx 文件, 增加下面内容

    ```Ini
    debugStub.listen.guest32.remote = "TRUE"  // 默认端口 8832
    debugStub.listen.guest64.remote = "TRUE"  // 默认端口 8864
    monitor.debugOnStartGuest32 = "TRUE"      // 在第一条指令 (在BIOS! 中警告) 中断进入调试存根, 这将在第一条指令在0xFFFF0处停止VM, 您可以设置下一个断点来破坏* 0x7c00引导加载程序由BIOS加载
    debugStub.hideBreakpoints = "TRUE"        // 启用使用硬件断点而不是软件（INT3）断点
    bios.bootDelay = "3000"                   // 延迟启动BIOS代码
    ```

2. IDA -> Debugger -> Attach -> Remote GDB debugger

    hostname 填 `localhost`, port 填上面给出的默认端口

3. Alt + S, 设置内存布局 (0 ~ 0xFFFFFFF0 or x64: 0xFFFFFFFFFFFFFFF0)

4. 如果是要调试 BIOS 代码, 那么应创建一个从 0xF0000 到 0x10000 的 16 位段
