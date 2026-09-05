#!/usr/bin/env node
/*
   This file is part of the Ouaricon Audio plugin suite.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    i18n-zh-lint.js — Simplified Chinese typography and terminology, across
    every plugin.

    ── THIS IS A GATE ────────────────────────────────────────────────────────

    This tool EXITS 2 on any finding. It shipped as a report — exit 0 whatever
    it found — and was promoted on 2026-09-05, at the start of Stage 4 wave 4b,
    once the promotion criterion it set for itself was met and measured: the
    Stage 2 pilot (O-Chorus) and the whole corpus at zero findings (1034
    entries, 0 / 43 plugins) with --self-test at 10/10. That is the exact
    lifecycle scripts/i18n-fr-lint.js went through: report-only on the day 43
    of 43 plugins failed it, a gate on 2026-08-31 once the rollout had taken
    every plugin to 0. Shipping either one as a gate on day one would have let
    a half-built lint block the rollout, which is the failure the ordering
    exists to avoid; keeping it a report past its own criterion is the opposite
    failure, and a report that has to be remembered is not a gate.

    AUTHORING IS NOT BLOCKED. Entries at reviewed:'mt' — machine draft, not yet
    read back — are counted in the BELOW SHIP BAR line and are NOT findings.
    A wave can draft a whole table under the live gate and still exit 0; only
    a typography or terminology violation blocks. That routing is load-bearing
    and must not be changed without changing this paragraph with it.

    WHAT IT IS. check-i18n.js proves the MECHANISM: every key resolves, every
    entry carries a reviewed flag, the canon has not drifted. It says nothing
    about whether the Chinese is any good. This file checks the part of "good"
    that a regex can see, so the part a person has to read is the only part
    left for the person.

    ── The checks ────────────────────────────────────────────────────────────

      Z1  punctuation    ASCII , . : ; ? ! ( ) inside Han prose — zh takes the
                         full-width forms. ASCII punctuation INSIDE a Latin or
                         unit token is legal and is masked out first.
      Z2  no U+00A0      a no-break space before : ; ! ? % — the deliberate
                         INVERSE of French T3/T4/T5. The full-width forms carry
                         their own half-em sidebearing; adding U+00A0 doubles it.
      Z3  variant        a Traditional-only character in a zh-Hans table. The
                         set is DERIVED from OpenCC dictionary data, never hand
                         written — see the provenance block below.
      Z4  Latin/CJK      spacing between a Latin/digit run and a Han run is one
                         plain U+0020 everywhere or nowhere; INCONSISTENCY is
                         the finding, not either form. A thin space (U+2009 /
                         U+200A) fires unconditionally — it has no glyph in some
                         of the faces this suite ships and would render as a box
                         where no gate looks. Same reasoning that chose U+00A0
                         over U+202F for French.
      Z5  glossary       a LABEL or tooltip TITLE whose English is a TERMS key
                         in scripts/i18n-zh-glossary.js renders as one of its
                         accepted zh-Hans forms. The French G1 analogue.
      Z6  budget         a rendering longer than its MEASURED character budget
                         (maxChars = floor(cellWidthPx / fontSizePx)). A term
                         with no measured cell is UNBUDGETED and Z6 is inert on
                         it — the summary block prints how many, because an
                         inert rule that does not announce itself is the "gate
                         green on unchecked content" failure this rollout exists
                         to prevent.
      Z7  full-width     full-width Latin letters or digits (ＬＦＯ, ２) — a
                         classic machine-translation artifact.
      F1  forbidden      a rendering from FORBIDDEN_IN_LABELS in a label or
                         title, or from FORBIDDEN_IN_PROSE in a body.
      R1  reviewed       a zh-Hans entry whose `reviewed` is absent or is not
                         one of 'mt' | 'bt' | 'native'. An entry at 'mt' is
                         reported separately as BELOW SHIP BAR (info) — that
                         disclosure is the value this rule adds over
                         check-i18n.js assertion [5], which only asks that the
                         flag be present.
      Z8  intra-Han sp   a plain U+0020 between two Han code points. Added
                         2026-09-04, after Stage 3 found the shape by READING
                         rather than by any rule — hence its position last in
                         CODES and last in every column, legend and table that
                         derives from CODES. Scoped to the PLAIN space only, so
                         its zero can be checked against an independent scan;
                         Z2 and Z4 police the other whitespace classes at their
                         own boundaries.

    NOT PORTED, deliberately. French T1-T7 are French typography and Z2 is the
    exact inverse of three of them; porting them would be actively wrong, not
    merely useless. C1 (casing) has no zh analogue at all — Han has no case and
    `text-transform: uppercase` is a no-op on it.

    INFO, never a finding: entries marked `sameAsEn: true`, and entries carrying
    a `termNote` (a reasoned glossary exemption). A termNote exempts the entry
    from BOTH term rules — Z5 and F1 — exactly as it does in French.
    I18N_EXEMPT stays check-i18n.js's concern; this lint neither reads nor
    re-interprets it.

    Usage:
        node scripts/i18n-zh-lint.js                    # all plugins, report
        node scripts/i18n-zh-lint.js --plugin O-Chorus  # one plugin
        node scripts/i18n-zh-lint.js --verbose          # every finding, not 12
        node scripts/i18n-zh-lint.js --codes            # the rule codes, one line
        node scripts/i18n-zh-lint.js --self-test        # prove each rule fires

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const { pathToFileURL } = require('url');
const G = require(path.join(__dirname, 'i18n-zh-glossary.js'));

const LANG  = 'zh-Hans';
const CODES = ['Z1', 'Z2', 'Z3', 'Z4', 'Z5', 'Z6', 'Z7', 'F1', 'R1', 'Z8'];
const REVIEWED_ENUM = ['mt', 'bt', 'native'];

const argv    = process.argv.slice(2);
const val     = (k) => { const i = argv.indexOf(k); return i >= 0 ? argv[i + 1] : null; };
const only    = val('--plugin');
const verbose = argv.includes('--verbose');
const MAX_SHOWN = verbose ? Infinity : 12;

const ROOT = path.resolve(__dirname, '..');
const UI_ROOTS = ['Source/ui/public/js/i18n.js', 'Resources/ui/js/i18n.js'];

// ── Traditional-only character set (rule Z3) ────────────────────────────────
// ZH_TRAD_ONLY_SENTINEL
// PROVENANCE — this set is DERIVED, never hand written. Regenerate, do not edit.
//
//   generator scripts/gen-zh-trad-only.js — COMMITTED, and it audits its own
//             output offline via `--verify`. The first generator was a
//             scratchpad throwaway, and that is precisely why an incorrect
//             derivation survived unread until it flagged the glossary's own
//             rendering of `pan`; the decision is reversed here on purpose.
//   sources   https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/TSCharacters.txt
//             https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/STCharacters.txt
//   sha256    TSCharacters.txt  737c21c66f55a419dd6956cb3089476cdefc5a36877452631617696df1e5d925
//             STCharacters.txt  a0ca1601c70648cf48b33c3c6210ccbecc5c7eead4b4c3daf76587ba2c03582b
//   fetched   2026-09-04 (digests unchanged from the 2026-09-01 fetch — the
//             inputs did not drift; the RULE was wrong)
//   rule      Traditional-only = a TS key that is NOT an ST key AND does not
//             appear anywhere in its own TS value list.
//             A TS key is a character the Traditional->Simplified table has to
//             convert; if that same character is ALSO an ST key it is a live
//             simplified form too (or an ambiguous both-scripts character) and
//             must not be flagged. THE THIRD CLAUSE IS THE 2026-09-04 FIX: a TS
//             key whose value list contains ITSELF survives T->S conversion
//             unchanged, so it is a live simplified character as well, and such
//             keys never appear as ST keys — the old two-clause rule could not
//             see the class at all. It held 941 characters, among them 像
//             (U+50CF), whose TS line reads `像<TAB>像 象` and which is the
//             second character of 声像, the glossary's only accepted rendering
//             of `pan`. Self is excluded ANYWHERE in the value list, not
//             merely first: 18 characters (乾 剋 劄 吒 夥 徵 扞 於 昇 氾 祕 脩
//             蒐 薹 袷 谿 釐 陞) map to themselves in a later position and are
//             the ambiguous both-scripts set.
//   parsing   A key is the first TAB-separated field of every line that is
//             non-empty and does NOT begin with '#'; the values are the
//             whitespace-separated tokens of the second field. THE COMMENT
//             FILTER IS LOAD-BEARING: the served files carry a '#' header plus
//             895 '# @tofu-risk:' annotation lines, and counting those as keys
//             inflates the set from 4139 to 5047 and the literal from 12.5 KB
//             to 77 KB. The generator also asserts that every TS key is a
//             single code point and stops if one is not — a multi-character
//             key in a CHARACTER table means the parse is wrong.
//   size      3198 characters, 9660 bytes UTF-8
//   spotcheck IN  這 後 說 龍 幾
//             OUT 这 音 声 像 乾 徵 於 夥
//             Asserted by the generator before it wrote this block, and
//             re-asserted against this committed literal by
//             `node scripts/gen-zh-trad-only.js --verify scripts/i18n-zh-lint.js`,
//             which stops rather than accept a set that fails it.
//
//   regenerate:
//     curl -fsSL -o /tmp/TSCharacters.txt https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/TSCharacters.txt
//     curl -fsSL -o /tmp/STCharacters.txt https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/STCharacters.txt
//     node scripts/gen-zh-trad-only.js --ts /tmp/TSCharacters.txt --st /tmp/STCharacters.txt --target scripts/i18n-zh-lint.js
//
// The fetched files are never committed and never executed. Only these
// characters — inert data — cross the network-to-source boundary.
const TRAD_ONLY = new Set([...(
    '㑯㑳㑶㓨㗲㘚㜄㜏㜢㠏㠣㥮㩜㩳㩵㺏䁪䁻䃮䊷䋙䋚䋹䋻䍦䎱䓣䙡䜀䝼䡵䥇䥑䥕䥱䦛䦟䧢䮄䯀䰾䱷䱽䲁䲘䴉丟並亂亙亞佇佈佔併來侖侶侷俁係俔俠俥俬倀倆倈倉個們倖倫倲偉偑側偵偽傌傑傖傘備傢傭傯傳傴債傷傾僂僅僉僑僕僞僤僥僨僱價儀儁儂億儈儉儎儐儔儕儘償優儲儷儸儺儻儼兇兌兒兗內兩' +
    '冊冑冪凈凍凜凱別刪剄則剎剗剛剝剮剴創剷劃劇劉劊劌劍劏劑劚勁動務勛勝勞勢勣勩勱勳勵勸勻匭匯匱區協卹卻卽厙厠厤厭厲厴參叄叢吳吶呂咼員唄唸問啓啞啟啢喎喚喪喫喬單喲嗆嗇嗊嗎嗚嗩嗰嗶嘆嘍嘓嘔嘖嘗嘜嘩嘮嘯嘰嘵嘸嘽噁噓噚噝噠噥噦噯噲噴噸噹嚀嚇嚌嚐嚕嚙嚥嚦嚧嚨嚮嚲嚳嚴嚶囀' +
    '囁囂囅囈囉囌囑囪圇國圍園圓圖團垻埡埨埰執堅堊堖堝堯報場塊塋塏塒塗塚塢塤塵塸塹塿墊墜墠墮墰墳墶墻墾壇壋壎壓壗壘壙壚壜壞壟壠壢壩壪壯壺壼壽夠夢夾奐奧奩奪奬奮奼妝姍姦娙娛婁婦婭媧媯媰媼媽嫋嫗嫵嫺嫻嫿嬀嬃嬈嬋嬌嬙嬡嬤嬪嬰嬸孃孋孌孫學孻孿宮寀寢實寧審寫寬寵寶將專尋對' +
    '導尷屆屍屓屜屢層屨屬岡峯峴島峽崍崑崗崙崢崬嵐嵗嵽嵾嶁嶄嶇嶔嶗嶠嶢嶧嶨嶮嶸嶺嶼嶽巋巒巔巖巘巰巹帥師帳帶幀幃幓幗幘幟幣幫幬幷幹幾庫廁廂廄廈廎廕廚廝廞廟廠廡廢廣廩廬廳弒弔弳張強彄彆彈彌彎彔彙彠彥彫彲彿後徑從徠復徹恆恥悅悞悵悶悽惡惱惲惻愛愜愨愴愷愾慄態慍慘慚慟慣慤' +
    '慪慫慮慳慶慺慼慾憂憊憐憑憒憖憚憤憫憮憲憶懇應懌懍懞懟懣懤懨懲懶懷懸懺懼懾戀戇戔戧戩戰戱戲戶拋拚挩挱挾捨捫捱捲掃掄掆掗掙掛採揀揚換揮揯損搖搗搧搵搶摑摜摟摯摳摶摺摻撈撏撐撓撝撟撣撥撫撲撳撻撾撿擁擄擇擊擋擓擔據擠擡擣擬擯擰擱擲擴擷擺擻擼擽擾攄攆攏攔攖攙攛攜攝攢攣' +
    '攤攪攬敎敓敗敘敵數斂斃斆斕斬斷旂旣時晉晛晝暈暉暐暘暢暫曄曆曇曉曏曖曠曥曨曬書會朥朧朮東枴柵柺査桱桿梔梘梜條梟梲棄棊棖棗棟棡棧棲棶椏椲楊楓楨業極榘榦榪榮榲榿構槍槓槤槧槨槮槳槶槼樁樂樅樑樓標樞樢樣樧樫樳樸樹樺樿橈橋機橢橫橯檁檉檔檜檟檢檣檮檯檳檸檻櫃櫍櫓櫚櫛櫝櫞' +
    '櫟櫥櫧櫨櫪櫫櫬櫱櫳櫸櫻欄欅權欏欒欓欖欞欽歎歐歟歡歲歷歸歿殘殞殤殨殫殭殮殯殰殲殺殻殼毀毆毿氂氈氌氣氫氬氳汎汙決沒沖況泝洩洶浹浿涇涗涼淒淚淥淨淩淪淵淶淺渙減渢渦測渾湊湋湞湧湯溈準溝溫溮溳溼滄滅滌滎滙滬滯滲滷滸滻滾滿漁漊漍漚漢漣漬漲漵漸漿潁潑潔潕潙潚潛潤潯潰潷潿' +
    '澀澆澇澐澗澠澤澦澩澫澮澱澾濁濃濄濆濕濘濚濛濜濟濤濧濫濰濱濺濼濾瀂瀅瀆瀇瀉瀋瀏瀕瀘瀝瀟瀠瀦瀧瀨瀰瀲瀾灃灄灑灒灕灘灙灝灡灣灤灧灩災為烏烴無煉煒煙煢煥煩煬煱熅熒熗熰熱熲熾燀燁燈燉燒燖燙燜營燦燬燭燴燶燻燼燾爍爐爛爭爲爺爾牀牆牘牴牽犖犛犢犧狀狹狽猙猶猻獁獃獄獅獎獨獪' +
    '獫獮獰獱獲獵獷獸獺獻獼玀現琱琺琿瑋瑒瑣瑤瑩瑪瑲璉璊璕璗璡璣璦璫璯環璵璸璽璿瓅瓊瓏瓔瓚瓛甌甕產産畝畢畫異畵當疇疊痙痠痾瘂瘋瘍瘓瘞瘡瘧瘮瘲瘺瘻療癆癇癉癒癘癟癡癢癤癥癧癩癬癭癮癰癱癲發皁皚皰皸皺盃盜盞盡監盤盧盪眞眥眾睍睏睜睞瞘瞜瞞瞶瞼矇矓矚矯硃硜硤硨硯碕碩碭碸確' +
    '碼碽磑磚磠磣磧磯磽磾礄礎礐礙礦礪礫礬礱祿禍禎禕禡禦禪禮禰禱禿秈稅稈稏稜稟種稱穀穇穌積穎穠穡穢穩穫穭窩窪窮窯窵窶窺竄竅竇竈竊竪競筆筍筧筴箇箋箏箚節範築篋篔篠篢篤篩篳篸簀簍簑簞簡簣簫簹簽簾籃籅籌籔籙籛籜籟籠籤籩籪籬籮籲粵糉糝糞糧糰糲糴糶糹糾紀紂紃約紅紆紇紈紉紋' +
    '納紐紓純紕紖紗紘紙級紛紜紝紞紡紬紮細紱紲紳紵紹紺紼紿絀終絃組絅絆絎結絕絛絝絞絡絢給絨絪絰統絲絳絶絹絺綁綃綄綆綈綉綌綎綏綐綑經綖綜綝綞綠綡綢綣綧綪綫綬維綯綰綱網綳綴綵綸綹綺綻綽綾綿緄緇緊緋緑緒緓緔緗緘緙線緝緞締緡緣緦編緩緬緯緱緲練緶緹緻縈縉縊縋縐縑縕縗縛縝縞' +
    '縟縣縧縫縭縮縯縱縲縳縴縵縶縷縹總績繃繅繆繒織繕繚繞繡繢繩繪繫繭繮繯繰繳繶繸繹繻繼繽繾繿纁纆纇纈纊續纍纏纓纔纕纖纘纜缽罃罈罌罎罰罵罷羅羆羈羋羣羥羨義羶習翫翬翹翽耬耮聖聞聯聰聲聳聵聶職聹聽聾肅脅脈脛脣脫脹腎腖腡腦腫腳腸膃膕膚膞膠膢膩膽膾膿臉臍臏臘臚臟臠臢臥臨臺' +
    '與興舉舊舖舘艙艤艦艫艱艷芻茲荊莊莖莢莧華菴菸萇萊萬萴萵葉葒葤葦葯葷蒍蒓蒔蒕蒞蒼蓀蓆蓋蓮蓯蓴蓽蔄蔔蔘蔞蔣蔥蔦蔭蔯蔿蕁蕆蕎蕒蕓蕕蕘蕢蕩蕪蕭蕷薀薈薊薌薑薔薘薟薦薩薳薴薵薺藍藎藝藥藪藭藶藹藺蘀蘄蘆蘇蘊蘋蘚蘞蘟蘢蘭蘺蘿虆虉處虛虜號虧虯蛺蛻蜆蝀蝕蝟蝦蝨蝸螄螞螢螮螻螿蟄' +
    '蟈蟎蟣蟬蟯蟲蟳蟶蟻蠁蠅蠆蠍蠐蠑蠔蠟蠣蠨蠱蠶蠻衆衊術衕衚衛衝袞裊裏補裝裡製複褌褘褲褳褸褻襀襇襉襏襖襝襠襤襪襬襯襲襴覈見覎規覓視覘覡覥覦親覬覯覲覷覺覽覿觀觴觶觸訁訂訃計訊訌討訏訐訒訓訕訖託記訛訝訟訢訣訥訩訪設許訴訶診註証詀詁詆詎詐詒詔評詖詗詘詛詝詞詠詡詢詣試詩' +
    '詪詫詬詭詮詰話該詳詵詷詼詿誄誅誆誇誌認誑誒誕誘誚語誠誡誣誤誥誦誨說説誰課誶誹誼誾調諂諄談諉請諍諏諑諒諓論諗諛諜諝諞諟諡諢諤諦諧諫諭諮諱諲諳諴諶諷諸諺諼諾謀謁謂謄謅謊謎謏謐謔謖謗謙謚講謝謠謡謨謫謬謭謳謹謾譁證譎譏譓譖識譙譚譜譞譟譫譭譯議譴護譸譽譾讀讅變讋讌讎' +
    '讒讓讕讖讚讜讞豈豎豐豔豬豶貍貓貙貝貞貟負財貢貧貨販貪貫責貯貰貲貳貴貶買貸貺費貼貽貿賀賁賂賃賄賅資賈賊賑賒賓賕賙賚賜賞賠賡賢賣賤賦賧質賫賬賭賰賴賵賺賻購賽賾贄贅贇贈贊贋贍贏贐贓贔贖贗贛贜赬趕趙趨趲跡踐踰踴蹌蹕蹟蹠蹣蹤蹺躂躉躊躋躍躎躑躒躓躕躚躡躥躦躪軀車軋軌軍' +
    '軏軑軒軔軛軝軟軤軫軲軸軹軺軻軼軾較輄輅輇輈載輊輋輒輓輔輕輗輛輜輝輞輟輥輦輩輪輬輮輯輳輶輸輻輾輿轀轂轄轅轆轉轍轎轔轟轡轢轤辦辭辮辯農迴逕這連週進遊運過達違遙遜遞遠遡適遲遶遷選遺遼邁還邇邊邏邐郟郵鄆鄉鄒鄔鄖鄧鄩鄭鄰鄲鄳鄴鄶鄺酇酈醃醜醞醟醣醫醬醱醲釀釁釃釅釋釒釓' +
    '釔釕釗釘釙針釣釤釦釧釩釴釵釷釹釺釾釿鈀鈁鈃鈄鈅鈇鈈鈉鈍鈎鈐鈑鈒鈔鈕鈞鈡鈣鈥鈦鈧鈮鈰鈳鈴鈷鈸鈹鈺鈽鈾鈿鉀鉅鉆鉈鉉鉊鉋鉍鉑鉕鉗鉚鉛鉝鉞鉢鉤鉥鉦鉧鉬鉭鉮鉳鉶鉷鉸鉺鉻鉿銀銃銅銈銍銑銓銖銘銚銛銜銠銣銥銦銨銩銪銫銬銱銳銶銷銹銻銼鋁鋃鋅鋇鋌鋏鋐鋒鋗鋙鋝鋟鋣鋤鋥鋦鋨鋩鋪鋭' +
    '鋮鋯鋰鋱鋶鋸鋹鋼錀錁錄錆錇錈錏錐錒錕錘錙錚錛錞錟錠錡錢錤錦錨錩錫錮錯録錳錶錸錼鍀鍁鍃鍅鍆鍇鍈鍊鍋鍍鍔鍘鍚鍛鍠鍤鍥鍩鍬鍭鍰鍵鍶鍺鍼鍾鎂鎄鎇鎊鎌鎓鎔鎖鎘鎚鎛鎝鎡鎢鎣鎦鎧鎩鎪鎬鎭鎮鎰鎲鎳鎵鎶鎸鎿鏃鏇鏈鏌鏍鏏鏐鏑鏗鏘鏜鏝鏞鏟鏡鏢鏤鏨鏰鏵鏷鏹鏺鏻鏽鐃鐄鐇鐋鐍鐏鐐鐒鐓' +
    '鐔鐘鐙鐝鐠鐥鐦鐧鐨鐩鐫鐮鐯鐲鐳鐵鐶鐸鐺鐽鐿鑄鑊鑌鑑鑒鑔鑕鑞鑠鑣鑥鑪鑭鑰鑱鑲鑷鑹鑼鑽鑾鑿钁钂長門閂閃閆閈閉開閌閎閏閑閒間閔閘閡閣閤閥閨閩閫閬閭閱閲閶閹閻閼閽閾閿闃闆闇闈闉闊闋闌闍闐闑闒闓闔闕闖關闞闠闡闢闤闥陘陝陣陰陳陸陽隉隊階隑隕際隤隨險隮隯隱隴隸隻雋雖雙雛' +
    '雜雞離難雲電霑霢霧霽靂靄靆靈靉靚靜靝靦靨鞏鞝鞦鞽韁韃韆韉韋韌韍韓韙韜韝韞韻響頁頂頃項順頇須頊頌頍頎頏預頑頒頓頔頗領頜頠頡頤頦頫頭頮頰頲頴頵頷頸頹頻頽顆題額顎顏顒顓顔顗願顙顛類顢顥顧顫顬顯顰顱顳顴風颭颮颯颱颳颶颸颺颻颼飀飄飆飈飛飠飢飣飥飩飪飫飭飯飱飲飴飼飽飾' +
    '飿餃餄餅餈餉養餌餎餏餑餒餓餕餖餗餘餚餛餜餞餡館餬餱餳餵餶餷餸餺餼餾餿饁饃饅饈饉饊饋饌饑饒饗饘饜饞饢馬馭馮馱馳馴馹馼駁駃駉駐駑駒駓駔駕駘駙駛駝駟駡駢駪駭駰駱駸駼駿騁騂騄騅騊騌騍騎騏騑騖騙騞騠騤騧騫騭騮騰騱騵騶騷騸騾驀驁驂驃驄驅驊驌驍驎驏驕驗驚驛驟驢驤驥驦驪驫' +
    '骯髏髒體髕髖髮鬆鬍鬚鬢鬥鬧鬨鬩鬮鬱鬹魎魘魚魛魟魢魨魯魴魷魺鮀鮁鮃鮆鮈鮊鮋鮍鮎鮐鮑鮒鮓鮚鮜鮝鮞鮟鮠鮡鮣鮦鮪鮫鮭鮮鮳鮶鮸鮺鯀鯁鯇鯉鯊鯒鯔鯕鯖鯗鯛鯝鯡鯢鯤鯧鯨鯪鯫鯰鯴鯷鯻鯽鯿鰁鰂鰃鰆鰈鰉鰊鰌鰍鰏鰐鰒鰓鰛鰜鰟鰠鰣鰤鰥鰧鰨鰩鰭鰮鰱鰲鰳鰵鰶鰷鰹鰺鰻鰼鰾鱀鱂鱅鱇鱈鱉鱒鱔' +
    '鱖鱗鱘鱚鱝鱟鱠鱣鱤鱧鱨鱭鱯鱲鱷鱸鱺鳥鳧鳩鳬鳲鳳鳴鳶鳾鴆鴇鴉鴒鴕鴛鴝鴞鴟鴣鴦鴨鴯鴰鴴鴷鴻鴿鵁鵂鵃鵏鵐鵑鵒鵓鵜鵝鵟鵠鵡鵪鵬鵮鵯鵰鵲鵷鵾鶄鶇鶉鶊鶓鶖鶘鶚鶠鶡鶥鶩鶪鶬鶯鶱鶲鶴鶹鶺鶻鶼鶿鷀鷁鷂鷄鷉鷊鷓鷖鷗鷙鷚鷟鷥鷦鷫鷭鷯鷲鷳鷴鷸鷹鷺鷽鸂鸇鸊鸌鸏鸑鸕鸘鸚鸛鸝鸞鹵鹹鹺鹼' +
    '鹽麗麥麩麪麫麬麯麳麴麵麼黃黌點黨黲黴黶黷黽黿鼂鼉鼕鼴齊齋齎齏齒齔齕齗齘齙齜齟齠齡齣齦齧齪齬齮齯齲齶齷齼龍龎龐龑龔龕龜鿁鿓𠁞𠗣𡃕𡅏𡑍𡑭𡓾𡔖𡞵𡠹𡢃𡮉𡮣𡳳𡻕𡾱𢣚𢶫𢹿𣈶𣙎𣞻𣠩𣠲𣯶𣾷𤁣𤅶𤓩𤪺𤫩𤳸𥊝𥌃𥕥𥖅𥗽𥢢𥸠𥼽𦘧𦣎𦪙𧜗𧜵𧝞𧟀𧩙𧵳𧶧𨊰𨊸𨋢𨤻𨦫𨧀𨧜𨨏𨭆𨭎𨯅𩞯𩠴𩣑𩶘𰻞'
)]);
// ZH_TRAD_ONLY_SENTINEL_END

// ── normalisation ───────────────────────────────────────────────────────────
// The English side is normalised exactly as the French lint normalises it, so
// the two glossaries stay interchangeable for Stage-2 tooling.
function normEn(s) {
    return String(s).replace(/’/g, "'").replace(/[   ]/g, ' ')
        .trim().replace(/\s+/g, ' ').replace(/\.$/, '').toLowerCase();
}
// The Chinese side drops a trailing period in EITHER script — a caption written
// with the ideographic full stop is the same caption.
function normZh(s) {
    return String(s).replace(/[   ]/g, ' ').trim().replace(/\s+/g, ' ')
        .replace(/[.。．]$/, '').toLowerCase();
}

const TERMS = Object.fromEntries(Object.entries(G.TERMS).map(([k, v]) => [normEn(k), v]));
const BUDGETS = Object.fromEntries(Object.entries(G.BUDGETS).map(([k, v]) => [normEn(k), v]));

const HAN = /\p{Script=Han}/u;

// ── Z1 ──────────────────────────────────────────────────────────────────────
// Mask every Latin/unit token before looking for ASCII punctuation. This mask
// is the rule's whole difficulty: `延迟 (delay) 20 ms` is legal and must stay
// silent, while `混音, 深度.` must fire.
function maskLatin(s) {
    let out = String(s);
    // A parenthesised aside with no Han inside is a Latin gloss; the parens
    // belong to it and go with it.
    out = out.replace(/\(([^()]*)\)/g, (m, inner) => (HAN.test(inner) ? m : ' '));
    // A MARKUP-SPLIT label: a single unmatched '(' at end of string, whose
    // closing paren lives in a sibling DOM node and is therefore not in this
    // string to be balanced against. O-MicrotonalSampler's
    // label.floTokensBefore / floTokensBefore2 are the corpus shapes.
    // ORDER IS LOAD-BEARING: this runs AFTER the balanced replacement above,
    // so a genuine balanced pair is never half-consumed. ANCHORING AT END OF
    // STRING IS THE WHOLE DISCRIMINATOR: an unmatched '(' anywhere else still
    // fires (self-test Z1 violation C).
    out = out.replace(/\(\s*$/, ' ');
    // A Latin/number token, including any ASCII punctuation BETWEEN two
    // alphanumerics (1.5, kHz/ms, don't, 20-40), an optional LEADING dot, and
    // a trailing percent.
    //
    // THE LEADING DOT is a file extension written bare: `.scl`, `.kbm`,
    // `.omspreset`. The glossary compels these renderings, so Z1 flagging them
    // put two rules in this one file in disagreement (Z5 compels the root, Z1
    // then flagged it). DISCLOSED BLINDNESS, the price of the widening: a Han
    // character followed IMMEDIATELY by an ASCII period and then Latin with no
    // space between (e.g. 混音.mix) now reads as an extension token and its
    // period goes unseen. That shape does not occur in the corpus and would be
    // a typing slip rather than a typography choice; the wider, spaced form
    // `混音. Mix` still fires because the period there is followed by a space.
    // Same house style as the Z6 coverage disclosure: an inert or blinded rule
    // that does not announce itself is the failure this rollout exists to
    // prevent.
    out = out.replace(/\.?[A-Za-z0-9]+(?:[.,:'’\/\-][A-Za-z0-9]+)*%?/g, ' ');
    return out;
}
const ASCII_PUNCT = /[,.:;?!()]/;
function ruleZ1(zh) {
    if (!HAN.test(zh)) return false;   // a pure-Latin entry is not Han prose
    return ASCII_PUNCT.test(maskLatin(zh));
}

// ── Z2 ──────────────────────────────────────────────────────────────────────
function ruleZ2(zh) {
    return / [:;!?%：；！？％]/.test(zh);
}

// ── Z3 ──────────────────────────────────────────────────────────────────────
// Code-point iteration. CJK extension characters are surrogate pairs and
// charAt/index iteration splits them in half.
function ruleZ3(zh) {
    if (!TRAD_ONLY.size) return null;   // inert until the set is generated
    const hits = [...String(zh)].filter((ch) => TRAD_ONLY.has(ch));
    return hits.length ? [...new Set(hits)] : null;
}

// ── Z4 ──────────────────────────────────────────────────────────────────────
// Every boundary between a Latin/digit run and a Han run, classified by the
// gap between them.
function boundaryKinds(s) {
    const kinds = [];
    const re = /(?:[A-Za-z0-9]([    ]*)\p{Script=Han})|(?:\p{Script=Han}([    ]*)[A-Za-z0-9])/gu;
    let m;
    while ((m = re.exec(s)) !== null) {
        const gap = m[1] !== undefined ? m[1] : m[2];
        if (/[  ]/.test(gap)) kinds.push('thin');
        else if (gap === '') kinds.push('none');
        else if (gap === ' ') kinds.push('space');
        else kinds.push('other');
        re.lastIndex = m.index + 1;   // boundaries may overlap
    }
    return kinds;
}

// ── Z6 ──────────────────────────────────────────────────────────────────────
function ruleZ6(en, zh) {
    const b = BUDGETS[normEn(en)];
    if (!b || typeof b.maxChars !== 'number') return null;   // UNBUDGETED: inert
    const n = G.charCount(zh);
    return n > b.maxChars ? { n, max: b.maxChars } : null;
}

// ── Z7 ──────────────────────────────────────────────────────────────────────
function ruleZ7(zh) {
    // Full-width digits, upper- and lower-case Latin. NOT full-width
    // punctuation — that is the correct form and Z1 exists to require it.
    return /[０-９Ａ-Ｚａ-ｚ]/.test(zh);
}

// ── Z8 ──────────────────────────────────────────────────────────────────────
// A plain U+0020 between two Han code points. Stage 3 found this by READING
// O-Octagon's `puck` body; no rule could see it, because Z4 only classifies the
// gap at a boundary between a Latin/digit run and a Han run, and here there is
// Han on both sides.
//
// SCOPED TO THE PLAIN SPACE ON PURPOSE, and the scope is not an oversight.
// U+00A0, U+2009, U+200A and U+3000 between two Han characters are all
// defensible additions on their own, and they are deliberately NOT included:
// this rule's zero column is validated by agreeing with an INDEPENDENT
// standalone scan, and that scan is defined on the plain space. Widening the
// predicate here would break the agreement test and leave the rule resting on
// its own word. The other whitespace classes are already policed at their own
// boundaries — Z2 for U+00A0 before punctuation, Z4 for the thin spaces and
// for Latin/Han spacing consistency.
const HAN_SPACE_HAN = /\p{Script=Han} \p{Script=Han}/u;
function ruleZ8(zh) {
    return HAN_SPACE_HAN.test(zh);
}

// ── F1 ──────────────────────────────────────────────────────────────────────
// Chinese has no word delimiter, so containment is the only available test —
// the French stem/lookahead machinery has nothing to anchor to here.
function forbidden(zh, table) {
    const hay = normZh(zh);
    return Object.keys(table).filter((w) => hay.includes(normZh(w)));
}

// ── row extraction ──────────────────────────────────────────────────────────
function rowsFromModule(m) {
    const rows = [];
    for (const [k, v] of Object.entries(m.LABELS || {}))
        rows.push({ kind: 'label', key: k, en: v.en?.t ?? '', zh: v[LANG]?.t ?? '', zhObj: v[LANG] || null });
    for (const [k, v] of Object.entries(m.I18N || {})) {
        rows.push({ kind: 'title', key: k, en: v.en?.t ?? '', zh: v[LANG]?.t ?? '', zhObj: v[LANG] || null });
        if ((v.en?.b ?? '') !== '' || (v[LANG]?.b ?? '') !== '')
            rows.push({ kind: 'body', key: k, en: v.en?.b ?? '', zh: v[LANG]?.b ?? '', zhObj: v[LANG] || null, isBody: true });
    }
    return rows;
}

// ── the lint proper ─────────────────────────────────────────────────────────
// Rows in, findings out. Every entry point — a real plugin, a self-test fixture
// — goes through this one function, so a rule proven by the self-test is the
// same code that runs on the corpus.
function lintRows(rows, opts = {}) {
    // The self-test supplies its own forbidden tables. F1's MECHANISM has to be
    // provable even on a day when the production table is small — otherwise the
    // rule's proof would be hostage to its content.
    const FB_LABELS = opts.forbiddenLabels || G.FORBIDDEN_IN_LABELS;
    const FB_PROSE  = opts.forbiddenProse  || G.FORBIDDEN_IN_PROSE;
    const findings = [];
    const info = { sameAsEn: [], termNote: [], mt: [] };
    const withBoundaries = [];

    for (const r of rows) {
        if (!r.zh) continue;
        const exempt = !!(r.zhObj && typeof r.zhObj.termNote === 'string' && r.zhObj.termNote.trim() !== '');

        if (ruleZ1(r.zh)) findings.push({ code: 'Z1', ...r, note: 'ASCII punctuation in Han prose' });
        if (ruleZ2(r.zh)) findings.push({ code: 'Z2', ...r, note: 'U+00A0 before punctuation; zh full-width forms carry their own sidebearing' });
        const z3 = ruleZ3(r.zh);
        if (z3) findings.push({ code: 'Z3', ...r, note: `Traditional-only: ${z3.join(' ')}` });
        if (ruleZ7(r.zh)) findings.push({ code: 'Z7', ...r, note: 'full-width Latin or digits' });
        // Evaluated here, with the row-scoped rules, so it reaches BODIES as
        // well as labels and titles. The real defect Stage 3 found was in a
        // body, and a rule pushed only on the label branch would have missed it.
        if (ruleZ8(r.zh)) findings.push({ code: 'Z8', ...r, note: 'plain U+0020 between two Han characters — intra-Han whitespace the Latin/Han boundary census cannot see' });

        const kinds = boundaryKinds(r.zh);
        if (kinds.includes('thin'))
            findings.push({ code: 'Z4', ...r, note: 'thin space (U+2009/U+200A) between Latin and Han — no glyph in some faces' });
        if (kinds.length) withBoundaries.push({ row: r, kinds });

        if (r.isBody) {
            for (const w of forbidden(r.zh, FB_PROSE))
                findings.push({ code: 'F1', ...r, note: `"${w}" -> ${FB_PROSE[w]}` });
            continue;
        }

        // labels and tooltip titles from here on
        if (String(r.zh).trim() === String(r.en).trim())
            info.sameAsEn.push({ ...r, flagged: r.zhObj?.sameAsEn === true });
        if (exempt) info.termNote.push(r);

        const allowed = TERMS[normEn(r.en)];
        const accepted = (allowed || []).map(normZh).includes(normZh(r.zh));
        if (!exempt && allowed && !accepted)
            findings.push({ code: 'Z5', ...r, note: `"${r.en}" -> ${allowed.join(' | ')}` });
        // A rendering the glossary itself ACCEPTS for this English can never be
        // a forbidden rendering — the French precedent, kept verbatim.
        if (!exempt && !accepted)
            for (const w of forbidden(r.zh, FB_LABELS))
                findings.push({ code: 'F1', ...r, note: `"${w}" -> ${FB_LABELS[w]}` });

        const z6 = ruleZ6(r.en, r.zh);
        if (z6) findings.push({ code: 'Z6', ...r, note: `${z6.n} characters over a measured budget of ${z6.max}` });

        // R1 is entry-scoped: evaluated on labels and titles, never twice for
        // the title/body pair of one tooltip.
        const rev = r.zhObj ? r.zhObj.reviewed : undefined;
        if (!REVIEWED_ENUM.includes(rev))
            findings.push({ code: 'R1', ...r, note: `reviewed=${JSON.stringify(rev)} — must be 'mt' | 'bt' | 'native'` });
        else if (rev === 'mt') info.mt.push(r);
    }

    // Z4's consistency half is TABLE-scoped: one form or the other, across the
    // whole table. Neither form is wrong on its own.
    const counts = { space: 0, none: 0 };
    for (const w of withBoundaries)
        for (const k of w.kinds) if (k in counts) counts[k]++;
    if (counts.space > 0 && counts.none > 0) {
        const minority = counts.none <= counts.space ? 'none' : 'space';
        for (const w of withBoundaries)
            if (w.kinds.includes(minority))
                findings.push({ code: 'Z4', ...w.row,
                    note: `Latin/Han spacing is inconsistent in this table (${counts.space} spaced, ${counts.none} unspaced); this entry uses the minority form "${minority}"` });
    }

    return { findings, info, zhEntries: rows.filter((r) => !r.isBody && r.zhObj).length };
}

async function lintPlugin(name) {
    const rel = UI_ROOTS.find((r) => fs.existsSync(path.join(ROOT, 'plugins', name, r)));
    if (!rel) return { name, error: 'no i18n.js under either UI root' };
    let m;
    try { m = await import(pathToFileURL(path.join(ROOT, 'plugins', name, rel)).href); }
    catch (e) { return { name, error: `import failed: ${String(e.message).split('\n')[0]}` }; }
    const rows = rowsFromModule(m);
    return { name, rows: rows.length, ...lintRows(rows) };
}

// ── self-test ───────────────────────────────────────────────────────────────
// A rule that cannot be SHOWN to fire on a deliberate violation is not
// implemented — it is decorative. Every rule declares its own violation/control
// pair here. Fixtures that need a plugin-shaped ESM module are written to
// os.tmpdir() at run time and deleted; nothing is ever written under plugins/
// and no fixture is committed.
const MOD = (labels, i18n = '{}') =>
    `export const LANGUAGES = ['en', 'fr', 'zh-Hans'];\n`
    + `export const LABELS = ${labels};\n`
    + `export const I18N = ${i18n};\n`;

const ROW  = (zh, over = {}) => ({ kind: 'label', key: 'label.fixture', en: 'Fixture', zh, zhObj: { reviewed: 'bt' }, ...over });
const BODY = (zh, en = 'fixture prose') => ({ kind: 'body', isBody: true, key: 'tip.fixture', en, zh, zhObj: { reviewed: 'bt' } });

const SELF_TESTS = {
    Z1: {
        why: 'ASCII comma and period in Han prose, where the full-width forms belong',
        // Three violations, each a different discriminator for the maskLatin
        // widenings of 2026-09-04 (quick task 260904-q4j). All three fired
        // BEFORE those widenings and must still fire after:
        //   D  \u6df7\u97f3, \u6df1\u5ea6.  genuine Han-prose comma and period \u2014 the
        //      leading-dot extension mask must not swallow them.
        //   C  \u6df7\u97f3 ( \u6df1\u5ea6  an unmatched open paren that is NOT at
        //      end of string \u2014 the end-of-string paren mask must not reach it.
        //   E  \u6df7\u97f3 (\u6df1\u5ea6)  a BALANCED ASCII pair with Han inside. Han in
        //      parens takes the full-width forms, so the balanced branch
        //      deliberately keeps this; the end-of-string branch must not
        //      reach around it and half-consume the pair.
        violation: [{ rows: [BODY('\u6df7\u97f3, \u6df1\u5ea6.')] },
                    { rows: [BODY('\u6df7\u97f3 ( \u6df1\u5ea6')] },
                    { rows: [BODY('\u6df7\u97f3 (\u6df1\u5ea6)')] }],
        // Controls 1-3 are the original set. The second is the rule's whole
        // difficulty: ASCII punctuation INSIDE a Latin or unit token is legal
        // and must stay silent. Controls A and B were added RED \u2014 both FIRED
        // before the maskLatin fix, which is what proves they are pointed at
        // the branch the fix changed. They are the two corpus shapes the fix
        // exists for:
        //   A  \u8f7d\u5165 .scl  a leading-dot file extension, compelled by the
        //      glossary itself (O-MicrotonalSampler load/save labels).
        //   B  \u5426\u5219\u4f7f\u7528\u6587\u4ef6\u540d\u6807\u8bb0 (  a markup-split label whose
        //      closing paren lives in a sibling DOM node.
        control: [{ rows: [BODY('\u6df7\u97f3\uff0c\u6df1\u5ea6\u3002')] },
                  { rows: [BODY('\u5ef6\u8fdf (delay) 20 ms')] },
                  { rows: [BODY('\u622a\u6b62\u9891\u7387 1.5 kHz\uff0c\u8303\u56f4 20 Hz-20 kHz\u3002')] },
                  { rows: [BODY('\u8f7d\u5165 .scl')] },
                  { rows: [BODY('\u5426\u5219\u4f7f\u7528\u6587\u4ef6\u540d\u6807\u8bb0 (')] }],
    },
    Z2: {
        why: 'a U+00A0 before punctuation — the deliberate inverse of French T3/T4/T5',
        violation: { rows: [ROW('\u6df1\u5ea6\u00a0\uff1a')] },
        control: [{ rows: [ROW('\u6df1\u5ea6\uff1a')] }, { rows: [ROW('50%')] }],
    },
    Z3: {
        why: 'a Traditional-only character inside a zh-Hans table',
        // Three violations. The two single-character specs are genuinely
        // Traditional-only and keep the rule proven to FIRE after the
        // 2026-09-04 set correction narrowed it by 941 characters.
        violation: [{ rows: [ROW('\u9019\u500b\u8072\u97f3')] },
                    { rows: [ROW('\u8aaa')] },
                    { rows: [ROW('\u9f8d')] }],
        // The second control is the exact false positive the old derivation
        // produced: \u58f0\u50cf is the glossary's ONLY accepted rendering of
        // `pan` (scripts/i18n-zh-glossary.js), inherited by pan rnd / pan
        // spray / pan sync, and \u50cf was in the set because its own TS value
        // list contains itself. The third is one of the 18 both-scripts
        // characters that map to themselves in a LATER position \u2014 same class,
        // and the provenance block always said they must not be flagged.
        control: [{ rows: [ROW('\u8fd9\u4e2a\u58f0\u97f3')] },
                  { rows: [ROW('\u58f0\u50cf')] },
                  { rows: [ROW('\u4e7e')] }],
    },
    Z4: {
        why: 'Latin/Han spacing that is inconsistent across the table, and any thin space',
        // Two violations: the table-scoped inconsistency, and the unconditional
        // thin space. A thin space has no glyph in some of the faces this suite
        // ships and would render as a box where no gate looks.
        violation: [{ rows: [ROW('20 ms \u5ef6\u8fdf'), ROW('20ms\u5ef6\u8fdf')] },
                    { rows: [ROW('20ms\u2009\u5ef6\u8fdf')] }],
        // Consistently spaced throughout: neither form is wrong on its own.
        control: [{ rows: [ROW('20 ms \u5ef6\u8fdf'), ROW('50 Hz \u6df7\u54cd')] },
                  { rows: [ROW('20ms\u5ef6\u8fdf'), ROW('50Hz\u6df7\u54cd')] }],
    },
    Z5: {
        why: 'a glossary term rendered as something the glossary does not accept',
        violation: { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '\u5f3a\u5ea6', reviewed: 'bt' } } }`) },
        control: [{ module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '\u6df1\u5ea6', reviewed: 'bt' } } }`) },
                  // a termNote is THE reasoned exemption and must silence Z5
                  { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '\u5f3a\u5ea6', reviewed: 'bt', termNote: 'excitation strength, not modulation depth' } } }`) }],
    },
    Z6: {
        why: 'a rendering longer than the MEASURED character budget for its English key',
        // 7 code points against "depth", whose budget is 6 (62 px / 10 px).
        violation: { rows: [ROW('\u6df1\u5ea6\u6df1\u5ea6\u6df1\u5ea6\u6df1', { en: 'Depth' })] },
        // The same key at exactly 6 fits; and an UNBUDGETED key is inert at any
        // length, which is the design, not an oversight.
        control: [{ rows: [ROW('\u6df1\u5ea6\u6df1\u5ea6\u6df1\u5ea6', { en: 'Depth' })] },
                  { rows: [ROW('\u7535\u5e73\u7535\u5e73\u7535\u5e73\u7535\u5e73\u7535\u5e73\u7535\u5e73', { en: 'Level' })] }],
    },
    Z7: {
        why: 'full-width Latin letters or digits — a classic machine-translation artifact',
        violation: { rows: [ROW('\uff2c\uff26\uff2f \uff12')] },
        control: [{ rows: [ROW('LFO 2')] },
                  // full-width PUNCTUATION is the correct zh form and Z1 requires it
                  { rows: [ROW('\u6df7\u97f3\uff0c\u6df1\u5ea6\u3002')] }],
    },
    F1: {
        why: 'a rendering listed as forbidden for a label or a body',
        violation: [{ rows: [ROW('\u6df7\u97f3', { en: 'Mix' })], opts: { forbiddenLabels: { '\u6df7\u97f3': '\u6df7\u5408 — \u6df7\u97f3 is the mixing PROCESS' } } },
                    { rows: [BODY('\u8fd9\u4e2a\u63d2\u5934\u7684\u589e\u76ca')], opts: { forbiddenProse: { '\u63d2\u5934': '\u63d2\u4ef6 — \u63d2\u5934 is an electrical plug' } } }],
        // A rendering the glossary itself ACCEPTS for this English can never be a
        // forbidden one, and a termNote exempts the entry from F1 as well as Z5.
        // Both carried over from the French precedent verbatim.
        control: [{ rows: [ROW('\u6df7\u5408', { en: 'Mix' })], opts: { forbiddenLabels: { '\u6df7\u97f3': '\u6df7\u5408' } } },
                  { rows: [ROW('\u6df7\u97f3', { en: 'Mix', zhObj: { reviewed: 'bt', termNote: 'this control mixes down, it is not a wet/dry blend' } })], opts: { forbiddenLabels: { '\u6df7\u97f3': '\u6df7\u5408' } } }],
    },
    R1: {
        why: "a zh-Hans entry with no reviewed flag, or one outside 'mt'|'bt'|'native'",
        violation: [{ module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '\u6df1\u5ea6' } } }`) },
                    { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '\u6df1\u5ea6', reviewed: true } } }`) }],
        control: [{ module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '\u6df1\u5ea6', reviewed: 'bt' } } }`) },
                  { module: MOD(`{ 'label.depth': { en: { t: 'Depth' }, 'zh-Hans': { t: '\u6df1\u5ea6', reviewed: 'mt' } } }`) }],
    },
    Z8: {
        why: 'a plain U+0020 between two Han code points \u2014 intra-Han whitespace the Latin/Han boundary census cannot see',
        // \u5199\u5165 \u6e90 \u2014 the shape Stage 3 found by READING O-Octagon's `puck`
        // body. No rule could see it: Z4 only classifies gaps at a Latin/Han
        // boundary, and this gap has Han on both sides.
        violation: { rows: [ROW('\u5199\u5165 \u6e90')] },
        // Control 1 is Z4's business, not Z8's, and Z8 must not poach it.
        // Control 2 is correct full-width punctuation with no space anywhere.
        // Control 3 has a space after \u8fdf, but what follows is an ASCII paren,
        // not a Han character.
        control: [{ rows: [ROW('20 ms \u5ef6\u8fdf')] },
                  { rows: [ROW('\u6df7\u97f3\uff0c\u6df1\u5ea6')] },
                  { rows: [BODY('\u5ef6\u8fdf (delay) 20 ms')] }],
    },
};

async function materialise(spec) {
    if (spec.rows) return spec.rows;
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'zh-lint-fixture-'));
    const file = path.join(dir, 'i18n.mjs');
    try {
        fs.writeFileSync(file, spec.module, 'utf8');
        const m = await import(pathToFileURL(file).href);
        return rowsFromModule(m);
    } finally {
        fs.rmSync(dir, { recursive: true, force: true });
    }
}

async function sideFires(spec, code) {
    const rows = await materialise(spec);
    return lintRows(rows, spec.opts || {}).findings.some((f) => f.code === code);
}

async function selfTest() {
    console.log('i18n-zh-lint --self-test — every rule against a deliberate violation and a clean control\n');
    let proven = 0;
    for (const code of CODES) {
        const t = SELF_TESTS[code];
        if (!t) { console.log(`  SELF-TEST ${code} NOT YET IMPLEMENTED — declared, inert, and not counted`); continue; }
        const vios = [].concat(t.violation);
        const ctls = [].concat(t.control);
        let vio = true, ctl = false;
        for (const v of vios) if (!(await sideFires(v, code))) vio = false;
        for (const c of ctls) if (await sideFires(c, code)) ctl = true;
        if (vio && !ctl) { proven++; console.log(`  SELF-TEST ${code} FIRES on violation, silent on control   (${t.why})`); }
        else {
            const which = !vio && ctl ? 'silent on a violation AND fires on a control'
                : !vio ? 'silent on a violation' : 'fires on a control';
            console.log(`  SELF-TEST ${code} BROKEN: ${which}`);
        }
    }
    console.log(`\nSELF-TEST: ${proven}/${CODES.length}`);
}

// ── main ────────────────────────────────────────────────────────────────────
(async () => {
    if (argv.includes('--codes')) { console.log(CODES.join(' ')); return; }
    if (argv.includes('--self-test')) { await selfTest(); return; }

    const plugins = fs.readdirSync(path.join(ROOT, 'plugins'))
        .filter((n) => n.startsWith('O-') && fs.statSync(path.join(ROOT, 'plugins', n)).isDirectory())
        .filter((n) => !only || n === only).sort();
    if (!plugins.length) { console.error(`i18n-zh-lint: no plugin matches --plugin ${only}`); return; }

    console.log('i18n-zh-lint — Simplified Chinese typography and terminology');
    console.log(`  plugins: ${plugins.length}   (GATE: exit 2 on any finding; entries at reviewed:'mt' are counted, not failed)\n`);

    const totals = Object.fromEntries(CODES.map((c) => [c, 0]));
    let failedPlugins = 0, errors = 0, zhTotal = 0, sameAsEnTotal = 0, termNoteTotal = 0, mtTotal = 0;

    console.log('  ' + 'plugin'.padEnd(28) + ' rows   zh ' + CODES.map((c) => c.padStart(4)).join('') + '   total');
    const details = [];
    for (const name of plugins) {
        const r = await lintPlugin(name);
        if (r.error) { errors++; console.log(`  ${name.padEnd(28)} ERROR ${r.error}`); continue; }
        const per = Object.fromEntries(CODES.map((c) => [c, r.findings.filter((f) => f.code === c).length]));
        for (const c of CODES) totals[c] += per[c];
        const n = r.findings.length;
        if (n) failedPlugins++;
        zhTotal += r.zhEntries;
        sameAsEnTotal += r.info.sameAsEn.length; termNoteTotal += r.info.termNote.length; mtTotal += r.info.mt.length;
        console.log(`  ${name.padEnd(28)} ${String(r.rows).padStart(4)} ${String(r.zhEntries).padStart(4)} `
            + CODES.map((c) => String(per[c] || '·').padStart(4)).join('') + `   ${String(n).padStart(5)}`);
        if (n || r.info.termNote.length) details.push(r);
    }
    console.log('  ' + '─'.repeat(28 + 11 + CODES.length * 4 + 8));
    console.log('  ' + 'TOTAL'.padEnd(28) + '      ' + String(zhTotal).padStart(4) + ' '
        + CODES.map((c) => String(totals[c]).padStart(4)).join('')
        + `   ${String(Object.values(totals).reduce((a, b) => a + b, 0)).padStart(5)}`);

    for (const r of details) {
        console.log(`\n-- ${r.name}`);
        const shown = r.findings.slice(0, MAX_SHOWN);
        for (const f of shown) {
            const snip = f.zh.length > 90 ? f.zh.slice(0, 87) + '…' : f.zh;
            console.log(`  ${f.code}  ${f.kind.padEnd(5)} ${f.key.padEnd(34)} "${snip}"${f.note ? `   ← ${f.note}` : ''}`);
        }
        if (r.findings.length > shown.length) console.log(`  … ${r.findings.length - shown.length} more (--verbose)`);
        for (const t of r.info.termNote) console.log(`  EXEMPT (termNote) ${t.kind} ${t.key}: "${t.zh}" — ${t.zhObj.termNote}`);
    }

    console.log(`\n-- summary`);
    if (zhTotal === 0) {
        // A vacuity result is NOT a pass, and this tool says so in its own
        // words. There is no success banner on this branch by design.
        console.log(`  VACUITY: 0 zh-Hans entries found across ${plugins.length} plugins — nothing was checked.`);
        console.log(`  This is not a pass. It is the correct Stage 1 result: the rollout has not yet written any Chinese.`);
    } else {
        console.log(`  zh-Hans entries checked: ${zhTotal}   plugins with findings: ${failedPlugins} / ${plugins.length}${errors ? `   (${errors} could not be read)` : ''}`);
    }
    console.log(`  straight copies zh === en (info): ${sameAsEnTotal}   termNote exemptions (info): ${termNoteTotal}`);
    console.log(`  BELOW SHIP BAR — entries at reviewed:'mt' (machine draft, unchecked): ${mtTotal}`);
    // Z6's coverage is DISCLOSED, never silent. A rule that is inert on 549 of
    // 552 terms and does not say so is the "gate goes green on unchecked
    // content" failure this whole rollout exists to prevent.
    const glossaryTerms = Object.keys(G.TERMS);
    const budgeted = glossaryTerms.filter((t) => G.TERM_META[t] && G.TERM_META[t].maxChars !== null).length;
    const unbudgeted = glossaryTerms.length - budgeted;
    console.log(`  Z6 coverage: ${budgeted} of ${glossaryTerms.length} glossary terms carry a measured budget; ${unbudgeted} are UNBUDGETED and Z6 is inert on them — Stages 2-4 fill these from the check-ui-labels zh arm`);
    console.log(`  codes: Z1 ASCII punctuation  Z2 U+00A0 before punctuation  Z3 Traditional-only  Z4 Latin/CJK spacing  Z5 glossary  Z6 budget  Z7 full-width Latin  F1 forbidden  R1 reviewed enum  Z8 intra-Han space`);
    // ── the gate ────────────────────────────────────────────────────────────
    // Promoted from report-only on 2026-09-05 (Stage 4 wave 4b), against a
    // corpus measured at 0 findings across 43 plugins with --self-test 10/10.
    //
    // A findings count is the only thing that blocks. mtTotal is deliberately
    // NOT in this expression: authoring a table happens at reviewed:'mt', and
    // a gate that failed on machine drafts would make it impossible to commit
    // a table between authoring and the reverse read — which is exactly the
    // shape every wave of this rollout uses.
    //
    // A plugin that could not be READ blocks too. Its row printed ERROR and
    // contributed no findings, so without this the corpus zero would include
    // a plugin nothing was checked on — the vacuity failure this whole tool
    // is built to refuse.
    const findingTotal = Object.values(totals).reduce((a, b) => a + b, 0);
    if (findingTotal || errors) {
        console.log(`\nGATE FAILED — exit 2. ${findingTotal} finding(s) across ${failedPlugins} plugin(s)`
            + `${errors ? `, and ${errors} plugin(s) could not be read` : ''}.`);
        process.exit(2);
    }
    console.log(`\nGATE PASSED — exit 0. 0 findings across ${plugins.length} plugin(s).`
        + `${mtTotal ? `  (${mtTotal} entr${mtTotal === 1 ? 'y is' : 'ies are'} at reviewed:'mt' — counted, not failed.)` : ''}`);
})();
