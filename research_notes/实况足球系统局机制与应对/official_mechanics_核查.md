# 核查报告：official_mechanics.md（Konami 官方声明与已记录的国际服手游机制）

> 核查日期：2026-11-06。范围只限 Konami 国际服 eFootball 手游（iOS/Android）。网易国服不核查、不引用。原笔记没有改动。
> 证据等级沿用原笔记：A＝Konami 官方确认或文档；B＝可复现测试、数据挖掘或逆向；C＝专利或论文；D＝广泛流传的玩家体验；E＝单个案例或推测。平台标注：【手游确认】【全平台共享】【主机/PC】【未知】。
>
> **方法限制（先读）**
> 1. **没能重新打开任何网页原文。** 本次 WebFetch 打开 x.com 失败（域名不在会话允许的网络范围内）。原笔记和姊妹核查报告已经记录 konami.com、gamereactor.eu、zaplatgaming.com 等站点同样打不开，所以按工具提示没有再试别的站点。WebSearch 也打不开：这一轮所有子任务共用的 200 次搜索额度已经用完。要逐字核对原文，需要在会话网络设置里放行 x.com、konami.com、us-support.konami.com、gamereactor.eu、videogameschronicle.com 等域名，或者在新一轮对话里补搜。
> 2. 因此本报告用了三种替代办法：
>    - **独立核对**：用 X 帖子 ID 的时间戳编码（Snowflake）算出发帖日期；直接读取本仓库的本地文件。这两类结论是本次独立得出的。
>    - **交叉核对**：和姊妹笔记里其他研究员拿到的搜索摘要对照（intl_community_核查.md、netcode_mitigation.md、patents_research.md、cn_community.md）。这只能算二手的一致性检查，不是逐字核对。
>    - **内部逻辑和时间线检查**：版本号、日期、措辞前后是否矛盾。
> 3. 下文“已核实”一栏会写明用的是哪种办法。

---

## 一、已核实

