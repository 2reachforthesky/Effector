# Effector

専用OSを搭載した楽器用低遅延エフェクタである。Raspberry Pi 4 の上で、Linux などの汎用OSを使わずに動作する（ベアメタル）。ベアメタル用の C++ ライブラリ [Circle](https://github.com/rsta2/circle) を土台とし、USB オーディオインターフェース YAMAHA AG03MK2 を通して楽器の音を入出力する。

卒業研究「専用OSを搭載した楽器用低遅延エフェクタの開発」の成果物であり、同じ Raspberry Pi 4 の上の Linux と、遅延・ジッタ・音切れを比較する。

## 全体の構成

```
ベース ──▶ AG03MK2 ──USB──▶ Raspberry Pi 4（Circle + Effector）
                 ▲                     │
ヘッドホン ◀─────┘◀────────USB─────────┘
                                       │
                 タクトスイッチ ──GPIO──▶│
                 LED ◀─────────GPIO─────│
                                       │
                 PC（Tera Term）◀─シリアル─┘  ログの確認
```

| 部分 | 役割 |
|---|---|
| AG03MK2 | 楽器の音をデジタルに変換して Pi に送り、Pi から返った音をヘッドホンに出す |
| Raspberry Pi 4 | 受け取った音にエフェクトをかけて返す。OS の代わりに Circle とこのリポジトリのプログラムが動く |
| タクトスイッチ・LED | エフェクトの選択、オン・オフ、測定状態への切り替えと、その表示 |
| PC | シリアル通信でログを表示する。動作には不要 |

## このリポジトリの構成

Circle 本体をまるごと含み、自分たちのコードと設計書を足している。

| 場所 | 中身 |
|---|---|
| `app/effector/` | **このエフェクタのプログラム**（自分たちのコード） |
| `01_要件定義/` `02_基本設計書/` `03_詳細設計書/` | 設計書 |
| `lib/` `include/` | Circle 本体。USB オーディオドライバを一部変更している（後述） |
| `sample/` `boot/` `doc/` ほか | Circle 本体のサンプル、起動ファイル、資料 |

### `app/effector/` のモジュール

基本設計書のモジュール分けに従う。

| ID | ファイル | 役割 | 状態 |
|---|---|---|---|
| M-00 | `kernel.cpp` `kernel.h` | 起動と初期化、メインループ、普段・測定状態の管理 | サンプル 42 を元に作成中 |
| M-05 | `audio_io` | AG03MK2 との音声の受け渡し（USB） | 未実装 |
| M-02 | `effect_chain` | 有効なエフェクトを順にかける | 未実装 |
| M-06 | `effects` | Fuzz、AutoWah、Reverb | 未実装 |
| M-01 | `controls` | ボタンの判定と LED の表示 | 未実装 |
| M-03 | `measure` | ループバックによる遅延の測定 | 未実装 |
| M-04 | `log` | ログ出力 | 未実装 |
| M-07 | `config.hpp` | 定数（サンプルレート、ブロックサイズなど） | 作成中 |

## Circle 本体への変更

Circle 51.1.1 の USB オーディオドライバは、24 ビットの音を 3 バイトの箱（サブスロット）で送る機器にしか対応していなかった。AG03MK2 は 24 ビットの音を 4 バイトの箱で送るため、そのままでは使えない。そこで次の変更を加えた。

- `cmdline.txt` のオプション `usbsoundsubslot=4` を追加した（`lib/koptions.cpp`、`include/circle/koptions.h`）
- 指定時に 4 バイトの箱を受け付け、24 ビットの値を上位詰めで送受信するようにした（`lib/usb/usbaudiostreaming.cpp`、`lib/sound/usbsoundbasedevice.cpp`）
- モニターなしで起動できるよう `SCREEN_HEADLESS` を有効にした（`include/circle/sysconfig.h`）

オプションを指定しない場合の動作は、元の Circle と同じである。

## ビルド

### 必要なもの

- Linux（Windows の場合は WSL 上の Ubuntu）
- `build-essential`、`git`
- Arm GNU Toolchain 15.2.Rel1（`aarch64-none-elf`）。PATH に `bin` フォルダを通しておく

### 手順

1. リポジトリの一番上に `Config.mk` を作る（git の管理外のファイルである）

   ```
   AARCH = 64
   RASPPI = 4
   PREFIX64 = aarch64-none-elf-
   ```

2. Circle 本体をビルドする

   ```bash
   ./makeall clean && ./makeall
   ```

3. Pi の起動ファイルを取得する（初回のみ。インターネットに接続が必要）

   ```bash
   cd boot && make && cd ..
   ```

4. エフェクタをビルドする

   ```bash
   cd app/effector && make
   ```

   `kernel8-rpi4.img` ができれば成功である。

## SD カードの作成

FAT32 でフォーマットした microSD カードの一番上に、次の 6 つのファイルを置く。

| ファイル | 取得元 |
|---|---|
| `start4.elf` `fixup4.dat` `bcm2711-rpi-4-b.dtb` | `boot/` |
| `config.txt` | `boot/config64.txt` の名前を変えたもの |
| `kernel8-rpi4.img` | `app/effector/` |
| `cmdline.txt` | 自分で作る（下記） |

`cmdline.txt` は 1 行のファイルで、中身は次のとおりである。

```
logdev=ttyS1 soundopt=24 usbsoundsubslot=4
```

| オプション | 意味 |
|---|---|
| `logdev=ttyS1` | ログをシリアル（GPIO14/15）に出す |
| `soundopt=24` | USB オーディオを 24 ビットで使う |
| `usbsoundsubslot=4` | 24 ビットの音を 4 バイトの箱で受け渡す（AG03MK2 に必要） |

## 動かす

1. AG03MK2 の LOOPBACK スイッチを OFF にし、Pi の USB ポートにつなぐ
2. ヘッドホンを AG03MK2 につなぐ。最初は音量を最小にしておく
3. ログを見る場合は、USB シリアル変換ケーブル（3.3V）を Pi のピン 6（GND）、ピン 8（TXD）、ピン 10（RXD）につなぎ、Tera Term を 115200bps で開いておく
4. SD カードを挿し、最後に電源をつなぐ

## チーム

| 担当 | 内容 |
|---|---|
| （氏名） | Circle・カーネル側、全体設計 |
| （氏名） | Circle・カーネル側 |
| （氏名） | エフェクト |
| （氏名） | エフェクト |

## ライセンス

GPL-3.0。Circle（Copyright (C) R. Stange ほか）を元にしている。Circle 本体の説明は [`README_circle.md`](README_circle.md) を参照。
