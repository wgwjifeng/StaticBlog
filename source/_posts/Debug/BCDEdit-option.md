---
title: BCDEdit 命令行选项帮助
comments: true
date: 2017-05-12 14:09:12
tags: ['Debug']
categories: ['Debug']
---

## `/? ID`

查看所有已知标识符

`bcdedit /? ID`

<!--more-->

## `/? types <apptype>`

查看所有数据类型附加信息

`apptype` 可以是以下其中一项:

apptype     | description
:-----------|:-----------
bootapp     | 启动应用程序.这些类型也应用于启动管理器, 内存诊断应用程序, Windows OS 加载器和回复应用程序
bootmgr     | 启动管理器
bootsector  | 启动扇区应用程序
customtypes | 自定义类型
devobject   | 设备对象附加选项
fwbootmgr   | 固件启动管理器
memdiag     | 内存诊断应用程序
ntldr       | Vista 之前版本附带的 OS 加载器
osloader    | Vista OS 加载器
resume      | 恢复应用程序

## `/enum`

列出存储中的项

`bcdedit [/store <filename>] /enum [<type> | <id>] [/v]`

`type` 指定要列出的项的类型. 可以是以下类型之一:
* Active  : 启动管理器显示顺序中的所有项, 这是默认值.
* Firmware: 所有固件应用程序.
* Bootapp : 所有启动环境应用程序.
* Bootmgr : 启动管理器.
* Osloader: 所有操作系统项.
* Resume  : 所有从休眠状态恢复项.
* Inherit : 所有继承项.
* All     : 所有项.

`/v` 完整显示标识符,而不是使用已知标识符名称

## `/store`

用于指定当前系统默认值以外的 BCD 存储.

`bcdedit /store <filename>`

此命令行选项可与大多数 `Bcdedit` 命令一起使用以指定要使用的存储. 
如果未指定此选项, 则使用系统存储.

此选项不能与 `/createstore`, `/import` 和 `/export` 命令一起使用.

## `/createstore`

新建空的启动配置数据存储, 创建的存储不是系统存储.

`bcdedit /createstore <filename>`

## `/export`

此命令将系统存储的内容导出到文件. 
以后可以使用此文件还原系统存储的状态. 此命令仅对系统存储有效.

`bcdedit /export <filename>`

## `/import`

此命令使用以前使用 `/export` 命令生成的备份数据文件.

`bcdedit /import <filename> [/clean]`

还原系统存储的状态. 
在进行导入前, 将删除系统存储中的所有现有项. 此命令仅对系统存储有效

`/clean` 仅影响 EFI 系统.

## `/sysstore`

此命令用于设置系统存储设备. 
对于 EFI 系统, 仅在系统存储设备不确定的情况下, 此命令才有效. 此设置在重新启动后不再有效.

`bcdedit /sysstore <devicename>`

devicename 为系统分区盘符

## `/copy`

创建指定启动项的副本.

`bcdedit [/store <filename>] /copy {<id>} /d <description>`

## `/create`

在启动配置数据存储中创建新项.
如果指定了已知的标识符, 则不能指定 `/application`, `/inherit` 和 `/device` 选项.
如果未指定 id, 或者 id 未知, 则必须指定 `/application`, `/inherit` 或 `/device` 选项.

`bcdedit /create [{<id>}] [/d <description>] [/application <apptype> | /inherit [<apptype>] | /inherit DEVICE | /device]`

## `/delete`

删除启动配置数据存储中的项.

`bcdedit [/store <filename>] /delete <id> [/f] [/cleanup | /nocleanup]`

`/f` 删除指定的项. 如果没有此项,则将无法删除任何具有已知标识符的项.

`/cleanup` 删除指定的项, 并从显示顺序中删除该项. 并将从存储中删除任何其他涉及到所删除的项目.

`/nocleanup` 删除指定的项,但不从显示顺序中删除该项.

## `/mirror`

创建指定启动项的镜像.

`bcdedit [/store <filename>] /mirror {<id>}`

## `/bootsequence`

设置启动管理器使用的一次性启动序列.

`bcdedit /bootsequence <id> [...] [ /addfirst | /addlast | /remove ]`

