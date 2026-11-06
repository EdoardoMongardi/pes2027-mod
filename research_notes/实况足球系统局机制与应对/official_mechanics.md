# Konami 官方声明与已记录的 eFootball 国际服手游机制：哪些能产生或解释“系统局”体感

> 范围：只看 Konami 国际服 eFootball 手游（iOS/Android），截至 2026-11-06。按用户要求，网易国服不调查。主机/PC 资料只在机制大概率与手游相同时使用，并已标注。
> 证据等级：A=Konami 官方确认或文档；B=可复现测试、数据挖掘或逆向；C=专利或论文；D=广泛流传的玩家体验；E=单个案例或推测。
> 平台标签：【手游确认】【全平台共享】【主机/PC】【未知】。
> **方法局限（很重要）**：本会话的 WebFetch 访问不了 konami.com、gamereactor.eu、inkl.com、players.com.ua 等站点（DNS 被会话网络策略拦截）。所以凡是 konami.com 页面的内容，都来自搜索引擎的摘要或转述，不是我直接读到的原文。只有 X 帖子标题里完整出现的文字可以当原文引用。下文所有“官方原文”都按这个标准区分对待。

## 一、排位（eFootball League / Division）匹配官方怎么说？手游里有哪些匹配设置，它们和客户端的 PING_LEVEL_BALANCE / TEAM_POWER_NO_LIMIT / control_style_filtering 是什么关系？

### Takeaway
Konami 官方写明：League 按 Division 分组匹配；Division 1–3 用 Rating（积分）决定升降级；2026 年 5 月还修改了匹配系统，“让水平相近的玩家更容易匹配到一起”。也就是说，连胜→Rating/Division 升高→对手变强，这是写进官方规则的。官方另外在测试“按是否使用 Smart Assist 筛选对手”，这对应客户端的 control_style_filtering。官方明确提醒，选择“只匹配同设置”可能更容易匹配到网络不稳定的对手。PING_LEVEL 和 TEAM_POWER_LIMIT 两项，我没找到任何公开的官方说明。

