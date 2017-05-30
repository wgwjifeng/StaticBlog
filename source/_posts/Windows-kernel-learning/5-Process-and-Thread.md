---
title: 'Windows kernel learning: 5. Process, Thread and Jobs'
comments: true
date: 2017-05-29 10:47:32
updated: 2017-05-30 10:47:32
tags: ['Windows', 'Kernel']
categories: ['Windows kernel learning']
---

从概念上来说, 进程是线程的容器, 进程提供了线程必要的地址空间, 上下文环境, 安全凭证等等..而线程是最基本的执行单位和调度单位. 作业呢? 作业可以看作是进程的容器, 使其可以对进程进行统一的管理.

从实际上来说, 内核就是各种各样的数据结构, 进程, 线程和作业也不例外.

<!--more-->

每个 Windows 进程都是由一个执行体进程结构 `EPROCESS` 来表示的, 结构中包含了许多进程有关的属性和数据结构. `EPROCESS` 和相关的数据结构位域系统空间中, 不过进程环境块 `PEB` 是个例外, 它位于进程地址空间中.

Windows 线程是由一个执行体线程结构 `ETHREAD` 来表示的, 同样, 除了线程环境块 `TEB` 位于进程地址空间中, 其他都位于系统空间中.

对于每个执行了一个 Win32 程序的进程, Win32子系统进程 (Csrss) 为它维护了一个平行的结构 `CSR_PROCESS`. 同时也为每个线程维护了平行的结构 `CSR_THREAD`.

Windows 子系统的内核模式部分 (Win32k.sys) 有一个针对每个进程的数据结构, `W32PROCESS`. 每当一个线程第一次调用 Windows 的 USER/GDI 函数时, W32PROCESS 结构就会被创建.

{% asset_img P-T-Associated.jpg Data structures associated with processes and threads %}

## 基础

我们先来看一下相关的数据结构

### 进程相关结构

{% include_code EPROCESS lang:C EPROCESS.h %}

{% include_code KPROCESS lang:C KPROCESS.h %}

{% include_code PEB lang:C PEB.h %}

### 线程相关结构

{% include_code ETHREAD lang:C ETHREAD.h %}

{% include_code KTHREAD lang:C KTHREAD.h %}

{% include_code TEB lang:C TEB.h %}



---

未完待续...

## APC, Asynchronous Procedure Calls

内容比较多, 所以另开一篇文章单独介绍 

{% post_link Windows-kernel-learning/6-APC-Asynchronous-Procedure-Calls %}

