---
title: XP 兼容系列：神奇的静态对象 (magic statics)
date: 2016-01-13 08:25:01
tags: [Windows,XP 兼容系列]
---

## 神奇的静态对象 (magic statics)

前置声明：文章可能有错误的地方，希望各位童鞋能够提出~

事故缘由…
为了使用很爽的C++11 特性，我司最新项目是用的VS2015进行开发的。
但是同时又要对XP做兼容（讲真，我个人是不支持对Win7之前的系统做兼容的，我觉得影响发展）。

我们写了个COM组件作为插件，和驱动进行通讯。
在我们进行单元测试的时候，一切正常。但是出了测试安装包之后，发现加载插件会崩溃。
然后我们挂载了Windbg神器来定位崩溃点。
崩溃点是一个读取TLS，这个值为空
(外部静态对象才会有TLS)

<!-- more -->

想到单元测试程序也是通过VS2015编译的。我们就比较两个进程有啥不一样。
如图:
![](1.jpg)
![](2.jpg)

然后我们看一下 nt!_TEB 结构，发现 Tls Storage 就是 _TEB::ThreadLocalStoragePointer 字段。如图：
![](3.jpg)

于是我们查了一下 ReactOS 0.3.15 看下这个字段到底是啥，找到了这个分配Tls的函数

```
NTSTATUS
NTAPI
LdrpAllocateTls(VOID)
{
    PTEB Teb = NtCurrentTeb();
    PLIST_ENTRY NextEntry, ListHead;
    PLDRP_TLS_DATA TlsData;
    SIZE_T TlsDataSize;
    PVOID *TlsVector;

    /* Check if we have any entries */
    if (!LdrpNumberOfTlsEntries)
        return STATUS_SUCCESS;

    /* Allocate the vector array */
    TlsVector = RtlAllocateHeap(RtlGetProcessHeap(),
                                    0,
                                    LdrpNumberOfTlsEntries * sizeof(PVOID));
    if (!TlsVector) return STATUS_NO_MEMORY;
    Teb->ThreadLocalStoragePointer = TlsVector;

    /* Loop the TLS Array */
    ListHead = &LdrpTlsList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the entry */
        TlsData = CONTAINING_RECORD(NextEntry, LDRP_TLS_DATA, TlsLinks);
        NextEntry = NextEntry->Flink;

        /* Allocate this vector */
        TlsDataSize = TlsData->TlsDirectory.EndAddressOfRawData -
                      TlsData->TlsDirectory.StartAddressOfRawData;
        TlsVector[TlsData->TlsDirectory.Characteristics] = RtlAllocateHeap(RtlGetProcessHeap(),
                                                                           0,
                                                                           TlsDataSize);
        if (!TlsVector[TlsData->TlsDirectory.Characteristics])
        {
            /* Out of memory */
            return STATUS_NO_MEMORY;
        }

        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: TlsVector %x Index %d = %x copied from %x to %x\n",
                    TlsVector,
                    TlsData->TlsDirectory.Characteristics,
                    &TlsVector[TlsData->TlsDirectory.Characteristics],
                    TlsData->TlsDirectory.StartAddressOfRawData,
                    TlsVector[TlsData->TlsDirectory.Characteristics]);
        }

        /* Copy the data */
        RtlCopyMemory(TlsVector[TlsData->TlsDirectory.Characteristics],
                      (PVOID)TlsData->TlsDirectory.StartAddressOfRawData,
                      TlsDataSize);
    }

    /* Done */
    return STATUS_SUCCESS;
}
```
但是这个函数并不能得到太多有用信息。我们又看了下谁调用了它，得到了 LdrpInitializeTls 这个函数，从这个函数里面，我们就知道，实际上 _TEB::ThreadLocalStoragePointer 这个字段就是 初始化好的PE文件里面的 Tls 表。
```
NTSTATUS
NTAPI
LdrpInitializeTls(VOID)
{
    PLIST_ENTRY NextEntry, ListHead;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    PLDRP_TLS_DATA TlsData;
    ULONG Size;

    /* Initialize the TLS List */
    InitializeListHead(&LdrpTlsList);

    /* Loop all the modules */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        NextEntry = NextEntry->Flink;

        /* Get the TLS directory */
        TlsDirectory = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_TLS,
                                                    &Size);

        /* Check if we have a directory */
        if (!TlsDirectory) continue;

        /* Check if the image has TLS */
        if (!LdrpImageHasTls) LdrpImageHasTls = TRUE;

        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: Tls Found in %wZ at %p\n",
                    &LdrEntry->BaseDllName,
                    TlsDirectory);
        }

        /* Allocate an entry */
        TlsData = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(LDRP_TLS_DATA));
        if (!TlsData) return STATUS_NO_MEMORY;

        /* Lock the DLL and mark it for TLS Usage */
        LdrEntry->LoadCount = -1;
        LdrEntry->TlsIndex = -1;

        /* Save the cached TLS data */
        TlsData->TlsDirectory = *TlsDirectory;
        InsertTailList(&LdrpTlsList, &TlsData->TlsLinks);

        /* Update the index */
        *(PLONG)TlsData->TlsDirectory.AddressOfIndex = LdrpNumberOfTlsEntries;
        TlsData->TlsDirectory.Characteristics = LdrpNumberOfTlsEntries++;
    }

    /* Done setting up TLS, allocate entries */
    return LdrpAllocateTls();
}
```

到了这步，我们以为可以很容易的解决问题，既然需要Tls目录，那我们给它一个不就行了？
所以我们给测试代码添加了一个Tls目录..
```
#pragma comment(linker, "/INCLUDE:__tls_used")

#pragma data_seg(".CRT$XLB")

	PIMAGE_TLS_CALLBACK TlsCallBackArray[] = { TlsCallBackFunction };

#pragma data_seg()
```

不过我们还是太天真了..我们的Tls的回调啥也没做，所以在程序执行的时候，执行到并没有初始化的对象直接崩溃了..（对，VS2015生成的Tls表（回调）就是用来初始化静态对象的。）

后来...
我们在 MSDN 发现一个相关的说明
>Starting in C++11, a static local variable initialization is guaranteed to be thread-safe.This feature is sometimes called magic statics.However, in a multithreaded application all subsequent assignments must be synchronized.The thread-safe statics feature can be disabled by using the /Zc:threadSafeInit- flag to avoid taking a dependency on the CRT.

大致意思是，由于在C++11开始可以保证静态本地变量初始化时是线程安全的，即“神奇的静态对象”
但是这个特性是默认需要CRT支持的，所以要关闭则需要增加一条编译选项
```
/Zc:threadSafeInit-
```
这样在XP上运行就不会出现问题了。

好了，结束~
以此记录，来避免自己再遇到同样的坑 (●ˇ∀ˇ●)


>**引用链接：**
>[Storage class (C++)](https://msdn.microsoft.com/zh-cn/library/y5f6w579.aspx)


