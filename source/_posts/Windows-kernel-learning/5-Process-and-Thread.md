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

{% include_code TEB lang:C TIB.h %}

### [Ps]GetCurrent[Thread | Process]

我们先看下内核层函数

```C
#define PsGetCurrentThread() ((PETHREAD)KeGetCurrentThread())

__forceinline struct _KTHREAD * KeGetCurrentThread (VOID)
{
#ifdef _WIN64
    return (struct _KTHREAD *)__readgsqword(FIELD_OFFSET(KPCR, Prcb.CurrentThread));
#else
    return (struct _KTHREAD *)__readfsdword(FIELD_OFFSET(KPCR, PrcbData.CurrentThread));
#endif
}
```

因为 KTHREAD 是 ETHREAD 的第一个字段, 他俩的地址是一样的, 所以获取 KTHREAD 就相当于获取 ETHREAD 了.

我们看到 CurrentThread 是从 pcr 的 prcb 里面获取的. 
在 x86 系统的内核层中, KPCR 结构由 fs:[0] 获取.
在 x64 系统的内核层中, KPCR 结构由 gs:[0] 获取.

```C
#define PsGetCurrentProcess() _PsGetCurrentProcess()

#define _PsGetCurrentProcess() (CONTAINING_RECORD(((KeGetCurrentThread())->ApcState.Process),EPROCESS,Pcb))
```

KTHREAD 结构中, 本来就有个指向 KPROCESS 的指针 Process, 为什么还要用 ApcState 内部的 Process 呢?  
因为要考虑到进程的 Attach. 一个进程可能会 Attach 到另一个进程上, 用以访问 Attach 进程的用户空间.
这样, 在常态下, PsGetCurrentProcess 返回的是当前线程所属进程的 EPROCESS 结构指针, 
而在 Attach 状态下, 返回的是当前线程所 Attach 进程的 EPROCESS 指针.

接下来我们看下用户层函数:

```C
HANDLE WINAPI GetCurrentProcess(VOID)
{
    return (HANDLE)NtCurrentProcess();
}

HANDLE WINAPI GetCurrentThread(VOID)
{
    return (HANDLE)NtCurrentThread();
}

#define NtCurrentProcess()                      ((HANDLE)(LONG_PTR)-1)
#define NtCurrentThread()                       ((HANDLE)(LONG_PTR)-2)
```

我们看到, -1 是用来表示当前进程的句柄, -2 用来表示当前线程的句柄.

再来看下用户层的 TEB 获取

```C
FORCEINLINE struct _TEB * NtCurrentTeb(void)
{
#ifdef _WIN64
    return (struct _TEB *)__readgsqword(FIELD_OFFSET(NT_TIB, Self));
#else
    return (struct _TEB *)__readfsqword(FIELD_OFFSET(NT_TIB, Self));
#endif
}
```

与内核层不同, 在用户层:  
x86 系统上 fs:[0] 指向 TEB 结构  
x64 系统上 gs:[0] 指向 TEB 结构

通过 Self 字段获取结构指针. 得到 TEB 进而可以得到 PEB

## Windows 进程的用户空间

在 Windows 内核中, 定义了几个全局变量来说明地址空间的范围: MmSystemRangeStart, MmUserProbeAddress 和 MmHighestUserAddress.  
这几个变量在 WRK 中, 是由 MmInitSystem 函数初始化的. 在 Win8.1 换了地方, 而且有些定义上的变化, 我根据 Win8.1 做了小小修改.

