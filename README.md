# virtual_gamepad

ゲームコントローラーの操作をネットワーク経由で送受信する。
通信は [SRT](https://github.com/Haivision/srt) 又は UDP を使って行う。

## システム構成

* 送信モジュール
* 中継モジュール (これを使わず直接接続も可能)
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

UDP で通信する場合は、
* `srt-live-transmit` は **srt://** の代わりに **udp://** を付ける
* `vgmpad_send` と `vgmpad_recv` はポート番号の後ろに **/udp** を付ける

## ファイヤーウォール経由で http/https/ssh くらいしか通信が通らない場合

SSH と [stone](http://www.gcd.org/sengoku/stone/Welcome.ja.html) を使う。
SRT は UDP で通信するけど、SSH のポートフォワーディングは TCP しか通さないから、stone で TCP <---> UDP 変換をする。

### ケース1 受信モジュールがファイヤーウォール内にいる場合

中継モジュールが動作しているところを **SERVER-01** 、
受信モジュールが動作しているところを **RECV-02** として、
実行手順に下記の作業を追加する。

#### **SERVER-01** での作業

stone (TCP -> UDP 変換)を設定する。
```bash
stone localhost:ポート番号2/udp ポート番号2 &    # バックグラウンド実行
```

#### **RECV-02** での作業

SSH ポートフォワーディングを設定する。
```bash
ssh -L ポート番号2:localhost:ポート番号2 SERVER-01 -N &    # バックグラウンド実行のために & ではなく -f オプションでも大丈夫
```

stone (UDP -> TCP 変換)を設定する。
```bash
stone localhost:ポート番号2 ポート番号2/udp &    # バックグラウンド実行
```

受信モジュールは、localhost(**RECV-02**) に繋げば OK。
```bash
vgmpad_recv localhost:ポート番号2
```

上記の結果、通信経路は下記のようになる。
```bash
vgmpad_recv
--> RECV-02:14202 @ UDP --> RECV-02:14202 @ TCP    # stone UDP --> TCP
  --> RECV-02:ssh --> SERVER-01:ssh    # ssh port forwarding
    --> SERVER-01:14202 @ TCP --> SERVER-01:14202 @ UDP    # stone TCP --> UDP
      --> 中継モジュール(srt-live-transmit)
```

## 送信モジュールと受信モジュールが直接つながる場合(どちらかの IP アドレスがもう一方から分かる)

実行手順は下記のようにシンプルになる。

ローカル側がリモート側の IP アドレスを分かる想定。
1. ターミナルを 2つ開く
1. (リモート用ターミナル) 受信モジュールを起動する
    * listener モードにする(ホスト名または IP アドレスを指定しないで接続待ちモード)
    * `vgmpad_recv :ポート番号`
1. (ローカル用ターミナル) 送信モジュールを起動する
    * caller モードにする
    * `vgmpad_send 受信モジュールのホスト名または IP アドレス:ポート番号`