`/addfirst` 将指定的项标识符添加到启动序列的顶部. 
如果已指定此参数, 则只能指定一个项标识符.如果列表中已存在该标识符, 则将其移动到列表顶部.

`/addlast` 将指定的项标识符添加到启动序列的尾部. 
如果已指定此参数, 则只能指定一个项标识符.如果列表中已存在该标识符, 则将其移动到列表尾部.

`/remove` 从启动序列中删除指定的项标识符. 
如果已指定此参数, 则只能指定一个项标识. 如果该标识符不在列表中, 则该操作不起作用.
如果删除最后一项, 则启动序列值将会从启动管理器项中删除.

## `/default`

设置启动管理器将使用的默认项

`bcdedit /default <id>`

## `/displayorder`

设置启动管理器显示, 多重启动菜单的顺序

`bcdedit /displayorder <id> [...] [ /addfirst | /addlast | /remove ]`

参数含义同 `/bootsequence`

## `/timeout`

设置启动管理器的超时值, 单位 `秒`

`bcdedit /timeout <timeout>`

## `/toolsdisplayorder`

设置启动管理器显示工具菜单的顺序

`bcdedit /toolsdisplayorder <id> [...] [ /addfirst | /addlast | /remove ]`

参数含义同 `/bootsequence`

## `/bootdebug`

开启当前或指定的系统引导调试

`bcdedit /bootdebug [{GUID}] { on | off }`

> 注: 使用 `/dbgsettings` 选项配置要使用的调试连接类型和连接参数.
> 如果没有指定, 则使用默认全局调试设置.

## `/dbgsettings`

指定或显示系统的全局调试程序设置. 此命令不启用或禁用内核调试程序；使用 `/debug` 选项完成此目的. 若要设置单个全局调试程序设置, 请使用 `bcdedit /setdbgsettings type value` 命令.

```bash
bcdedit /dbgsettings SERIAL [DEBUGPORT:port] [BAUDRATE:baud] [/start startpolicy] [/noumex] 
bcdedit /dbgsettings 1394 [CHANNEL:channel] [/start startpolicy] [/noumex] 
bcdedit /dbgsettings USB [TARGETNAME:targetname] [/start startpolicy] [/noumex] 
bcdedit /dbgsettings NET HOSTIP:ip PORT:port [KEY:key] [nodhcp] [newkey] [/start startpolicy] [/noumex] 
```

`/start startpolicy` 此选项指定了调试器启动策略, 下面显示了策略选项:
Option	    | Description
:-----------|:-----------
ACTIVE      | 指定内核调试器处于活动状态
AUTOENABLE  | 指定当异常或者其他严重事件发生时, 内核调试器将自动启用. 在此之前, 内核调试器处于活动状态, 但已被禁用
DISABLE     | 指定当您键入kdbgctrl以清除启用块时启用内核调试器. 在此之前, 调试器处于活动状态, 但已被禁用

`/noumex` 指定内核调试器忽略用户模式异常,默认情况下, 某些用户模式异常会中断内核调试器, 例如 STATUS_BREAKPOINT 和 STATUS_SINGLE_STEP. 只有当没有用户模式调试器附加到进程时, 该参数才有效