| # | 笔记中的说法 | 核对来源 | 结论 |
|---|---|---|---|
| 1 | 官方 X 于 2026-05-19 宣布在 Division 1–3 测试“按 Smart Assist 匹配”，选“Same setting as mine”可能匹配更慢、可能匹配到连接不稳定的对手 | [eFootball 官方 X 2056702522933514339](https://x.com/play_eFootball/status/2056702522933514339)；netcode_mitigation.md 第 120 行独立引用了同一段文字 | **日期已独立核实**：按帖子 ID 换算是 2026-05-19 11:44 UTC。英文原文和姊妹笔记一致，但没能重新打开帖子。等级 A。平台标签要改，见第二节第 1 条。 |
| 2 | 官方 X 于 2026-05-28 列出 League 调整：匹配系统让水平相近的玩家更容易匹配、调整 Rating 计算公式等 | [eFootball 官方 X 2060020420867158381](https://x.com/play_eFootball/status/2060020420867158381) | **日期已独立核实**：2026-05-28 15:28 UTC。前两项内容属实（A，全平台）。第三项“取消降级”被笔记读错了，见第二节第 3 条。 |
| 3 | v5.5.0 修改了 League 的 Rating 计算公式和匹配逻辑 | [Konami v5.5.0 Information](https://www.konami.com/efootball/en/page/v5/versioninfo_v5-50)；netcode_mitigation.md 第 121 行引用的摘要原文：“Changed the matchmaking system so that users around the same level of ability will match together more frequently”，并标注 Smart Assist 匹配为测试功能 | **两份笔记的摘要一致（二手）**。v5.5.0 里同时有 Smart Assist 匹配测试，而官方 5-19 的帖子说这项测试放在“5 月下旬至 6 月初”的更新里，所以 v5.5.0 应是 **2026 年 5 月下旬至 6 月初**发布的那次更新（推断，原笔记写“具体日期未取得”）。A，全平台。 |
| 4 | 官方 X 于 2026-11-04 宣布新增服务器节点（开普敦），并扩容东京、大阪、雅加达、新加坡等地 | [eFootball 官方 X 2117913032785350965](https://x.com/play_eFootball/status/2117913032785350965) | **日期已独立核实**：2026-11-04 09:33 UTC。本地 `gamerelay_qualitycheck_list.json`（2026-10-19 入库）的 62 个中继区域里有 GCP 的 `africa-south1`（约翰内斯堡）和 AWS 拉各斯本地区 `af-south-1-los-1`，但**没有**单独的 AWS `af-south-1`（开普敦），和“开普敦是 11 月新增”不矛盾。A，全平台含手游。 |
| 5 | Konami 在 eFootball CONNECT / Spotlight Interview 中否认存在偏袒一方的 scripting，把部分“剧本感”归因于疲劳/技能和切换光标（cursor change）修正 | [Gamereactor](https://www.gamereactor.eu/konami-reiterates-that-there-is-no-scripting-in-efootball-and-announces-the-master-league-event-for-april-1674973/)；[官方 X 发布访谈的帖子 2019007136819519702](https://x.com/play_eFootball/status/2019007136819519702)；intl_community_核查.md 第 13 行 | **属实（二手）**。访谈帖的日期已独立核实：2026-02-04 11:16 UTC。按笔记“约 266 天前”推算，Gamereactor 的报道日期是 2026-02-13，和姊妹核查一致。这里的 A 级只能证明“Konami 公开否认过”，不能证明系统局不存在。逐字原话仍未取得。归因于“连接不稳定、按键输入延迟”的部分只有一份二手转述，见第二节第 8 条。 |
| 6 | Smart Assist 匹配选项与客户端字段 `control_style_filtering` 对应；登录响应里有 `ping_level: PING_LEVEL_BALANCE`、`team_power_limit: TEAM_POWER_NO_LIMIT`、`control_style_filtering: NO` | 本地 [CmdLogin.json](/home/user/pes2027-mod/ziru-extra/responses/CmdLogin.json)（三个字段都是登录响应的顶层字段） | **字段确实存在**（独立核实）。说明 11.1.1 版国际服 iOS 客户端认识这三个匹配偏好字段（B，手游确认）。但文件来源和取值的可信度要降级，见第二节第 4 条。字段和官方功能的对应关系仍是推断。 |
| 7 | `GetServerEnv.json` 里有 `enable_control_style_filtering_match_category: []`，配置时间 `date=1789384820`（约 2026-09-14） | 本地 [GetServerEnv.json](/home/user/pes2027-mod/ziru-extra/responses/GetServerEnv.json) | **字段和数值都存在**。时间戳换算为 2026-09-14 11:20 UTC，正确。“服务器没有启用任何类别”这个推论要降级，见第二节第 4 条。 |
| 8 | `CONNECTION_REPORT_VIEW` 链接到官方天线说明页；有两组天线阈值：30/120/171/209 和 `_cs` 15/50/110/200 | 本地 GetServerEnv.json 的 `ulist` 和 `communication_antenna_status_info(_cs)` | **数值全部属实**（独立核实）。不带 `_cs` 的那组被笔记称为“P2P 阈值”，这只是推断，见第二节第 9 条。 |
| 9 | 客户端配置里有 `MatchCommandBufferingControl` 段，并区分 P2P 和 CS 两种模式 | 本地 GetServerEnv.json 的 `proto_opt` 字段（Base64 编码，解码后能看到） | **属实**（独立核实）。netcode_mitigation.md 说“没 grep 到这个键名”，原因是它藏在 Base64 里。解码后是 `"CS":{"Network":{…},"MatchCommandBufferingControl":{"PredictKeepBufferSizeDeltaFromMatchStopCountEnable":false},"Match":{"NoMoveOperationTimeoutMs":120000}}`。也就是说，这个缓冲控制段**只出现在 CS 模式下**，里面唯一的开关是“按比赛停顿次数预测缓冲区增减”，取值 false。B（配置键存在），含义属推断（E）。新增细节见第三节第 9 条。 |
| 10 | 62 个区域的 game-relay 中继，质量阈值 75/175 | 本地 [gamerelay_qualitycheck_list.json](/home/user/pes2027-mod/ziru-extra/responses/gamerelay_qualitycheck_list.json) | **属实**（独立核实）。区域名是 AWS/GCP 的真实区域代号，并带有真实的 EC2 主机名。每个区域的 quality 和 last_update 都是 0。阈值的单位和用途没有文档。 |
| 11 | 荣誉称号里有 WIN_STREAK（30/50 连胜） | 本地 cmd_get_league_com_info.json、ZIRU_GET_VSCOM_GAME_RESULT_TEMPLATE.json | **属实，可以更精确**：这是 `fame`（荣誉称号）条目。`EFOOTBALL_LEAGUE`（真人 League）的 WIN_STREAK 值为 30，`EFOOTBALL_LEAGUE_COM`（人机 League）为 50，档位都是 tier 4。用途是展示成就，不能证明连胜会触发比赛调整。 |
| 12 | 赛后结果结构里，`cont_win_num` 和 `rating`、`oldrating`、`rating_boost_type`（NONE）、`rating_boost_rate`（1）并列 | 本地 ZIRU_GET_VSCOM_GAME_RESULT_TEMPLATE.json、ZIRU_ML_GAME_RESULT.json 的 `match_result.player_data` | **字段和数值属实**。但这两个文件是离线私服的**模板**（文件名带 ZIRU_，用户名是 “t.me/ziru_official”），不是赛后抓包，见第二节第 4 条。 |
| 13 | vs-COM League 的 AI 难度由服务器按 Division 下发（进攻/防守 com level） | 本地 cmd_get_league_com_info.json：`com_level_info: {offense_com_level: EASY, offense_sub_com_level: NONE, defense_com_level: EASY, defense_sub_com_level: NONE}`；官方 v4.4.0（姊妹核查第三节第 7 条：人机类别的 AI 强度随 Division 提高） | **属实，而且可以升到 A**：“人机 League 难度随 Division 变化”有官方 v4.4.0 版本说明支持（A，全平台）。客户端字段（B）说明难度分进攻、防守两套，各有子档。这个文件本身由私服编写（Division 8 用 EASY 不代表官方取值）。这些字段只挂在 vs-COM 上，本地文件里没有 PvP 的同类字段。 |
| 14 | v3.0.0（2023）取消了“球员随球队风格熟练度加减能力” | [Konami v3.0.0](https://www.konami.com/efootball/en-us/page/2024/versioninfo_v3-00)；intl_community_核查.md 第 26、72 行也以这个页面作官方文档 | **页面存在，与姊妹笔记一致（二手）**。没能逐字核对。A，全平台。 |
| 15 | 天线图标显示“你自己”到游戏服务器的实时连接质量，双方天线可以不同；赛后有 Network Graph；手机要靠近接入点 | [Konami Online PvP Setup](https://www.konami.com/efootball/en/page/online_match)；netcode_mitigation.md 第 17–19、84 行；intl_community_核查.md 第 104 行 | **三份笔记的摘要一致（二手）**。A，全平台含手游。同一页面还有一句更关键的官方说明，原笔记漏了，见第三节第 1 条。 |
| 16 | 笔记整体结论：系统局“未被证实，也未被完全排除” | 全文 | **表述恰当**。笔记没有把系统局写成已证实或已证伪，也没有引用网易国服的材料。需要收紧的是若干证据等级和措辞，见第二节。 |

---

## 二、需更正或降级

1. **Smart Assist 匹配帖子的平台标签**
   - 问题：笔记标为“A｜手游确认（全平台公告）”。
   - 更正：帖子没有区分平台，Smart Assist 也不是手游独有。应标为 **A｜全平台共享（含手游）**。

2. **版本号和日期的时间线矛盾：v5.5.1 = 2026-04-06**
   - 问题：笔记写 v5.5.1 于 2026-04-06 发布，并说“先后顺序无法确认”。但按第一节第 3 条，v5.5.0 应在 2026 年 5 月下旬至 6 月初（它包含 5-19 才宣布的 Smart Assist 测试）。v5.5.1 不可能早于 v5.5.0。另外，v5.3.0 是 2026-02-05（官方 X），v5.3.1 约在 2026-03-05。
   - 更正：“2026-04-06”对应的可能是 v5.4.x，也可能是日期本身有误。报告中**不要引用这个版本号和日期的组合**。“进球动画期间的输入可能让射门朝向意外方向”这条修复，只能写成“某个 2026 年版本修过（版本号和日期待核实）”。

3. **“取消降级”读错了：Phases 和 Divisions**
   - 问题：笔记第 20 行写“2026-05 的调整改为 Division 1–3 在 Phase 中途不降级”。但笔记自己引用的帖子原文是 “Abolition of relegation during **Phases** 1–3”。
   - 更正：原文说的是 **Phase 1–3 期间**取消降级（Phase 是赛季里的时间段）。具体覆盖哪些 Division，标题被截断，无法确认。应写成：“官方 2026-05-28 宣布‘Phase 1–3 期间取消降级’，适用的 Division 范围未取得原文。”此外，第 19、20 行的 v3.2.0 分组规则（Division 1–3 / 4–6 / 7–9、每个 Phase 初期分组更宽）是 2024 年的资料。2025–2026 年匹配逻辑改过多次，报告引用时应标注“2024 年规则，可能已被替换”。

4. **本地文件不是“国际服抓包”，证据等级要拆开写（影响第一、三、六节的 B 级说法）**
   - 问题：笔记多处写“本仓库抓到的国际服 iOS 登录响应”“赛后结果结构”“抓包显示服务器未启用”，都按“B｜手游确认”处理。但本仓库是一个**离线私服移植项目**（基于 Ziru 的安卓离线服务器，移植到 iOS 11.1.1）。`ziru-extra/README.md` 明说这些文件是“Server responses written for this port”（为这个移植版编写的服务器响应）。逐个看：
     - `gamerelay_qualitycheck_list.json`、`GetServerEnv.json`：很像从真实服务器响应改出来的。证据是里面有真实的 AWS/GCP 主机名、`legal.konami.com/…/index_202602.html`、`ip_country_code: TUR`、`challenge_code`。但也被改过：EULA 链接改成了 `file:///android_asset/…`，网关指向 `127.0.0.1`。抓取时间和账号都没有记录。
     - `cmd_get_league_com_info.json`：明显是私服编写的。phase 类型是 `CUSTOM`，结束时间是 2029-12-31，人机难度 EASY。
     - `CmdStartMatch.json`、`CmdSetMatchResult.json`：私服编写（内容是 “Offline Campaign … running locally”“Offline Barca”）。
     - `ZIRU_*` 结果模板：私服模板。
   - 更正：
     - **字段名**（客户端必须能解析，所以反映 11.1.1 客户端的数据结构）可以保留 **B｜手游确认**。
     - **字段取值**（`PING_LEVEL_BALANCE`、`NO`、空列表、`NONE`/`1`、10 个 `NO`）应降为 **E｜来源未记录的私服文件**，不能说明 Konami 正式服务器在排位里的实际设置。
     - 第 16 行“服务器按类别、按 Division 开关这项筛选”：字段结构支持这个说法（B）。但“抓包时服务器未启用任何类别”应删去，或改为“本仓库文件里为空，来源不明”。
     - 第 145 行“没有证据显示存在按个人连胜调整能力值的字段（B）”应改为“**在本仓库的私服响应文件里没看到这类字段（E）；这些文件不是排位 PvP 的官方抓包，不能用来排除**”。

5. **“开赛时服务器会下发双方球员能力数组 `parameters`”**
   - 问题：本地文件里，`parameters` 数组出现在两处：`CMD_GET_VSCOM_OPPONENT`（人机对手阵容）和 `CMD_GET_MYCLUB_ENTRY_INFO`（自己俱乐部阵容）。没有一个“开赛包”同时下发双方能力。而且这些是私服文件。
   - 更正：写成“**人机模式下，对手阵容的能力数组由服务器下发；自己球员的能力数组随俱乐部阵容数据下发。排位 PvP 时双方能力数据怎么传，没有资料**（B 仅限字段结构）”。

6. **状态箭头“全球统一”的推论范围**
   - 问题：PlayerWeekly.bin（17,073 名球员、取值 0–4）这个文件名在仓库里任何文件中都找不到，无法复核。笔记还由此推出“不是个人化削弱”。
   - 更正：
     - 能复核的部分：`ziru-extra/native-livedata.json` 显示，Live 数据包从全球 CloudFront 路径 `d1ln4m3c7n87ju.cloudfront.net/livedata/202609/<hash>/r146343/…` 下载，路径里没有用户或 Rating 成分。这支持“周表对所有人相同”（B，部分复核）。
     - 结论只能管到**状态箭头本身**。能力值另有下发渠道（见上一条），所以“箭头全球统一”不能推出“比赛中不存在个人化调整”。报告里要把这两件事分开写。

7. **Condition/Form 的证据等级偏低**
   - 问题：第 43 行把“状态开赛前决定、由 Live Update 驱动”标为 D（第三方攻略）。
   - 更正：姊妹核查确认，官方 v3.0.0 写有 Form 类型和 Condition 的规则，例如 Inconsistent 球员的 Live Update Rating 达到 B 或以上时，Condition 至少为 Normal。所以“规则存在”应升为 **A｜全平台**。“箭头具体加减多少能力值”仍无官方数字，维持 D。另外，GamingonPhone 的链接 slug 是 `efootball-a-guide-to-…`（笔记称“2027 指南”），姊妹核查核对的是 `efootball-2024-a-guide-to-…`，两者可能不是同一页。Gamemarket.gg 那篇在姊妹核查的检索里没出现，未核实。

8. **官方访谈的归因范围被放宽了**
   - 问题：第三节 Takeaway 和第 138 行写，Konami 高层把剧本感归因于“疲劳/技能、切换光标修正、**连接不稳定和按键输入延迟**”。
   - 更正：两份以上摘要一致支持的只有“疲劳/技能”和“切换光标修正”（姊妹核查也说 Kimura 提到体力、能力随比赛时间下降，光标切换可能出现不一致）。“连接不稳定、按键输入延迟”只见于一条摘要（很可能是 Zaplat 的转述），应标为“**据二手转述，未核实**”。
   - 人名也要改：日文名是**木村征太郎**，不是“木村诚太郎”（Famitsu 2022、ASCII 2021，见姊妹核查第二节第 1 条）。官方头衔是 General Director。“田谷淳一”这几个汉字没有核实，建议只写 Junichi Taya。

9. **把不带 `_cs` 的天线阈值叫作“P2P 阈值”**
   - 问题：官方页面（netcode_mitigation.md 第 19 行摘要）说，天线图标**只在 CS 模式里显示**。那么不带后缀的 `communication_antenna_status_info` 是不是 P2P 专用，没有依据。它也可能是房间、大厅或旧版界面用的。
   - 更正：写成“一组通用阈值和一组 CS 专用阈值；通用那组的适用场景未知（E）”。

10. **v0.9.1 已知问题“指令输入响应可能变慢”的年份和平台**
    - 问题：笔记把它列为 A，平台“全平台”，年份有矛盾。
    - 更正：v0.9.x 是 2021 年 eFootball 2022 首发期的版本号，这条大概率是 **2021 年、主机/PC** 的已知问题，不能拿来说明 2026 年手游。官方承认输入延迟有更直接、更晚的材料：Operation Sports 报道，Konami 通过服务器更新降低了主机和手游的输入延迟（见第三节第 2 条）。建议改引那条，把 v0.9.1 降为“2021 年旧作背景，未核实”。

11. **2021 年首发道歉的平台标签**
    - 问题：第 69 行标“全平台共享”。
    - 更正：道歉针对 2021-09-30 首发的主机/PC 版（v0.9.0）。据我的背景知识，手游版晚于主机/PC 版推出（具体日期本次未核实）。应标为 **A（经报道）｜主机/PC，2021 年**。“传球速度”是否原文提到，本次未能复核。能支撑的只是一个泛化结论：手感参数会通过版本补丁全服统一调整。

12. **eFootball_Radar 的 v5.3.1 消息被标成了 A**
    - 问题：第 74 行注明“转发账号消息，非官方账号”，却标为 A。
    - 更正：帖子日期已核实，是 2026-03-03 06:06 UTC。但它是第三方账号，应降为 **D｜第三方转述**。

13. **Sikayetvar/Xolvie 的“匹配范围随等待时间扩大”问答**
    - 问题：第 21 行的 `q-25598` 页面属于 sikayetvar 的 “q-” 问答页。姊妹核查发现这类页面结构固定（“Why this happens / What to try”），更像平台自动生成的帮助问答，不是玩家投诉，也不是官方说法。第 144 行又把“搜不到对手时匹配范围扩大”标为 D。
    - 更正：降为 **E｜未核实，不作为证据**。“长时间搜不到对手时放宽匹配范围”是常见的匹配做法，但 eFootball 手游没有任何来源证实。

14. **“2025 年已知问题：主教练熟练度只是显示问题”**
    - 问题：第 47 行没有 URL，原文未见。
    - 更正：报告里**不要引用**。

15. **缓解建议 ③“选 Matchmake freely 可减少遇到差网对手”的作用被高估**
    - 问题：官方 Online PvP 页明说 CS 模式下 “Each user's network connection is not affected by the quality of another user's connection”（每个人的连接不受他人连接质量影响）。League 用的就是 CS 模式。所以就算匹配到差网对手，按官方说法也不会直接拖慢你自己的操作。官方警告“可能匹配到连接不稳定的对手”和这句说明之间有张力，官方没有解释差网对手具体会造成什么影响（掉线、等待、对手动作异常等）。
    - 更正：这条建议的可靠收益只有“匹配更快”（A）。“减少差网对手→改善自己手感”是推断，降为 **E**。

16. **“这些已知机制足以解释大部分症状”**
    - 问题：第 147 行用了“足以解释”。这是没有经过测试的推断，读者容易理解成“系统局已被排除”。
    - 更正：改为“**可能解释**大部分症状，但没有对照测试验证（E）”。这条和“系统局未被证实、也未被独立证伪”的总结论应该并列写出。

17. **“在连胜前后抓包对比 `parameters`”不能当作给用户的建议**
    - 问题：第 150 行把排位抓包写成检验方法。但据 cn_community.md 引用的 Last Word on Gaming 报道，Konami 于 2026-10-23 发布了 “Strict Action Against Program Tampering or Analysis” 警告，明确禁止分析和篡改程序。
    - 更正：报告里只能把它写成“研究上的空白”。**不能建议用户在自己的正式账号上抓包或逆向**，否则有封号风险。给用户的可行做法，应限于游戏内能看到的东西：天线、赛后 Network Graph、录屏、逐场记录。

---

## 三、补充发现

> 由于网页搜索额度已用完，下面的网页来源都来自姊妹笔记里其他研究员拿到的搜索摘要，**本次没有重新打开**，等级按“二手核对”处理。本地文件的发现是本次独立核实的。

1. **官方明说：排位用 CS，对手的网络不影响你的连接**（原笔记漏掉的关键句）
   - Konami《Online PvP Setup for eFootball》页：结果重要的模式（eFootball League、挑战活动、排名活动等）用 client-server 方式，“to ensure a stable connection and prevent cheating”；“Each user's network connection is not affected by the quality of another user's connection (lag, stability, etc.) or device performance.” 快速比赛、1v1 好友赛用 P2P。服务器选择会综合双方位置和网络基础设施，有时更远的服务器连接反而更好。
   - 意义：在手游排位里，“我网很好但卡”最该排查的是**你自己到游戏服务器的链路**，不是家里 Wi-Fi 测速，也不是对手的网络。
   - URL：<https://www.konami.com/efootball/en/page/online_match>（简中页 <https://www.konami.com/efootball/zh-cn/page/online_match>，这是 Konami 国际服的中文页，不是网易）。页面日期未取得（搜索工具显示约 2025-04）。**A（二手）｜全平台含手游**。

2. **官方承认输入延迟，并通过服务器更新降低过**
   - Operation Sports 根据 Konami 的 X 长帖报道：一次“10 月 29 日”面向主机和手游的服务器更新降低了输入延迟；Konami 会增设全球服务器站点；赛后会弹出连接质量和操作响应的问卷。文章日期按检索元数据推算约 2025-12，所以“10 月 29 日”很可能是 2025 年（未核实）。
   - 意义：“慢半拍”有一部分来自官方承认、随服务器版本变化的延迟问题，比 2021 年的 v0.9.1 更适合引用。
   - URL：<https://www.operationsports.com/efootball-outlines-new-efforts-to-improve-online-match-quality/>。**A（经媒体转述）｜全平台含手游**。

3. **官方处罚过 League 中的漏洞利用和宏**
   - Konami News 5312（2026-05-28）：处罚在 eFootball League 中利用漏洞的账号，包括异常高频中止对局、自动化工具或宏、篡改程序或数据。Konami News 5907（2026-10-01）：封禁疑似使用自动化工具、程序或宏的账号。
   - 意义：“对手动作不符合能力值”在少数情况下可能来自作弊。这是官方承认存在的另一种解释，但发生频率不明。
   - URL：<https://www.konami.com/efootball/en/topic/news/5312>；<https://www.konami.com/efootball/en/topic/news/5907>。**A（二手）｜全平台**。

4. **官方禁止程序分析和篡改（2026-10-23）**
   - Konami 发布 “Strict Action Against Program Tampering or Analysis”。这影响两件事：B 级证据（拆包、抓包）今后更难由玩家公开产出；报告不应建议用户自行抓包。
   - URL：[Last Word on Gaming（2026-10-25）](https://lastwordongaming.com/2026/10/25/konami-zero-tolerance-mod-efootball/)。**A（经媒体转述）｜全平台**。

5. **地区性连接故障的官方公告（2026-10-23，待核实）**
   - 搜索摘要提到 Konami 承认土耳其、希腊和非洲部分地区的比赛连接比平时不稳定，并称已修复。具体页面没有确认，可能是 <https://www.konami.com/efootball/en-us/topic/news/6043>。
   - 意义：某段时间突然“全场卡”，可能是官方承认过的区域性服务器问题，可以先查官方公告。
   - **A？（URL 未确认，暂按 E 处理）｜全平台**。

6. **手游排位是单独的玩家池**
   - v4.4.0（2025-04-17）的跨平台对战只覆盖 PlayStation、Xbox、Steam、Windows，不含手游。v5.3.0（官方 X，2026-02-05，日期已用帖子 ID 核实）只允许手游在 VS AI 和好友赛里使用蓝牙手柄。
   - 意义：Steam 和主机上的“系统局”抱怨与手游排位不是同一批对手，只能当“同引擎、同类症状”的旁证。
   - URL：<https://www.konami.com/efootball/en-us/page/v4/versioninfo_v4-40>；<https://x.com/play_eFootball/status/2019358515807023546>。**A｜手游确认（v5.3.0）/主机/PC（v4.4.0）**。

7. **v4.4.0 为 CS 对战加入天线显示，并设立人机 League 类别（AI 强度随 Division 提高）**
   - 意义：天线和赛后网络图表是官方专门提供的排查工具。人机 League 的难度分档有官方文档。玩家如果把人机 League 的“难度随段位变”迁移到 PvP，容易误以为排位也会调难度。
   - URL：<https://www.konami.com/efootball/en-us/page/v4/versioninfo_v4-40>（2025-04-17）。**A（二手）｜全平台**。

8. **`cont_win_num` 的另一个已知用途：解说**（本地，独立核实）
   - `CmdGetMyclubCommentaryInfo.json`（`CMD_GET_MYCLUB_COMMENTARY_INFO`，赛前解说信息）里有 `cont_win_num`、`cont_win_max`、`team_spirit_prematch`、`team_power_level_prematch`、`total_match_num`、`highest_division` 等字段。
   - 意义：服务器记录连胜数，至少有称号和赛前解说（“已经连赢 X 场”）这两个明确用途。这不证明，也不排除连胜被用于别处。
   - 文件：[/home/user/pes2027-mod/ziru-extra/responses/CmdGetMyclubCommentaryInfo.json](/home/user/pes2027-mod/ziru-extra/responses/CmdGetMyclubCommentaryInfo.json)。**B（字段结构）｜手游确认；取值来自私服文件，E**。

9. **CS 模式网络配置的更多细节**（本地 `proto_opt` 解码，独立核实）
   - `CS.Network` 下有 `DcTestForClientServerModeRecommendedRegionMode: 2`、`DcTestForClientServerModeRecommendedRegionThreshold: 10`、`DcTestUdpPingRequestLength: 2`。这说明 CS 模式会用 UDP ping 测各数据中心，再选推荐区域，和官方“按位置和网络选服务器”的说法一致。
   - CS 和 P2P 都有 `NoMoveOperationTimeoutMs: 120000`（2 分钟无操作超时）。
   - `onsys_opt` 里有 `quality_check_timeout_msec: 30000`、`game_result_timeout_sec: 45`。
   - **B（配置键）｜手游确认；含义是推断（E）**。

10. **`slowdown_judgment_threshold_list: [51, 25, 17]` 很可能是帧时间阈值（毫秒）**（推断）
    - 换算：17 ms ≈ 1000/60（60 帧），25 ms = 1000/40（40 帧），51 ms ≈ 1000/20（约 20 帧）。这三个数恰好对应常见的帧率档位，支持“这是掉帧判定”的解读。
    - 意义：手机本身在禁区附近掉帧的假设可以自测，例如打 VS AI 时禁区附近卡不卡，降画质或帧率后是否改善。
    - **E｜手游确认（字段），含义未经官方说明**。

11. **专利方面**（交叉引用 patents_research.md，均未打开原文）
    - 没找到 Konami 名下“按连胜调整能力值”的专利。
    - 能找到的日本候选专利（JP6205138B2：按历史成绩计算公开让分；JP2012196424A：胜负点数用于成长）都是公开、可见的让分或成长机制，权利人未核实。
    - 另有一族美国专利（US11712631 / US12053705 / US12427429）描述按连胜/连败阈值调整对手强度，权利人未知。
    - 玩家常引用的 DDA 专利属于 EA（US10286323B2、US10807004B2），这和原笔记引用的 Zaplat 说法一致。
    - 意义：“按连胜调对手”作为技术方案确实有人申请过专利，但没有任何材料把它和 eFootball 联系起来。
    - **C｜与 eFootball 关系未知**。

12. **版本号对照提醒（背景知识，未核实）**
    - Konami 官网的版本号（v5.5.0 等）和手机商店里的 App 版本号可能不同。本仓库客户端是 iOS 11.1.1，配置日期是 2026-09。据我的记忆，手游 App 版本号通常比官网版本号大 5（例如官网 v3.0.0 对应手游 8.0.0），但本次没有核实。报告引用版本号时，建议同时写发布日期，避免用户对不上。
    - **E**。

---

### 本次核查的总体判断

- 原笔记没有把系统局写成“已证实”或“已证伪”，总结论“未被证实，也未被完全排除”成立。没有发现网易国服材料混入。
- 主要问题有四类：
  1. 把私服移植项目的本地响应文件当成“国际服抓包”，抬高了“未见调整字段”的证据力。
  2. 几处版本、日期和措辞错误：v5.5.1 的日期；把“Phases 1–3”读成 Division；人名汉字。
  3. 几个 A 级标签过宽：第三方账号的消息；2021 年主机/PC 的资料被标成全平台；没有 URL 的已知问题。
  4. 官方访谈的归因范围被放宽。
- 最该补进报告的官方信息是：排位用 CS，**对手网络不影响你的连接**（官方原话）；官方承认并降低过输入延迟；官方处罚过漏洞利用和宏；2026-10 起禁止程序分析，所以不能建议用户自己抓包。
