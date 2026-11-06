# 核查报告：intl_community.md（国际服 eFootball 手游社区对“系统局/scripting/momentum/handicap/DDA”的说法）

> 核查日期：2026-11-06。范围只限 Konami 国际服 eFootball 手游（iOS/Android），网易国服材料不核查、不引用。原笔记未改动。
> 证据等级沿用原笔记：(A) Konami 官方确认或文档；(B) 可复现测试、数据集、拆包或逆向；(C) 专利或论文；(D) 广泛流传的玩家说法；(E) 单个说法或推测。平台标注：【手游确认】【跨平台共享】【主机/PC专属】【未知】。
> **方法限制**：本会话 WebFetch 打开 www.gamereactor.eu 和 zaplatgaming.com 都失败（域名解析失败，不在会话允许的网络域名内）。按工具提示，之后没有再用 WebFetch 尝试其他站点，所有核对都依靠 WebSearch 返回的页面摘要。因此下文的“已核实”指“检索摘要与笔记说法一致”，属于二手核对，没有逐字核对原文。要核对原文，需要在会话网络设置里放行 gamereactor.eu、zaplatgaming.com、steamcommunity.com、konami.com、x.com 等域名。

---

## 一、已核实

| # | 笔记中的说法 | 核对来源 | 结论 |
|---|---|---|---|
| 1 | Konami 在 eFootball CONNECT“Spotlight Interview”中否认存在有意偏袒一方的 scripting，并把部分不公平感归因于疲劳/技能和切换光标（cursor change）修正 | [Gamereactor（2026-02-13）](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)；[eFootball 官方 X 发布访谈的帖子（2026-02-04，由帖子 ID 换算）](https://x.com/play_eFootball/status/2019007136819519702) | **属实（二手）**。检索摘要和 Gamereactor 标题一致：否认“偏袒任一方的有意脚本”，归因于 fatigue/skills 和 cursor change corrections。另一条检索摘要转述 Kimura 的话：体力和能力会随比赛时间下降，光标移动可能出现不一致。逐字原话仍未取得。官方没有区分平台；同一访谈还谈到手游手柄支持，所以这次否认至少覆盖手游。【跨平台共享，含手游】等级 A（官方表态，经二手转述，只是否认，不是技术文档）。日期需更正，见第二节。 |
| 2 | 标题用 “reiterates”（重申） | 同上 | 标题属实。但这次检索没有找到 Konami 在 2026-02 之前对 scripting 的公开否认原文，所以“之前否认过”只是从标题推出来的，没有出处。 |
| 3 | zaplat 调查（约 2026-09）：没有独立、可复现的技术证据；截至 2026-09 没有公开独立审计；没有证据不等于证明不存在；提出受控测试设计 | [zaplat](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)（原文无法打开，以检索摘要核对） | **基本属实（二手）**。摘要原文包括 “found no independently reproducible technical evidence…” 和 “as of September 2026, we found no public independent audit”，测试设计（相同阵容、状态、体力、辅助、战术，受控网络，大量重复）也一致。笔记引用的英文结论句 “the player experience is real… remains unproven” 在检索里没有逐字命中，应视为意译。zaplat 是粉丝/攻略站，不是独立研究机构，等级维持“二手分析”。【跨平台】 |
| 4 | revibyte 2026 手游评测：scripting 争论普遍，Konami 从未官方确认 | [revibyte](https://www.revibyte.blog/posts/efootball-2026-mobile-real-football-or-scripted/) | **存在，“Konami 从未确认”属实**。但摘要显示作者本人倾向认为 momentum 是一种“幕后调整”，这是观点文章，不是中立综述（见第二节）。“连胜之后接连败”的具体说法未能核实。【手游】D/E |
| 5 | Steam eFootball（App 1665460）讨论区有大量 handicap/scripting 帖子；“Handicap, meta lineups, scripted matches and a complete incompetence of the devs have destroyed it”；“Higher level = more and more scripting”；“the worse my opponent is, the 'luckier' he is” | [Steam 3818543331788527110](https://steamcommunity.com/app/1665460/discussions/0/3818543331788527110)；[Steam 3486374957056007672](https://steamcommunity.com/app/1665460/discussions/0/3486374957056007672)；[Steam 7008219766206736996](https://steamcommunity.com/app/1665460/discussions/0/7008219766206736996) | **大体属实**。第一句出自帖子 3818543331788527110（摘要给出的日期一说 2023-08，一说 2022，有冲突）。第二句出自 2023-09 的一个帖子，摘要没有把它对应到笔记列出的帖子 ID。第三句只找到意思相近的转述，没有逐字命中。App 1665460 是 Steam（PC）版，标【主机/PC专属】正确。D（单帖 E）。 |
| 6 | PES 2020 / PES 2021 时代 Steam 也有 handicap 抱怨 | [Steam PES 2020](https://steamcommunity.com/app/996470/discussions/0/1629663905426859457)；[Steam PES 2021](https://steamcommunity.com/app/1259970/discussions/0/3005549744947547598) | **属实**。摘要还显示有 2019 年帖子称 handicap 让人无法连胜。【主机/PC，旧作】D |
| 7 | Steam 玩家说打 CPU 没有延迟，只在 PvP 出现；有人归咎 lag switch 或服务器 | [Steam 3271312763274877449](https://steamcommunity.com/app/1665460/discussions/0/3271312763274877449) 等 | **属实**。摘要里有 2024-12 帖子写“Against CPU PVE there are no lag nor delay, only in PVP matches”，也有连续 16 场遇到 lag switch 的说法，以及“是服务器问题、不是对手”的反驳。具体哪句在哪个帖子 ID 下没有逐一对上。【主机/PC】D/E |
| 8 | MMOBomb：v4.4.0 加入 PC/Steam/Xbox/PlayStation 跨平台对战，Konami 在 CONNECT 里公开服务器位置；报道没提手游 | [MMOBomb](https://www.mmobomb.com/news/konami-f2p-efootball-finally-adds-crossplay-matches-against-ai-opponents)；[VGC](https://www.videogameschronicle.com/news/efootball-is-adding-crossplay-and-ai-opponents-in-a-major-update-this-week)；[Konami v4.4.0 Information](https://www.konami.com/efootball/en-us/page/v4/versioninfo_v4-40) | **属实**，日期可以精确到 **2025-04-17（v4.4.0 上线）**。跨平台只覆盖 PlayStation、Xbox、Steam、Windows，**不含手游**。“公开服务器位置”只有 MMOBomb 一家说法。【主机/PC】A（版本说明）/ 二手报道 |
| 9 | EA 约 2019 年否认 FIFA 使用 scripting/DDA，称专利从未用于 FIFA/Madden/NHL | [PCGamesN](https://www.pcgamesn.com/fifa-19/scripting-dda-momentum)；[Goal](https://www.goal.com/en-gb/news/ea-sports-deny-theres-been-scripting-in-fifa-19-or-previous-game-versions/1fmb32lnhiwlw1jppwxap7ki9w)；[Gamer Revolution](https://www.gamerevolution.com/news/554581-fifa-scripting-isnt-real-ea-confirms) | **属实**，时间是 **2019 年 6 月**（法语报道日期 2019-06-19；EA 社区经理 Corey Andress 在官方论坛表态）。背景对照，不是 eFootball。 |
| 10 | EOMM 论文（arXiv 1702.06820，2017，EA 员工参与） | [arXiv PDF](https://arxiv.org/pdf/1702.06820)；[ADS 摘要](https://ui.adsabs.harvard.edu/abs/2017arXiv170206820C/abstract) | **论文存在，WWW 2017，作者含 EA 研究人员，属实**。“worst among all matchmaking schemes”这句引文是有条件的，见第二节。C |
| 11 | 2020-11 美国集体诉讼，EA 回应 “baseless and misrepresent our games” | [PC Gamer](https://www.pcgamer.com/class-action-lawsuit-claims-eas-dynamic-difficulty-tech-encourages-loot-box-spending)；[Game Informer 2020-12-01](https://www.gameinformer.com/2020/12/01/ea-sports-lawsuit-alleges-fifa-21-difficulty-is-a-deliberate-push-for-loot-box-purchases) | **属实**（起诉于 2020-11，北加州联邦地区法院，原告为三名加州玩家）。撤诉日期需更正，见第二节。 |
| 12 | Dexerto 称 EA 在法国还面临一起类似 “scripting” 的长期诉讼 | [Dexerto](https://www.dexerto.com/fifa/ea-sports-debunk-fifa-scripting-claims-1526503) | **Dexerto 确实这么写**，但其他报道对法国案的描述不同，见第二节。 |
| 13 | Marvel Rivals 玩家质疑 EOMM，开发方承诺解释匹配系统 | [AV Club](https://www.avclub.com/marvel-rivals-matchmaking-eomm-explained)；[GamesRadar](https://www.gamesradar.com/games/third-person-shooter/marvel-rivals-dev-denies-using-eomm-explains-that-an-overwatch-like-role-queue-wont-work-because-the-games-tank-and-support-heroes-arent-interesting-enough) | **属实**。补充一点：开发方（NetEase）明确否认使用 EOMM。这是另一款游戏，与 eFootball 和网易实况国服都无关，只作跨游戏背景。 |
| 14 | 球员 Form 分 Inconsistent/Standard/Unwavering，Condition 是每场状态值 | [GamingOnPhone 2024 指南](https://gamingonphone.com/guides/efootball-2024-a-guide-to-live-update-rating-player-form-and-conditioning/)；[Konami v3.0.0 Version Info](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-00) | **属实，而且有官方文档，等级应该上调**（见第二节）。GameMarket 那篇在检索里没有出现，未核实。 |
| 15 | r/pesmobile（镜像，2025-03-18）：Inconsistent 球员拿顶级绿箭头的次数比 Unwavering 多 | [r/pesmobile 镜像](https://lr.eu.psf.lt/r/pesmobile/comments/1jdzs3k/for_those_who_are_planning_to_get_inconsistent) | **存在，日期 2025-03-18 一致**。内容和摘要一致，属于评论者的经验说法。【手游】E |
| 16 | Scribd 上的 “God Mode/auto-goal” 是第三方作弊脚本推广页，不是官方脚本 | [Scribd 942686403](https://www.scribd.com/document/942686403/text-1) | **页面存在，定性正确**。 |
| 17 | 西语视频 “🚨 Konami lo CONFIRMÓ 🚨 El 'Handicap' NO EXISTE en efootball” | [YouTube 7W8kz3ZlQxY](https://www.youtube.com/watch?v=7W8kz3ZlQxY) | **标题属实**。它只是转述 Konami 的否认，不算独立证据。其余葡语/西语视频（EOh6L7LqnTE、mFC4ntWXFRE、JTo1m4EjrJU）检索无法索引，未能核实，维持 E。 |

---

## 二、需更正或降级

1. **Kimura 的日文名和头衔**
   - 问题：笔记写“Seitaro Kimura（木村诚太郎）……总经理”。
   - 更正：日文名应为**木村征太郎**（Famitsu 2022 年访谈、ASCII 2021 年报道都写作“木村征太郎”）。头衔方面，Konami 官方 X 称两人为 “General Director and General Producer”，Gamereactor 写作 “general manager”。建议写成“Seitaro Kimura（木村征太郎，General Director）与总制作人 Junichi Taya”。Taya 的汉字名未核实，不要自行补写。来源：[Famitsu](https://www.famitsu.com/news/202204/20258930.html)；[ASCII](https://ascii.jp/elem/000/004/068/4068770/)；[eFootball 官方 X](https://x.com/play_eFootball/status/2019007136819519702)

2. **访谈时间**
   - 问题：笔记写“约 2026 年 2–3 月”“推断为 2026 年 2 月”。
   - 更正：官方 X 于 **2026-02-04** 发布 Spotlight Interview（由帖子 ID 换算）。Gamereactor 的报道发于 **2026-02-13**。v5.3.0 在访谈次日（2026-02-05）上线。

3. **官方否认的等级表述（问题3 Takeaway）**
   - 问题：笔记写“A 级”。这容易被理解成“系统局不存在”已有 A 级证据。
   - 更正：A 级只能用来证明“Konami 在 2026-02 公开否认存在偏袒一方的有意脚本”这件事本身。它是利益相关方的口头否认，没有附技术文档，没有第三方审计，原话也只能看到二手转述。报告里应写成：“官方否认（A，二手转述），但没有外部验证。系统局既没有被证明存在，也没有被独立证伪。”另外，“There is absolutely no 'scripting'”这类引号内的英文只出现在检索摘要里，不要当逐字引语使用。

4. **“对手网络差或用 lag switch，会让你也感到不同步”（问题5 Inferences）**
   - 问题：这是笔记作者从通用网络常识推出来的，但它和 Konami 官方说明冲突。Konami《Online PvP Setup for eFootball》页说，eFootball League、Ranking Event 等看重结果的模式使用 client-server 方式，原文是：“Each user's network connection is not affected by the quality of another user's connection (lag, stability, etc.) or device performance.” 只有 P2P 方式的模式才会“受双方各自连接质量影响”。
   - 更正：按官方说法，手游排位（client-server）里，影响你操作手感的主要是**你自己到游戏服务器的链路**，不是对手的网络。“本地测速正常”不等于“到服务器的链路正常”。官方还说服务器选择会综合位置和网络状况，较远的服务器有时连接质量反而更好。官方没有公开说明：对手连接很差时，服务器如何处理对手的延迟输入，在你的屏幕上会不会出现对手“瞬移”或“突然加速”这类现象。这部分只能标为【未知】。Steam 上的 lag switch 说法来自 PC 玩家，不能直接套到手游排位。来源：[Konami Online PvP Setup](https://www.konami.com/efootball/en/page/online_match)（A，全平台含手游，页面日期未取得）
   - 补充推测（E，无来源）：如果用户在中国大陆使用国际服，跨境链路的延迟和抖动可能比本地测速显示的差很多。这一点可以引导用户去看赛中天线图标和赛后 Network Graph，而不是只看本地网速。

5. **“跨语种、跨平台的普遍体验”（问题1 Inferences）**
   - 问题：笔记里能直接引用的抱怨，大部分来自 Steam（PC）和 FIFA，手游专属的材料很少。而且截至 2026-02，**手游排位和主机/PC 不是同一个玩家池**：v4.4.0 的跨平台不含手游；v5.3.0 只允许手游在 VS AI 和好友赛里用蓝牙手柄；访谈总结称“Mobile to Console/PC not planned due to software incompatibility”。
   - 更正：可以说“症状描述在手游和主机/PC 社区高度相似（D）”。但 Steam 证据描述的是另一个玩家池，部分模式还是 P2P 连接，不能当作手游排位的直接证据。手游专属的 D 级材料要另外补充，见第三节第 9 条。

6. **EOMM 论文引文**
   - 问题：笔记写论文认为同水平匹配 “worst among all matchmaking schemes”，漏掉了前提条件。
   - 更正：原句是**有条件的理论结论**：“If c(Win) + c(Lose) < 2·c(Draw), equal-skill based matchmaking is actually worst among all matchmaking schemes”，也就是只有当“输一场加赢一场的流失代价之和小于平局流失代价的两倍”时才成立。仿真结果是：SkillMM 在留存上并不稳定优于随机匹配；EOMM 在一轮匹配后比 SkillMM 多留存约 0.7% 的玩家。“针对游戏时长甚至消费调参”是论文提到的可调目标，不代表任何已上线游戏这么做。C 级，与 eFootball 无关。

7. **美国集体诉讼的撤诉时间**
   - 问题：笔记写“约 2021-03”。
   - 更正：原告于 **2021-02-11** 在北加州联邦地区法院自愿撤诉。媒体报道集中在 2021 年 2 月中旬到 3 月初，Sportbible 的 URL 显示 2021-03-05。“EA 提供了详细技术资料并让工程师接受询问”是 EA 自己的说法，没有独立审查。来源：[PC Gamer](https://pcgamer.com/fifa-dynamic-difficulty-lawsuit-dropped-after-plaintiffs-talk-to-eas-engineers)；[Game Developer](https://www.gamedeveloper.com/console/dynamic-difficulty-adjustment-lawsuit-ends-as-ea-denies-its-use-in-i-fifa-i-)；[EA: Fair Play & Dynamic Difficulty Adjustment（约 2021）](https://www.ea.com/news/fair-play-and-dynamic-difficulty-adjustment)

8. **法国诉讼**
   - 问题：笔记只引用 Dexerto 的“类似 scripting 的诉讼”。
   - 更正：Ginx 把法国案描述为“主张把 FUT 认定为赌博”的诉讼，其他报道提到的是美国和加拿大的开箱诉讼。法国案的性质有出入，应写成“据 Dexerto，另有一起法国诉讼（性质各家报道不一，未核实）”。

9. **EA 否认的时间**：“约 2019 年”可以改为“2019 年 6 月（FIFA 19 时期，EA 在官方论坛表态）”。

10. **Form/Condition 的证据等级（问题5）**
    - 问题：笔记标为 D（玩家/媒体指南）。
    - 更正：Form 类型和 Condition 规则有 Konami 官方文档。v3.0.0 版本说明写到：Standard 和 Inconsistent 球员出现 “Bad” 或更差 Condition 的概率降低；Inconsistent 球员的 Live Update Rating 为 B 或以上时，Condition 会设为 “Normal” 或以上。规则存在、可影响当场表现，应定为 **A【跨平台共享】**。箭头具体加减多少能力值、各档位的概率仍然没有官方数字，这部分维持 D。来源：[Konami v3.0.0 Version Info（eFootball 2024，2023 年）](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-00)

11. **sikayetvar 的 “control errors… button inputs respond with a delay” 条目（问题1）**
    - 问题：笔记把它当作“有玩家提问”（E）。这个链接是 `q-25600` 问答页，同类页面带 “Why this happens / What to try” 这种固定结构，更像平台生成的帮助问答，不是某个玩家的具体投诉。检索也没有找到这句原文。
    - 更正：降为“未核实，不作为玩家证据”。sikayetvar 上确实有大量真实标题的手游投诉，可以换用，见第三节第 9 条。

12. **Netduma 论坛的建议（问题5）**
    - 问题：笔记说该帖建议关闭抗锯齿和垂直同步、降低分辨率、改用有线手柄。
    - 更正：帖子 58171 存在（约 2025-03），但摘要只显示用户在问路由器设置，管理员的回复里看不到这些建议。Netduma 是路由器厂商论坛，用户以主机为主。这些建议的出处未核实，应降为“E，未核实，主机/PC 背景”。

13. **efootballmobileguide.com 和 lagx.app（问题5）**：两站在检索中都没有出现，内容未能核实，维持 E 并标“未核实”。类似的说法可以在 GearUP 的指南里找到（掉帧属于设备问题，输入晚到或对手瞬移属于网络问题），但 GearUP 也是推销加速器的营销内容。来源：[GearUP: Fix eFootball Mobile Lag](https://www.gearupbooster.com/blog/fix-efootball-mobile-lag.html)

14. **revibyte 的定性**：笔记写成“评测说 scripting 争论很普遍”，语气偏中立。检索摘要显示作者本人主张 momentum 像一种幕后调整，并否认这是在模拟真实足球的偶然性。应标为“倾向存在系统局的观点博客，作者不明，D/E”，不能当中立评测引用。

15. **Steam 帖 “Joke of a handicap and script again”（3711558744928656613）**：按标题和帖子 ID 都检索不到，未核实。报告不要把这个标题当作已核实引文。

16. **“B 级证据为零”的表述（问题2）**：对公开社区资料来说成立。但要提醒：本仓库 `ziru-extra/responses/` 下的服务器响应文件，按 `ziru-extra/README.md` 的说明，是“为这个移植版编写的服务器响应”（written for this port），不是从 Konami 正式服务器抓取的。如果最终报告引用这些本地文件里的匹配或连接参数（例如 official_mechanics.md 引用的 GetServerEnv.json 天线阈值）并当作“B｜手游确认”，就高估了证据等级。使用前要先确认每个值的来源。另外两个字段容易被误读：本地文件里的 `WIN_STREAK` 是荣誉/称号（fame）类型，用来统计连胜成就，不能证明连胜会触发比赛调整；`is_enable_send_adjust_param` 的同一段里有 `adjust_id`，指的是 Adjust 广告归因 SDK，和“难度调整”无关。

17. **MMOBomb 条目的时间（问题3）**：“约 2025 年春”可精确为 2025-04-17。笔记写“报道没有提到手游”，这一点可以更明确：v4.4.0 的跨平台本来就不包括手游。

---

## 三、补充发现

1. **官方发布 Spotlight Interview 的原帖**：eFootball 官方 X，2026-02-04，“our General Director and General Producer directly respond to many of the comments and feedback”。这是访谈的一手入口（视频本身未能取得）。A｜跨平台共享（含手游）— [x.com/play_eFootball/status/2019007136819519702](https://x.com/play_eFootball/status/2019007136819519702)

2. **手游排位是单独的玩家池（截至 2026-02）**
   - v5.3.0（官方 X，2026-02-05）：蓝牙手柄只能在手游的 VS AI 比赛和好友赛（1v1、合作）里使用。A｜手游确认 — [x.com/play_eFootball/status/2019358515807023546](https://x.com/play_eFootball/status/2019358515807023546)
   - 社区账号 Arrow Games 对 2026-02 访谈的总结（2026-02-04）：“Mobile to Console/PC not planned due to software incompatibility”。D（社区转述官方访谈）— [x.com/ArrowgamesEF/status/2019012002924871857](https://x.com/ArrowgamesEF/status/2019012002924871857)
   - 冲突说法：2025-08-20 有粉丝账号转述 IGN，称 Konami 表示跨平台“会覆盖所有版本，手游对主机/PC 需用手柄”。以时间更晚的 2026-02 访谈为准，但两者冲突，报告应注明。E — [x.com/efootball_BDX/status/1958139349716943154](https://x.com/efootball_BDX/status/1958139349716943154)
   - 影响：Steam 和主机上的抱怨与手游排位不是同一批对手，只能作为“同引擎、同类症状”的旁证。

3. **官方说明：排位用 client-server，对手的连接质量不影响你的连接**。Konami《Online PvP Setup for eFootball》页说：看重结果的模式用 client-server，每个用户的连接不受他人连接质量或设备性能影响；P2P 模式会受双方各自连接质量影响；服务器选择会综合位置和网络状况；天线图标实时显示你到服务器的连接质量，赛后有 Network Graph。v4.4.0 版本说明宣布为 client-server 对战加入天线显示。这是“自己网络正常却卡顿”最直接的官方排查工具。A｜跨平台共享（含手游），页面日期未取得 — [konami.com/efootball/en/page/online_match](https://www.konami.com/efootball/en/page/online_match)

4. **官方承认作弊和违规操作存在**。Konami 新闻 5312：2026-05-28 对 eFootball League 中使用漏洞（exploits）的用户封号或处罚，违规类型包括异常结果导致的高频中止对局、自动化工具或宏、篡改程序或游戏数据。Konami 新闻 5907：2026-10-01 对疑似使用自动化工具、程序、宏的账号封号。这说明“对手表现异常”有一部分可能来自少数作弊者，这是官方承认存在的替代解释，但发生频率不明。A｜跨平台共享（公告未区分平台）— [Konami News 5312](https://www.konami.com/efootball/en/topic/news/5312)；[Konami News 5907](https://www.konami.com/efootball/en/topic/news/5907)

5. **官方承认存在输入延迟，并通过服务器端更新调整过**。Operation Sports 报道 Konami 在 X 上发长帖说明：10 月 29 日面向主机和手游的服务器更新，按 Konami 自己的数据降低了输入延迟；会扩充全球服务器站点；赛后会弹出关于连接质量和响应性的问卷。文章发布日期未能确认（检索元数据推算约 2025-12，所以“10 月 29 日”很可能是 2025 年，但未核实）。另有一条已知问题“比赛中指令输入响应可能变慢”，见 official_mechanics.md。意义在于：“操作慢半拍”至少有一部分来自官方承认、会随服务器版本变化的延迟问题。A（二手报道）｜跨平台含手游 — [Operation Sports](https://www.operationsports.com/efootball-outlines-new-efforts-to-improve-online-match-quality/)；[inkl 转载](https://www.inkl.com/news/efootball-outlines-new-efforts-to-improve-online-match-quality)
   - 相关但未核实：2026-11-04 有玩家在 X 上称“tuning update 后输入延迟降低约 50%”，没有官方出处。E — [x.com/SnubbbS_/status/2118082045171912886](https://x.com/SnubbbS_/status/2118082045171912886)

6. **Form/Condition 规则的官方文档**：见第二节第 10 条。A｜跨平台共享 — [Konami v3.0.0](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-00)

7. **排位里的 AI 对局**：v4.4.0 新增独立的 “eFootball League (VS AI Category)”，AI 强度随 Division 提高。更早的 v3.2.0 规定，Division 7–9 在特定条件下会匹配到 AI。用户在高段位 PvP 一般遇到的是真人，但报告可以说明官方存在 AI 对局分类，避免读者混淆。A｜跨平台共享 — [Konami v4.4.0](https://www.konami.com/efootball/en-us/page/v4/versioninfo_v4-40)；[Konami v3.2.0](https://www.konami.com/efootball/en/page/2024/versioninfo_v3-20)

8. **zaplat 提到的“2023 年 Reddit 实验”**：zaplat 称 2023 年有 Reddit 用户连续几周追踪异常对局，最初的结论是：许多被叫作 scripting 的事件，可以用网络表现和对机制的误解来解释。原帖没找到，方法、数据和后续结论都无法核实。这是目前唯一提到的“社区追踪测试”，只能算 E（二手）。zaplat 同时称排位等模式用 client-server，与第 3 条官方说明一致。— [zaplat](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)

9. **手游专属的 D 级玩家投诉（补上笔记缺的手游材料）**：sikayetvar 英文区有多条标题明确写了手游或机型的投诉，日期大多不明，内容是个人感受，没有数据。例如：
   - [eFootball Mobile Momentum System Makes Matches Feel Unfair](https://www.sikayetvar.com/en/efootball-us/efootball-mobile-momentum-system-makes-matches-feel-unfair)（5 月 5 日，年份不明）
   - [Konami eFootball 2025 Mobile PVP is Rigged](https://www.sikayetvar.com/en/konami-us/konami-efootball-2025-mobile-pvp-is-rigged)
   - [eFootball Passes Go Wrong and Players Won't Respond in Division 2 and 3](https://www.sikayetvar.com/en/konami-us/efootball-passes-go-wrong-and-players-wont-respond-in-division-2-and)（段位升高后操作响应变慢，和用户描述最接近）
   - [I Face eFootball 2026 Players Losing Performance Mid-Match on iPhone](https://www.sikayetvar.com/en/efootball-us/i-face-efootball-2026-players-losing-performance-mid-match-on-iphone)
   - [Having eFootball Momentum Issue on Vivo V29 Lite](https://www.sikayetvar.com/en/konami-us/having-efootball-momentum-issue-on-vivo-v29-lite)
   - [eFootball Mobile Lag And Input Delay Issue On Redmi 14C](https://www.sikayetvar.com/en/efootball-us/efootball-mobile-lag-and-input-delay-issue-on-redmi-14c)

   D｜手游确认（平台由标题判断）。

10. **“进攻三区卡顿”的另一种可能（E，未经测试）**：GearUP（营销内容）把“角球、任意球、禁区人多时掉帧”归为设备性能问题，把“对手瞬移、动作延后生效”归为网络问题。EA FC 26 的 PC 玩家也报告过“进入对方禁区时卡顿”。所以“只在进攻三区卡”可能是禁区附近画面负载高导致手机掉帧，也可能是网络抖动恰好出现在对抗密集的时刻，目前没有 eFootball 手游的测试能区分这两种情况。建议报告把它写成可以自测的假设：打 VS AI 时禁区附近是否也卡；降低画质或帧率后是否改善；卡顿时天线格数是否下降。— [GearUP](https://www.gearupbooster.com/blog/fix-efootball-mobile-lag.html)；[EA Forums FC 26](https://forums.ea.com/discussions/fc-26-technical-issues-en/set-piece-lag--random-gameplay-stutters/13449505)

11. **EA FC 时代（2023 年后）的官方立场（补上笔记的空白，背景对照）**：EA 官方论坛在 FC 24/25/26/27 和 FC Mobile 版块都置顶了 “[Info] Handicap, Momentum, Scripting, DDA”，称这些机制在游戏里不存在，相关话题会被删除。另有粉丝账号转述 FC 26 开发者访谈（2025-07-25），开发者把“剧本感”归因于比赛后段的疲劳，这个思路和 Konami 2026 年的说法很像。A（EA 官方表态，仅针对 EA 游戏）/ E（转述）— [EA Forums FC 25](https://forums.ea.com/discussions/fc-25-general-discussion-en/info-handicap-momentum-scripting-dda/7930299)；[EA FC Mobile Info Hub](https://forums.ea.com/blog/ea-sports-fc-mobile-game-info-hub-en/info-handicap-momentum-scripting-dda/12392959)；[X @WeaverFUT](https://x.com/WeaverFUT/status/1948782802125865374)

12. **仍然找不到的东西**：Konami 关于“是否参考连胜、胜率或隐藏 MMR 匹配”的官方说明；针对手游排位的抓包或网络同步分析；任何可复现的大样本测试；Konami 名下与“按连胜调整能力”相关的专利（patents_research.md 也没找到）。因此报告的总体结论应写成：**系统局既未被证实，也未被独立证伪。“已证明存在”和“已证明不存在”两种说法都超出了现有证据。**
