---
title: 'Windows kernel learning: 2. System Call'
comments: true
date: 2017-05-12 13:41:43
tags: ['Windows', 'Kernel']
categories: ['Windows kernel learning']
---

CPU 既可以运行于非特权的"用户空间", 也可以运行于特权的"系统空间".

CPU 要从系统空间转入用户空间是容易的, 因为运行于系统空间的CPU可以通过一些特权指令改变其运行状态. 但是从用户空间转入系统空间就不容易了, 因为运行于用户空间的 CPU 是不能执行特权指令的.

一般而言,只有下面几种手段可以从用户空间转入系统空间:

* 中断 (Interrupt): 在开启了中断的情况下,只要有中断请求到来, CPU 就会自动转入系统空间, 并指定对应的中断例程, 从而为中断请求提供服务. 中断发生在两条指令之间, 所以不会使正在执行的指令半途而废, 中断是无法预知且异步的.

* 异常 (Exception): 异常和中断极其相似, 只是一行发生在执行一条指令的过程中, 而不是两条指令之间. 实践中, 可以通过故意引起异常而进入内核.

* 自陷 (Trap): 为了让 CPU 能主动地进入系统空间, 绝大多数 CPU 都设有专门的 "自陷" 指令, 系统调用通常都是通过自陷指令实现的. 自陷指令在形式上也与中断相似,就像是 CPU 主动发出的中断请求.

* 快速系统调用 (fast call): 可以说是对自陷机制的改进.

这篇笔记主要就是写自陷和快速系统调用机制.

<!--more-->


## 系统调用机制概述

### 自陷指令 int 2eh

在 Pentium Ⅱ 之前的 x86 处理器上, Windows 使用 `int 2eh` 自陷指令进入内核实现系统调用. Windows 填充 IDT 的46 号表项, 使其指向系统服务分发器. 使用 EAX 寄存器传递系统服务号, EDX 寄存器指向调用者传递给系统服务的参数列表.

### sysenter/sysexit

在之后, Windows 使用专门的 `sysenter` 指令, 这是 Intel 特别为快速系统分发而定义的指令, 与此配套, CPU 中增加了三个 MSR (Machine Specific Register) 寄存器: `SYSENTER_CS_MSR`, `SYSENTER_EIP_MSR`, `SYSENTER_ESP_MSR`. 

这些寄存器可以通过 `wrmsr` 指令来设置, 执行 `wrmsr` 指令时, 通过寄存器 edx, eax 指定设置的值, edx 指定值的高 32 位, eax 指定值的低 32 位, 在设置上述寄存器时, edx 都是 0, 通过寄存器 ecx 指定填充的 MSR 寄存器, `SYSENTER_CS_MSR`, `SYSENTER_ESP_MSR`, `SYSENTER_EIP_MSR` 寄存器分别对应 `0x174, 0x175, 0x176`, 需要注意的是, `wrmsr/rdmsr` 指令只能在 Ring 0 执行.

与自陷指令一样, sysenter 使用 EAX 寄存器传递系统服务号, EDX 寄存器指向调用者传递给系统服务的参数列表.

### syscall/sysret

在之后的 x64 体系架构上, Windows 使用 `syscall` 指令进行系统调用,将系统调用号通过EAX寄存器来传递, 前四个参数放在寄存器中传递, 剩下的参数都被放入栈中.

为了支持 `syscall/sysret`, AMD 新增了4个 MSR 寄存器:
* STAR
* LSTAR
* CSTAR
* SFMASK

![syscall msr](syscall_msr.jpg)
![syscall star](syscall_star.jpg)

通过上图我们已经明白了 STAR 寄存器的用途：

在 `legacy x86` 下提供 `eip` 值（仅在 `egacy x86` 模式下）  
为 `syscall` 指令提供目标代码的 `CS` 和 `SS` selector  
为 `sysret` 指令提供返回代码的 `CS` 和 `SS` selector

因此, STAR 寄存器分为三部分：

1. [31:00] - SYSCALL_EIP - legacy 模式的 EIP
2. [47:32] - SYSCALL_CS
3. [63:48] - SYSRET_CS

* SYSRET_CS：32-bit code segment descriptor selector (包括 legacy x86 的 16-bit 代码)
* SYSRET_CS+8：stack segment descriptor selector
* SYSRET_CS+16：64-bit code segment descriptor selector

SFMASK 寄存器中的值为1的位,就会在 EFLAGS 寄存器中置零.

在 Intel 下 STSR 被称作 `IA32_STAR`, LSTAR 被称作 `IA32_LSTAR`,  SFMASK 被称作 `IA32_SFMASK`,  虽然是冠以 IA32 体系, 但是请相信它们是 64 位的. 除前面所说的只能在 64 位环境执行, 其它方面完全是兼容 AMD 的. 

在 Windows 中, LSTAR 实际指向 KiSystemCall64, CSTAR 指向 KiSystemCall32.

