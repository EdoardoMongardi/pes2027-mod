# 系统局相关技术：专利、学术研究与数据分析（eFootball 国际服手游视角）

> 范围：只看 Konami 国际服 eFootball 手游（iOS/Android）；未调研网易国服。主机/PC 或旧 PES 资料只在机制可能与手游共用时引用，并标为"主机/PC 来源"。
> 证据等级：(A) Konami 官方确认/文档；(B) 可复现测试、数据集、数据挖掘/逆向；(C) 专利或学术论文（只证明技术存在，不证明本游戏在用）；(D) 广泛流传的玩家经验；(E) 单一经验或推测。
> 适用平台标签：【手游已确认】【跨平台共用】【仅主机/PC】【未知】【与eFootball无关（行业背景）】
> 检索时间：2026-11-06。本次会话网络限制：patents.google.com 与 zaplatgaming.com 无法直接抓取，专利全文和权利要求只能依靠搜索摘要和新闻报道，见各节"Gaps"。

## 1. Konami 是否有根据胜负/连胜调整难度、球员能力值、匹配或让分的专利？有没有证据说明 eFootball 在用？

### Takeaway
本次检索没有找到任何 Konami 专利，内容是"在真人 PvP 足球对战中，根据连胜/胜负暗中削弱或增强一方球员能力值"。找到的几件候选日本专利是公开可见的让分（handicap）设定或胜负积分机制，而且权利人是否为 Konami 还未核实。Konami 的官方立场是否认存在"scripting（脚本）"，最近一次公开重申在 2026 年 2 月（A级，但只是否认，不是第三方审计）。

