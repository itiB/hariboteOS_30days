# 30日OS

<https://github.com/kamaboko123/30daysOS>

## 1日目

- フロッピーなんてないんだけど...
  - VirtualBoxでつくるqiita
  - <https://qiita.com/nao18/items/e1b9b77f154e4d5239e5>

## 2日目

- Makefileどう書き換えればいいんだ...
  - <http://tsurugidake.hatenablog.jp/entry/2017/08/15/202414>

## 3日目

- (AT)BIOS
  - INT(0x13) <http://oswiki.osask.jp/?%28AT%29BIOS>
- <https://qiita.com/pollenjp/items/8fcb9573cdf2dc6e2668>
- コンパイルのためにmformatなるものでなんか変えるらしい
  - `apt install mtools`
- <https://github.com/chikoyoshi/30days_OS>
- haribote.hrb の作成
  - bootpack.o が _GLOBAL_OFFSET_TABLE_ というシンボルを参照してしまう。
  - -> PIC のためのシンボルとのことだが、今回は不要なので CFLAGS に -fno-pic を追加して回避した。
  - <https://github.com/harrybotter30/haribote/blob/master/doc/harib00i.md>

## 4日目

とくにつまるところ無し

## 5日目

- またなんか持ってきやがったな！！！！！
  - Font探すのが面倒だったから[keita/30daysOS](https://github.com/keita99/30dayOS/blob/master/day_05/harib02e/hankaku.txt)さんからいただいてきた
  - 変換はこっちの方のコードを参考にさせていただいた
- `sprintf`が`#include<stdio.h>`じゃできないぞ～～～
  - 直接ライブラリを指定してみる？`@Makefile`
  - `/lib/x86_64-linux-gnu/libc.so.6`
  - `/usr/include/stdio.h`
  - うまくいかねぇ...
  - `stdio.h`の中見てみたけどexternで実質なにもないじゃん
  - <http://www.rcc.ritsumei.ac.jp/2017/1217_8241/>
  - あきらめてみんながやっているように`sprintf`を作っていただいたものをコピペ...
- nasmfunc.asmを書き換えて関数を増やした．

## 6日目

- 分割コンパイル
  - 結構なやんだ
  - gccのオプションとかまとめた
  - オブジェクトファイルとソースコードがっとまとめちゃってるけど
- 割り込み処理がいかなくて悩んでいた
  - `io_sti()`の宣言を`bootpack.h`に書きそびれていた
  - `init_pic()`を`int.c`で`init_pid()`とタイポしていた
- なんか重大なエラーが発生する
  - ![image](https://pbs.twimg.com/media/EP8Hm3TX0AE7UD1.png)
  - `nasmfunc.asm`の`io_in32:`が間違っていたので書き換え
  - `dsctbl.c`の中のアドレスを定数宣言した値にすべて書き換えたらうまく動いた！

## 7日目

- a なんかキーボードがリリースされないんだけど...
  - なおった
- VirtualboxをCUIから起動できる用にした
  - <https://qiita.com/apatchk/items/8b1029ceaee60d3e18a2>
  - imageが無かったエラるのどうにかしたいよねー
  - `/mnt/c/Program\ Files/Oracle/VirtualBox/VBoxManage.exe list vms | grep helloOS`

## 8日目

マウス動くようになった！！！やったね

## 9日目

- 2(harib06b)
  - なんか筆者はうまく出なかったらしいけど...うまく32MBって出るわ
  - gccのなんかっぽいしつぎのcはとばそう

## 10日目

なんか演算と->がわかりにくいから->の前後につけていたスペースをなくすことにした

## 11日目

なんか画面が崩れたらひたすらデバッグすることになるからわかりやすい．  
とくに詰まることはない(何時間やったかは知らない)．