### Cited Findings
- 【A｜手游确认（全平台公告）】2026-05-19，eFootball 官方 X 宣布将在“5 月下旬至 6 月初”的更新中测试按 Smart Assist 匹配，只针对 eFootball League 的 Division 1–3。原文（帖子标题完整出现）："A test implementation of a function that sorts match opponents for Division 1–3 in eFootball™ League based on whether or not they are using Smart Assist… ・Select “Same setting as mine” to match only with an opponent who has the same Smart Assist setting. ・Select “Matchmake freely” to match with any opponent. Please note that the following issues may occur when selecting “Same setting as mine”. ・It takes time to match with an opponent ・A match may be made with an opponent with an unstable online connection… While taking into account feedback received from users and data concerned with online connection quality, we will consider how this function will be implemented in the future."（大意：选“与我相同”会延长匹配时间，并可能匹配到连接不稳定的对手；官方会结合连接质量数据决定之后怎么做。）— [eFootball 官方 X，2026-05-19（由帖子 ID 换算）](https://x.com/play_eFootball/status/2056702522933514339)
- 【B｜手游确认】本仓库抓到的国际服 iOS 登录响应里，用户的匹配偏好字段为 `"ping_level": "PING_LEVEL_BALANCE", "team_power_limit": "TEAM_POWER_NO_LIMIT", "control_style_filtering": "NO"`。— [本地文件 /home/user/pes2027-mod/ziru-extra/responses/CmdLogin.json](/home/user/pes2027-mod/ziru-extra/responses/CmdLogin.json)
- 【B｜手游确认】服务器环境配置里有 `"enable_control_style_filtering_match_category":[]`（空列表，也就是抓包时没有哪个比赛类别启用这项筛选）。vs-COM 版 League 信息里有 `"is_enable_control_style_filtering":["NO",…]`，共 10 个元素，对应 10 个 Division，全部为 NO。由此看，这个筛选是服务器按比赛类别、按 Division 开关的，和官方“只在 Division 1–3 测试”的说法对得上。— [本地文件 GetServerEnv.json（配置内 date=1789384820，约 2026-09-14）](/home/user/pes2027-mod/ziru-extra/responses/GetServerEnv.json)；[本地文件 cmd_get_league_com_info.json](/home/user/pes2027-mod/ziru-extra/responses/cmd_get_league_com_info.json)
- 【A｜全平台共享】2026-05-28 官方 X 列出对 eFootball League 的调整："Revisions to the matchmaking system to make it easier for players of similar skill levels to be matched ・Adjustments to the rating calculation formula ・Abolition of relegation during Phases 1–3 ・Changes…"（标题截断，后文未取得）。— [eFootball 官方 X，2026-05-28](https://x.com/play_eFootball/status/2060020420867158381)
- 【A｜全平台共享】v5.5.0 版本说明称修改了 eFootball League 使用的 Rating 计算公式和匹配逻辑（来自搜索摘要，原文未读到）。— [Konami v5.5.0 Information（2026，具体日期未取得）](https://www.konami.com/efootball/en/page/v5/versioninfo_v5-50)
- 【A｜全平台共享，2024 年资料，部分可能已被 2026 年调整覆盖】v3.2.0 版本说明（据搜索摘要转述）："The eFootball™ League has been divided into 3 main groups: Division 1-3, Division 4-6 and Division 7-9. From now on, users will only be matched with other users from the same group." 每个 Phase 初期分组更宽；同组内优先匹配相邻 Division；"Originally, only users in Division 1 used the Rating system, but now users from Division 2 and Division 3 will also be playing for Ratings"；Division 1–3 的升降级按 Rating 决定；下一 Phase 的起始 Division 取“历史最高到达 Division”。— [Konami Version Info v3.2.0（2024 年）](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-20)；[Konami News 5296（日期未取得）](https://www.konami.com/efootball/en/topic/news/5296)
- 【A｜全平台共享】据搜索摘要，旧规则是到达 Division 3/6/9 后本 Phase 内不再降级，而 Division 1 有降级风险；2026-05 的调整改为 Division 1–3 在 Phase 中途不降级。两条来源时间不同，以 2026-05-28 的为准。— [eFootball 官方 X，2026-05-28](https://x.com/play_eFootball/status/2060020420867158381)；[Konami Version Info v3.2.0](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-20)
- 【D/E｜未知平台】社区问答（非官方）称匹配依据 Rating Points 和队伍实力，搜索时间过长会扩大匹配范围，玩家无法手动设置匹配范围。没有官方出处。— [Sikayetvar/Xolvie 问答（日期未取得）](https://www.sikayetvar.com/en/efootball-us/in-efootball-matchmaking-is-weak-i-keep-getting-matched-with-players-that-are-too-strong-how-does-the-matchmaking-system-work-and-how-can-i-influence-it-q-25598)
- 【D｜第三方转述】第三方文章称：在 Play Settings 关闭 Smart Assist，再在 League 比赛设置里选“Same setting as mine”，就能只匹配同样关闭辅助的对手。属于二手信息。— [Sportsdunia（2026）](https://www.sportsdunia.com/esports/efootball-smart-assist-settings)
- 【D｜第三方】有低权威站点称 Konami 从未说过匹配“只看队伍总实力（Collective/Team Strength）”。— [FIFPlay（2026-27 赛季页）](https://www.fifplay.com/efootball-2027-collective-strength/)

### Inferences
- control_style_filtering（客户端字段）≈ 官方的“按 Smart Assist 匹配”（Same setting as mine / Matchmake freely）。抓包里是 NO，等于“自由匹配”。这一对应是推断（B+A 交叉），官方没有公开字段名。
- PING_LEVEL_BALANCE 从命名看，是“连接质量 vs 匹配速度/水平”之间的平衡档，可能还有偏向连接优先的其他档位；TEAM_POWER_NO_LIMIT 看起来是“对手队伍实力上限：不限”。两者都没有官方说明（E）。官方 Smart Assist 公告承认“筛选条件越严→越可能匹配到连接不稳定的对手”，说明匹配系统确实在连接质量和其他条件之间做取舍（A 支持这个大方向，具体算法未知）。
- “让水平相近的玩家更容易匹配”加上按 Rating 升降级，意味着连胜后 Rating 上升，系统会主动给更强的对手。这是公开写明的、全服统一的机制，和“系统针对你”不是一回事。但从玩家视角看，结果就是“赢多了对手突然变强”。

### Gaps
- 没找到官方文档说明 PING_LEVEL 有哪些档位、手游 UI 里叫什么，也没找到 TEAM_POWER_LIMIT 出现在哪个模式（排位、好友赛还是活动）。
- 官方从未公开 Rating 计算公式、连胜时的加减分规则、匹配范围随等待时间扩大的参数。
- 是否把“队伍实力（Team Strength）”纳入 League 匹配，没有官方确认。
- 无法确认 Smart Assist 筛选测试 2026-06 之后是否转为正式功能（本次抓包显示服务器未启用任何类别）。

## 二、球员状态（Live Update 箭头）、队伍实力、主教练熟练度的官方文档；这些是全服统一还是按用户区分？

### Takeaway
状态箭头来自每周 Live Update，开赛前就确定、整场不变。本仓库数据挖掘显示，状态表从一个全球统一的 CDN 地址下载，不含用户或 Rating 成分，所以同一球员的箭头对所有人都一样。“球队风格熟练度改变球员能力”的机制，官方已在 v3.0.0（2023 年）取消。我没找到官方文档说明状态箭头具体加减多少能力值。

### Cited Findings
- 【B｜手游确认】本会话早前对国际服 iOS 11.1.1 的挖掘结果：每周状态表 PlayerWeekly.bin（17,073 名球员，取值 0–4）从一个全球 CDN 地址下载，URL 不含用户或 Rating 成分。因此状态箭头对所有玩家相同，不是“个人化削弱”。— 来源：本会话前期挖掘结论（任务背景第 5 条，复述自先前研究）
- 【D｜第三方攻略，非官方】状态（Condition）开赛前就决定，整场固定，和比赛中下降的体力不同；它由 Konami 每周的 Live Update 驱动。不同攻略对箭头档位数、颜色和数值的对应说法不一，具体公式未公开。— [GamingonPhone（eFootball 2027 指南）](https://gamingonphone.com/guides/efootball-a-guide-to-live-update-rating-player-form-and-conditioning/)；[Gamemarket.gg](https://gamemarket.gg/news/efootball/efootball-condition-arrows-explained-how-player-form-works)
- 【D｜旧作（PES 2021，主机/PC+手游共通时期）】PES 2021 时期的攻略称箭头颜色由球员的 Form（稳定性）属性和每周 Live Condition（A–E 五档）共同决定，精确公式未公开。— [PES myClub Guide（PES 2021 时期，可能不适用于现版本）](https://pesmyclubguide.com/player-form-condition-rating/)
- 【A｜全平台共享，2023 年起生效】v3.0.0 说明（据搜索摘要）："Team Playstyle Level" will no longer be a gameplay element… Players will no longer have "Team Playstyle Proficiency", and their Abilities will no longer be increased or decreased according to the Team Playstyle of choice.（大意：球员不再因球队风格熟练度而加减能力。）— [Konami Version Info v3.0.0（eFootball 2024，2023 年）](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-00)；[Konami Update v3.0 页面](https://www.konami.com/efootball/en/page/2024/update_v3_0)
- 【A｜旧文档】Dream Team 页面称有 5 种 Team Playstyle（如 Possession Game、Quick Counter），"To make the tactics work more effectively for your team, it is best to employ a manager with a higher proficiency for your chosen Team Playstyle."（主教练熟练度越高，战术越有效）。这是 v3.0 之前的描述，现版本是否还有效未确认。— [Konami Dream Team 页面（日期未取得，偏旧）](https://www.konami.com/efootball/en/page/dreamteam)
- 【A｜全平台】据搜索摘要，一份 2025 年的已知问题说明称，Exhibition 中主教练 Team Playstyle Proficiency 的显示问题“只是显示问题，不影响实际比赛和结果”。— 出处页面无法直接打开确认，搜索结果未给出具体 URL，列为待核实（见 Gaps）。

### Inferences
- 状态箭头是全服统一的周表，所以“对面什么卡都像神”有一个有据可查的解释：对手可以只上当周箭头好的球员，你阵中如果有红/黄箭头的高评分卡，实际能力可能不如对面箭头好的低评分卡。这是公开机制带来的差距，不是针对个人的调整（A 支持机制存在，B 支持全服统一，加成幅度仍是 D）。
- 体力和疲劳不在周表里，会在比赛中变化。见第三节，Konami 高层把部分“剧本感”归因于疲劳。

### Gaps
- 没有拿到 Konami 官方对箭头加成幅度的数值说明（官方帮助页无法直接读取）。
- 2025 赛季以后主教练“熟练度/增益（Booster/Coaching affinity 等）”对能力值的现行影响，没找到官方文档。
- 所谓“2025 年已知问题”里关于主教练显示问题的那条，搜索结果没给出可直接核对的 URL，原文未核实。
- 没有官方说明“队伍实力”数值怎么计算、是否进入匹配。

## 三、Konami 是否官方承认或否认在线 PvP 有让分、势头、剧本或难度调节？哪些补丁改过响应、输入延迟、传球速度或服务器？

### Takeaway
目前能找到的最直接官方表态，是 2026 年 2 月的开发者访谈（eFootball Connect / Spotlight Interview）：总监木村诚太郎（Seitaro Kimura）和总制作人田谷淳一（Junichi Taya）否认存在偏向任一方的“剧本”，并把玩家的“剧本感”部分归因于疲劳/技能、切换球员光标的修正、连接不稳定和按键输入延迟。不过我只看到二手报道，没读到原始视频或文字稿。官方补丁记录里确实有过“比赛中指令输入响应可能变慢”的已知问题、输入相关修复和服务器扩容，说明“卡顿、慢半拍”可以有不涉及剧本的官方来源。没找到任何官方承认存在让分或势头机制的材料。

### Cited Findings
- 【A（经二手报道）｜全平台共享】Gamereactor 报道：Konami 在 eFootball Connect 开发者访谈中“重申 eFootball 没有 scripting”。报道称更新依据游戏内问卷和遥测数据，服务器也按赛后问卷和数据检视、调整，并提到区域服务器上线和扩展计划。按搜索工具给出的文章“约 266 天前”推算，发布日期约为 2026-02 中旬（推算值）。原文无法打开，没拿到逐字引语。— [Gamereactor（约 2026-02）](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)
- 【A（经二手报道）｜全平台共享】另一次搜索的摘要称：2026 年 2 月 Spotlight Interview 中，Kimura 断然否认游戏内存在 scripting，并表示玩家感受到的“script”可能来自球员疲劳、连接不稳定、按键输入延迟等多种原因；也有摘要称两人否认的是“偏向任一方的故意剧本”，并把部分感受归因于 fatigue/skills 和 cursor change corrections（切换控制球员时的修正）。各摘要措辞略有差异，原话未核实。— [Gamereactor](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)；[Zaplat 调查文（2026，引用同一访谈）](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)；[Vietgame（eFootball CONNECT 报道，提到 Taya 参与问答）](https://vietgame.asia/efootball-connect-master-league-tro-lai-chuyen-de-game/)；[YouTube “Konami says there's no script in eFootball”（日期未取得）](https://www.youtube.com/watch?v=uTJ5-TknHF0)
- 【E｜第三方分析】Zaplat 指出，玩家常引用的“动态难度调整（DDA）”专利属于 Electronic Arts，不是 Konami；并认为单个异常片段或一次输入延迟不能证明人为操控。— [Zaplat（2026）](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)
- 【D｜玩家投诉，非官方】投诉平台上有多篇英文帖子称 eFootball 有“momentum system”，感觉被操纵。这些只是投诉，不是官方说法。— [Sikayetvar：Momentum System Feels Scripted](https://www.sikayetvar.com/en/konami-us/konamis-efootball-momentum-system-feels-scripted)；[Sikayetvar：eFootball 2025 Momentum Issue](https://www.sikayetvar.com/en/konami-us/efootball-2025-momentum-issue-unfair-to-longtime-players)
- 【A｜全平台共享，2021 年旧资料】eFootball 2022 首发后，Konami 公开道歉，承认过场、表情、球员动作和球的行为等问题，并提到要调整包括传球速度和防守操作在内的游戏平衡。可见传球速度是通过补丁全服统一调整的参数。— [VGC（2021-10）](https://www.videogameschronicle.com/news/konami-apologises-for-efootballs-disastrous-launch-promises-improvements)；[Pure Xbox（2021-10）](https://www.purexbox.com/news/2021/10/konami-apologises-for-efootball-problems-promises-improvements)
- 【A｜版本与日期待核实】据搜索摘要，Konami 某份已知问题列表（v0.9.1）写有 "responsiveness to command inputs may become slower during matches"（比赛中指令输入响应可能变慢）。搜索工具称这是“2025 年代”的资料，但 v0.9.x 的版本号按我的背景知识对应 2021 年 eFootball 2022 首发期，日期存在矛盾，未核实。— [Konami Announcements/Updates 页](https://www.konami.com/efootball/en/page/updates)
- 【A｜全平台】v5.4.0 版本说明（2026，据摘要）：调整了防守球员的反应，让盘带和 Sharp Touch 时的响应和推球更一致。这是玩法调校，不是网络修复。— [Konami Version Info v5.4.0](https://www.konami.com/efootball/en/page/v5/versioninfo_v5-40)
- 【A｜全平台】据摘要，v5.5.1（2026-04-06，全平台）修复了“进球动画期间的输入可能让射门朝向意外方向”等 bug。（日期和具体页面出自搜索摘要，未直接核对。另：这个日期和“5 月下旬至 6 月初”的匹配改动先后顺序是否一致，无法确认。）— [Konami News 5330](https://www.konami.com/efootball/en/topic/news/5330)；[Konami Updates 页](https://www.konami.com/efootball/en/page/updates)
- 【A｜全平台（含手游）】2026-11-04 官方 X："To provide a smoother gameplay experience, we have added a new server location and expanded server capacity…"，新增开普敦节点，并扩容东京、大阪、雅加达、新加坡、马来西亚、孟买等地（后面还列有巴黎、西班牙等，标题截断）。— [eFootball 官方 X，2026-11-04](https://x.com/play_eFootball/status/2117913032785350965)
- 【A｜全平台】v5.3.1 补丁 2026-03-05 面向全平台发布（转发账号消息，非官方账号）。— [eFootball_Radar X（2026-03-03）](https://x.com/eFootball_Radar/status/2028713612534075690)
- 【B｜手游确认，仅能说明“未见”】本仓库抓到的赛后结果结构里，`cont_win_num`（连胜数）和 `rating`、`oldrating`、`rating_boost_type`（抓包为 NONE）、`rating_boost_rate`（抓包为 1）并列；排行榜称号有 WIN_STREAK（30/50 连胜）。开赛时服务器会下发双方球员能力数组 `parameters`（至少 vs-AI 抓包如此），但没有任何字段名叫 handicap、streak 或 adjustment。没有抓到排位 PvP 的数据，所以既证明不了有，也证明不了没有。— 来源：本会话前期挖掘结论（任务背景第 1、4 条）；相关文件见 [/home/user/pes2027-mod/ziru-extra/responses/](/home/user/pes2027-mod/ziru-extra/responses/)

### Inferences
- 官方的否认（A，二手）本身不能当作证明，玩家的体验（D）也不能直接证明“系统局”存在。官方在否认的同时给出的替代解释，例如疲劳、切换光标修正、连接不稳定、输入延迟，都各自对应可以查到的机制（见第四、六节）。
- `cont_win_num` 的存在只说明服务器记录连胜，已知用途是称号和展示。`rating_boost_*` 看起来是 Rating 加成倍率（可能用于活动），抓包里是 NONE/1。在拿到排位 PvP 抓包之前，不能把它们解读成“连胜惩罚”参数，这一点只能列为待验证（E）。
- 传球速度、防守反应这类手感参数，Konami 一直是通过版本补丁全服统一改的（A）。所以补丁前后“传球变慢”的体感，应该先对照版本说明。

### Gaps
- 没能读到 2026 年 2 月访谈的原始视频或官方文字稿，逐字引语和“否认范围”（排位？所有 PvP？）未核实。
- 没找到 Konami 针对“连胜后被削弱”这一具体说法的点对点回应。
- 没找到官方专门讨论手游输入延迟或网络缓冲的补丁说明；客户端里 `MatchCommandBufferingControl` 和 `slowdown_judgment_threshold_list [51,25,17]` 的含义没有官方解释（数值可能是帧率阈值，属 E 级推测）。
- 没有排位 PvP 的开赛数据包，无法用数据确认或排除比赛内的能力调整。

## 四、信号天线图标、连接质量警告、中继/服务器对战模式是什么意思（online_match#antenna_icon）？

### Takeaway
官方帮助页说：天线图标实时显示“你自己”到游戏服务器的连接质量，按玩家分别测量，所以你屏幕上的天线可以和对手的不一样；赛后有 Network Graph 显示整场连接质量变化。官方不推荐用 Wi-Fi、移动数据或热点打 PvP。客户端配置把这个帮助页直接挂在“连接报告”入口上，并为 P2P 和 CS（客户端-服务器）两种模式分别设了天线阈值。

### Cited Findings
- 【A｜全平台共享（含手游），页面日期未取得】Konami《Online PvP Setup for eFootball》页（据搜索摘要）：在 client-server 模式的在线 PvP 中，天线图标显示你到游戏服务器的实时连接质量。因为按玩家分别测量，你屏幕上的天线可能和对手不同。赛后的 Network Graph 显示本场连接质量变化，数值越高越稳定。连接不稳时，官方建议关闭后台应用、远离微波炉等无线电设备、避免同一 Wi-Fi 下有大流量传输；手游要尽量靠近接入点，主机用有线；并说明这些措施不保证有效。— [Konami Online PvP Setup（en）](https://www.konami.com/efootball/en/page/online_match)；[同页 en-us](https://www.konami.com/efootball/en-us/page/online_match)；[同页 fr](https://www.konami.com/efootball/fr/page/online_match)
- 【A｜全平台】Konami 客服 FAQ（据搜索摘要）：PvP 不推荐 Wi-Fi、高速移动网络或热点，这些容易在对战中通信不稳或断线，建议光纤等宽带，同样不保证效果。— [Konami Support：What is the recommended internet connection to play PvP?（日期未取得）](https://us-support.konami.com/hc/en-us/articles/4813389230615-What-is-the-recommended-internet-connection-to-play-PvP)
- 【B｜手游确认】国际服服务器环境配置里，`{"type":"CONNECTION_REPORT_VIEW","url":"https://www.konami.com/efootball/_/page/online_match#antenna_icon"}` 把连接报告入口直接链到官方天线说明。另有两组天线阈值：`communication_antenna_status_info`：antenna_5_limit 30 / 4: 120 / 3: 171 / 2: 209；`communication_antenna_status_info_cs`：15 / 50 / 110 / 200。— [本地文件 GetServerEnv.json](/home/user/pes2027-mod/ziru-extra/responses/GetServerEnv.json)
- 【B｜手游确认】本会话早前挖掘：配置分 P2P 和 CS 两种对战模式，列有 62 个区域 game-relay 服务器，质量阈值为 75/175，并有 `MatchCommandBufferingControl`（输入指令缓冲控制）段。— 来源：本会话前期挖掘结论（任务背景第 3 条）
- 【D｜第三方，2024-12】社区指南称“Matchmaking Successful”画面会显示天线，只有 2 格或 1 格时建议取消重匹配。UI 可能已变化。— [community.efootball.fans（2024-12）](https://community.efootball.fans/2024/12/smooth-efootball-online-match-how-to.html)
- 【A｜全平台】官方 X 2026-11-04 新增服务器节点、扩容多个地区，目的是“smoother gameplay experience”，说明服务器位置和容量会影响对局流畅度。— [eFootball 官方 X，2026-11-04](https://x.com/play_eFootball/status/2117913032785350965)

### Inferences
- 天线阈值“越小越好”（≤30 为 5 格），数值单位没有文档，很可能是延迟（ms）或综合质量分（E）。CS 模式阈值更严，说明 CS 模式对延迟更敏感，或者测量口径不同（E）。
- “我网很好，但进攻三区一卡一卡”有官方可查的解释：①天线测的是到游戏服务器或中继的路径，不是你家 Wi-Fi 的测速，Wi-Fi 速度快不等于到中继稳定；②官方明说双方天线可以不同；P2P 模式下对手的差网会影响双方，CS 模式下各自到服务器的质量独立；③存在输入指令缓冲（MatchCommandBufferingControl），网络抖动时为了同步，加大缓冲是联网游戏的常见做法，体感就是“接球慢半拍、调整慢、传球慢”。手游上具体怎么实现未公开（③为 B+E）。
- 进攻三区球员密集、动画多，若手机本身掉帧（`slowdown_judgment_threshold_list` 可能是掉帧判定），也会表现为“只在关键区域卡”（E，需要测试）。

### Gaps
- 官方页面原文无法直接读取，“client-server 模式”和 P2P 模式各自用于哪些比赛类别（League、活动、好友赛），官方表述未拿到。
- relay 服务器质量阈值 75/175 的单位和用途，没有官方解释。
- 没找到官方说明“连接质量差时游戏会怎样补偿或加缓冲”。

## 五、PES/eFootball 是否官方宣传过自适应或学习型 AI（如 PES 2017 Adaptive AI）？是否只用于人机？

### Takeaway
PES 2017（2016 年，主机/PC 为主）官方确实把 Adaptive AI 作为卖点，宣传 AI 会“学习你的打法”并在比赛中调整战术。公开报道中的例子都是 AI 对手（人机），没找到它用于在线 PvP 或延续到 eFootball 手游的官方说法。

### Cited Findings
- 【A（经报道）｜主机/PC 旧作，2016】Konami 2016-05 公布 PES 2017 时称 Adaptive AI 会学习玩家打法，并号称会“change sports games forever”。— [God is a Geek（2016-05）](https://godisageek.com/2016/05/pes-2017-coming-konami-reveals-improvements-game/)；[XtremePS3（2016-05-25）](https://xtremeps3.com/2016/05/25/pes-2017-announced-for-ps3-and-ps4)
- 【A（制作人访谈）｜主机/PC 旧作】制作人 Adam Bhatti 举例：如果你大量依赖边锋，AI 会在边路包夹。这是 AI 对手层面的战术调整。— [God is a Geek 访谈（2016-08）](https://godisageek.com/2016/08/pes-2017-interview-adam-bhatti/)
- 【D｜媒体试玩】GameSpot 试玩约 10 场后表示难以判断 AI 到底多聪明；Pocket-lint 称 AI 会针对玩家习惯调整战术和跑位。— [GameSpot（2016）](https://www.gamespot.com/articles/pes-2017-seeks-to-become-the-most-realistic-soccer/1100-6440345/)；[Pocket-lint（2016，荷兰语）](https://www.pocket-lint.com/nl-nl/games/reviews/konami/137765-pes-2017-preview-eerste-uitgebreide-spel-van-potentiele-fifa-beater)
- 【B｜手游确认】本会话早前挖掘：国际服 vs-AI 的 Division 中，AI 难度由服务器按 Division 下发（进攻/防守 com level）。可见“难度参数”在 vs-COM 模式中确实存在、由服务器控制，但它挂在 vs-COM 上，不是 PvP 字段。— 来源：本会话前期挖掘结论（任务背景第 6 条）

### Inferences
- 在 PvP 中，对手由真人操作，但双方未被操控的球员（队友跑位、协防）仍然由 AI 驱动。“对面补防神勇”这类体感，理论上也可能来自对方的战术设置、防守 AI 参数或 Smart Assist 辅助等普通机制。没有证据表明有针对单个玩家的自适应调节（E）。
- vs-COM 难度按 Division 变化，玩家如果把人机 League 的体验迁移到 PvP，可能更容易认为“系统会调难度”（E）。

### Gaps
- 没找到 eFootball（2021 起）官方宣传自适应或学习型 AI 的材料，也没找到 PES 2017 Adaptive AI 是否在在线模式启用的官方说明。

## 六、综合：哪些有据可查的机制，会让玩家在连胜后感受到“系统局”？

### Takeaway
不需要假设存在隐藏的“连胜惩罚”，也有至少五类官方或数据可查的机制，会在连胜后集中出现，产生“自己被削弱、对面像神、卡顿慢半拍”的体感：①Rating/Division 驱动的同水平匹配（连胜→对手更强）；②全服统一的状态箭头，对手可以挑箭头好的球员；③疲劳和切换光标修正（官方访谈点名）；④连接质量因人而异、指令缓冲和中继路径；⑤个别版本的输入响应 bug 和平衡调整。玩家体验（D）很普遍，但目前没有公开数据证明 PvP 有针对连胜的数值调整，也没有排位 PvP 抓包能排除这一点。

### Cited Findings
- 【A】同水平匹配和 Rating 升降级：官方 2026-05-28 “make it easier for players of similar skill levels to be matched”；Division 1–3 按 Rating 升降。— [eFootball 官方 X，2026-05-28](https://x.com/play_eFootball/status/2060020420867158381)；[Konami v3.2.0](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-20)
- 【A】匹配条件越严→越可能匹配到连接不稳定的对手（官方原话）。— [eFootball 官方 X，2026-05-19](https://x.com/play_eFootball/status/2056702522933514339)
- 【A】天线按玩家分别测量，你和对手的连接质量可以不同；官方不推荐 Wi-Fi 和移动网络打 PvP。— [Konami Online PvP Setup](https://www.konami.com/efootball/en/page/online_match)；[Konami Support](https://us-support.konami.com/hc/en-us/articles/4813389230615-What-is-the-recommended-internet-connection-to-play-PvP)
- 【A（二手）】Konami 高层把“剧本感”部分归因于疲劳/技能、切换光标修正、不稳定连接和输入延迟。— [Gamereactor（约 2026-02）](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)；[Zaplat](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)
- 【B】状态箭头是全服统一的周表，不含用户成分。— 本会话前期挖掘（PlayerWeekly.bin）
- 【A，版本日期待核实】官方曾列出“比赛中指令输入响应可能变慢”的已知问题。— [Konami Updates 页](https://www.konami.com/efootball/en/page/updates)
- 【D】大量玩家报告“momentum/剧本”体感。— [Sikayetvar 投诉](https://www.sikayetvar.com/en/konami-us/konamis-efootball-momentum-system-feels-scripted)

### Inferences
- **触发时机（推断）**：最可能的“触发点”是连胜让 Rating 或 Division 上升、对手池变强。每个 Phase 初期分组更宽（A），也会让对手强度波动更大。另外，长时间搜不到对手时匹配范围可能扩大（D），在高分段和深夜人少时更容易匹配到远距离、连接更差的对手（E）。
- **“改了哪些数值”（推断）**：公开证据里能改变球员实际表现的数值只有三类：①全服统一的状态箭头（每周）；②比赛中的体力和疲劳；③版本补丁的全局平衡（传球速度、防守反应等）。网络层面的延迟和指令缓冲不改能力值，但会改“你的操作生效的时间”，主观上很像“属性被削”。没有证据显示存在按个人连胜调整能力值的字段（B，仅限已抓包的范围）。
- **降低影响的做法（由官方文档推出）**：①看匹配成功画面和赛后 Network Graph，天线差就取消或换网，打完回看曲线确认是不是网络问题（A+D）；②手机靠近路由器，优先用稳定的 5GHz 或有线转接，关闭后台下载和应用，同一网络避免大流量（A）；③Smart Assist 匹配选“Matchmake freely”能缩短匹配时间、减少遇到差网对手的概率（A 官方警告的反面，属推断）；④每周按状态箭头轮换阵容，避免红/黄箭头的主力首发（A 机制+D 加成幅度）；⑤管理体力，及时换人（呼应官方点名的疲劳）；⑥连胜后对手变强是 Rating 匹配的正常结果，胜率回落到约五成是预期现象（推断），可以用赛后数据（控球、射门、网络曲线）区分“对手更强”和“网络问题”，并做记录（E）。
- **对“系统局一定存在”的评估**：现有官方材料是否认（A，二手），数据挖掘没有发现相关字段，但没有 PvP 数据（B，不完整），玩家体验很普遍（D）。三者合起来，结论是“未被证实，也未被完全排除”，而上面这些已知机制足以解释大部分描述的症状。

### Gaps
- 缺少排位 PvP 的开赛和赛后数据包。要直接检验“连胜后能力被调整”，需要在连胜前后抓包，对比服务器下发的 `parameters` 能力数组是否和卡面、状态箭头一致（可复现测试，B 级）。
- 缺少官方对 Rating 公式、匹配扩展规则、PING_LEVEL 档位的说明。
- 本会话无法直接读取 konami.com 原文，以上 konami.com 内容都是搜索摘要转述，报告写作时应注明，或另行核对原文。
