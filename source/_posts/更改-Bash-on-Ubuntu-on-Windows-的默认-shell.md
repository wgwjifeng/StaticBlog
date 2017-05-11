---
title: 更改 Bash on Ubuntu on Windows 的默认 shell
date: 2016-08-28 23:32:15
tags: [Linux]
---

> 引用参考 [How to Use Zsh (or Another Shell) in Windows 10](http://www.howtogeek.com/258518/how-to-use-zsh-or-another-shell-in-windows-10/)

这里以 zsh 为例：

1. vim ~/.bashrc
2. 添加如下内容到 `#for examples` 注释下面，并保存
```bash
# Launch Zsh
if [ -t 1 ]; then
exec zsh
fi
```

3. 退出bash，到cmd，重新bash进入

