# eFootball 国际服手游：联网同步（netcode）、设备性能与降低“系统局”体感的实用手册

> 范围：只覆盖 Konami 国际服 eFootball 手游（iOS/Android）。没有调查网易国服。主机/PC 资料只在机制可能共用时引用，并标为【主机/PC】。
> 标注格式：【证据等级｜适用范围】。A＝Konami 官方；B＝可复现测试/抓包/数据挖掘；C＝专利/论文/他作官方文档（只证明某技术存在）；D＝广泛流传的玩家经验；E＝单一说法/推测。适用范围：手游确认／跨平台（手游适用）／主机/PC／未知。
> 资料截至 2026-11-06。本会话的 WebFetch 打不开 konami.com 和 operationsports.com（域名不在会话网络白名单内）。这两个站的内容来自搜索引擎摘要，没有逐字核对原文。要核对原文，需在会话网络设置里放行这两个域名。
> “约 X 年 X 月”的日期按搜索引擎显示的页面年龄推算，只是近似值。

---

## 一、eFootball 的联网同步是怎么做的（P2P、中继、C/S、输入延迟、是否回滚）？有没有手游输入延迟的实测？

### Takeaway
官方只确认了两种对战方式。排位（eFootball League/Division）走“客户端-服务器（C/S）”，由服务器管理连接和比赛进程。快速比赛和 1v1 好友赛走 P2P。官方没有公开输入延迟帧数，也没有说明是否用回滚（rollback）。抓包看到客户端有 62 个区域中继、天线阈值、“慢动作/掉帧判定”阈值和输入指令缓冲配置。网上找不到可靠的手游在线/离线输入延迟实测数据。

