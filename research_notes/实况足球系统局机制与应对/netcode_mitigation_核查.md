# 核查报告：netcode_mitigation.md（国际服手游联网同步、设备性能与降低“系统局”体感的手册）

> 核查日期：2026-11-06。范围只限 Konami 国际服 eFootball 手游（iOS/Android）。网易国服不核查、不引用。原笔记没有改动。
> 证据等级沿用原笔记：A＝Konami 官方；B＝可复现测试/抓包/数据挖掘；C＝专利/论文/他作官方文档（只证明技术存在）；D＝广泛流传的玩家经验；E＝单一说法/推测。平台标注：【手游确认】【跨平台（含手游）】【主机/PC】【未知】。
>
> **方法限制（先读）**
> 1. **本次没能重新打开任何网页原文。** WebFetch 打开 `www.konami.com` 和 `steamcommunity.com` 都失败（域名解析失败，不在会话允许的网络域名内），按工具提示没有再试其他站点。WebSearch 的本轮额度（200 次，所有子任务共用）已经用完，一次也没能搜索。要逐字核对，需要在会话网络设置里放行 www.konami.com、steamcommunity.com、x.com、operationsports.com、gamereactor.eu、zaplatgaming.com 等域名，或在新一轮对话里补搜。
> 2. 所以本报告用的核对办法只有四种，下文逐条写明用了哪种：
>    - **本地独立核对**：直接读取本仓库 `ziru-extra/` 下的文件，包括把 `GetServerEnv.json` 里 Base64 编码的 `proto_opt`、`onsys_opt` 解码；查 git 提交记录。
>    - **日期独立换算**：用 X 帖子 ID 的 Snowflake 编码换算发帖时间；把 Unix 时间戳换成日期；按“约 N 天前”倒推日期。
>    - **交叉核对（二手）**：和姊妹笔记及其核查报告里其他研究员拿到的搜索摘要对照（`official_mechanics_核查.md`、`intl_community_核查.md`、`cn_community_核查.md`、`patents_research.md`）。这只是一致性检查，不是逐字核对。
>    - **内部逻辑检查**：版本号、日期、平台、推理链是否自洽。

---

## 一、已核实

