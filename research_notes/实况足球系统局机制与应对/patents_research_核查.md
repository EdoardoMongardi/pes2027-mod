# 核查报告：patents_research.md（专利、学术研究与数据分析）

> 核查日期：2026-11-06。范围只限 Konami 国际服 eFootball 手游（iOS/Android）。原笔记未改动。
> 证据等级沿用原笔记：(A) Konami 官方；(B) 可复现测试、数据集、数据挖掘或逆向；(C) 专利或论文（只证明技术存在）；(D) 广泛流传的玩家经验；(E) 单一经验或推测。
> **方法限制（必读）**：本次核查**没能在线重新打开任何来源**。WebFetch 访问 patents.google.com 和 www.gamereactor.eu 都报域名解析失败（不在会话允许的网络域名内），按工具提示没有再试其他站点；WebSearch 在本轮一开始就报告共享额度已用完。因此下面的核对依据只有三类，并在每条里写明用的是哪一类：
> ①【姊妹核查】本会话早些时候 `intl_community_核查.md`、`cn_community_核查.md` 用 WebSearch 摘要核对过的同一来源（属于二手核对）；
> ②【背景知识】核查者截至 2026-06 的背景知识，没有在线复核，只用于书目、号码、日期这类可查证的事实；
> ③【本次重算】用 Python 独立重算或重写模拟，结果可复现。
> 需要放行 patents.google.com、gamereactor.eu、konami.com、arxiv.org、dl.acm.org 后，才能逐字核对原文。
> **网易国服检查**：通读全文，没有发现网易国服材料混入。笔记中的"手游"材料只有 Change.org 的旧 PES Mobile 请愿，它属于 Konami 国际版。

---

## 一、已核实

