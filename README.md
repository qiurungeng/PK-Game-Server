# 蔡徐坤大战吴亦凡小游戏-服务端   

东北大学软件学院《Linux操作系统》大作业——Linux C编程实现网络游戏服务器   
C语言大一入学前学过一点，目前也忘得挺干净了，让我一上来就直接编个服务器属实不易。本次作业事先用Java实现了客户端+服务端，理清了思路，再根据Java服务端改造成Linux C服务端。中途踩坑不少，所幸作业能按时完成提交。做游戏确实挺有意思，有空的话将会完善升级本项目。    

客户端项目地址:https://github.com/qiurungeng/PK-Game-Client      

#### 服务端介绍：

1. TCP建立连接，UDP传输游戏数据(现代网络游戏为保障低延迟一般不采用TCP收发游戏数据)。
2. 双线程，分别处理普通游戏数据和重要游戏数据。
3. 每个线程中Select()函数实现多路IO复用，负责多个客户端的数据处理与读写。、
4. 一旦通过套接字描述符接收到客户端发来数据，调用线程对应的逻辑处理函数执行服务
5. 普通游戏数据只做转发，重要游戏数据解析并进行后续操作。

#### 客户端服务端通信介绍

游戏开始时会向服务端建立TCP连接，客户端向服务端发送客户端的两个UDP端口号，分别负责普通消息通信和重要消息通信（攻击其他玩家，是否被击中，死亡）。服务端接收后向客户端分配指定客户端ID并发送回客户端。

考虑到多人联机游戏，若所有消息包服务端都拆开解析处理的话，服务端将不堪重负。服务端只负责处理和维护重要游戏数据(客户端英雄的生命值等)。   
服务端有两个UDP端口分别于客户端的普通消息端口、重要消息端口对应，建立连接(connect)通信。这两个连接的描述符分别为:fd_normal, fd_vital。   
对于fd_normal发来的消息视作普通消息，只做转发处理，不做解析。   
对于fd_vital发来的消息视作重要消息，做解析处理，并根据解析得的数据通过游戏逻辑进行后续操作。

客户端服务端通信的所有消息类型如下：

| 消息类型        | 描述                                                         |
| ---------------- | ------------------------------------------------------------ |
| 新用户创建消息| 普通类型消息：客户端连接成功时向服务端发送该消息，以通知其他联机玩家，在他们的客户端中增添本机英雄信息。 |
| 已存在用户消息| 普通类型消息：收到[ **新用户创建消息** ]后发送该消息给服务器，告诉刚才发送新用户创建消息的用户其存在，在它的客户端中怎天本机英雄信息。 |
| 英雄移动消息| 普通类型消息：客户端监听到键盘按下时发送，通知其他客户端本英雄移动及攻击动作状态。 |
| 英雄发动攻击消息| 重要类型消息：当执行攻击动作线程执行完毕时，向服务端发送此类消息，由服务端处理攻击的命中与否 |
| 英雄被攻击消息| 重要类型消息：服务端收到[ **英雄发动攻击消息** ]时，向除该消息发送方以外的其他客户端发送该消息。客户端收到该消息后，追加本机英雄位置信息，转发回客户端。 |
| 英雄受伤消息| 重要类型消息：服务端收到客户端反馈的[ **英雄被攻击消息** ]时，由攻击发动方及被攻击方的位置信息计算，判断被攻击方是否被击中。若被击中，向所有客户端发送该英雄受伤消息，并为服务端中维护的该客户端信息结构体进行扣血操作。客户端接收该消息后对受伤英雄进行扣血操作，同时伴有僵直效果。 |
| 英雄死亡消息| 重要类型消息：服务端检测到其维护的某一客户端信息中的血量小于等于0，向所有客户端发送该消息。客户端接收到该消息后，将对应英雄状态更新为已死。 |
| 英雄获胜消息| 重要类型消息：服务端检测到其维护的客户端中只剩一个未死，向所有客户端发送该消息。客户端接收到该消息后，显示对应英雄获胜通知。 |