### Serial
[Setting Up Kernel-Mode Debugging over a Serial Cable Manually](https://msdn.microsoft.com/en-us/library/windows/hardware/ff556867(v=vs.85).aspx)

### 1394
[ Setting Up Kernel-Mode Debugging over a 1394 Cable Manually](https://msdn.microsoft.com/en-us/library/windows/hardware/ff556866(v=vs.85).aspx)

### USB
[Setting Up Kernel-Mode Debugging over a USB 2.0 Cable Manually](https://msdn.microsoft.com/en-us/library/windows/hardware/ff556869(v=vs.85).aspx)

[Setting Up Kernel-Mode Debugging over a USB 3.0 Cable Manually](https://msdn.microsoft.com/en-us/library/windows/hardware/hh439372(v=vs.85).aspx)

### Net
[Setting Up Kernel-Mode Debugging over a Network Cable Manually](https://msdn.microsoft.com/en-us/library/windows/hardware/hh439346(v=vs.85).aspx)

[Supported Ethernet NICs for Network Kernel Debugging in Windows 8.1](https://msdn.microsoft.com/en-us/library/windows/hardware/dn337010(v=vs.85).aspx)

[Supported Ethernet NICs for Network Kernel Debugging in Windows 8](https://msdn.microsoft.com/en-us/library/windows/hardware/dn337009(v=vs.85).aspx)

### Local
[Setting Up Local Kernel Debugging of a Single Computer Manually](https://msdn.microsoft.com/en-us/library/windows/hardware/dn553412(v=vs.85).aspx)

### 默认全局调试设置

dbgsetting parameter | Default value
:--------------------|:-------------
Debugtype            | Serial
Debugport            | 1
Baudrate             | 115200

## `/debug`

启用或禁用指定启动项的内核调试程序。

`bcdedit /debug [{ID}] { on | off }`

> 注: 使用 `/dbgsettings` 选项配置要使用的调试连接类型和连接参数.
> 如果没有指定, 则使用默认全局调试设置.

## `/hypervisorsettings`

设置虚拟机监控程序的参数

若要为特定的操作系统加载器项启用或禁用虚拟机监控程序调试程序, 请使用 `bcdedit /set <id> hypervisordebug on`

`bcdedit /hypervisorsettings [ <debugtype> [DEBUGPORT:<port>] [BAUDRATE:<baud>] [CHANNEL:<channel>] [HOSTIP:<ip>] [PORT:<port>] ]`

`debugtype` 指定调试程序的类型. 可以是 Serial, 1394, Net 之一

`port` 对于 Serial 调试, 指定要用作调试端口的串口

`baud` 对于 Serial 调试, 指定用于调试的波特率

`channel` 对于 1394 调试, 指定用于调试的 1394 通道

`ip` 对于 Net 调试, 指定主机调试程序的 IPV4 地址

`port` 对于 Net 调试, 指定在主机调试程序上通信的端口,应该是 49152 或更高

## `/event`

启用或禁用操作系统项的远程事件日志记录

`bcdedit /event [<id>] { ON | OFF }`

标识符只能指定 Windows 启动加载程序项.如果未指定,则使用 `{current}`

## `/deletevalue`

删除启动项中指定的元素。

`bcdedit /deletevalue [{ID}] datatype`

## `/bootems`

启用或禁用引导项的紧急管理服务

`bcdedit /bootems [<id>] { ON | OFF }`

## `/ems`

启用或禁用指定的操作系统启动项的紧急管理服务 (EMS).

`bcdedit /ems [{ID}] { on | off }`

## `/emsstings`

设置计算机的全局 EMS 设置. `/emssettings` 不启用或禁用任何特定启动项的 EMS.

`bcdedit /emssettings [ BIOS ] | [ EMSPORT: port | [EMSBAUDRATE: baudrate] ]`

## `/set`

设置一个项选项值.

`bcdedit  /set [{ID}] datatype value`

`datatype` 有以下几种类型:

`bootlog [yes | no]`

`bootmenupolicy [ Legacy | Standard ]`

`bootstatuspolicy policy`

`bootux [ disabled | basic | standard ]`

`disabledynamictick [ yes | no ]`

`disableelamdrivers [ yes | no ]`

`forcelegacyplatform [ yes | no ]`

`groupsize maxsize`

`groupaware [ on | off ]`

`hal file`

`hypervisorbusparams Bus.Device.Function`

`hypervisordebug [ On | Off ]`

`hypervisordebugport port`

`hypervisordebugtype [ Serial | 1394 | Net ]`

`hypervisorbaudrate [ 9600 | 19200 | 38400 | 57600 | 115200 ]`

`hypervisorchannel [ channel ]`

`hypervisorhostip IP`

`hypervisorhostport [ port ]`

`hypervisordhcp [ yes | no ]`

`hypervisoriommupolicy [ default | enable | disable]`

`hypervisorlaunchtype [ Off | Auto ]`

`hypervisorloadoptions NOFORCESNOOP [ Yes | No ]`

`hypervisornumproc number`

`hypervisorrootproc number`

`hypervisorrootprocpernode number`

`hypervisorusekey [ key ]`

`hypervisoruselargevtlb [ yes | no ]`

`increaseuserva Megabytes`

`kernel file`

`loadoptions busparams=Bus.Device.Function`

`maxgroup [ on | off ]`

`nointegritychecks [ on | off ]`

`nolowmem [ on | off ]`

`novesa [ on | off ]`

`novga [ on | off ]`

`nx [Optin |OptOut | AlwaysOn |AlwaysOff]`

`onecpu [ on | off ]`

`onetimeadvancedoptions [ on | off ]`

`pae [ Default | ForceEnable | ForceDisable ]`

`pciexpress [ default | forcedisable]`

`quietboot [ on | off ]`

`removememory Megabytes`

`sos [ on | off ]`

`testsigning [ on | off ]`

`tpmbootentropy [ default | ForceEnable | ForceDisable]`

`truncatememory address`

`tscsyncpolicy [ Default | Legacy | Enhanced ]`

`usefirmwarepcisettings [ yes | no ]`

`useplatformclock [ yes | no ]`

`uselegacyapicmode [ yes | no ]`

`useplatformtick [ yes | no ]`

`vga [ on | off ]`

`xsavedisable [ 0 | 1 ]`

`x2apicpolicy [ enable | disable ]`

# Boot.in 选项和 BCDEdit 选项的映射关系

Boot.ini            | BCDEdit option        | BCD element type
:-------------------|:----------------------|:----------------
/3GB                | increaseuserva        | BcdOSLoaderInteger_IncreaseUserVa
/BASEVIDEO          | vga                   | BcdOSLoaderBoolean_UseVgaDriver
/BOOTLOG            | bootlog               | BcdOSLoaderBoolean_BootLogInitialization
/BREAK              | halbreakpoint         | BcdOSLoaderBoolean_DebuggerHalBreakpoint
/CRASHDEBUG         | /dbgsettings /start   | 
/DEBUG, BOOTDEBUG   | /debug  /bootdebug    | BcdLibraryBoolean_DebuggerEnabled
/DEBUG              | /debug                | BcdOSLoaderBoolean_KernelDebuggerEnabled
/DEBUG, /DEBUGPORT= | /dbgsettings          | BcdLibraryInteger_DebuggerType
/DEBUGPORT=         | /dbgsettings          | BcdLibraryInteger_SerialDebuggerPort  BcdLibraryInteger_SerialDebuggerBaudRate  BcdLibraryInteger_1394DebuggerChannel  BcdLibraryString_UsbDebuggerTargetName  BcdLibraryInteger_DebuggerNetHostIP  BcdLibraryInteger_DebuggerNetPort  BcdLibraryBoolean_DebuggerNetDhcp  BcdLibraryString_DebuggerNetKey
/EXECUTE            | nx                    | BcdOSLoaderInteger_NxPolicy
/FASTDETECT         |                       | 
/HAL=               | hal                   | BcdOSLoaderString_HalPath
/KERNEL=            | kernel                | BcdOSLoaderString_KernelPath
/MAXMEM=            | truncatememory        | BcdLibraryInteger_TruncatePhysicalMemory
/NODEBUG            | /debug                | 
/NOEXECUTE          | nx {                  | BcdOSLoaderInteger_NxPolicy
/NOGUIBOOT          | quietboot             | BcdOSLoaderBoolean_DisableBootDisplay
/NOLOWMEM           | nolowmem              | BcdOSLoaderBoolean_NoLowMemory
/NOPAE              | pae                   | BcdOSLoaderInteger_PAEPolicy
/ONECPU             | onecpu                | BcdOSLoaderBoolean_UseBootProcessorOnly
/PAE                | pae                   | BcdOSLoaderInteger_PAEPolicy
/PCILOCK            | usefirmwarepcisettings| BcdOSLoaderInteger_UseFirmwarePciSettings
/REDIRECT           | /ems  /emssettings [ BIOS ] or  [ EMSPORT:{port} | [EMSBAUDRATE: {baudrate}] ] | BcdOSLoaderBoolean_EmsEnabled
/SOS                | sos                   | 
