---
title: PDU 编码规则
comments: true
date: 2017-05-18 11:36:02
tags: ['PDU']
---

PDU模式不仅支持中文短信，也能发送英文短信。PDU模式收发短信可以使用3种编码：7-bit、8-bit和UCS2编码。
7-bit编码用于发送普通的ASCII字符，8-bit编码通常用于发送数据消息，UCS2编码用于发送Unicode字符。

<!--more-->

PDU 内容总长度 140 个字节 (1120位)，支持采用三种编码方式：7-bit、8-bit 和 UCS2 编码。
7-bit 编码——用于发送普通的 ASCII 字符，ASCII码表最大到0x7X，最高位为0，总 7-bit，实际编码时则可把8-bit的最高位比特使用起来，所以可支持1120/7=160个字符；
8-bit 编码——用于发送数据消息，比如图片和铃声、二进制数据等，此类数据无法使用 7-bit 编码，因为那样会丢掉一位，也不能用下面UCS2编码，因为不符合 UNICODE 编码检查（范围）。8-bit 编码最多支持 140 个字节数据。
UCS2 编码——用于发送 Unicode 字符，每个中文（韩文、日文），占用 2 字节，只要短信里包含这些多字节编码文字，那么即使还有英文，英文也需要安装 UCS2 编码，也占用 2 字节，所以，最多支持 70 个中文字（或中英混合短信）

# 编码格式

一般的PDU编码由A B C D E F G H I J K L M十三项组成:

SCA (短信中心) 结构部分：    
AT指令中 AT+CMGS=<Len> Len不包含此段位组的长度

Index   | Item          | Description
:-------|:--------------|:------------
A       | `<sc_len>`    | B+C的长度, 1Byte  
B       | `<type_addr>` | SMSC地址类型, 1Byte  
C       | `<number>`    | SMSC 号码  

TPUD 结构部分：

Index   | Item          | Description
:-------|:--------------|:------------
D       | `<option>`    | 基本参数(TP-MTI/VFP), 1Byte  
E       | `<MR>`        | 短信标识符(TP-MR), 1Byte  
F       | `v`           | 目标号码长度, 1Byte  
G       | `<DA>`        | 被叫号码类型, 1Byte  
H       | `^`           | 被叫号码, 长度由F中的数据决定  
I       | `<PID>`       | 协议标识(TP-PID), 1Byte  
J       | `<DCS>`       | 数据编码(TP-DCS), 1Byte  
K       | `<VP>`        | 有效期(TP-VP), 1Byte  
L       | `<UDL>`       | 用户数据长度(TP-UDL), 1Byte  
M       | `<UD>`        | 用户数据  

## SCA 结构部分详细说明

`<sc_len>`：表示 <SCA>（短信中心号码）的长度，包含两个字符，指示 <type_addr> 和 <numbers> 所占字符的个数除于 2。

`<type_addr>`：表示号码地址类型，包含两个字符，其结构如下：

```
bit7    bit6    bit5    bit4    bit3    bit2    bit1    bit0
1          Type-of-number       Numbering-plan-identification
1       0       0       1       0       0       0       1
```

bit7 固定为 1

Type-of-number (bit6-bit4) 取值如下：

value   | Descation
:-------|:---------------
000     | 若用户不能识别目标地址号码时，选用此值。此时地址号码由网络侧决定。
001     | 若用户能识别是国际号码或者或者认为是国内范围时，选用此值。
010     | 国内号码，不允许加前缀或者后缀。在用户发送国内电话时，选用此值。
011     | 本网络内的特定号码，用于管理或者服务，用户不能选用此值。
101     | 号码类型为 GSM 的缺省 7-bit 编码方式。
110     | 短小号码，暂不使用。
111     | 扩展保留，暂不使用。

Numbering-plan-identification (bit3-bit0) 取值如下：
(注：当 bit6-bit4 取值为 000, 001, 010 时才有效，其他情况 bit3-bit0 无效)

value   | Descation
:-------|:---------------
0000    | 号码由网络侧的号码方案确定
0001    | ISDN/电话号码方案
0011    | 数据号码方案，暂不使用
0100    | Telex 号码方案，暂不使用
1000    | 国内号码方案，暂不使用
1001    | 私人号码方案，暂不使用
1010    | ERMES 号码方案，暂不使用

`<numbers>`：表示地址号码，一个字节储存两个数字，且 bit3~bit0 储存第一个数字，bit7~bit4 储存第二个数字，如果号码为奇数，则最后一位补F后，再进行反转。如果为偶数，则不需要补F。像是反序的 8421 BCD 码，可如下例所示：

例如有  
13 81 23 45 67 8  
存储为  
21 18 32 54 76 F8

## TPDU 结构部分

`<option>`：

```
RP    UDHI    SRR    VPF    RD    MTI
bit7  bit6    bit5   bit4-3 bit2  bit1-0
```

* `<MTI>`：表示短消息类型。

bit1  | bit0  | Descrption
:-----|:------|:-----
0     | 0     | SMS-DELIVER (SC 到 MT 方向)
0     | 0     | SMS-DELIVER-REPORT (MT 到 SC 方向)
1     | 0     | SMS-STATUS-REPORT (SC 到 MT 方向)
1     | 0     | SMS-COMMAND (MT 到 SC 方向)
0     | 1     | SMS-SUBMIT (MT 到 SC 方向)
0     | 1     | SMS-SUBMIT-REPROT (SC 到 MT 方向)
1     | 1     | 保留

