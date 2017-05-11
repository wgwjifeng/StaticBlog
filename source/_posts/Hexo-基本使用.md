---
title: Hexo 基本使用
date: 2016-01-11 21:52:07
tags: Hexo
---

### Hexo 安装步骤

1. 安装`node.js`
2. 全局安装 hexo `npm install -g hexo-cli`
3. 如果有版本什么的警告可以试试这个 `npm install --unsafe-perm --verbose -g hexo`
4. `cd <file folder>` 到指定目录
5. `hexo init` 初始化hexo
6. `npm install` 安装依赖
7. `hexo generate` 生成静态网页
8. `hexo server` 在服务器上运行
9. 可以登录 [Loaclhost](http://localhost:4000/) 测试了

<!-- more -->

10. `hexo new post “标题” ` 创建一个博文
11. MD博文示例
	- `title: 博文标题`
	- `data : 2015-12-13 19:05:28`
	- `tags : 标签`
	- `updata: 最后修改时间`
	- `comments: `定义能否评论此文章（true）
	- `categories: 文章分类`
	- `<!-- more -->` 使文章在 more 位置折叠
	- `多级分类`
	```
	categories:
    - Sports
    - Baseball
	```


12. 配置文件——“_config.yml”
13. 主题在 themes 目录
14. 默认主题在 themes/landscape 目录 可以在配置文件里面改
15. 部署设置
	- 在配置文件 deploy 项
	- type: git (github 也是 git)
	- repository: 库地址
	- branch: 分支

16. 更新新版本 hexo `npm update -g hexo`
17. 查看 hexo 版本 `hexo version`

---

### 常用命令

#### 简写

`hexo n "我的博客"` == `hexo new "我的博客"` #新建文章
`hexo g` == `hexo generate` #生成
`hexo s` == `hexo server` #启动服务预览
`hexo d` == `hexo deploy` #部署

#### 服务器
`hexo server` #Hexo 会监视文件变动并自动更新，您无须重启服务器。
`hexo server -s` #静态模式
`hexo server -p 5000` #更改端口
`hexo server -i 192.168.1.1` #自定义 IP

`hexo clean` #清楚缓存，网页正常情况写可以忽略此条命令
`hexo g` #生成静态页面
`hexo d` #部署 （要先在 _Config.yml 配置文件配置 deploy 项）

#### 监视文件变动
>`hexo generate` #使用 Hexo 生成静态文件快速而且简单
>`hexo generate --watch` #监视文件变动

#### 完成后部署
两个命令作用是一样的
`hexo generate --deploy`
`hexo deploy --generate`

`hexo deploy -g`
`hexo server -g`

#### 模板
`hexo new "postname"` #新建文章
`hexo new page "pagename"` #新建页面

|变量      |描述        |
|:---------|:----------|
|:layout   |布局        |
|:title    |标题        |
|:date     |文件建立日期 |
> 其实这是测试表格的..