| # | 笔记中的说法 | 核对依据 | 结论 |
|---|---|---|---|
| 1 | Gamereactor 2026-02-13 报道：Konami "reiterates that there is no 'scripting' in eFootball"，出自 Kimura 和 Taya 的访谈 | 【姊妹核查】intl_community_核查 第一节 #1：检索摘要与标题一致；官方 X 于 2026-02-04 发布 Spotlight Interview（[帖子](https://x.com/play_eFootball/status/2019007136819519702)，本次按帖子 ID 换算为 2026-02-04 11:16 UTC） | **属实（二手）**。【跨平台，含手游】。A 级只能证明"Konami 公开否认"这件事本身，不能证明系统局不存在。人名、头衔和访谈形式要更正，见第二节 #1、#4。 |
| 2 | 没有找到 Konami 名下"按连胜或胜负在 PvP 中暗调能力值或匹配"的专利 | 【姊妹核查】cn_community_核查 #15 独立检索 Konami 专利，也只找到公开的能力更新和 rating 计算专利（见第三节 #6） | **结论一致**。这是检索没有命中（阴性结果），不能当作"Konami 没有此类机制"的证据。原笔记 Inferences 第 1 条已经说明了这一点，表述正确。 |
| 3 | EA DDA 专利族 US10286323B2 和续案 US10807004B2，发明人含 Navid Aghdaie，最早在 2016 年申请 | 【背景知识】US10286323B2 授权于 2019-05-14，续案 US10807004B2 授权于 2020-10-20（都是周二，符合 USPTO 周二授权的惯例）；该族首次公开号为 US20170259177A1（2017-09-14）。【姊妹核查】cn_community_核查 #17：Zaplat 也把 US20170259177A1 归于 EA | **号码、权利人和年份属实**（没有在线重开）。权利要求原文没有核对。与 eFootball 无关（C）。 |
| 4 | EA 2019-06 承诺 FIFA 不使用该 DDA 专利 | 【姊妹核查】intl_community_核查 #9：2019 年 6 月，EA 社区经理 Corey Andress 在官方论坛表态，法语报道日期为 2019-06-19 | **属实**。行业背景，与 eFootball 无关。 |
| 5 | 2020-11 加州原告（领衔原告 Zajonc）起诉 EA，指控 DDA 诱导开包；EA 声明"never was in FIFA, Madden or NHL, and never will be"，并称不会在在线多人模式里用 DDA 让玩家占优或吃亏 | 【姊妹核查】intl_community_核查 #11 与第二节 #7；【背景知识】案名为 Zajonc et al. v. Electronic Arts（北加州联邦地区法院） | **属实**。撤诉日期需要更正，见第二节 #8。 |
| 6 | EOMM：Chen, Xue, Kolen, Aghdaie, Zaman, Sun, Seif El-Nasr，WWW 2017，arXiv 1702.06820，由技能模型、流失预测和图匹配三部分组成 | 【姊妹核查】intl_community_核查 #10；【背景知识】WWW 2017 论文集第 1143–1150 页，和笔记里的 proceedings 链接 p1143 一致 | **书目和框架描述属实**。实验数字有问题，见第二节 #10。C 级，与 eFootball 无关。 |
| 7 | Activision US 9,789,406《System and method for driving microtransactions in multiplayer video games》2017-10-17 授权；发言人称其为"exploratory patent… has not been implemented in-game"；Bungie 表示《命运2》未使用 | 【背景知识】授权日 2017-10-17 是周二，标题和发言人原话与背景知识一致 | **属实**（没有在线重开）。手段是"匹配层安排对手"，不改属性，原笔记对它的定性正确。行业背景。 |
| 8 | US 8,475,274 被检索摘要归给 EA，这一点很可能有误 | 【背景知识】这个号码通常归于 Sony Computer Entertainment（DDA 方法专利），本次没有在线核实 | **原笔记的警示正确**。报告不要把它当 EA 专利，也不要用它推论任何足球游戏。 |
| 9 | Hunicke (2005) ACE 2005，DOI 10.1145/1178477.1178573 | 【背景知识】书目和 DOI 正确 | **书目属实**。内容的适用范围需要收窄，见第二节 #11。 |
| 10 | Miller & Sanjurjo (2018) Econometrica 86(6):2019–2047，doi:10.3982/ECTA14943 | 【背景知识】书目正确 | **属实**。可以补充一个与本题直接相关的推论，见第三节 #11。 |
| 11 | 每局胜率 50% 时：100 局内至少一次 5 连败 81.0%，至少一次 6 连败 54.6%；50 局内至少一次 5 连败 55.2%；30 局内至少一次 4 连败 63.9% | 【本次重算】用精确动态规划计算，得 81.01% / 54.61% / 55.19% / 63.91% | **完全复现**。属模型推演，不是游戏数据，所以不评 A–E 等级。 |
| 12 | Elo 式匹配模拟：长期胜率 50.0%；5 连胜后的 5 局胜率为 47.8%（K=16）、45.8%（K=32）、42.4%（K=64） | 【本次重算】独立重写模拟：玩家真实实力固定，对手真实实力＝当前积分＋N(0,60)，按 400 分制逻辑函数计算胜率，200 万局，每次首次达到 5 连胜后取后 5 局。结果为长期 50.0%，连胜后 48.1% / 46.0% / 42.4% | **复现，定性一致**。但"稳定制造"的措辞偏强，见第二节 #14。 |
| 13 | Steam eFootball（app 1665460）和 PES 2020（app 996470）讨论区长期有 handicap/scripting 抱怨帖 | 【姊妹核查】intl_community_核查 #5、#6 | **属实**。这是 PC 版玩家池，标【主机/PC】正确。 |
| 14 | Steam 帖：游戏内 Ping 约 20ms 时，P2P 对局仍有严重输入延迟 | netcode_mitigation.md 引用的是同一帖，intl_community_核查 #7 也核到同类说法（"Against CPU PVE there are no lag nor delay, only in PVP matches"） | **帖子存在**（D/E）。这是 PC 版、P2P 模式，**不能直接套到手游排位**，见第二节 #15。 |
| 15 | REMATCH 公开过固定输入延迟档位 | netcode_mitigation.md 引用官方补丁说明：33ms / 50ms（默认）/ 67ms | **一致**。这是他作，只证明"用固定输入延迟换同步稳定"是足球类联机游戏的常见做法。 |
| 16 | 未找到 FIFA 或 eFootball"脚本/势头"的同行评议统计研究；未找到公开的 eFootball 数据挖掘或逆向报告 | 【姊妹核查】intl_community_核查 第三节 #12 的结论相同 | **一致**（检索阴性）。 |

