---
layout: post
title: 久しぶりに LOUDS trie を実装して気付いたこと
---

## 概要

LOUDS を用いた trie をいくつか実装してみて，その中で気付いたことをまとめてみることにしました．
LOUDS およびに LOUDS を用いた trie については [LOUDS カテゴリーの記事一覧 - アスペ日記](https://takeda25.hatenablog.jp/archive/category/LOUDS) に詳しい説明があるため，ざっくりと省略します．

実装例は [s-yata/louds-trie: LOUDS-trie implementation example (C++)](https://github.com/s-yata/louds-trie) にあります．

## 要点

要点は以下の 3 つです．

- LOUDS の 0/1 を反転することで，少し実装しやすくなります．
- 階層別にデータを持つことで，整列済みのキー集合から無駄なく構築できます．
- 子ノードの探索に二分探索を使うことで，検索を高速化できます．

### LOUDS の 0/1 を反転すること

LOUDS というデータ構造では，各ノードについて，子ノードの数だけ 1 を並べ，その後に 0 を置きます．
この 0 と 1 を反転することで，実装が少しだけ楽になります．
楽になる理由は，実装に用いる CPU 命令 POPCNT (`__builtin_popcountll`), CTZ (`__builtin_ctzll`) との相性が良いからです．
簡潔ビットベクトルの実装によっては，速度に影響が出ることもあります．

簡潔データ構造の根幹にあたる 0/1 を反転することは暴挙に思えるかもしれませんが，実装に都合が良い方を選べば良いのではないでしょうか．

### 階層別にデータを持つこと

LOUDS を用いた trie の構成は以下のようになります．

```cpp
struct Trie {
  BitVector louds;
  BitVector outs;
  vector<uint8_t> labels;
};
```

しかし，この構成では，整列済みのキー集合から 1-pass で構築することができません．
一巡目では各階層のビット数やノード数がわからず，書き込み位置が定まらないからです．

各配列を階層別に持つように変更すれば，この問題は解決します．
変更後の構成は以下のようになります．

```cpp
struct Level {
  BitVector louds;
  BitVector outs;
  vector<uint8_t> labels;
};

struct Trie {
  vector<Level> levels;
};
```

階層別にすれば，一巡目でも各階層の書き込み位置が定まるため， 1-pass で構築できます．
入力し終わったキーを破棄しながら構築できるため，メモリ使用量も減らせます．
LOUDS trie はサイズが小さいため，ほかの trie を構築する準備段階として使っても良いと思います．

### 子ノードの探索に二分探索を使うこと

LOUDS trie の実装を探してみると，子ノードの探索に線形探索を使うものが見つかります．
子ノードが少なければ問題になりませんが，子ノードが多いと効率が悪くなります．
二分探索を使うことで高速化できる可能性が高いです．
[LOUDS++(6): trie改良試作(TAIL配列版) - sileのブログ](http://sile.hatenablog.jp/entry/20100619/1276985956) や [LOUDS++(8): trie - 検索速度向上 - sileのブログ](http://sile.hatenablog.jp/entry/20100625/1277423827) で言及されているのと同じことです．

整列済みのキー集合から構築した LOUDS trie であれば，子ノードのラベルは昇順に並んでいるはずです．
後は，子ノードの数さえわかれば，二分探索は簡単に実装できます．

重要になるのは子ノードの数を求める方法です．
LOUDS としては select で求めるのが正攻法ですが， select は若干重たい処理になるので，あまり使いたくありません．
子ノードが多いノードばかりということは稀なので， LOUDS を少し先まで見て，子ノードの終わりを示す 1 を探す方が速いケースが大半だと思います．

実装例から子ノードの数を求める部分を抜き出したものが以下になります．
子ノードの数は最大でも 256 なので， select は使わずに， LOUDS を 64 bits ずつチェックするようにしています．

```cpp
uint64_t end = node_pos;
uint64_t word = level.louds.words[end / 64] >> (end % 64);
if (word == 0) {
  end += 64 - (end % 64);
  word = level.louds.words[end / 64];
  while (word == 0) {
    end += 64;
    word = level.louds.words[end / 64];
  }
}
end += __builtin_ctzll(word);
uint64_t begin = node_id;
end = begin + end - node_pos;
```

ラベルの種類が多くて子ノードの数が大きくなる用途では，なかなか見つからないときに select を使うなど，最悪ケースを避ける工夫をするべきです．
たとえば，ラベルの型を `uint16_t` にするのであれば，回避策を入れておかないと最悪ケースで残念なことになります．