### Cited Findings
- 【与"系统局"直接相关的 Konami 专利：未找到】用英文和日文检索"Konami + handicap/matching/win streak/能力値補正/連勝"，都没有返回 Konami 名下、讲"按连胜在 PvP 中调整能力值或匹配"的专利。检索工具的结论也是"未找到"。——[检索结果页之一：IP Force Konami 专利登记列表](https://en.ipforce.jp/applicant-1898)（可用作后续人工核查入口）
- 【C级｜权利人未核实｜与eFootball关系未知】JP6205138B2《ゲームシステム及びプログラム》（ゲームシステム及びプログラム，Konami 惯用的标题格式，但权利人未核实）：根据玩家过去的游戏记录（参考游戏数据）计算让分（ハンデ），再按"距最近一次参考游戏的时间、参加的参考游戏数量、参加频率"中的至少一项修正，数据越多越接近默认算法。原文："参考ゲームデータに基づき求めたハンデを、プレーヤの直近の参考ゲームデータからの経過時間、プレーヤが参加した参考ゲームデータの数、プレーヤが参加した参考ゲームデータの頻度の少なくとも１つで補正する"（释义：用经过时间、场数、频率修正根据历史数据算出的让分）。这是类似高尔夫/保龄球的公开让分制度，不是隐藏的胜率控制。——[Google Patents JP6205138B2](https://patents.google.com/patent/JP6205138B2/en)（本会话无法直接打开，内容来自搜索摘要；公开/授权日期未取得）
- 【C级｜权利人未核实】JP2012196424A《ゲーム制御装置、ゲームプログラム、ゲーム制御方法及びゲームシステム》：对战胜负可作为点数，用来更新玩家角色的能力值；选择对手时会显示双方技能水平；文中写到"能力值差越大，胜率可越高"。这是公开的成长/选对手机制。——[Google Patents JP2012196424A](https://patents.google.com/patent/JP2012196424A/en)（搜索摘要）
- 【C级｜权利人未核实】JP2020127848A《Game system, game control device, and program》也出现在 Konami 关键词检索结果中，但摘要没有给出与胜负调整相关的内容。——[Google Patents JP2020127848A](https://patents.google.com/patent/JP2020127848A/en)
- 【C级｜权利人未核实｜行业背景】JP6426315B1《対戦ゲームを提供するためのシステム、方法、及びプログラム》：玩家可以使用道具，把刚输掉的一场从连胜判定中剔除，让输家也能保住连胜；该专利带有匹配处理部。目的是"敗戦したプレイヤであっても継続して楽しむことができる"（让输了的玩家也能继续玩下去），属于留存设计，权利人未显示为 Konami。——[Google Patents JP6426315B1](https://patents.google.com/patent/JP6426315B1/ja)
- 【背景】Konami 的专利诉讼主要在音游（beatmania 系列）和《实况力量棒球》对《赛马娘》（Cygames）的案件，与足球匹配/难度无关。——[note: コナミ VS サイゲームス 特許裁判](https://note.com/tokuchi_ip/n/n1e51ab9a4619)；[pixiv百科: beatmania特許問題](https://dic.pixiv.net/a/beatmania%E7%89%B9%E8%A8%B1%E5%95%8F%E9%A1%8C)
- 【A级（官方否认）｜跨平台（声明针对 eFootball 整体，手游包含在内）】Gamereactor 2026-02-13 报道：在 eFootball Connect 开发日志/问答中，总经理木村诚太郎（Seitaro Kimura）和总制作人田谷淳一（Junichi Taya）回答社区问题，Konami "reiterates that there is no 'scripting' in eFootball"（重申 eFootball 没有"脚本"）。标题用"reiterates（重申）"，说明以前也否认过。本次没有取得原话全文。——[Gamereactor, 2026-02-13](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)
- 【二手来源】一篇 2026 年的调查文章把这次表态概括为 Konami 否认"刻意编程操纵比赛、偏袒任何一方"，同时提醒这不是独立的源代码或服务器审计。——[zaplat: Is eFootball Scripted? 2026 Evidence Investigation](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)（本会话无法打开原文，内容来自搜索摘要）
- 【D/E级｜手游（旧 PES Mobile）】Change.org 请愿《Fix scripting of higher difficulty in PES Mobile》：抱怨高难度下（对 AI）像是被让分/削弱，而不是更难。请愿针对人机难度，不是 PvP 天梯，日期未取得。——[Change.org 请愿](https://www.change.org/p/konami-fix-scripting-of-higher-difficulty-in-pes-mobile-we-want-a-tough-challenge-not-a-handicap)
- 【E级】YouTube 视频《KONAMI's 92% PROOF RIGGING EXPOSED》声称 Konami 操纵比赛，但据检索摘要，它引用的是 EA 和 Activision 的专利，不是 Konami 的，方法不明。——[YouTube](https://www.youtube.com/watch?v=6hehK9nwB-I)

### Inferences
- 没找到 Konami 专利不能证明 Konami 没有这类机制：公司完全可以把内部算法当商业秘密，不申请专利。反过来，就算有专利也不能证明在用。因此专利这条线对"系统局是否存在"的证据力有限，两个方向都是这样。
- 找到的日本候选专利（JP6205138B2、JP2012196424A）都是公开、可见的让分或成长机制，和用户描述的"隐藏削弱能力值"在性质上不同。
- 目前唯一的 A 级材料是 Konami 的否认。和 EA 不同，Konami 没有经过诉讼或向原告技术披露这类外部检验（见第2节），所以这份否认的可验证性比 EA 的弱。

### Gaps
- 本会话无法访问 patents.google.com，无法核实 JP6205138B2、JP2012196424A、JP2020127848A、JP6426315B1 的申请人/权利人、日期和权利要求原文。建议人工在 J-PlatPat（出願人＝株式会社コナミデジタルエンタテインメント，关键词：マッチング／ハンデ／連勝／能力値／補正）或 Google Patents（assignee:Konami）复查。
- 没有取得 2026-02 eFootball Connect 访谈的原话，也不知道 Konami 是否专门回答过"天梯 Division 连胜后是否调整匹配"。
- 没有找到 Konami 官方发布的 Division 匹配算法文档（例如是否只按 Division/积分匹配、是否考虑战绩或队伍实力）。

## 2. EA 的 DDA 专利族、EOMM 论文、Activision 匹配专利（及其他手游胜率控制专利简述）

### Takeaway
EA 确实拥有"根据预测的留存/流失调整游戏难度"的专利族，并发表过以降低流失为目标的匹配框架 EOMM（2017）。Activision 拥有"引导内购的匹配"专利（2017）。三者都只证明技术存在（C级）。EA 和 Activision 都公开否认在线对战中使用这些技术；EA 的 FIFA"DDA"集体诉讼在原告与 EA 工程师交流后被原告撤回（2021）。这些都与 eFootball 无直接关系。

### Cited Findings
- 【C级｜与eFootball无关】EA 的 DDA 专利族：Google Patents 上有 US10286323B2《Dynamic difficulty adjustment》和续案 US10807004B2，发明人之一是 Navid Aghdaie（也是 EOMM 论文作者）。媒体报道说该族最早在 2016 年申请，2020 年 10 月又提交了续案。——[Google Patents US10286323B2](https://patents.google.com/patent/US10286323)；[US10807004B2](https://patents.google.com/patent/US10807004)；[EssentiallySports](https://www.essentiallysports.com/esports-news-electronic-arts-patent-on-dynamic-difficulty-adjustment-system-intends-to-keep-gamers-engaged-for-longer/)
- 【C级】技术内容（来自报道和检索摘要，不是核实过的权利要求原文）：系统生成"a game retention prediction model"（游戏留存预测模型），预测玩家的游戏时长或留存概率，再据此确定难度调整值、修改游戏运行。检索摘要中类似权利要求的片段为："accessing the set of input data… determining the retention probability… identifying the adjustment value… modifying the execution of the video game"。专利说明书写有"In some embodiments, the difficulty adjustment is undetectable by a user"（某些实施例中，难度调整玩家察觉不到）。——[TechRadar: EA resubmits patent](https://www.techradar.com/news/ea-resubmits-patent-for-dynamic-difficulty-adjustment)；[TechSpot](https://www.techspot.com/news/89238-ea-patent-dynamic-difficulty-adjustment-system-has-approved.html)；[OPAttack](https://opattack.com/ea-patent-dynamic-difficulty-adjustment/)
- 【日期冲突】续案"获批"日期：检索摘要说是"Thursday, 25 March"。2021-03-25 是星期四，2020-03-25 是星期三，所以更可能是 2021 年 3 月（TechSpot 报道也在 2021 年前后）。报道日期之间有冲突，以专利局记录为准。——[TechSpot](https://www.techspot.com/news/89238-ea-patent-dynamic-difficulty-adjustment-system-has-approved.html)；[NME](https://www.nme.com/news/gaming-news/ea-granted-patent-dynamic-difficulty-adjustment-technology-2915676)
- 【EA 官方表态，2019】2019-06 媒体报道 EA 承诺 FIFA 不使用该 DDA 专利。——[TheSixthAxis, 2019-06-19](https://www.thesixthaxis.com/2019/06/19/fifa-20-dynamic-difficulty-patent-ea-promise-they-arent-using/)；[GamesRadar: EA 否认 FIFA 19 有动态难度](https://www.gamesradar.com/fifa-19-dynamic-difficulty/)
- 【诉讼，2020-11 至 2021-03】2020 年 11 月，加州三名原告（领衔原告 Jason Zajonc）提起集体诉讼，指控 EA 在 FIFA Ultimate Team、Madden、NHL 中使用 DDA 诱导开包。2021 年 3 月初原告撤诉，EA 称是在向原告提供"detailed technical information and access to speak with our engineers"（详细技术资料和与工程师交流的机会）之后。EA 声明："While EA does own a patent for DDA technology, that technology never was in FIFA, Madden or NHL, and never will be"，以及"We would not use DDA technology to give players an advantage or disadvantage in online multiplayer modes in any of our games"（不会在任何游戏的在线对战模式中用 DDA 让玩家占优或吃亏）。——[PC Gamer](https://www.pcgamer.com/fifa-dynamic-difficulty-lawsuit-dropped-after-plaintiffs-talk-to-eas-engineers/)；[Slate, 2020-12](https://slate.com/technology/2020/12/electronic-arts-fifa-madden-nhl-lawsuit-dynamic-difficulty-adjustment.html)；[VGC](https://www.videogameschronicle.com/news/ea-loot-box-lawsuit-dropped-after-company-convinces-lawyers-it-doesnt-adjust-difficulty/)；[Wccftech](https://wccftech.com/dynamic-difficulty-adjustment-lawsuit-dismissed-ea-vows-never-to-use-the-technology/)
- 【C级｜EOMM，2017】Chen, Xue, Kolen, Aghdaie, Zaman, Sun, Seif El-Nasr，《EOMM: An Engagement Optimized Matchmaking Framework》，WWW 2017（arXiv 1702.06820，2017-02）。框架由三部分组成：技能模型、流失（churn）预测模型、图匹配模型（最小权完美匹配 MWPM）。目标是让全体玩家一局后的总流失风险最小，相当于"安排谁和谁打"。论文证明传统的技能匹配（SBMM）是 EOMM 的特例；模拟中 EOMM 优于随机匹配和 SBMM。二手摘要中"20 轮内留存提升约15%"的数字、"流失=至少8小时不玩"的定义未在原文核实，需复查。——[arXiv 1702.06820](https://arxiv.org/pdf/1702.06820)；[WWW 2017 proceedings](https://archives.iw3c2.org/www2017/proceedings/proceedings/p1143.pdf)
- 【C级｜Activision，2017】US 9,789,406《System and method for driving microtransactions in multiplayer video games》，2015 年申请（申请号 14/712,566），2017-10-17 授权（早期公开号 US20160005270A1）。内容：把未付费玩家和已购买某道具的玩家匹配在一起，以刺激购买；购买后把玩家匹配到该道具特别强势的对局里。Activision 发言人："This was an exploratory patent filed in 2015 by an R&D team working independently from our game studios. It has not been implemented in-game."（2015 年由独立于游戏工作室的研发团队提交的探索性专利，未在游戏中实装）。Bungie 也表示《命运2》未使用。媒体指出声明没有承诺将来不用。——[PCGamesN](https://www.pcgamesn.com/activision-microtransaction-matchmaking-patent)；[Kotaku](https://kotaku.com/activision-patents-matchmaking-that-encourages-players-1819630937)；[Game Informer, 2017-10-17](https://gameinformer.com/b/news/archive/2017/10/17/activision-patent-encourages-microtransaction-spending-through-matchmaking.aspx)；[USPTO 9789406](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/9789406)；[Google Patents US20160005270A1](https://patents.google.com/patent/US20160005270A1/en)
- 【C级｜权利人未核实】相关匹配专利还有 US10857468《Systems and methods for dynamically weighing match variables to better tune player matches》和 US20160001181A1《Matchmaking system and method for multiplayer video games》。——[USPTO 10857468](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/10857468)；[Google Patents US20160001181A1](https://patents.google.com/patent/US20160001181A1/en)
- 【行业背景，简述｜权利人未核实】美国专利 US11712631 / US12053705 / US12427429《System and method for matching users of client applications》描述一种"Game Skill Level Matcher"：玩家长期连胜或连败时，用连胜/连败阈值和胜率上下限调整对手强度。这说明"按连胜调整对手"作为技术方案有人申请过专利，但检索摘要没有给出权利人。——[USPTO 11712631](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/11712631)；[USPTO 12053705](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/12053705)；[USPTO 12427429](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/12427429)
- 【C级｜权利人未核实】另一件 DDA 专利 US 8,475,274《Method and apparatus for dynamically adjusting game or other simulation difficulty》也常被引用。检索摘要把它归为 EA 的专利，但这一点未核实，很可能是误归属，不要当作 EA 专利引用。——[USPTO 8475274](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/8475274)

### Inferences
- EOMM 和 Activision 专利的手段都是"选谁当你的对手"（匹配层），不改双方球员属性。EA 的 DDA 专利改的是"游戏难度"，报道语境主要是玩家对抗游戏内容；EA 已公开承诺不在在线对战中这样用。所以就公开文献而言，"在真人 PvP 中暗中削弱一方球员属性"这种做法，本次没有找到对应的专利或论文。这是基于有限文本（摘要/报道）的推断。
- 如果某个游戏采用 EOMM 式的流失导向匹配，玩家能感受到的效果是"连胜后对手变强、对手阵容更克制"，而不是"自己球员的数值被改"。这和用户描述的一部分（遇到强队）相容，和另一部分（自己球员反应慢、传球慢、接球慢半拍）对不上。后者更像网络延迟或判定问题，见第4节。
- EA 的案例说明，大厂确实研究过"以留存为目标"的难度和匹配技术，玩家怀疑有现实基础。但 EA 的诉讼结果（原告看过技术资料后撤诉）是目前唯一有外部检验的公开案例，结论偏向"未在线上对战中使用"。

### Gaps
- 任务简报中的"US 2016/0067612"无法核实对应哪件专利：检索没有返回该号码的页面。已确认的 EA DDA 族成员是 US10286323B2 和 US10807004B2；2020-10 续案的授权号（检索摘要猜测为 US11369880）未核实。
- 无法打开 patents.google.com，EA 专利权利要求原文、优先权日、是否提到多人对战/匹配都未直接核对。
- EOMM 原文中的具体实验数字（留存提升幅度、所用数据来自哪款 EA 游戏）未核实。

## 3. 学术与数据研究：玩家能否分辨"脚本"和统计效应（DDA 可察觉性、Elo 类匹配下胜率收敛约50%、连胜/升段后对手变强、回归均值、热手、确认/负面偏差）

### Takeaway
没有找到任何针对 FIFA/eFootball"脚本感"的同行评议研究。可用的证据是：(1) DDA 文献认为粗糙的橡皮筋式调整很容易被察觉，精细的调整玩家往往察觉不到；(2) 基于技能/积分的匹配本身就会让长期胜率收敛到约50%，连胜后对手变强是排位系统的数学结果，不需要"系统局"；(3) 公平条件下长连败非常常见；(4) 热手研究提醒，凭感觉判断"连胜/连败有没有被操纵"在统计上很容易出错，两个方向都会错。

### Cited Findings
- 【C级】Hunicke (2005)《The Case for Dynamic Difficulty Adjustment in Games》（ACE 2005，ACM DL）专门研究了 DDA 是否提升表现、玩家能否察觉、是否影响乐趣。检索片段显示，有玩家以为自己没有得到调整，实际上全程都在被帮助，说明玩家对调整的察觉并不可靠。这一点来自片段，需要核对原文。——[ACM DL: Hunicke 2005](https://dl.acm.org/doi/pdf/10.1145/1178477.1178573)
- 【C级】2024 年 DDA 系统综述（Multimedia Tools and Applications）：经典启发式 DDA 调难度有效，但常常"very noticeable, e.g. as 'rubber-banding'"（非常明显，例如橡皮筋效应）。——[Springer 2024 系统综述](https://link.springer.com/article/10.1007/s11042-024-18768-x)
- 【C级】Mi & Gao (2022, ICGA Journal) 提出赛车游戏的自适应橡皮筋系统 ARBS，报告它更自适应、体验更好。前提是传统橡皮筋会被玩家明显感知并觉得不公平。——[SAGE: Adaptive rubber-banding system, 2022](https://journals.sagepub.com/doi/10.3233/ICG-220207)
- 【行业观点】Game Developer 文章认为橡皮筋机制容易被察觉，其他形式的 DDA 可以隐藏，并主张 DDA 要"看不见"才有效。——[Game Developer: The Secrets of DDA](https://www.gamedeveloper.com/design/more-than-meets-the-eye-the-secrets-of-dynamic-difficulty-adjustment)
- 【C级｜与eFootball无关（LoL）】arXiv 2506.02706（2025，英雄联盟数据）：匹配算法随胜负把玩家上调或下调，因此多数玩家胜率约50%。arXiv 2603.16136（2026-03）：LoL 和 TFT 平均胜率都在50%附近，作者归因于技能匹配追求势均力敌。——[arXiv 2506.02706](https://arxiv.org/pdf/2506.02706)；[arXiv 2603.16136](https://arxiv.org/pdf/2603.16136)
- 【社区数据】英雄联盟 wiki 用户页：即使每局胜率严格为50%，100局中出现5连败的概率约80%。Riot 的立场（转述）：只有一个排位队列，按 MMR 和位置匹配，没有"败者队列/胜者队列"。——[LoL Wiki: Losers queue and winners queue](https://wiki.leagueoflegends.com/en-us/User:Uxu/Losers_queue_and_winners_queue)
- 【E级（单一未评审分析，与结论相反）】一篇 Substack 文章分析约4,820名玩家，称"公平对局"胜率仅35.4%而赛季胜率52.7%，并据此认为存在强制50%机制。方法未经同行评议，无法核实。——[Substack: Losers Queue Is Real — Just Not What You Think](https://lorenzocons.substack.com/p/losers-queue-is-real-just-not-what)
- 【C级】热手：Miller & Sanjurjo (2018, Econometrica 86(6):2019–2047, doi:10.3982/ECTA14943) 指出，用"连中之后的命中率"衡量热手，在有限序列中有系统性向下偏差（流选择偏差）。修正后，Gilovich, Vallone & Tversky (1985) 的经典结论被反转（存在一定热手效应）。评论也强调，这不代表人们不会犯热手谬误。——[Econometrica 2018](https://econometricsociety.org/publications/econometrica/2018/11/01/surprised-hot-hand-fallacy-truth-law-small-numbers)；[arXiv 1902.01265](https://pdf.arxiv.org/pdf/1902.01265)；[R-bloggers 评论, 2019-03](https://www.r-bloggers.com/2019/03/an-interesting-subtlety-of-statistics-the-hot-hand-fallacy-fallacy/)
- 【D/E级（主机，FIFA）】FUT 社区文章认为脚本论缺乏证据，把"被削弱感"归为认知偏差，并引用自评调查"约70%玩家认为自己高于平均水平"（虚幻优越感）。属于立场性文章，约5年前，非严谨研究。——[fifauteam.com: FIFA Scripting, Handicapping and Momentum](https://fifauteam.com/fifa-scripting-handicapping-momentum/)
- 【D/E级（主机，FIFA 13，2012）】博客描述 FIFA 13 的"比赛强度/势头"模型随比分和时间变化，但认为游戏不决定胜负，没有数据。另有 2019 年文章提到 FIFA 早年显示过势头条（momentum bar），后来移除。——[fifa13scripting blog, 2012-10](http://fifa13scripting.blogspot.com/2012/10/fifa-13-scripting.html)；[RealSport101, 2019](https://realsport101.com/article/fifa-20-scripting-gameplay-fut-ultimate-team-career-mode)
- 【本次检索结论】未找到任何针对 FIFA 或 eFootball"脚本/势头"的同行评议统计研究，网上所谓的"证据"多为没有方法说明的视频。——[示例: YouTube "TOP 10 SCRIPTING PROOF" (FIFA 19)](https://www.youtube.com/watch?v=Rm0FZ_Akm8c)

### Inferences
- 【模型推演（可复现，不是游戏数据）】我用 Python 做了精确计算和模拟。连败概率（每局胜率=50%）：100局内至少一次5连败 = 81.0%，至少一次6连败 = 54.6%；50局内至少一次5连败 = 55.2%；30局内至少一次4连败 = 63.9%。和上面 LoL wiki 的约80%一致。
- 【模型推演】Elo 式匹配模拟：玩家真实实力固定，系统按当前积分找实力相近的对手（对手实力 = 当前积分 ± 60 的正态噪声），共200万局。长期胜率 = 50.0%。但在"刚打出5连胜之后的5局"里，胜率降到 47.8%（K=16）、45.8%（K=32）、42.4%（K=64）。原因是连胜让积分暂时高于真实实力，接下来匹配到的对手更强。也就是说，即使完全没有"系统局"，单靠"按段位/积分匹配"也会稳定制造"连胜后就开始输"的体验；积分变化越剧烈，这种效应越强。
- 由此推断，用户描述中的"连胜后对手阵容变强、对手打得像神"，在 Division 积分制下至少有一部分可以用匹配数学和回归均值解释（推断，适用于【跨平台】排位逻辑；eFootball 的具体匹配参数【未知】）。但这类统计效应不能解释"自己球员接球慢半拍、进攻三区卡顿、传球极慢"这类操作层面的症状，后者应该放到网络延迟（见第4节）或判定机制里另外分析。
- 文献显示粗糙的橡皮筋会被察觉，而精细的调整玩家察觉不到。所以凭主观感觉，既证明不了有系统局，也排除不了。真正有说服力的是可复现的数据，例如记录大量对局（连胜后对手队伍总评、己方传球/接球的帧级延迟、Ping/丢包）做对照。

### Gaps
- 本次没有取得确认偏差、负面偏差（"输球记得更清楚"）在电子游戏情境中的专门实证研究。可供写作者参考、但本次未核验的经典文献：Baumeister et al. (2001)《Bad Is Stronger Than Good》。
- 没有找到 eFootball 手游的公开对局数据集或社区统计（例如"连胜后对手 Division/阵容评分是否系统性上升"的记录）。
- 没有找到针对体育类游戏"脚本感/势头感"的心理学实验。

## 4. PES/eFootball 的数据挖掘、逆向工程与网络代码：有没有发现或排除隐藏让分参数？

### Takeaway
本次没有找到任何公开的 eFootball（包括手游）或 PES 数据挖掘/逆向成果，既没有发现，也没有排除"按连胜调整能力值"的隐藏参数（B级证据空白）。唯一相关的技术线索来自主机/PC 版玩家：P2P 对局在显示低 Ping 时仍有严重输入延迟（D/E级）。这和用户描述的"接球慢半拍、进攻时卡顿"更吻合，但手游网络架构未知。

### Cited Findings
- 【无 B 级材料】面向"eFootball/PES datamine hidden parameter momentum handicap"的检索没有返回任何数据挖掘或逆向报告。——（检索返回内容见下列网络相关来源；无专门数据挖掘页面可引用）
- 【D/E级｜仅主机/PC（Steam 版）】Steam 讨论帖《Severe input delay in P2P matches despite low ping》：游戏内 Ping 约20ms 时，P2P 对局仍有严重输入延迟。发帖者推测原因是 Ping 指示器没反映的丢包/抖动、P2P 同步不佳或延迟补偿不均衡。另有帖子推测评分差异影响谁感到延迟，属于推测。——[Steam: eFootball P2P input delay](https://steamcommunity.com/app/1665460/discussions/0/804595075203018750/)；[Steam 讨论](https://steamcommunity.com/app/1665460/discussions/0/3824158244542838410)
- 【背景（通用网络代码知识）】P2P 锁步设计通常要等所有端的输入到齐才推进一帧，端与端之间的延迟会直接进入模拟。回滚类方案（如 GGPO）先预测远端输入再纠正，很多游戏在回滚之上再加固定输入延迟来掩盖纠正。——[Wikipedia: Netcode](https://en.wikipedia.org/wiki/Netcode)；[Wikipedia: GGPO](https://en.wikipedia.org/wiki/GGPO)
- 【D/E级｜跨平台（主机/PC 为主）】Steam eFootball（app 1665460）、PES 2020（app 996470）讨论区长期有"handicap/scripting"抱怨帖（2019–2022），没有方法或数据。——[Steam PES 2020 讨论](https://steamcommunity.com/app/996470/discussions/0/1629663905426859457)；[Steam eFootball 讨论](https://steamcommunity.com/app/1665460/discussions/0/3486374957056007672)
- 【D级｜跨平台】X 粉丝账号 EFOOTBALL INFORMER 的帖子：无论射正多少次都进不了球、赢不了（#KonamiScript）。代表常见玩家感受，无数据。——[X: @EFOOT_INFORMER](https://x.com/EFOOT_INFORMER/status/2002802986415001717)
- 【A级（官方否认）｜跨平台】Konami 2026-02 重申没有"scripting"（见第1节）。——[Gamereactor, 2026-02-13](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)
- 【行业类比（非 eFootball）】另一款足球游戏 REMATCH 公开过网络代码细节（约50ms 输入延迟、可选延迟档位），说明足球游戏的"手感慢"可能直接来自设计上的输入延迟和同步方案。但这不是 eFootball，不能当作 eFootball 的参数。——[REMATCH Dev Blog: Netcode 2.0](https://www.playrematch.com/post/dev-blog-netcode-2-0-next-beta)

### Inferences
- 用户描述中的"自己网络正常却在进攻三区卡顿、接球慢半拍、调整慢、传球极慢"，在网络代码层面有现成的非脚本解释：P2P/锁步或延迟补偿结构下，对手一侧或两端之间路径的延迟、抖动、丢包会让本方操作被延后或纠正，而本方 Ping 显示可能正常。这只是推断：手游是 P2P、服务器中转还是回滚方案，本次【未知】。它也能解释为什么这种感觉是一阵一阵、按对局出现的（取决于当局对手的网络）。
- 由于没有任何数据挖掘成果，"隐藏让分参数"在证据上处于既未证实、也未证伪的状态。不能把它当既成事实，也不能断言不存在。

### Gaps
- 没有找到任何 eFootball 手游（iOS/Android）客户端文件、协议或网络架构的公开逆向分析，也没有 PES 时代"隐藏势头/让分参数"的数据挖掘报告。Evo-web、GitHub 上的 PES 改档社区本次未检出相关内容；PES 改档多针对 PC 单机文件，难以触及在线匹配和服务器逻辑。
- 手游的对战网络架构（P2P、专用服务器还是混合）、输入延迟和延迟补偿策略都没有官方或可复现资料，这是解释"卡顿/慢半拍"最关键的缺口。
- zaplatgaming.com 和 patents.google.com 在本会话中无法访问（需在会话网络设置中放行才能抓取原文）。