| # | 笔记中的说法（行号） | 核对来源与办法 | 结论 |
|---|---|---|---|
| 1 | 官方把对战分成 P2P（快速比赛、1v1 好友赛等）和 C/S（eFootball League、挑战/排名活动等），结果重要的模式用 C/S，“to ensure a stable connection and prevent cheating”（第 17 行） | [Konami: Online PvP Setup](https://www.konami.com/efootball/en/page/online_match)；交叉核对 `official_mechanics_核查.md` 第一节第 15 条、`intl_community_核查.md` 第二节第 4 条 | **属实（二手）**。三份笔记的摘要一致。页面日期：2026-11-06 倒推 569 天是 **2025-04-16**，正好是 v4.4.0（2025-04-17，为 C/S 对战加入天线显示）上线前一天，说明这页很可能是随 v4.4.0 发布或改版的。C/S 清单里的“合作好友房”“主题活动”“Strike Arena”等细项，别的笔记没有提到，未交叉核实。A｜跨平台（含手游）。 |
| 2 | 服务器设在各城市的数据中心，系统会结合对手位置和网络基础设施选服务器，有时更远的服务器反而更好（第 18 行） | 同上；`official_mechanics_核查.md` 第三节第 1 条 | **属实（二手）**。小更正：这句话**英文页也有**，不是简中页独有的“补充”。简中页是 Konami 国际服的中文页，不是网易。A｜跨平台（含手游）。 |
| 3 | 天线只在 C/S 模式显示，反映“你到游戏服务器”的连接，双方天线可以不同；赛后有网络图表（第 19、56、155 行） | 同上；`intl_community_核查.md` 第三节第 3 条 | **属实（二手）**。A｜跨平台（含手游）。 |
| 4 | Gamereactor 报道 v4.4.0 加入天线和赛后延迟图表；“v4.4.0 推测在 2025 年春季”（第 20 行） | `intl_community_核查.md` 第一节第 8 条（MMOBomb、VGC、Konami v4.4.0 版本说明） | **属实，日期可以精确到 2025-04-17**。注意 v4.4.0 的跨平台对战只含 PlayStation、Xbox、Steam、Windows，**不含手游**（见第三节第 6 条）。A（经媒体转述）｜跨平台。 |
| 5 | 官方 X 于 2026-05-19 宣布 Div 1–3 测试 Smart Assist 分类匹配；选 “Same setting as mine” 可能匹配更久、可能配到连接不稳定的对手（第 120 行） | [官方 X 2056702522933514339](https://x.com/play_eFootball/status/2056702522933514339)；Snowflake 换算；`official_mechanics_核查.md` 第一节第 1 条 | **日期独立核实**：2026-05-19 11:44 UTC。英文引文与姊妹笔记一致（二手）。A｜跨平台（含手游）。它的实际收益见第二节第 13 条。 |
| 6 | v5.5.0：“users around the same level of ability will match together more frequently”，并改了 League 积分公式和匹配逻辑（第 121 行） | [Konami v5.5.0](https://www.konami.com/efootball/en/page/v5/versioninfo_v5-50)；`official_mechanics_核查.md` 第一节第 3 条 | **两份笔记的摘要一致（二手）**。v5.5.0 包含 5-19 才宣布的 Smart Assist 测试，所以发布时间应在 **2026 年 5 月下旬至 6 月初**（推断）。官方 X 2026-05-28 的帖子（ID 2060020420867158381，已换算）也列了同样的匹配调整。A｜跨平台。 |
| 7 | Operation Sports 据 Konami 的 X 帖报道：增设全球服务器、已降低输入延迟、赛后问卷（第 22、156 行） | [Operation Sports](https://www.operationsports.com/efootball-outlines-new-efforts-to-improve-online-match-quality/)；`intl_community_核查.md` 第三节第 5 条、`cn_community_核查.md` 第三节第 5 条 | **属实（二手）**。原笔记漏了一个关键细节：降低输入延迟的是一次“10 月 29 日、**面向主机和手游**”的服务器更新。文章约 2025-12，所以那次更新很可能在 2025-10-29（未核实）。A（经媒体转述）｜跨平台（含手游）。 |
| 8 | Konami 在 eFootball Connect 访谈中否认 scripting（第 23、58 行） | [官方 X 访谈帖 2019007136819519702](https://x.com/play_eFootball/status/2019007136819519702)；Snowflake 换算；[Gamereactor](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/) | **属实（二手）**。访谈帖日期独立核实为 **2026-02-04 11:16 UTC**。Gamereactor 报道按“约 266 天前”倒推为 2026-02-13。原笔记写“这是官方否认，不能当作独立证明”，表述恰当。A 只证明“Konami 公开否认过”。 |
| 9 | `GetServerEnv.json`：天线阈值 30/120/171/209 与 `_cs` 15/50/110/200；`slowdown_judgment_threshold_list [51,25,17]`；`ip_country_code: TUR`；配置时间 2026-09-14（第 27 行） | 本地 [GetServerEnv.json](/home/user/pes2027-mod/ziru-extra/responses/GetServerEnv.json) | **数值全部属实（本地独立核实）**。`date`/`page_timestamp` = 1789384820 = **2026-09-14 11:20:20 UTC**。但文件来源和平台标签要改，见第二节第 1 条；不带 `_cs` 那组叫“P2P 阈值”没有依据，见第二节第 5 条。 |
| 10 | `gamerelay_qualitycheck_list.json`：62 个 game-relay 区域，`quality_threshold_1: 75`、`_2: 175`，`quality`/`last_update` 全为 0，没有中国大陆节点；亚洲最近的是 asia-east2/east1、asia-northeast1/2/3、asia-southeast1/2/3、ap-southeast-5/7（第 28 行） | 本地 [gamerelay_qualitycheck_list.json](/home/user/pes2027-mod/ziru-extra/responses/gamerelay_qualitycheck_list.json) | **全部属实（本地独立核实）**。62 个区域，没有 `cn-` 开头的区域。地址是真实格式的 GCP（`*.bc.googleusercontent.com:5521`）和 AWS（EC2、ELB）主机名。全 0 的质量值来自私服文件，不说明任何问题。 |
| 11 | 配置里有 `MatchCommandBufferingControl`（第 30 行） | 本地 GetServerEnv.json 的 `proto_opt`（Base64 解码） | **存在（本地独立核实）**。原笔记说“没 grep 到”，是因为它藏在 Base64 里。具体内容和“区分 P2P/CS”的说法需要更正，见第二节第 2 条。 |
| 12 | 登录响应里有 `ping_level: PING_LEVEL_BALANCE`、`team_power_limit: TEAM_POWER_NO_LIMIT`、`control_style_filtering: NO`（第 122 行） | 本地 [CmdLogin.json](/home/user/pes2027-mod/ziru-extra/responses/CmdLogin.json) | **字段和取值存在**（本地独立核实，都是顶层字段）。全仓库只出现 `PING_LEVEL_BALANCE` 这一个枚举值，找不到其他档位名。取值的可信度见第二节第 1、14 条。 |
| 13 | 服务器跟踪 `cont_win_num`，有 WIN_STREAK 称号（第 158 行） | 本地 `cmd_get_league_com_info.json`、`ZIRU_GET_VSCOM_GAME_RESULT_TEMPLATE.json`、`CmdGetMyclubCommentaryInfo.json` | **字段属实**。更精确的说法见第二节第 12 条。 |
| 14 | REMATCH 官方补丁说明提供 33/50/67ms 三档固定输入延迟（第 34 行） | [REMATCH Netcode 2.0](https://playrematch.com/post/netcode-2-0-beta-patch-notes-update-1)；交叉核对 `patents_research.md` 第 103 行（另一篇 REMATCH 开发日志：约 50ms 输入延迟、可选档位） | **大意一致（二手）**。33/50/67ms 正好是 60Hz 下 2/3/4 帧，数字自洽。C｜他作，只证明这种做法存在，原笔记的限定语写得恰当。 |
| 15 | Steam 帖 804595075203018750：游戏内 ping 约 20ms，P2P 仍有严重输入延迟（第 33 行） | `patents_research.md` 第 98 行引用同一帖子，摘要一致 | **属实（二手）**，日期未知。E｜主机/PC（Steam 版）。注意这是 P2P 模式，和手游排位（C/S）不是同一种连接。 |
| 16 | zaplat 2026 调查：大量玩家描述“领先后变沉”；认为网络延迟“can impersonate manipulation extremely well”（第 59 行） | [zaplat](https://zaplatgaming.com/articles/is-there-script-in-efootball-2026-investigation?lang=en)；`intl_community_核查.md` 第一节第 3 条 | **文章存在，约 2026-09（二手）**。核心结论“没有独立可复现的技术证据、截至 2026-09 没有公开的独立审计”与姊妹核查一致。笔记里加了引号的英文短语在姊妹核查中没有逐字命中，应改成转述。zaplat 是攻略/粉丝站。E。 |
| 17 | 网易 UU 加速器博客、掘金加速器文：国际服卡顿归因于海外服务器、路由远、丢包抖动（第 87 行） | `cn_community_核查.md` 第一节第 9–11 条 | **属实（二手）**。说的是国际服加速，不是网易国服游戏本身。掘金文的数字是“晚高峰直连 120–200ms、丢包 2–5%；加速后 50–80ms、丢包 0.5–1.5%”，属于 biubiu 加速器推广文，同批模板文章的数字互相矛盾。另外，这两篇说 eFootball 用 P2P，与官方“排位用 C/S”不符，引用时不要沿用这个说法。E。 |
| 18 | lagx.app：默认路由 ping 120–217ms，优化后 28–32ms（第 86 行） | `intl_community.md` 第 102 行引用了同一页 | **页面被另一位研究员引用过，但具体数字没人核实过**；`intl_community_核查.md` 第二节第 13 条说该站在检索里没出现。厂商推广，E，标“未核实”。 |
| 19 | 全文总体立场 | 全文 | **基本恰当**。笔记没有把系统局写成“已证实”，第 68 行明确写“不等于证明系统局不存在”。核心条目没有混入网易国服材料。需要收紧的是几处“能解释”式措辞、几处 B 级标签，以及若干来源不明的中文链接，见第二节。 |

---

## 二、需更正或降级

1. **本地文件不是“iOS 11.1.1 国际服抓包”（影响第 26–30、63、85、122、157–158 行）**
   - 问题：笔记把 `GetServerEnv.json`、`gamerelay_qualitycheck_list.json`、`CmdLogin.json` 统一标成“B｜手游确认（iOS 11.1.1 国际服，抓包）”。但本仓库是**离线私服移植项目**。`ziru-extra/README.md` 第一句就是 “Server responses written for this port”（为本移植版编写的服务器响应）。本次核对发现：
     - `CmdLogin.json` 的 `pes_user_info.platform` 是 **`ANDROID`**，用户名是 `t.me/ziru_official`。它是私服的安卓模板，不是 iOS 抓包。
     - `GetServerEnv.json` 的 EULA 链接是 `file:///android_asset/responses/…`，`notification_url` 是 `hogehoge`（日文占位符），说明被改写过。不过它也有真实服务器的痕迹：真实格式的 `challenge_code`，`ip_country_code: TUR`。
     - 三个文件都是 2026-10-19 一次提交（03b5d8a）进仓库的，没有任何抓取时间、账号或设备记录。
   - 更正：
     - **字段名和结构**（11.1.1 客户端必须能解析）可以保留 **B｜手游确认**。
     - **字段取值**（阈值数字、`PING_LEVEL_BALANCE`、全 0 质量值、`TUR`、`rating_boost_*`）改为 **E｜来源未记录的私服文件，可能部分取自真实响应**。不能当作 Konami 正式服务器在排位里的实际设置。
     - 第 63 行“抓包中没有 handicap/streak/adjustment 字段”改为：“**本仓库的私服响应文件里没看到这类字段（E）。这些文件不是排位 PvP 的官方抓包，不能用来排除调整机制。**”

2. **`MatchCommandBufferingControl` 的位置、内容和推论（第 30、38 行）**
   - 问题：笔记说它“区分 P2P 和 CS 两种对战模式”“没 grep 到”，并由它加上官方承认的“指令变慢”推出“存在可动态调整的输入缓冲”。
   - 核对结果：`proto_opt` 解码后是：
     `"CS":{"Network":{…},"MatchCommandBufferingControl":{"PredictKeepBufferSizeDeltaFromMatchStopCountEnable":false},"Match":{"NoMoveOperationTimeoutMs":120000}},"P2P":{"Match":{"NoMoveOperationTimeoutMs":120000}}`
   - 更正：
     - 这个段**只出现在 CS 下**，P2P 下没有。
     - 里面唯一的开关，字面意思是“根据比赛停顿次数预测缓冲区应保持的增减量”，在这份文件里是 **false（关闭）**。
     - 键名暗示指令缓冲区的大小可以变化（B 只证明键存在，含义是 E），但这份配置里那项“预测调整”是关的。键名只和网络或比赛停顿有关，**跟连胜、积分都没有关系**。
     - 原推论里配对的“指令变慢”官方承认，其实是 2021 年主机/PC 的旧问题（见第 3 条），不能拿来和 2026 年手游配置一起推。应改写为：“CS 模式有一个指令缓冲控制配置段，键名暗示缓冲大小可变；具体实现和帧数没有公开资料（E）。”

3. **“Konami 公告页承认 command inputs could slow down 以及球和球员动作掉帧（约 2024-09）”（第 21、57 行）**
   - 问题：
     - 据 `cn_community_核查.md` 第三节第 6 条的摘要，原文是 v0.9.1 的已知问题：“The responsiveness to command inputs may become slower during matches” 和 “**The speed of the ball and player movement may decrease** during matches”。第二句说的是**球和球员移动速度下降（类似慢动作）**，不是“掉帧”，笔记译错了。
     - v0.9.x 是 2021 年 eFootball 2022 首发期的版本号，属于**主机/PC**。手游版上线晚于主机/PC 版。“约 2024-09”只是那个常年更新的公告页被搜索引擎抓到的时间，不是这条已知问题的日期。
     - 第 57 行由此推出“操作迟滞被官方当作技术质量问题处理，不是设计出来的平衡机制”，推过头了。2021 年的一条 bug 记录只能说明官方当时把它当 bug 修，不能说明不存在任何平衡机制。
   - 更正：降为“**A｜主机/PC，2021 年，仅作背景**”。说明“官方承认输入延迟”时，应改引第一节第 7 条的 Operation Sports 报道（服务器更新降低了主机和手游的输入延迟，约 2025 年）。第 57 行改为：“官方曾把输入变慢、移动速度下降列为已知问题（2021，主机/PC），说明这类症状有技术来源；这不能证明不存在其他机制。”
   - 附带一个有用的点（E）：“球和球员移动速度下降”和 `slowdown_judgment_threshold_list` 的字面意思（処理落ち/慢动作判定）一致，可以作为“传球很慢、动作发黏”的一个可自测假设：本机处理跟不上时，游戏可能变慢而不是跳帧。

4. **漏了官方关键句，导致第 67、101 行的两个推论用错依据**
   - 问题：官方 Online PvP 页还说：“**Each user's network connection is not affected by the quality of another user's connection (lag, stability, etc.) or device performance.**”（C/S 模式下，每个人的连接不受对方连接质量或设备性能影响。）见 `intl_community_核查.md` 第二节第 4 条、`official_mechanics_核查.md` 第三节第 1 条，A（二手）。笔记没有收录这句，于是：
     - 第 67 行②用 Smart Assist 帖的“可能配到连接不稳定的对手”来支持“积分高 → 配到远方对手 → 我的延迟和抖动变差”。这个依据用错了：那句警告说的是收窄筛选条件，跟积分无关；而且按官方说法，C/S 下对手连接差不会直接拖慢你。
     - 第 101 行说“对手是本地玩家，你有 60–100ms 加抖动，C/S 下你就是稳定吃亏的一方”，写得像已确认。
   - 更正：
     - 第 67 行②改为：“官方说服务器会结合**双方位置**选择。如果配到地理上较远的对手，选出的服务器可能离你更远，你自己的延迟会上升（E，基于官方选服说明的推断）。积分越高、同段在线人数越少，这种情况可能越多，但没有数据。”删去那句 Smart Assist 引文。
     - 第 101 行改为：“你自己到服务器的延迟和抖动，会直接影响你的操作响应（天线反映的就是这一段，A）。至于低延迟的对手在对抗中是否系统性占优，取决于服务器是权威模拟还是中继加缓冲，官方没有说明（E）。”
     - 结论里要写明：用户说“我网很好”，通常指本地 Wi-Fi 或测速正常，这**不等于**到 Konami 游戏服务器的链路正常。应该看赛中天线和赛后网络图表。

5. **把不带 `_cs` 的天线阈值叫作“P2P 阈值”（第 27 行）**
   - 问题：官方说天线只在 C/S 模式显示。不带后缀的 `communication_antenna_status_info` 是不是 P2P 用的，没有依据。它也可能用于房间、大厅或旧界面。
   - 更正：写成“一组通用阈值（30/120/171/209）和一组 C/S 专用阈值（15/50/110/200），通用那组的使用场景未知；单位也未知（E）”。

6. **访谈日期和 Recapio“日期不详”（第 23、58 行）**
   - 更正：访谈帖发于 **2026-02-04**（Snowflake 换算），Gamereactor 报道约 2026-02-13。Recapio 是同一访谈的摘要，日期同上。Recapio 的英文句子只能当转述，不能当引语（`cn_community_核查.md` 第一节第 16 条）。

7. **“访谈提到新增泰国区域服务器”与区域代码对照（第 23、29 行）**
   - 问题：其他笔记对同一篇 Gamereactor 报道的摘要只提到“区域服务器上线和扩展计划”（`official_mechanics.md` 第 65 行），没有一份提到“泰国”。本地中继清单里有 `ap-southeast-7`、`asia-southeast3`，这和“泰国服务器”一致，但不是证明。
   - 更正：“新增泰国服务器”标为“**据单一搜索摘要，未交叉核实（E）**”。区域代码对照表里：asia-east2＝香港、asia-east1＝台湾、asia-northeast1/2/3＝东京/大阪/首尔、asia-southeast1＝新加坡、asia-southeast2＝雅加达、ap-southeast-5＝马来西亚、ap-southeast-7＝泰国，这些和我的背景知识一致。**`asia-southeast3`＝曼谷**这一项我没有把握，需要按 Google Cloud 区域列表复核。

8. **2026-10-23 土耳其、希腊、非洲连接不稳定的公告（第 24 行）**
   - 问题：页面 URL 没确认。
   - 更正：在打开原页之前按 **E（待核实）** 处理，不要标 A。旁证（E）：本地中继清单里正好有 AWS 在伊斯坦布尔（`eu-central-1-ist-1`）、雅典（`eu-central-1-ath-1`）、拉各斯（`af-south-1-los-1`）的本地区节点，地区和公告说的对得上。

9. **Steam 帖子 ID 和引语的对应（第 61、159 行）**
   - 问题：笔记把 “against CPU PVE there are no lag nor delay, only in PVP matches” 归到帖子 3824158244542838410 和 601890191585598538。`intl_community_核查.md` 第一节第 7 条找到这句出自一个 **2024-12** 的帖子，并列的帖子 ID 是 3271312763274877449，具体哪句在哪帖没有逐一对上。
   - 更正：引语保留，标“出处帖 ID 未逐一核对”，D/E｜主机/PC。还要加一句：v4.4.0 起的跨平台对战**不含手游**，所以 Steam 的抱怨来自另一个玩家池，部分还是 P2P 模式，只能当“同引擎、同类症状”的旁证。

10. **“比赛开始时服务器会下发每位球员的能力数组 `parameters`”（第 63 行）**
    - 问题：本地文件里 `parameters` 出现在 `CMD_GET_VSCOM_OPPONENT`（人机对手阵容）和自己俱乐部阵容数据里，没有一个“开赛包”同时下发双方能力。而且都是私服文件。
    - 更正：“人机模式下对手阵容的能力数组由服务器下发；排位 PvP 中双方能力数据怎么传，没有资料（B 仅限字段结构）。”

11. **“每周状态表 `PlayerWeekly.bin` 来自全球统一 CDN，所以不会单独给你差状态”（第 157、173 行）**
    - 问题：`PlayerWeekly.bin` 这个文件名在全仓库搜不到，无法复核。能复核的是 `ziru-extra/native-livedata.json`：Live 数据从 `d1ln4m3c7n87ju.cloudfront.net/livedata/202609/<hash>/r146343/…` 下载，路径里没有用户或积分成分。
    - 更正：改为 **B（部分复核）**：“状态箭头数据从全球统一的 CDN 路径下载，路径里没有个人成分，所以同一周、同一张卡的箭头对所有人相同。”这只管得到**状态箭头**，不能推出“比赛中不存在个人化调整”，两件事要分开写。

12. **`cont_win_num` 与 WIN_STREAK（第 158 行）**
    - 更正（更精确）：WIN_STREAK 是 `fame`（荣誉称号）条目。真人 League 的值是 30，人机 League 是 50。`cont_win_num` 还出现在赛前解说信息 `CMD_GET_MYCLUB_COMMENTARY_INFO` 里，和 `cont_win_max`、`team_spirit_prematch` 等字段并列，大概是用于“已经连赢 X 场”这类解说。已知用途是称号和解说两项。这不证明、也不排除连胜被用于别处。字段结构 B，取值来自私服文件（E）。

13. **“选 Matchmake freely 更可能配到连接正常的对手”（第 133、172 行）**
    - 问题：按官方说法，C/S 下对手连接差不影响你的连接（见第 4 条）。官方没解释“不稳定对手”具体会造成什么影响（掉线、等待、对手动作异常等）。
    - 更正：可靠的收益只有“**匹配更快**”（A）。“减少差网对手 → 改善自己手感”降为 **E**。

14. **`PING_LEVEL_BALANCE` 暗示有“连接优先”档位（第 122、133 行）**
    - 问题：取值来自私服文件，全仓库只有 BALANCE 一个枚举值，UI 是否开放也不知道。
    - 更正：只能写“客户端有一个 `ping_level` 匹配偏好字段（B，结构）；可选值、默认值和是否开放给玩家，都未知（E）”。原笔记“UI 是否开放未经确认”的限定语要保留。

15. **第三节几条中文来源：范围不明、可能不准，或不该引用（第 87–91、97 行）**
    - **武汉晚报（2026-07-17）“调整匹配区域”**：官方说服务器自动选择（A），国际服手游没有公开的手动选区选项。这条建议很可能不准确，文章是不是讲国际服也没核实。降为 **E，不宜采用**。
    - **腾讯新闻（2026-03-31）**、**搜狐 1007396982**、**kayuwang**：没有别的笔记交叉核到，标“未核实”。
    - **B 站帧率解锁教程 BV1a2cMeqEP9**：不知道是针对国际服还是网易国服。中文“实况足球”帧率解锁视频很多是国服的，应标【范围未知，可能是国服】。它本来也只用来做风险提示。
    - **jichangmianfei、勇哥博客**：这是代理“机场”节点站，不是游戏加速器，和 eFootball 无关，数字互相矛盾。在中国大陆，这类工具还有合规风险。建议**删去，不作证据，也不作建议**。
    - **NoPing（EA FC 26）**：另一款游戏，原笔记已标明，维持 E。

16. **eslamgaming “How I Fixed Slow Motion After v5.5.0 Update”（第 98 行）**
    - 问题：链接的 URL 是 `…/efootball-2026-connection-lost-fix.html`（连接中断修复），和笔记给的标题（慢动作修复）对不上，可能是两篇文章被混在了一起。
    - 更正：标“标题与链接不符，未核实”，E。

17. **帧率选项、120FPS、发热数字（第 94–95 行）**
    - 都没有交叉核实，也没有官方文档。维持 E；“42°C、降频 30–50%”按原笔记建议不引用。

18. **第五节“记录表格”“录屏测延迟”标成 B（第 180–181 行），以及“值得进一步抓包”**
    - 问题：这两条是**方法**，现在还没有任何数据，不能标 B。第 180 行说“这才是值得进一步抓包的线索”，等于建议用户抓包。据 `official_mechanics_核查.md` 第三节第 4 条，Konami 在 2026-10-23 发布了 “Strict Action Against Program Tampering or Analysis”，禁止篡改和分析程序。
    - 更正：两条改标“**自测方法（尚无数据）**”。删去“抓包”，改为“保存录屏和网络图表截图，通过游戏内问卷或客服反馈给 Konami”。不能建议用户在正式账号上抓包、逆向或用第三方插帧工具。

19. **第二节 Takeaway：“这能解释‘粘滞/沉重’，而且不需要任何隐藏的能力值调整”（第 53 行）**
    - 问题：这是没经过对照测试的推断。“不需要”容易被读成“已经排除”。
    - 更正：“网络延迟、本机掉帧、体力和对手变强**可能**解释大部分症状，但没有 eFootball 手游的对照测试（E）。系统局既没有被证实，也没有被独立证伪；目前唯一的官方材料是 Konami 的否认（A，只是否认）。”

20. **第四节战术来源（第 126–130 行）**
    - gamemarket.gg 那篇在姊妹核查的检索里没有出现（`official_mechanics_核查.md` 第二节第 7 条），未核实。skycoach、playhub、revibyte、qnabangla 都未核实。revibyte 是倾向“存在幕后调整”的观点博客（`intl_community_核查.md` 第二节第 14 条），不是中立攻略。
    - gamingonphone 那篇标题里的“eFootball 2027”未核实，链接 slug 不带年份。
    - 这些都维持 **D/E**。战术建议本身是通用足球游戏思路，可以用，但别写成“经验证有效”。

21. **第五节第 2 条的加速器建议默认用户在中国大陆（第 101–102、164 行）**
    - 问题：用户只说自己是中文玩家，没说在哪里。
    - 更正：写成条件句：“如果你在中国大陆连国际服，……”。对海外华人玩家，应改为“看天线和网络图表，必要时换网络”。

---

## 三、补充发现

> 网页来源都来自姊妹笔记里其他研究员拿到的搜索摘要，**本次没能重新打开**，等级按“二手”处理。本地文件和日期换算是本次独立核实的。

1. **官方：C/S 模式下，你的连接不受对手连接和设备性能影响（原笔记漏掉的关键句）**
   - 原文：“Each user's network connection is not affected by the quality of another user's connection (lag, stability, etc.) or device performance.” P2P 模式则受双方各自连接质量影响。
   - 意义：手游排位（League/Division）走 C/S。“我网很好但卡”最该查的是**你自己到游戏服务器的链路**，不是本地测速，也不是对手的网。官方没有说明：一方延迟高时，另一方在对抗中会不会占便宜。
   - URL：<https://www.konami.com/efootball/en/page/online_match>（页面约 2025-04-16，按“569 天前”倒推，与 v4.4.0 同期）。**A（二手）｜跨平台（含手游）**。

2. **官方 X 2026-11-04：新增服务器节点，并扩容亚洲多地**
   - 新增开普敦节点，扩容东京、大阪、雅加达、新加坡、马来西亚、孟买等地（摘要被截断）。发帖时间用 Snowflake 换算为 **2026-11-04 09:33 UTC**。
   - 意义：亚洲玩家可能用到的节点刚刚扩容。用户可以对比 11 月 4 日前后的天线和手感，记录里应标出这个时间点。
   - URL：<https://x.com/play_eFootball/status/2117913032785350965>。**A｜跨平台（含手游）**。（来源：`official_mechanics.md` 第 73 行、`official_mechanics_核查.md` 第一节第 4 条。）

3. **官方通过服务器更新降低过主机和手游的输入延迟**
   - Operation Sports 转述 Konami 的 X 长帖：一次“10 月 29 日”面向主机和手游的服务器更新，按 Konami 自己的数据降低了输入延迟；会扩充服务器；赛后弹出连接质量和操作响应问卷。文章约 2025-12。
   - 意义：“慢半拍”至少有一部分是官方承认、会随服务器版本变化的延迟问题。说明这一点时，应引这条，而不是 2021 年的 v0.9.1。
   - URL：<https://www.operationsports.com/efootball-outlines-new-efforts-to-improve-online-match-quality/>；转载 <https://www.inkl.com/news/efootball-outlines-new-efforts-to-improve-online-match-quality>。**A（经媒体转述）｜跨平台（含手游）**。
   - 相关的单一说法：2026-11-04 20:44 UTC（Snowflake 换算），有玩家在 X 上称“tuning update 后输入延迟降低约 50%”，没有官方出处。<https://x.com/SnubbbS_/status/2118082045171912886>，**E**。

4. **官方禁止程序分析和篡改（2026-10-23）**
   - Konami 发布 “Strict Action Against Program Tampering or Analysis”。
   - 意义：报告不应建议用户抓包、逆向或使用插帧和解锁工具。今后玩家公开的 B 级数据挖掘也会更少。
   - URL：[Last Word on Gaming（2026-10-25）](https://lastwordongaming.com/2026/10/25/konami-zero-tolerance-mod-efootball/)。**A（经媒体转述）｜跨平台**。

5. **官方处罚过 League 中的漏洞利用、宏和自动化工具**
   - Konami News 5312（2026-05-28）：处罚在 eFootball League 中利用漏洞的账号，包括异常高频中止对局、自动化工具或宏、篡改程序或数据。Konami News 5907（2026-10-01）：封禁疑似使用自动化工具、程序或宏的账号。
   - 意义：“对手动作不符合能力值”在少数情况下可能来自作弊。官方承认作弊存在，但发生频率不明，也解释不了大多数“系统局”症状。
   - URL：<https://www.konami.com/efootball/en/topic/news/5312>；<https://www.konami.com/efootball/en/topic/news/5907>。**A（二手）｜跨平台**。

6. **手游排位是独立的玩家池**
   - v4.4.0（2025-04-17）的跨平台对战只含 PlayStation、Xbox、Steam、Windows，**不含手游**。v5.3.0（官方 X，2026-02-05 10:32 UTC，已换算）只允许手游在 VS AI 和好友赛里用蓝牙手柄。
   - 意义：笔记里 Steam 和主机的延迟抱怨，与手游排位不是同一批对手，有的还是 P2P 模式，只能当同类症状的旁证。
   - URL：<https://www.konami.com/efootball/en-us/page/v4/versioninfo_v4-40>；<https://x.com/play_eFootball/status/2019358515807023546>。**A｜主机/PC（v4.4.0）／手游确认（v5.3.0）**。

7. **C/S 模式的选服配置细节**（本地 `proto_opt` 解码，独立核实）
   - `CS.Network` 下有：`DcTestUdpPingRequestLength: 2`、`DcTestProbeDataLength: 2`、`DcTestForClientServerModeRecommendedRegionMode: 2`、`DcTestForClientServerModeRecommendedRegionThreshold: 10`。这说明 C/S 模式会先用 UDP ping 测各数据中心，再按某个阈值（可能是毫秒差）选推荐区域，和官方“按位置和网络选服务器”一致。
   - `onsys_opt` 里有 `quality_check_timeout_msec: 30000`、`game_result_timeout_sec: 45`、`align_progress_ready_timeout_sec: 40`。C/S 和 P2P 都有 `NoMoveOperationTimeoutMs: 120000`（2 分钟无操作超时）。
   - 这些键都只和网络测速、超时有关，没有一个和战绩、连胜挂钩。
   - 文件：[/home/user/pes2027-mod/ziru-extra/responses/GetServerEnv.json](/home/user/pes2027-mod/ziru-extra/responses/GetServerEnv.json)。**B（配置键）｜手游确认；含义是推断（E）；取值来自私服文件（E）**。

8. **中继清单里有 AWS 本地区节点和 Agones ping 端点**（本地，独立核实）
   - 62 个区域里有 10 个 AWS 本地区（Local Zone）条目，覆盖 9 个城市（拉各斯出现两次）：伊斯坦布尔、雅典、汉堡、华沙、马斯喀特、拉各斯、布宜诺斯艾利斯、利马、加尔各答。有 10 个区域的测速地址包含 `agones-ping`（Agones 是开源的专用游戏服务器托管框架），包括苏黎世、特拉维夫、巴林、巴黎、西班牙和上述几个本地区。端口方面，49 个区域用 5521，其余用 50000、10000、30000，`GLOBAL` 用 443。
   - 意义：结构上像“专用服务器加边缘节点”的部署，和官方说的 C/S 一致。但看不出服务器是做权威模拟，还是只做中继加缓冲。中国大陆没有节点，离大陆最近的是香港 `asia-east2`。
   - 文件：[/home/user/pes2027-mod/ziru-extra/responses/gamerelay_qualitycheck_list.json](/home/user/pes2027-mod/ziru-extra/responses/gamerelay_qualitycheck_list.json)。**B（结构）｜手游确认；部署含义是推断（E）**。

9. **v0.9.1 已知问题的准确措辞（背景，可用于“慢动作”自测假设）**
   - “The responsiveness to command inputs may become slower during matches”；“The speed of the ball and player movement may decrease during matches”。
   - 意义：官方曾把“输入变慢”“移动速度下降”列为 bug。用户说的“传球极慢、动作发黏”有可能是本机处理跟不上（慢动作），而不是数值被削弱。可以这样自测：同一台手机打 VS AI，看禁区附近是否也变慢；降低画质或帧率后是否改善。
   - URL：<https://www.konami.com/efootball/en/page/updates>（来源 `cn_community_核查.md` 第三节第 6 条）。**A｜主机/PC，2021 年，仅作背景**。

10. **设备掉帧和网络延迟的区分（营销来源）**
    - GearUP 指南把“角球、任意球、禁区人多时掉帧”归为设备问题，把“对手瞬移、动作延后生效”归为网络问题。可以作为自测分类的参考，但它是推销加速器的内容。
    - URL：<https://www.gearupbooster.com/blog/fix-efootball-mobile-lag.html>（日期未知）。**E**。（来源：`intl_community_核查.md` 第二节第 13 条、第三节第 10 条。）

11. **手游玩家的同类投诉（补上原笔记缺的手游 D 级材料）**
    - sikayetvar 英文区有多条标题写明手游或机型的投诉，日期大多不明，内容是个人感受，没有数据：
      - [eFootball Mobile Lag And Input Delay Issue On Redmi 14C](https://www.sikayetvar.com/en/efootball-us/efootball-mobile-lag-and-input-delay-issue-on-redmi-14c)
      - [eFootball Passes Go Wrong and Players Won't Respond in Division 2 and 3](https://www.sikayetvar.com/en/konami-us/efootball-passes-go-wrong-and-players-wont-respond-in-division-2-and)（段位升高后操作响应变差，和用户描述最接近）
      - [I Face eFootball 2026 Players Losing Performance Mid-Match on iPhone](https://www.sikayetvar.com/en/efootball-us/i-face-efootball-2026-players-losing-performance-mid-match-on-iphone)
    - **D/E｜手游**。（来源：`intl_community_核查.md` 第三节第 9 条。）

12. **专利：没找到 Konami 名下“按连胜调整能力值”的专利**
    - `patents_research.md` 用英文和日文检索 Konami 名下“连胜/让分/能力值修正”，没有找到相关专利。另有一族美国专利（US11712631 / US12053705 / US12427429）描述按连胜或连败阈值调整对手强度，权利人未知；玩家常引用的 DDA 专利属于 EA。
    - 意义：这类技术方案确实存在，但没有材料把它和 eFootball 联系起来。反过来，没找到专利也不能证明 Konami 没用。
    - **C｜与 eFootball 关系未知**。

---

### 本次核查的总体判断

- 原笔记没有把系统局写成“已证实”或“已证伪”，核心条目也没有混入网易国服材料。但有三个中文链接范围不明（武汉晚报、腾讯新闻 2026-03-31、B 站解锁教程），代理“机场”节点站应删去。
- 主要问题有四类：
  1. 把私服移植项目的本地文件当成“iOS 11.1.1 国际服抓包”，其中登录文件的平台字段其实是 ANDROID。字段结构可以算 B，取值只能算 E。
  2. `MatchCommandBufferingControl` 只在 CS 下出现，唯一的开关是关闭的，而且和连胜无关。原笔记的推论链把它和一条 2021 年主机/PC 的已知问题配在一起，还把“移动速度下降”误译成“掉帧”。
  3. 漏了官方关键句“C/S 下你的连接不受对手连接和设备性能影响”，导致两处推论用错了依据，“你就是稳定吃亏的一方”写得过于肯定。
  4. 几条 B 标签其实是“方法”或私服文件取值；“抓包”式建议与 Konami 2026-10-23 的禁令冲突，应删去。
- 应补进最终报告的内容：官方关键句、2026-11-04 亚洲节点扩容、2025 年服务器更新降低主机和手游输入延迟、2026-10-23 禁止程序分析、手游排位是独立玩家池。
