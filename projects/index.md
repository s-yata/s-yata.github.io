---
layout: page
title: 旧プロジェクトの一覧
---

プロジェクト名|概要
--|--
[darts-clone](http://code.google.com/p/darts-clone/)|ダブル配列のライブラリである [Darts](http://chasen.org/~taku/software/darts/) のクローンです．要素サイズを半分にしたので，辞書のサイズは約半分になります．おまけに，各キーに関連付ける値が重複していれば，DAWG 化による圧縮が可能です．検索時間はほぼ同等です．
[dawgdic](http://code.google.com/p/dawgdic/)|darts-clone の派生として開発しました．成果を darts-clone に還元したため，インタフェースは異なるものの，データ構造は同じになっています．基本的なダブル配列と比べると，Prefix Trie の代わりに DAWG を使用，各要素を 4 bytes に圧縮，加算の代わりに排他的論理和を使用，オフセットに拡張用の bit を導入などの改造を施しています．
[marisa-trie](http://code.google.com/p/marisa-trie/)|Prefix/Patricia Trie の入れ子による辞書を構築・操作するためのライブラリとツールです．検索にかかる時間は長くなりますが，辞書のサイズをとても小さくできます．
[nwc-toolkith](ttp://code.google.com/p/nwc-toolkit/)|[NWC 2010](http://s-yata.jp/corpus/nwc2010/) の構築に利用したライブラリとツールの一部を改修したものです．使えなさそうで使えません．
[ssgnc](http://code.google.com/p/ssgnc/)|大規模な N-gram コーパスの検索に使えるライブラリとツールです．索引を構築することにより，AND 検索やフレーズ検索ができるようになります．地味に使えます．
[sumire-tries](http://code.google.com/p/sumire-tries/)|いろいろなトライを比較するためのライブラリです．TST とダブル配列に加えて，木構造の簡潔表現（LOB, LOUDS, LOUDS++）による実装が含まれています．
[taiju](http://code.google.com/p/taiju/)|簡潔データ構造を用いた 6 種類のトライを構築・操作できるライブラリです．メモリが 32GB もあれば，Google N-gram コーパスを丸ごとトライに格納することも可能です．