```C
/*WRK1.2*/

#if defined(_WIN64)
#define MM_KSEG0_BASE  0xFFFF800000000000UI64
#else
#define KSEG0_BASE 0x80000000
#endif

#define MM_SYSTEM_SPACE_END 0xFFFFFFFFFFFFFFFFUI64

#define MI_HIGHEST_USER_ADDRESS (PVOID) (ULONG_PTR)((0x800000000000 - 0x10000 - 1)) // highest user address
#define MI_USER_PROBE_ADDRESS ((ULONG_PTR)(0x800000000000UI64 - 0x10000)) // starting address of guard page
#define MI_SYSTEM_RANGE_START (PVOID)(MM_KSEG0_BASE) // start of system space

/*-----*/

BOOLEAN MmInitSystem (IN ULONG Phase, IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ...
#if defined(_WIN64)

        MmHighestUserAddress = MI_HIGHEST_USER_ADDRESS;
        MmUserProbeAddress = MI_USER_PROBE_ADDRESS;
        MmSystemRangeStart = MI_SYSTEM_RANGE_START;
#else
        MmHighestUserAddress = (PVOID)(KSEG0_BASE - 0x10000 - 1);
        MmUserProbeAddress = KSEG0_BASE - 0x10000;
        MmSystemRangeStart = (PVOID)KSEG0_BASE;

#endif
    ...
}
```

MmSystemRangeStart 就是系统空间与用户空间的分界线.
MmHighestUserAddress 是应用软件在用户层可以访问的最高地址. 从 MmUserProbeAddress 开始, 就不让访问了.
这是因为在分界线下面留了 64KB(0x10000) 的隔离区.

### SharedUserData

SharedUserData 也要说一下, 我们先看一下定义

```C
/* Ring 0 */

#if defined(_WIN64)
#define KI_USER_SHARED_DATA 0xFFFFF78000000000UI64
#define SharedUserData      ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)
#else
#define KI_USER_SHARED_DATA 0xFFDF0000
#define SharedUserData      ((KUSER_SHARED_DATA * const) KI_USER_SHARED_DATA)
#endif

/* Ring 3 */

#define USER_SHARED_DATA    (0x7FFE0000)
#define SharedUserData      ((KUSER_SHARED_DATA *)USER_SHARED_DATA)
```

我们看到在 Ring0 中的定义的地址按理来说应该是属于系统空间, 却同时又映射到用户空间地址, 目的是用来让用户空间的程序访问内核中的一些数据.
而且, 这个区间是由系统空间和所有用户空间共享, 即为所有进程所共享的.

我们来看一下 `KUSER_SHARED_DATA` 结构

