# virtual_gamepad

ゲームコントローラーの操作をネットワーク経由で送受信する。
通信は [SRT](https://github.com/Haivision/srt) を使って行う。

## システム構成

* 送信モジュール
* 中継モジュール
* 受信モジュール

の 3要素で構成されている。

この内、中継モジュールは、 [SRT](https://github.com/Haivision/srt) に含まれている
`srt-live-transmit` [これ](https://github.com/Haivision/srt/blob/master/docs/apps/srt-live-transmit.md)
を使うと便利。

送信モジュールと受信モジュールのサンプルは、ビルドして生成される `vgmpad_send` と `vgmpad_recv` 。

## ビルド手順

下記を実施すると、***install*** ディレクトリにサンプルアプリが出来る。
```bash
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make -j$(nproc) install
```

## 実行手順

1. ターミナルを 3つ開く
1. (ターミナル1) 中継モジュールを起動する
    * 入出力ともに listener モードにする
    * `srt-live-transmit srt://:ポート番号1 srt://:ポート番号2`
1. (ターミナル2) 送信モジュールを起動する
    * caller モードにする
    * `vgmpad_send 中継モジュールのホスト名または IP アドレス:ポート番号1`
1. (ターミナル3) 受信モジュールを起動する
    * caller モードにする
    * `vgmpad_recv 中継モジュールのホスト名または IP アドレス:ポート番号2`