---

## 二、需更正或降级

1. **Kimura 的名字和头衔**
   - 原说法："总经理木村诚太郎（Seitaro Kimura）"。
   - 问题：汉字名不对；Konami 官方对他的称呼是 General Director，不是"总经理"。
   - 更正：写成"General Director 木村征太郎（Seitaro Kimura）与总制作人田谷淳一（Junichi Taya）"。Taya 的汉字是否为"田谷淳一"，姊妹核查也没有核实，建议只写英文名。依据：【姊妹核查】intl_community_核查 第二节 #1（Famitsu 2022、ASCII 2021 都写作"木村征太郎"；官方 X 写作 "General Director and General Producer"）。

2. **"最近一次公开重申在 2026 年 2 月"**
   - 问题：本次检索覆盖不到 2026-02 到 2026-11 之间是否还有更新的表态。
   - 更正：写成"本次检索到的最近一次公开否认在 2026 年 2 月"。

3. **"标题用 reiterates（重申），说明以前也否认过"**
   - 问题：这只是从标题推出来的。姊妹核查没有找到 2026-02 之前 Konami 否认 scripting 的原文。
   - 更正：写成"Gamereactor 标题用了 reiterates；更早的官方否认原文本次没有找到"（E）。

4. **访谈形式，以及笔记漏掉的官方替代解释**
   - 原说法："在 eFootball Connect 开发日志/问答中"。
   - 更正：官方 X 于 2026-02-04 发布 eFootball CONNECT "Spotlight Interview" 视频，Gamereactor 2026-02-13 报道。Konami 在否认的同时，把玩家的"剧本感"部分归因于**疲劳和技能随比赛时间下降、切换光标（cursor change）修正、连接不稳定、按键输入延迟**。这些归因与用户描述的"接球慢半拍、调整慢"直接相关，原笔记没有写，报告应该补上（A，二手转述，原话未取得）。依据：【姊妹核查】intl_community_核查 #1；official_mechanics.md 第三节。

5. **JP6205138B2"Konami 惯用的标题格式"，以及四件日本专利的使用方式**
   - 问题：
     - "ゲームシステム及びプログラム"是日本游戏公司专利的通用标题写法，很多公司都用【背景知识】，不能当作判断权利人的线索。
     - JP2020127848A 的英文标题"Game system, game control device, and program"倒更接近 Konami 常见的"ゲームシステム、ゲーム制御装置、及びプログラム"格式，但同样不能凭标题定权利人。
     - JP6426315B1 是 B1（没有经过公开就直接授权），常见于走快速审查的公司，权利人很可能不是 Konami，但没有核实。
   - 更正：这四件专利的权利人、日期和权利要求都没有核实。最终报告**不应以"Konami 专利"的名义引用其中任何一件**，最多写成"日本游戏专利中存在公开的让分、积分或连胜保护设计（行业背景，C，未核实）"。另外，第 14 行"IP Force applicant-1898 是 Konami 申请人页"也没有核实，不要当作来源引用。