### Cited Findings
**官方对战方式（A）**
- 【A｜跨平台（手游适用）】Konami 官方页面把对战模式分成两类。P2P 包括快速比赛（Quick Match）、好友赛（仅 1v1 房间）和点球大战类挑战活动。C/S 包括 eFootball League、挑战活动、排名活动、主题活动、合作好友房和 Strike Arena。原话大意是 “Modes in which match results are particularly important use the client-server method in order to ensure a stable connection and prevent cheating”（结果重要的模式用 C/S，以确保连接稳定并防作弊），服务器负责管理连接和比赛进程。页面约 2025-04（搜索引擎显示约 569 天前），没找到更新版本，映射关系可能已变。— [Konami: Online PvP Setup for eFootball™](https://www.konami.com/efootball/en/page/online_match)
- 【A｜跨平台】官方说游戏服务器设在 Konami 合作的全球公司管理的各城市数据中心里。离数据中心近，一般连接更好，但距离不是唯一因素。中文版页面补充说，系统会结合对手位置和网络基础设施选最佳服务器，有时更远的服务器反而更好。— [Konami 英文页](https://www.konami.com/efootball/en/page/online_match)；[Konami 简中页](https://www.konami.com/efootball/zh-cn/page/online_match)
- 【A｜跨平台】天线图标只在 C/S 模式里显示，实时反映“你到游戏服务器”的连接质量。双方看到的天线等级可以不同。赛后有网络图表，纵轴是连接质量变化，越高越稳定。— [Konami: online_match#antenna_icon](https://www.konami.com/efootball/en/page/online_match)
- 【A/媒体转述｜跨平台】Gamereactor 报道 v4.4.0 更新时说，天线显示在每位玩家名字旁边，赛后有延迟图表。文章日期不明确，v4.4.0 推测在 2025 年春季。— [Gamereactor: Konami unveils major update 4.4.0](https://www.gamereactor.eu/konami-unveils-major-update-440-for-efootball-1530173/)
- 【A（经搜索摘要）｜跨平台】Konami 公告页曾承认比赛中存在 “command inputs could slow down” 以及球和球员动作掉帧（“ball and player movement could drop”）的问题，并说会继续改进。页面约 2024-09，未能直接打开核对原文。— [Konami: Announcements](https://www.konami.com/efootball/en/page/updates)
- 【A（经媒体转述）｜跨平台】Operation Sports 根据 Konami 的 X 帖子报道（约 2025-12）：Konami 正在全球增设服务器站点，并称已降低输入延迟（reduced input delays）。玩家打几场后可能收到关于连接质量和操作响应的简短问卷。— [Operation Sports](https://www.operationsports.com/efootball-outlines-new-efforts-to-improve-online-match-quality/)
- 【A（经媒体转述）｜跨平台】eFootball Connect 访谈（约 2026-02）中，开发方说会用赛后问卷和数据审查、更新服务器，提到新增泰国区域服务器，计划扩展更多地区。— [Gamereactor](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)
- 【A（经搜索摘要）｜跨平台】搜索摘要提到一则 2026-10-23 的 Konami 公告，承认土耳其、希腊和非洲地区的比赛连接比平时不稳定，并称已修复。具体页面 URL 未能确认，可能在 [Konami 公告页](https://www.konami.com/efootball/en/page/updates) 或 [topic/news/6043](https://www.konami.com/efootball/en-us/topic/news/6043)。

**抓包/配置（B，手游确认，iOS 11.1.1 国际服，配置时间戳 2026-09-14）**
- 【B｜手游确认】`GetServerEnv.json` 里有两组天线阈值：P2P 为 `antenna_5_limit 30 / 4: 120 / 3: 171 / 2: 209`，C/S 为 `communication_antenna_status_info_cs: 15 / 50 / 110 / 200`。同一文件里还有 `"slowdown_judgment_threshold_list":[51,25,17]` 和 `ip_country_code`（该抓包为 "TUR"），说明服务器按 IP 判定国家。— 本地文件 `/home/user/pes2027-mod/ziru-extra/responses/GetServerEnv.json`
- 【B｜手游确认】`gamerelay_qualitycheck_list.json`（`CMD_GET_GAMERELAY_QUALITYCHECK_LIST`）列出 62 个 game-relay 区域，`quality_threshold_1: 75`、`quality_threshold_2: 175`，每个区域有 `quality`/`last_update` 字段（抓包时全为 0）。区域里没有中国大陆节点。离中国最近的是 GCP 风格命名的 `asia-east2`、`asia-east1`、`asia-northeast1/2/3`、`asia-southeast1/2/3`，以及 AWS 风格的 `ap-southeast-5`、`ap-southeast-7`。— 本地文件 `/home/user/pes2027-mod/ziru-extra/responses/gamerelay_qualitycheck_list.json`
- 【E｜—】上述区域代码对应的城市，按 Google Cloud/AWS 公开的区域命名推断（训练知识，本会话没有抓取核实）：asia-east2＝香港，asia-east1＝台湾，asia-northeast1/2/3＝东京/大阪/首尔，asia-southeast1＝新加坡，asia-southeast2＝雅加达，asia-southeast3＝曼谷，ap-southeast-7＝泰国，ap-southeast-5＝马来西亚。曼谷/泰国节点的存在，与访谈里“新增泰国服务器”的说法吻合。— [Google Cloud regions](https://cloud.google.com/compute/docs/regions-zones)；[AWS regions](https://docs.aws.amazon.com/general/latest/gr/rande.html)
- 【B｜手游确认】按本会话早前的挖掘结论，配置里有 `MatchCommandBufferingControl`（输入指令缓冲控制）段，并区分 P2P 和 CS 两种对战模式。这次在现存的 `GetServerEnv.json`/`gamerelay_qualitycheck_list.json` 里没有 grep 到这个键名，可能在别的抓包文件里，需要复核。— 任务背景；姊妹笔记 `/home/user/pes2027-mod/research_notes/实况足球系统局机制与应对/official_mechanics.md`

**实测与他作参考**
- 【E｜主机/PC】Steam 玩家报告：游戏内 ping 只有约 20ms，P2P 比赛里仍有严重输入延迟，怀疑是 ping 数字看不到的抖动或丢包。— [Steam: Severe input delay in P2P matches despite low ping](https://steamcommunity.com/app/1665460/discussions/0/804595075203018750/)（日期未知）
- 【C｜他作，仅证明技术存在】另一款足球联机游戏 REMATCH 的官方补丁说明，提供了可选的固定输入延迟档位：33ms（优先响应）、50ms（默认）、67ms（优先稳定）。这说明“用固定输入延迟换取同步稳定”是足球类联机游戏的常见做法，但不代表 eFootball 也这样做。— [REMATCH Netcode 2.0 Beta Patch Notes](https://playrematch.com/post/netcode-2-0-beta-patch-notes-update-1)

### Inferences
- 排位 Division 走 C/S。所以“对手网差直接拖慢我”的典型 P2P 现象，在排位里应该比快速比赛少。但每位玩家到服务器/中继的路径是独立的，**谁离中继更远、抖动更大，谁的指令就更晚到、画面更晚更新**。这是推断（E），基于官方确认的 C/S 结构和双方天线可以不同（A）。
- `MatchCommandBufferingControl` 加上“输入指令变慢”的官方承认，说明存在一种**可动态调整的输入缓冲**。联网游戏的通用做法是：网络抖动时加大缓冲，以免卡顿或瞬移。体感上就是“按键到动作慢半拍、接球后调整慢、传球出脚慢”。在手游上具体怎么实现、缓冲多少帧，没有公开资料（B 只证明存在，机制 E）。
- `slowdown_judgment_threshold_list [51,25,17]` 有两种可能。一是帧时间阈值（ms）：51≈20fps，25≈40fps，17≈59fps。二是 fps 阈值。“slowdown judgment”的字面意思接近日文“処理落ち判定”。合理推测是客户端会判定本机或比赛是否“处理落后/掉帧”，并据此处理或上报（E）。
- 没找到 eFootball 使用回滚（rollback）的任何证据。“对手瞬移、球突然出现在对手脚下”等现象，在纯缓冲/锁步和回滚下都可能出现，没法从体感判断（E）。

### Gaps
- 官方没有公开输入延迟帧数、指令缓冲策略、是否回滚，也没说 C/S 是“服务器权威模拟”还是“服务器中继+锁步”。这一点决定了一方网差时，是双方一起卡，还是只有网差方吃亏。
- 没找到第三方用高速摄像对 eFootball 手游做的“离线 vs 在线、不同 ping”输入延迟测量（B 级空白）。
- 天线阈值（30/120/171/209 与 15/50/110/200）和中继质量阈值（75/175）的单位（ms？分数？）没有官方说明。
- konami.com 原文没能直接打开，以上 A 级条目是经搜索摘要转述的。

---

## 二、双方延迟不对称时，有没有一方系统性占优？“粘滞/沉重”在技术上对应什么？

### Takeaway
官方确认双方连接质量可以不同。在 C/S 架构下，延迟更高、抖动更大的一方，操作生效更晚，看到的画面也更旧。表现就是“总是慢一步、对手总先到球”。这能解释“粘滞/沉重”，而且不需要任何隐藏的能力值调整。不过这是基于通用 netcode 原理的推断，没有 eFootball 专门的实测。另外，体力、状态、对手实力提升也会造成类似体感。

### Cited Findings
- 【A｜跨平台】官方：天线反映各自到服务器的连接，对手屏幕上的天线等级可能和你不同。— [Konami online_match](https://www.konami.com/efootball/en/page/online_match)
- 【A（经搜索摘要）｜跨平台】Konami 承认过“指令输入变慢”和“球/球员动作掉帧”这类问题。也就是说，“操作迟滞”被官方当作技术质量问题处理，不是设计出来的平衡机制。— [Konami Announcements（约 2024-09）](https://www.konami.com/efootball/en/page/updates)
- 【A（经媒体转述）｜跨平台】Konami 在 eFootball Connect 访谈中明确否认存在 “scripting”，也否认有任何偏袒某一方的刻意操控（约 2026-02）。注意：这是官方否认，不能当作独立证明。— [Gamereactor](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)；[Recapio 访谈摘要](https://recapio.com/digest/efootballtm-connect-spotlight-interview-global-by-efootball)（日期不详）
- 【E｜跨平台综述】一篇 2026 年的“eFootball 是否有脚本”调查文章说，Reddit/Steam 上大量玩家描述领先后球队变沉、防守球员远离球、门将神扑等现象，并认为网络延迟“can impersonate manipulation extremely well”（能非常逼真地伪装成操控）。文章还列了“什么证据才能证明脚本”的标准（本会话没读到全文）。— [zaplat: Is eFootball Scripted? 2026 Evidence Investigation](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)
- 【E｜他作（EA FC 27）】EA FC 27 的解说文章认为，服务器距离或糟糕路由造成的输入延迟会让球员“不响应”。体力随比赛下降，经常逼抢、冲刺的球员掉得更快，这也会造成后段“变沉”。只可作为跨游戏的通用参考。— [Elocarry: EA FC 27 Scripting Explained](https://elocarry.net/blog/ea-fc-27/scripting-and-momentum-explained/)
- 【D｜主机/PC】Steam 讨论区多名玩家描述 PvP 中手柄输入迟滞，有人说进球后出现 0.5–1 秒延迟，“against CPU PVE there are no lag nor delay, only in PVP matches”（打电脑没有延迟，只有 PvP 有）。也有人指责对手用“lag switch”作弊（未证实）。由搜索摘要汇总，没逐帖核对是哪一帖。— [Steam 讨论 1](https://steamcommunity.com/app/1665460/discussions/0/3824158244542838410)；[Steam 讨论 2](https://steamcommunity.com/app/1665460/discussions/0/601890191585598538)
- 【E｜跨平台】第三方测速博客说，即使 ping 低，明显丢包也会导致回弹（rubberbanding）、球员瞬移和指令不响应。— [pingtesti: ping test efootball](https://www.pingtesti.com/en/blog/ping-test-efootball/)
- 【B｜手游确认（仅 vs AI 抓包）】比赛开始时服务器会下发每位球员的能力数组（`parameters`），抓包中没有名为 handicap/streak/adjustment 的字段。赛后结果里有 `cont_win_num`（连胜数），还有 `rating_boost_type`（抓到为 NONE）和 `rating_boost_rate`（抓到为 1）。没有抓到排位 PvP 的数据，所以既不能证明，也不能排除 PvP 中有调整。— 任务背景；[姊妹笔记 official_mechanics.md](/home/user/pes2027-mod/research_notes/实况足球系统局机制与应对/official_mechanics.md)

### Inferences
- **“粘滞/沉重”的技术对应（E，基于通用 netcode 原理）**：①输入缓冲加大：按键到动作的时间变长。②画面基于更旧的状态：你看到的对手位置比服务器真实位置落后半拍，所以“对手总先到球”“抢断出脚比数值快”。③丢包或抖动触发补偿：表现为一卡一顿、瞬移、球“粘”在对手脚下。④本机掉帧：进攻三区人多、动画多，更容易掉帧，表现为“只在关键区域卡”。
- **为什么偏偏连胜后更明显（E，可检验的非脚本解释）**：①Konami 已把匹配改成“同水平玩家更常配对”（见第四节），连胜后积分上升，对手真实水平更高，直塞和回追更精准本来就正常。②积分越高，同段在线玩家越少，匹配要等更久或扩大范围，越可能配到地理上更远的对手和中继，延迟和抖动随之变差。Konami 自己就警告过，收窄匹配条件“may be made with an opponent with an unstable online connection”（可能配到连接不稳定的对手），见第四节，A。③回归均值和选择性记忆：连胜本身是偏高的运气样本，之后的失利更容易被记成“系统局”。
- 这些解释能覆盖用户描述的大部分症状，但不等于证明“系统局”不存在。目前没有能证明排位 PvP 中存在按连胜调整能力的公开证据，也没有能完全排除它的排位抓包。

### Gaps
- 没有排位 PvP 的抓包，不能直接验证双方下发的 `parameters` 是否与卡面一致。
- 没找到 eFootball 在“一方高延迟、一方低延迟”时谁更占优的对照实验。
- 不知道 C/S 模式下服务器是否会为了公平给低延迟方额外加缓冲（有些游戏会做“延迟均衡”）。

---

## 三、手机端：帧率、发热降频、后台、Wi-Fi 与 5G/4G、加速器、服务器区域怎么影响手感？玩家和测试者怎么建议？

### Takeaway
官方只给出“靠近 Wi-Fi 接入点”“离数据中心近更好”这类原则。帧率、发热、加速器方面全是第三方和厂商的说法，质量参差，延迟数字大多来自推广文章。共识是：网络（路由、抖动、丢包）是在线延迟的主要来源。帧率和发热决定画面是否卡顿，也会放大手感迟滞。中国玩家走加速器时，节点和出口地区会影响匹配区域和中继路径。

### Cited Findings
**网络与区域**
- 【A（经搜索摘要）｜手游】Konami 对手机的建议是尽量靠近 Wi-Fi 接入点。— [Konami online_match](https://www.konami.com/efootball/en/page/online_match)
- 【B｜手游确认】服务器会记录客户端的 IP 国家（抓包里的 `ip_country_code`）。客户端会拿到 62 个中继区域的质量检测清单，没有中国大陆节点。— 本地 `GetServerEnv.json`、`gamerelay_qualitycheck_list.json`
- 【E｜手游，厂商自测】某路由加速厂商自称：同一场测试比赛，默认路由 ping 120–217ms，换到更优路由后降到 28–32ms。该文还认为 eFootball 的延迟主要在连接上，画质设置只影响画面卡顿。— [lagx.app: eFootball Mobile Lag Fix](https://lagx.app/blog/efootball-mobile-fix-lag/)（厂商推广）
- 【E｜中文厂商/媒体】国内加速器文章普遍把国际服卡顿归因于服务器在海外、路由距离远、丢包和抖动。部分加速器的海外线路优化不足，延迟偏高。有“实测”称加速后晚高峰的延迟和丢包下降，但数据来自推广方，没有第三方验证。— [网易 UU 加速器博客](https://uu.163.com/blog/e-football-lagging-net.html)；[掘金：eFootball 手游加速器实测](https://juejin.cn/post/7687493428758200371)；[掘金：加速入门攻略](https://juejin.cn/post/7686562995334561802)；[腾讯新闻（2026-03-31）](https://news.qq.com/rain/a/20260331A062JC00)
- 【E｜中文媒体】有指南建议：联机卡顿时可以用加速器或调整匹配区域。— [武汉晚报（2026-07-17）](https://whwb.cjn.cn/html/2026-07/17/content_152970_10318.htm)
- 【E｜通用】有文章专门讨论“为什么用了加速器延迟反而更高”（本会话只看到标题）。— [加速器排行榜](https://www.kayuwang.com/jsqzx/1572.html)
- 【E｜通用，非 eFootball】通用节点对比称：香港节点延迟最低（约 15–30ms，另一来源称华南出发 5–20ms），日本和新加坡约 35–90ms，游戏首选香港、备选日本。数字互相矛盾，来源是代理节点站，只能作数量级参考。— [jichangmianfei](https://jichangmianfei.my/free-node/regions)；[勇哥博客](https://yongjichang.com/wiki-node-types)
- 【E｜他作】加速器厂商对 EA FC 26 的建议是：中国玩家优先选日本服务器（约 50–70ms），新加坡备选。这是另一款游戏，不能直接套用。— [NoPing](https://noping.com/cn/blog/valorant-ea-sports)

**帧率、发热、后台**
- 【E｜手游】有指南说画质设置里有帧率选项（20/30/60 FPS），高帧率只在支持的机型上出现。另有说法称某版本加入了 120FPS（仅限 120Hz 屏的安卓机，需要手动开启）。没有官方文档，选项以游戏内实际为准。— [Facebook fb-answers: eFootball 120 FPS Android](https://www.facebook.com/fb-answers/efootball-120-fps-android/)；[androidayuda 画质指南](https://en.androidayuda.com/games/Tutorials/eFootball-graphics-settings-guide-for-Android:-forget-about-lag-and-stuttering/)；[draggo 60FPS 指南](https://draggo-gg.web.app/guides/efootball-mobile-controller-60fps-guide/)
- 【D/E｜手游】玩家普遍反映高帧率更跟手，但更耗电、发热更大。有指南描述手机连续玩约 20 分钟后芯片降频、比赛中途掉帧。有网站给出“42°C 触发、降频 30–50%”这类具体数字，可信度低，不宜引用。由搜索摘要汇总，具体出处在上面三篇里，没逐一核实。— 同上；[eslamgaming 4GB RAM 设置（2026-09）](https://www.eslamgaming.com/2026/09/efootball-2026-lag-fix-for-4gb-ram-best-settings-for-smooth-gameplay.html)
- 【E｜手游】中文攻略建议开局前关闭所有非必要后台应用，适度调低分辨率、关闭部分高级特效，以减轻 GPU 压力。— [搜狐：国际服闪退与下载全攻略](https://www.sohu.com/a/1007396982_121957097)
- 【E｜风险提示】有网站和 B 站视频提供“帧率解锁”“60/90/120FPS 解锁包”。相关说法提到 root 设备上的插帧工具在竞技模式有封号风险。不建议使用。— [efootfix graphics pack](https://efootfix.top/efootball-graphics-pack/)；[B 站：帧率解锁教程](https://www.bilibili.com/video/BV1a2cMeqEP9/)
- 【E｜跨平台】标题为 “How I Fixed Slow Motion After v5.5.0 Update” 的 2026-09 文章说明，v5.5.0 后有玩家遇到“慢动作”问题（只看到标题，内容未读）。— [eslamgaming（2026-09）](https://www.eslamgaming.com/2026/09/efootball-2026-connection-lost-fix.html)

### Inferences
- **中国玩家走加速器打国际服的路径（E）**：手机 → 国内运营商 → 加速器入口 → 境外出口（例如香港/日本） → Konami 按对手位置选中继（如 asia-east2 香港、asia-northeast1 东京） → 服务器。服务器按出口 IP 判断国家（B），这可能影响你被放进哪个地区的匹配池（推断）。如果对手是本地玩家（到中继几毫秒），你却有 60–100ms 加上晚高峰抖动，C/S 下你就是稳定吃亏的一方。这和“接球慢半拍、对手总先到球”的症状高度吻合。
- **节点选择原则（E，需要自测）**：选到出口延迟低、**抖动和丢包低**的节点，比单看 ping 重要。离可用中继近的出口（香港、台湾、东京、新加坡）理论上更好。固定使用同一个节点，方便对比记录。跨洲节点（欧美）会让你进入远距离对手池，应避免。
- 帧率和发热主要影响“画面卡顿”和本机处理延迟。网络不变时，60fps 比 30fps 每帧少约 16ms 的显示和处理间隔，这是通用原理，不是 eFootball 实测。手机降频后掉帧，正好会落在比赛后段和进攻三区这类高负载时刻，容易和“系统局”混为一谈（E）。

### Gaps
- 没找到 Wi-Fi 与 5G/4G 在 eFootball 手游上的对比测试。
- 没找到独立机构对各加速器在 eFootball 国际服上的延迟和抖动实测，现有数字全来自厂商。
- 没找到官方说明“出口 IP 国家”如何影响匹配池（只有抓包字段存在 B，加上官方“按对手位置选服务器”A）。
- iOS 低电量模式、安卓省电模式对 eFootball 帧率的影响，没找到 eFootball 专门的资料。

---

## 四、哪些游戏内设置有帮助（连接优先匹配、操作设置、传球辅助、视角）？针对直塞、高位逼抢、回追铲断有哪些战术对策？

### Takeaway
官方确认的匹配选项，只有 Div 1–3 的“智能辅助（Smart Assist）分类匹配”测试。Konami 明说选“和我设置相同”可能配到连接不稳定的对手。抓包里还有 `ping_level = PING_LEVEL_BALANCE`、`team_power_limit`、`control_style_filtering` 三个匹配偏好字段，但没找到公开资料说明它们在 UI 中是否可调。战术对策来自 2026 赛季前后的攻略，属于 D/E 级：防直塞靠压缩阵型、卡传球线路、不乱扑；破高压靠一脚出球、提前传和长传越过逼抢线。

### Cited Findings
**匹配设置**
- 【A｜跨平台（手游适用）】Konami 2026-05-19 在 X 上宣布：在 5 月底到 6 月初的更新中，eFootball League Div 1–3 测试“按 Smart Assist 分类匹配”。选项有 “Same setting as mine” 和 “Matchmake freely”。选前者时 “It takes time to match with an opponent ・ A match may be made with an opponent with an unstable online connection”（匹配更久，可能配到连接不稳定的对手）。日期按帖子 ID 解码。— [eFootball 官方 X](https://x.com/play_eFootball/status/2056702522933514339)
- 【A｜跨平台】v5.5.0 版本说明写道 “Changed the matchmaking system so that users around the same level of ability will match together more frequently”（让能力相近的玩家更常配对），还修改了 eFootball League 的积分计算公式和匹配逻辑。Smart Assist 匹配被标为测试功能。— [Konami v5.5.0 Information](https://www.konami.com/efootball/en/page/v5/versioninfo_v5-50)
- 【B｜手游确认】登录响应里存有匹配偏好：`"ping_level": "PING_LEVEL_BALANCE", "team_power_limit": "TEAM_POWER_NO_LIMIT", "control_style_filtering": "NO"`。— 本地 `/home/user/pes2027-mod/ziru-extra/responses/CmdLogin.json`
- 【E｜跨平台】用户问答站 2025-09 的回答称：匹配基于积分和球队实力，等久了会扩大范围，没有手动范围设置，降低阵容总评可能遇到更弱的对手。非官方，可靠性低。— [sikayetvar/Xolvie 问答](https://www.sikayetvar.com/en/efootball-us/in-efootball-matchmaking-is-weak-i-keep-getting-matched-with-players-that-are-too-strong-how-does-the-matchmaking-system-work-and-how-can-i-influence-it-q-25598)

**战术对策（D/E，跨平台，手游适用）**
- 【D/E｜跨平台】防直塞：要点是切断传球角度，而不是追身后的跑位者。用手控的中场站在持球人和前锋之间的线路上，收紧阵型，宁可放对方往边路传。持球人只施加轻度压迫，用“呼叫第二防守人”封堵传球线，追着球扑是丢球的主因。— [gamemarket.gg: eFootball Defending Guide](https://gamemarket.gg/news/efootball/efootball-defending-guide-pressure-match-up-and-tackling)
- 【E｜跨平台】操作说明：直塞键有地面和高空两种，另有“Call 2nd Defender”键。— [qnabangla: eFootball Controls Guide 2026](https://en.qnabangla.com/efootball-controls-guide/)
- 【D/E｜跨平台】阵型：多份 2026 攻略认为紧凑的 Long Ball Counter 更容易防直塞。Quick Counter 压得更高，身后空间更大。— [skycoach: Best eFootball Formations 2026](https://skycoach.gg/blog/efootball/articles/best-meta-formations)；[playhub: Best eFootball Formations 2026](https://playhub.com/blog/efootball/best-formations-470122)
- 【E｜跨平台】高位逼抢的弱点：一脚越过逼抢线的直接长传会迅速拉散阵型，边路球员的体力很关键。— [revibyte: Best Quick Counter Formation 2026](https://www.revibyte.blog/efootball/best-quick-counter-formation-efootball-2026/)
- 【E｜跨平台】当季（标注 eFootball 2027）的防守型打法风格指南，可用于给后卫和后腰选合适的 playing style（只看到标题和摘要）。— [gamingonphone: eFootball 2027 Defensive Playing Styles guide](https://gamingonphone.com/guides/efootball-defensive-playing-styles-guide/)

### Inferences
- **匹配建议（A+E）**：如果你在 Div 1–3，并且能看到 Smart Assist 分类匹配，选 “Matchmake freely” 更可能快速配到连接正常的对手（依据是官方对另一选项的风险提示）。`PING_LEVEL_BALANCE` 这个命名暗示可能有一个“连接优先”的档位。如果游戏设置里出现连接或延迟相关的匹配选项，优先选连接优先，但目前 UI 是否开放未经确认。
- **针对症状的战术调整（E，按延迟环境推演）**：
  - *接球慢半拍、调整慢*：少盘带、少花式，提前做传球决定，尽量一脚出球。接球前先看好下一脚传给谁，抵消固定的输入迟滞。
  - *传球慢、被拦*：少在密集区打短传渗透，多用转移、边路传中和高空直塞，减少对精确时机的依赖。
  - *对手一脚直塞打穿*：防线别压太高，后卫不要主动上抢，用中场手控卡线路，必要时用门将出击。延迟高时更该“站位防守”，少依赖反应式抢断。
  - *对手极限回追、铲断*：别在对方后卫附近长距离盘带，提前分球，用身体护球，或者回传后重组。
  - *体力*：高位逼抢消耗大，比赛后段“变沉”部分来自体力。60–70 分钟左右换下逼抢最多的边路和中场。
- 这些调整对“真延迟”和“假如真有系统局”同样有效，因为它们都在降低你对精确时机和身体对抗的依赖。

### Gaps
- 没找到当前版本手游的传球辅助、操作类型（经典/智能辅助）、镜头设置与延迟手感的对照测试。
- 没能确认 `ping_level`、`team_power_limit`、`control_style_filtering` 是否在手游 UI 中开放，以及有哪些可选值。
- 当前赛季（2027 赛季）具体的主流战术和打法风格细节，本会话没读到全文。

---

## 五、实用习惯：连胜后停手、记录战绩、录屏查延迟——各有什么证据？

### Takeaway
没有哪条习惯有 A 级或 B 级证据证明能直接提高胜率。但“记录天线和赛后网络图表”“录屏测延迟”能把“系统局”的体感变成可检验的数据。“状态箭头全球统一”有抓包证据（B）。“连胜后停手”主要依据是回归均值、对手变强和身心疲劳，没有证据表明它能躲开某种触发机制。

### Cited Findings
- 【A｜跨平台】每场 C/S 比赛后有网络图表，比赛中有实时天线，可以拿来逐场记录连接质量。— [Konami online_match#antenna_icon](https://www.konami.com/efootball/en/page/online_match)
- 【A（经媒体转述）｜跨平台】Konami 会在赛后弹出连接质量和响应性问卷，并按各地的比赛量和连接质量扩建服务器。认真填写，等于向官方反馈本地区的连接问题。— [Operation Sports（约 2025-12）](https://www.operationsports.com/efootball-outlines-new-efforts-to-improve-online-match-quality/)；[Gamereactor（约 2026-02）](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)
- 【B｜手游确认】每周状态表 `PlayerWeekly.bin`（17,073 名球员，取值 0–4）从同一个全球 CDN 地址下载，地址里没有用户或积分成分。所以同一周、同一张卡的状态对所有人都一样，不会因为你连胜而单独给你差状态。— 任务背景（本会话早前挖掘）
- 【B｜手游确认】服务器跟踪连胜数（`cont_win_num`），排行榜有 WIN_STREAK 30/50 的称号类型。这说明连胜被统计并用于称号，但不能证明它被用来调整比赛。— 任务背景（本会话早前挖掘）
- 【D｜主机/PC】玩家报告：只在 PvP 有延迟，打电脑没有（见第二节 Steam 汇总）。这一对比可以作为自测方法：同一台机器、同一网络下，对比打 AI 和打真人的手感。— [Steam 讨论](https://steamcommunity.com/app/1665460/discussions/0/3824158244542838410)

### Inferences —— 分级应对手册（适用国际服手游）
**网络（优先级最高）**
1. 【A+D】玩排位时用 5GHz Wi-Fi，靠近路由器，同一网络里避免大流量下载和视频。如果 Wi-Fi 不稳，再试 5G，并且整场不要切换网络。（Konami 建议靠近 AP，A；Wi-Fi 和移动网络哪个好没有对比测试，需要自测。）
2. 【E，需自测】加速器用“游戏模式/UDP 加速”，选出口在**香港、台湾、东京或新加坡**、抖动和丢包最低的节点，**固定同一节点**，避免跨洲节点。晚高峰（约北京时间 20–23 点，跨境链路拥塞，这是厂商文章的普遍说法）尽量少打排位，或者先测一下丢包。
3. 【A】每场看天线：开场天线就只有 1–2 格，或者赛后图表波动大，这场的体感问题首先归因于网络。连续几场都差，就停打或换节点。

**设备**
4. 【D/E】帧率选本机能**全场稳定**的最高档。手机会烫，就宁可 60fps 配低画质，也不要 120fps 中途降频。不要边充电边打，摘掉厚手机壳。打开勿扰，关闭后台应用和省电模式（省电模式会限频，属通用常识，没有 eFootball 专门资料）。
5. 【E，风险】不要用第三方帧率解锁或插帧工具（有封号风险）。

**匹配与阵容**
6. 【A，限 Div 1–3 测试功能】有 Smart Assist 分类匹配时，选 “Matchmake freely”，减少配到不稳定连接对手的概率。
7. 【B】开赛前看状态箭头（全球统一），把状态差的主力换成状态好的替补。高位逼抢的球员 60–70 分钟换下（D）。

**战术（见第四节）**
8. 【D/E】防守压缩阵型、卡线路、少扑抢。进攻提前出球、多一脚传、多用转移和长传。延迟越高，越要减少依赖精确时机的操作。

**连胜、心态与记录**
9. 【E，有通用依据】连胜到一定场次后休息一下，或者连输两场就停。依据不是“躲开系统局触发”，而是：积分上升后对手确实更强（v5.5.0 匹配逻辑，A），疲劳和上头会让操作变形，晚高峰网络也会变差。
10. 【B，自建证据】用表格逐场记录：时间、网络（Wi-Fi/5G）、加速器节点、开场和赛后天线、赛后图表截图、对手所在地区（如果能看到）、比分、主观“粘滞”评分（1–5 分）。积累 30–50 场后看两件事：“粘滞局”是否集中在某些时段、节点或天线等级上；是否和连胜场次有关。如果粘滞局几乎都伴随低天线或高峰时段，就是网络问题。如果和网络完全无关、而且只出现在连胜之后，这才是值得进一步抓包的线索。
11. 【B，自测方法】录屏测延迟：用另一台手机的慢动作模式（240fps）同时拍下手指点按和屏幕画面，数“按下到球员出脚”的帧数。分别在打 AI（离线基准）和打真人、不同天线等级下测几次，就能量化在线输入延迟及其波动。这个方法是通用的输入延迟测量方法，没找到针对 eFootball 手游的公开实测结果。

### Gaps
- 没有任何研究或数据集量化“连胜后休息”对 eFootball 胜率的影响。
- 没找到公开的 eFootball 手游玩家逐场记录数据集，无法验证“系统局”和连胜、网络的相关性。
- 本会话只能用搜索摘要访问 konami.com，官方页面原文（包括在线对战页的手机网络建议原话）需要在网络设置中放行 www.konami.com 后再逐字核对。
