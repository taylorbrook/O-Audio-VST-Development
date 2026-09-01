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

    ── REPORT TODAY, GATE LATER ──────────────────────────────────────────────

    This tool ships as a REPORT: it exits 0 regardless of what it finds. It is
    promoted to a GATE — exit 2 on any finding — only once the Stage 2 pilot
    (O-Chorus) is at zero findings. That is the exact lifecycle
    scripts/i18n-fr-lint.js went through: report-only on the day 43 of 43
    plugins failed it, a gate on 2026-08-31 once the rollout had taken every
    plugin to 0. Shipping it as a gate on day one would let a half-built lint
    block Stage 2, which is the failure this ordering exists to avoid.

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
const CODES = ['Z1', 'Z2', 'Z3', 'Z4', 'Z5', 'Z6', 'Z7', 'F1', 'R1'];
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
//   sources   https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/TSCharacters.txt
//             https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/STCharacters.txt
//   sha256    TSCharacters.txt  737c21c66f55a419dd6956cb3089476cdefc5a36877452631617696df1e5d925
//             STCharacters.txt  a0ca1601c70648cf48b33c3c6210ccbecc5c7eead4b4c3daf76587ba2c03582b
//   fetched   2026-09-01
//   rule      Traditional-only = keys(TSCharacters) \ keys(STCharacters).
//             A TS key is a character the Traditional->Simplified table has to
//             convert; if that same character is ALSO an ST key it is a live
//             simplified form too (or an ambiguous both-scripts character) and
//             must not be flagged. The difference is the set that can only be
//             Traditional.
//   parsing   A key is the first TAB-separated field of every line that is
//             non-empty and does NOT begin with '#'. THE COMMENT FILTER IS
//             LOAD-BEARING: the served files carry a '#' header plus 895
//             '# @tofu-risk:' annotation lines, and counting those as keys
//             inflates the set from 4139 to 5047 and the literal from 12.5 KB
//             to 77 KB. The generator also asserts that every TS key is a
//             single code point and stops if one is not — a multi-character
//             key in a CHARACTER table means the parse is wrong.
//   size      4139 characters, 12773 bytes UTF-8
//   spotcheck 這 IN, 後 IN, 这 OUT, 音 OUT — asserted by the generator before
//             it wrote this block; it stops rather than emit a set that fails.
//
//   regenerate:
//     curl -fsSL -o TSCharacters.txt https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/TSCharacters.txt
//     curl -fsSL -o STCharacters.txt https://raw.githubusercontent.com/BYVoid/OpenCC/master/data/dictionary/STCharacters.txt
//     node gen-trad-only.js scripts/i18n-zh-lint.js
//   (the generator is a throwaway kept in the task scratchpad, deliberately not
//    under scripts/: the OUTPUT is the artifact, the fetch is not a build step,
//    and this lint has no runtime dependency on anything it downloaded.)
//
// The fetched files are never committed and never executed. Only these
// characters — inert data — cross the network-to-source boundary.
const TRAD_ONLY = new Set([...(
    '㑮㑯㑳㑶㒓㓄㓨㔋㖮㗲㗿㘉㘓㘔㘚㛝㜄㜏㜐㜗㜢㜷㞞㟺㠏㠣㢗㢝㥮㦎㦛㦞㨻㩋㩜㩳㩵㪎㯤㰙㵗㵾㶆㷍㷿㸇㹽㺏㺜㻶㿖㿗㿧䀉䀹䁪䁻䂎䃮䅐䅳䆉䉑䉙' +
    '䉬䉲䉶䊭䊷䊺䋃䋔䋙䋚䋦䋹䋻䋼䋿䌈䌋䌖䌝䌟䌥䌰䍤䍦䍽䎙䎱䓣䕤䕳䖅䗅䗿䙔䙡䙱䚩䛄䛳䜀䜖䝭䝻䝼䞈䞋䞓䟃䟆䟐䠆䠱䡐䡩䡵䢨䤤䥄䥇䥑䥕䥗䥩䥯' +
    '䥱䦘䦛䦟䦯䦳䧢䪊䪏䪗䪘䪴䪾䫀䫂䫟䫴䫶䫻䫾䬓䬘䬝䬞䬧䭀䭃䭑䭔䭿䮄䮝䮞䮠䮫䮰䮳䮾䯀䯤䰾䱀䱁䱙䱧䱬䱰䱷䱸䱽䲁䲅䲖䲘䲰䳜䳢䳤䳧䳫䴉䴋䴬䴱' +
    '䴴䴽䵳䵴䶕䶲丟並乾亂亙亞仝佇佈佔併來侖侶侷俁係俓俔俠俥俬俱倀倆倈倉個們倖倫倲偉偑側偵偽傌傑傖傘備傢傭傯傳傴債傷傾僂僅僉像僑僕僞僤' +
    '僥僨僱價儀儁儂億儈儉儎儐儔儕儘償儣優儭儲儷儸儺儻儼兇兌兒兗內兩冊冑冪凈凍凙凜凱別刪剄則剋剎剗剛剝剮剴創剷剾劃劄劇劉劊劌劍劏劑劚勁' +
    '勑動務勛勝勞勢勣勩勱勳勵勸勻匭匯匱區協卹卻卽厙厠厤厭厲厴參叄叚叢吒吳吶呂咼員哩哯唄唓唸問啓啞啟啢喆喎喚喪喫喬單喲嗆嗇嗊嗎嗚嗩嗰嗶' +
    '嗹嘆嘍嘓嘔嘖嘗嘜嘩嘪嘮嘯嘰嘳嘵嘸嘺嘽噁噅噓噚噝噞噠噥噦噯噲噴噸噹嚀嚇嚌嚐嚕嚙嚛嚥嚦嚧嚨嚮嚲嚳嚴嚶嚽囀囁囂囃囅囈囉囌囑囒囪圇國圍園' +
    '圓圖團圞垻埡埨埬埰執堃堅堊堖堚堝堯報場塊塋塏塒塗塚塢塤塵塸塹塿墊墜墠墮墰墲墳墶墻墾壇壈壋壎壓壗壘壙壚壜壞壟壠壢壣壩壪壯壺壼壽夠夢' +
    '夥夾奐奧奩奪奬奮奼妝姍姦娙娛婁婡婦婭媈媧媯媰媼媽嫋嫗嫵嫺嫻嫿嬀嬃嬇嬈嬋嬌嬙嬡嬣嬤嬦嬪嬰嬸嬻孃孄孆孇孋孌孎孫學孻孾孿宮寀寠寢實寧審' +
    '寫寬寵寶將專尋對導尷屆屍屓屜屢層屨屩屬岡峯峴島峽崍崑崗崙崢崬嵐嵗嵼嵽嵾嶁嶄嶇嶈嶔嶗嶘嶠嶢嶧嶨嶮嶸嶹嶺嶼嶽巊巋巒巔巖巗巘巰巹帥師帳' +
    '帶幀幃幓幗幘幝幟幣幩幫幬幷幹幾庫廁廂廄廈廎廕廚廝廞廟廠廡廢廣廧廩廬廳弒弔弳張強彃彄彆彈彌彎彔彙彠彥彫彲彷彿後徑從徠復徵徹徿恆恥悅' +
    '悞悵悶悽惡惱惲惻愛愜愨愴愷愻愾慄態慍慘慚慟慣慤慪慫慮慳慶慺慼慾憂憊憐憑憒憖憚憢憤憫憮憲憶憸憹懀懇應懌懍懎懞懟懣懤懨懲懶懷懸懺懼懾' +
    '戀戇戔戧戩戰戱戲戶扞拋拚挩挱挾捨捫捱捲掃掄掆掗掙掚掛採揀揚換揮揯損搖搗搧搵搶摋摐摑摜摟摯摳摶摺摻撈撊撏撐撓撝撟撣撥撧撫撲撳撻撾撿' +
    '擁擄擇擊擋擓擔據擟擠擡擣擫擬擯擰擱擲擴擷擺擻擼擽擾攄攆攋攏攔攖攙攛攜攝攢攣攤攪攬敎敓敗敘敵數斂斃斅斆斕斬斷斸於旂旣昇時晉晛晝暈暉' +
    '暐暘暢暫曄曆曇曉曊曏曖曠曥曨曬書會朥朧朮東枴柵柺査桱桿梔梖梘梜條梟梲棄棊棖棗棟棡棧棲棶椀椏椲楇楊楓楨業極榘榦榪榮榲榿構槍槓槤槧槨' +
    '槫槮槳槶槼樁樂樅樑樓標樞樠樢樣樤樧樫樳樸樹樺樿橈橋機橢橫橯檁檉檔檜檟檢檣檭檮檯檳檵檸檻櫃櫅櫍櫓櫚櫛櫝櫞櫟櫠櫥櫧櫨櫪櫫櫬櫱櫳櫸櫻欄' +
    '欅欇權欍欏欐欑欒欓欖欘欞欽歎歐歟歡歲歷歸歿殘殞殢殤殨殫殭殮殯殰殲殺殻殼毀毆毊毿氂氈氌氣氫氬氭氳氾汎汙決沒沖況泝洩洶浹浿涇涗涼淒淚' +
    '淥淨淩淪淵淶淺淼渙減渢渦測渾湊湋湞湧湯溈準溝溡溫溮溳溼滄滅滌滎滙滬滯滲滷滸滻滾滿漁漊漍漚漢漣漬漲漵漸漿潁潑潔潕潙潚潛潣潤潯潰潷潿' +
    '澀澂澅澆澇澐澗澠澤澦澩澫澬澮澱澾濁濃濄濆濕濘濚濛濜濟濤濧濫濰濱濺濼濾濿瀂瀃瀅瀆瀇瀉瀋瀏瀕瀘瀝瀟瀠瀦瀧瀨瀰瀲瀾灃灄灍灑灒灕灘灙灝灡' +
    '灣灤灧灩災為烏烴無煇煉煒煙煢煥煩煬煱熂熅熉熌熒熓熗熚熡熰熱熲熾燀燁燈燉燒燖燙燜營燦燬燭燴燶燻燼燾爃爄爇爍爐爖爛爥爧爭爲爺爾牀牆牘' +
    '牴牽犇犖犛犞犢犧狀狹狽猌猙猶猻獁獃獄獅獊獎獨獩獪獫獮獰獱獲獵獷獸獺獻獼玀玁珼現琱琺琿瑋瑒瑣瑤瑩瑪瑲瑻瑽璉璊璕璗璝璡璣璦璫璯環璵璸' +
    '璼璽璾璿瓄瓅瓊瓏瓔瓕瓚瓛甌甕產産甦甯畝畢畫異畵當畼疇疊痙痠痮痾瘂瘋瘍瘓瘞瘡瘧瘮瘱瘲瘺瘻療癆癇癉癐癒癘癟癡癢癤癥癧癩癬癭癮癰癱癲發' +
    '皁皚皟皰皸皺盃盜盞盡監盤盧盨盪眝眞眥眾睍睏睜睞瞘瞜瞞瞤瞭瞶瞼矇矉矑矓矚矯硃硜硤硨硯碕碙碩碭碸確碼碽磑磚磠磣磧磯磽磾礄礆礎礐礒礙礦' +
    '礪礫礬礮礱祇祕祿禍禎禕禡禦禪禮禰禱禿秈稅稈稏稜稟種稱穀穇穌積穎穠穡穢穩穫穭窩窪窮窯窵窶窺竄竅竇竈竊竚竪竱競筆筍筦筧筴箇箋箏箚節範' +
    '築篋篔篘篠篢篤篩篳篸簀簂簍簑簞簡簢簣簫簹簽簾籃籅籋籌籔籙籛籜籟籠籤籩籪籬籮籲粵糉糝糞糧糰糲糴糶糹糺糾紀紂紃約紅紆紇紈紉紋納紐紓純' +
    '紕紖紗紘紙級紛紜紝紞紟紡紬紮細紱紲紳紵紹紺紼紿絀絁終絃組絅絆絍絎結絕絙絛絜絝絞絡絢絥給絧絨絪絰統絲絳絶絹絺綀綁綃綄綆綇綈綉綋綌綎' +
    '綏綐綑經綖綜綝綞綟綠綡綢綣綧綪綫綬維綯綰綱網綳綴綵綸綹綺綻綽綾綿緄緇緊緋緍緑緒緓緔緗緘緙線緝緞緟締緡緣緤緦編緩緬緮緯緰緱緲練緶緷' +
    '緸緹緻縈縉縊縋縍縎縐縑縕縗縛縝縞縟縣縧縫縬縭縮縯縰縱縲縳縴縵縶縷縸縹縺總績繂繃繅繆繈繏繐繒繓織繕繚繞繟繡繢繨繩繪繫繬繭繮繯繰繳繶' +
    '繷繸繹繻繼繽繾繿纁纆纇纈纊續纍纏纓纔纕纖纗纘纚纜缽罃罈罌罎罰罵罷羅羆羈羋羣羥羨義羵羶習翫翬翹翽耑耬耮聖聞聯聰聲聳聵聶職聹聻聽聾肅' +
    '脅脈脛脣脥脩脫脹腎腖腡腦腪腫腳腸膃膕膚膞膠膢膩膹膽膾膿臉臍臏臗臘臚臟臠臢臥臨臺與興舉舊舖舘艙艣艤艦艫艱艷芻茲荊莊莖莢莧菉菕華菴菸' +
    '萇萊萬萴萵葉葒葝葤葦葯葷蒍蒐蒓蒔蒕蒞蒭蒼蓀蓆蓋蓧蓮蓯蓴蓽蔄蔔蔘蔞蔣蔥蔦蔭蔯蔿蕁蕆蕎蕒蕓蕕蕘蕝蕢蕩蕪蕭蕳蕷蕽薀薆薈薊薌薑薔薘薟薦薩' +
    '薳薴薵薹薺藉藍藎藝藥藪藭藶藷藹藺蘀蘄蘆蘇蘊蘋蘚蘞蘟蘢蘭蘺蘿虆虉處虛虜號虧虯蛺蛻蜆蝀蝕蝟蝦蝨蝸螄螞螢螮螻螿蟂蟄蟈蟎蟘蟜蟣蟬蟯蟲蟳蟶' +
    '蟻蠀蠁蠅蠆蠍蠐蠑蠔蠙蠟蠣蠦蠨蠱蠶蠻蠾衆衊術衕衚衛衝衹袞袷裊裏補裝裡製複褌褘褲褳褸褻襀襇襉襏襓襖襗襘襝襠襤襪襬襯襰襲襴襵覆覈見覎規' +
    '覓視覘覛覡覥覦親覬覯覲覷覹覺覼覽覿觀觴觶觸訁訂訃計訊訌討訏訐訑訒訓訕訖託記訛訜訝訞訟訢訣訥訨訩訪設許訴訶診註証詀詁詆詊詎詐詑詒詓' +
    '詔評詖詗詘詛詝詞詠詡詢詣試詩詪詫詬詭詮詰話該詳詵詷詼詿誂誄誅誆誇誋誌認誑誒誕誘誚語誠誡誣誤誥誦誨說誫説誰課誳誴誶誷誹誺誼誾調諂諄' +
    '談諉請諍諏諑諒諓論諗諛諜諝諞諟諡諢諣諤諥諦諧諫諭諮諯諰諱諲諳諴諶諷諸諺諼諾謀謁謂謄謅謆謉謊謎謏謐謔謖謗謙謚講謝謠謡謨謫謬謭謯謱謳' +
    '謸謹謾譁譂譅譆證譊譎譏譑譓譖識譙譚譜譞譟譨譫譭譯議譴護譸譽譾讀讅變讋讌讎讒讓讕讖讚讜讞谿豈豎豐豔豬豵豶貍貓貗貙貝貞貟負財貢貧貨販' +
    '貪貫責貯貰貲貳貴貶買貸貺費貼貽貿賀賁賂賃賄賅資賈賊賑賒賓賕賙賚賜賝賞賟賠賡賢賣賤賦賧質賫賬賭賰賴賵賺賻購賽賾贃贄贅贇贈贉贊贋贍贏' +
    '贐贑贓贔贖贗贚贛贜赬趕趙趨趲跡踐踰踴蹌蹔蹕蹟蹠蹣蹤蹳蹺蹻躂躉躊躋躍躎躑躒躓躕躘躚躝躡躥躦躪軀軉車軋軌軍軏軑軒軔軕軗軛軜軝軟軤軨軫' +
    '軬軲軷軸軹軺軻軼軾軿較輄輅輇輈載輊輋輒輓輔輕輖輗輛輜輝輞輟輢輥輦輨輩輪輬輮輯輳輶輷輸輻輾輿轀轂轄轅轆轇轉轊轍轎轐轔轗轟轠轡轢轣轤' +
    '辦辭辮辯農迴迺逕這連週進遊運過達違遙遜遞遠遡適遱遲遶遷選遺遼邁還邇邊邏邐邨郟郵鄆鄉鄒鄔鄖鄟鄧鄩鄭鄰鄲鄳鄴鄶鄺酇酈醃醜醞醟醣醫醬醱' +
    '醲醶釀釁釃釅釋釐釒釓釔釕釗釘釙釚針釟釣釤釦釧釨釩釲釳釴釵釷釹釺釾釿鈀鈁鈃鈄鈅鈆鈇鈈鈉鈋鈍鈎鈐鈑鈒鈔鈕鈖鈗鈛鈞鈠鈡鈣鈥鈦鈧鈮鈯鈰鈲' +
    '鈳鈴鈷鈸鈹鈺鈽鈾鈿鉀鉁鉅鉆鉈鉉鉊鉋鉍鉑鉔鉕鉗鉚鉛鉝鉞鉠鉢鉤鉥鉦鉧鉬鉭鉮鉳鉶鉷鉸鉺鉻鉽鉾鉿銀銁銂銃銅銈銊銍銏銑銓銖銘銚銛銜銠銣銥銦' +
    '銨銩銪銫銬銱銳銶銷銹銻銼鋁鋂鋃鋅鋇鋉鋌鋏鋐鋒鋗鋙鋝鋟鋠鋣鋤鋥鋦鋨鋩鋪鋭鋮鋯鋰鋱鋶鋸鋹鋼錀錁錂錄錆錇錈錏錐錒錕錘錙錚錛錜錝錞錟錠錡' +
    '錢錤錥錦錨錩錫錮錯録錳錶錸錼錽鍀鍁鍃鍄鍅鍆鍇鍈鍉鍊鍋鍍鍒鍔鍘鍚鍛鍠鍤鍥鍩鍬鍭鍮鍰鍵鍶鍺鍼鍾鎂鎄鎇鎈鎊鎌鎍鎓鎔鎖鎘鎙鎚鎛鎝鎞鎡鎢鎣' +
    '鎦鎧鎩鎪鎬鎭鎮鎯鎰鎲鎳鎵鎶鎷鎸鎿鏃鏆鏇鏈鏉鏌鏍鏏鏐鏑鏗鏘鏚鏜鏝鏞鏟鏡鏢鏤鏥鏦鏨鏰鏵鏷鏹鏺鏻鏽鏾鐃鐄鐇鐈鐋鐍鐎鐏鐐鐒鐓鐔鐘鐙鐝鐠鐥' +
    '鐦鐧鐨鐩鐪鐫鐮鐯鐲鐳鐵鐶鐸鐺鐼鐽鐿鑀鑄鑉鑊鑌鑑鑒鑔鑕鑞鑠鑣鑥鑪鑭鑰鑱鑲鑴鑷鑹鑼鑽鑾鑿钁钂長門閂閃閆閈閉開閌閍閎閏閐閑閒間閔閗閘閝' +
    '閞閡閣閤閥閨閩閫閬閭閱閲閵閶閹閻閼閽閾閿闃闆闇闈闉闊闋闌闍闐闑闒闓闔闕闖關闞闠闡闢闤闥阪陘陝陞陣陰陳陸陽隉隊階隑隕際隤隨險隮隯隱' +
    '隴隸隻雋雖雙雛雜雞離難雲電霑霢霣霧霼霽靂靄靆靈靉靚靜靝靦靧靨鞏鞝鞦鞽鞾韁韃韆韉韋韌韍韓韙韚韛韜韝韞韠韻響頁頂頃項順頇須頊頌頍頎頏' +
    '預頑頒頓頔頗領頜頠頡頤頦頫頭頮頰頲頴頵頷頸頹頻頽顂顃顅顆題額顎顏顒顓顔顗願顙顛類顢顣顥顧顫顬顯顰顱顳顴風颭颮颯颰颱颳颶颷颸颺颻颼' +
    '颾飀飄飆飈飋飛飠飢飣飥飦飩飪飫飭飯飱飲飴飵飶飼飽飾飿餃餄餅餈餉養餌餎餏餑餒餓餔餕餖餗餘餚餛餜餞餡餦餧館餪餫餬餭餱餳餵餶餷餸餺餼餾' +
    '餿饁饃饅饈饉饊饋饌饑饒饗饘饜饞饟饠饢馬馭馮馯馱馳馴馹馼駁駃駉駊駎駐駑駒駓駔駕駘駙駚駛駝駞駟駡駢駤駧駩駪駫駭駰駱駶駸駻駼駿騁騂騃騄' +
    '騅騉騊騌騍騎騏騑騔騖騙騚騜騝騞騟騠騤騧騪騫騭騮騰騱騴騵騶騷騸騻騼騾驀驁驂驃驄驅驊驋驌驍驎驏驓驕驗驙驚驛驟驢驤驥驦驨驪驫骯髏髒體髕' +
    '髖髮鬆鬍鬖鬚鬠鬢鬥鬧鬨鬩鬮鬱鬹魎魘魚魛魟魢魥魦魨魯魴魵魷魺魽鮀鮁鮃鮄鮅鮆鮈鮊鮋鮍鮎鮐鮑鮒鮓鮚鮜鮝鮞鮟鮠鮡鮣鮤鮦鮪鮫鮭鮮鮯鮰鮳鮵鮶' +
    '鮸鮺鮿鯀鯁鯄鯆鯇鯉鯊鯒鯔鯕鯖鯗鯛鯝鯞鯡鯢鯤鯧鯨鯪鯫鯬鯰鯱鯴鯶鯷鯻鯽鯾鯿鰁鰂鰃鰆鰈鰉鰊鰋鰌鰍鰏鰐鰑鰒鰓鰕鰛鰜鰟鰠鰣鰤鰥鰦鰧鰨鰩鰫鰭' +
    '鰮鰱鰲鰳鰵鰶鰷鰹鰺鰻鰼鰽鰾鱀鱂鱄鱅鱆鱇鱈鱉鱊鱒鱔鱖鱗鱘鱚鱝鱟鱠鱢鱣鱤鱧鱨鱭鱮鱯鱲鱷鱸鱺鳥鳧鳩鳬鳲鳳鳴鳶鳷鳼鳽鳾鴀鴃鴅鴆鴇鴉鴐鴒鴔' +
    '鴕鴗鴛鴜鴝鴞鴟鴣鴥鴦鴨鴮鴯鴰鴲鴳鴴鴷鴻鴽鴿鵁鵂鵃鵊鵏鵐鵑鵒鵓鵚鵜鵝鵟鵠鵡鵧鵩鵪鵫鵬鵮鵯鵰鵲鵷鵾鶄鶇鶉鶊鶌鶒鶓鶖鶗鶘鶚鶠鶡鶥鶦鶩鶪' +
    '鶬鶭鶯鶰鶱鶲鶴鶹鶺鶻鶼鶿鷀鷁鷂鷄鷅鷉鷊鷐鷓鷔鷖鷗鷙鷚鷟鷣鷤鷥鷦鷨鷩鷫鷭鷯鷲鷳鷴鷷鷸鷹鷺鷽鷿鸂鸇鸊鸋鸌鸏鸑鸕鸗鸘鸚鸛鸝鸞鹵鹹鹺鹼鹽' +
    '麗麥麨麩麪麫麬麯麲麳麴麵麷麼黃黌點黨黲黴黶黷黽黿鼂鼉鼕鼴齊齋齎齏齒齔齕齗齘齙齜齟齠齡齣齦齧齩齪齬齭齮齯齰齲齴齶齷齼齾龍龎龐龑龓龔' +
    '龕龜龢龭龯鿁鿓𠁞𠌥𠏢𠐊𠗣𠞆𠠎𠬙𠼤𠽃𠿕𡂡𡃄𡃕𡃤𡄔𡄣𡅏𡅯𡑍𡑭𡓁𡓾𡔖𡞵𡟫𡠹𡢃𡮉𡮣𡳳𡸗𡹬𡻕𡽗𡾱𡿖𢍰𢠼𢣐𢣚𢣭𢤩𢤱𢤿𢯷𢶒𢶫𢷮𢹿𢺳𣈶𣋋𣍐𣙎𣜬𣝕' +
    '𣞻𣠩𣠲𣯩𣯴𣯶𣽏𣾷𣿉𤁣𤄷𤅶𤑳𤑹𤒎𤒻𤓌𤓎𤓩𤘀𤛮𤛱𤜆𤠮𤢟𤢻𤩂𤪺𤫩𤬅𤳷𤳸𤷃𤸫𤺔𥊝𥌃𥏝𥕥𥖅𥖲𥗇𥗽𥜐𥜰𥞵𥢢𥢶𥢷𥨐𥪂𥯤𥴨𥴼𥵃𥵊𥶽𥸠𥻦𥼽𥽖𥾯𥿊𦀖' +
    '𦂅𦃄𦃩𦅇𦅈𦆲𦒀𦔖𦘧𦟼𦠅𦡝𦢈𦣎𦧺𦪙𦪽𦱌𦾟𧎈𧒯𧔥𧕟𧜗𧜵𧝞𧞫𧟀𧡴𧢄𧦝𧦧𧩕𧩙𧩼𧫝𧬤𧭈𧭹𧳟𧵳𧶔𧶧𧷎𧸘𧹈𧽯𨂐𨄣𨅍𨆪𨇁𨇞𨇤𨇰𨇽𨈊𨈌𨊰𨊸𨊻𨋢𨌈𨍰' +
    '𨎌𨎮𨏠𨏥𨞺𨟊𨢿𨣈𨣞𨣧𨤻𨥛𨥟𨦫𨧀𨧜𨧰𨧱𨨏𨨛𨨢𨩰𨪕𨫒𨬖𨭆𨭎𨭖𨭸𨮂𨮳𨯅𨯟𨰃𨰋𨰥𨰲𨲳𨳑𨳕𨴗𨴹𨵩𨵸𨶀𨶏𨶮𨶲𨷲𨼳𨽏𩀨𩅙𩎖𩎢𩏂𩏠𩏪𩏷𩑔𩒎𩓣𩓥𩔑' +
    '𩔳𩖰𩗀𩗓𩗴𩘀𩘝𩘹𩘺𩙈𩚛𩚥𩚩𩚵𩛆𩛌𩛡𩛩𩜇𩜦𩜵𩝔𩝽𩞄𩞦𩞯𩟐𩟗𩠴𩡣𩡺𩢡𩢴𩢸𩢾𩣏𩣑𩣫𩣵𩣺𩤊𩤙𩤲𩤸𩥄𩥇𩥉𩥑𩦠𩧆𩭙𩯁𩯳𩰀𩰹𩳤𩴵𩵦𩵩𩵹𩶁𩶘𩶰𩶱' +
    '𩷰𩸃𩸄𩸡𩸦𩻗𩻬𩻮𩼶𩽇𩿅𩿤𩿪𪀖𪀦𪀾𪁈𪁖𪂆𪃍𪃏𪃒𪃧𪄆𪄕𪅂𪆷𪇳𪈼𪉸𪋿𪌭𪍠𪓰𪔵𪘀𪘯𪙏𪟖𪷓𫒡𫜦𰻞'
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
    // A Latin/number token, including any ASCII punctuation BETWEEN two
    // alphanumerics (1.5, kHz/ms, don't, 20-40) and a trailing percent.
    out = out.replace(/[A-Za-z0-9]+(?:[.,:'’\/\-][A-Za-z0-9]+)*%?/g, ' ');
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
        violation: { rows: [BODY('\u6df7\u97f3, \u6df1\u5ea6.')] },
        // Both controls matter. The second is the rule's whole difficulty: ASCII
        // punctuation INSIDE a Latin or unit token is legal and must stay silent.
        control: [{ rows: [BODY('\u6df7\u97f3\uff0c\u6df1\u5ea6\u3002')] },
                  { rows: [BODY('\u5ef6\u8fdf (delay) 20 ms')] },
                  { rows: [BODY('\u622a\u6b62\u9891\u7387 1.5 kHz\uff0c\u8303\u56f4 20 Hz-20 kHz\u3002')] }],
    },
    Z2: {
        why: 'a U+00A0 before punctuation — the deliberate inverse of French T3/T4/T5',
        violation: { rows: [ROW('\u6df1\u5ea6\u00a0\uff1a')] },
        control: [{ rows: [ROW('\u6df1\u5ea6\uff1a')] }, { rows: [ROW('50%')] }],
    },
    Z3: {
        why: 'a Traditional-only character inside a zh-Hans table',
        violation: { rows: [ROW('\u9019\u500b\u8072\u97f3')] },
        control: { rows: [ROW('\u8fd9\u4e2a\u58f0\u97f3')] },
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
    console.log(`  plugins: ${plugins.length}   (REPORT: exits 0 whatever it finds; becomes a gate once the O-Chorus pilot is at zero)\n`);

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
    console.log(`  codes: Z1 ASCII punctuation  Z2 U+00A0 before punctuation  Z3 Traditional-only  Z4 Latin/CJK spacing  Z5 glossary  Z6 budget  Z7 full-width Latin  F1 forbidden  R1 reviewed enum`);
    console.log(`\nREPORT ONLY — exit 0. This becomes a gate (exit 2) once the O-Chorus pilot is at zero findings.`);
})();
