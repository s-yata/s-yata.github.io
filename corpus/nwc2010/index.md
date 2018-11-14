---
layout: page
title: 日本語ウェブコーパス 2010
---

本コーパスの HTML アーカイブは， [ipadic-2.7.0](https://sourceforge.jp/projects/ipadic/) の見出し語をシードとして，かつての [Yahoo! Web API](https://developer.yahoo.co.jp/) による検索結果に含まれるウェブページを収集したものです．
テキストの抽出においては，文字コードを UTF-8 に統一した後，いくつかの記号をデリミタとして文への分割をおこない，さらに文を構成する文字の種類や数によるフィルタリングを施しています．
N-gram コーパスについては，テキストアーカイブに出現する頻度 10 以上の N-gram を収録しています．

コーパス名の英語表記は Nihongo Web Corpus 2010 (NWC 2010) です．

名前|概要
--|--
[HTML アーカイブ](htmls)|約 1 億件の HTML 文書<br />圧縮時 197GB，展開時 3.25TB
[テキストアーカイブ](texts)|HTML アーカイブから抽出されたテキスト<br />圧縮時 69GB，展開時 396GB
[N-gram コーパス](ngrams)|形態素 N-gram：圧縮時 12.1GB，展開時 75.2GB<br />文字 N-gram：圧縮時 11.8GB，展開時 81.7GB
[タグ使用頻度](tags)|HTML タグの使用頻度（TF・DF）
[セクションターゲット](adsense)|セクションターゲット（AdSence）の用例
[ツールキット](https://code.google.com/archive/p/nwc-toolkit/)|テキストアーカイブや N-gram コーパスの作成ツール

本コーパスの作成においては，様々なウェブサービス，ツール，コーパスを利用させていただきました．
開発者・研究者の皆様に感謝いたします．

- コーパスの作成・保存・配布には [Amazon Web Services](https://aws.amazon.com/) を利用しています．
- ウェブ検索には Yahoo! JAPAN 検索 Web API を利用しました．
- ウェブコーパスのシードには [IPAdic](https://sourceforge.jp/projects/ipadic/) を利用しました．
- 文字コードの変換には [日本語用のパッチ](http://www2d.biglobe.ne.jp/~msyk/software/libiconv-patch.html) を適用した [libiconv](https://www.gnu.org/software/libiconv/) を利用しました．
- Unicode の正規化には [ICU](http://site.icu-project.org/) を利用しました．
- 形態素解析には [MeCab](https://taku910.github.io/mecab/) を利用しました．
- コーパスの圧縮には [XZ Utils](https://tukaani.org/xz/) を利用しました．
- 他にも様々なソフトウェアを利用しました．