```x86asm
1: kd> dtx _KUSER_SHARED_DATA 0xFFFFF78000000000
(*((_KUSER_SHARED_DATA *)0xfffff78000000000))                 [Type: _KUSER_SHARED_DATA]
    [+0x000] TickCountLowDeprecated : 0x0 [Type: unsigned long]
    [+0x004] TickCountMultiplier : 0xfa00000 [Type: unsigned long]
    [+0x008] InterruptTime    [Type: _KSYSTEM_TIME]
    [+0x014] SystemTime       [Type: _KSYSTEM_TIME]
    [+0x020] TimeZoneBias     [Type: _KSYSTEM_TIME]
    [+0x02c] ImageNumberLow   : 0x8664 [Type: unsigned short]
    [+0x02e] ImageNumberHigh  : 0x8664 [Type: unsigned short]
    [+0x030] NtSystemRoot     : "C:\Windows" [Type: wchar_t [260]]
    [+0x238] MaxStackTraceDepth : 0x0 [Type: unsigned long]
    [+0x23c] CryptoExponent   : 0x0 [Type: unsigned long]
    [+0x240] TimeZoneId       : 0x0 [Type: unsigned long]
    [+0x244] LargePageMinimum : 0x200000 [Type: unsigned long]
    [+0x248] AitSamplingValue : 0x0 [Type: unsigned long]
    [+0x24c] AppCompatFlag    : 0x0 [Type: unsigned long]
    [+0x250] RNGSeedVersion   : 0xd [Type: unsigned __int64]
    [+0x258] GlobalValidationRunlevel : 0x0 [Type: unsigned long]
    [+0x25c] TimeZoneBiasStamp : 10 [Type: long]
    [+0x260] Reserved2        : 0x0 [Type: unsigned long]
    [+0x264] NtProductType    : NtProductWinNt (1) [Type: _NT_PRODUCT_TYPE]
    [+0x268] ProductTypeIsValid : 0x1 [Type: unsigned char]
    [+0x269] Reserved0        [Type: unsigned char [1]]
    [+0x26a] NativeProcessorArchitecture : 0x9 [Type: unsigned short]
    [+0x26c] NtMajorVersion   : 0x6 [Type: unsigned long]
    [+0x270] NtMinorVersion   : 0x3 [Type: unsigned long]
    [+0x274] ProcessorFeatures [Type: unsigned char [64]]
    [+0x2b4] Reserved1        : 0x7ffeffff [Type: unsigned long]
    [+0x2b8] Reserved3        : 0x80000000 [Type: unsigned long]
    [+0x2bc] TimeSlip         : 0x0 [Type: unsigned long]
    [+0x2c0] AlternativeArchitecture : StandardDesign (0) [Type: _ALTERNATIVE_ARCHITECTURE_TYPE]
    [+0x2c4] AltArchitecturePad [Type: unsigned long [1]]
    [+0x2c8] SystemExpirationDate : {0} [Type: _LARGE_INTEGER]
    [+0x2d0] SuiteMask        : 0x110 [Type: unsigned long]
    [+0x2d4] KdDebuggerEnabled : 0x3 [Type: unsigned char]              ; <-- 哈?
    [+0x2d5] MitigationPolicies : 0xa [Type: unsigned char]
    [+0x2d5 ( 1: 0)] NXSupportPolicy  : 0x2 [Type: unsigned char]
    [+0x2d5 ( 3: 2)] SEHValidationPolicy : 0x2 [Type: unsigned char]
    [+0x2d5 ( 5: 4)] CurDirDevicesSkippedForDlls : 0x0 [Type: unsigned char]
    [+0x2d5 ( 7: 6)] Reserved         : 0x0 [Type: unsigned char]
    [+0x2d6] Reserved6        [Type: unsigned char [2]]
    [+0x2d8] ActiveConsoleId  : 0x1 [Type: unsigned long]
    [+0x2dc] DismountCount    : 0x0 [Type: unsigned long]
    [+0x2e0] ComPlusPackage   : 0xffffffff [Type: unsigned long]
    [+0x2e4] LastSystemRITEventTickCount : 0x106ce6 [Type: unsigned long]
    [+0x2e8] NumberOfPhysicalPages : 0x7fef1 [Type: unsigned long]
    [+0x2ec] SafeBootMode     : 0x0 [Type: unsigned char]
    [+0x2ed] Reserved12       [Type: unsigned char [3]]
    [+0x2f0] SharedDataFlags  : 0xf [Type: unsigned long]
    [+0x2f0 ( 0: 0)] DbgErrorPortPresent : 0x1 [Type: unsigned long]
    [+0x2f0 ( 1: 1)] DbgElevationEnabled : 0x1 [Type: unsigned long]
    [+0x2f0 ( 2: 2)] DbgVirtEnabled   : 0x1 [Type: unsigned long]
    [+0x2f0 ( 3: 3)] DbgInstallerDetectEnabled : 0x1 [Type: unsigned long]
    [+0x2f0 ( 4: 4)] DbgLkgEnabled    : 0x0 [Type: unsigned long]
    [+0x2f0 ( 5: 5)] DbgDynProcessorEnabled : 0x0 [Type: unsigned long]
    [+0x2f0 ( 6: 6)] DbgConsoleBrokerEnabled : 0x0 [Type: unsigned long]
    [+0x2f0 ( 7: 7)] DbgSecureBootEnabled : 0x0 [Type: unsigned long]
    [+0x2f0 (31: 8)] SpareBits        : 0x0 [Type: unsigned long]
    [+0x2f4] DataFlagsPad     [Type: unsigned long [1]]
    [+0x2f8] TestRetInstruction : 0xc3 [Type: unsigned __int64]
    [+0x300] QpcFrequency     : 2540039 [Type: __int64]
    [+0x308] SystemCallPad    [Type: unsigned __int64 [3]]
    [+0x320] TickCount        [Type: _KSYSTEM_TIME]
    [+0x320] TickCountQuad    : 0x10635c7d [Type: unsigned __int64]
    [+0x320] ReservedTickCountOverlay [Type: unsigned long [3]]
    [+0x32c] TickCountPad     [Type: unsigned long [1]]
    [+0x330] Cookie           : 0xc8d16c81 [Type: unsigned long]
    [+0x334] CookiePad        [Type: unsigned long [1]]
    [+0x338] ConsoleSessionForegroundProcessId : 1444 [Type: __int64]
    [+0x340] TimeUpdateLock   : 0x49ae902 [Type: unsigned __int64]
    [+0x348] BaselineSystemTimeQpc : 0x9f94d50871a [Type: unsigned __int64]
    [+0x350] BaselineInterruptTimeQpc : 0x9f94d50871a [Type: unsigned __int64]
    [+0x358] QpcSystemTimeIncrement : 0xd35cfe4f14ff7eb6 [Type: unsigned __int64]
    [+0x360] QpcInterruptTimeIncrement : 0xd35cfe4f14ff7eb6 [Type: unsigned __int64]
    [+0x368] QpcSystemTimeIncrement32 : 0xd35cfe4f [Type: unsigned long]
    [+0x36c] QpcInterruptTimeIncrement32 : 0xd35cfe4f [Type: unsigned long]
    [+0x370] QpcSystemTimeIncrementShift : 0x15 [Type: unsigned char]
    [+0x371] QpcInterruptTimeIncrementShift : 0x15 [Type: unsigned char]
    [+0x372] Reserved8        [Type: unsigned char [14]]
    [+0x380] UserModeGlobalLogger [Type: unsigned short [16]]
    [+0x3a0] ImageFileExecutionOptions : 0x0 [Type: unsigned long]
    [+0x3a4] LangGenerationCount : 0x1 [Type: unsigned long]
    [+0x3a8] Reserved4        : 0x0 [Type: unsigned __int64]
    [+0x3b0] InterruptTimeBias : 0x27079e3b9800 [Type: unsigned __int64]
    [+0x3b8] QpcBias          : 0x27a78d780ed587 [Type: unsigned __int64]
    [+0x3c0] ActiveProcessorCount : 0x2 [Type: unsigned long]
    [+0x3c4] ActiveGroupCount : 0x1 [Type: unsigned char]
    [+0x3c5] Reserved9        : 0x0 [Type: unsigned char]
    [+0x3c6] QpcData          : 0xa01 [Type: unsigned short]
    [+0x3c6] QpcBypassEnabled : 0x1 [Type: unsigned char]
    [+0x3c7] QpcShift         : 0xa [Type: unsigned char]
    [+0x3c8] TimeZoneBiasEffectiveStart : {131406845483478733} [Type: _LARGE_INTEGER]
    [+0x3d0] TimeZoneBiasEffectiveEnd : {131592096000000000} [Type: _LARGE_INTEGER]
    [+0x3d8] XState           [Type: _XSTATE_CONFIGURATION]
```