6. **EA 续案"获批"日期冲突**
   - 原说法：续案"获批"日期更可能是 2021-03-25。
   - 问题：USPTO 每周二发布授权，每周四公开申请。2021-03-25 是**周四**（本次已核对星期），所以这天更可能是 2020-10 提交的续案的**申请公开**（pre-grant publication），而不是"获批"。部分媒体标题写"approved"，很可能写错了。
   - 更正：写成"2021-03 前后公开的是 EA DDA 续案申请，不是授权"。续案后来的授权号，原笔记的检索摘要猜是 US11369880，背景知识也指向 US11369880B2（约 2022-06 授权），但都没有经过 USPTO 核对，报告里应写"待核"。

7. **"In some embodiments, the difficulty adjustment is undetectable by a user"**
   - 问题：这句来自检索摘要或媒体报道，没有核对专利原文。而且"某些实施例"只是专利撰写里的常见措辞，不代表实际产品这样做。
   - 更正：不要把它当作逐字引语，可以写成"据媒体报道，专利说明书提到调整可以不被玩家察觉（未核原文，C，仅针对 EA）"。

8. **EA 集体诉讼的撤诉时间**
   - 原说法："2021 年 3 月初原告撤诉"。
   - 更正：原告于 **2021-02-11** 自愿撤诉，媒体报道集中在 2 月中旬到 3 月初。"原告与 EA 工程师交流后撤诉"是 **EA 自己的说法**，法院没有做实体判决，也没有公开任何技术结论。依据：【姊妹核查】intl_community_核查 第二节 #7。

9. **"EA 的诉讼结果是目前唯一有外部检验的公开案例，结论偏向未在线上对战中使用"（第 2 节 Inferences）**
   - 问题：说法偏强。实际情况只是原告律师看过 EA 自己提供的资料后撤诉，不是独立审计，没有公开报告，而且对象是 EA 的游戏，不是 Konami。
   - 更正：写成"EA 案以原告自愿撤诉结束；EA 称撤诉前向原告提供了技术资料，但没有独立审计或公开的技术结论。它对 eFootball 没有直接证明力"。

10. **EOMM 的实验数字**
    - 问题：
      - "20 轮内留存提升约 15%"和"流失＝至少 8 小时不玩"都没有核实，原笔记已经标了"需复查"。
      - 姊妹核查用检索摘要核到的是：EOMM 在**一轮**匹配后比 SkillMM 多留存约 **0.7%**；"同水平匹配是最差方案"是**有条件**的理论结论（只在 c(Win)+c(Lose) < 2·c(Draw) 时成立）。
    - 更正：最终报告只用定性内容，即"EA 研究人员提出过以降低流失为目标的匹配框架，属于匹配层，不改属性"。不要引用 15% 这个数字。依据：【姊妹核查】intl_community_核查 第二节 #6。

11. **Hunicke (2005) 的适用范围**
    - 问题：该研究是单机 FPS（Half-Life）中**玩家对 AI** 的 PvE 实验，样本量小。"有玩家以为没被调整、其实全程被帮助"这一细节来自检索片段，没有核对原文。
    - 更正：只能写成"在 PvE 中，细微的难度调整可能不被玩家察觉（C）"，不能外推到真人 PvP 天梯。

12. **没能复核的学术来源**
    - 涉及：Springer 2024 系统综述里的 "very noticeable… rubber-banding" 引语、Mi & Gao (2022)、arXiv 2506.02706、arXiv 2603.16136、LoL wiki 用户页。
    - 问题：本次都没能重新打开。arXiv 2603.16136（2026-03）超出背景知识范围，书目信息完全没有核实。
    - 更正：维持 C（LoL wiki 是社区页，应为 D/E），并标"未复核"。LoL 研究只是"同类排位系统胜率收敛到 50%"的旁证，与 eFootball 无关。

13. **热手文献的用法**
    - 问题：原笔记的推论（"凭感觉判断连胜是否被操纵容易出错"）方向正确，但比较抽象。另外，Miller & Sanjurjo"热手确实存在"的结论在学界仍有讨论。
    - 更正：报告不要写成"热手已被证明"。更直接的用法见第三节 #11：玩家用自己有限的战绩统计"连胜后胜率"，即使每局都是 50%，平均也会算出明显低于 50% 的数字。

