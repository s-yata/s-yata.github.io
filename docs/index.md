---
layout: page
title: Documentation
---

ライブラリの解説記事です．

名前|概要
--|--
[zlib の使い方](http://s-yata.jp/docs/zlib/)|zlib は圧縮アルゴリズムの一種である Deflate のライブラリであり，C#, Haskell, Java, Perl, Python, Ruby など，主要なプログラミング言語では，軒並み使えるように整備されています．圧縮・復元が高速なこともあり，ディスク領域の有効利用や通信量の削減を目的として，zlib は気軽に利用できます．本記事は，C 言語から zlib を利用する方法の解説になっています．
[libbzip2 の使い方](http://s-yata.jp/docs/libbzip2/)|libbzip2 は bzip2 で採用されている圧縮アルゴリズムのライブラリです．Burrows-Wheeler Transform (BWT) を用いることが特徴の一つであり，gzip と比較すると，圧縮・復元には時間がかかるものの，優れた圧縮率を示すことが多くなります．本記事は，C 言語から libbzip2 を利用する方法の解説になっています．
[XZ Utils の使い方](http://s-yata.jp/docs/xz-utils/)|XZ Utils は LZMA Utils の後継となるソフトウェアであり，Lempel–Ziv–Markov chain Algorithm（LZMA）の改良版である LZMA2 の実装になっています．圧縮には時間がかかるものの，bzip2 を上回る圧縮率を誇り，高速に復元できるという利点から，tar に採用されるなど，普及が進んでいます．本記事は，C 言語から XZ Utils を利用する方法の解説になっています．
[Snappy の使い方](http://s-yata.jp/docs/snappy/)|Snappy は高速な圧縮アルゴリズムのライブラリです．zlib や libbzip2 などの有名な圧縮ライブラリと比べると，圧縮率は低いものの，圧縮・伸長にかかる時間を桁一つ短縮することができます．圧縮・伸長速度が HDD の転送速度を上回るため，ディスク使用量や通信量の削減だけでなく，I/O の高速化を目的として利用することもできます．本記事は，C/C++ から Snappy を利用する方法の解説になっています．