我们来看一个用到这个结构的栗子

```C
DWORD WINAPI GetTickCount(VOID)
{
    ULARGE_INTEGER TickCount;

#ifdef _WIN64
    TickCount.QuadPart = *((volatile ULONG64*)&SharedUserData->TickCount);
#else
    while (TRUE)
    {
        TickCount.HighPart = (ULONG)SharedUserData->TickCount.High1Time;
        TickCount.LowPart = SharedUserData->TickCount.LowPart;

        if (TickCount.HighPart == (ULONG)SharedUserData->TickCount.High2Time)
            break;

        YieldProcessor();
    }
#endif

    return (ULONG)((UInt32x32To64(TickCount.LowPart,
                                  SharedUserData->TickCountMultiplier) >> 24) +
                    UInt32x32To64((TickCount.HighPart << 8) & 0xFFFFFFFF,
                                  SharedUserData->TickCountMultiplier));
}
```

用户空间的这个函数需要通过 SharedUserData 的 Tick 时钟计数.

## CreateProcess

接下来我们就要看一些 CreateProcess 具体是怎么做的, 对比 Win8.1 发现变化还是蛮大的.. 所以只能祭出逆向大法了..

```C
```

## CreateThread

占坑

## APC, Asynchronous Procedure Calls

内容比较多, 所以另开一篇文章单独介绍 

{% post_link Windows-kernel-learning/6-APC-Asynchronous-Procedure-Calls %}


---

## 未完待续...