14. **Elo 推断"单靠匹配也会稳定制造'连胜后就开始输'的体验"**
    - 问题（模拟本身已复现）：
      - 效应**幅度不大**，只比 50% 低约 2–8 个百分点，大小取决于 K 值，以及"对手积分≈对手真实实力"这个假设。
      - eFootball 官方只说 Division 1–3 用 Rating，其他 Division 按点数升级（v3.2.0，2024 年；见 official_mechanics.md）；2026 年 v5.5.0 改为"能力相近的玩家更常配对"，具体公式没有公开。
    - 更正：写成"按积分或段位匹配，会让连胜之后的胜率出现可测的回落倾向（模型推演，eFootball 参数未知）。单靠这一点解释不了'每次连胜后都遇到系统局'的强烈体感，需要叠加个人战绩的统计偏差（第三节 #11）、选择性记忆、疲劳和网络因素"。

15. **第 4 节"手游网络架构未知""取决于当局对手的网络"**
    - 问题：这与 Konami 官方说明冲突。官方《Online PvP Setup》页说：eFootball League（Division 排位）等看重结果的模式使用 **client-server**，"Each user's network connection is not affected by the quality of another user's connection (lag, stability, etc.) or device performance"；只有快速比赛、1v1 好友赛等走 P2P（A，经检索摘要，页面日期约 2025）。
    - 更正：写成"官方称手游排位走 C/S，影响你手感的主要是**你自己到游戏服务器的链路**，本地测速正常不代表这条链路正常。服务器如何处理对手的延迟输入、有没有回滚，官方没有公开（未知）"。Steam 的 P2P 低 Ping 延迟帖属于另一个玩家池、另一种连接模式，只能当旁证（【主机/PC】）。依据：【姊妹核查】intl_community_核查 第二节 #4、第三节 #3；netcode_mitigation.md 第一节。

16. **第 4 节"无 B 级材料"**
    - 结论：对公开资料来说成立，而且应该更严格。
    - 补充提醒：本仓库 `ziru-extra/responses/` 下的文件，按 `ziru-extra/README.md` 的说法是"Server responses written for this port"，即为移植版手写的服务器响应，不是 Konami 服务器的抓包。姊妹笔记里凡是以这些文件为据、标成"B｜手游确认"的匹配或连接参数，进最终报告前都要先确认来源。这条反而加强了原笔记"既没有发现、也没有排除隐藏参数"的结论。

17. **X @EFOOT_INFORMER 帖**
    - 问题：原笔记标"D 级｜跨平台"。这是单帖，本身只是 E，只能代表一种常见感受（D）。帖子没有说明平台。
    - 更正：标为"单帖 E，代表 D 级普遍感受，平台未知"，并补上日期：按帖子 ID 换算为 **2025-12-21**。

18. **Change.org 请愿和 YouTube"92% PROOF"视频**
    - 问题：请愿针对的是旧 PES Mobile 的**人机难度**，与 PvP 天梯无关；视频引用的是 EA 和 Activision 的专利，方法不明。
    - 更正：最终报告不应把两者当证据，最多当作"玩家质疑早就存在"的例子（E）。

19. **"目前唯一的 A 级材料是 Konami 的否认"（第 1 节 Inferences）**
    - 问题：不准确。官方另有几份与本题相关的 A 级文档：v5.5.0 匹配规则（能力相近的玩家更常配对）、Online PvP Setup（排位走 C/S、天线和 Network Graph）、v3.0.0 Form/Condition 规则。它们不直接回答"系统局"，但给出了有官方依据的非脚本解释。
    - 更正：写成"直接回应系统局的 A 级材料只有 Konami 的否认；另有多份官方机制文档提供替代解释（见第三节）"。

