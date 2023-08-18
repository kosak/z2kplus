// Copyright 2023 The Z2K Plus+ Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "z2kplus/backend/util/automaton/fuzzy_unicode.h"

#include <string_view>
#include "kosak/coding/coding.h"

namespace z2kplus::backend::util::automaton {

namespace {
// Credits:
//   Unicode generator: http://qaz.wtf/u/convert.cgi?text=Cinnabon
//   Unicode identifier: http://www.babelstone.co.uk/Unicode/whatisit.html
std::string_view rawExpansions[] = {
  "ⓐ🅐ａ𝐚𝖆𝒂𝓪𝕒𝚊𝖺𝗮𝙖𝘢⒜🇦🄰🅰󠁡áﾑคαаል𝔞äᴀȺₐᵃɐⒶＡ𝐀𝕬𝑨𝓐𝔸𝙰𝖠𝗔𝘼𝘈󠁁ÁД𝔄Äᴬàāãάăåवâ",
  "ⓑ🅑ｂ𝐛𝖇𝒃𝓫𝕓𝚋𝖻𝗯𝙗𝘣⒝🇧🄱🅱󠁢乃๒въጌ𝔟ḅʙƀᵇⒷＢ𝐁𝕭𝑩𝓑𝔹𝙱𝖡𝗕𝘽𝘉󠁂Б𝔅ḄɃᴮ",
  "ⓒ🅒ｃ𝐜𝖈𝒄𝓬𝕔𝚌𝖼𝗰𝙘𝘤⒞🇨🄲🅲󠁣ćƈ¢ςсር𝔠ċᴄȼᶜɔↄⒸＣ𝐂𝕮𝑪𝓒ℂ𝙲𝖢𝗖𝘾𝘊󠁃ĆҀℭĊȻↃ",
  "ⓓ🅓ｄ𝐝𝖉𝒅𝓭𝕕𝚍𝖽𝗱𝙙𝘥⒟🇩🄳🅳󠁤ɗ∂๔ↁዕ𝔡ḋᴅđᵈⒹＤ𝐃𝕯𝑫𝓓𝔻𝙳𝖣𝗗𝘿𝘋󠁄𝔇ḊĐᴰ",
  "ⓔ🅔ｅ𝐞𝖊𝒆𝓮𝕖𝚎𝖾𝗲𝙚𝘦⒠🇪🄴🅴󠁥é乇ﻉєэቿ𝔢ëᴇɇₑᵉǝɘⒺＥ𝐄𝕰𝑬𝓔𝔼𝙴𝖤𝗘𝙀𝘌󠁅ÉЄ𝔈ЁɆᴱƎēĕêẽěЀ⋷⋶еĒ已∈ё",
  "ⓕ🅕ｆ𝐟𝖋𝒇𝓯𝕗𝚏𝖿𝗳𝙛𝘧⒡🇫🄵🅵󠁦ｷिƒŦቻ𝔣ḟꜰᶠɟꟻⒻＦ𝐅𝕱𝑭𝓕𝔽𝙵𝖥𝗙𝙁𝘍󠁆𝔉Ḟ",
  "ⓖ🅖ｇ𝐠𝖌𝒈𝓰𝕘𝚐𝗀𝗴𝙜𝘨⒢🇬🄶🅶󠁧ǵﻭﻮБኗ𝔤ġɢǥᵍƃⒼＧ𝐆𝕲𝑮𝓖𝔾𝙶𝖦𝗚𝙂𝘎󠁇Ǵ𝔊ĠǤᴳ",
  "ⓗ🅗ｈ𝐡𝖍𝒉𝓱𝕙𝚑𝗁𝗵𝙝𝘩⒣🇭🄷🅷󠁨んɦнђЂዘ𝔥ḧʜħₕʰɥⒽＨ𝐇𝕳𝑯𝓗ℍ𝙷𝖧𝗛𝙃𝘏󠁈НℌḦĦᴴ",
  "ⓘ🅘ｉ𝐢𝖎𝒊𝓲𝕚𝚒𝗂𝗶𝙞𝘪⒤🇮🄸🅸󠁩íﾉٱιเіጎ𝔦ïɪɨᵢⁱıⒾＩ𝐈𝕴𝑰𝓘𝕀𝙸𝖨𝗜𝙄𝘐󠁉ІℑЇƗᴵīìĪϊîĭÌĩΪİỉїⅰί",
  "ⓙ🅙ｊ𝐣𝖏𝒋𝓳𝕛𝚓𝗃𝗷𝙟𝘫⒥🇯🄹🅹󠁪ﾌﻝנןјጋ𝔧ᴊɉⱼʲɾⒿＪ𝐉𝕵𝑱𝓙𝕁𝙹𝖩𝗝𝙅𝘑󠁊Ј𝔍Ɉᴶ",
  "ⓚ🅚ｋ𝐤𝖐𝒌𝓴𝕜𝚔𝗄𝗸𝙠𝘬⒦🇰🄺🅺󠁫ḱズᛕкጕ𝔨ḳᴋꝁₖᵏʞⓀＫ𝐊𝕶𝑲𝓚𝕂𝙺𝖪𝗞𝙆𝘒󠁋ḰЌ𝔎ḲꝀᴷ",
  "ⓛ🅛ｌ𝐥𝖑𝒍𝓵𝕝𝚕𝗅𝗹𝙡𝘭⒧🇱🄻🅻󠁬ĺﾚɭℓረ𝔩ḷʟłₗˡןⓁＬ𝐋𝕷𝑳𝓛𝕃𝙻𝖫𝗟𝙇𝘓󠁌Ĺ𝔏ḶŁᴸ⅃",
  "ⓜ🅜ｍ𝐦𝖒𝒎𝓶𝕞𝚖𝗆𝗺𝙢𝘮⒨🇲🄼🅼󠁭ḿﾶ๓мጠ𝔪ṁᴍₘᵐɯⓂＭ𝐌𝕸𝑴𝓜𝕄𝙼𝖬𝗠𝙈𝘔󠁍ḾМ𝔐Ṁᴹ",
  "ⓝ🅝ｎ𝐧𝖓𝒏𝓷𝕟𝚗𝗇𝗻𝙣𝘯⒩🇳🄽🅽󠁮ń刀กηภиክ𝔫ṅɴₙⁿᴎⓃＮ𝐍𝕹𝑵𝓝ℕ𝙽𝖭𝗡𝙉𝘕󠁎ŃИ𝔑Ṅᴺ",
  "ⓞ🅞ｏ𝐨𝖔𝒐𝓸𝕠𝚘𝗈𝗼𝙤𝘰⒪🇴🄾🅾󠁯őѻσ๏оዐ𝔬öᴏøₒᵒⓄＯ𝐎𝕺𝑶𝓞𝕆𝙾𝖮𝗢𝙊𝘖󠁏ŐФ𝔒ÖØᴼŌŏōةÕõÒēŎòóૅόбÔਠ",
  "ⓟ🅟ｐ𝐩𝖕𝒑𝓹𝕡𝚙𝗉𝗽𝙥𝘱⒫🇵🄿🅿󠁰ṕｱρקрየ𝔭ṗᴩᵽₚᵖⓅＰ𝐏𝕻𝑷𝓟ℙ𝙿𝖯𝗣𝙋𝘗󠁐ṔР𝔓ṖⱣᴾꟼ",
  "ⓠ🅠ｑ𝐪𝖖𝒒𝓺𝕢𝚚𝗊𝗾𝙦𝘲⒬🇶🅀🆀󠁱۹ợዒ𝔮ꝗⓆＱ𝐐𝕼𝑸𝓠ℚ𝚀𝖰𝗤𝙌𝘘󠁑𝔔Ꝗ",
  "ⓡ🅡ｒ𝐫𝖗𝒓𝓻𝕣𝚛𝗋𝗿𝙧𝘳⒭🇷🅁🆁󠁲ŕ尺ɼягѓዪ𝔯ṛʀɍᵣʳɹᴙⓇＲ𝐑𝕽𝑹𝓡ℝ𝚁𝖱𝗥𝙍𝘙󠁒ŔЯℜṚɌᴿ",
  "ⓢ🅢ｓ𝐬𝖘𝒔𝓼𝕤𝚜𝗌𝘀𝙨𝘴⒮🇸🅂🆂󠁳ś丂รѕነ𝔰ṡꜱₛˢꙅⓈＳ𝐒𝕾𝑺𝓢𝕊𝚂𝖲𝗦𝙎𝘚󠁓Ѕ𝔖ṠꙄ",
  "ⓣ🅣ｔ𝐭𝖙𝒕𝓽𝕥𝚝𝗍𝘁𝙩𝘵⒯🇹🅃🆃󠁴ｲՇтፕ𝔱ẗᴛŧₜᵗʇⓉＴ𝐓𝕿𝑻𝓣𝕋𝚃𝖳𝗧𝙏𝘛󠁔Г𝔗ṪŦᵀ",
  "ⓤ🅤ｕ𝐮𝖚𝒖𝓾𝕦𝚞𝗎𝘂𝙪𝘶⒰🇺🅄🆄󠁵úપυยцሁ𝔲üᴜᵾᵤᵘⓊＵ𝐔𝖀𝑼𝓤𝕌𝚄𝖴𝗨𝙐𝘜󠁕ŰЦ𝔘ÜᵁūŪϋŨÙÚŬủŭύỦû",
  "ⓥ🅥ｖ𝐯𝖛𝒗𝓿𝕧𝚟𝗏𝘃𝙫𝘷⒱🇻🅅🆅󠁶√۷νשሀ𝔳ṿᴠᵥᵛʌⓋＶ𝐕𝑽𝓥𝕍𝚅𝖵𝗩𝙑𝘝󠁖Ṿⱽ𐌡",
  "ⓦ🅦ｗ𝐰𝖜𝒘𝔀𝕨𝚠𝗐𝘄𝙬𝘸⒲🇼🅆🆆󠁷ẃฝωฬшሠ𝔴ẅᴡʷʍⓌＷ𝐖𝖂𝑾𝓦𝕎𝚆𝖶𝗪𝙒𝘞󠁗ẂЩ𝔚Ẅᵂ",
  "ⓧ🅧ｘ𝐱𝖝𝒙𝔁𝕩𝚡𝗑𝘅𝙭𝘹⒳🇽🅇🆇󠁸ﾒซχאхሸ𝔵ẍₓˣⓍＸ𝐗𝖃𝑿𝓧𝕏𝚇𝖷𝗫𝙓𝘟󠁘Ж𝔛Ẍ",
  "ⓨ🅨ｙ𝐲𝖞𝒚𝔂𝕪𝚢𝗒𝘆𝙮𝘺⒴🇾🅈🆈󠁹ӳﾘץуЎሃ𝔶ÿɏʸʎⓎＹ𝐘𝖄𝒀𝓨𝕐𝚈𝖸𝗬𝙔𝘠󠁙ӲЧ𝔜ŸɎ",
  "ⓩ🅩ｚ𝐳𝖟𝒛𝔃𝕫𝚣𝗓𝘇𝙯𝘻⒵🇿🅉🆉󠁺ź乙չጊ𝔷żᴢƶᶻⓏＺ𝐙𝖅𝒁𝓩ℤ𝚉𝖹𝗭𝙕𝘡󠁚ŹℨŻƵ"
};
}  // namespace

std::string_view getFuzzyEquivalents(char ch) {
  static std::string_view empty;
  static_assert(STATIC_ARRAYSIZE(rawExpansions) == 26);
  if (ch >= 'a' && ch <= 'z') {
    return rawExpansions[ch - 'a'];
  }
  return empty;
}
}  // namespace z2kplus::backend::util::automaton
