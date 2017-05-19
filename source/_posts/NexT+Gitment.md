---
title: 为 hexo NexT 添加 Gitment 评论插件
comments: true
date: 2017-05-19 01:28:52
tags: [Hexo]
---

Gitment 是作者[imsun](https://github.com/imsun/gitment)实现的一款基于 GitHub Issues 的评论系统. 支持在前端直接引入, 不需要任何后端代码. 可以在页面进行登录, 查看, 评论, 点赞等操作. 同时有完整的 Markdown / GFM 和代码高亮支持. 尤为适合各种基于 GitHub Pages 的静态博客或项目页面. 

这篇文章仅介绍如果在 hexo-NexT 中添加 Gitment 评论插件, 并且增加一个点开显示评论的按钮, 对于 Gitment 的使用请参考 imsun 的博客.

另外, 本教程的按钮样式和代码均直接取自 [ehlxr](https://ehlxr.me/) 博主.

<!--more-->

## "显示 Gitment 评论" 的按钮样式

在 `next/source/css/_common/components` 目录中新建一个 `gitment.styl` 的 css 样式文件, 复制以下代码

```css
.gitment_title:hover {
    color: #fff;
    background: #0a9caf;
    background-image: initial;
    background-position-x: initial;
    background-position-y: initial;
    background-size: initial;
    background-repeat-x: initial;
    background-repeat-y: initial;
    background-attachment: initial;
    background-origin: initial;
    background-clip: initial;
    background-color: rgb(10, 156, 175);
}

.gitment_title {
    border: 1px solid #0a9caf;
    border-top-color: rgb(10, 156, 175);
    border-top-style: solid;
    border-top-width: 1px;
    border-right-color: rgb(10, 156, 175);
    border-right-style: solid;
    border-right-width: 1px;
    border-bottom-color: rgb(10, 156, 175);
    border-bottom-style: solid;
    border-bottom-width: 1px;
    border-left-color: rgb(10, 156, 175);
    border-left-style: solid;
    border-left-width: 1px;
    border-image-source: initial;
    border-image-slice: initial;
    border-image-width: initial;
    border-image-outset: initial;
    border-image-repeat: initial;
    border-radius: 4px;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
    border-bottom-right-radius: 4px;
    border-bottom-left-radius: 4px;
}

.gitment_title {
    display: inline-block;
    padding: 0 15px;
    padding-top: 0px;
    padding-right: 15px;
    padding-bottom: 0px;
    padding-left: 15px;
    color: #0a9caf;
    cursor: pointer;
    font-size: 14px;
}
```

然后打开同目录中的 `components.styl` 文件, 找个顺眼的位置添加一句 `@import "gitment";`

## 添加 Gitment 插件

打开 `/next/layout/_partials/comments.swig` 文件, 在最后一个 elseif 代码块下面添加 Gitment 的内容.

例如我的就是这样

```javascript
    ... // 上面内容省略了..
    
    {% elseif theme.changyan.appid and theme.changyan.appkey %}
      <div id="SOHUCS"></div>
    {% elseif theme.gitment.enable %}
      <div onclick="showGitment()" id="gitment_title" class="gitment_title">显示 Gitment 评论</div>
      <div id="container" style="display:none"></div>
      <link rel="stylesheet" href="https://imsun.github.io/gitment/style/default.css">
      <script src="https://imsun.github.io/gitment/dist/gitment.browser.js"></script>
      <script>
      const myTheme = {
        render(state, instance) {
          const container = document.createElement('div');
          container.lang = "en-US";
          container.className = 'gitment-container gitment-root-container';
          container.appendChild(instance.renderHeader(state, instance));
          container.appendChild(instance.renderEditor(state, instance));
          container.appendChild(instance.renderComments(state, instance));
          container.appendChild(instance.renderFooter(state, instance));
          return container;
        }
      }

      function showGitment() {
        $("#gitment_title").attr("style", "display:none");
        $("#container").attr("style", "").addClass("gitment_container");
        var gitment = new Gitment({
          id: window.location.pathname,
          theme: myTheme,
          owner: '{{ theme.gitment.owner }}',
          repo: '{{ theme.gitment.repo }}',
          oauth: {
            client_id: '{{ theme.gitment.client_id }}',
            client_secret: '{{ theme.gitment.client_secret }}'
          }
        });
        gitment.render('container');
      }
      </script>
    {% endif %}
```

然后打开 NexT 主题的 `_config.yml` 文件, 在评论相关设置的区域添加下面的代码, 并根据 Gitment 文档说明来添加相应的值

```yaml
# Gitment comments
gitment:
  enable: true
  owner: xxxx
  repo: xxxx
  client_id: xxxx
  client_secret: xxxx
```

最终效果见本博客下方~
