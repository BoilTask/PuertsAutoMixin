用于支持类`Unlua`式的`Puerts`开发体验，允许通过给类添加`PuertsInterface`来绑定脚本（`mixin`）。

开发中，提供思路和基础实现，遇见奇奇怪怪的问题欢迎提`issue`。

比起常规的思路，核心是监听Object创建时才真正执行绑定，避免一启动就需要加载所有蓝图类。

> 可以在项目Config下创建TsTemplates文件夹，参考Plugins\PuertsAutoMixin\Config\TsTemplates编写模板文件，可以改变生成模板时的文件。

使用方式可以参考：[https://github.com/BoilTask/puerts-ue-template](https://github.com/BoilTask/puerts-ue-template)

参考项目：
- [Puerts](https://github.com/Tencent/Puerts)
- [UnLua](https://github.com/Tencent/UnLua)
