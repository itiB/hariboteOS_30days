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