20. **"EA 的 DDA 专利改的是游戏难度，报道语境主要是玩家对抗游戏内容"（第 2 节 Inferences）**
    - 问题：没有核对专利全文是否涉及多人对战或匹配。
    - 更正：只能写成"本次没有核对 EA 专利是否涵盖多人对战；EA 公开承诺不在在线多人模式中使用"，不能据此断言该专利与 PvP 无关。

**总体评估**：原笔记对系统局的总结论是"专利这条线两个方向的证据力都有限；隐藏让分既没有被证实，也没有被证伪"，这个判断恰当，没有发现把系统局写成"已证实"或"已证伪"的地方。偏强的说法集中在以下几处：
- 四件日本专利与 Konami 的关联；
- EA 诉讼"外部检验"的分量；
- Elo 效应"稳定制造"的幅度；
- 网络部分忽略了官方的 C/S 说明。

---

## 三、补充发现

| # | 来源 | 日期 | 等级｜平台 | 要点与用途 |
|---|---|---|---|---|
| 1 | [Konami: Online PvP Setup for eFootball™](https://www.konami.com/efootball/en/page/online_match) | 页面约 2025（未取得确切日期） | A（经检索摘要）｜跨平台含手游 | League 等看重结果的模式走 client-server，"Each user's network connection is not affected by the quality of another user's connection"；P2P 只用于快速比赛、1v1 好友赛等；天线显示你到服务器的连接质量，赛后有 Network Graph。这是解释"网正常却慢半拍"最直接的官方依据，用来更正原笔记第 4 节。【姊妹核查】 |
| 2 | [Konami v5.5.0 Information](https://www.konami.com/efootball/en/page/v5/versioninfo_v5-50)；[官方 X 2026-05-28](https://x.com/play_eFootball/status/2060020420867158381) | 2026 | A（经摘要）｜跨平台 | "Changed the matchmaking system so that users around the same level of ability will match together more frequently"，并修改了 Rating 公式。这为第一节 #12 的 Elo 推演提供了官方前提：连胜→积分上升→对手更强。公式没有公开。 |
| 3 | [官方 X：Spotlight Interview](https://x.com/play_eFootball/status/2019007136819519702) | 2026-02-04 | A（访谈，经二手转述）｜跨平台含手游 | 否认偏袒一方的脚本，把体感部分归因于疲劳和技能下降、切换光标修正、连接不稳、输入延迟（见第二节 #4）。逐字原话没有取得。 |
| 4 | [Konami News 5312](https://www.konami.com/efootball/en/topic/news/5312)；[Konami News 5907](https://www.konami.com/efootball/en/topic/news/5907) | 2026-05-28；2026-10-01 | A（经摘要）｜跨平台 | 官方处罚在 eFootball League 中利用漏洞、使用自动化工具或宏、篡改数据的账号。"对手表现异常"有一部分可能来自少数作弊者，这是官方承认存在的替代解释，频率不明。【姊妹核查】 |
| 5 | [Operation Sports：eFootball outlines new efforts to improve online match quality](https://www.operationsports.com/efootball-outlines-new-efforts-to-improve-online-match-quality/) | 约 2025-12 | A（媒体转述官方 X）｜跨平台含手游 | Konami 称某次面向主机和手游的服务器更新降低了输入延迟，并会增设服务器、做赛后连接问卷。说明"慢半拍"至少部分是官方承认、并随服务器调整而变化的技术问题。【姊妹核查】 |
| 6 | [US9485328B2](https://patents.google.com/patent/US9485328)（Konami，2011 年日本优先权）；[US8092300B2](https://www.9235.net/patent/US8092300B2.html)（Konami） | 2012–2016 年授权 | C｜与系统局无直接关系 | 原笔记漏掉的、确属 Konami 的专利：前者是对战后按结果更新能力值，再自动进行下一场；后者是服务器按比赛结果计算玩家 rating。都是**公开**的成长或积分机制，不是"按连胜暗中削弱真人 PvP 数值"。【姊妹核查】cn_community_核查 #15 |
| 7 | [US20170259177A1](https://patents.google.com/patent/US20170259177A1/en)（EA DDA 族首次公开）；续案授权号可能是 US11369880B2（待核） | 2017-09-14 公开 | C｜与 eFootball 无关 | 把原笔记的 EA 专利族补全：Zaplat 引用的"DDA 专利"就是这一族，属于 EA。续案授权号没有核实。【背景知识】【姊妹核查】 |
| 8 | Denisova & Cairns,《The Placebo Effect in Digital Games: Phantom Perception of Adaptive Artificial Intelligence》, CHI PLAY 2015, [ACM DL](https://dl.acm.org/doi/10.1145/2793107.2793109)（DOI 凭背景知识，未在线核对） | 2015 | C｜行业和心理背景 | 实验中告诉玩家"AI 会自适应"（实际并不会），玩家的体验评价就会改变。说明"相信有调整"本身会改变感受，正好补上原笔记第 3 节 Gaps 提到的确认偏差和预期效应缺口。 |
| 9 | Delalleau et al.,《Beyond Skill Rating: Advanced Matchmaking in Ghost Recon Online》, IEEE TCIAIG 4(3), 2012（DOI 10.1109/TCIAIG.2012.2188833，凭背景知识，未在线核对） | 2012 | C｜与 eFootball 无关 | Ubisoft 研究：匹配目标从"实力均衡"扩展到"预测玩家体验"。和 EOMM 一样作用在匹配层，进一步说明业界的"参与度导向"研究集中在**选对手**，而不是改双方属性。 |
| 10 | [EA Forums: FC Mobile "[Info] Handicap, Momentum, Scripting, DDA"](https://forums.ea.com/blog/ea-sports-fc-mobile-game-info-hub-en/info-handicap-momentum-scripting-dda/12392959) | 未取得（FC 时代） | A（仅针对 EA 游戏）｜他作手游 | 另一款足球手游的厂商公开否认 handicap/momentum/scripting/DDA。和 Konami 一样只是否认，没有审计。只作行业对照。【姊妹核查】intl_community_核查 第三节 #11 |
| 11 | **本次模型推演：个人战绩里的"连胜后胜率"偏差**（Miller–Sanjurjo 效应用于本题） | 2026-11-06 | 模型推演（可复现，不是游戏数据） | 设定：每局严格 50% 胜率且相互独立，每名玩家只看自己的一段战绩，计算"3 连胜之后下一局的胜率"，再对许多玩家取平均（每种设定 5 万次模拟）。结果：30 局约 **38.3%**，50 局约 **42.0%**，100 局约 **46.1%**；看"5 连胜之后"，100 局约 36.7%。含义：**即使完全公平，多数玩家翻自己的战绩也会"发现"连胜后更容易输**。这是统计口径造成的假象，能直接解释"很多人都有同感"。（把所有玩家的对局合并起来统计时，这种偏差会消失。） |
| 12 | **本次 Elo 模拟复现**（见第一节 #12） | 2026-11-06 | 模型推演 | 连胜后 5 局胜率 48.1% / 46.0% / 42.4%（K=16/32/64），长期 50.0%。与原笔记一致，幅度说明见第二节 #14。 |
| 13 | 仍然缺失（本次无法补检索） | — | — | ①J-PlatPat 核对（出願人＝株式会社コナミデジタルエンタテインメント；关键词：マッチング、ハンデ、連勝、能力値、補正）；②Konami 关于"排位是否参考连胜、胜率或隐藏 MMR"的官方说明；③任何手游排位 PvP 的真实抓包或大样本对照测试；④Spotlight Interview 的逐字原话。在补齐这些之前，报告只能写成：**系统局既没有被证实，也没有被独立证伪。** |
