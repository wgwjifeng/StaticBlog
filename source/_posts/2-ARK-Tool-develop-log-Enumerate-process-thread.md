---
title: '[2] ARK-Tool develop log : Enumerate Process & Thread.'
date: 2016-02-09 10:48:10
tags: [内核,ARK,Windows]
---

# 枚举 Process & Thread

按照惯例，玩儿这些东西总是从进程开始哒，那么我们今儿就说一下枚举进程&线程~

在R3，我们阔以用以下方法来遍历进程&线程：

1. ToolHelp
2. ZwQuerySystemInformation

但是到了R0，就阔以用各种方法来实现：

1. ZwQuerySystemInformation
2. 遍历 活动进程链（PEPROCESS->ActiveProcessLinks）
3. 通过 PsLookupProcessByProcessId 搜索
4. 遍历 PspCidTable 句柄表

<!--more-->

###### ZwQuerySystemInformation

这个算是标准的方法，网上一堆，不再赘述。

###### 遍历进程活动链表

什么是活动进程链？

> EPROCESS 块中有一个 ActiveProcessLinks 成员，它是一个 PLIST_ENTRY 结构的双向链表。当一个新进程建立的时候父进程负责完成 EPROCESS 块，然后把 ActiveProcessLinks 链接到一个全局内核变量 PsActiveProcessHead 链表中。
> 
> 在 PspCreateProcess 内核API中能清晰的找到：
> 
> `InsertTailList (&PsActiveProcessHead, &Process->ActiveProcessLinks);`
> 
> 当进程结束的时候，该进程EPROCESS结构从活动进程链上摘除。（但是 EPROCESS 结构不一定马上释放。）
> 
> 在 PspExitProcess 内核API中能看到
> 
> `RemoveEntryList(&Process->ActiveProcessLinks);`
> [遍历进程活动链表（ActiveProcessLinks）、DKOM隐藏进程](http://www.blogfshare.com/activeprocesslinks-dkom.html)

ZwQuerySystemInformation 就是遍历这个链表来实现的。

由于很容易摘链来隐藏进程，所以这里只是简单的说下。

有一个要注意的地方，就是链表里面存的是 EPROCESS->ActiveProcessLinks

所以要得到EPROCESS就要减去这个偏移。

``` 
kd> dt nt!_EPROCESS 0xfffffa80`03510f80
   +0x000 Pcb              : _KPROCESS
   +0x160 ProcessLock      : _EX_PUSH_LOCK
   +0x168 CreateTime       : _LARGE_INTEGER 0x0
   +0x170 ExitTime         : _LARGE_INTEGER 0x00000003`00000006
   +0x178 RundownProtect   : _EX_RUNDOWN_REF
   +0x180 UniqueProcessId  : 0xfffffa80`03511010 Void
   +0x188 ActiveProcessLinks : _LIST_ENTRY [ 0xfffff8a0`014aac28 - 0xfffffa80`03511138 ]
   ...
   
kd> ? 0xfffffa80`02de4ab8 - 0x188
Evaluate expression: -6047265830608 = fffffa80`02de4930
kd> !object fffffa80`02de4930
Object: fffffa8002de4930  Type: (fffffa800184aa20) Process
    ObjectHeader: fffffa8002de4900 (new version)
    HandleCount: 1  PointerCount: 25
```

###### 通过 PsLookupProcessByProcessId 搜索

这个方案虽然简单，但相对于以上两种方法，更推荐使用这种方案。

原因就是 活动进程链 太简单，很容易被摘掉。

而 PsLookupProcessByProcessId 是通过遍历 PspCidTable 来实现。

进程要逃避检测，必须从 PspCidTable 中删除自身对象，句柄项被用 NULL 替代。但当系统关闭进程的时候，它将找到 PspCidTable 并且得到一个 NULL 对象指针，这将导致蓝屏。隐藏进程在被终止之前必须调用 PsSetCreateProcessNotifyRoutine 安装一个回调避免BSOD，但实现方法难度相对来说略高。

当然，也有我不并不知道的方法，请各位童鞋告知~共同探讨学习~~

**注意：可以通过HOOK来解决掉这个函数，所以阔以自己遍历 PspCidTable 来尽量避免隐藏进程**

``` 
template <typename F>
static NTSTATUS EnumProcess(
    _In_ F aCallBack
    )
{
   NTSTATUS vStatus    = STATUS_UNSUCCESSFUL;
   PEPROCESS vProcess  = NULL;

   for (Size_t i = 4; i < 262144; i += 4) // 262144 = 2^18
   {
        // 遍历线程同理，换成 PsLookupThreadByThreadId
        vStatus = PsLookupProcessByProcessId((HANDLE)i, &vProcess);
        if (NT_SUCCESS(vStatus))
        {
            vStatus = aCallBack(vProcess);
            ObDereferenceObject(vProcess);

            if (STATUS_SUCCESS == vStatus)
            {
               break;
            }
        }
    }
    return vStatus;
}
```

###### 遍历 PspCidTable 句柄表

这部分内容留到下一篇文章 “检测隐藏进程” ~

我先去研究研究（逃~
