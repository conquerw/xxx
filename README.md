简介
通过抽象 port gateway event, 实现基于事件驱动的中间件, port 是通信的基础, 通常情况下它不会单独使用, 都是嵌入到 gateway event 里面，实现一对一和广播通信，已经适配了 mosquitto、 iceoryx2 和 nng
0-9999是给port使用
10000-65535是给广播使用

编译
./script/build.sh
./script/delete.sh

例子


测试
参考 Fast-DDS 的性能测试
https://www.eprosima.com/developer-resources/performance
https://github.com/eProsima/Fast-DDS/tree/master/test/performance