### 对于 `sysenter` 和 `syscall` 的关系:
> 在 AMD 与 Intel 的 processor 上还是有区别的: 
>
> 在 AMD 的 processor 上: syscall/sysret 指令在 long mode 和 protected mode ( 指的是 Legacy x86 和 compatibility mode ) 上都是有效的 ( valid ).
>
> 在 Intel processor 上: syscall/sysret 指令只能在 64-bit 模式上使用, compatibility 模式和 Legacy x86 模式上都是无效的. 可是 sysret 指令虽然不能在 compatibility 模式下执行, 但 sysret 却可以返回到 compaitibility 模式. 这一点只能是认为了兼容 AMD 的 sysret 指令. 
> 
>怎么办, 这会不会出现兼容上的问题? 这里有一个折衷的处理办法: 
> 
> 在 64 位环境里统一使用 syscall/sysret 指令, 在 32 位环境里统一使用 sysenter/sysexit 指令
> 
> 然而依旧会产生一些令人不愉快的顾虑, 没错, 在 compatibility 模式下谁都不兼容谁:
> 
> Intel 的 syscall/sysret 指令不能在 compatibility 模式下执行; AMD 的 sysenter/sysexit 指令也不能在 compatibility 模式下执行.
> 
> 因此: 在 compatibility 模式下必须切换到 64 位模式, 然后使用 syscall/sysret 指令
> 
> 详见: [mik-使用 syscall/sysret 指令](http://www.mouseos.com/arch/syscall_sysret.html)

## 系统调用机制的切换过程

### `int 2eh` 指令

CPU 的运行状态从用户态切换成内核态. 从任务状态段 TSS 中装入本线程的内核栈寄存器 SS 和 ESP, 再保存现场, 依次 PUSH SS, ESP, EFLAGS, CS, EIP, 然后执行 IDT[0x2e] 中的系统服务分发器开始执行内核中的程序. 最后系统调用返回则通过中断返回指令 `iret` 实现上述的逆过程.

### `sysenter/sysexit` 指令

在 Ring3 的代码调用了 `sysenter` 指令之后, CPU 会做出如下的操作：

1. 将 `SYSENTER_CS_MSR` 的值装载到 cs 寄存器
2. 将 `SYSENTER_EIP_MSR` 的值装载到 eip 寄存器
3. 将 `SYSENTER_CS_MSR` 的值加 8（Ring0 的堆栈段描述符）装载到 ss 寄存器. 
4. 将 `SYSENTER_ESP_MSR` 的值装载到 esp 寄存器
5. 将特权级切换到 Ring0
6. 如果 EFLAGS 寄存器的 VM 标志被置位, 则清除该标志
7. 开始执行指定的 Ring0 代码

在 Ring0 代码执行完毕, 调用 `SYSEXIT` 指令退回 Ring3 时, CPU 会做出如下操作：

1. 将 `SYSENTER_CS_MSR` 的值加 16（Ring3 的代码段描述符）装载到 cs 寄存器
2. 将寄存器 edx 的值装载到 eip 寄存器
3. 将 `SYSENTER_CS_MSR` 的值加 24（Ring3 的堆栈段描述符）装载到 ss 寄存器
4. 将寄存器 ecx 的值装载到 esp 寄存器
5. 将特权级切换到 Ring3
6. 继续执行 Ring3 的代码

### `syscall/sysret` 指令

用伪代码来表示

```cpp
MSR_EFER EFER;
MSR_STAR STAR;
MSR_LSTAR LSTAR;
MSR_CSTAR CSTAR;
MSR_SFMASK SFMASK;

void syscall()
{
    if (EFER.SCE == 0)        /* system call extensions is disable */
        do_exception_UD();    /* #UD exception */
       

    if (EFER.LMA == 1) {      /* long mode is active */
        rcx = rip;            /* save rip for syscall return */
        r11 = rflags;         /* save rflags to r11 */

        /*
         * CS.L == 1 for 64-bit mode, rip from MSR_LSTAR
         * CS.L == 0 for compatibility, rip from MSR_CSTAR
         */
        rip = CS.attribute.L ? LSTAR : CSTAR;

        /*
         * processor set CS register 
         */       
        CS.selector = STAR.SYSCALL_CS;       /* load selector from MSR_STAR.SYSCALL_CS */
        CS.selector.RPL = 0;                 /* RPL = 0 */
        CS.attribute.S = 1;                  /* user segment descriptor */
        CS.attribute.C_D = 1;                /* code segment */
        CS.attribute.L = 1;                  /* 64-bit */
        CS.attribute.D = 0;                  /* 64-bit */
        CS.attribute.DPL = 0;                /* CPL = 0 */                   
        CS.attribute.P = 1;                  /* present = 1 */
        CS.base = 0;
        CS.limit = 0xFFFFFFFF;

        /*
         * processor set SS register
         */
         SS.selector = STAR.SYSCALL_CS + 8;
         SS.attribute.S = 1;
         SS.attribute.C_D = 0;
         SS.attribute.P = 1;
         SS.attribute.DPL = 0;
         SS.base = 0;
         SS.limit = 0xFFFFFFFF;

         /* set rflags */
         rflags = rflags & ~ SFMASK;
         rflags.RF = 0;

         /* goto rip ... */


    } else {
        /* legacy mode */

        rcx = (unsigned long long)eip;            /* eip extend to 64 load into rcx */
        rip = (unsigned long long)STAR.EIP;       /* get eip from MSR_STAR.EIP */
       
        CS.selector = STAR.SYSCALL_CS;
        CS.selector.RPL = 0;
        CS.attribute.S = 1;                  /* user descriptor */
        CS.attribute.C_D = 1;                /* code segment */
        CS.attribute.D = 1;                  /* 32-bit */
        CS.attribute.C = 0;                  /* non-conforming */
        CS.attribute.R = 1;                  /* read/execute */
        CS.attribute.DPL = 0;                /* CPL = 0 */                   
        CS.attribute.P = 1;                  /* present = 1 */
        CS.attribute.G = 1;                  /* G = 1 */
        CS.base = 0;
        CS.limit = 0xFFFFFFFF;                     

        SS.selector = STAR.SYSCALL_CS + 8;
        SS.attribute.S = 1;                 /* user descriptor */
        SS.attribute.C_D = 0;               /* data segment */
        SS.attribute.D = 1;                 /* 32-bit esp */
        SS.attribute.E = 0;                 /* expand-up */
        SS.attribute.W = 1;                 /* read/write */
        SS.attribute.P = 1;                 /* present */
        SS.attribute.DPL = 0;               /* DPL = 0 */
        SS.attribute.G = 1;                 /* G = 1 */
        SS.base = 0;
        SS.limit = 0xFFFFFFFF;

        rflags.VM = 0;
        rflags.IF = 0;
        rflags.RF = 0;

        /* goto rip */
    }

}
```

```cpp
void sysret()
{
    if (EFER.SCE == 0)                          /* System Call Extension is disable */
        do_exception_UD();

    if (CR0.PE == 0 || CS.attribute.DPL != 0)   /* protected mode is disable or CPL != 0 */
        do_exception_GP();    

    if (CS.attribute.L == 1)                    /* 64-bit mode */
    {   
        if (REX.W == 1)                         /* 64-bit operand size */
        {
             /* 
              * return to 64-bit code !
              */
             CS.selector = STAR.SYSRET_CS + 16; /* 64-bit code segment selector */
             CS.selector.RPL = 3;               /* CPL = 3 */
             CS.attribute.L = 1;
             CS.attribute.D = 0;
             CS.attribute.P = 1;
             CS.attribute.DPL = 3;
             CS.base = 0;
             CS.limit = 0xFFFFFFFF;
           
             rip = rcx;                     /* restore rip for return */

        } else {
             /*
              * return to compatibility !
              */
             CS.selector = STAR.SYSRET_CS;  /* 32-bit code segment selector */
             CS.selector.RPL = 3;
             CS.attribute.L = 0;            /* compatibility mode */
             CS.attribute.D = 1;            /* 32-bit code */
             CS.attribute.P = 1;
             CS.attribute.C = 0;
             CS.attribute.R = 1;
             CS.attribute.DPL = 3;
             CS.base = 0;
             CS.limit = 0xFFFFFFFF; 

             rip = (unsigned long long)ecx;              
        }
        
        SS.selector = START.SYSRET_CS + 8;  /* SS selector for return */
        rflags = r11;                       /* restore rflags */

        /* goto rip */

    } else {                                /* compatibility or legacy mode */

         CS.selector = STAR.SYSRET_CS;      /* 32-bit code segment selector */
         CS.selector.RPL = 3
         CS.attribute.L = 0;                /* compatibility mode */
         CS.attribute.D = 1;                /* 32-bit code */
         CS.attribute.P = 1;
         CS.attribute.C = 0;
         CS.attribute.R = 1;
         CS.attribute.DPL = 3;
         CS.base = 0;
         CS.limit = 0xFFFFFFFF; 

         SS.selector = STAR.SYSRET_CS + 8;

         rflags.IF = 1;

         rip = (unsigned long long)ecx;
    }

}
```

## System Service Descriptor Table (SSDT) & Shadow SSDT

现在我们知道, Ring3 通过 `syscall` 进行系统调用到 Ring0, 那么 `syscall` 是怎么找到对应的内核服务函数的呢? 就是通过 SSDT 和 Shadow SSDT 这两张表来找到的.

SSDT 的全称是 System Services Descriptor Table, 系统服务描述符表. 这个表就是一个把 Ring3 的 Win32 API 和 Ring0 的内核 API 联系起来. SSDT 并不仅仅只包含一个庞大的地址索引表, 它还包含着一些其它有用的信息, 诸如地址索引的基地址, 服务函数个数等. 通过修改此表的函数地址可以对常用Windows 函数及 API 进行 Hook, 从而实现对一些关心的系统动作进行过滤, 监控的目的. 一些 HIPS, 防毒软件, 系统监控, 注册表监控软件往往会采用此接口来实现自己的监控模块. 

例如, Windows API OpenProcess是从Kernel32导出的, 所以调用首先转到了Kernel32的OpenProcess函数. 在OpenProcess中又调用了ntdll!NtOpenProcess函数. 然后通过快速系统调用进入内核, 根据传进来的索引在SSDT中得到函数的地址, 然后调用函数. 

在 NT 4.0 以上的 Windows 操作系统中, 默认就存在两个系统服务描述表, 这两个调度表对应了两类不同的系统服务, 这两个调度表为：KeServiceDescriptorTable 和 KeServiceDescriptorTableShadow, 其中 KeServiceDescriptorTable 主要是处理来自 Ring3 层得 Kernel32.dll中的系统调用, 而 KeServiceDescriptorTableShadow 则主要处理来自 User32.dll 和 GDI32.dll 以及 Win32u.dll 中的系统调用, 并且 KeServiceDescriptorTable 在ntoskrnl.exe(Windows 操作系统内核文件, 包括内核和执行体层)是导出的, 而 KeServiceDescriptorTableShadow 则是没有被 Windows 操作系统所导出, 而关于 SSDT 的全部内容则都是通过 KeServiceDescriptorTable 来完成的 ~

ntoskrnl.exe中的一个导出项 KeServiceDescriptorTable 即是SSDT的真身, 亦即它在内核中的数据实体. SSDT的数 据结构定义如下: 

```c
typedef struct _KSYSTEM_SERVICE_TABLE
{
    PULONG  ServiceTableBase;                               // SSDT (System Service Dispatch Table)的基地址
    PULONG  ServiceCounterTableBase;                        // 用于 checked builds, 包含 SSDT 中每个服务被调用的次数
    ULONG   NumberOfService;                                // 服务函数的个数, NumberOfService * 4 就是整个地址表的大小
    ULONG   ParamTableBase;                                 // SSPT(System Service Parameter Table)的基地址, 该表格包含了每个服务所需的参数字节数
} KSYSTEM_SERVICE_TABLE, *PKSYSTEM_SERVICE_TABLE;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
    KSYSTEM_SERVICE_TABLE   ntoskrnl;                       // ntoskrnl.exe 的服务函数
    KSYSTEM_SERVICE_TABLE   win32k;                         // win32k.sys 的服务函数(GDI32.dll/User32.dll 的内核支持)
    KSYSTEM_SERVICE_TABLE   notUsed1;
    KSYSTEM_SERVICE_TABLE   notUsed2;
}KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;
```

然而, x86 与 x64 有些许差别, x86 中 ServiceTableBase 存储的就是系统服务函数地址.  
而 x64 中 ServiceTableBase 存储的是相对于ServiceTableBase的系统服务函数的偏移, 同样使用4字节表示一项.  
由于函数的起始地址最低四位都是0, 所以微软将 SSDT 中的低四位用来记录这个函数有多少个参数.

那么根据 KiSystemServiceStart 函数可得到算法:

```c
x86: Address = ServiceTableBase + (SystemCallNumber * 4)

x64: Address = ServiceTableBase + (((int*)(ServiceTableBase + (SystemCallNumber & 0x0FFF) * 4))[0] >> 4)
```

## 拿个栗子说事儿

### Windows 8.1 x64, syscall/sysret

首先我们来看下 `syscall/sysret` 相关的几个寄存器

```
1: kd> rdmsr c0000081
msr[c0000081] = 00230010`00000000
1: kd> rdmsr c0000082
msr[c0000082] = fffff800`2a492200
1: kd> ln fffff800`2a492200
Browse module
Set bu breakpoint

(fffff800`2a492200)   nt!KiSystemCall64   |  (fffff800`2a492348)   nt!KiSystemServiceStart
Exact matches:
    nt!KiSystemCall64 (<no parameter info>)
1: kd> rdmsr c0000083
msr[c0000083] = fffff800`2a491f40
1: kd> ln fffff800`2a491f40
Browse module
Set bu breakpoint

(fffff800`2a491f40)   nt!KiSystemCall32   |  (fffff800`2a492080)   nt!KiSystemServiceHandler
Exact matches:
    nt!KiSystemCall32 (<no parameter info>)
1: kd> rdmsr c0000084
msr[c0000084] = 00000000`00004700
```

首先看一下 STAR 寄存器. 通过 `rdmsr c0000081` 得到 `0023001000000000`, 根据 STAR 的结构得知:

* sysret  CS        : 0023
* sysret  SS        : 002B ; CS + 8
* sysret  CS 64bit  : 0033 ; CS + 16
* syscall CS        : 0010 
* syscall SS        : 0018 ; CS + 8
* syscall 32bit EIP : 00000000

我们通过分别对 ntdll!NtCreateFile 和 nt!NtCreateFile 下断点来验证一下:

```x86asm
Breakpoint 1 hit
ntdll!NtCreateFile:
0033:00007ff8`8d282670 48894c2408      mov     qword ptr [rsp+8],rcx
1: kd> r
rax=0000000000000000 rbx=000000000eb94a60 rcx=000000001128eb80
rdx=0000000000100001 rsi=00000000133ea660 rdi=00007ff875680088
rip=00007ff88d282670 rsp=000000001128eb08 rbp=000000001128eba9
 r8=000000001128eba8  r9=000000001128ebe0 r10=00000000133e6050
r11=00000000133ea668 r12=0000000000000001 r13=00007ff875680088
r14=0000000000000000 r15=000000000eb58e90
iopl=0         nv up ei pl zr na po nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00000246
ntdll!NtCreateFile:
0033:00007ff8`8d282670 48894c2408      mov     qword ptr [rsp+8],rcx ss:002b:00000000`1128eb10=000000000eb94a60
1: kd> g
Breakpoint 2 hit
nt!NtCreateFile:
fffff802`2b5f8784 488bc4          mov     rax,rsp
1: kd> r
rax=0000000000000000 rbx=ffffe0000883e080 rcx=000000001128eb80
rdx=0000000000100001 rsi=000000001128eb28 rdi=ffffd00026fb2aa8
rip=fffff8022b5f8784 rsp=ffffd00026fb2a88 rbp=ffffd00026fb2b80
 r8=000000001128eba8  r9=000000001128ebe0 r10=fffff8022b5f8784
r11=fffff8022af1a478 r12=0000000000000001 r13=00007ff875680088
r14=0000000000000000 r15=000000000eb58e90
iopl=0         nv up ei pl zr na po nc
cs=0010  ss=0018  ds=002b  es=002b  fs=0053  gs=002b             efl=00000246
nt!NtCreateFile:
fffff802`2b5f8784 488bc4          mov     rax,rsp
```

可以看到  
ntdll!NtCreateFile 的 CS, SS 寄存器的值分别对应 sysret 的 CS 64it, SS.  
nt!NtCreateFile 的 CS, SS 寄存器的值分别对应 syscall 的 CS, SS.

接下来, 我们来看看 syscall 是如何从 Ring3 切换到 Ring0 的.
我们来看下 LSTAR 指向的函数, 即 nt!KiSystemCall64 :

不知道什么原因, 我只要对这个函数下断点就会导致 VMWare 虚拟机显示CPU异常而退出...  
所以我们直接用 IDA 看代码

#### KiSystemCall64

```x86asm
.text:000000000028F200     KiSystemCall64  proc near               ; DATA XREF: .pdata:000000000088430C
.text:000000000028F200                                             ; KiInitializeBootStructures+348
.text:000000000028F200
.text:000000000028F200     var_1C0         = qword ptr -1C0h
.text:000000000028F200     var_1B8         = qword ptr -1B8h
.text:000000000028F200     var_1B0         = qword ptr -1B0h
.text:000000000028F200     var_1A8         = qword ptr -1A8h
.text:000000000028F200     var_1A0         = qword ptr -1A0h
.text:000000000028F200     var_178         = byte ptr -178h
.text:000000000028F200     TF_Xmm1Offset   = byte ptr -110h
.text:000000000028F200     arg_F8          = qword ptr  100h
.text:000000000028F200
.text:000000000028F200 000                 swapgs                           ; GS.Base 与 MSR[C0000102] (KernelGSBase) 交换, 此时指向内核 GS
.text:000000000028F203 000                 mov     gs:10h, rsp              ; 保存用户态栈到 _KPCR.UserRsp
.text:000000000028F20C 000                 mov     rsp, gs:1A8h             ; 从 _KPCR.Prcb.RspBase 加载内核态栈
.text:000000000028F215 000                 push    2Bh                      ; 开始构建 TrapFrame, TrapFrame.SegSs = 0x2B
.text:000000000028F217 008                 push    qword ptr gs:10h         ; TrapFrame.Rsp = _KPCR.UserRsp (用户态栈)
.text:000000000028F21F 010                 push    r11                      ; TrapFrame.EFlags = r11 (用户态 rflags)
.text:000000000028F221 018                 push    33h                      ; TrapFrame.SegCs = 0x33
.text:000000000028F223 020                 push    rcx                      ; TrapFrame.Rip = rcx (这个是用户态 syscall 下一条指令的地址)
.text:000000000028F224 028                 mov     rcx, r10                 ; 把 FirstArgument 赋值给 rcx
.text:000000000028F227 028                 sub     rsp, 8                   ; 调整栈, 跳过 TrapFrame.ErrorCode
.text:000000000028F22B 030                 push    rbp                      ; TrapFrame.Rbp = rbp
.text:000000000028F22C 038                 sub     rsp, 158h                ; 调整 TrapFrame 起始地址, 0x158 + 0x38 = 0x190 即这个结构体从尾部开始填充数据, 然后其他未处理的部分直接调整栈来分配足够的空间.
.text:000000000028F233 190                 lea     rbp, [rsp+190h+TF_Xmm1Offset] ; 不理解为啥要从这个 TrapFrame.Xmm1 字段开始
.text:000000000028F23B 190                 mov     [rbp+0C0h], rbx          ; TrapFrame.Rbx = rbx
.text:000000000028F242 190                 mov     [rbp+0C8h], rdi          ; TrapFrame.Rdi = rdi
.text:000000000028F249 190                 mov     [rbp+0D0h], rsi          ; TrapFrame.Rsi = rsi
.text:000000000028F250 190                 mov     byte ptr [rbp-55h], 2    ; TrapFrame.ExceptionActive = 2
.text:000000000028F254 190                 mov     rbx, gs:188h             ; rbx = _KPCR.Prcb.CurrentThread (_KTHREAD)
.text:000000000028F25D 190                 prefetchw byte ptr [rbx+90h]     ; 提示 CPU 预加载 _KPCR.Prcb.CurrentThread.TrapFrame
.text:000000000028F264 190                 stmxcsr dword ptr [rbp-54h]      ; TrapFrame.MxCsr = mxcsr
.text:000000000028F268 190                 ldmxcsr dword ptr gs:180h        ; mxcsr = _KPCR.Prcb.MxCsr
.text:000000000028F271 190                 cmp     byte ptr [rbx+3], 0      ; _KPCR.Prcb.CurrentThread.DispatchHeader.DebugActive
.text:000000000028F275 190                 mov     word ptr [rbp+80h], 0    ; TrapFrame.ErrorCode = 0
.text:000000000028F27E 190                 jz      NoDebugActive            ; 一般从这里跳, 未调试
.text:000000000028F284 190                 mov     [rbp-50h], rax           ; TrapFrame.Rax = rax
.text:000000000028F288 190                 mov     [rbp-48h], rcx           ; TrapFrame.Rcx = rcx
.text:000000000028F28C 190                 mov     [rbp-40h], rdx           ; TrapFrame.Rdx = rdx
.text:000000000028F290 190                 test    byte ptr [rbx+3], 3      ; _KPCR.Prcb.CurrentThread.DispatchHeader.DebugActive(0x3).(ActiveDR7 & Instrumented)
.text:000000000028F294 190                 mov     [rbp-38h], r8            ; TrapFrame.R8 = r8
.text:000000000028F298 190                 mov     [rbp-30h], r9            ; TrapFrame.R9 = r9
.text:000000000028F29C 190                 jz      short NoSaveDebugRegisterState
.text:000000000028F29E 190                 call    KiSaveDebugRegisterState
.text:000000000028F2A3
.text:000000000028F2A3     NoSaveDebugRegisterState:               ; CODE XREF: KiSystemCall64+9C
.text:000000000028F2A3 190                 test    byte ptr [rbx+3], 4      ; _KPCR.Prcb.CurrentThread.DispatchHeader.DebugActive(0x4).Minimal
.text:000000000028F2A7 190                 jz      short NoDebugActiveMinimal
.text:000000000028F2A9 190                 sti
.text:000000000028F2AA 190                 mov     ecx, [rbp-50h]  ; _QWORD
.text:000000000028F2AD 190                 mov     rdx, rsp        ; _QWORD
.text:000000000028F2B0 190                 call    cs:__imp_PicoSystemCallDispatch
.text:000000000028F2B6 190                 jmp     KiSystemServiceExit
.text:000000000028F2BB     ; ---------------------------------------------------------------------------
.text:000000000028F2BB
.text:000000000028F2BB     NoDebugActiveMinimal:                   ; CODE XREF: KiSystemCall64+A7
.text:000000000028F2BB 190                 test    byte ptr [rbx+3], 80h    ; _KPCR.Prcb.CurrentThread.DispatchHeader.DebugActive(0x80).UmsPrimary
.text:000000000028F2BF 190                 jz      short NoDebugActiveUmsPrimary
.text:000000000028F2C1 190                 mov     ecx, 0C0000102h          ; KernelGSBase
.text:000000000028F2C6 190                 rdmsr                            ; 由于开头调用了 swapgs, 所以返回的是 用户态 GS
.text:000000000028F2C8 190                 shl     rdx, 20h
.text:000000000028F2CC 190                 or      rax, rdx
.text:000000000028F2CF 190                 cmp     [rbx+0F0h], rax
.text:000000000028F2D6 190                 jz      short NoDebugActiveUmsPrimary
.text:000000000028F2D8 190                 cmp     [rbx+200h], rax
.text:000000000028F2DF 190                 jz      short NoDebugActiveUmsPrimary
.text:000000000028F2E1 190                 mov     rdx, [rbx+1F0h]
.text:000000000028F2E8 190                 bts     dword ptr [rbx+74h], 9
.text:000000000028F2ED 190                 dec     word ptr [rbx+1E6h]
.text:000000000028F2F4 190                 mov     [rdx+80h], rax
.text:000000000028F2FB 190                 sti
.text:000000000028F2FC 190                 call    KiUmsCallEntry
.text:000000000028F301 190                 jmp     short loc_28F30E
.text:000000000028F303     ; ---------------------------------------------------------------------------
.text:000000000028F303
.text:000000000028F303     NoDebugActiveUmsPrimary:                ; CODE XREF: KiSystemCall64+BF
.text:000000000028F303                                             ; KiSystemCall64+D6 ...
.text:000000000028F303 190                 test    byte ptr [rbx+3], 40h
.text:000000000028F307 190                 jz      short loc_28F30E
.text:000000000028F309 190                 bts     dword ptr [rbx+74h], 11h
.text:000000000028F30E
.text:000000000028F30E     loc_28F30E:                             ; CODE XREF: KiSystemCall64+101
.text:000000000028F30E                                             ; KiSystemCall64+107
.text:000000000028F30E 190                 mov     rax, [rbp-50h]
.text:000000000028F312 190                 mov     rcx, [rbp-48h]
.text:000000000028F316 190                 mov     rdx, [rbp-40h]
.text:000000000028F31A 190                 mov     r8, [rbp-38h]
.text:000000000028F31E 190                 mov     r9, [rbp-30h]
.text:000000000028F322                     db      66h, 66h, 66h, 66h, 66h, 66h
.text:000000000028F322 190                 nop     word ptr [rax+rax+00000000h]
.text:000000000028F330
.text:000000000028F330     NoDebugActive:                          ; CODE XREF: KiSystemCall64+7E
.text:000000000028F330 190                 sti
.text:000000000028F331 190                 mov     [rbx+88h], rcx           ; _KPCR.Prcb.CurrentThread.FirstArgument
.text:000000000028F338 190                 mov     [rbx+80h], eax           ; _KPCR.Prcb.CurrentThread.SystemCallNumber
.text:000000000028F33E 190                 cmp     byte ptr [rbx+232h], 1   ; 检查 PreviousMode 应该为 UserMode
.text:000000000028F345 190                 jz      short KiSystemServiceStart
.text:000000000028F347 190                 int     3               ; Trap to Debugger
.text:000000000028F348
.text:000000000028F348     KiSystemServiceStart:                   ; CODE XREF: KiSystemCall64+145
.text:000000000028F348                                             ; DATA XREF: KiServiceInternal+5A ...
.text:000000000028F348 190                 mov     [rbx+90h], rsp           ; _KPCR.Prcb.CurrentThread.TrapFrame = TrapFrame
.text:000000000028F34F 190                 mov     edi, eax                 ; _KPCR.Prcb.CurrentThread.SystemCallNumber
.text:000000000028F351 190                 shr     edi, 7                   ; 这三行是用来在后面检查是否为 GUI API 调用的, 如果是 GUI API, 则计算出 Shadow SSDT 的偏移号.
.text:000000000028F354 190                 and     edi, 20h                 ; GUI API 调用号都是从 0x1000 开始的. 0x1000 >> 7 刚好是 0x20
.text:000000000028F357 190                 and     eax, 0FFFh               ; 修正调用号 (消除 GUI 调用号的 0x1000 基本号)
.text:000000000028F35C
.text:000000000028F35C     KiSystemServiceRepeat:                  ; CODE XREF: KiSystemCall64+4B1
.text:000000000028F35C                                                      ; 这一段就是根据调用号计算出系统服务例程地址的算法部分了
.text:000000000028F35C 190                 lea     r10, KeServiceDescriptorTable
.text:000000000028F363 190                 lea     r11, KeServiceDescriptorTableShadow
.text:000000000028F36A 190                 test    dword ptr [rbx+78h], 40h ; test _KPCR.Prcb.CurrentThread.ThreadFlags.GuiThread, 1
.text:000000000028F371 190                 cmovnz  r10, r11                 ; if GuiThread Then r10 = KeServiceDescriptorTableShadow;
.text:000000000028F375 190                 cmp     eax, [rdi+r10+10h]       ; SystemCallNumber > _KSERVICE_TABLE_DESCRIPTOR.NumberOfServices ?
.text:000000000028F375 190                                                  ; 这里加 rdi 表示,如果是 GDI 调用,则直接索引到 Shadow SSDT 的字段
.text:000000000028F37A 190                 jnb     loc_28F678
.text:000000000028F380 190                 mov     r10, [rdi+r10]           ; r10 = _KSERVICE_TABLE_DESCRIPTOR.ServiceTableBase
.text:000000000028F384 190                 movsxd  r11, dword ptr [r10+rax*4] ; r11 = [ServiceTableBase + SystemCallNumber * 4]
.text:000000000028F388 190                 mov     rax, r11
.text:000000000028F38B 190                 sar     r11, 4                   ; r11 >> 4
.text:000000000028F38F 190                 add     r10, r11                 ; 系统服务例程地址 r10 = ServiceTableBase + r11
.text:000000000028F392 190                 cmp     edi, 20h
.text:000000000028F395 190                 jnz     short NonGDITebAccess    ; 检查是否为 GUI API
.text:000000000028F397 190                 mov     r11, [rbx+0F0h]          ; r11 = _KPCR.Prcb.CurrentThread.Teb
.text:000000000028F39E
.text:000000000028F39E     KiSystemServiceGdiTebAccess:            ; DATA XREF: KiSystemServiceHandler+D
.text:000000000028F39E 190                 cmp     dword ptr [r11+1740h], 0 ; Teb.GdiBatchCount
.text:000000000028F3A6 190                 jz      short NonGDITebAccess
.text:000000000028F3A8 190                 mov     [rbp-50h], rax
.text:000000000028F3AC 190                 mov     [rbp-48h], rcx
.text:000000000028F3B0 190                 mov     [rbp-40h], rdx
.text:000000000028F3B4 190                 mov     rbx, r8
.text:000000000028F3B7 190                 mov     rdi, r9
.text:000000000028F3BA 190                 mov     rsi, r10
.text:000000000028F3BD 190                 mov     rcx, 7
.text:000000000028F3C4 190                 xor     edx, edx
.text:000000000028F3C6 190                 xor     r8, r8
.text:000000000028F3C9 190                 xor     r9, r9
.text:000000000028F3CC 190                 call    PsInvokeWin32Callout
.text:000000000028F3D1 190                 mov     rax, [rbp-50h]
.text:000000000028F3D5 190                 mov     rcx, [rbp-48h]
.text:000000000028F3D9 190                 mov     rdx, [rbp-40h]
.text:000000000028F3DD 190                 mov     r8, rbx
.text:000000000028F3E0 190                 mov     r9, rdi
.text:000000000028F3E3 190                 mov     r10, rsi
.text:000000000028F3E6                     db      66h, 66h
.text:000000000028F3E6 190                 nop     word ptr [rax+rax+00000000h]
.text:000000000028F3F0
.text:000000000028F3F0     NonGDITebAccess:                        ; CODE XREF: KiSystemCall64+195
.text:000000000028F3F0                                             ; KiSystemCall64+1A6
.text:000000000028F3F0 190                 and     eax, 0Fh                 ; 检查需要通过栈传递的参数有几个, ArgumentCount - 4 (RCX, RDX, R8, R9)
.text:000000000028F3F3 190                 jz      KiSystemServiceCopyEnd
.text:000000000028F3F9 190                 shl     eax, 3                   ; 计算栈参数总字节数 Count * 8
.text:000000000028F3FC 190                 lea     rsp, [rsp-70h]           ; 
.text:000000000028F401 190                 lea     rdi, [rsp+190h+var_178]  ; 
.text:000000000028F406 190                 mov     rsi, [rbp+100h]          ; rsi = Ring3 Rsp
.text:000000000028F40D 190                 lea     rsi, [rsi+20h]           ; 
.text:000000000028F411 190                 test    byte ptr [rbp+0F0h], 1
.text:000000000028F418 190                 jz      short loc_28F430
.text:000000000028F41A 190                 cmp     rsi, cs:MmUserProbeAddress
.text:000000000028F421 190                 cmovnb  rsi, cs:MmUserProbeAddress
.text:000000000028F429 190                 nop     dword ptr [rax+00000000h]
.text:000000000028F430
.text:000000000028F430     loc_28F430:                             ; CODE XREF: KiSystemCall64+218
.text:000000000028F430 190                 lea     r11, KiSystemServiceCopyEnd
.text:000000000028F437 190                 sub     r11, rax
.text:000000000028F43A 190                 jmp     r11                      ; r11指向的 KiSystemServiceCopyStart 会拷贝系统调用的参数到内核栈
.text:000000000028F43D     ; ---------------------------------------------------------------------------
.text:000000000028F43D 190                 nop     dword ptr [rax]
.text:000000000028F440
.text:000000000028F440     KiSystemServiceCopyStart:               ; DATA XREF: KiSystemServiceHandler+1A
.text:000000000028F440 190                 mov     rax, [rsi+70h]
.text:000000000028F444 190                 mov     [rdi+70h], rax
.text:000000000028F448 190                 mov     rax, [rsi+68h]
.text:000000000028F44C 190                 mov     [rdi+68h], rax
.text:000000000028F450 190                 mov     rax, [rsi+60h]
.text:000000000028F454 190                 mov     [rdi+60h], rax
.text:000000000028F458 190                 mov     rax, [rsi+58h]
.text:000000000028F45C 190                 mov     [rdi+58h], rax
.text:000000000028F460 190                 mov     rax, [rsi+50h]
.text:000000000028F464 190                 mov     [rdi+50h], rax
.text:000000000028F468 190                 mov     rax, [rsi+48h]
.text:000000000028F46C 190                 mov     [rdi+48h], rax
.text:000000000028F470 190                 mov     rax, [rsi+40h]
.text:000000000028F474 190                 mov     [rdi+40h], rax
.text:000000000028F478 190                 mov     rax, [rsi+38h]
.text:000000000028F47C 190                 mov     [rdi+38h], rax
.text:000000000028F480 190                 mov     rax, [rsi+30h]
.text:000000000028F484 190                 mov     [rdi+30h], rax
.text:000000000028F488 190                 mov     rax, [rsi+28h]
.text:000000000028F48C 190                 mov     [rdi+28h], rax
.text:000000000028F490 190                 mov     rax, [rsi+20h]
.text:000000000028F494 190                 mov     [rdi+20h], rax
.text:000000000028F498 190                 mov     rax, [rsi+18h]
.text:000000000028F49C 190                 mov     [rdi+18h], rax
.text:000000000028F4A0 190                 mov     rax, [rsi+10h]
.text:000000000028F4A4 190                 mov     [rdi+10h], rax
.text:000000000028F4A8 190                 mov     rax, [rsi+8]
.text:000000000028F4AC 190                 mov     [rdi+8], rax
.text:000000000028F4B0
.text:000000000028F4B0     KiSystemServiceCopyEnd:                 ; CODE XREF: KiSystemCall64+1F3
.text:000000000028F4B0                                             ; DATA XREF: KiSystemServiceHandler+27 ...
.text:000000000028F4B0 190                 test    cs:[PerfGlobalGroupMask + 8], 40h    ; Check PERF_SYSCALL ???
.text:000000000028F4BA 190                 jnz     loc_28F716
.text:000000000028F4C0 190                 call    r10                      ; 调用计算出来的系统服务例程
.text:000000000028F4C3
.text:000000000028F4C3     loc_28F4C3:                             ; CODE XREF: KiSystemCall64+56B
.text:000000000028F4C3 190                 inc     dword ptr gs:2E38h       ; ++_KPCR.Pcrb.KeSystemCalls
.text:000000000028F4CB
.text:000000000028F4CB     KiSystemServiceExit:                    ; CODE XREF: KiSystemCall64+B6
.text:000000000028F4CB                                             ; KiSystemCall64+4D2 ...
.text:000000000028F4CB                                                      ; 开始恢复寄存器
.text:000000000028F4CB 190                 mov     rbx, [rbp+0C0h]          ; rbx = TrapFrame.Rbx
.text:000000000028F4D2 190                 mov     rdi, [rbp+0C8h]          ; rdi = TrapFrame.Rdi
.text:000000000028F4D9 190                 mov     rsi, [rbp+0D0h]          ; rsi = TrapFrame.Rsi
.text:000000000028F4E0 190                 mov     r11, gs:188h             ; r11 = _KPCR.Prcb.CurrentThread (_KTHREAD)
.text:000000000028F4E9 190                 test    byte ptr [rbp+0F0h], 1   ; TrapFrame.SegCs.CPL == Ring0 ?
.text:000000000028F4F0 190                 jz      ServiceExitRing0         ; CPL 为 Ring0 则跳转
.text:000000000028F4F6 190                 mov     rcx, cr8                 ; Task Priority Register
.text:000000000028F4FA 190                 or      cl, [r11+242h]           ; CurrentThread.ApcStateIndex
.text:000000000028F501 190                 or      ecx, [r11+1E4h]          ; CurrentThread.KernelApcDisable
.text:000000000028F508 190                 jnz     loc_28F6E2
.text:000000000028F50E 190                 cli
.text:000000000028F50F 190                 mov     rcx, gs:188h
.text:000000000028F518 190                 cmp     byte ptr [rcx+0C2h], 0
.text:000000000028F51F 190                 jz      short loc_28F578
.text:000000000028F521 190                 mov     [rbp-50h], rax
.text:000000000028F525 190                 xor     eax, eax
.text:000000000028F527 190                 mov     [rbp-48h], rax
.text:000000000028F52B 190                 mov     [rbp-40h], rax
.text:000000000028F52F 190                 mov     [rbp-38h], rax
.text:000000000028F533 190                 mov     [rbp-30h], rax
.text:000000000028F537 190                 mov     [rbp-28h], rax
.text:000000000028F53B 190                 mov     [rbp-20h], rax
.text:000000000028F53F 190                 pxor    xmm0, xmm0
.text:000000000028F543 190                 movaps  xmmword ptr [rbp-10h], xmm0
.text:000000000028F547 190                 movaps  xmmword ptr [rbp+0], xmm0
.text:000000000028F54B 190                 movaps  xmmword ptr [rbp+10h], xmm0
.text:000000000028F54F 190                 movaps  xmmword ptr [rbp+20h], xmm0
.text:000000000028F553 190                 movaps  xmmword ptr [rbp+30h], xmm0
.text:000000000028F557 190                 movaps  xmmword ptr [rbp+40h], xmm0
.text:000000000028F55B 190                 mov     ecx, 1
.text:000000000028F560 190                 mov     cr8, rcx
.text:000000000028F564 190                 sti
.text:000000000028F565 190                 call    KiInitiateUserApc
.text:000000000028F56A 190                 cli
.text:000000000028F56B 190                 mov     ecx, 0
.text:000000000028F570 190                 mov     cr8, rcx
.text:000000000028F574 190                 mov     rax, [rbp-50h]
.text:000000000028F578
.text:000000000028F578     loc_28F578:                             ; CODE XREF: KiSystemCall64+31F
.text:000000000028F578 190                 mov     rcx, gs:188h
.text:000000000028F581 190                 test    dword ptr [rcx], 40010000h
.text:000000000028F587 190                 jz      short loc_28F5B7
.text:000000000028F589 190                 mov     [rbp-50h], rax
.text:000000000028F58D 190                 test    byte ptr [rcx+2], 1
.text:000000000028F591 190                 jz      short loc_28F5A1
.text:000000000028F593 190                 call    KiCopyCounters
.text:000000000028F598 190                 mov     rcx, gs:188h
.text:000000000028F5A1
.text:000000000028F5A1     loc_28F5A1:                             ; CODE XREF: KiSystemCall64+391
.text:000000000028F5A1 190                 test    byte ptr [rcx+3], 40h
.text:000000000028F5A5 190                 jz      short loc_28F5B3
.text:000000000028F5A7 190                 lea     rsp, [rbp-80h]
.text:000000000028F5AB 190                 xor     rcx, rcx
.text:000000000028F5AE 190                 call    KiUmsExit
.text:000000000028F5B3
.text:000000000028F5B3     loc_28F5B3:                             ; CODE XREF: KiSystemCall64+3A5
.text:000000000028F5B3 190                 mov     rax, [rbp-50h]
.text:000000000028F5B7
.text:000000000028F5B7     loc_28F5B7:                             ; CODE XREF: KiSystemCall64+387
.text:000000000028F5B7 190                 ldmxcsr dword ptr [rbp-54h]
.text:000000000028F5BB 190                 xor     r10, r10
.text:000000000028F5BE 190                 cmp     word ptr [rbp+80h], 0    ; TrapFrame.ErrorCode == 0 ?
.text:000000000028F5C6 190                 jz      short ServiceExitRing3
.text:000000000028F5C8 190                 mov     [rbp-50h], rax
.text:000000000028F5CC 190                 call    KiRestoreDebugRegisterState
.text:000000000028F5D1 190                 mov     rax, gs:188h
.text:000000000028F5DA 190                 mov     rax, [rax+0B8h]
.text:000000000028F5E1 190                 mov     rax, [rax+2C0h]
.text:000000000028F5E8 190                 or      rax, rax
.text:000000000028F5EB 190                 jz      short loc_28F605
.text:000000000028F5ED 190                 cmp     word ptr [rbp+0F0h], 33h ; TrapFrame.SegCs == User CS ?
.text:000000000028F5F5 190                 jnz     short loc_28F605
.text:000000000028F5F7 190                 mov     r10, [rbp+0E8h]          ; CurrentThread.Queue
.text:000000000028F5FE 190                 mov     [rbp+0E8h], rax
.text:000000000028F605
.text:000000000028F605     loc_28F605:                             ; CODE XREF: KiSystemCall64+3EB
.text:000000000028F605                                             ; KiSystemCall64+3F5
.text:000000000028F605 190                 mov     rax, [rbp-50h]
.text:000000000028F609
.text:000000000028F609     ServiceExitRing3:                       ; CODE XREF: KiSystemCall64+3C6
.text:000000000028F609 190                 mov     r8, [rbp+100h]           ; r8 = TrapFram.Rsp
.text:000000000028F610 190                 mov     r9, [rbp+0D8h]           ; r9 = TrapFram.Rbp
.text:000000000028F617 190                 xor     edx, edx                 ; 0
.text:000000000028F619 190                 pxor    xmm0, xmm0               ; 下面 pxor 的全是重置为 0
.text:000000000028F61D 190                 pxor    xmm1, xmm1
.text:000000000028F621 190                 pxor    xmm2, xmm2
.text:000000000028F625 190                 pxor    xmm3, xmm3
.text:000000000028F629 190                 pxor    xmm4, xmm4
.text:000000000028F62D 190                 pxor    xmm5, xmm5
.text:000000000028F631 190                 mov     rcx, [rbp+0E8h]          ; rcx = TrapFrame.Rip
.text:000000000028F638 190                 mov     r11, [rbp+0F8h]          ; r11 = TrapFrame.EFlags
.text:000000000028F63F 190                 mov     rbp, r9                  ; 恢复 Ring3 栈 
.text:000000000028F642 190                 mov     rsp, r8                  ;
.text:000000000028F645 000                 swapgs                           ; 从 MSR[KernelGSBase] 交换回 User GS
.text:000000000028F648 000                 sysret                           ; 返回 Ring3
.text:000000000028F64B
.text:000000000028F64B     ServiceExitRing0:                             ; CODE XREF: KiSystemCall64+2F0
.text:000000000028F64B 190                 mov     rdx, [rbp+0B8h]          ; rdx = TrapFrame.TrapFrame
.text:000000000028F652 190                 mov     [r11+90h], rdx           ; CurrentThread.TrapFrame = rdx
.text:000000000028F659 190                 mov     dl, [rbp-58h]            ; dl = TrapFrame.PreviousMode
.text:000000000028F65C 190                 mov     [r11+232h], dl           ; CurrentThread.PreviousMode = dl
.text:000000000028F663 190                 cli
.text:000000000028F664 190                 mov     rsp, rbp
.text:000000000028F667 000                 mov     rbp, [rbp+0D8h]
.text:000000000028F66E 000                 mov     rsp, [rsp+arg_F8]
.text:000000000028F676 000                 sti
.text:000000000028F677 000                 retn
.text:000000000028F678     ; ---------------------------------------------------------------------------
.text:000000000028F678
.text:000000000028F678     loc_28F678:                             ; CODE XREF: KiSystemCall64+17A
.text:000000000028F678 190                 cmp     edi, 20h
.text:000000000028F67B 190                 jnz     short loc_28F6D8
.text:000000000028F67D 190                 mov     [rbp-80h], eax
.text:000000000028F680 190                 mov     [rbp-78h], rcx
.text:000000000028F684 190                 mov     [rbp-70h], rdx
.text:000000000028F688 190                 mov     [rbp-68h], r8
.text:000000000028F68C 190                 mov     [rbp-60h], r9
.text:000000000028F690 190                 call    KiConvertToGuiThread
.text:000000000028F695 190                 or      eax, eax
.text:000000000028F697 190                 mov     eax, [rbp-80h]
.text:000000000028F69A 190                 mov     rcx, [rbp-78h]
.text:000000000028F69E 190                 mov     rdx, [rbp-70h]
.text:000000000028F6A2 190                 mov     r8, [rbp-68h]
.text:000000000028F6A6 190                 mov     r9, [rbp-60h]
.text:000000000028F6AA 190                 mov     [rbx+90h], rsp
.text:000000000028F6B1 190                 jz      KiSystemServiceRepeat
.text:000000000028F6B7 190                 lea     rdi, qword_8AABA0
.text:000000000028F6BE 190                 mov     esi, [rdi+10h]
.text:000000000028F6C1 190                 mov     rdi, [rdi]
.text:000000000028F6C4 190                 cmp     eax, esi
.text:000000000028F6C6 190                 jnb     short loc_28F6D8
.text:000000000028F6C8 190                 lea     rdi, [rdi+rsi*4]
.text:000000000028F6CC 190                 movsx   eax, byte ptr [rax+rdi]
.text:000000000028F6D0 190                 or      eax, eax
.text:000000000028F6D2 190                 jle     KiSystemServiceExit
.text:000000000028F6D8
.text:000000000028F6D8     loc_28F6D8:                             ; CODE XREF: KiSystemCall64+47B
.text:000000000028F6D8                                             ; KiSystemCall64+4C6
.text:000000000028F6D8 190                 mov     eax, 0C000001Ch          ;STATUS_INVALID_SYSTEM_SERVICE
.text:000000000028F6DD 190                 jmp     KiSystemServiceExit
.text:000000000028F6E2     ; ---------------------------------------------------------------------------
.text:000000000028F6E2
.text:000000000028F6E2     loc_28F6E2:                             ; CODE XREF: KiSystemCall64+308
.text:000000000028F6E2 190                 mov     ecx, 4Ah
.text:000000000028F6E7 190                 xor     r9d, r9d
.text:000000000028F6EA 190                 mov     r8, cr8
.text:000000000028F6EE 190                 or      r8d, r8d
.text:000000000028F6F1 190                 jnz     short loc_28F707
.text:000000000028F6F3 190                 mov     ecx, 1
.text:000000000028F6F8 190                 movzx   r8d, byte ptr [r11+242h]
.text:000000000028F700 190                 mov     r9d, [r11+1E4h]
.text:000000000028F707
.text:000000000028F707     loc_28F707:                             ; CODE XREF: KiSystemCall64+4F1
.text:000000000028F707 190                 mov     rdx, [rbp+0E8h]
.text:000000000028F70E 190                 mov     r10, rbp
.text:000000000028F711 190                 call    KiBugCheckDispatch
.text:000000000028F716     ; ---------------------------------------------------------------------------
.text:000000000028F716
.text:000000000028F716     loc_28F716:                             ; CODE XREF: KiSystemCall64+2BA
.text:000000000028F716 190                 sub     rsp, 50h
.text:000000000028F71A 1E0                 mov     [rsp+1E0h+var_1C0], rcx
.text:000000000028F71F 1E0                 mov     [rsp+1E0h+var_1B8], rdx
.text:000000000028F724 1E0                 mov     [rsp+1E0h+var_1B0], r8
.text:000000000028F729 1E0                 mov     [rsp+1E0h+var_1A8], r9
.text:000000000028F72E 1E0                 mov     [rsp+1E0h+var_1A0], r10
.text:000000000028F733 1E0                 mov     rcx, r10
.text:000000000028F736 1E0                 call    PerfInfoLogSysCallEntry
.text:000000000028F73B 1E0                 mov     rcx, [rsp+1E0h+var_1C0]
.text:000000000028F740 1E0                 mov     rdx, [rsp+1E0h+var_1B8]
.text:000000000028F745 1E0                 mov     r8, [rsp+1E0h+var_1B0]
.text:000000000028F74A 1E0                 mov     r9, [rsp+1E0h+var_1A8]
.text:000000000028F74F 1E0                 mov     r10, [rsp+1E0h+var_1A0]
.text:000000000028F754 1E0                 add     rsp, 50h
.text:000000000028F758 190                 call    r10
.text:000000000028F75B 190                 mov     [rbp-50h], rax
.text:000000000028F75F 190                 mov     rcx, rax
.text:000000000028F762 190                 call    PerfInfoLogSysCallExit
.text:000000000028F767 190                 mov     rax, [rbp-50h]
.text:000000000028F76B 190                 jmp     loc_28F4C3
.text:000000000028F76B     KiSystemCall64  endp
.text:000000000028F76B
.text:000000000028F770     ; ---------------------------------------------------------------------------
.text:000000000028F770                     retn
```

#### KiSystemCall32

```x86asm
.text:000000014028EF40     KiSystemCall32  proc near               ; DATA XREF: .pdata:00000001408842E8o
.text:000000014028EF40                                             ; KiInitializeBootStructures+333o
.text:000000014028EF40
.text:000000014028EF40     TF_Xmm1Offset          = byte ptr  80h
.text:000000014028EF40
.text:000000014028EF40 000                 swapgs                           ; GS.Base 与 MSR[C0000102] (KernelGSBase) 交换, 此时指向内核 GS
.text:000000014028EF43 000                 mov     gs:10h, rsp              ; 保存用户态栈到 _KPCR.UserRsp
.text:000000014028EF4C 000                 mov     rsp, gs:1A8h             ; 从 _KPCR.Prcb.RspBase 加载内核态栈
.text:000000014028EF55 -190                push    2Bh                      ; 开始构建 TrapFrame, TrapFrame.SegSs = 0x2B
.text:000000014028EF57 -188                push    qword ptr gs:10h         ; TrapFrame.Rsp = _KPCR.UserRsp (用户态栈)
.text:000000014028EF5F -180                push    r11                      ; TrapFrame.EFlags = r11 (用户态 rflags)
.text:000000014028EF61 -178                push    23h                      ; TrapFrame.SegCs = 0x23
.text:000000014028EF63 -170                push    rcx                      ; TrapFrame.Rip = rcx (这个是用户态 syscall 下一条指令的地址)
.text:000000014028EF64 -168                swapgs                           ; GS.Base 与 MSR[C0000102] (KernelGSBase) 交换, 此时指向用户 GS
.text:000000014028EF67 -168                sub     rsp, 8                   ; 调整栈, 跳过 TrapFrame.ErrorCode
.text:000000014028EF6B -160                push    rbp                      ; TrapFrame.Rbp = rbp
.text:000000014028EF6C -158                sub     rsp, 158h                ; 调整 TrapFrame 起始地址, 0x158 + 0x38 = 0x190 即这个结构体从尾部开始填充数据, 然后其他未处理的部分直接调整栈来分配足够的空间.
.text:000000014028EF73 000                 lea     rbp, [rsp+TF_Xmm1Offset]
.text:000000014028EF7B 000                 mov     byte ptr [rbp-55h], 1    ; TrapFrame.ExceptionActive = 1
.text:000000014028EF7F 000                 mov     [rbp-50h], rax           ; TrapFrame.Rax = rax
.text:000000014028EF83 000                 mov     [rbp-48h], rcx           ; TrapFrame.Rcx = rcx
.text:000000014028EF87 000                 mov     [rbp-40h], rdx           ; TrapFrame.Rdx = rdx
.text:000000014028EF8B 000                 mov     [rbp-38h], r8            ; TrapFrame.R8 = r8
.text:000000014028EF8F 000                 mov     [rbp-30h], r9            ; TrapFrame.R9 = r9
.text:000000014028EF93 000                 mov     [rbp-28h], r10           ; TrapFrame.R10 = r10
.text:000000014028EF97 000                 mov     [rbp-20h], r11           ; TrapFrame.R11 = r11
.text:000000014028EF9B 000                 test    byte ptr [rbp+0F0h], 1   ; TrapFrame.SegCs.CPL == Ring0 ?
.text:000000014028EFA2 000                 jz      short loc_14028F008      ; 
.text:000000014028EFA4 000                 swapgs                           ; GS.Base 与 MSR[C0000102] (KernelGSBase) 交换, 此时指向内核 GS
.text:000000014028EFA7 000                 mov     r10, gs:188h             ; r10 = _KPCR.Prcb.CurrentThread
.text:000000014028EFB0 000                 test    byte ptr [r10+3], 80h    ; CurrentThread.DispatchHeader.DebugActive(0x80).UmsPrimary
.text:000000014028EFB5 000                 jz      short loc_14028EFF3
.text:000000014028EFB7 000                 mov     ecx, 0C0000102h          ; 读取用户 GS
.text:000000014028EFBC 000                 rdmsr
.text:000000014028EFBE 000                 shl     rdx, 20h
.text:000000014028EFC2 000                 or      rax, rdx                 ; rax = User GS, 即 TEB
.text:000000014028EFC5 000                 cmp     [r10+0F0h], rax          ; CurrentThread.Teb == User GS ?
.text:000000014028EFCC 000                 jz      short loc_14028EFF3
.text:000000014028EFCE 000                 cmp     [r10+200h], rax          ; CurrentThread.TebMappedLowVa == User GS ?
.text:000000014028EFD5 000                 jz      short loc_14028EFF3
.text:000000014028EFD7 000                 mov     rdx, [r10+1F0h]          ; rdx = CurrentThread.Ucb (_UMS_CONTROL_BLOCK)
.text:000000014028EFDE 000                 bts     dword ptr [r10+74h], 9   ; CurrentThread.MiscFlags.UmsDirectedSwitchEnable = 1
.text:000000014028EFE4 000                 dec     word ptr [r10+1E6h]      ; --CurrentThread.SpecialApcDisable
.text:000000014028EFEC 000                 mov     [rdx+80h], rax
.text:000000014028EFF3
.text:000000014028EFF3     loc_14028EFF3:                          ; CODE XREF: KiSystemCall32+75j
.text:000000014028EFF3                                             ; KiSystemCall32+8Cj ...
.text:000000014028EFF3 000                 test    byte ptr [r10+3], 3      ; CurrentThread.DispatchHeader.DebugActive(0x3).(ActiveDR7 & Instrumented)
.text:000000014028EFF8 000                 mov     word ptr [rbp+80h], 0
.text:000000014028F001 000                 jz      short loc_14028F008
.text:000000014028F003 000                 call    KiSaveDebugRegisterState
.text:000000014028F008
.text:000000014028F008     loc_14028F008:                          ; CODE XREF: KiSystemCall32+62j
.text:000000014028F008                                             ; KiSystemCall32+C1j
.text:000000014028F008 000                 cld
.text:000000014028F009 000                 stmxcsr dword ptr [rbp-54h]
.text:000000014028F00D 000                 ldmxcsr dword ptr gs:180h
.text:000000014028F016 000                 movaps  xmmword ptr [rbp-10h], xmm0
.text:000000014028F01A 000                 movaps  xmmword ptr [rbp+0], xmm1
.text:000000014028F01E 000                 movaps  xmmword ptr [rbp+10h], xmm2
.text:000000014028F022 000                 movaps  xmmword ptr [rbp+20h], xmm3
.text:000000014028F026 000                 movaps  xmmword ptr [rbp+30h], xmm4
.text:000000014028F02A 000                 movaps  xmmword ptr [rbp+40h], xmm5
.text:000000014028F02E 000                 test    qword ptr [rbp+0F8h], 200h   ; TrapFrame.EFlags & 0x200 ?
.text:000000014028F039 000                 jz      short loc_14028F03C
.text:000000014028F03B 000                 sti
.text:000000014028F03C
.text:000000014028F03C     loc_14028F03C:                          ; CODE XREF: KiSystemCall32+F9j
.text:000000014028F03C 000                 mov     ecx, 0C000001Dh ; STATUS_ILLEGAL_INSTRUCTION
.text:000000014028F041 000                 xor     edx, edx
.text:000000014028F043 000                 mov     r8, [rbp+0E8h]
.text:000000014028F04A 000                 call    KiExceptionDispatch
.text:000000014028F04F 000                 nop
.text:000000014028F050 000                 retn
.text:000000014028F050     KiSystemCall32  endp
```

#### KiInitializeBootStructures

那么, STAR, LSTAR, CSTAR, SFMASK 这几个寄存器是在哪里初始化的呢? 是在 nt!KiInitializeBootStructures 这个函数里面, 我们看下部分代码:

```x86asm
PAGELK:00000001408C5A00     loc_1408C5A00:                          ; CODE XREF: KiInitializeBootStructures+131j
PAGELK:00000001408C5A00                                             ; KiInitializeBootStructures+2C6j ...
PAGELK:00000001408C5A00 058                 mov     rax, [r14+8]
PAGELK:00000001408C5A04 058                 mov     ecx, 68h
PAGELK:00000001408C5A09 058                 mov     edx, 230010h
PAGELK:00000001408C5A0E 058                 mov     [rax+66h], cx
PAGELK:00000001408C5A12 058                 xor     eax, eax
PAGELK:00000001408C5A14 058                 mov     ecx, 0C0000081h         ; STAR
PAGELK:00000001408C5A19 058                 wrmsr
PAGELK:00000001408C5A1B 058                 lea     rdx, KiSystemCall32
PAGELK:00000001408C5A22 058                 mov     ecx, 0C0000083h         ; CSTAR
PAGELK:00000001408C5A27 058                 mov     rax, rdx
PAGELK:00000001408C5A2A 058                 shr     rdx, 20h
PAGELK:00000001408C5A2E 058                 wrmsr
PAGELK:00000001408C5A30 058                 lea     rdx, KiSystemCall64
PAGELK:00000001408C5A37 058                 mov     ecx, 0C0000082h         ; LSTAR
PAGELK:00000001408C5A3C 058                 mov     rax, rdx
PAGELK:00000001408C5A3F 058                 shr     rdx, 20h
PAGELK:00000001408C5A43 058                 wrmsr
PAGELK:00000001408C5A45 058                 mov     eax, 4700h
PAGELK:00000001408C5A4A 058                 xor     edx, edx
PAGELK:00000001408C5A4C 058                 mov     ecx, 0C0000084h         ;SFMASK
PAGELK:00000001408C5A51 058                 wrmsr
PAGELK:00000001408C5A53 058                 mov     eax, gs:1A4h
PAGELK:00000001408C5A5B 058                 test    eax, eax
PAGELK:00000001408C5A5D 058                 jnz     short loc_1408C5A64
PAGELK:00000001408C5A5F 058                 call    KiInitializeNxSupportDiscard
```

## 总结

最后, 拿 << Windows Internal 6th >> 中的一张图来简单总结以下调用过程:

![系统调用过程](serivce_dispatch.jpg)

## 引用参考

> << Windows内核情景分析 >>  
> << Windows Internal >>  
> << AMD64 Architecture Programmer’s Manual Volume 2: System Programming >>  
> << 64-ia-32-architectures-software-developer-vol-3-system-programming >>  
> << [mik - 使用 syscall/sysret 指令](http://www.mouseos.com/arch/syscall_sysret.html) >>  
