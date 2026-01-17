# 简介
- 通过抽象port、gateway和event，实现基于事件驱动的中间件，port是通信的基础，通常情况下它不会单独使用，都是嵌入到gateway或event里面，通常情况下一个进程要有一个gateway，从而可以和别的进程通信，业务逻辑都可以抽象成event，0-9999是给event使用，10000-65535是给event广播使用，最终实现一对一通信和广播通信，目前已经适配了mosquitto、iceoryx2和nng。

# 编译
- ./script/build.sh
- ./script/delete.sh

# 例子
- 参考example

# 测试
- 参考Fast-DDS的性能测试
- https://www.eprosima.com/developer-resources/performance
- https://github.com/eProsima/Fast-DDS/tree/master/test/performance
