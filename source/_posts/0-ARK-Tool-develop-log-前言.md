---
title: '[0] ARK-Tool develop log : 前言'
date: 2016-02-08 17:24:33
tags: [内核,ARK,Windows]
---

# 前言

**声明：这一系列文章是我边学边写，内容不免会有错误，望指出不足，希望能够共同提高~么么哒~**


写一款自己的ARK工具的怨念已经产生很久了，一直拖拖拉拉到现在终于行动起来。

原因无非如下：

1. 学习
2. 装13，（ 哈哈哈哈哈~
3. 给自己的作品库填点儿玩具

<!--more-->

##### 开发环境：

* 测试平台	：Windows 7 x86/x64


* 语言		：C++11


* IDE		：Visual Studio 2015
* 其他工具 ：Windbg，IDA，Source Insight，PowerTool，WinObj，SymbolTypeViewer

##### 开发原则：

* Warning Level：4
* 安全，稳定，尽量无硬编码，设计时就支持x86/x64平台。

以 PowerTool 和 WIN64AST 为目标~

将采用C++11的一些特性来写，毕竟用起来很爽~

当然，在驱动不能使用原有的标准库，用到的东西基本上要自己先实现一套。

**注意：自己要在测试环境中测试，若造成系统损坏，本人将不负任何责任~**

##### 大致分为以下项目：

用户层：

* 界面
* 驱动管理模块

驱动层：

* 驱动模板库（即R0的STL）
* 驱动基础库
* ARK驱动

库将采用静态库的方式编译，每个静态库项目都会有个Unit的编译选项方便调试~

ARK项目我将其命名为 Illidan Stormrage，即魔兽里面的那个帅锅~

而驱动部分我将其命名为 Warglaive，就是它那把“埃辛诺斯战刃”

这一系列文章将会记录开发ARK这一路各个功能的实现方法，遇到的问题以及解决方案。

##### 致谢

[毛哥](https://github.com/erriy), [羡B](http://SmallB.github.io/), [ithurricanept (PowerTool作者)](http://powertool.s601.xrea.com/), [Tesla.Angela](http://www.m5home.com/bbs/forum.php), 小丽 (我司大神..)...

感谢 毛哥 平日里的各种科普，各种唠叨~

感谢 羡B 对我各种白痴问题解答~

感谢 Ithurricanept 大神的工具，以及公众号分享的知识~

感谢 TA 大神提供的基础教程，以及提供的论坛~

感谢 小丽 对我潜移默化的影响和鞭策~

... ...

![Illidan Stormrage](IllidanBCart.jpg)