* `<RD>`：指示 SC 是否需要接受一个仍保存在 SC 中，与以前同一 OA 发出具有相同的 MR 和 DA 的短消息。

0 接受    
1 不接受

* `<VPF>`：指示 VP 字段格式的有效性，格式指示。

bit1  | bit0  | Descrption
:-----|:------|:-----
0     | 0     | VP 字段无效
1     | 0     | VP 字段有效，格式为 "relative"
0     | 1     | VP 字段有效，格式为 "enhanced"
1     | 1     | VP 字段有效，格式为 "absolute"

* `<RP>`：回复短信路径的设置指示，与短信发送时的设置相同。

0 没有设置    
1 设置，指示回复短信与发送时具有相同的 SC 号码设置，返回路径相同。

* `UDHI`：用户数据头的指示。

0 用户数据段只有短消息的内容    
1 用户数据段除了短消息外，还包含有一个数据头

* `<SRR>`：状态报告请求指示。

0 不需要一个短信成功发送的状态报告信息    
1 需要一个短信成功发送的状态报告信息

* `<MR>`：表示短信标识符，取值范围为 0～255。

`<DA>`：目标地址信息，结构同 SCA，但是有一点不同需要注意！就是 len 字段不再是 type 和 number 的字节长度，而是 number 的字符长度  

`<PID>`：协议指示。

* Bit7-6 :

Bit7 | Bit6 | (目前， Bit 7=0 和 Bit 6=0)
:----|:-----|:--------------
0    | 0    | 分配 bits 0-5
1    | 0    | 分配 bits 0-5
0    | 1    | 保留
1    | 1    | 分配 bits 0-5，为 SC 的特殊用途

* Bit5 :

0 无交互操作，但有 SME-to-CSME 协议    
1 Telematic 交互操作 （此情况下， bit 4-0 的取值有效）

* Bit4-0 :

若取值为 10010，则表示 Email ，其它取值暂不支持。

`<DSC>`：表示用户数据的编码方式。

00 7-bit 编码 (英文)
04 8-bit 编码 (图片和铃声)
08 16-bit 编码 (UCS2)

Bit No.7与Bit No.6：
一般设置为00

Bit No.5：
0-文本未压缩
1-文本用GSM标准压缩算法压缩

Bit No.4：
0-指示Bit No.1 Bit No.0为保留位，不含信息类型信息
1-指示Bit No.1 Bit No.0含信息类型信息

Bit No.3与Bit No.2：
00-默认的字符集，每字符占7bit，此时最大可发送160字符
01-8bit，此时最大可发送140字符
10-USC2（16bit），发送双字节字符集
11-预留

Bit N0.1与Bit No.0：
00-Class 0，短消息直接显示在屏幕上
01-Class 1，
10-Class 2（SIM卡特定信息），
11-Class 3

`<VP>`：表示有效期，时间从短消息被 SC 接收到开始计算。如果 <VPF>=00，则该字段缺失，时间表示如下：

VP 取值   | 说明
:---------|:----------
0~143     | (VP + 1) x 5 minutes
144~167   | 12 hours + ((VP - 143) x 30 minutes)
168~196   | (VP - 166) x 1 day
197~255   | (VP - 192) x 1 week

`<UDL>`：表示用户数据长度，取值取决于具体的编码方式。

* 若是 7-bit 缺省编码， <UDL> 表示共有多少个 septets。
* 若是 8-bit 编码， <UDL> 表示共有多少个 Octets。
* 若是 UCS2 编码， <UDL> 表示共有多少个 Octets。
* 若是有压缩的 7-bit 或 8-bit 或 UCS2 编码， <UDL> 表示压缩后共有多少个 Octets。
* 对压缩的短信编码， <UD> 的数据长度不超过 160 septets；对无压缩编码的短信， <UD> 长度不超过 140 Octets。

`<UD>`：表示用户数据，其有效数据由参数 <UDL> 决定。

# 举个栗子

```
AT+CMGW=30
0891683108705505F011000791680180F60008AA1200480065006C006C006F002055B555B5FF01

AT 指令后面这个长度是 <TPUD 结构的8位字节个数>

首先看下 SCA 结构 0891683108705505F0

0x08
SCA 号码类型和号码的长度

0x91
SCA 号码为国际号码，即 "+"

683108705505F0
可解析短信中心号码为 8613800755500

然后看下 TPUD 结构 11000791680180F60008AA1200480065006C006C006F002055B555B5FF01

0x11 options
Bit No. 7       6       5       4       3       2       1       0
        RP      UDHI    SRR     VPF     VPF     RD      MTI     MTI
        0       0       0       1       0       0       0       1

0x00 MR
置为00即可

0791680180F6 DA
解析为 07 91 8610086

0x00 PID
对于标准情况下的MS-to-SC短消息传送，只需设置PID为00

0x08 DCS
USC2（16bit）双字节字符集

0xAA 有效期

第一种情况（相对的）：VPF=10  VP=AAH（四天），在这里是第一种
第二种情况（绝对的）：VPF=11

年	月	日	时	分	秒	时区
30	80	02	90	54	33	20
           表示：03-08-20 09:45:33

0x12 UDL
18 个8位字节

0048 0065 006C 006C 006F 0020 55B5 55B5 FF01
H    e    l    l    o    空格  喵   喵   ！
```